
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <sys/timeb.h>

#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <pthread.h>
#include "socast.h"

#define OK_STR "OK"
#define BC_IP "255.255.255.255"
#define PORT 8883

#define MAX_SIZE 128

#define TIMEOUT 50000  //50 ms


unsigned char *HOST_IP;

int FRAME_INDEX = 0;

list_t *FRAME_LIST;

//写pcm 的条件
int is_write_to_pcm=0;


long long getSystemTime() {
    struct timeb t;
    ftime(&t);
    return 1000 * t.time + t.millitm;
}


//Thread2. 发送hostip 去获取数据,放入到FRAME_LIST中
void *request_data(void *msg){
   struct sockaddr_in servaddr;
   int sock_c;
   int recv_len;
   
  socklen_t socklen = sizeof(struct sockaddr_in);

   /* create what looks like an ordinary UDP socket */  
   if ((sock_c=socket(AF_INET,SOCK_DGRAM,0)) < 0)   
   {  
      perror("socket");  
      exit(1);  
   }  
   printf("msg = %s\n",msg);
   /* Construct the server sockaddr_in structure */
   memset(&servaddr, 0, sizeof(servaddr));       /* Clear struct */
   servaddr.sin_family = AF_INET;                  /* Internet/IP */
   servaddr.sin_addr.s_addr = inet_addr(HOST_IP);  /* IP address */
   servaddr.sin_port = htons(PORT);       /* server */

    //向hostip 发送请求消息

    char index[16];
    unsigned char *cRecvBuf;
    so_play_frame *rframe;
    
    pthread_t write_pcm_p;

    int REC_BUF_LEN = sizeof(so_play_frame);

    /**

    =======
    **/
    struct timeval tv;
    fd_set readfds;

    int ret;

    

    tv.tv_sec = 0;
    tv.tv_usec = TIMEOUT;

    /**
    =======
    **/
    bzero(index,16);
    sprintf(index,"%d",FRAME_INDEX);
    printf("sendto timestamp = %lld\n",getSystemTime());
    recv_len = sendto(sock_c, index, strlen(index), 0, (struct sockaddr*)&servaddr, sizeof(servaddr));

    for(FRAME_INDEX=0;;){
         // printf("request frame index = %d\n",FRAME_INDEX);
        tv.tv_sec = 0;
        tv.tv_usec = TIMEOUT;
        

        FD_ZERO(&readfds);
        FD_SET(sock_c,&readfds);

        ret = select (sock_c+1,
                &readfds,
                NULL,
                NULL,
                &tv
        );

        if(ret==-1){
            perror("select error");
            return NULL;
        } else if(!ret){
            printf("%d seconds elapsed. %d timestamp = %lld\n", TIMEOUT,FRAME_INDEX,getSystemTime());
            bzero(index,16);
    sprintf(index,"%d",FRAME_INDEX);
    printf("sendto timestamp = %lld\n",getSystemTime());
    recv_len = sendto(sock_c, index, strlen(index), 0, (struct sockaddr*)&servaddr, sizeof(servaddr));
            continue;
        }

        if(FD_ISSET(sock_c, &readfds)){
            printf("recv timestamp = %lld\n",getSystemTime());

            cRecvBuf = (unsigned char *)malloc(REC_BUF_LEN);
            recv_len = recvfrom(sock_c, cRecvBuf, REC_BUF_LEN+1, 0, (struct sockaddr*)&servaddr,&socklen);
            if(recv_len>0){
                 rframe=(so_play_frame *)cRecvBuf;
                 if(rframe->sequence>=FRAME_INDEX || (FRAME_INDEX>=MAX_INDEX && rframe->sequence<FRAME_INDEX)){
                    // printf("rframe timestamp = %lld value= %s\n",rframe->timestamp,rframe->frames);
                    list_node_t *node = list_node_new(rframe);
                    // printf("cRecvBuf = %p ,index = %d ,node.timestamp = %p\n",cRecvBuf,FRAME_LIST->len,((so_play_frame *)node->val)->frames);
                    list_rpush(FRAME_LIST, node);
                    FRAME_INDEX++;
                    printf("sendto timestamp = %lld\n",getSystemTime());
                    bzero(index,16);
                    sprintf(index,"%d",FRAME_INDEX);
                    recv_len = sendto(sock_c, index, strlen(index), 0, (struct sockaddr*)&servaddr, sizeof(servaddr));
                 }
                
            }else{
                free(cRecvBuf);
            }
        }
       
    }
}





int main(int argc,char *argv[]){

    FRAME_LIST = list_new();
    //Thread1. 监听广播开始获取
    //listent_socket();

    //指定host ip
    HOST_IP = argv[1];
    //Thread2. 发送hostip 去获取数据,放入到FRAME_LIST中

    request_data(NULL);

    return 0;
}
