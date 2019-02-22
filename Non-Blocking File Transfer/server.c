#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/types.h>  
#include <sys/stat.h> 
#include <dirent.h>
#include <fcntl.h>
#include <errno.h>

const char WelcomeMsg[100] = "Welcome to the dropbox-like server! : ";
char SERVER_READY[20]="Server is ready.";
char SERVER_NOT_READY[30]="Server is not ready.";
char HAVE_FILE[20]="Have file.";
char No_FILE[20]="No file.";


struct ClientInformation
{
	int comsock,datasock;
	char name[20];		
	FILE *fp;	
	char filename[20];
	int filesize;
	int cursize;
	int status;			// 0 = no client, 1 = idle, 2 = receive file from client, 3 = sending file to client
}client[100];



int clientnum=0;


int Nonblock(int socketfd,int timeout1,int timeout2,int num){
	struct timeval tv; 

	int fdmax=socketfd;
	fd_set master,read_fds;
	FD_ZERO(&master);
	FD_ZERO(&read_fds);
	FD_SET(socketfd,&master);
	int i;
	for(i=0;i<num;i++){
		read_fds = master;
		tv.tv_sec = timeout1;
		tv.tv_usec = timeout2;			
		if(select(fdmax+1,&read_fds,NULL,NULL,&tv)==1)	return 1;
		printf("%d socket  timeout %d\n", socketfd,i);		
	}
	
	return 0;
}

void ClearBuf(int socketfd){
	
	char readbuf[65536];

	while(recv(socketfd,readbuf,sizeof(readbuf),0)>0){
		printf("Clear Buffer!\n");
	}
	return;
}
void RecvFilename(struct ClientInformation *c,char *readbuf,int *receiving){
	*receiving +=1;								

	char path[50];
	bzero(path,sizeof(path));

	printf("||%s||\n", readbuf);
	// receive filename
 	char *temp = strtok(readbuf," \n\r");							
	sprintf(path,"./%s/%s",(*c).name,temp);
	// file size
	temp = 	strtok(NULL," \n\r");							
	(*c).cursize = 0;
	(*c).filesize = atoi(temp);

	(*c).fp = fopen(path,"wb");			
	(*c).status = 2;
								
	//send Server_ready msg								
	send((*c).comsock,SERVER_READY,strlen(SERVER_READY),0);	
}

void RecvData(struct ClientInformation *c,int *receiving){
	char readbuf[65536];
	int nbytes;

	//receive file
	bzero(readbuf,sizeof(readbuf));
	if((nbytes = recv((*c).datasock,readbuf,sizeof(readbuf),0))<0){
		return;
	}

	//write data
	fwrite(readbuf,sizeof(char),nbytes,(*c).fp);
	fflush((*c).fp);

	(*c).cursize +=nbytes;
	printf("Writing %d bytes. Total %d bytes\n", nbytes,(*c).cursize);
	if((*c).cursize == (*c).filesize){
		printf("File transfer success! Total %d bytes\n", (*c).cursize);									
		(*c).status = 1;
		fclose((*c).fp);
		*receiving -=1;
	}	
}
void SendData(struct ClientInformation *c,int *sending){
	char sendbuf[65536];
	int nbytes;

	bzero(sendbuf,sizeof(&sendbuf));
	nbytes = fread(sendbuf,sizeof(char),sizeof(sendbuf),(*c).fp);
	send((*c).datasock,sendbuf,nbytes,0);
	(*c).cursize += nbytes;
	
	if((*c).cursize == (*c).filesize){
		fclose((*c).fp);
		(*c).status = 1;
		*sending -=1;
	}			
}
void CloseSocket(struct ClientInformation *c,fd_set *master,int *clientnum){
	
	// close socket
	close((*c).comsock);
	close((*c).datasock);
	FD_CLR((*c).comsock,master);
	FD_CLR((*c).datasock,master);
	(*c).status = 0;
	printf("Socket %d,%d closed\n" ,(*c).comsock, (*c).datasock);
	*clientnum -=1;

}
int main(int argc,char *argv[])
{
	if(argc<=1){
		printf("Too few arguments\n");
		exit(1);
	}

	int socketfd,newSocketfd;
	int addrlen;
	socketfd = socket(PF_INET,SOCK_STREAM,0);
	fcntl(socketfd, F_SETFL, O_NONBLOCK);
	
	struct sockaddr_in serverInfo,clientInfo;	
	bzero(&serverInfo,sizeof(serverInfo));
	serverInfo.sin_family = AF_INET;
	serverInfo.sin_port = htons(atoi(argv[1]));
	serverInfo.sin_addr.s_addr=INADDR_ANY;

	printf("Binding...\n");
	bind(socketfd,(struct sockaddr*)&serverInfo,sizeof(serverInfo));		

	printf("Listening...\n");
	listen(socketfd,5);

	int fdmax,newfd;
	fd_set master,read_fds;	
	FD_ZERO(&master);
	FD_ZERO(&read_fds);
	FD_SET(socketfd,&master);
	fdmax=socketfd;

	char sendbuf[65536],readbuf[65536];
	char path[100];

	addrlen = sizeof(clientInfo);

	//set timeout
	struct timeval tv; 	  	

	//FILE *fp;
    //fp = fopen("d.jpg","wb");
    
    int nbytes;   

    bzero(client,sizeof(client));    
    int sending=0;
    int receiving=0;
	while(1){
		bzero(sendbuf,sizeof(sendbuf));		
		bzero(readbuf,sizeof(readbuf));		
		int i;		
		//printf("send %d,recv %d\n", sending,receiving);
		//check every client have all files
		if(sending == 0 && receiving == 0){					
			for(i=0;i<100;i++){				
				if(client[i].status != 1){
					continue;
				}				
						
				DIR *dir;
				struct dirent *ptr;

				//check every file
				bzero(path,sizeof(path));
				sprintf(path,"./%s",client[i].name);			
				
				dir = opendir(path);

				while((ptr = readdir(dir))!=NULL){				
					if(!strcmp(ptr->d_name,".")||!strcmp(ptr->d_name,".."))	continue;															

					bzero(readbuf,sizeof(readbuf));									
					//send file name check client whether have this file
					ClearBuf(client[i].comsock);
					if(send(client[i].comsock,ptr->d_name,strlen(ptr->d_name),0)<0){																						
						send(client[i].comsock,ptr->d_name,strlen(ptr->d_name),0);
					}
					if((nbytes = recv(client[i].comsock,readbuf,sizeof(readbuf),0))<=0 ){						
						if(nbytes == 0){							
							CloseSocket(&client[i],&master,&clientnum);
							break;
						}
						else if(errno == EAGAIN || errno == EWOULDBLOCK){							
							//Client is possibly sleeping		
							if(Nonblock(client[i].comsock,0,50000,3)==1){
								//Client is not sleeping								
								nbytes = recv(client[i].comsock,readbuf,sizeof(readbuf),0);
								if(nbytes == 0){									
									CloseSocket(&client[i],&master,&clientnum);
									break;
								}								
							}
							else{
								//printf("Client %d is sleeping\n",client[i].comsock);								
								break;
							}
						}	
					}

					if(!strcmp(No_FILE,readbuf)){							
						printf("%d no file : %s\n", client[i].comsock,ptr->d_name);
						bzero(path,sizeof(path));
						sprintf(path,"./%s/%s",client[i].name,ptr->d_name);					
						client[i].fp = fopen(path,"rb");

						fseek(client[i].fp,0,SEEK_END);
						client[i].filesize = ftell(client[i].fp);
						fseek(client[i].fp,0,SEEK_SET);	

						printf("File size:%d\n", client[i].filesize);			
						client[i].cursize = 0;

						//send filename and size
						sprintf(sendbuf,"%s %d",ptr->d_name,client[i].filesize);						
						send(client[i].comsock,sendbuf,strlen(sendbuf),0);

						sending+=1;
						client[i].status = 3;					
						break;
					}
					else if(!strcmp(HAVE_FILE,readbuf)){						
						continue;
					}
					else{
						bzero(readbuf,sizeof(readbuf));

						if((nbytes = recv(client[i].comsock,readbuf,sizeof(readbuf),0))<=0){
							if(nbytes == 0){
								CloseSocket(&client[i],&master,&clientnum);
								break;
							}
							else if(errno == EAGAIN){
								if(Nonblock(client[i].comsock,1,0,3)==0){
									continue;
								}
								else{
									recv(client[i].comsock,readbuf,sizeof(readbuf),0);
								}						
							}
						}								
						RecvFilename(&client[i],readbuf,&receiving);		
						break;
					}
				}
				closedir(dir);		
				if(sending!= 0 || receiving!=0){
					break;
				}
			}
		}
		
		
		tv.tv_sec = 0;
		tv.tv_usec = 5000;		
		 	
		read_fds = master;		
		if( select(fdmax+1,&read_fds,NULL,NULL,&tv)==0 ){	
			for(i=0;i<100;i++){				
				if(client[i].status == 2){					
					RecvData(&client[i],&receiving);
				}
				if(client[i].status == 3){										
					SendData(&client[i],&sending);
				}
			}						
		}
		else{					
			for(i=0;i<=fdmax;i++){				
				if(FD_ISSET(i,&read_fds)){
					
					bzero(sendbuf,sizeof(sendbuf));		
					bzero(readbuf,sizeof(readbuf));			

					// new connection
					if(i == socketfd){						
						int c;
						newSocketfd=accept(socketfd,(struct sockaddr *)&clientInfo,&addrlen);
						FD_SET(newSocketfd,&master);

						if(newSocketfd>fdmax){
							fdmax=newSocketfd;
						}				
						c= newSocketfd;
						//change status
						client[c].status = 1;

						//connect command socket
						client[c].comsock = c;

						//set name
						recv(c,readbuf,sizeof(readbuf),0);					
						strcpy(client[c].name,readbuf);

						//send welcome message
						bzero(sendbuf,sizeof(&sendbuf));
						sprintf(sendbuf,"%s%s\n",WelcomeMsg,readbuf);		
											
						send(c,sendbuf,strlen(sendbuf),0);

						//connect data socket						
						if((newSocketfd=accept(socketfd,(struct sockaddr *)&clientInfo,&addrlen))<0){
							if(errno == EAGAIN){								
								// if have datasock connection
								if(Nonblock(socketfd,1,0,3) == 1){
									newSocketfd=accept(socketfd,(struct sockaddr *)&clientInfo,&addrlen);
								}
							}							
						}
						if(newSocketfd>fdmax){
							fdmax=newSocketfd;
						}	

						client[c].datasock = newSocketfd;					
						
						//set socket to nonblocking
						fcntl(client[c].comsock, F_SETFL, O_NONBLOCK);

						clientnum++;						
						printf("%s connected. Socket %d,%d \n",client[c].name,c,newSocketfd);		

						bzero(path,sizeof(path));
						sprintf(path,"./%s",client[c].name);
						if(access(path,F_OK) == -1){
							mkdir(path,S_IRWXU|S_IRWXO);
							printf("Folder created: %s\n", client[c].name);
						}																			
					}
					else{							
						if((nbytes = recv(i,readbuf,sizeof(readbuf),0))<=0){							
							CloseSocket(&client[i],&master,&clientnum);
						}
						else{					
							if(!strcmp(No_FILE,readbuf)||!strcmp(HAVE_FILE,readbuf))	continue;
							
							if(client[i].status == 1){																		
								RecvFilename(&client[i],readbuf,&receiving);
							}								
						}													
					}
				}			
			}
		}	
	}	
	

}