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
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>


char SERVER_READY[20]="Server is ready.";
char SERVER_NOT_READY[30]="Server is not ready.";
char HAVE_FILE[20]="Have file.";
char No_FILE[20]="No file.";


void ClearBuf(int socketfd){
	
	fcntl(socketfd,F_SETFL, O_NONBLOCK);
	char readbuf[65536];

	while(recv(socketfd,readbuf,sizeof(readbuf),0)>0){
		printf("Buffer : %s\n", readbuf);
		printf("Clear Buffer!\n");
	}
		

	fcntl(socketfd,F_SETFL, !O_NONBLOCK);
	return;
}

int main(int argc,char *argv[])
{

	if(argc<=3){
		printf("Too few arguments\n");
		exit(1);
	}

	//int status=0;	
	/*
		status=0 connect and send file name
		status=1 idle
		status=2 send file
		status=3 server received, start transfer
	*/

	char sendbuf[65536],readbuf[65536];
	int readbytes,nbytes;


	int comSocketfd,dataSocketfd;
	int addrlen;
	comSocketfd = socket(PF_INET,SOCK_STREAM,0);	
	dataSocketfd = socket(PF_INET,SOCK_STREAM,0);	

	struct sockaddr_in serverInfo;
	bzero(&serverInfo,sizeof(serverInfo));
	serverInfo.sin_family = AF_INET;
	serverInfo.sin_port = htons(atoi(argv[2]));
	serverInfo.sin_addr.s_addr= inet_addr(argv[1]);
	

	connect(comSocketfd,(struct sockaddr *)&serverInfo,sizeof(serverInfo));	

	bzero(readbuf,sizeof(readbuf));
	send(comSocketfd,argv[3],strlen(argv[3]),0);
	recv(comSocketfd,readbuf,sizeof(readbuf),0);
	//Welcome message
	printf("%s\n", readbuf);
	
	connect(dataSocketfd,(struct sockaddr *)&serverInfo,sizeof(serverInfo));			
	
	//set timeout
	struct timeval tv; 	
  	

  	int fdmax;
  	fd_set master,read_fds;	
	FD_ZERO(&master);
	FD_ZERO(&read_fds);
	FD_SET(comSocketfd,&master);	
	FD_SET(fileno(stdin),&master);
	fdmax=comSocketfd;

	FILE *fp;
	char filename[20];
	int filesize;
	int cursize;

	while(1){

		read_fds = master;
		
		tv.tv_sec = 1;
		tv.tv_usec = 0;

		bzero(readbuf,sizeof(readbuf));
		bzero(sendbuf,sizeof(sendbuf));		

		ClearBuf(comSocketfd);
		select(fdmax+1,&read_fds,NULL,NULL,NULL);

		if(FD_ISSET(fileno(stdin),&read_fds)){

			//type command			
			fscanf(stdin, "\n%[^\n]", sendbuf);			
			char *command;
			command = strtok(sendbuf," \n\r");			
							
			if(!strcmp("/put",command)){					

				char *temp;
				temp = strtok(NULL," \n\r");				
				bzero(filename,sizeof(filename));
				strcpy(filename,temp);
				//open file
				fp = fopen(filename,"rb");
				//get file size	and send
				fseek(fp, 0, SEEK_END); 					
				filesize = ftell(fp); 
				fseek(fp, 0, SEEK_SET);
				
				bzero(sendbuf,sizeof(sendbuf));
				sprintf(sendbuf,"%s %d",filename,filesize);
					
				//send file name and size
				send(comSocketfd,sendbuf,strlen(sendbuf),0);
									
				//check whether server received
				while(1){
					bzero(readbuf,sizeof(readbuf));
					recv(comSocketfd,readbuf,sizeof(readbuf),0);					
					if(!strcmp(SERVER_READY,readbuf)){													
						break;
					}
					else {
						sleep(1);						
						send(comSocketfd,sendbuf,strlen(sendbuf),0);
					}					
				}	
				
				cursize=0;
				while((readbytes = fread(sendbuf,sizeof(char),sizeof(sendbuf),fp))>0){
					cursize += readbytes;						
					send(dataSocketfd,sendbuf,readbytes,0);									
					
					printf("Uploading file: %s\n", filename);
					printf("Progress :[");
					int progress = cursize/(filesize/20);					
					int i;
					for(i=0;i<progress && i<20;i++){
						printf("#");
					}
					for(;i<20;i++){
						printf(" ");
					}
					printf("]\n");
					
					printf("cursize:%d ,total: %d\n", cursize,filesize);
					bzero(readbuf,sizeof(readbuf));						
					bzero(sendbuf,sizeof(sendbuf));						
				}
				printf("Upload %s complete!\n", filename);
				bzero(readbuf,sizeof(readbuf));									
								
				fclose(fp);
			}				
			else if(!strcmp("/sleep",command)){
				int time;
				char *temp; 
				temp = strtok(NULL," \n\r");
				time = atoi(temp);		
				printf("Client starts to sleep\n");
				int j;
				for(j=1; j<= time;j++){
					sleep(1);	
					printf("Sleep %d\n", j);
				}							
				printf("Client wakes up\n");
				ClearBuf(comSocketfd);
				ClearBuf(dataSocketfd);							
			}		
			else if(!strcmp("/exit",command)){
				break;
			}
			else{
				printf("Error command\n");
			}
		}
		
		if(FD_ISSET(comSocketfd,&read_fds)){				
			
			//check file name		
			bzero(readbuf,sizeof(readbuf));
			recv(comSocketfd,readbuf,sizeof(readbuf),0);				
			//if doesn't have this file
			if(access(readbuf,F_OK) == -1){
				printf("sssss|%s|\n", readbuf);					
				send(comSocketfd,No_FILE,strlen(No_FILE),0);
				//receive filename and size
				bzero(readbuf,sizeof(readbuf));
				recv(comSocketfd,readbuf,sizeof(readbuf),0);													
				
				char *temp;								
				temp = strtok(readbuf," \n\r");					
				bzero(filename,sizeof(filename));					
				strcpy(filename,temp);					
				fp = fopen(filename,"wb");
				temp = strtok(NULL," \n\r");
											
				filesize = atoi(temp);
				cursize = 0;
				while(cursize != filesize){
					bzero(readbuf,sizeof(readbuf));
					nbytes = recv(dataSocketfd,readbuf,sizeof(readbuf),0);
					int k = fwrite(readbuf,sizeof(char),nbytes,fp); 
					fflush(fp);
					cursize += k;
					printf("Downloading file: %s\n", filename);
					printf("Progress :[");
					int progress = cursize/(filesize/20);
					int i;
					for(i=0;i<progress && i<20;i++){
						printf("#");
					}
					for(;i<20;i++){
						printf(" ");
					}
					printf("]\n");
					printf("Cur size: %d, File size: %d\n",cursize,filesize);
				}					
				fclose(fp);					
				printf("Download %s complete!\n", filename);
				
				ClearBuf(comSocketfd);
			}				
			else{									
				send(comSocketfd,HAVE_FILE,strlen(HAVE_FILE),0);
			}				
		}
	}		


	close(comSocketfd);
	close(dataSocketfd);


}