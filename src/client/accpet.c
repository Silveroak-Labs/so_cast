/*
   1. 接收server的命令处理，（根据命令启动定时处理任务）

   2. 得到流保存到buffer

   3. 从buffer中开发写入pcm buffer中
*/

#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/fcntl.h> 
#include <alsa/asoundlib.h> 
#include <pthread.h>

#include "../lib/list.h"

#define GROUP_IP "225.0.0.37"
#define PORT 8883

#define MAX_SIZE 1024
#define RATE 44100
#define CHANNELS 2
#define LATENCY 500000

static char *device = "default";                        /* playback device */

snd_output_t *output = NULL;

unsigned char *buffer; 

snd_pcm_t *handle;

snd_pcm_uframes_t frames;  

void close_playback(){
    snd_pcm_drain(handle); 
    snd_pcm_close(handle);
    return;
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
    buffer = (unsigned char *)malloc(MAX_SIZE);
    frames = MAX_SIZE/4;
    return;
}

void write_to_buffer(unsigned char *buffer){
   int ret;
    while ((ret = snd_pcm_writei(handle, buffer, frames)) < 0) {
          snd_pcm_prepare(handle);
          fprintf(stderr, "<<<<<<<<<<<<<<< Buffer Underrun >>>>>>>>>>>>>>>\n");
    }
    printf("Test ret:%d, frames %d\n",ret,frames);
}

void *listent_socket()
{
    int g_running = 1;
	int sockfd;
	struct sockaddr_in servaddr;
	
	int rcv_size = 0;
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

	if(bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) == -1)	{
		perror("bind error");
		close(sockfd);
		sockfd = -1;
	}
    /* use setsockopt() to request that the kernel join a multicast group */  
    mreq.imr_multiaddr.s_addr=inet_addr(GROUP_IP);  
    mreq.imr_interface.s_addr=htonl(INADDR_ANY);  
    if (setsockopt(sockfd,IPPROTO_IP,IP_ADD_MEMBERSHIP,&mreq,sizeof(mreq)) < 0)   
    {  
        perror("setsockopt");  
        exit(1);  
    }  
    int length;
    while(1){
        bzero(buffer, MAX_SIZE);
        length = recvfrom(sockfd, buffer, MAX_SIZE, 0,(struct sockaddr *) &servaddr, &socklen);
        if (length <= 0)
        {
            printf("Server Recieve Data complete!\n");
            break;
        }else {
            write_to_buffer(buffer);
            printf("length = %d\n",length);
        }
     }
}

int main(int argc,char *argv[]){
    init_playback();
    listent_socket();
    close_playback();
    return 0;
}
