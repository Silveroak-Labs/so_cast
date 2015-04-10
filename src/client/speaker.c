
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <sys/timeb.h>
#include <pthread.h>
#include <alsa/asoundlib.h> 
#include "socast.h"

#define OK_STR "OK"
#define BC_IP "255.255.255.255"
#define PORT 8883

#define MAX_SIZE 128


#define RATE 44100
#define CHANNELS 2
#define LATENCY 500000

static char *device = "default";                        /* playback device */

snd_output_t *output = NULL;

snd_pcm_t *handle;

snd_pcm_uframes_t frames;  

unsigned char *HOST_IP;

int FRAME_INDEX = 0;

list_t *FRAME_LIST;

//写pcm 的条件
int is_write_to_pcm=0;




void close_playback(){
    snd_pcm_drain(handle); 
    snd_pcm_close(handle);
}

void init_playback(){
    int err;

    if ((err = snd_pcm_open(&handle, device, SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
            printf("Playback open error: %s\n", snd_strerror(err));
            exit(EXIT_FAILURE);
    }
    if ((err = snd_pcm_set_params(handle,
                                  SND_PCM_FORMAT_S16_LE,
                                  SND_PCM_ACCESS_RW_INTERLEAVED,
                                  CHANNELS,
                                  RATE,
                                  1,
                                  LATENCY)) < 0) {   /* 0.5sec */
            printf("Playback open error: %s\n", snd_strerror(err));
            exit(EXIT_FAILURE);
    }
    //open audio file
    int ret;
    frames = MAX_FRAMES/4;
}

void write_to_buffer(unsigned char *buffer){
    int ret;
    while ((ret = snd_pcm_writei(handle, buffer, frames)) < 0) {
          snd_pcm_prepare(handle);
          fprintf(stderr, "<<<<<<<<<<<<<<< Buffer Underrun >>>>>>>>>>>>>>>\n");
    }
    printf("Test ret:%d, frames %d\n",ret,frames);
}

/*
   当Thread2，有数据后开始启动该thread
Thread3. 从FRAME_LIST中获取数据写入到pcm中 
*/
void *write_pcm(void *msg){
    // init_playback();
    int index = 0;
    list_iterator_t *it = list_iterator_new(FRAME_LIST, LIST_HEAD);
  int tindex=0;
  for(tindex=0;tindex<FRAME_LIST->len;tindex++){
     list_node_t *a = list_iterator_next(it);
    so_play_frame *data = (so_play_frame *)a->val;
    printf("timestamp = %lld frames len = %lu \n frame = %s\n",data->timestamp,sizeof(data->frames),data->frames);
}

    // while(is_write_to_pcm){
    //     list_node_t *node = list_at(FRAME_LIST,index);
    //     if(node){
    //         index++;
    //         // char *frames = ((so_play_frame *)node->val)->frames;
    //         printf("frames = %s\n",((so_play_frame *)node->val)->frames);
    //         write_to_buffer(((so_play_frame *)node->val)->frames);
    //     }
    // }
    // close_playback();
    return NULL;
}


//Thread2. 发送hostip 去获取数据,放入到FRAME_LIST中
void *request_data(void *msg){
   struct sockaddr_in servaddr;
   int sock_c;
   int addr_len;
   int recv_len;
   

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

    addr_len = sizeof(servaddr);
    //向hostip 发送请求消息

    char index[16];
    char cRecvBuf[MAX_FRAMES*2];
    so_play_frame *rframe;
    
    pthread_t write_pcm_p;


    for(FRAME_INDEX=0;;FRAME_INDEX++){
        // printf("request frame index = %d\n",FRAME_INDEX);
        sprintf(index,"%d\0",FRAME_INDEX);

        recv_len = sendto(sock_c, index, strlen(index), 0, (struct sockaddr*)&servaddr, addr_len);
        bzero(cRecvBuf, MAX_FRAMES*2);
        recv(sock_c, cRecvBuf, sizeof(cRecvBuf)+1, 0);
        rframe=(so_play_frame *)cRecvBuf;
        // printf("rframe timestamp = %lld value= %s\n",rframe->timestamp,rframe->frames);
        list_node_t *node = list_node_new(rframe);
        list_rpush(FRAME_LIST, node);
        if(is_write_to_pcm==0){
            is_write_to_pcm = 1;
            //启动thread3 write_pcm
            pthread_create(&write_pcm_p,NULL,write_pcm,NULL);

        }
        
    }
    
}


//1. 监听广播
void *listent_socket()
{
    int sockfd;
    struct sockaddr_in servaddr;
    socklen_t socklen = sizeof(struct sockaddr_in);
    struct ip_mreq mreq;


    if ((sockfd=socket(AF_INET,SOCK_DGRAM,0)) < 0)   
    {  
        perror("socket");  
        exit(1);  
    }  
    //close hsa_syslogd release 18879 port
    ///* allow multiple sockets to use the same PORT number */ 

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr=htonl(INADDR_ANY); 
    servaddr.sin_port=htons(PORT);  

    if(bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) == -1)  {
        perror("bind error");
        close(sockfd);
        sockfd = -1;
    }
    /* use setsockopt() to request that the kernel join a multicast group */  
    mreq.imr_multiaddr.s_addr=inet_addr(BC_IP);  
    mreq.imr_interface.s_addr=htonl(INADDR_ANY);  
    if (setsockopt(sockfd,IPPROTO_IP,IP_ADD_MEMBERSHIP,&mreq,sizeof(mreq)) < 0)   
    {  
        perror("setsockopt");  
        exit(1);  
    }  
    int length;
    char rervBuffer[MAX_SIZE];
    while(1){
        bzero(rervBuffer, MAX_SIZE);
        length = recvfrom(sockfd, rervBuffer, MAX_SIZE, 0,(struct sockaddr *) &servaddr, &socklen);
        if (length <= 0)
        {
            printf("Server Recieve Data complete!\n");
            break;
        }else {
            //todo 根据接收到的信号，初始化FRAME_LIST
            list_destroy(FRAME_LIST);
             FRAME_INDEX = 0;
             FRAME_LIST = list_new();
             //根据接受的信号，启动定时播放

             //todo 如果是结束的
             is_write_to_pcm = 0;
        }
     }
     close(sockfd);
}

int main(int argc,char *argv[]){

    FRAME_LIST = list_new();
    //Thread1. 监听广播开始获取
    //listent_socket();

    //指定host ip
    HOST_IP = argv[1];
    //Thread2. 发送hostip 去获取数据,放入到FRAME_LIST中

    pthread_t request_data_p,read_p;

    pthread_create(&request_data_p,NULL,request_data,NULL);
  
    pthread_join(request_data_p,NULL);
   
    
    return 0;
}
