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
 
  //open audio file
  char cRecvBuf[100]; 
  int i;
  long long start;
  long long end;
  list_node_t *frame;
  for (i = 0; ; i++) {
      //data num_frames
      start = getSystemTime();
      printf("start time: %lld ms\n", start);
      printf("i=%d\n",i);

      recv(sock_c, cRecvBuf, strlen(cRecvBuf)+1, 0);
      frame = list_at(FRAME_LIST, atoi(cRecvBuf));
      if(sendto(sock_c,(char *)frame->val, sizeof(frame->val), 0, (struct sockaddr *) &echoserver, sizeof(echoserver))<0){
         perror("sendto");  
         break;
      }
      end = getSystemTime();
      printf("end time: %lld ms\n", end);
      printf("rece buffer = %s timestamp = %d\n",cRecvBuf,0);
  }
  
  /* Send the word to the server */
  /* Receive the word back from the server */
  fprintf(stdout, "Received: ");
  fprintf(stdout, "\n");
  close(sock_c);
  return NULL;
}

void *read_buffer(void *filename){
  // 1. 使用广播方式发送命令

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
          frame->frames = (unsigned char *)malloc(MAX_FRAMES);
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
          printf("Test %d ret:%d, frames %s\n",index,ret,frame->frames);
      }
  }
  lseek(fd,0,SEEK_SET);
  close(fd);
  printf("Play music finish !");
  
   // 3. 通知停止
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
   //pthread_join(server_p,NULL);
   //printf("2\n");

   //Thread 2
    
   //2. 读取内容，解析放入LIST中
   //pthread_join(read_p,NULL);

   // read_buffer(argv[1]);

   return 0;
}

