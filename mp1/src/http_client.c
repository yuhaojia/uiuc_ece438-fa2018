/*
 ** client.c -- a stream socket client demo
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <arpa/inet.h>

#define PORT "80" // http only accepts ports 80

#define MAXDATASIZE 3000 // max number of bytes we can get at once

// Error checking
#define PREFIX_S "http://"
#define PREFIX_H_S "localhost"
#define ERROR -1
#define TRUE 1
#define FALSE 0

// Default values
#define DEFAULT_PORT "80"
#define MAXDATASIZE 3000  // max bytes for communication

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char *argv[])
{
    int sockfd, numbytes;
    static char wget_buf[MAXDATASIZE];
    struct addrinfo hints, *servinfo, *p;
    int rv;
    char s[INET6_ADDRSTRLEN];
    static int httpstarter = TRUE;
    static int httpstarterlocal = FALSE;
    static int httpstarternum = TRUE;
    char recvingbuf[MAXDATASIZE];

    if (argc != 2) {
        fprintf(stderr,"usage: client hostname\n");
        exit(1);
    }
    printf("%s\n",argv[1]);
    if(strncmp(argv[1], PREFIX_S, strlen(PREFIX_S)) != 0){
    	printf("NO http:// as a start\n");
    	httpstarter = FALSE;
    }



    //parse the command line URL
    char hostname[50];
    char port[4];
    char identifier[50];
    int pro = 0;
    int i,j,u;
    int k =0;
    static int portdigit = 0;
    int httpstarterlength = 7;
    if(httpstarter == FALSE){
    	httpstarterlength = 0;
    }
    //printf("%c\n",argv[1][6]);
    for(i = httpstarterlength; i < strlen(argv[1]);i++){
      if(argv[1][i] == ':'){
      	for(j = i; (j<strlen(argv[1]))&&(argv[1][j] != '/');j++){
      		portdigit++;
      		//if(argv[1][i] == '/')
      		//	break;
      		//}
      	}
      	portdigit--;
      	printf("the port have %d digit\n", portdigit);
        pro = -2;
        for(j =httpstarterlength; j < i;j++){
          hostname[j-httpstarterlength] = argv[1][j];
          printf("host:");
          printf("%c\n",hostname[j-httpstarterlength]);
         }
        hostname[i] = '\0';
      }
      if(pro == -2){
        for(u = 0; u<portdigit;u++){
          port[u] = argv[1][i+u+1];
          printf("port");
          printf("%d\n",port[u]);
        }
        //if(portdigit<4){
        //	port[portdigit]='\0';
        //}
        pro =-1;
      }
      if(pro == -1 && i+4 <strlen(argv[1])){
	
        identifier[k] = argv[1][i+4];
        printf("identifier");
        printf("%c\n",identifier[k]);
        k++;

      }

      if(argv[1][i] == '/' && pro == 0){
        port[0] = '8';
        port[1] = '0';
        port[2] = '\0';
        //port[3] = '\0';
        printf("port");
        printf("%c\n",port[0]);
        printf("port");
        printf("%c\n",port[1]);
        pro = 1;
        for(j =httpstarterlength; j < i;j++){
          hostname[j-httpstarterlength] = argv[1][j];
          printf("host/");
          printf("%c\n",hostname[j-httpstarterlength]);
        }
        hostname[i] = '\0';
      }
      if(pro == 1){
        identifier[k] = argv[1][i];
        printf("identifier");
        printf("%c\n",identifier[k]);
        k++;
      }

      }

      // format the information about the HTTP GET that wget uses
      //char wget_buf[MAXDATASIZE];
      //char wget_buf_tail[100];
      printf("%s\n", port);
      strcat(wget_buf,"GET ");
      strcat(wget_buf,identifier);
      strcat(wget_buf," HTTP/1.1\r\n");
      strcat(wget_buf,"User-Agent:  Wget/1.12 (linux-gnu)\r\n");
      strcat(wget_buf,"Host: ");
      //printf("hostname:%s\n",hostname);
      if(strncmp(hostname, PREFIX_H_S, strlen(PREFIX_H_S)) == 0){
    	printf("localhost\n");
    	//strcat(wget_buf,"localhost");
    	httpstarterlocal = TRUE;
      }
      for(i = 0; i < strlen(hostname); i++){
      	if(((hostname[i]-'0'>9)||(hostname[i]-'0'<0))&&(hostname[i]!='.')){
      		httpstarternum=FALSE;
      		//printf("aaaaaa\n" );
      	}
      }
      
      if(httpstarter && (httpstarterlocal==FALSE) && (httpstarternum==FALSE)){
      	//printf("www.\n");
      	strcat(wget_buf,"www.");
      }

      strcat(wget_buf,hostname);
      // condition on port number 2digit or 4 digit 
      char portb = port;
      //printf("%d\n",portdigit);
      //printf("%d",port[1]);
      if (portdigit!=0){
      	strcat(wget_buf,":");
      	strcat(wget_buf,port);
      	/*for(u = 0; u<portdigit;u++){
      		printf(":%d\n",port[u]);
      		strcat(wget_buf,port);
      	}*/
      }
      strcat(wget_buf,"\r\n");
      strcat(wget_buf,"Connection: Keep-Alive\r\n\r\n");
      //printf("hahahha.\n");
      printf("%s",wget_buf);
      printf("\n");
      //strcat(wget_buf_tail,"Connection: Keep-Alive\n");
      //printf("%s\n",wget_buf_tail);

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo(hostname, port, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and connect to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                             p->ai_protocol)) == -1) {
            perror("client: socket");
            continue;
        }

        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("client: connect");
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "client: failed to connect\n");
        return 2;
    }
    //printf("hhhhhh1\n");
    send(sockfd, wget_buf,sizeof(wget_buf),0);

    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
              s, sizeof s);
    printf("client: connecting to %s\n", s);

    freeaddrinfo(servinfo); // all done with this structure
    FILE *fp;
	fp=fopen("output","a");
  /*do{
      numbytes =recv(sockfd, recvingbuf, MAXDATASIZE-1, 0);
      if (numbytes > 0){
        i = fwrite(recvingbuf, 1, numbytes, fp);
        recvingbuf[numbytes] = '\0';
        //printf("%s",buf);
    }
  }while(numbytes > 0);*/
    while(1){
      

		if((numbytes=recv(sockfd,recvingbuf,MAXDATASIZE-1,0))>0){
			fprintf(fp,"%s",recvingbuf);
			printf("%s",recvingbuf);
			printf("num in line: %d\n",numbytes);
		}
		else {fclose(fp);break;}
	}


	close(sockfd);

    return 0;
}
