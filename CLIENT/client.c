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
char *ip_address;
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
    if(portint<0 || portint>((1<<16)-1))return -1;
    return portint;
}

char* read_request(char *filename,char *mode){
    struct sockaddr_in servaddr; 
    memset(&servaddr, 0, sizeof(servaddr)); 
    char *hello=malloc(1000);
    char *response=malloc(520);
    
    //To assign op code
    hello[0]='0';
    hello[1]='1';
    
    char* helloitr=hello+2;
    while(*filename!='\0'){
        *helloitr=*filename;
        helloitr++;
        filename++;
    }
    *helloitr='0';
    helloitr++;
    //To assign mode of file
    while (*mode!='\0'){
        *helloitr=*mode;
        mode++;
        helloitr++;
    }
    *helloitr='0';
    helloitr++;
    *helloitr='\0';
    // Filling server information 
    servaddr.sin_family = AF_INET; 
    servaddr.sin_port = htons(port); 
    servaddr.sin_addr.s_addr = inet_addr(ip_address); 
    int n, len; 
    sendto(sockfd, (const char *)hello, strlen(hello), 
        MSG_CONFIRM, (const struct sockaddr *) &servaddr,  
            sizeof(servaddr));     
    n = recvfrom(sockfd, (char *)response, MAXLINE,  
                MSG_WAITALL, (struct sockaddr *) &servaddr, 
                &len);
    return response;
}
char* write_request(char *filename,char *mode){
    struct sockaddr_in servaddr; 
    memset(&servaddr, 0, sizeof(servaddr)); 
    char *hello=malloc(1000);
    char *response=malloc(520);
    hello[0]='0';
    hello[1]='2';
    char* helloitr=hello+2;
    while(*filename!='\0'){
        *helloitr=*filename;
        helloitr++;
        filename++;
    }
    *helloitr='0';
    helloitr++;
    //To assign mode of file
    while (*mode!='\0'){
        *helloitr=*mode;
        mode++;
        helloitr++;
    }
    *helloitr='0';
    helloitr++;
    *helloitr='\0';
    // Filling server information 
    servaddr.sin_family = AF_INET; 
    servaddr.sin_port = htons(port); 
    servaddr.sin_addr.s_addr = inet_addr(ip_address); 
    int n, len; 
    sendto(sockfd, (const char *)hello, strlen(hello), 
        MSG_CONFIRM, (const struct sockaddr *) &servaddr,  
            sizeof(servaddr));
          
    n = recvfrom(sockfd, (char *)response, MAXLINE,  
                MSG_WAITALL, (struct sockaddr *) &servaddr, 
                &len);
    return response;
}
char* acknowledgement(){
    struct sockaddr_in servaddr; 
    memset(&servaddr, 0, sizeof(servaddr)); 
    char *hello=malloc(1000);
    char *response=malloc(520);
    //To assign op code
    hello[0]='0';
    hello[1]='4';
    hello[2]='3';
    hello[3]='7';
    hello[4]='\0';
    servaddr.sin_family = AF_INET; 
    servaddr.sin_port = htons(port); 
    servaddr.sin_addr.s_addr = inet_addr(ip_address); 
    int n=0, len; 
    increment_bn();
    hello[2]=block_number[0];
    hello[3]=block_number[1];
    sendto(sockfd, (const char *)hello, strlen(hello), 
        MSG_CONFIRM, (const struct sockaddr *) &servaddr,  
            sizeof(servaddr));
    n=recvfrom(sockfd,(char *)response,MAXLINE,
                        MSG_WAITALL,(struct sockaddr *)&servaddr,
                        &len);
    return response;
}
char* send_data(char *data){
    struct sockaddr_in servaddr; 
    memset(&servaddr, 0, sizeof(servaddr)); 
    char *hello=malloc(1000);
    char *response=malloc(520);
    hello[0]='0';
    hello[1]='3';
    hello[2]='3';
    hello[3]='7';
    char* helloitr=hello+4;
    while(*data!='\0'){
        *helloitr=*data;
        helloitr++;
        data++;
    }
    *helloitr='\0';
    increment_bn();
    hello[2]=block_number[0];
    hello[3]=block_number[1];
    // Filling server information 
    servaddr.sin_family = AF_INET; 
    servaddr.sin_port = htons(port); 
    servaddr.sin_addr.s_addr = inet_addr(ip_address); 
    int n, len; 
    int x=sendto(sockfd, (const char *)hello, strlen(hello), 
        MSG_CONFIRM, (const struct sockaddr *) &servaddr,  
            sizeof(servaddr));
    n = recvfrom(sockfd, (char *)response, MAXLINE,  
                MSG_WAITALL, (struct sockaddr *) &servaddr, 
                &len);
    return response;
}

int main(int argc,char ** argv) {
    block_number=malloc(5);
    if(argc!=3){
        printf("IP address and port number in the following format::\n./client <IP addr> <Port No.>\n");
        return -1;
    } 
    port=port_validator(argv[2]);
    ip_address=argv[1];
    if(port==-1){
        printf("PORT b/w 0 and 65533\naborting ...\n");
        return -1;
    } 
    // Creating socket file descriptor 
    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) { 
        perror("socket creation failed"); 
        exit(EXIT_FAILURE); 
    } 
    filename=malloc(1000);
    while(1){
        printf("Available Functions:\n1)GET \n2)PUT \n3)Exit\n");
        printf("Enter 1 or 2 or 3\n");
        init_bn();
        int x;
        scanf("%d",&x);
        if(x==3)break;
        if(x==1){
            printf("Enter file name\n");
            scanf("%s",filename);
            // printf("%s\n",filename);
            char *response=read_request(filename,"ascii");
            fd=open(filename,O_RDWR|O_CREAT,0777);
            char *towritetofile=malloc(600);
            char *towritetofileitr=towritetofile;
            char *responseitr=response+4;
            while(*responseitr!='\0'){
                *towritetofileitr=*responseitr;
                responseitr++;
                towritetofileitr++;
            }
            *towritetofileitr='\0';
            write(fd,towritetofile,strlen(towritetofile));
            while(strlen(response=acknowledgement())>4){
                responseitr=response+4;
                towritetofileitr=towritetofile;
                while(*responseitr!='\0'){
                    *towritetofileitr=*responseitr;
                    responseitr++;
                    towritetofileitr++;
                }
                *towritetofileitr='\0';
                write(fd,towritetofile,strlen(towritetofile));     
            }
            close(fd); 
            continue;
        }
        printf("Enter file name\n");
        scanf("%s",filename);
        fd=open(filename,O_RDONLY,0777);
        if(fd<0){
            printf("The file to be put does not exist\n");
            continue;
        }
        char *response=write_request(filename,"ascii");
        while(1){
            char *data=malloc(1000);
            int count=read(fd,data,512);
            printf("count = %d\n",count);
            data[count]='\0';
            if(count==0)break;
            send_data(data);
            printf("THE CURRENT BLOCK IS %d\n",block_number[0]*256+block_number[1]);
        }
        close(fd);
    }
    close(sockfd); 
    return 0; 
} 