#include <sys/types.h>
#include <sys/socket.h> 
#include <string.h>
#include <netinet/in.h> 
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h> 
#include <time.h>


char END_FLAG[10]="END_FILE";

int main(int argc, char const *argv[])
{    

    while(1){


    int socketfd = 0;
    printf("Socket Creating...\n");
    if((socketfd = socket(AF_INET,SOCK_DGRAM,0))==-1){
        perror("Socket create failed");
        exit(1);
    }

    struct sockaddr_in senderInfo,receiverInfo; 
    bzero(&receiverInfo, sizeof(receiverInfo)); 
    receiverInfo.sin_family = AF_INET;
    receiverInfo.sin_addr.s_addr=htonl(INADDR_ANY);
    receiverInfo.sin_port = htons(atoi(argv[1]));
    int addrlen = sizeof(receiverInfo);


    printf("Binding...\n");    
    if(bind(socketfd,(struct sockaddr*)&receiverInfo,sizeof(receiverInfo))==-1){
        perror("Binding failed\n");
        exit(1);
    }


    char buf[1025]; 
    char filename[100];
    bzero(filename,sizeof(filename));
    //waiting for sender
    if((recvfrom(socketfd,filename,sizeof(filename),0,(struct sockaddr*)&senderInfo,&addrlen))<0){
        printf("recv error\n");
    }
    else{
        printf("File name :%s\n", filename);            

        if((sendto(socketfd,filename,strlen(filename),0,(struct sockaddr*)&senderInfo,addrlen))<0){
            printf("error\n");
        }
    }


    FILE *fp;
    //char temp[6]="copy ";
    //strcat(temp,filename);
    //fp = fopen(temp,"wb+");
    
    fp = fopen(filename,"wb+");


    int nbytes;
    int i=0;

    while((nbytes= recvfrom(socketfd,buf,sizeof(buf),0,(struct sockaddr*)&senderInfo,&addrlen))>0)
    {   
        char ack[1];    

        if(!strcmp(buf,filename)){
            printf("File name send before\n");
            sendto(socketfd,filename,strlen(filename),0,(struct sockaddr*)&senderInfo,addrlen);
            continue;
        }

        if(!strcmp(buf,END_FLAG)){
            printf("End file\n");                        
            sendto(socketfd,END_FLAG,strlen(END_FLAG),0,(struct sockaddr*)&senderInfo,addrlen);
            break;
        }                    

        printf("Pack %d Received.\nReceiving %d bytes\n", buf[nbytes-1],nbytes);  
        if(i%50 == buf[nbytes-1]){
            ack[0]=i%50;

            int n = fwrite(buf,sizeof(char),nbytes-1,fp);
            printf("Writing %d bytes\n", n);    
                        
            if((sendto(socketfd,ack,1,0,(struct sockaddr*)&senderInfo,addrlen))==-1){
                perror("Send Ack failed");
                exit(1);
            }
            else{
               printf("Ack num %d send\n\n",ack[0]);            
            }                
            i++;             
        }
        else{
            ack[0]=(i-1)%50;
            printf("%d\n", i);
            printf("Package %d has received already!\n", buf[nbytes-1]);

            if((sendto(socketfd,ack,1,0,(struct sockaddr*)&senderInfo,addrlen))==-1){
                perror("Send Ack failed");
                exit(1);
            }
            else{
                printf("Ack num %d send\n\n",ack[0]);
            }
        }                                        

        bzero(buf,sizeof(buf));
    }    
    close(socketfd);
    fclose(fp);
    }           
    return 0;
}