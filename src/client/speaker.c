
#include <time.h>
#include "socast.h"

/**
speaker 
  1. 增加缓存， 是为了40ms的 数据缓存，有助于前期的数据准备
  2. 增加重传是因为前期的数据，有可能失败，通过重传减少和避免丢失。
      因为要求时间较短因此短期内重试可以有效避免传输慢而丢失的问题

*/

#define TIMEOUT 500000  //50 ms


#define RATE 44100
#define CHANNELS 2
#define LATENCY 50000

static char *PCM_DEVICE = "default";                        /* playback PCM_DEVICE */

snd_pcm_t *PCM_HANDLE;

const snd_pcm_uframes_t PCM_FRAME_NUM = MAX_FRAMES/4;

unsigned char *HOST_IP;

int FRAME_INDEX = 0;

so_play_frame FRAME_LIST[MAX_INDEX];

//写pcm 的条件
int IS_WRITE_TO_PCM = 0;

int IS_REQUEST = 0;

struct timespec SLEEP_TIME = {
    .tv_sec = 0,
    .tv_nsec = 1000000  //1ms

}; //1us ;


int S_MAX = 0;

so_play_cmd *RECV_CMD;


/**
   初始化 pcm 
*/
void init_playback_p(){
    printf("init_playback\n");
    int err;

    if ((err = snd_pcm_open(&PCM_HANDLE, PCM_DEVICE, SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
            printf("Playback open error: %s\n", snd_strerror(err));
            // exit(EXIT_FAILURE);
    }
    if ((err = snd_pcm_set_params(PCM_HANDLE,
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
    printf("========== PCM_FRAME_NUM %d\n",PCM_FRAME_NUM);

}


/**
   向pcm缓存中写数据
*/
void *write_to_buffer(void *msg){
    // printf("write_to_buffer\n");
    int ret;
    int index = 0;
    int s_max = S_MAX;
    int len;
    while(1){
        // printf("index = %d ,node = %p , node->val = %p, frames = %s\n",index,node, node->val, ((so_play_frame *)node->val)->frames);
        if(IS_WRITE_TO_PCM){
            if(index<FRAME_INDEX || (index>=FRAME_INDEX && s_max!=S_MAX)){
                #ifdef DEBUG
                printf("wirte to buffer index %d , timestamp %lu\n",index, getSystemTime());
                printf("frames len %lu\n",strlen(FRAME_LIST[index].frames));
                #endif
                len = strlen(FRAME_LIST[index].frames);
                len = len>MAX_FRAMES?MAX_FRAMES:len;
                FRAME_LIST[index].frames[len+1]='\0';
                while ((ret = snd_pcm_writei(PCM_HANDLE, FRAME_LIST[index].frames, PCM_FRAME_NUM)) < 0) {
                      snd_pcm_prepare(PCM_HANDLE);
                      fprintf(stderr, "<<<<<<<<<<<<<<< Buffer Underrun >>>>>>>>>>>>>>>\n");
                }               
                if(index>=MAX_INDEX-1){
                    s_max++;
                    if(s_max>1000){
                        s_max=0;
                    }
                    index = 0;

                }else{
                    index++;
                }
            }
        }else{
            if(index>0){
               index=0; 
            }
            nanosleep(&SLEEP_TIME,NULL);

        }
    }
    // printf("Test PCM_FRAME_NUM %d ,ret:%lld, \n",PCM_FRAME_NUM, ret);
}

/*
   当Thread2，有数据后开始启动该thread
Thread3. 从FRAME_LIST中获取数据写入到pcm中 
*/
void start_write_pcm(){
    printf("start_write_pcm\n");
    IS_WRITE_TO_PCM = 1;
}
void stop_write_pcm(){
    IS_WRITE_TO_PCM = 0;
}

int send_to_socast(int socket, int idx, void *servaddr){
        char index[16];
        sprintf(index,"%d",idx);
        // printf("sendto timestamp = %lld\n",getSystemTime());
        return sendto(socket, index, strlen(index), 0, (struct sockaddr*)servaddr, sizeof(struct sockaddr));
}

//Thread2. 发送hostip 去获取数据,放入到FRAME_LIST中
void *request_data(void *msg){
   struct sockaddr_in servaddr;
   int sock_c;
   int recv_len;
   FRAME_INDEX = 0;

   /* create what looks like an ordinary UDP socket */  
   if ((sock_c=socket(AF_INET,SOCK_DGRAM,0)) < 0)   
   {  
      perror("socket");  
      exit(1);  
   }  
   /* Construct the server sockaddr_in structure */
   memset(&servaddr, 0, sizeof(servaddr));       /* Clear struct */
   servaddr.sin_family = AF_INET;                  /* Internet/IP */
   servaddr.sin_addr.s_addr = inet_addr(HOST_IP);  /* IP address */
   servaddr.sin_port = htons(PORT);       /* server */

    //向hostip 发送请求消息
    
    unsigned char *cRecvBuf;
    so_play_frame *rframe;
    
    pthread_t write_pcm_p;

    int REC_BUF_LEN = sizeof(so_play_frame);

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
    // send_to_socast(sock_c,FRAME_INDEX,&servaddr);
    FRAME_INDEX=0;
    while(1){
        if(IS_REQUEST){
            if(FRAME_INDEX==0){
                send_to_socast(sock_c,FRAME_INDEX,&servaddr);
            }
            
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
                printf("%d us elapsed. %d\n", TIMEOUT,FRAME_INDEX);
                tv.tv_sec = 0;
                tv.tv_usec = TIMEOUT;
                send_to_socast(sock_c,FRAME_INDEX,&servaddr);
                continue;
            }

            if(FD_ISSET(sock_c, &readfds)){
                bzero(&(FRAME_LIST[FRAME_INDEX].frames),MAX_FRAMES);
                recv(sock_c, &FRAME_LIST[FRAME_INDEX], REC_BUF_LEN+1, 0);
                if(FRAME_LIST[FRAME_INDEX].sequence>=FRAME_INDEX){
                    // write_to_buffer(FRAME_INDEX);
                     //index最大值为MAX_INDEX 
                    if( FRAME_INDEX >= MAX_INDEX-1){
                        S_MAX++;
                        if(S_MAX>1000){
                            S_MAX = 0;
                        }
                       FRAME_INDEX=0;
                    }else{
                        FRAME_INDEX++;
                    }
                    send_to_socast(sock_c,FRAME_INDEX,&servaddr);
                }
                // if(IS_WRITE_TO_PCM == 0 && FRAME_INDEX >=1 && IS_REQUEST == 1){
                //     //启动thread3 write_pcm
                //     //todo 定时启动 write pcm
                //     start_write_pcm();
                // }

            }
        }else{
            if(FRAME_INDEX>0){
               FRAME_INDEX =0; 
            }
            nanosleep(&SLEEP_TIME,NULL);
        }
    }
    
    close(sock_c);
}

void start_request_data(){
    printf("start_request_data\n");
    FRAME_INDEX = 0;
    IS_REQUEST = 1;
}
void stop_request_data(){
    IS_REQUEST = 0;
}

long secends=0;
long usec =0;
void *time_start(void *msg){
    
    printf("1  recv time = ");
    print_timestamp();
    struct timeval current_time;
    // printf("recv seconds = %ld ,useconds = %ld\n",(long) RECV_CMD->current_t.tv_sec, (long)RECV_CMD->current_t.tv_usec);

    gettimeofday(&current_time,NULL);
    secends = (long)(RECV_CMD->current_t.tv_sec - current_time.tv_sec);
    usec =  (long)(RECV_CMD->current_t.tv_usec - current_time.tv_usec);
    if(secends>0){
        if(usec<0){
            secends = secends-1;
            usec = usec+secends*1000000;
        }
    }
    printf("secends = %ld ,usec = %ld\n",secends,usec);
    printf("2  recv time = ");
    print_timestamp();
    const struct timespec spec ={
        .tv_sec = secends,
        .tv_nsec = usec*1000

    };
    printf("3  recv time = ");
    print_timestamp();
    nanosleep(&spec,NULL);
    print_timestamp();
    start_write_pcm();
    printf("4  recv time = ");
    print_timestamp();

}
//1. 监听广播
void *listent_socket(void *msg){
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
    int recvLen = sizeof(so_play_cmd)+1;
    char rervBuffer[recvLen];
    
    pthread_t start_write_p;
    RECV_CMD = malloc(sizeof(so_play_cmd));
    while(1){
        bzero(rervBuffer, recvLen);
        length = recvfrom(sockfd, rervBuffer, recvLen, 0,(struct sockaddr *) &servaddr, &socklen);
        if (length <= 3)
        {
            printf("Server Recieve Data error!\n");
        }else {
            printf("recv buffer = %s\n",rervBuffer);
            so_play_cmd *tmp_cmd = (so_play_cmd *)rervBuffer;
            RECV_CMD->type = tmp_cmd->type;
            RECV_CMD->current_t.tv_sec = tmp_cmd->current_t.tv_sec;
            RECV_CMD->current_t.tv_usec = tmp_cmd->current_t.tv_usec;

            if(RECV_CMD->type==CMD_START){
                printf("start request\n");
                //todo 根据 cmd->current_t 的时间戳来启动
                start_request_data();
                printf("recv seconds = %ld ,useconds = %ld\n",(long) RECV_CMD->current_t.tv_sec, (long)RECV_CMD->current_t.tv_usec);
                pthread_create(&start_write_p,NULL,time_start,NULL);
                pthread_detach(start_write_p);
            }else{
                 printf("stop request\n");
                stop_write_pcm();
                stop_request_data();
            }
        }
     }
     close(sockfd);
}



int main(int argc,char *argv[]){

    if(argc<2){
        printf("./speaker (host_ip)\n\n");
        return 0;
    }
    //void init_playback(snd_pcm_t *handle,char *device,int type,int format,int access,int channels,int rate,int latency);
    init_playback_p();
    //指定host ip
    HOST_IP = argv[1];
    //Thread2. 发送hostip 去获取数据,放入到FRAME_LIST中
    //Thread1. 监听广播开始获取

    pthread_t listen_p,request_data_p,read_p;
    pthread_create(&request_data_p,NULL,request_data,NULL);
    pthread_detach(request_data_p);
    pthread_create(&read_p,NULL,write_to_buffer,NULL);
    pthread_detach(read_p);
    pthread_create(&listen_p,NULL,listent_socket,NULL);
    
    pthread_join(listen_p,NULL);
   
    close_playback(PCM_HANDLE);
    return 0;
}
