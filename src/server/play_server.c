#include <arpa/inet.h>
#include <errno.h>
#include "socast.h"

/**
  目前存在的问题：
   1. 重新开始后，延迟过大
   2. 容易出现 pcm buffer申请不到内存的现象
   3. 定时任务还没有完成
   4. 基于广播的命令有可能丢失（后期改为TCP链接）
   5. 增加多线程处理同时请求的问题

*/


#define RATE 44100
#define CHANNELS 2
#define LATENCY 50000

so_play_frame FRAME_LIST[MAX_INDEX];

int IS_RUNNING = 0;

int FRAME_INDEX = 0;


snd_pcm_t *PCM_HANDLE;

static char *PCM_DEVICE = "default";                        /* playback device */

snd_pcm_uframes_t PCM_FRAME_NUM = MAX_FRAMES/4;

struct timespec SLEEP_TIME = {
    .tv_sec = 0,
    .tv_nsec = 1000000 //1ms

}; 

int SOCK_DATA_QUEST;

socklen_t socklen = sizeof(struct sockaddr_in);

int S_MAX = 0;


void *read_record(void *msg){
  printf("start record \n");
  
  int ret;
  FRAME_INDEX=0;
  while(1){
    if(IS_RUNNING) {
      //当index == 0 等时候走一次时间戳的获取及校准
      gettimeofday(&(FRAME_LIST[FRAME_INDEX].timestamp),NULL);
      FRAME_LIST[FRAME_INDEX].sequence = FRAME_INDEX;

      bzero(FRAME_LIST[FRAME_INDEX].frames,MAX_FRAMES);
      ret = snd_pcm_readi(PCM_HANDLE, FRAME_LIST[FRAME_INDEX].frames,PCM_FRAME_NUM); 
      if (ret == -EPIPE) {  
          /* EPIPE means overrun */  
          fprintf(stderr, "overrun occurred\n");  
          snd_pcm_prepare(PCM_HANDLE);  
          continue;
      } else if (ret < 0) {  
          fprintf(stderr,  
            "error from read: %s\n",  
            snd_strerror(ret)); 
            snd_pcm_prepare(PCM_HANDLE);  
          continue;
      } else if (ret != (int)PCM_FRAME_NUM) {  
          fprintf(stderr, "short read, read %d frames\n", ret);  
      }  
      if( FRAME_INDEX >= MAX_INDEX-1){
        S_MAX++;
        FRAME_INDEX=0;
      } else{
        FRAME_INDEX++;
      }
    }else{
            nanosleep(&SLEEP_TIME,NULL);
    }
  }
  return NULL;
}
void start_record(){
  FRAME_INDEX = 0;
  IS_RUNNING = 1;
}
void stop_record(){
  IS_RUNNING = 0;
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
          printf("after send to request INDEX %d , %lu\n",FRAME_INDEX, getSystemTime());
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
  printf("msg = %s\n",msg);
   /* Construct the server sockaddr_in structure */
  memset(&echoserver, 0, sizeof(echoserver));       /* Clear struct */
  echoserver.sin_family = AF_INET;                  /* Internet/IP */
  echoserver.sin_addr.s_addr = inet_addr(LISTEN_IP);  /* IP address */
  echoserver.sin_port = htons(PORT);       /* server port */
 
  if((bind(SOCK_DATA_QUEST, (struct sockaddr*)&echoserver, sizeof(echoserver))) == -1){
    perror("bind");
    exit(1);
  }else
    printf("bind address to socket.\n\r");


  //open audio file
  char cRecvBuf[64]; 
  int length;
  int data_len = sizeof(so_play_frame);
  so_sp *sp;
  int msg_len = sizeof(sp->msg);
  pthread_t request_p;
  for (; ;) {
        //data num_frames
        // start = getSystemTime();
        // printf("start time: %lld ms\n", start);
        // printf("i=%d\n",i);
      if(IS_RUNNING){
        
        sp = malloc(sizeof(so_sp));//线程中释放process_request
        sp->from_len = socklen;
        #ifdef DEBUG
        printf("before send to request INDEX %d , %lu\n",FRAME_INDEX, getSystemTime());
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
     
      // end = getSystemTime();
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

void *read_buffer(void *filename){
  // todo  1. 使用广播方式发送命令
  //2. 读取内容，解析放入LIST中
  int fd;
  int ret;
  fd = open((char *)filename,O_RDONLY);
  printf("stdout to pcm : filename=%s\n",filename);
  if ( fd < 0 ) {
      printf("open file %s error\n",filename);
  }else{
      so_play_frame *frame;
      for(FRAME_INDEX=0;IS_RUNNING;) {
          //当index == 0 等时候走一次时间戳的获取及校准

         //根据list排序写入列表
          //申请一块内存一个frame struct


           bzero(&(FRAME_LIST[FRAME_INDEX].frames),MAX_FRAMES);
           gettimeofday(&(FRAME_LIST[FRAME_INDEX].timestamp),NULL);

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
  printf("FRAME_LIST LENGTH = %d\n",sizeof(FRAME_LIST)/sizeof(so_play_frame));
   //todo  3. 通知停止
  pthread_exit(NULL);
  return NULL;
}


int send_broadcast(char *buffer,int len){
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
    b_ip.sin_addr.s_addr = inet_addr(BC_IP);
    b_ip.sin_port = htons(PORT_C);
    int sendBytes;
    if((sendBytes = sendto(brdcfd,buffer,len,0,(struct sockaddr *)&b_ip, sizeof(b_ip))) == -1){
        printf("sendto fail errno = %d\n",errno);
        close(brdcfd);
        return -1;
    }
    close(brdcfd);
    return 0;
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
                                  LATENCY)) < 0) {   /* 0.5sec */
            printf("Playback open error: %s\n", snd_strerror(err));
            // exit(EXIT_FAILURE);
    }
    //open audio file
    int ret;
    printf("========== PCM_FRAME_NUM %d\n",PCM_FRAME_NUM);

}

int main(int argc, char *argv[]) {
  
   //printf("2\n");
  pthread_t server_p,play_p;
  char *play="-p";
  char *record="-r";
  int type=0;  // 0 表示错误， 1 直接播放  2 表示录制播放
  //输入命令使用
  char buf[1024];
  int len = -1;

  so_play_cmd *play_cmd; 
  play_cmd = malloc(sizeof(so_play_cmd));

  //Thread 2
  init_playback_r();
  if(argc>1){
    //如果是播放音乐，可以先读取音乐，然后发送命令播放
    if(strncmp(argv[1],play,2)==0){
        if(argc>2){
         
           type = 1;
        }else{
           printf("./socast -p filename\n");
           return -1;
        }
    }
    //如果是录制，则需要等命令后再开始录制
    if(strncmp(argv[1],record,2)==0){
        //录加播放
      type = 2;
      printf("type 2\n");
      pthread_create(&play_p,NULL,read_record,NULL);  
      pthread_detach(play_p);
    }
  }else{
    printf("./socast -p|r [filename]\n");
    return 0;
  }


   pthread_create(&server_p,NULL,data_server,NULL);
   pthread_detach(server_p);
   //Thread 1. 启动一个监听请求的UDP server，处理到LIST中获取frames
       //接收数据
       //发送数据
   // pthread_join(server_p,NULL);

  
  
  
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
          printf("buffer = %s\n",buf);
          if(atoi(buf)==CMD_START){
            if(IS_RUNNING==1){
              printf("Already is running ,please input 'stop'\n");
              continue;
            }
            IS_RUNNING = 1;
              //发送组播到其他speaker 开始播放
            printf(" start %d : ",CMD_START);
            printf(" start %lu\n",getSystemTime());
            play_cmd->type=CMD_START;
            gettimeofday(&(play_cmd->current_t),NULL);
            play_cmd->current_t.tv_usec = (play_cmd->current_t.tv_usec+40000);

            send_broadcast((char *)play_cmd,sizeof(so_play_cmd));
            printf(" send_broadcast %lu\n",getSystemTime());
            send_broadcast((char *)play_cmd,sizeof(so_play_cmd));
            printf(" send_broadcast %lu\n",getSystemTime());
            
            send_broadcast((char *)play_cmd,sizeof(so_play_cmd));
            printf(" send_broadcast %lu\n",getSystemTime());



            FRAME_INDEX=0;
            if(type==1){
                printf(" play %s\n",argv[2]);
                pthread_create(&play_p,NULL,read_buffer,(void *)argv[2]);  
                pthread_detach(play_p);
            }else{
                printf(" record play\n");
                start_record();
                printf(" pthread_create record_play %lu\n",getSystemTime());
            }
          }else  if(atoi(buf)==CMD_STOP){
              //发送组播到其他speaker 停止播放
            stop_record();
            play_cmd->type=CMD_STOP;
            gettimeofday(&(play_cmd->current_t),NULL);
            send_broadcast((char *)play_cmd,sizeof(so_play_cmd));
            printf("STOP %d\n",CMD_STOP);
            send_broadcast((char *)play_cmd,sizeof(so_play_cmd));
            printf("STOP %d\n",CMD_STOP);
            send_broadcast((char *)play_cmd,sizeof(so_play_cmd));
            printf("STOP %d\n",CMD_STOP);

          }else{
            printf("error\n");
          }
        // printf("read:%s\n",buf);
      }
  }
  close_playback(PCM_HANDLE);
  return 0;
}

