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

static void sig_alrm(int signo){
    printf("Times up\n");
    return;
}

void dg_cli(FILE *fp, int socketfd, struct sockaddr *receiverInfo, socklen_t addrlen,const char *filename){
        
    char sendbuf[1025],recvbuf[1];
    int readbytes,recvbytes,sendbytes;
    int packnum=0;

    struct sigaction act;
    act.sa_handler = sig_alrm;
    sigemptyset(&act.sa_mask);
    sigaddset(&act.sa_mask,SIGALRM);    
    act.sa_flags = SA_INTERRUPT;
    sigaction(SIGALRM,&act,0);


    while((readbytes = fread(sendbuf,sizeof(char),1024,fp))>0)
    {   
        sendbuf[readbytes] = packnum%50;           

        printf("Reading %d bytes\n", readbytes);
        if((sendbytes = sendto(socketfd,sendbuf,readbytes+1,0,receiverInfo,addrlen))==-1){           
            perror("Send failed ");
            exit(1);
        }
        else{
            printf("Sending %d bytes. Package %d\n",sendbytes,sendbuf[readbytes]);
        }           
                    
        while(1){
            bzero(recvbuf,sizeof(recvbuf));            
            
            alarm(1);    
            if((recvbytes= recvfrom(socketfd,recvbuf,sizeof(recvbuf),0,NULL,NULL))<0){
                if(errno = EINTR){
                    //resend                    
                    printf("Pack %d time up \n", sendbuf[readbytes]);
                    sendbytes = sendto(socketfd,sendbuf,readbytes+1,0,receiverInfo,addrlen);
                    continue;
                }
                else{
                    printf("Recv error\n");                
                }            
            }
            else{
                if(recvbuf[0] == packnum%50){
                    printf("Ack %d received\n", recvbuf[0]);
                    alarm(0);    
                }
                else{
                    sendbytes = sendto(socketfd,sendbuf,readbytes+1,0,receiverInfo,addrlen);
                    continue;
                }                
                break;
            }                    
        }
        bzero(sendbuf,sizeof(sendbuf));        
        packnum++;
    }            
    int counter=0;
    while(1){

        sendbytes = sendto(socketfd,END_FLAG,strlen(END_FLAG),0,receiverInfo,addrlen);
        alarm(1);

        if((recvbytes= recvfrom(socketfd,sendbuf,sizeof(sendbuf),0,NULL,NULL))<0){            
            if(errno == EINTR){
                if(counter >=10)    break;                
                sendto(socketfd,END_FLAG,strlen(END_FLAG),0,(struct sockaddr*)receiverInfo,addrlen);                   
            }                     
        }
        else{
            if(!strcmp(sendbuf,END_FLAG)){
                printf("End file\n");
                sendto(socketfd,END_FLAG,strlen(END_FLAG),0,(struct sockaddr*)&receiverInfo,addrlen);            
                break;
            }              
        }        
        counter++;        
    }   
    alarm(0);

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

    struct sigaction act;
    act.sa_handler = sig_alrm;
    sigemptyset(&act.sa_mask);
    sigaddset(&act.sa_mask,SIGALRM);    
    act.sa_flags = SA_INTERRUPT;
    sigaction(SIGALRM,&act,0);    

    printf("Waiting for receiver online\n");
    //Check is receiver Online
    while(1){        
        bzero(buf,sizeof(buf));
        sendto(socketfd,argv[3],strlen(argv[3]),0,(struct sockaddr*)&receiverInfo,addrlen);

        alarm(1);
        if(recvfrom(socketfd,buf,sizeof(buf),0,(struct sockaddr*)&receiverInfo,&addrlen)==-1){
            if(errno = EINTR){
                //resend                    
                printf("Receiver is not online.\n");                
                continue;
            }
            else{
               printf("Recv error\n");                
            }            
        }
        else{
            if(!strcmp(buf,argv[3])){
                printf("Receiver found.  IP: %s Port: %d\n",inet_ntoa(receiverInfo.sin_addr),ntohs(receiverInfo.sin_port));    
                break;
            }        
            continue;            
        }
    }
    alarm(0);

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