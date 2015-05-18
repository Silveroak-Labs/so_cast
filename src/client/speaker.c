
#include <time.h>
#include "socast.h"

/**
speaker 
  1. 增加缓存， 是为了40ms的 数据缓存，有助于前期的数据准备
  2. 增加重传是因为前期的数据，有可能失败，通过重传减少和避免丢失。
      因为要求时间较短因此短期内重试可以有效避免传输慢而丢失的问题

   增加自动查找：
    1. 发送广播，等待回复获取IP  端口PORT_B
    2. 监听控制端口接收命令控制 起线程一直监听  控制端口PORT_C
       CMD_START
       CMD_STOP
*/

#define TIMEOUT 500000  //50 ms


#define RATE 44100
#define CHANNELS 2
#define LATENCY 60000

static char *PCM_DEVICE = "default";                        /* playback PCM_DEVICE */
//读取tsf timetamp的句柄
int TSF_FD = -1;
snd_pcm_t *PCM_HANDLE;

const snd_pcm_uframes_t PCM_FRAME_NUM = MAX_FRAMES/4;

unsigned char HOST_IP[128];

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

int CURRENT_STATUS = CMD_STOP; //2 stop 1 start


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
                #ifdef DEBUG
            printf("wirte to buffer index %d , timestamp %lld\n",index, get_tsf(&TSF_FD));
            printf("frames len %lu\n",strlen(FRAME_LIST[index].frames));
            #endif
            len = strlen(FRAME_LIST[index].frames);
            //len = len>MAX_FRAMES?MAX_FRAMES:len;
            //FRAME_LIST[index].frames[len]='\0';
            ret = snd_pcm_writei(PCM_HANDLE, FRAME_LIST[index].frames, PCM_FRAME_NUM);
		    if (ret == -EPIPE) {
		    	 printf("<<<<<<<<<<<<<< Buffer Underrun >>>>>>>>>>>>>>>>> len = %d\n",len);
                 snd_pcm_prepare(PCM_HANDLE);
				 index++;
		    }
		/*	snd_pcm_recover(PCM_HANDLE,ret,1);
			if(ret<0){
                  snd_pcm_prepare(PCM_HANDLE);
                  //fprintf(stderr, "<<<<<<<<<<<<<<< Buffer Underrun >>>>>>>>>>>>>>>\n");
            }               
			*/
			memset(FRAME_LIST[index].frames,0,MAX_FRAMES);
            if(index>=MAX_INDEX-1){
                index = 0;
            }else{
                index++;
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
                memset(&(FRAME_LIST[FRAME_INDEX].frames),0,MAX_FRAMES);
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
    #ifdef DEBUG
    printf("1  recv time = ");
    print_timestamp();
    #endif

	long msec = RECV_CMD->current_t-get_tsf(&TSF_FD);
	printf("sleep ms = %ld\n",msec);
    #ifdef DEBUG
    print_timestamp();
    #endif
    const struct timespec spec ={
        .tv_sec = 0,
        .tv_nsec = msec*1000
    };
    #ifdef DEBUG
    printf("3  recv time = ");
    print_timestamp();
    #endif
    nanosleep(&spec,NULL);
    print_timestamp();
    start_write_pcm();
    #ifdef DEBUG
    printf("4  recv time = ");
    print_timestamp();
    #endif

}
//1. 接收命令
void *listent_control_socket(void *msg){
    printf("listent_control_socket \n");
    int sockfd;
    struct sockaddr_in servaddr;
    socklen_t socklen = sizeof(struct sockaddr_in);

    if ((sockfd=socket(AF_INET,SOCK_DGRAM,0)) < 0)   
    {  
        perror("socket");  
        exit(1);  
    }  
    //close hsa_syslogd release 18879 port
    ///* allow multiple sockets to use the same PORT number */ 

    memset(&servaddr,0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr= inet_addr(LISTEN_IP); 
    servaddr.sin_port=htons(PORT_C);  

    if(bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) == -1)  {
        perror("bind error");
        close(sockfd);
        sockfd = -1;
    }else {
        printf("bind address to socket. %d\n\r",servaddr.sin_port);
    }
    /* use setsockopt() to request that the kernel join a multicast group */  
    // int set = 1;
    // if (setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&set,sizeof(int)) < 0)   
    // {  
    //     perror("setsockopt");  
    //     exit(1);  
    // }  
    int length;
    int recvLen = sizeof(so_play_cmd)+1;
    char rervBuffer[recvLen];
    
    pthread_t start_write_p;
    RECV_CMD = malloc(sizeof(so_play_cmd));
    while(1){
        memset(rervBuffer,0, recvLen);
        length = recvfrom(sockfd, rervBuffer, recvLen, 0,(struct sockaddr *) &servaddr, &socklen);
        printf("length = %d\n",length);
        if (length <= 3)
        {
            printf("Server Recieve Data error!\n");
        }else {
            printf("recv buffer = %s\n",rervBuffer);
            so_play_cmd *tmp_cmd = (so_play_cmd *)rervBuffer;
            RECV_CMD->type = tmp_cmd->type;
            RECV_CMD->current_t = tmp_cmd->current_t;

            if((RECV_CMD->type==CMD_START) && CURRENT_STATUS== CMD_STOP){
                CURRENT_STATUS = CMD_START;
                printf("start request\n");
                //todo 根据 cmd->current_t 的时间戳来启动
                start_request_data();
                printf("recv seconds = %ld \n", RECV_CMD->current_t);
                pthread_create(&start_write_p,NULL,time_start,NULL);
                pthread_detach(start_write_p);
            }else{
                CURRENT_STATUS = CMD_STOP;;
                 printf("stop request\n");
                stop_write_pcm();
                stop_request_data();
            }
        }
     }
     close(sockfd);
}


//发送广播后接受 server_ip
int send_find_broadcast(){
    int brdcfd;

    if((brdcfd = socket(PF_INET,SOCK_DGRAM,0))==-1){
         printf("socket failed \n");
        return -1;
    }

    int optval = 1;
    setsockopt(brdcfd,SOL_SOCKET,SO_BROADCAST,(const char *)&optval, sizeof(int));

    struct sockaddr_in b_ip;

    memset(&b_ip,0,sizeof(struct sockaddr_in));

    b_ip.sin_family = AF_INET;
    //b_ip.sin_addr.s_addr = htonl(INADDR_BROADCAST);

    b_ip.sin_port = htons(PORT_B);
    int sendBytes;
    char buffer[128];

    struct timeval tv;
    fd_set readfds;
    int ret;

    tv.tv_sec = 10;
    tv.tv_usec = 0;

    int inet_nums = get_nio_nums();
    if(inet_nums<1){
        printf("no network interfaces");
        return -1;
    }
    so_network_io *NETWORK_IO_S[inet_nums];

    get_network_io(NETWORK_IO_S);
    printf("inet-nums = %d\n",inet_nums);
    int ii= 0;
    for(ii=0;ii<inet_nums;ii++){
        printf("send to broadcast %s\n",(char*)inet_ntoa(NETWORK_IO_S[ii]->broadcast));
        if(NETWORK_IO_S[ii]->is_up==1){
            b_ip.sin_addr.s_addr = NETWORK_IO_S[ii]->broadcast.s_addr;

            if((sendBytes = sendto(brdcfd,CMD_FIND,strlen(CMD_FIND),0,(struct sockaddr *)&b_ip, sizeof(b_ip))) == -1){
                printf("sendto fail errno = %d\n",errno);
                close(brdcfd);
                exit(-1);      
            }
        }else{
            printf("interfaces is down \n");
        }
        
    }

    
    while(1){
        FD_ZERO(&readfds);
        FD_SET(brdcfd,&readfds);

        ret = select (brdcfd+1,
                &readfds,
                NULL,
                NULL,
                &tv
        );
        if(ret==-1){
            perror("select error");
            close(brdcfd);
            return -1;
        } else if(!ret){
            printf("find 10s elapsed. \n");
            tv.tv_sec = 10;
            tv.tv_usec = 0;

             for(ii=0;ii<inet_nums;ii++){
                printf("send to broadcast %s\n",(char*)inet_ntoa(NETWORK_IO_S[ii]->broadcast));
                if(NETWORK_IO_S[ii]->is_up==1){
                    b_ip.sin_addr.s_addr = NETWORK_IO_S[ii]->broadcast.s_addr;

                    if((sendBytes = sendto(brdcfd,CMD_FIND,strlen(CMD_FIND),0,(struct sockaddr *)&b_ip, sizeof(b_ip))) == -1){
                        printf("sendto fail errno = %d\n",errno);
                        close(brdcfd);
                        exit(-1);      
                    }
                }else{
                    printf("interfaces is down \n");
                }
                
            }
           
            continue;
        }

        if(FD_ISSET(brdcfd, &readfds)){
            memset(buffer,0,sizeof(buffer));
            int len = recv(brdcfd, buffer, sizeof(buffer), 0);
            if(len<0){
                continue;
            }
            memset(HOST_IP,0,sizeof(HOST_IP));
            strncpy(HOST_IP,buffer,sizeof(buffer));
            printf("HOST_IP %s\n",HOST_IP);
            break;
        }
    }
    close(brdcfd);
    return 0;
}

void *heartbeat(void *msg){
    struct timespec sleep = {
    .tv_sec = 30,
    .tv_nsec = 0  //nsec

    }; //1us ;

    while(1){
            nanosleep(&sleep,NULL);
            //获取到主机后，直接发送主机
            send_find_broadcast();
    }
    return NULL;
}




int main(int argc,char *argv[]){

    //void init_playback(snd_pcm_t *handle,char *device,int type,int format,int access,int channels,int rate,int latency);
    //指定host ip

    if(send_find_broadcast()!=0){
        printf("find device failed\n");
        return -1;
    }
    TSF_FD = open("/dev/tsf",O_RDONLY);

    init_playback_p();
    //Thread2. 发送hostip 去获取数据,放入到FRAME_LIST中
    //Thread1. 监听广播开始获取
    pthread_t listen_p,request_data_p,read_p,sleep_p;

    pthread_create(&sleep_p,NULL,heartbeat,NULL);
    pthread_detach(sleep_p);

   
    pthread_create(&request_data_p,NULL,request_data,NULL);
    pthread_detach(request_data_p);
    pthread_create(&read_p,NULL,write_to_buffer,NULL);
    pthread_detach(read_p);
    pthread_create(&listen_p,NULL,listent_control_socket,NULL);
    pthread_join(listen_p,NULL);


   
    close_playback(PCM_HANDLE);
    return 0;
}
