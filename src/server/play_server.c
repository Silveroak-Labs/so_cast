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
#include "socast.h"
list_t *FRAME_LIST;

long long getSystemTime() {
    struct timeb t;
    ftime(&t);
    return 1000 * t.time + t.millitm;
}

void Die(char *mess) { perror(mess); exit(1); }

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
  char data[MAX_FRAMES+8] = {0};
  for (i = 0; ; i++) {
      //data num_frames
      // start = getSystemTime();
      // printf("start time: %lld ms\n", start);
      // printf("i=%d\n",i);
      length = recvfrom(sock_c, cRecvBuf, 128, 0,(struct sockaddr *) &echoserver, &socklen);
      if(length<=0){
        continue;
      }

      node_t = list_at(FRAME_LIST, atoi(cRecvBuf));

      printf("index = %d\n",atoi(cRecvBuf));
      bzero(data, MAX_FRAMES+8);
      so_play_frame *frames = (so_play_frame *)node_t->val;
      strncpy(data,node_t->val,MAX_FRAMES+8);
      // printf("data = %s len = %lu\n",frames->frames,sizeof(frames->frames));
      if(sendto(sock_c, node_t->val, MAX_FRAMES+8, 0, (struct sockaddr *) &echoserver, sizeof(echoserver))<0){
         perror("sendto");  
         break;
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

      for(index=0;;index++) {
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
          // printf("frames p=%p\n",frame->frames);
          //data num_frames
          if((ret = read(fd,frame->frames,MAX_FRAMES))<=0){
              printf("muisc play end\n");
              break;
          }   
          lseek(fd,MAX_FRAMES*index,SEEK_SET);
          //新建一个node
          node = list_node_new(frame);
          list_rpush(FRAME_LIST, node);
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
  //   printf("timestamp = %lld frames len = %lu \n frame = %s\n",data->timestamp,sizeof(data->frames),data->frames);

  // }


   //todo  3. 通知停止
  return NULL;
}

int main(int argc, char *argv[]) {
   printf("--------------0\n");
   FRAME_LIST = list_new();


   printf("--------------1\n");
   pthread_t server_p,read_p;


   pthread_create(&server_p,NULL,data_server,NULL);
   pthread_create(&read_p,NULL,read_buffer,(void *)argv[1]);   
  //Thread 1. 启动一个监听请求的UDP server，处理到LIST中获取frames
       //接收数据
       //发送数据
   pthread_join(server_p,NULL);
   //printf("2\n");

   //Thread 2
    
   //2. 读取内容，解析放入LIST中
   pthread_join(read_p,NULL);

   // read_buffer(argv[1]);

   return 0;
}

