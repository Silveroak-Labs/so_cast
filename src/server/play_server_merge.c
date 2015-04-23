
#ifndef _SOCAST_INCLUDE_
#define _SOCAST_INCLUDE_

#include <stdio.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <sys/fcntl.h>

#include <sys/timeb.h>

#include <sys/types.h>
#include <unistd.h>

#include <pthread.h>
#include <alsa/asoundlib.h> 

#include <net/if.h>
#include <net/if_arp.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#include <unistd.h>

#include <arpa/inet.h>
#include <errno.h>

#define LISTEN_IP "0.0.0.0"
#define PORT_B 18884

#define PORT 8883

#define PORT_C 8884


#define MAX_INDEX 1024*8

#define MAX_FRAMES 2048 //传输 的数据大小，2048 比较稳定

#define OK_STR "OK"
//定义命令字符串

#define CMD_START 1
#define CMD_STOP 2
#define CMD_ADD 0 //增加设备到server中

#define CMD_FIND "find_server"

#define MAXINTERFACES 16    /* 最大接口数 */


typedef struct so_play_frame_
{
  int sequence;
  struct timeval timestamp;
  unsigned char frames[MAX_FRAMES];
  /* data */
} so_play_frame;

typedef struct so_play_cmd_
{
  int type;  //1.start 2.stop
  struct timeval current_t;

} so_play_cmd;


typedef struct so_play_server_process_
{
  char msg[64];
  struct sockaddr_in from_server;
  int from_len;
} so_sp;

typedef struct so_play_speaker_
{
  struct sockaddr_in address;
  int addr_len;
} so_speaker;

typedef struct so_network_io_
{
  char name[64];
  struct in_addr ip;
  struct in_addr netmask;
  struct in_addr broadcast;
  int is_up;  //1 up 0 down
} so_network_io;


long long getSystemTime() {
    struct timeb t;
    ftime(&t);
    return 1000 * t.time + t.millitm;
}

void print_timestamp(){
  struct timeval tv;
  int ret;
  ret = gettimeofday(&tv,NULL);
  #ifdef DEBUG
    if(!ret){
      printf("seconds = %ld ,useconds = %ld\n",(long) tv.tv_sec, (long)tv.tv_usec);
    }
  #endif
}

void close_playback(snd_pcm_t *PCM_HANDLE){
    snd_pcm_prepare(PCM_HANDLE); 
    snd_pcm_drain(PCM_HANDLE); 
    snd_pcm_close(PCM_HANDLE);
    #ifdef DEBUG
       printf("close_playback\n");
    #endif
}


void Die(char *mess) { perror(mess); exit(1); }


int get_nio_nums(){
   int fd;
  int if_len;     /* 接口数量 */
  struct ifreq buf[MAXINTERFACES];    /* ifreq结构数组 */
  struct ifconf ifc;   
  /* 建立IPv4的UDP套接字fd */
    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
    {
        perror("socket(AF_INET, SOCK_DGRAM, 0)");
        return -1;
    }
 
    /* 初始化ifconf结构 */
    ifc.ifc_len = sizeof(buf);
    ifc.ifc_buf = (caddr_t) buf;
 
    /* 获得接口列表 */
    if (ioctl(fd, SIOCGIFCONF, (char *) &ifc) == -1)
    {
        perror("SIOCGIFCONF ioctl");
        close(fd);
        return -1;
    }
 
    if_len = ifc.ifc_len / sizeof(struct ifreq); /* 接口数量 */
    close(fd);
    return if_len;
}

int get_network_io(so_network_io *buffer[MAXINTERFACES]){
    int fd;
    int if_len;     /* 接口数量 */
    struct ifreq buf[MAXINTERFACES];    /* ifreq结构数组 */
    struct ifconf ifc;   
  /* 建立IPv4的UDP套接字fd */
    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
    {
        perror("socket(AF_INET, SOCK_DGRAM, 0)");
        return -1;
    }
 
    /* 初始化ifconf结构 */
    ifc.ifc_len = sizeof(buf);
    ifc.ifc_buf = (caddr_t) buf;
 
    /* 获得接口列表 */
    if (ioctl(fd, SIOCGIFCONF, (char *) &ifc) == -1)
    {
        perror("SIOCGIFCONF ioctl");
        close(fd);
        return -1;
    }
 
    if_len = ifc.ifc_len / sizeof(struct ifreq); /* 接口数量 */
    int index = 0;
    for(index=0;index<if_len;index++)
    {
        buffer[index] = (so_network_io *)malloc(sizeof(so_network_io));
        strncpy(buffer[index]->name,buf[index].ifr_name,strlen(buf[index].ifr_name));

        printf("interface name：%s\n", buf[index].ifr_name); /* 接口名称 */
 
        /* 获得接口标志 */
        if (!(ioctl(fd, SIOCGIFFLAGS, (char *) &buf[index])))
        {
            /* 接口状态 */
            if (buf[index].ifr_flags & IFF_UP)
            {
              buffer[index]->is_up = 1;
              printf("status: UP\n");
            }
            else
            {
              buffer[index]->is_up = 0;
              printf("status: DOWN\n");
            }
        }
        else
        {
            char str[256];
            sprintf(str, "SIOCGIFFLAGS ioctl %s", buf[index].ifr_name);
            perror(str);
        }
 
 
        /* IP地址 */
        if (!(ioctl(fd, SIOCGIFADDR, (char *) &buf[index])))
        {
            buffer[index]->ip = ((struct sockaddr_in*) (&buf[index].ifr_addr))->sin_addr;

            printf("IP :%s\n",
                    (char*)inet_ntoa(((struct sockaddr_in*) (&buf[index].ifr_addr))->sin_addr));
        }
        else
        {
            char str[256];
            sprintf(str, "SIOCGIFADDR ioctl %s", buf[index].ifr_name);
            perror(str);
        }
 
        /* 子网掩码 */
        if (!(ioctl(fd, SIOCGIFNETMASK, (char *) &buf[index])))
        {
            buffer[index]->netmask = ((struct sockaddr_in*) (&buf[index].ifr_addr))->sin_addr;

            printf("NETMASK :%s\n",
                    (char*)inet_ntoa(((struct sockaddr_in*) (&buf[index].ifr_addr))->sin_addr));
        }
        else
        {
            char str[256];
            sprintf(str, "SIOCGIFADDR ioctl %s", buf[index].ifr_name);
            perror(str);
        }
 
        /* 广播地址 */
        if (!(ioctl(fd, SIOCGIFBRDADDR, (char *) &buf[index])))
        {
            buffer[index]->broadcast = ((struct sockaddr_in*) (&buf[index].ifr_addr))->sin_addr;
            printf("broadcast:%s\n",
                    (char*)inet_ntoa(((struct sockaddr_in*) (&buf[index].ifr_addr))->sin_addr));
        }
        else
        {
            char str[256];
            sprintf(str, "SIOCGIFADDR ioctl %s", buf[index].ifr_name);
            perror(str);
        }
 
    }//–while end
 
    //关闭socket
    close(fd);
    return 0;
}

int get_ip(char *name,struct ifreq *ifr){
     int inet_sock;
      inet_sock = socket(AF_INET, SOCK_DGRAM, 0);
      printf("interface name = %s strlen = %d\n",name,strlen(name));

      strcpy(ifr->ifr_name, name);
      //SIOCGIFADDR标志代表获取接口地址
      if (ioctl(inet_sock, SIOCGIFADDR, ifr) <  0){
          perror("ioctl");
          return -1;
      }
      printf("%s\n", inet_ntoa(((struct sockaddr_in*)&(ifr->ifr_addr))->sin_addr));
      return 0;
}


#endif  // _SOCAST_INCLUDE_


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

char SERVER_IP[128];
int SOCK_CONTROL;

so_speaker CLIENT_SPEAKER[10];
int SPEAKER_NUMS = sizeof(CLIENT_SPEAKER)/sizeof(so_speaker);
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
                                  LATENCY)) < 0) {   /* 0.5sec */
            printf("Playback open error: %s\n", snd_strerror(err));
            // exit(EXIT_FAILURE);
    }
    //open audio file
    int ret;

}

int main(int argc, char *argv[]) {
  
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
      init_playback_r();
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

 int sock_send_to;
 struct sockaddr_in send_addr;
 socklen_t send_addr_len = sizeof(struct sockaddr_in);

  if ((sock_send_to=socket(AF_INET,SOCK_DGRAM,0)) < 0)   
  {  
      perror("socket");  
      exit(1);  
  }  
   
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
            gettimeofday(&(play_cmd->current_t),NULL);
            play_cmd->current_t.tv_usec = (play_cmd->current_t.tv_usec+40000);

            //循环发送已经添加额客户端；
            int i_len=0;
            for(i_len=0;i_len<SPEAKER_NUMS;i_len++){
                int slen ;
                if(CLIENT_SPEAKER[i_len].addr_len){
                  send_addr.sin_addr.s_addr=CLIENT_SPEAKER[i_len].address.sin_addr.s_addr; 
                  printf("ip %s %d\n",inet_ntoa(CLIENT_SPEAKER[i_len].address.sin_addr),CLIENT_SPEAKER[i_len].address.sin_port);
                  slen = sendto(sock_send_to,play_cmd,sizeof(so_play_cmd),0,(struct sockaddr *)&CLIENT_SPEAKER[i_len].address,CLIENT_SPEAKER[i_len].addr_len);
                  printf("sendto len = %d\n",slen);
                }
            }


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
            gettimeofday(&(play_cmd->current_t),NULL);

            int i_len=0;
            for(i_len=0;i_len<SPEAKER_NUMS;i_len++){
                if(CLIENT_SPEAKER[i_len].addr_len){
                  sendto(sock_send_to,play_cmd,sizeof(so_play_cmd),0,(struct sockaddr *)&CLIENT_SPEAKER[i_len].address,CLIENT_SPEAKER[i_len].addr_len);
                }
            }
            printf("STOP %d\n",CMD_STOP);

          }else{
            printf("error\n");
          }
        // printf("read:%s\n",buf);
      }
  }
  free(ifr_localhost);
  close_playback(PCM_HANDLE);
  return 0;
}

