#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <alsa/asoundlib.h> 

#define CHANNELS 2
static char *device = "default";  
int sock_c;

int MAX_SIZE = 512;
int rate = 32000
int latency = 10000;

unsigned char *buffer;  
snd_pcm_t *handle;
snd_pcm_uframes_t frames;
snd_pcm_hw_params_t *params;

void Die(char *mess) { perror(mess); exit(1); }


void close_record(){
  snd_pcm_drain(handle); 
  snd_pcm_close(handle);
  free(buffer);
}
void init_record(){
  int err;
  unsigned int i;

  if ((err = snd_pcm_open(&handle, device, SND_PCM_STREAM_CAPTURE, 0)) < 0) {
          printf("Playback open error: %s\n", snd_strerror(err));
          exit(EXIT_FAILURE);
  }

  snd_pcm_hw_params_alloca(&params);

  int dir;
  int rc;
  frames = MAX_SIZE/4;
  rc = snd_pcm_hw_params(handle, params); 

  if ((err = snd_pcm_set_params(handle,
          SND_PCM_FORMAT_S16_LE,
          SND_PCM_ACCESS_RW_INTERLEAVED,
          CHANNELS,
          rate,
          1,
          latency)) < 0) {  /* 0.5sec */
    printf("Playback open error: %s\n", snd_strerror(err));
    exit(EXIT_FAILURE);
  }
  buffer = (unsigned char *)malloc(MAX_SIZE);

  return;
}
int read_record(){
  int ret;
  ret = snd_pcm_readi(handle, buffer, frames); 
  if (ret == -EPIPE) {  
      /* EPIPE means overrun */  
      fprintf(stderr, "overrun occurred\n");  
      snd_pcm_prepare(handle);  
  } else if (ret < 0) {  
      fprintf(stderr,  
        "error from read: %s\n",  
        snd_strerror(ret));  
  } else if (ret != (int)frames) {  
      fprintf(stderr, "short read, read %d frames\n", ret);  
      return 0;
  }  
  return ret;
}
void *send(){
    send(sock_l, buffer, MAX_SIZE, 0);
}

int main(int argc, char *argv[]) {
  struct sockaddr_in echoserver;
  int received = 0;

  if (argc != 6) {
    fprintf(stderr, "USAGE: TCPecho <b_ip> <port> <rate> <latency> <max_size\n");
    exit(1);
  }
  /* Create the TCP socket */
  if ((sock_c = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
    Die("Failed to create socket");
  }

  /* Construct the server sockaddr_in structure */
  memset(&echoserver, 0, sizeof(echoserver));       /* Clear struct */

  echoserver.sin_family = AF_INET;                  /* Internet/IP */
  echoserver.sin_addr.s_addr = inet_addr(argv[1]);  /* IP address */
  echoserver.sin_port = htons(atoi(argv[2]));       /* server port */
  /* Establish connection */
  if (connect(sock_c,
              (struct sockaddr *) &echoserver,
              sizeof(echoserver)) < 0) {
    Die("Failed to connect with server");
  }

  /* Send the word to the server */
  int i;
  init_record();
  while(1){
    i=read_record();
    if(i<0){
        continue;
    }
    send();
  }

  /* Receive the word back from the server */
  fprintf(stdout, "Received: ");
  fprintf(stdout, "\n");
   close(sock_c);
   exit(0);
}
