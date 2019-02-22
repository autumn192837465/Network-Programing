#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#define PORT 9996




struct ClientInformation
{
	int sock;
	char name[20];	
	char Ip[20];
	int  port;
}client[18];
int clientNum=0;

int CheckName(char *name,struct ClientInformation *c){

	//check length and english
	char *temp;
	temp=strtok(NULL," \n\r");		

	if(strlen(temp)<=1||strlen(temp)>=13)
		return 3;
	

	for(int i=0;i<strlen(temp);i++){		
		if(temp[i]<65 || (temp[i]>90 && temp[i]<97) || temp[i]>172){
			//printf("%d\n", name[i]);
			return 3;
		}
	}

	//check anonymous
	
		
	if(strcmp("anonymous",temp)==0 || strcmp("Anonymous",temp)==0){		
		return 1;
	}

	for(int i=0;i<18;i++){
		if(i!=c->sock && strcmp(temp,client[i].name)==0)	return 2;
	}

	strcpy(c->name, temp);
	
	return 0;
}

int CheckTell(char *command,char *targetName){
	char *name;		
	
	if((name=strtok(NULL," "))==NULL){
		return 2;
	}	
	if(name==NULL)	return 4;
	if(strcmp(name,"anonymous")==0 || strcmp(name,"Anonymous")==0)	return 1;	
	
	int existClient=0;	
	for(int i=0;i<18;i++){		
	
		if(strcmp(name,client[i].name)==0){
			existClient=1;
			break;
		}
	}
	
	if(existClient==0)	return 2;
	
	strcpy(targetName,name);
	
	return 0;

}
int strLen(char *str){
	return strlen(str);
}
int main()
{

	int socketfd,newSocketfd;
	int addrlen;

	for(int i=0;i<18;i++){
		client[i].sock=-1;
		strcpy(client[i].name,"Anonymous");
	}	

	//maxium file descriptor number
	int fdmax,newfd;
	printf("Building Socket...\n");
	socketfd = socket(PF_INET,SOCK_STREAM,0);
	if(socketfd==-1){
		printf("Fail!!\n");
	}

	struct sockaddr_in serverInfo,clientInfo;	

	//initialize serverInfo
	printf("initialize...\n");	
	bzero(&serverInfo,sizeof(serverInfo));
	serverInfo.sin_family = AF_INET;
	serverInfo.sin_port = htons(PORT);
	serverInfo.sin_addr.s_addr=INADDR_ANY;

	printf("Binding...\n");
	bind(socketfd,(struct sockaddr*)&serverInfo,sizeof(serverInfo));

	printf("Listening...\n");
	listen(socketfd,5);
	
	char sendToRequestMessage[1600]={};
	char sendToOthersMessage[1600]={};
	char clientMessage[1600]={};
	char *command;	
	int  nbytes;
	
	//initialize client;


	//add socket to maseter set
	

	//set of socket descriptors	
	fd_set master,read_fds;	
	FD_ZERO(&master);
	FD_ZERO(&read_fds);
	FD_SET(socketfd,&master);
	fdmax=socketfd;
	int msglen=0;	
	while(1){
		read_fds=master;		
	
		for(int i=0;i<1600;i++)	sendToRequestMessage[i]='\0';
		for(int i=0;i<1600;i++)	sendToOthersMessage[i]='\0';
		for(int i=0;i<1600;i++)	clientMessage[i]='\0';			
						
		
		if(select(fdmax+1,&read_fds,NULL,NULL,NULL)==-1){
			perror("Select failed");
			exit(1);
		}
		
		for(int i=0;i<=fdmax;i++){			
			if(FD_ISSET(i,&read_fds)){
				if(i==socketfd){
					addrlen=sizeof(clientInfo);					
					if((newSocketfd=accept(socketfd,(struct sockaddr *)&clientInfo,&addrlen))==-1){
						perror("accept failed");
					}
					else{
						if(clientNum>=15){
							sprintf(sendToRequestMessage,"Too many client, please connet later!\n");
							send(newSocketfd,sendToRequestMessage,strLen(sendToRequestMessage),0);
							close(newSocketfd);
							printf("Too many client\n");
						}
						else{							
							printf("Online client: %d\n", clientNum);
							clientNum++;
							FD_SET(newSocketfd,&master);
							if(newSocketfd>fdmax){
								fdmax=newSocketfd;
							}
							
							client[newSocketfd].sock=newSocketfd;				
							strcpy(sendToOthersMessage,"[Server] Someone is comming!\n");
							
							inet_ntop(AF_INET,&(clientInfo.sin_addr),client[newSocketfd].Ip,sizeof(client[newSocketfd].Ip));
							client[newSocketfd].port=clientInfo.sin_port;
							sprintf(sendToRequestMessage,"[Server] Hello, anonymous! From: %s/%d\n",client[newSocketfd].Ip,client[newSocketfd].port);	

							for(int j=0;j<=fdmax;j++){
								if(j==newSocketfd){
									send(j,sendToRequestMessage,strLen(sendToRequestMessage),0);
								}
								else if(j!=socketfd){
									send(j,sendToOthersMessage,strlen(sendToOthersMessage),0);	
								}							
							}						
							printf("New Connection from %s on socket %d\n",client[newSocketfd].Ip, newSocketfd);
						}
					}				
				}
				else{
					if((nbytes = recv(i,clientMessage,sizeof(clientMessage),0))<=0){						
						//connection closed						

						clientNum--;
						printf("%s offline, Socket %d closed\n", client[i].name,client[i].sock);
						sprintf(sendToOthersMessage,"[Server] %s is offline\n",client[i].name);
						printf("Online client: %d\n", clientNum);
						for(int j=0;j<=fdmax;j++){
							if(j!=socketfd && j!=i){
								send(j,sendToOthersMessage,strLen(sendToOthersMessage),0);
							}
						}

						client[i].sock=-1;
						strcpy(client[i].name,"anonymous");
						close(i);
						FD_CLR(i,&master);
					}
					else{						
						printf("%s : %s\n",client[i].name,clientMessage);												
						command=strtok(clientMessage," \n\r");														
						if(strcmp(command,"name")==0){							
							char oldname[12];
							for(int j=0;j<12;j++)	oldname[j]='\0';								
							strcpy(oldname,client[i].name);			

							switch(CheckName(clientMessage,&client[i])){
								case 0:
									//valid name
									sprintf(sendToRequestMessage,"[Server] You're now known as %s.\n",client[i].name);
									sprintf(sendToOthersMessage,"[Server] %s is now known as %s.\n",oldname,client[i].name);
									send(i,sendToRequestMessage,strLen(sendToRequestMessage),0);
									for(int j=0;j<=fdmax;j++){
										if(j!=socketfd && j!=i){
											send(j,sendToOthersMessage,strLen(sendToOthersMessage),0);
										}
									}
									break;
								case 1:
									//can't be anonymous
									sprintf(sendToRequestMessage,"[Server] ERROR: Username cannot be anonymous.\n");									
									send(i,sendToRequestMessage,strLen(sendToRequestMessage),0);
									break;
								case 2:
									//not unique																										
									for(int j=5;j<=sizeof(clientMessage);j++)	oldname[j-5]=clientMessage[j];
									sprintf(sendToRequestMessage,"[Server] ERROR: %s has been used by others.\n",oldname);
									send(i,sendToRequestMessage,strLen(sendToRequestMessage),0);
									break;
								case 3:
									//doesn't have english									
									sprintf(sendToRequestMessage,"[Server] ERROR: Username can only consists of 2~12 English letters.\n");		
									send(i,sendToRequestMessage,strLen(sendToRequestMessage),0);																
									break;

							}																														
						}
						else if(strcmp(command,"who")==0){							
							for(int j=0;j<18;j++){
								if(client[j].sock!=-1){
									if(j==i){
										sprintf(sendToRequestMessage,"[Server] %s %s/%d ->me\n",client[j].name,client[i].Ip,client[i].port);			
									}
									else{
										sprintf(sendToRequestMessage,"[Server] %s %s/%d\n",client[j].name,client[j].Ip,client[j].port);			
									}					
									send(i,sendToRequestMessage,strLen(sendToRequestMessage),0);
								}								
							}
						}
						else if(strcmp(command,"tell")==0){							
							if(strcmp(client[i].name,"Anonymous")==0){
								sprintf(sendToRequestMessage,"[Server] ERROR: You are anonymous.\n");
								send(i,sendToRequestMessage,strLen(sendToRequestMessage),0);
							}
							else{
								char targetName[12];
								char *message;								
								for(int j=0;j<12;j++)	targetName[j]='\0';
								switch(CheckTell(clientMessage,targetName)){
									case 0:										
										for(int j=0;j<=fdmax;j++){

											if(strcmp(client[j].name,targetName)==0){												
												
												message=strtok(NULL,"\n\r");												
												sprintf(sendToOthersMessage,"[Server] %s tell you %s\n",client[i].name,message);
												sprintf(sendToRequestMessage,"[Server] SUCCESS: Your message has been sent.\n");
												send(j,sendToOthersMessage,strLen(sendToOthersMessage),0);
												send(i,sendToRequestMessage,strLen(sendToRequestMessage),0);
												break;
											}												
										}
										break;
									case 1:
										sprintf(sendToRequestMessage,"[Server] ERROR: The client to which you sent is anonymous.\n");
										send(i,sendToRequestMessage,strLen(sendToRequestMessage),0);
										break;
									case 2:
										sprintf(sendToRequestMessage,"[Server] ERROR: The receiver doesn't exist.\n");
										send(i,sendToRequestMessage,strLen(sendToRequestMessage),0);										
										break;
								}
							}
							
						}
						else if(strcmp(command,"yell")==0){							
							char *message=strtok(NULL,"\n\r");							
							sprintf(sendToOthersMessage,"[Server] %s yell %s\n",client[i].name,message);
							for(int j=0;j<=fdmax;j++){
								if(j!=socketfd)
									send(j,sendToOthersMessage,strLen(sendToOthersMessage),0);
							}
						}
						else{																					
							/*
							sprintf(sendToOthersMessage,"%s: %s\n",client[i].name,clientMessage);																									
							for(int j=0;j<=fdmax;j++){
								if(j!=socketfd && j !=i){
									send(j,sendToOthersMessage,strLen(sendToOthersMessage),0);
								}
							}
							*/							
							sprintf(sendToRequestMessage,"[Server] ERROR: Error command.\n");
							send(i,sendToRequestMessage,sizeof(sendToRequestMessage),0);
							
						}
																				
					}
				}
			}
			else{

			}
		}
		
	}

	close(socketfd);
	return 0;

}
