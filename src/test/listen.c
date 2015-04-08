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

#define MAX_SIZE 16*1024
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
	unsigned short port;
	
	int rcv_size = 0;
	socklen_t optlen;

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	
        port = 8883;
	servaddr.sin_addr.s_addr = inet_addr("0.0.0.0");

	sockfd = socket(AF_INET, SOCK_STREAM, 0); 
        {
//close hsa_syslogd release 18879 port
            int opt = 1;
            setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));
        }
// set sock recv buf
	optlen = sizeof(rcv_size);
	int ret = getsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &rcv_size, &optlen);
	if(ret < 0)
		printf("getsockopt error\n");
	printf("original recv buffer size: %d\n", rcv_size);

// end
	servaddr.sin_port = htons(port);
	printf("socket_port = %d\n",port);

	if(bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) == -1)	{
		perror("bind error");
		close(sockfd);
		sockfd = -1;
	}
        if ( listen(sockfd, 20) )
        {
            printf("Server Listen Failed!");
            exit(1);
        }

        int n = 0;
	for (; g_running == 1;) {
            struct sockaddr_in cliaddr;
            socklen_t len = sizeof(cliaddr);
            int new_server_socket = accept(sockfd,(struct sockaddr*)&cliaddr,&len);
            if ( new_server_socket < 0)
            {
                printf("Server Accept Failed!\n");
                break;
            }
            while(1){
                bzero(buffer, MAX_SIZE);
                int length = recv(new_server_socket,buffer,MAX_SIZE,0);
                if (length <= 0)
                {
                    printf("Server Recieve Data Failed!\n");
                    break;
                }else {
                    write_to_buffer(buffer);
                }
             }
            close(new_server_socket);

        }
        close(sockfd);
}

int main(int argc,char *argv[]){
    init_playback();
    listent_socket();
    close_playback();
    return 0;
}
