
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
#include <alsa/asoundlib.h> 
#include "socast.h"



#define MAX_SIZE 1024

#define TIMEOUT 50000  //50 ms


#define RATE 44100
#define CHANNELS 2
#define LATENCY 500000

static char *device = "default";                        /* playback device */

snd_pcm_t *handle;

snd_pcm_uframes_t frames;  

unsigned char *HOST_IP;

int FRAME_INDEX = 0;

list_t *FRAME_LIST;

//写pcm 的条件
int IS_WRITE_TO_PCM = 0;

int IS_REQUEST = 0;


long long getSystemTime() {
    struct timeb t;
    ftime(&t);
    return 1000 * t.time + t.millitm;
}

/**
   关闭pcm
*/
void close_playback(){
    printf("close_playback\n");
    snd_pcm_drain(handle); 
    snd_pcm_close(handle);
}

/**
   初始化 pcm 
*/
void init_playback(){
    printf("init_playback\n");
    int err;

    if ((err = snd_pcm_open(&handle, device, SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
            printf("Playback open error: %s\n", snd_strerror(err));
            // exit(EXIT_FAILURE);
    }
    if ((err = snd_pcm_set_params(handle,
                                  SND_PCM_FORMAT_S16_LE,
                                  SND_PCM_ACCESS_RW_INTERLEAVED,
                                  CHANNELS,
                                  RATE,
                                  1,
                                  LATENCY)) < 0) {   /* 0.5sec */
            printf("Playback open error: %s\n", snd_strerror(err));
            // exit(EXIT_FAILURE);
    }
    //open audio file
    int ret;
    frames = MAX_FRAMES/4;
}

/**
   向pcm缓存中写数据
*/
void write_to_buffer(unsigned char *buffer){
    // printf("write_to_buffer\n");
    int ret;
    while (IS_WRITE_TO_PCM && (ret = snd_pcm_writei(handle, buffer, frames)) < 0) {
          snd_pcm_prepare(handle);
          fprintf(stderr, "<<<<<<<<<<<<<<< Buffer Underrun >>>>>>>>>>>>>>>\n");
    }
    // printf("Test ret:%d, frames %d\n",ret,frames);
}

/*
   当Thread2，有数据后开始启动该thread
Thread3. 从FRAME_LIST中获取数据写入到pcm中 
*/
void *write_pcm(void *msg){
    printf("write_pcm\n");
    init_playback();
    int index = 0;
    list_node_t *node;
    while(IS_WRITE_TO_PCM){
        node = list_at(FRAME_LIST,index);
        if(node){
            // printf("index = %d ,node = %p , node->val = %p, frames = %s\n",index,node, node->val, ((so_play_frame *)node->val)->frames);
            write_to_buffer(((so_play_frame *)node->val)->frames);
            index++;
        }
    }
    close_playback();
    return NULL;
}



int send_to_socast(int socket, char index[16], void *servaddr){
        bzero(index,16);
        sprintf(index,"%d",FRAME_INDEX);
        // printf("sendto timestamp = %lld\n",getSystemTime());
        return sendto(socket, index, strlen(index), 0, (struct sockaddr*)servaddr, sizeof(struct sockaddr));
}

//Thread2. 发送hostip 去获取数据,放入到FRAME_LIST中
void *request_data(void *msg){
   struct sockaddr_in servaddr;
   int sock_c;
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

    //向hostip 发送请求消息
    char index[16];
    unsigned char *cRecvBuf;
    so_play_frame *rframe;
    
    pthread_t write_pcm_p;

    int REC_BUF_LEN = sizeof(so_play_frame);

    list_node_t *node;
    so_play_frame *tmpframe;

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
    //第一次发送请求,重传成功后开始请求下面一个，
    send_to_socast(sock_c,index,&servaddr);
    for(FRAME_INDEX=0;IS_REQUEST;){
         // printf("request frame index = %d\n",FRAME_INDEX);
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
            // printf("%d us elapsed. %d timestamp = %lld\n", TIMEOUT,FRAME_INDEX,getSystemTime());
            tv.tv_sec = 0;
            tv.tv_usec = TIMEOUT;
            send_to_socast(sock_c,index,&servaddr);
            continue;
        }

        if(FD_ISSET(sock_c, &readfds)){

            cRecvBuf = (unsigned char *)malloc(REC_BUF_LEN);
            recv_len = recv(sock_c, cRecvBuf, REC_BUF_LEN+1, 0);
            if(recv_len>0){
                 rframe=(so_play_frame *)cRecvBuf;
                 if(rframe->sequence>=FRAME_INDEX || (FRAME_INDEX>=MAX_INDEX && rframe->sequence<FRAME_INDEX)){
                    if(rframe->sequence>=FRAME_LIST->len){
                         // printf("rframe timestamp = %lld value= %s\n",rframe->timestamp,rframe->frames);
                        node = list_node_new(rframe);
                        // printf("cRecvBuf = %p ,index = %d ,node.timestamp = %p\n",cRecvBuf,FRAME_LIST->len,((so_play_frame *)node->val)->frames);
                        list_rpush(FRAME_LIST, node);
                        FRAME_INDEX++;
                    }else{
                        node = list_at(FRAME_LIST,rframe->sequence);
                        if(node){
                           tmpframe = (so_play_frame *)node->val;
                        }
                    }
                   
                    //index最大值为MAX_INDEX 
                    if( index >= MAX_INDEX){
                       index=0;
                    }
                    send_to_socast(sock_c,index,&servaddr);
                 }else{
                    free(cRecvBuf);
                 }
                if(IS_WRITE_TO_PCM == 0 && FRAME_INDEX == 1){
                    IS_WRITE_TO_PCM = 1;
                    //启动thread3 write_pcm
                    //todo 定时启动 write pcm
                    printf("first frame timestamp = %lld\n",rframe->timestamp);
                    pthread_create(&write_pcm_p,NULL,write_pcm,NULL);
                }
            }else{
                free(cRecvBuf);
            }
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

    //多线程启动数据
    pthread_t request_data_p;

  


    if ((sockfd=socket(AF_INET,SOCK_DGRAM,0)) < 0)   
    {  
        perror("socket");  
        exit(1);  
    }  
    //close hsa_syslogd release 18879 port
    ///* allow multiple sockets to use the same PORT number */ 

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr=INADDR_ANY; 
    servaddr.sin_port=htons(PORT_C);  

    if(bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) == -1)  {
        perror("bind error");
        close(sockfd);
        sockfd = -1;
    }
    /* use setsockopt() to request that the kernel join a multicast group */  
    int set = 1;
    if (setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&set,sizeof(int)) < 0)   
    {  
        perror("setsockopt");  
        exit(1);  
    }  
    int length;
    char rervBuffer[MAX_SIZE];
    while(1){
        bzero(rervBuffer, MAX_SIZE);
        length = recvfrom(sockfd, rervBuffer, MAX_SIZE, 0,(struct sockaddr *) &servaddr, &socklen);
        if (length <= 3)
        {
            printf("Server Recieve Data error!\n");
        }else {
            printf("recv buffer = %s\n",rervBuffer);
            if(strncmp(rervBuffer,CMD_START,strlen(CMD_START))==0){
                printf("start request\n");
                IS_REQUEST = 1;
                pthread_create(&request_data_p,NULL,request_data,NULL);
            }
            else if(strncmp(rervBuffer,CMD_STOP,strlen(CMD_STOP))==0){
                printf("stop request\n");

                IS_REQUEST = 0;
                IS_WRITE_TO_PCM = 0;
                 //todo 根据接收到的信号，初始化FRAME_LIST
                list_destroy(FRAME_LIST);
                FRAME_LIST = list_new();
            }
        }
     }
     close(sockfd);
}



int main(int argc,char *argv[]){

    FRAME_LIST = list_new();
    if(argc<2){
        printf("./speaker (host_ip)\n\n");
        return 0;
    }
    //指定host ip
    HOST_IP = argv[1];
    //Thread2. 发送hostip 去获取数据,放入到FRAME_LIST中

    //Thread1. 监听广播开始获取
    listent_socket();

    

    // pthread_t request_data_p,read_p;

    // pthread_create(&request_data_p,NULL,request_data,NULL);
  
    // pthread_join(request_data_p,NULL);
   
    
    return 0;
}
