/*
 * File:   sender_main.c
 * Author:
 *
 * Created on
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <sys/time.h>
#include <errno.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/stat.h>
#include <signal.h>
#include <string.h>
#include <sys/time.h>
#include <math.h>

enum socket_state {CLOSED, SLOW_START, CONGESTION_AVOID, FAST_RECOVERY, FIN_WAIT};

#define MSS 1000
#define SWND 150
#define DATA 0
#define SYN 1
#define SYN_ACK 2
#define ACK 3
#define FIN 4
#define FIN_ACK 5
#define MAX_SEQ 150000

#define min(a, b) a < b ? a : b
#define max(a, b) a > b ? a : b

int sockfd;

typedef struct{
    int     data_size;
    int     seq_num;
    int     ack_num;
    int     msg_type; 
    //SYN 0 ACK 1 FIN 2 FINACK 3
    char data[MSS];
}packet;

struct addrinfo hints, *recvinfo, *p;
struct sockaddr_storage other_addr;
socklen_t addr_len = sizeof other_addr;

FILE *fp;

double cwnd = 1;
int ssthread = 8, dupACK = 0;
int sendBase = 0, sendNext = 0, windowIndex = 0, SequenceNum = 0;

int timeout = 100000;
ssize_t bytesNum;
unsigned long long int bytesReadin;
unsigned long long int package_sent = 0, package_received = 0, totalPackageNum = 0;

packet window_buffer[SWND];
char buf[sizeof(packet)];
int soc_state = SLOW_START;

void diep(char *s) {
    perror(s);
    exit(1);
}

int fillingWindow(int packageNum) {
    char file_buffer[MSS+1];
    int counter = 0, byte_trans = 0;
    if (packageNum == 0) { return 0; }
    packet pkt;
    for (counter = 0; bytesReadin && counter < packageNum; ++ counter) {
        byte_trans = (int) min(bytesReadin, (unsigned long long int) MSS);
        int readSize = fread (file_buffer, sizeof(char), byte_trans, fp);
        if (readSize > 0) {
            pkt.data_size = readSize;
            pkt.seq_num = SequenceNum;
            pkt.msg_type = DATA;
            memcpy(pkt.data, &file_buffer, sizeof(char) * byte_trans);
            memcpy(&window_buffer[windowIndex], &pkt, sizeof(packet));
            windowIndex = (windowIndex + 1) % SWND;
            SequenceNum = (SequenceNum + 1) % MAX_SEQ;
        }
        bytesReadin -= readSize;
    }
    
    return counter;
}


int sendPackets (int sockfd) {
    int last_pkg_num = sendBase + (int)cwnd;
    if (package_sent == totalPackageNum) {
        printf("There is not any packet to send!!!");
        return 0;
    }
    if (sendNext < sendBase) {
        sendNext += SWND;
    }
    //Go to the end og files
    if ((long long int) (totalPackageNum - package_sent) < last_pkg_num - sendNext ) {
        printf("There are still %llu remaining, From %d\n", totalPackageNum - package_sent, last_pkg_num - sendNext);
        last_pkg_num = sendNext + totalPackageNum - package_sent;
    }
    printf("base: %d, cwnd: %f, base+cwnd: %d, next send: %d\n", sendBase, cwnd, last_pkg_num, sendNext);
    if (last_pkg_num <= sendNext) {
        printf("CWND is too small to send.\n");
    } else {
        //sendMultiPackets, 1: sockfd, 2: sendNext, 3: last_pkg_num;
        for (int i = sendNext; i < last_pkg_num; ++i) {
            int iInSWND = i % SWND;
            memcpy(buf, &window_buffer[iInSWND], sizeof(packet));
            if((bytesNum = sendto(sockfd, buf, sizeof(packet), 0, p->ai_addr, p->ai_addrlen))== -1){
                perror("Error: Fail when sending data");
                exit(2);
            }
            printf("-----------Sending packet------------\n");
        }
        package_sent += (last_pkg_num - sendNext) % SWND;
    }
    sendNext = max(sendNext, last_pkg_num) % SWND;
    return 0;
}

int manage (packet pkt, int sockfd) {
    int ackNum = pkt.ack_num % SWND;
    if ((sendBase < sendNext && (ackNum > sendNext || ackNum < sendBase)) || (sendBase > ackNum && ackNum > sendNext )) {
        perror("Out of order ACK!!!!!");
        return -1;
    }
    printf("-------Receiving ACK packet : %d\n", pkt.ack_num);
    if ((ackNum > sendBase) || (sendNext < sendBase && ackNum <= sendNext)) {
        package_received += (pkt.ack_num - sendBase) % SWND;
        printf("-------Received a new ACK packet : %d\n", pkt.ack_num);
        fillingWindow ((pkt.ack_num - sendBase) % SWND);
        sendBase = ackNum;
        // new ACK
        if (soc_state == FAST_RECOVERY) {
            cwnd = ssthread * 1.0;
            dupACK = 0;
            soc_state = CONGESTION_AVOID;
        }
        printf("-------Total Received %llu packages\n", package_received);
        switch (soc_state) {
            case SLOW_START:
                cwnd = (cwnd + 1 < SWND) ? cwnd + 1 : SWND - 1;
                dupACK = 0;
                break;
            case CONGESTION_AVOID:
                cwnd = (cwnd + 1.0/cwnd < SWND) ? cwnd + 1.0 / cwnd : SWND - 1;
                dupACK = 0;
                break;
            case FAST_RECOVERY:
                cwnd = ssthread;
                dupACK = 0;
                soc_state = CONGESTION_AVOID;
                break;
            default:
                perror("The socket status is Wrong!!!!!");
                return -1;
        }
    } else if (ackNum == sendBase) { // Dup ACK
        if (soc_state == SLOW_START || soc_state == CONGESTION_AVOID) {
            ++dupACK;
            if (dupACK >= 3) {//Dup is 3!!!!!
                ssthread = (int) cwnd / 2;
                cwnd = ssthread + 3.0;
                memcpy(buf, &window_buffer[sendBase], sizeof(packet));
                if((bytesNum = sendto(sockfd, buf, sizeof(packet), 0, p->ai_addr, p->ai_addrlen))== -1){
                    printf("Fail to send %d pkt", sendBase);
                    perror("Error: data sending (single)");
                    exit(2);
                }
                soc_state = FAST_RECOVERY;
            }
        } else { cwnd = (cwnd + 1 < SWND) ? cwnd + 1 : SWND - 1; }
    } else {
        perror("Sender got an invalid ACK packet!!!!!!");
    }
    if (soc_state == SLOW_START && cwnd > ssthread) {
        soc_state = CONGESTION_AVOID;
    }
    //We did not update the timeout value!!!!!!
    sendPackets(sockfd);
    return 0;
}

void reliablyTransfer(char* hostname, unsigned short int hostUDPport, char* filename, unsigned long long int bytesToTransfer) {
    //Open the file
    bytesReadin = bytesToTransfer;
    fp = fopen(filename, "rb");
    if (fp == NULL) {
        printf("Could not open the file!!!!!!");
        exit(1);
    }
    
    totalPackageNum = (unsigned long long int) ((bytesReadin-1) * 1.0 / MSS + 1);
    
    if (fillingWindow(SWND) == -1) {
        perror("Error during the first attempt to filling sender's window");
    }
    
    //int sockfd = createSocket(hostname, hostUDPport);
    int rv;
    struct addrinfo hints, *servinfo;
    char portStr[10];
    memset(&hints, 0, sizeof hints);
    sprintf(portStr, "%d", hostUDPport);
    memset(&hints,0,sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    memset(&servinfo,0,sizeof servinfo);
    if ((rv = getaddrinfo(hostname, portStr, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return;
    }

    // loop through all the results and connect to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                             p->ai_protocol)) == -1) {
            perror("Error: opening socket");
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "Error: Failed to bind socket\n");
        exit(1);
    }
    
    struct timeval tv;
    tv.tv_sec = 0; tv.tv_usec = timeout;
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        perror("Sender Error: setsockopt");
    }
    printf("Timeout is set to %d\n", timeout);
    
    sendPackets(sockfd);
    while (package_sent < totalPackageNum || package_received < package_sent) {
        if((bytesNum = recvfrom(sockfd, buf, sizeof(packet), 0, NULL, NULL)) == -1) {
            if (errno != EAGAIN || errno != EWOULDBLOCK) {
                perror("Error: Can not receive ACK number");
                exit(2);
            }
            memcpy(buf, &window_buffer[sendBase], sizeof(packet));
            if((bytesNum = sendto(sockfd, buf, sizeof(packet), 0, p->ai_addr, p->ai_addrlen))== -1){
                perror("Error: Fail when doing timeout sending");
                exit(2);
            }
            printf("---------- Timeout resending packet %d ----------\n", window_buffer[sendBase].seq_num);
            ssthread = cwnd/2; cwnd = 1.0; dupACK = 0;
        }
        packet pkt;
        memcpy(&pkt, buf, sizeof(packet));
        if(pkt.msg_type == ACK) {
            manage(pkt, sockfd);
            continue;
        }
    }
    
    fclose(fp);
    
    packet pktend;
    while (1) {
        pktend.msg_type = FIN;
        pktend.data_size=0;
        memcpy(buf, &pktend, sizeof(packet));
        bytesNum = sendto(sockfd, buf, sizeof(packet), 0, p->ai_addr, p->ai_addrlen);
        packet ack;
        bytesNum = recvfrom(sockfd, buf, sizeof(packet), 0, (struct sockaddr *) &other_addr, &addr_len);
        memcpy(&ack, buf, sizeof(packet));
        if (ack.msg_type == FIN_ACK) {
            printf("Receive the FIN_ACK.\n");
            soc_state = FIN_WAIT;
            break;
        }
    }
    
}

/*
 *
 */
int main(int argc, char** argv) {
    
    unsigned short int udpPort;
    unsigned long long int numBytes;
    
    if (argc != 5) {
        fprintf(stderr, "usage: %s receiver_hostname receiver_port filename_to_xfer bytes_to_xfer\n\n", argv[0]);
        exit(1);
    }
    udpPort = (unsigned short int) atoi(argv[2]);
    numBytes = atoll(argv[4]);
    
    
    
    reliablyTransfer(argv[1], udpPort, argv[3], numBytes);
    
    
    return (EXIT_SUCCESS);
}

