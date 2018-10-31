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

typedef struct{
    int     data_size;
    int     seq_num;
    int     ack_num;
    int     msg_type; //SYN 0 ACK 1 FIN 2 FINACK 3
    char data[MSS];
}packet;

//struct socketaddr_in bind_addr, receiver_addr;
struct sockaddr_storage other_addr; // connector's address information
socklen_t addr_len = sizeof other_addr;
struct addrinfo hints, *recvinfo, *p;

FILE *fp;

// For congestion control
double cwnd = 1;
int ssthread = 8, dupACK = 0;
int send_base = 0, next_send = 0, file_pt = 0;
int send_base_acked = 0, global_seq_num = 0;

// Timing related
int timeout = 60000;
//estimatedRTT = 20000, deviation = ;
//uint64_t start_time;

// Tmp parameters or flags
ssize_t numofbytes;
int file_read_finished = 0;
unsigned long long int bytesReadin;
unsigned long long int num_pkt_sent = 0, num_pkt_received = 0, num_pkt_total = 0;

// Sliding window related
packet window_buffer[SWND];
//int64_t sent_time[SWND];
char buf[sizeof(packet)];
int soc_state = SLOW_START;

void diep(char *s) {
    perror(s);
    exit(1);
}
/*uint64_t timeNow() {
    struct timeval current;
    gettimeofday(&current, 0);
    return (uint64_t)(current.tv_sec * 1000000 + current.tv_usec);
}
int64_t procTimeNow() {
    struct timeval current1;
    gettimeofday(&current1, 0);
    uint64_t timeNow = (uint64_t)(current1.tv_sec * 1000000 + current1.tv_usec);
    return (int64_t) (timeNow - start_time);
}*/

int createSocket(char * hostname, unsigned short int hostUDPport)
{
    int sockfd;
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
        return 1;
    }

    // loop through all the results and connect to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("sender: error opening socket");
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "sender: failed to bind socket\n");
        exit(1);
    }
    return sockfd;
}

int fillingWindow(int num_pkt) {
    if (num_pkt == 0) { return 0; }    
    int counter = 0, byte_trans = 0;
    char file_buffer[MSS+1];
    packet pkt;
    for (counter = 0; bytesReadin && counter < num_pkt; ++ counter) {
        /*if (bytesReadin < (unsigned long long int) MSS) {
            byte_trans_once = (int) bytesReadin;
        } else {
            byte_trans_once = MSS;
        }*/
        byte_trans = (int) min(bytesReadin, (unsigned long long int) MSS);
        int read_size = fread (file_buffer, sizeof(char), byte_trans, fp);
        if (read_size > 0) {
            pkt.data_size = read_size;
            pkt.seq_num = global_seq_num;
            pkt.msg_type = DATA;
            memcpy(pkt.data, &file_buffer, sizeof(char) * byte_trans);
            memcpy(&window_buffer[file_pt], &pkt, sizeof(packet));
            file_pt = (file_pt + 1) % SWND;
            global_seq_num = (global_seq_num + 1) % MAX_SEQ;
        }
        bytesReadin -= read_size;
    }

    //cout << "Filling the window by size " << num_pkt << endl;
    return counter;
}


int sendPackets (int sockfd) {
    if (num_pkt_sent == num_pkt_total) {
        printf("No packet to send");
        return 0;
    }

    int last_pk = send_base + (int)cwnd;
    if (next_send < send_base) {
        next_send += SWND;
    }
    if ((long long int) (num_pkt_total - num_pkt_sent) < last_pk - next_send ) {
        printf("With %llu remaining. Initially: %d\n", num_pkt_total - num_pkt_sent, last_pk - next_send);
        last_pk = next_send + num_pkt_total - num_pkt_sent;
    }

    printf("base: %d, cwnd: %f, base+cwnd: %d, next send: %d\n", send_base, cwnd, last_pk, next_send);
    if (last_pk <= next_send) {
        printf("CWND is too small to send.\n");
        //cout << "CWND is too small to send." << endl;
    } else {
        //sendMultiPackets(sockfd, next_send, last_pk);
        for (int i = next_send; i < last_pk; ++i) {
            int i_pos_in_swnd = i % SWND;
            memcpy(buf, &window_buffer[i_pos_in_swnd], sizeof(packet));
            if((numofbytes = sendto(sockfd, buf, sizeof(packet), 0, p->ai_addr, p->ai_addrlen))== -1){
                printf("Fail to send %d pkt in SWND", i_pos_in_swnd);
                perror("Error: data sending");
                exit(2);
            }
            //cout << "-----------Sending packet " << window_buffer[i_pos_in_swnd].seq_num << "---------------" << endl;
            //sent_time[i_pos_in_swnd] = procTimeNow();
        }
        num_pkt_sent += (last_pk - next_send) % SWND;

    }

    next_send = max(next_send, last_pk) % SWND;
    //cout << "send_base: " << send_base << " next_send: " << next_send << endl;
    return 0;
}

int manage (packet pkt, int sockfd) {
    //cout << "~~~~ Receiving packet " << pkt.ack_num << " ~~~~" << endl;
    int ack_pos_in_swnd = pkt.ack_num % SWND;
    if ((send_base < next_send && (ack_pos_in_swnd > next_send || ack_pos_in_swnd < send_base)) ||
        (send_base > ack_pos_in_swnd && ack_pos_in_swnd > next_send )) {
        perror("Out of order ACK");
        return -1;
    }

    if ((ack_pos_in_swnd > send_base) ||
            (next_send < send_base && ack_pos_in_swnd <= next_send)) {
        num_pkt_received += (pkt.ack_num - send_base) % SWND;
        //cout << "Received a new ACK with " << pkt.ack_num << endl;
        fillingWindow ((pkt.ack_num - send_base) % SWND);
        send_base = ack_pos_in_swnd;
        // new ACK
        if (soc_state == FAST_RECOVERY) {
            cwnd = ssthread * 1.0;
            dupACK = 0;
            soc_state = CONGESTION_AVOID;
        }

        //cout << "Total Received pkt: " << num_pkt_received << endl;
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
                perror("Wrong socket status");
                return -1;
        }
    } else if (ack_pos_in_swnd == send_base) { // dup ACK
        //printSWNDseq();
        if (soc_state == SLOW_START || soc_state == CONGESTION_AVOID) {
            ++dupACK;
            // dupACK to 3
            if (dupACK >= 3) {
                ssthread = (int) cwnd / 2;
                cwnd = ssthread + 3.0;
                //sendSinglePacket(sockfd, send_base);

                memcpy(buf, &window_buffer[send_base], sizeof(packet));
                if((numofbytes = sendto(sockfd, buf, sizeof(packet), 0, p->ai_addr, p->ai_addrlen))== -1){
                    printf("Fail to send %d pkt", send_base);
                    perror("Error: data sending (single)");
                    exit(2);
                }
                //cout << "-----------Sending single packet " << window_buffer[pt].seq_num << "---------------" << endl;
                //sent_time[pt] = procTimeNow();

                soc_state = FAST_RECOVERY;
            }
        } else { cwnd = (cwnd + 1 < SWND) ? cwnd + 1 : SWND - 1; }
    } else {
        perror("Invalid ACK packet");
    }

    // cwnd > ssthread
    if (soc_state == SLOW_START && cwnd > ssthread) {
        soc_state = CONGESTION_AVOID;
    }

//    updateTimeout(sent_time[ack_pos_in_swnd]);
    sendPackets(sockfd);

    return 0;
}

void reliablyTransfer(char* hostname, unsigned short int hostUDPport, char* filename, unsigned long long int bytesToTransfer) {
    //Open the file
    bytesReadin = bytesToTransfer;
    //FILE *fp;
    fp = fopen(filename, "rb");
    if (fp == NULL) {
        printf("Could not open file to send.");
        exit(1);
    }
    //num_pkt_total = (unsigned long long int) ceil(bytesToRead * 1.0 / MSS);
    //cout << num_pkt_total << endl;

    if (fillingWindow(SWND) == -1) {
        perror("Error during filling window");
    }

    int sockfd = createSocket(hostname, hostUDPport);
    /*struct timeval current;
    gettimeofday(&current, 0);
    start_time = (uint64_t)(current.tv_sec * 1000000 + current.tv_usec);*/

    struct timeval tv;
    tv.tv_sec = 0; tv.tv_usec = timeout;
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        perror("sender: setsockopt");
    }
    printf("Setting timeout to %d\n", timeout);

    sendPackets(sockfd);
    while (num_pkt_sent < num_pkt_total ||
           num_pkt_received < num_pkt_sent) {
        // Wait for ack
        if((numofbytes = recvfrom(sockfd, buf, sizeof(packet), 0, NULL, NULL)) == -1) {
            if (errno != EAGAIN || errno != EWOULDBLOCK) {
                perror("can not receive main ack");
                exit(2);
            }
            // Timeout
            memcpy(buf, &window_buffer[send_base], sizeof(packet));
            if((numofbytes = sendto(sockfd, buf, sizeof(packet), 0, p->ai_addr, p->ai_addrlen))== -1){
                perror("Error: timeout sending");
                printf("Fail to send %d pkt in SWND", send_base);
                exit(2);
            }
            printf("---------- Timeout resending packet %d ----------\n", window_buffer[send_base].seq_num);
            //printSentTime();
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
            numofbytes = sendto(sockfd, buf, sizeof(packet), 0, p->ai_addr, p->ai_addrlen);
            packet ack;
            numofbytes = recvfrom(sockfd, buf, sizeof(packet), 0, (struct sockaddr *) &other_addr, &addr_len);
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


