#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 
#include<sys/stat.h>
#include <fcntl.h>
  
#define PORT     8080 
#define MAXLINE 1024 
// 1 is read
// 2 is write
// 3 is data
// 4 is ack
// 5 is error

int port,sockfd,fd,pos_in_file;
char* filename;
char *block_number;
void init_bn(){
    block_number[0]=1;
    block_number[1]=1;
    block_number[2]='\0';
}
void increment_bn(){
    if(block_number[1]<255)block_number[1]++;
    else if(block_number[0]<255){
        block_number[0]++;
        block_number[1]=1;
    }
}

int isnum(char *s){
    while(*s!='\0'){
        if(*s>'9' || *s<'0')return 0;
        s++;
    }
    return 1;
}

int port_validator(char *port){
    if(isnum(port)==0)return -1;
    int portint=atoi(port);
    if(portint<0 || portint>((1<<16)-3))return -1;
    return portint;
}

//response is data and a file is opened
void response_read_request(struct sockaddr_in cliaddr,int len){
    char *buffer= malloc(600); 
    char* toput=malloc(600);
        read(fd,buffer,512);
        toput[0]='0';
        toput[1]='3';
        toput[2]='3';
        toput[3]='7';
        char *toputitr=toput+4;
        while(*buffer!='\0'){
            *toputitr=*buffer;
            buffer++;
            toputitr++;
        }
        *toputitr='\0';
        increment_bn();
        toput[2]=block_number[0];
        toput[3]=block_number[1];
        int x=sendto(sockfd, (const char *)toput, strlen(toput),  
            MSG_CONFIRM, (const struct sockaddr *) &cliaddr, 
                len); 
}

//response is acknowledgement
void response_write_request(struct sockaddr_in cliaddr,int len){ 
    char* toput=malloc(5); 
    increment_bn();
    toput[0]='0';
    toput[1]='4';
    toput[2]=block_number[0];
    toput[3]=block_number[1];
    toput[4]='\0';
    sendto(sockfd, (const char *)toput, strlen(toput),  
        MSG_CONFIRM, (const struct sockaddr *) &cliaddr, 
            len); 
}

//response is data
int response_acknowledgement(struct sockaddr_in cliaddr,int len){
    char *buffer= malloc(600); 
    char* toput=malloc(600);
        int y=read(fd,buffer,512);
        toput[0]='0';
        toput[1]='3';
        toput[2]='3';
        toput[3]='7';
        char *toputitr=toput+4;
        while(*buffer!='\0'){
            *toputitr=*buffer;
            buffer++;
            toputitr++;
        }
        *toputitr='\0';
        increment_bn();
        toput[2]=block_number[0];
        toput[3]=block_number[1];
        int x=sendto(sockfd, (const char *)toput, strlen(toput),  
                    MSG_CONFIRM, (const struct sockaddr *) &cliaddr, 
                    len);
        if(strlen(toput)==4)return -1;
        return 0;
}


int main(int argc,char **argv) {
    block_number=malloc(5);
    if(argc!=2){
        printf("Execute in the following way\n./server <port>\n");
        return -1;
    } 
    int port=port_validator(argv[1]);
    if(port<0){
        printf("PORT INVALID\nabortin...\n");
        return -1;
    }
    struct sockaddr_in servaddr, cliaddr; 
      
    // Creating socket file descriptor 
    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) { 
        perror("socket creation failed"); 
        exit(EXIT_FAILURE); 
    } 
      
    memset(&servaddr, 0, sizeof(servaddr)); 
    memset(&cliaddr, 0, sizeof(cliaddr)); 
      
    // Filling server information 
    servaddr.sin_family    = AF_INET; // IPv4 
    servaddr.sin_addr.s_addr = INADDR_ANY; 
    servaddr.sin_port = htons(port); 
      
    // Bind the socket with the server address 
    if ( bind(sockfd, (const struct sockaddr *)&servaddr,sizeof(servaddr)) < 0 ){ 
        perror("bind failed"); 
        exit(EXIT_FAILURE); 
    }
    filename=malloc(10000);
    while(1){
        char *buffer=malloc(1000);  
        int len, n; 
        len = sizeof(cliaddr);  
        n = recvfrom(sockfd,(char *)buffer,MAXLINE,MSG_WAITALL,( struct sockaddr *) &cliaddr,&len); 
        init_bn();
        if(buffer[1]=='1'){
            for(int i=2;i<=n-7;i++)filename[i-2]=buffer[i];
            filename[n-9]='\0';
            // printf("%s\n",filename);
            fd=open(filename,O_RDONLY,0777);
            response_read_request(cliaddr,len);
            while(1){
                len = sizeof(cliaddr);  
                memset(&cliaddr,0,sizeof(cliaddr));
                n = recvfrom(sockfd,(char *)buffer,MAXLINE,MSG_WAITALL,( struct sockaddr *) &cliaddr,&len); 
                // printf("n=%d\n",n);
                buffer[n] = '\0';
                if(response_acknowledgement(cliaddr,len)==-1)break;
            }
        }
        else if(buffer[1]=='2'){
            for(int i=2;i<=n-7;i++)filename[i-2]=buffer[i];
            filename[n-9]='\0';
            fd=open(filename,O_RDWR|O_CREAT,0777);
            response_write_request(cliaddr,len);
            while(1){
                len=sizeof(cliaddr);
                memset(&cliaddr,0,sizeof(cliaddr));
                n = recvfrom(sockfd,(char *)buffer,MAXLINE,MSG_WAITALL,( struct sockaddr *) &cliaddr,&len); 
                buffer[n]='\0';
                char *temp=buffer+4;
                int strlen=0;
                char *tempitr=temp;
                while(*tempitr!='\0')tempitr++,strlen++;
                if(strlen==0)break;
                write(fd,temp,strlen);
                response_write_request(cliaddr,len);
            }
            close(fd);
        }
        
    }
    return 0; 
} 