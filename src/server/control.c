/*
   1. 启动主要处理的udpsocket  
       消息通知： 
          开始（时间，rate，采样位数，声道数），
          停止（时间）
       发送数据流，顺序发送（失败的话重新发送）
   2. 当有client 过来连接，记录client channel
*/
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#define GROUP_IP "225.0.0.37"
#define PORT 8883

#define MAX_SIZE 1024

int sock_c;

unsigned char *buffer; 

void *send_c(void *msg){
    send(sock_c, buffer, MAX_SIZE, 0);
    return NULL;
}

void Die(char *mess) { perror(mess); exit(1); }

int main(int argc, char *argv[]) {
  
  struct sockaddr_in echoserver;
  int received = 0;

  /* create what looks like an ordinary UDP socket */  
    if ((sock_c=socket(AF_INET,SOCK_DGRAM,0)) < 0)   
    {  
        perror("socket");  
        exit(1);  
    }  

  /* Construct the server sockaddr_in structure */
  memset(&echoserver, 0, sizeof(echoserver));       /* Clear struct */
  echoserver.sin_family = AF_INET;                  /* Internet/IP */
  echoserver.sin_addr.s_addr = inet_addr(GROUP_IP);  /* IP address */
  echoserver.sin_port = htons(PORT);       /* server port */
 
  //open audio file
  int ret;
  buffer = (unsigned char *)malloc(MAX_SIZE);

  int i;
  for (i = 0; 1; i++) {
      //data num_frames
      ret = read(0, buffer, MAX_SIZE);
      if(ret<=0){
          printf("muisc play end\n");
          break;
      }   
      printf("i=%d\n",i);
      if(sendto(sock_c,buffer, MAX_SIZE, 0, (struct sockaddr *) &echoserver, sizeof(echoserver))<0){
         perror("sendto");  
         break;
      }
  }
  
  /* Send the word to the server */
  /* Receive the word back from the server */
  fprintf(stdout, "Received: ");
  fprintf(stdout, "\n");
  close(sock_c);
  free(buffer);
  exit(0);
}
