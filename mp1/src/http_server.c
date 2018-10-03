/*
** server.c -- a stream socket server demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#define PORT "80"  // the port users will be connecting to

#define MAXIMUMDATA 2980
#define MAXDATASIZE 3000

#define BACKLOG 10	 // how many pending connections queue will hold

#define GET_S "GET /"
#define HTTP_0 "HTTP/1.0"
#define HTTP_1 "HTTP/1.1"
#define RESP_200 "HTTP/1.1 200 OK\r\n\r\n"
#define RESP_404 "HTTP/1.1 404 Not Found\r\n\r\nError: 404\nWhoops, file not found!\n"
#define RESP_400 "HTTP/1.1 400 Bad Request\r\n\r\nError: 400\nBad Request\n"
#define TWO_CRLF "\r\n\r\n"

void sigchld_handler(int s)
{
	while(waitpid(-1, NULL, WNOHANG) > 0);
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc,char*argv[])
{
	int sockfd, num_bytes;  // listen on sock_fd, new connection on new_fd
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_storage their_addr; // connector's address information
	socklen_t sin_size;
	struct sigaction sa;
	int yes=1;
	char s[INET6_ADDRSTRLEN];
	int rv;
	int recvsize;
	char serverbuff[MAXIMUMDATA];

	if(argc!=2){fprintf(stderr,"host port");exit(1);}

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; // use my IP

	if ((rv = getaddrinfo(NULL, argv[1], &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("server: socket");
			continue;
		}

		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
				sizeof(int)) == -1) {
			perror("setsockopt");
			exit(1);
		}

		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("server: bind");
			continue;
		}

		break;
	}

	if (p == NULL)  {
		fprintf(stderr, "server: failed to bind\n");
		return 2;
	}

	freeaddrinfo(servinfo); // all done with this structure

	if (listen(sockfd, BACKLOG) == -1) {
		perror("listen");
		exit(1);
	}

	sa.sa_handler = sigchld_handler; // reap all dead processes
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		perror("sigaction");
		exit(1);
	}

	printf("server: waiting for connections...\n");

	while(1) {  // main accept() loop
		//printf("hello");
		int new_fd;
		sin_size = sizeof their_addr;
		new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
		if (new_fd == -1) {
			perror("accept");
			continue;
		}

		inet_ntop(their_addr.ss_family,
			get_in_addr((struct sockaddr *)&their_addr),
			s, sizeof s);
		printf("server: got connection from %s\n", s);

		if (!fork()) { // this is the child process
			close(sockfd); // child doesn't need the listener
			recvsize=recv(new_fd,serverbuff,MAXDATASIZE-1,0);
			serverbuff[recvsize] = '\0';
			printf("%s\n",serverbuff);

			char buffbuff[1000];
			char buffbuff2[1000];
			int index=0;
			int index2=0;
			int numspace=0;
			int numline=0;
			//char buffbuff[MAXDATASIZE];

			if(serverbuff[0]!='G'||serverbuff[1]!='E'||serverbuff[2]!='T'||serverbuff[3]!=' '||serverbuff[4]!='/')
			{
				printf("The three first chars in server buff are %c %c %c %c %c, but it should be GET /",serverbuff[0],serverbuff[1],serverbuff[2],serverbuff[3],serverbuff[4]);
				if((num_bytes = send(new_fd, RESP_400, strlen(RESP_400), 0)) == -1)
				{
					perror("send");
					exit(1);
				}
				exit(1);
			}	

			/////////
			for(int i=0;i<strlen(serverbuff);++i){
			  if(serverbuff[i]==' '){
			  	numspace++;
			  	continue;
			  }
			  if(serverbuff[i]=='\r'){
			  	numline++;numspace=0;
			  	continue;
			  }
			  if(numspace==1&&numline==0){
				if(serverbuff[i]=='\r'||serverbuff[i]==' ')
					break;
				buffbuff[index]=serverbuff[i];
				index++;
			  }
			  if(numspace==1&&numline==2){
				if(serverbuff[i]=='\r'||serverbuff[i]==' ')
					break;
				buffbuff2[index2]=serverbuff[i];
				index2++;
			  }
			}

			//char *extracts=buffbuff;
			//char *extracts2=buffbuff2;
			printf("URL: %s\n",buffbuff);
			printf("Hostname: %s\n",buffbuff2);
			strcat(buffbuff2,buffbuff);

			//But we only need URL as extracts;
			int numsent;
			char URLpath[100];
			char line[500];
			char filebuff[MAXDATASIZE];
			strncpy(URLpath,buffbuff+1,strlen(buffbuff));
			printf("%s\n",URLpath);
			FILE*pfile=fopen(URLpath,"r");
			//char filebuff[MAXIMUMDATA];
			if(pfile==NULL){
			  printf(RESP_404);
			  send(new_fd,RESP_404,strlen(RESP_404),0);
			fclose(pfile);
			  close(new_fd);
exit(0);
			}
			


			else{
			memset(filebuff,'\0',MAXDATASIZE);
			//send(new_fd,RESP_200,strlen(RESP_200),0);
			  fseek(pfile,0,SEEK_END);//located the END position
        		int filelen=ftell(pfile);//got file length
        		fseek(pfile,0,SEEK_SET);//return to the begging position
        		int readlen=0;
        	sleep(1);
			  do{
			  	readlen = fread(line,sizeof(char),MAXDATASIZE,pfile);

			    //line[strlen(line)]='\0';
			    if(readlen>0){
			    	send(new_fd,filebuff,strlen(line),0);
			    	filelen=filelen-readlen;
			    	printf("numof bytes sent:%d\n",strlen(line));
			    }
			    memset(filebuff,'\0',MAXDATASIZE);
		            
			  }while((filelen>0)&&(readlen>0));
			  fclose(pfile);
			  close(new_fd);
			  exit(0);
			
			}
		}
		close(new_fd);  // parent doesn't need this
	}

	return 0;
}

