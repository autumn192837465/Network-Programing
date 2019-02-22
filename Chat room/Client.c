#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <netdb.h>
int std_ready(void);

int strLen(char *str){
	return strlen(str);
}

int main(int argc, char const *argv[])
{
	if(argc!=3){
		printf("Too few arguments\n");
		exit(1);
	}

	int socketfd=0;
	int fdmax;
	socketfd = socket(PF_INET,SOCK_STREAM,0);		
	if(socketfd==-1){
		printf("Unable to create socket!!\n");
	}

	struct sockaddr_in info;

	bzero(&info,sizeof(info));
	info.sin_family = AF_INET;	
	info.sin_port = htons(atoi(argv[2]));
	
	if(argv[1][0]<=57 && argv[1][0]>=48)
	{
		inet_pton(AF_INET,argv[1],&(info.sin_addr));		
	}	
	else{
		 struct hostent* pHostEntry = gethostbyname("nplinux1.cs.nctu.edu.tw");
		 info.sin_addr.s_addr = ((struct in_addr*)pHostEntry->h_addr)->s_addr;
		 printf("%s\n", inet_ntoa(info.sin_addr));
	}

	fd_set read_fds,master;
	int nbytes;

	int err=connect(socketfd,(struct sockaddr *)&info,sizeof(info));
	if(err==-1){
		printf("Connection error\n");
	}
	else{
		fdmax=socketfd;		

		printf("Connection succeed\n");

		char userInput[1600];	
		char receiveMessage[1600];
		char msg[1600];
		int nread;

		FD_ZERO(&read_fds);
		FD_ZERO(&master);

		FD_SET(socketfd,&master);

		struct timeval timeout;
		timeout.tv_sec=0;
		timeout.tv_usec=1;
		
		while(1){
			int connectClose=0;
			read_fds=master;
			if(std_ready()){
				fscanf(stdin, "\n%[^\n]", userInput);
            	sprintf(msg,"%s", userInput);                         	
            	if(!strcmp(userInput,"exit")){
            		break;
            	}
            	send(socketfd, msg, strLen(msg), 0);
			}
			if(select(fdmax+1,&read_fds,NULL,NULL,&timeout)==-1){
				perror("Select failed");
				exit(1);				
			}
			int i;
			for(i=0;i<=fdmax;i++){
				if(FD_ISSET(i,&read_fds)){
					if(i==socketfd){
						int j;
						for(j=0;j<1600;j++)	userInput[j]='\0';		

						if((nbytes = recv(i,userInput,sizeof(userInput),0))<=0){
							//close
							connectClose=1;
							close(socketfd);
							break;
						}
						else{
							printf("%s", userInput);
						}
					}					
				}
			}
			if(connectClose==1){
				break;
			}
		}

		close(socketfd);		
		printf("End Connection\n");
	}

	

	return 0;
}

int std_ready()
{
	fd_set fdset;
	struct timeval timeout;
	int fd;
	fd=fileno(stdin);
	FD_ZERO(&fdset);
	FD_SET(fd,&fdset);
	timeout.tv_sec=0;
	timeout.tv_usec=1;
	
	usleep(5000);
	return select(fd+1,&fdset,NULL,NULL,&timeout)==1?1:0;
}
