
#include "socast.h"

/**
  目前存在的问题：
   1. 容易出现 pcm buffer申请不到内存的现象
   2. 基于广播的命令有可能丢失（后期改为TCP链接）
   



   增加查找功能：
    1. 启动时获取IP地址（自动获取，或者手动指定）  
    2. 监听广播，回复链接IP帝制  监听广播端口PORT_B，
*/

#define RATE 44100
#define CHANNELS 2
#define LATENCY 60000

so_play_frame FRAME_LIST[MAX_INDEX];

int IS_RUNNING = 0;

int FRAME_INDEX = 0;
//读取tsf timestamp的文件句柄
int TSF_FD = -1;


static char *PCM_DEVICE = "default";                        /* playback device */

snd_pcm_uframes_t PCM_FRAME_NUM = MAX_FRAMES/4;

struct timespec SLEEP_TIME = {
    .tv_sec = 0,
    .tv_nsec = 1000000 //1ms

}; 

int SOCK_DATA_QUEST;

socklen_t socklen = sizeof(struct sockaddr_in);

int S_MAX = 0;


char SERVER_IP[128];
int SOCK_CONTROL;

snd_pcm_t *PCM_HANDLE;

so_speaker CLIENT_SPEAKER[10];
int SPEAKER_NUMS = sizeof(CLIENT_SPEAKER)/sizeof(so_speaker);

void *read_record(void *msg){
  printf("start record \n");
  FRAME_INDEX=0;
  open_and_set_pcm(&PCM_HANDLE,PCM_DEVICE,SND_PCM_STREAM_CAPTURE,SND_PCM_FORMAT_S16_LE,SND_PCM_ACCESS_RW_INTERLEAVED,RATE,CHANNELS,LATENCY,PCM_FRAME_NUM,FRAME_LIST[0].frames); 
  
  int ret;
  while(1){
      //当index == 0 等时候走一次时间戳的获取及校准
      FRAME_LIST[FRAME_INDEX].timestamp=get_tsf(&TSF_FD);
      FRAME_LIST[FRAME_INDEX].sequence = FRAME_INDEX;
      memset(FRAME_LIST[FRAME_INDEX].frames,0,MAX_FRAMES);
      ret = snd_pcm_readi(PCM_HANDLE, FRAME_LIST[FRAME_INDEX].frames,PCM_FRAME_NUM); 
     if(IS_RUNNING){
       if( FRAME_INDEX >= MAX_INDEX-1){
        S_MAX++;
        FRAME_INDEX=0;
       } else{
        FRAME_INDEX++;
       }
     }
     if (ret == -EPIPE) {  
          /* EPIPE means overrun */  
          fprintf(stderr, "overrun occurred\n");  
          snd_pcm_prepare(PCM_HANDLE);  
          continue;
     } else if (ret < 0) {  
          fprintf(stderr,  
            "ret = %d ,error from read: %s\n", ret,
            snd_strerror(ret)); 
            //snd_pcm_prepare(PCM_HANDLE);  
          continue;
     } else if (ret != (int)PCM_FRAME_NUM) {  
          fprintf(stderr, "short read, read %d frames\n", ret);  
		  continue;
     }  
  }
  close_pcm(PCM_HANDLE);
  close_playback(PCM_HANDLE);
  return NULL;
}
void send_client(so_play_cmd *play_cmd){
     int i_len=0;
     struct sockaddr_in send_addr;
     int sock_send_to;
     if ((sock_send_to=socket(AF_INET,SOCK_DGRAM,0)) < 0)   
     {  
          perror("socket");  
		  return;
     }  
     for(i_len=0;i_len<SPEAKER_NUMS;i_len++){
          if(CLIENT_SPEAKER[i_len].addr_len){
            send_addr.sin_addr.s_addr=CLIENT_SPEAKER[i_len].address.sin_addr.s_addr; 
            sendto(sock_send_to,play_cmd,sizeof(so_play_cmd),0,(struct sockaddr *)&CLIENT_SPEAKER[i_len].address,CLIENT_SPEAKER[i_len].addr_len);
          }
     }
}

void start_record(){
  FRAME_INDEX = 0;
  IS_RUNNING = 1;
}
void stop_record(){
  IS_RUNNING = 0;
}


void *read_buffer(void *filename){
  // todo  1. 使用广播方式发送命令
  //2. 读取内容，解析放入LIST中
  int fd;
  int ret;
  fd = open((char *)filename,O_RDONLY);
  printf("stdout to pcm : filename=%s\n",(char *)filename);
  if ( fd < 0 ) {
      printf("open file %s error\n",(char *)filename);
  }else{
      so_play_frame *frame;
      for(FRAME_INDEX=0;IS_RUNNING;) {
          //当index == 0 等时候走一次时间戳的获取及校准

         //根据list排序写入列表
          //申请一块内存一个frame struct


           bzero(&(FRAME_LIST[FRAME_INDEX].frames),MAX_FRAMES);
           FRAME_LIST[FRAME_INDEX].timestamp=get_tsf(&TSF_FD);

          FRAME_LIST[FRAME_INDEX].sequence = FRAME_INDEX;
          // printf("frames p=%p\n",frame->frames);
          //data num_frames
          if((ret = read(fd,FRAME_LIST[FRAME_INDEX].frames,MAX_FRAMES))<=0){
              printf("muisc play end\n");
              break;
          }   
          lseek(fd,MAX_FRAMES*FRAME_INDEX,SEEK_SET);
          
          //index最大值为MAX_INDEX 
          if( FRAME_INDEX >= MAX_INDEX-1){
            FRAME_INDEX=0;
          }else{
            FRAME_INDEX++;
          }
          // printf("Test %d ret:%d, frames len%d\n",index,ret,FRAME_LIST->len);
      }
  }
  lseek(fd,0,SEEK_SET);
  close(fd);
  printf("Play music finish !\n");
  // so_play_frame *data = (so_play_frame *)(list_at(FRAME_LIST,1)->val);
  // printf("timestamp %lld , frams = %s\n",data->timestamp,data->frames);

  // data = (so_play_frame *)(list_at(FRAME_LIST,FRAME_LIST->len-1)->val);
  // printf("timestamp %lld , frams  = %s\n",data->timestamp,data->frames);



  // list_iterator_t *it = list_iterator_new(FRAME_LIST, LIST_HEAD);
  // int tindex=0;
  // for(tindex=0;tindex<FRAME_LIST->len;tindex++){
  //    list_node_t *a = list_iterator_next(it);
  //   so_play_frame *data = (so_play_frame *)a->val;
  //   printf("timestamp = %lld frames len = %lu \n frame = %p\n",data->timestamp,sizeof(data->frames),data->frames);
  // }
   //todo  3. 通知停止
  pthread_exit(NULL);
  return NULL;
}

void *process_request(void *msg){
   //todo fork 处理
    // printf("data = %s len = %lu\n",frames->frames,sizeof(frames->frames));
    int data_len = sizeof(so_play_frame);
    so_sp *sp = (so_sp *)msg;
    int recIndex = atoi(sp->msg);
    #ifdef DEBUG
    printf("new thread ");
    print_timestamp();
    #endif
    while(IS_RUNNING){
       if(recIndex < FRAME_INDEX || (recIndex-FRAME_INDEX)>(MAX_INDEX-10)){
          #ifdef DEBUG
          printf("request index %d <= FRAME_INDEX %d\n",recIndex,FRAME_INDEX);
          #endif
          while((sendto(SOCK_DATA_QUEST, &FRAME_LIST[recIndex], data_len, 0, (struct sockaddr *)&sp->from_server, socklen)<0) && IS_RUNNING){
             perror("sendto+");  
             print_timestamp();
          }
          #ifdef DEBUG
          printf("after send to request INDEX %d , %lld\n",FRAME_INDEX, get_tsf(&TSF_FD));
          #endif

          break;
       }
    }
    free(msg);
    #ifdef DEBUG

    printf("1end thread %d  ",pthread_self());
    
    print_timestamp();
    #endif
    pthread_exit(NULL);
    return NULL;
}

void *data_server(void *msg){
  printf("start data server\n");
  struct sockaddr_in echoserver;
  

  /* create what looks like an ordinary UDP socket */  
  if ((SOCK_DATA_QUEST=socket(AF_INET,SOCK_DGRAM,0)) < 0)   
  {  
      perror("socket");  
      exit(1);  
  }  
   /* Construct the server sockaddr_in structure */
  memset(&echoserver, 0, sizeof(echoserver));       /* Clear struct */
  echoserver.sin_family = AF_INET;                  /* Internet/IP */
  echoserver.sin_addr.s_addr = inet_addr(LISTEN_IP);  /* IP address */
  echoserver.sin_port = htons(PORT);       /* server port */
 
  if((bind(SOCK_DATA_QUEST, (struct sockaddr*)&echoserver, sizeof(echoserver))) == -1){
    perror("bind");
    exit(1);
  }else {
    printf("bind address to socket. %d\n\r",echoserver.sin_port);

  }

  //open audio file
  char cRecvBuf[64]; 
  int length;
  int data_len = sizeof(so_play_frame);
  so_sp *sp;
  int msg_len = sizeof(sp->msg);
  pthread_t request_p;
  for (; ;) {
        //data num_frames
        // printf("start time: %lld ms\n", start);
        // printf("i=%d\n",i);
      if(IS_RUNNING){
        
        sp = malloc(sizeof(so_sp));//线程中释放process_request
        sp->from_len = socklen;
        #ifdef DEBUG
        printf("before send to request INDEX %d , %lld\n",FRAME_INDEX, get_tsf(TSF_DF));
        #endif
        bzero(sp->msg,msg_len);
        length = recvfrom(SOCK_DATA_QUEST, sp->msg, msg_len, 0,(struct sockaddr *) &sp->from_server, &sp->from_len);
        if(length<=0){
          printf("length = %d\n",length);
          continue;
        }
        #ifdef DEBUG
        printf("sp->msg = %s\n",sp->msg);
        #endif
        pthread_create(&request_p,NULL,process_request,sp);
        pthread_detach(request_p);
      }else{
        nanosleep(&SLEEP_TIME,NULL);
      }
     
      // printf("end time: %lld ms\n", end);
      // printf("rece buffer = %s timestamp = %d\n",cRecvBuf,0);
  }
  
  /* Send the word to the server */
  /* Receive the word back from the server */
  fprintf(stdout, "Received: ");
  fprintf(stdout, "\n");
  close(SOCK_DATA_QUEST);
  return NULL;
}

//监听广播地址，返回ip地址给客户端
void *listen_broadcast_find(void *msg){
    int SOCK_CONTROL;
    struct sockaddr_in servaddr;
    socklen_t socklen = sizeof(struct sockaddr_in);

    if ((SOCK_CONTROL=socket(AF_INET,SOCK_DGRAM,0)) < 0)   
    {  
        perror("socket");  
        exit(1);  
    }  
    //close hsa_syslogd release 18879 port
    ///* allow multiple sockets to use the same PORT number */ 

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr=INADDR_ANY; 
    servaddr.sin_port=htons(PORT_B);  

    if(bind(SOCK_CONTROL, (struct sockaddr *)&servaddr, sizeof(servaddr)) == -1)  {
        perror("bind error");
        close(SOCK_CONTROL);
        exit(1);
    }
    /* use setsockopt() to request that the kernel join a multicast group */  
    int set = 1;
    if (setsockopt(SOCK_CONTROL,SOL_SOCKET,SO_REUSEADDR,&set,sizeof(int)) < 0)   
    {  
        perror("setsockopt");  
        exit(1);  
    }  
    int length;
    int recvLen = strlen(CMD_FIND);
    char rervBuffer[recvLen];

    char client_ip[128];
    char server_ip[128];


    while(1){
        bzero(rervBuffer, recvLen);
        length = recvfrom(SOCK_CONTROL, rervBuffer, recvLen, 0,(struct sockaddr *) &servaddr, &socklen);
        if (length <= 2)
        {
            printf("Server Recieve Data error!\n");
        }else {
            printf("recv buffer = %s rblen = %ld , find len = %ld \n",rervBuffer,strlen(rervBuffer),strlen(CMD_FIND));

            if(strncmp(rervBuffer,CMD_FIND,recvLen-1)==0){
              //  todo //把servaddr ，socklen 写在数组中
              int i=0;
              int isExist = 0;
              int empty = -1;
              for( i = 0 ; i < SPEAKER_NUMS ; i++ ){
                 if(CLIENT_SPEAKER[i].addr_len){
                    bzero(client_ip,128);
                    bzero(server_ip,128);
                    sprintf(client_ip,"%s",inet_ntoa(CLIENT_SPEAKER[i].address.sin_addr));
                    sprintf(server_ip,"%s",inet_ntoa(servaddr.sin_addr));
                    printf("client_ip = %s, server_ip=%s\n",client_ip,server_ip);
                    if(strncmp(client_ip,server_ip,strlen(client_ip)) == 0){
                       #ifdef DEBUG
                       printf("Server address = %s already is exist\n",inet_ntoa(servaddr.sin_addr));
                       #endif
                       isExist = 1;
                       break;
                    }
                 }else{
                  if(empty==-1){
                    empty = i;
                  }
                 }
              }
              printf("empty = %d\n",empty);
              if(isExist==0){
                if(empty!=-1){
                  CLIENT_SPEAKER[empty].address = servaddr;
                  CLIENT_SPEAKER[empty].address.sin_port = htons(PORT_C);
                  CLIENT_SPEAKER[empty].addr_len = socklen;

                  sendto(SOCK_CONTROL,SERVER_IP,strlen(SERVER_IP),0,(struct sockaddr *) &servaddr, socklen);
                }else{
                  printf("CLIENT_SPEAKER is fill!\n");
                }
                
              }else{
                  sendto(SOCK_CONTROL,SERVER_IP,strlen(SERVER_IP),0,(struct sockaddr *) &servaddr, socklen);
              }
            }else{
              printf("recv cmd error!\n");
            }
        }
     }
   close(SOCK_CONTROL);
  return NULL;
}

void init_playback_r(){
    printf("init_playback\n");
    int err;

    if ((err = snd_pcm_open(&PCM_HANDLE, PCM_DEVICE, SND_PCM_STREAM_CAPTURE, 0)) < 0) {
            printf("Playback open error: %s\n", snd_strerror(err));
            // exit(EXIT_FAILURE);
    }
    if ((err = snd_pcm_set_params(PCM_HANDLE,
                                  SND_PCM_FORMAT_S16_LE,
                                  SND_PCM_ACCESS_RW_INTERLEAVED,
                                  CHANNELS,
                                  RATE,
                                  1,
                                  LATENCY)) < 0) {   
            printf("Playback open error: %s\n", snd_strerror(err));
            // exit(EXIT_FAILURE);
    }
    //open audio file
    int ret;
}

int main(int argc, char *argv[]) {
  
  TSF_FD = open("/dev/tsf",O_RDONLY);

   //printf("2\n");
  pthread_t server_p,play_p,find_listen_broadcast_p;
  char *play="-p";
  char *record="-r";
  int type=0;  // 0 表示错误， 1 直接播放  2 表示录制播放
  //输入命令使用
  char buf[1024];
  int len = -1;


  so_play_cmd *play_cmd; 
  play_cmd = malloc(sizeof(so_play_cmd));
  struct ifreq *ifr_localhost=(struct ifreq *)malloc(sizeof(struct ifreq));
  //Thread 2
  if(argc>2){
    if(get_ip(argv[1],ifr_localhost)!=0){
      printf("input interface name is error\n");
      return -1;
    }
    bzero(SERVER_IP,128);
    printf("localhost = %s\n", inet_ntoa(((struct sockaddr_in*)&(ifr_localhost->ifr_addr))->sin_addr));
    strcpy(SERVER_IP,inet_ntoa(((struct sockaddr_in*)&(ifr_localhost->ifr_addr))->sin_addr));
    //如果是播放音乐，可以先读取音乐，然后发送命令播放
    if(strncmp(argv[2],play,2)==0){
        if(argc>2){
         
           type = 1;
        }else{
           printf("./socast interface -p filename\n");
           return -1;
        }
    }
    //如果是录制，则需要等命令后再开始录制
    if(strncmp(argv[2],record,2)==0){
        //录加播放
      type = 2;
      //init_playback_r();
      printf("type 2\n");
      pthread_create(&play_p,NULL,read_record,NULL);  
      pthread_detach(play_p);
    }
  }else{
    printf("./socast interface -p|r [filename]\n");
    return 0;
  }
  

  //启动监听查找的广播
  pthread_create(&find_listen_broadcast_p,NULL,listen_broadcast_find,NULL);
  pthread_detach(find_listen_broadcast_p);

  //启动数据请求的server
 pthread_create(&server_p,NULL,data_server,NULL);
 pthread_detach(server_p);

 //初始化发送命令的socket

   
  while(1){
      bzero(buf,1024);
      printf("Input key : [start:%d,stop:%d]\n",CMD_START,CMD_STOP);
      len = read(STDIN_FILENO, buf,1024);
      if(len == -1){
        perror("read error");
        return 1;
      }

      if(len>0){
          buf[len] = '\0';
          printf("Input key = %s\n",buf);
          if(atoi(buf)==CMD_START){
            if(IS_RUNNING==1){
              printf("Already is running ,please input 'stop'\n");
              continue;
            }
            IS_RUNNING = 1;
              //发送组播到其他speaker 开始播放
            printf(" start %d : ",CMD_START);
            play_cmd->type=CMD_START;
			play_cmd->current_t = get_tsf(&TSF_FD)+100000;

            //循环发送已经添加额客户端；
			send_client(play_cmd);

            FRAME_INDEX=0;
            if(type==1){
                printf(" play %s\n",argv[3]);
                pthread_create(&play_p,NULL,read_buffer,(void *)argv[3]);  
                pthread_detach(play_p);
            }else{
                printf(" record play\n");
                start_record();
            }
          }else  if(atoi(buf)==CMD_STOP){
              //发送组播到其他speaker 停止播放
            stop_record();
            play_cmd->type=CMD_STOP;
	        play_cmd->current_t = get_tsf(&TSF_FD);
			send_client(play_cmd);
            printf("STOP %d\n",CMD_STOP);
          }else{
            printf("error\n");
          }
        // printf("read:%s\n",buf);
      }
  }
  free(ifr_localhost);
  return 0;
}

