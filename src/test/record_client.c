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

#define MAX_SIZE 512 
#define RATE 32000
#define CHANNELS 2
#define LATENCY 10000
static char *device = "default";  

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
          RATE,
          1,
          LATENCY)) < 0) {  /* 0.5sec */
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

int main(int argc, char *argv[]) {
  int sock;
  struct sockaddr_in echoserver;
  int received = 0;

  if (argc != 3) {
    fprintf(stderr, "USAGE: TCPecho <server_ip> <port>\n");
    exit(1);
  }
  /* Create the TCP socket */
  if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
    Die("Failed to create socket");
  }


  /* Construct the server sockaddr_in structure */
  memset(&echoserver, 0, sizeof(echoserver));       /* Clear struct */
  echoserver.sin_family = AF_INET;                  /* Internet/IP */
  echoserver.sin_addr.s_addr = inet_addr(argv[1]);  /* IP address */
  echoserver.sin_port = htons(atoi(argv[2]));       /* server port */
  /* Establish connection */
  if (connect(sock,
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
    send(sock, buffer, MAX_SIZE, 0);
      
  }

  /* Receive the word back from the server */
  fprintf(stdout, "Received: ");
  fprintf(stdout, "\n");
   close(sock);
   exit(0);
}
