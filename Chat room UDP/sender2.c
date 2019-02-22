#include <sys/types.h>
#include <sys/socket.h> 
#include <string.h>
#include <netinet/in.h> 
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h> 
#include <signal.h>
#include <errno.h>
#define PORT 8887

char END_FLAG[10]="END_FILE";

int readable_timeo(int fd,int sec){
    fd_set rset;
    struct timeval tv;

    FD_ZERO(&rset);
    FD_SET(fd, &rset);

    tv.tv_sec = sec;
    tv.tv_usec = 0;

    return (select(fd+1, &rset, NULL, NULL, &tv));
}


void dg_cli(FILE *fp, int socketfd, struct sockaddr *receiverInfo, socklen_t addrlen,const char *filename){

       
    char sendbuf[1025],recvbuf[1];    
    int readbytes,recvbytes,sendbytes;
    int packnum=0;


    while((readbytes = fread(sendbuf,sizeof(char),1024,fp))>0)
    {   
        sendbuf[readbytes] = packnum%50;                

        printf("Reading %d bytes\n", readbytes);
        if((sendbytes = sendto(socketfd,sendbuf,readbytes+1,0,receiverInfo,addrlen))==-1){           
            perror("Send failed ");
            exit(1);
        }
        else{
            printf("Sending %d bytes\n",sendbytes);
        }           
        
                
        while(1){
            bzero(recvbuf,sizeof(recvbuf));            
            
            if(readable_timeo(socketfd,1)==0){
                printf("Pack %d time up \n", sendbuf[readbytes]);
                sendbytes = sendto(socketfd,sendbuf,readbytes+1,0,receiverInfo,addrlen);
            }
            else{
                recvbytes= recvfrom(socketfd,recvbuf,sizeof(recvbuf),0,NULL,NULL);
                if(recvbuf[0] == packnum%50){
                    printf("Ack %d received\n", recvbuf[0]);                                        
                    break;
                }
                else{                    
                    sendbytes = sendto(socketfd,sendbuf,readbytes+1,0,receiverInfo,addrlen);                    
                    continue;
                }                                                
            }
        }

        bzero(sendbuf,sizeof(sendbuf));        
        packnum++;
    }        
    
    
    int counter=0;
    while(1){
        sendbytes = sendto(socketfd,END_FLAG,strlen(END_FLAG),0,receiverInfo,addrlen);    

        if(readable_timeo(socketfd,1)==0){                        
                if(counter >=10)    break;
                sendto(socketfd,END_FLAG,strlen(END_FLAG),0,(struct sockaddr*)receiverInfo,addrlen);                                                   
        }
        else{
            recvfrom(socketfd,sendbuf,sizeof(sendbuf),0,NULL,NULL);
            if(!strcmp(sendbuf,END_FLAG)){
                printf("End file\n");
                sendto(socketfd,END_FLAG,strlen(END_FLAG),0,(struct sockaddr*)&receiverInfo,addrlen);            
                break;
            }                                      
        }
        counter++;        
    }   

    return;
}


int main(int argc, char const *argv[])
{
    if(argc!=4){
        printf("Error\n");
        exit(1);
    }    

    int socketfd = 0;
    printf("Socket Creating...\n");
    if((socketfd = socket(AF_INET,SOCK_DGRAM,0))==-1){
        perror("Socket create failed");
        exit(1);
    }   
        
    //Initializing
    struct sockaddr_in receiverInfo; 
    bzero(&receiverInfo,sizeof(receiverInfo));
    receiverInfo.sin_family = AF_INET;
    receiverInfo.sin_addr.s_addr = inet_addr(argv[1]);
    receiverInfo.sin_port = htons(atoi(argv[2]));

    char buf[1024];    
    int addrlen = sizeof(receiverInfo);
    int recvbytes,sendbytes;

    printf("Waiting for receiver online\n");
    //Check is receiver Online
    while(1){        
        bzero(buf,sizeof(buf));
        sendto(socketfd,argv[3],strlen(argv[3]),0,(struct sockaddr*)&receiverInfo,addrlen);

        if(readable_timeo(socketfd,1)==0){
            printf("Receiver is not online.\n");  
        }
        else{
            recvfrom(socketfd,buf,sizeof(buf),0,(struct sockaddr*)&receiverInfo,&addrlen);
            if(!strcmp(buf,argv[3])){
                printf("Receiver found.  IP: %s Port: %d\n",inet_ntoa(receiverInfo.sin_addr),ntohs(receiverInfo.sin_port));    
                break;
            }        
            continue; 
        }
    }    
    
    //Open file and send    
    printf("Sending file\n\n");
    FILE *fp;    
    if(!(fp = fopen(argv[3],"rb"))){
        perror("File doesn't exist!\n");
        exit(1);
    }   
    
    dg_cli(fp,socketfd,(struct sockaddr*)&receiverInfo,addrlen,argv[3]);

    close(socketfd);
    fclose(fp);
    return 0;
}