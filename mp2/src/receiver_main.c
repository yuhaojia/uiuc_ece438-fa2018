/*
 * File:   receiver_main.c
 * Author:
 *
 * Created on
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>

#define MSS 1000
#define RWND 20000
#define DATA 0
#define SYN 1
#define SYN_ACK 2
#define ACK 3
#define FIN 4
#define FIN_ACK 5
#define MAXSEQUENCE 150000

typedef struct{
    int     data_size;
    int     seq_num;
    int   ack_num;
    int   msg_type; //SYN 0 ACK 1 FIN 2 FINACK 3
    char  data[MSS];
}packet_t;

#define CLOSED 0
#define LISTEN 1
#define SYN_RCVD 2
#define ESTABLISHED 3
#define CLOSE_WAIT 4
#define LAST_ACK 5
#define MAXBUFSIZE 10000
#define HEADERSIZE 16

socklen_t addr_len;
FILE* fp;
char buf[sizeof(packet_t)];
struct sockaddr_in si_me, si_other;
int s, slen;
int nextACK = 0;
char file_buffer[MAXBUFSIZE];
int bufferindex = 0;
int  receive_window[RWND];
packet_t window_buffer[RWND];



void diep(char *s) {
    perror(s);
    exit(1);
}

void manage(packet_t pkt){
    if(pkt.seq_num == nextACK){// in order packet comes
        int i;
        for(i =0; i < pkt.data_size;i++){//load data into file buffer
            file_buffer[bufferindex] = pkt.data[i];
            bufferindex++;
            if(bufferindex==MAXBUFSIZE){
                // write to file
                fwrite(file_buffer,sizeof(char),MAXBUFSIZE,fp);
                bufferindex=0;
            }
        }
        nextACK = (nextACK+1) % MAXSEQUENCE;
        
        ///////////////////////////////////////////////////////////////////////////
        while(receive_window[nextACK % RWND] != 0 ){
            receive_window[nextACK % RWND]=0;
            for(i=0;i<window_buffer[nextACK % RWND].data_size;i++){
                file_buffer[bufferindex] = window_buffer[nextACK % RWND].data[i];
                bufferindex++;
                if(bufferindex==MAXBUFSIZE){
                    // write to file
                    fwrite(file_buffer,sizeof(char),MAXBUFSIZE,fp);
                    bufferindex=0;
                }
            }
            nextACK=(nextACK+1) % MAXSEQUENCE;
        }// end for while loop for ......
        //send ack
        packet_t ack;
        ack.msg_type=ACK;
        ack.ack_num = nextACK;
        ack.data_size = 0; // data size is 0 since we are sending ack
        memcpy(buf,&ack,sizeof(packet_t));
        sendto(s, buf, sizeof(packet_t), 0, (struct sockaddr *) &si_other,addr_len);
    }// end if for in order packet comes
    else if(pkt.seq_num > nextACK){ //out of order packet comes send dupack
        printf("out of order! send dup ack %d",pkt.seq_num);
        if (receive_window[pkt.seq_num % RWND]==0){
            receive_window[pkt.seq_num % RWND]=1;
            memcpy(&window_buffer[pkt.seq_num % RWND], &pkt, sizeof(packet_t));
        }
        //send ack
        packet_t ack;
        ack.msg_type=ACK;
        ack.ack_num = nextACK;
        ack.data_size = 0; // data size is 0 since we are sending ack
        memcpy(buf,&ack,sizeof(packet_t));
        sendto(s, buf, sizeof(packet_t), 0, (struct sockaddr *) &si_other,addr_len);
    }else{ // the missing packet comes
        packet_t ack;
        ack.msg_type=ACK;
        ack.ack_num = nextACK;
        ack.data_size = 0; // data size is 0 since we are sending ack
        memcpy(buf,&ack,sizeof(packet_t));
        //send ack
        sendto(s, buf, sizeof(packet_t), 0, (struct sockaddr *) &si_other,addr_len);
    }
    return;
}



void reliablyReceive(unsigned short int myUDPport, char* destinationFile) {
    // open up a socket
    slen = sizeof (si_other);
    if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
        diep("socket");
    memset((char *) &si_me, 0, sizeof (si_me));
    si_me.sin_family = AF_INET;
    si_me.sin_port = htons(myUDPport);
    si_me.sin_addr.s_addr = htonl(INADDR_ANY);
    printf("Now binding\n");
    if (bind(s, (struct sockaddr*) &si_me, sizeof (si_me)) == -1)
        diep("bind");
    
    //prepare destination file
    fp = fopen(destinationFile,"wb");
    // open file for read and write
    if(fp == NULL){
        diep("file open");
        return;
    }
    
    //receive packets
    int numbytes = 0;
    addr_len = sizeof si_other;
    while(1){
        if((numbytes = recvfrom(s,buf,sizeof(packet_t),0,(struct sockaddr *) &si_other,&addr_len))==-1){
            diep("receive data");
            exit(2);
        }
        packet_t pkt;
        memcpy(&pkt,buf,sizeof(packet_t));
        //determine packet received
        if(pkt.msg_type == DATA){//it is a regular packet
            manage(pkt);
            continue;
        }else if(pkt.msg_type == FIN){//receive last signal, send lastask close the socket
            fwrite(file_buffer,sizeof(char),bufferindex,fp);
            while(1){
                packet_t pkt;
                pkt.msg_type = FIN_ACK;
                pkt.data_size=0;
                memcpy(buf,&pkt,sizeof(packet_t));
                if((numbytes = sendto(s, buf, sizeof(packet_t), 0, (struct sockaddr *) &si_other,addr_len))== -1){
                    diep("send final ack");
                    exit(2);
                }
                break;
            }
            break;
        }//receive signal for final packet sent
        
    }//end of big while
    
    close(s);
    fclose(fp);
    printf("%s received.", destinationFile);
    return;
}

/*
 *
 */
int main(int argc, char** argv) {
    
    unsigned short int udpPort;
    
    if (argc != 3) {
        fprintf(stderr, "usage: %s UDP_port filename_to_write\n\n", argv[0]);
        exit(1);
    }
    
    udpPort = (unsigned short int) atoi(argv[1]);
    
    reliablyReceive(udpPort, argv[2]);
}
