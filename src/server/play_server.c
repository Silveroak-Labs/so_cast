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
#include <errno.h>
#include "socast.h"
list_t *FRAME_LIST;

int IS_RUNNING = 0;


#define RATE 44100
#define CHANNELS 2
#define LATENCY 50000

static char *PCM_DEVICE = "default";                        /* playback device */

snd_pcm_t *PCM_HANDLE;

snd_pcm_uframes_t PCM_FRAMES;  



long long getSystemTime() {
    struct timeb t;
    ftime(&t);
    return 1000 * t.time + t.millitm;
}

void Die(char *mess) { perror(mess); exit(1); }

void close_playback(){
    
    snd_pcm_prepare(PCM_HANDLE); 
    snd_pcm_drain(PCM_HANDLE); 
    snd_pcm_close(PCM_HANDLE);
    printf("close_playback\n");
}

/**
   初始化 pcm 
*/
void init_playback(){
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
    PCM_FRAMES = MAX_FRAMES/4;
}


int read_record(){
  init_playback();
  int ret;
  int index;  //序列号
  so_play_frame *frame;
  list_node_t *node;

  list_destroy(FRAME_LIST);
  FRAME_LIST = list_new();
  
  for(index=0;IS_RUNNING;) {
    //当index == 0 等时候走一次时间戳的获取及校准

   //根据list排序写入列表
    //申请一块内存一个frame struct
    
    if (!(frame = LIST_MALLOC(sizeof(so_play_frame))))
       break;
    if(index==0){
      frame->timestamp = getSystemTime()+40; //40ms的延迟处理
    }else{
      frame->timestamp = getSystemTime(); //
    }
    frame->sequence = index;
    bzero(frame->frames,MAX_FRAMES);
    printf("after index = %d\n",index);
    ret = snd_pcm_readi(PCM_HANDLE, frame->frames,PCM_FRAMES); 
    printf("frames size = %lu\n",strlen(frame->frames));
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
    } else if (ret != (int)PCM_FRAMES) {  
        fprintf(stderr, "short read, read %d frames\n", ret);  
    }  
    printf("index = %d\n",index);
    node = list_node_new(frame);
    list_rpush(FRAME_LIST, node);
    index++;
    if( index >= MAX_INDEX){
      index=0;
    }
  }
  close_playback();
  return 0;
}

void *data_server(void *msg){
  struct sockaddr_in echoserver;
  int sock_c;
  socklen_t socklen = sizeof(struct sockaddr_in);

  /* create what looks like an ordinary UDP socket */  
  if ((sock_c=socket(AF_INET,SOCK_DGRAM,0)) < 0)   
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
 
  if((bind(sock_c, (struct sockaddr*)&echoserver, sizeof(echoserver))) == -1){
    perror("bind");
    exit(1);
  }else
    printf("bind address to socket.\n\r");


  //open audio file
  char cRecvBuf[128]; 
  int i;
  long long start;
  long long end;
  list_node_t *node_t;
  int length;
  int data_len = sizeof(so_play_frame);
  printf("so_play_frame len = %lu\n",sizeof(so_play_frame));
  char data[sizeof(so_play_frame)] = {0};
  if(FRAME_LIST->len<1){
     printf("Frame list is null\n");
  }
  for (i = 0; ; i++) {
      //data num_frames
      // start = getSystemTime();
      // printf("start time: %lld ms\n", start);
      // printf("i=%d\n",i);
      bzero(cRecvBuf,128);
      length = recvfrom(sock_c, cRecvBuf, 128, 0,(struct sockaddr *) &echoserver, &socklen);
      if(length<=0){
        continue;
      }


      node_t = list_at(FRAME_LIST, atoi(cRecvBuf));
      if(node_t){
        // printf("index = %d\n",atoi(cRecvBuf));
        bzero(data, data_len);
        so_play_frame *frames = (so_play_frame *)node_t->val;
        strncpy(data,node_t->val,data_len);
        // printf("data = %s len = %lu\n",frames->frames,sizeof(frames->frames));
        if(sendto(sock_c, node_t->val, data_len, 0, (struct sockaddr *) &echoserver, sizeof(echoserver))<0){
           perror("sendto");  
           break;
        }
      }
      // end = getSystemTime();
      // printf("end time: %lld ms\n", end);
      // printf("rece buffer = %s timestamp = %d\n",cRecvBuf,0);
  }
  
  /* Send the word to the server */
  /* Receive the word back from the server */
  fprintf(stdout, "Received: ");
  fprintf(stdout, "\n");
  close(sock_c);
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
      int index;
      so_play_frame *frame;
      list_node_t *node;

      list_destroy(FRAME_LIST);
      FRAME_LIST = list_new();

      for(index=0;IS_RUNNING;index++) {
          //当index == 0 等时候走一次时间戳的获取及校准

         //根据list排序写入列表
          //申请一块内存一个frame struct

          if(index>=FRAME_LIST->len){
              if (!(frame = LIST_MALLOC(sizeof(so_play_frame)))){
                  break;
              }
           } else{
               node = list_at(FRAME_LIST,index);
               if(node){
                   frame = (so_play_frame *)node->val;
               }
               bzero(frame->frames,MAX_FRAMES);
           }
          if(index==0){
            frame->timestamp = getSystemTime()+40; //40ms的延迟处理
          }else{
            frame->timestamp = getSystemTime(); //
          }
          frame->sequence = index;
          // printf("frames p=%p\n",frame->frames);
          //data num_frames
          if((ret = read(fd,frame->frames,MAX_FRAMES))<=0){
              printf("muisc play end\n");
              break;
          }   
          lseek(fd,MAX_FRAMES*index,SEEK_SET);
          if(index>=FRAME_LIST->len){
               //新建一个node
              node = list_node_new(frame);
              list_rpush(FRAME_LIST, node);
           }
          //index最大值为MAX_INDEX 
          if( index >= MAX_INDEX){
            index=0;
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
  printf("FRAME_LIST LENGTH = %d\n",FRAME_LIST->len);
   //todo  3. 通知停止
  return NULL;
}


void *record_play(void *msg){
  read_record();
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

int main(int argc, char *argv[]) {
   FRAME_LIST = list_new();
  
   //printf("2\n");
  pthread_t server_p,play_p;
  char *play="-p";
  char *record="-r";
  int type=0;  // 0 表示错误， 1 直接播放  2 表示录制播放
  //输入命令使用
  char buf[1024];
  int len = -1;
  //Thread 2
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
    }
  }else{
    printf("./socast -p|r [filename]\n");
    return 0;
  }

   pthread_create(&server_p,NULL,data_server,NULL);
   //Thread 1. 启动一个监听请求的UDP server，处理到LIST中获取frames
       //接收数据
       //发送数据
   // pthread_join(server_p,NULL);


  
  while(1){
      bzero(buf,1024);
      printf("Input key : [%s,%s]\n",CMD_START,CMD_STOP);
      len = read(STDIN_FILENO, buf,1024);
      if(len == -1){
        perror("read error");
        return 1;
      }

      if(len>3){
        buf[len] = '\0';
        printf("buffer = %s\n",buf);
        if(strncmp(buf,CMD_START,len-1)==0){
          if(IS_RUNNING==1){
            printf("Already is running ,please input 'stop'\n");
            continue;
          }
          IS_RUNNING = 1;
            //发送组播到其他speaker 开始播放
          send_broadcast(CMD_START,strlen(CMD_START));
          printf("%s : ",CMD_START);
            if(type==1){
                printf(" play %s\n",argv[2]);
                pthread_create(&play_p,NULL,read_buffer,(void *)argv[2]);  
            }else{
                printf(" record play\n");
                pthread_create(&play_p,NULL,record_play,NULL);  
            }
            
        }else if(strncmp(buf,CMD_STOP,len-1)==0){
          IS_RUNNING = 0;
            //发送组播到其他speaker 停止播放
          send_broadcast(CMD_STOP,strlen(CMD_STOP));

            printf("%s\n",CMD_STOP);
        }else{
          printf("error\n");
        }
        // printf("read:%s\n",buf);
      }
  }

  return 0;
}

