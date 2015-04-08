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
#include <pthread.h>
#define MAX_SIZE 1024

int sock_l;
int sock_r;
unsigned char *buffer; 

void *send_l(void *msg){
    send(sock_l, buffer, MAX_SIZE, 0);
}
void *send_r(void *msg){
    send(sock_r, buffer, MAX_SIZE, 0);
}
void Die(char *mess) { perror(mess); exit(1); }

int main(int argc, char *argv[]) {
  
  struct sockaddr_in echoserver_l;
  struct sockaddr_in echoserver_r;
  int received = 0;

  if (argc != 3) {
    fprintf(stderr, "USAGE: TCPecho <server_ip_left> <server_ip_right> \n");
    exit(1);
  }
  /* Create the TCP socket */
  if ((sock_l = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
    Die("Failed to create socket");
  }

  if ((sock_r = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
    Die("Failed to create socket");
  }

  /* Construct the server sockaddr_in structure */
  memset(&echoserver_l, 0, sizeof(echoserver_l));       /* Clear struct */
  memset(&echoserver_r, 0, sizeof(echoserver_r));       /* Clear struct */
  echoserver_l.sin_family = AF_INET;                  /* Internet/IP */
  echoserver_l.sin_addr.s_addr = inet_addr(argv[1]);  /* IP address */
  echoserver_l.sin_port = htons(8883);       /* server port */
  echoserver_r.sin_family = AF_INET;                  /* Internet/IP */
  echoserver_r.sin_addr.s_addr = inet_addr(argv[2]);  /* IP address */
  echoserver_r.sin_port = htons(8883);       /* server port */
  /* Establish connection */
  if (connect(sock_l,
              (struct sockaddr *) &echoserver_l,
              sizeof(echoserver_l)) < 0) {
    Die("Failed to connect with server");
  }
  if (connect(sock_r,
              (struct sockaddr *) &echoserver_r,
              sizeof(echoserver_r)) < 0) {
    Die("Failed to connect with server");
  }
 
  //open audio file
  int ret;
  buffer = (unsigned char *)malloc(MAX_SIZE);

  pthread_t pt_l,pt_r;


  int i;
  for (i = 0; 1; i++) {
      //data num_frames
      ret = read(0, buffer, MAX_SIZE);
      if(ret<=0){
          printf("muisc play end\n");
          break;
      }   
      printf("i=%d\n",i);
      pthread_create(&pt_l,NULL,send_l,NULL);
      pthread_create(&pt_r,NULL,send_r,NULL);
      pthread_join(pt_l,NULL); 
      pthread_join(pt_r,NULL); 
      
  }
  
  /* Send the word to the server */
  /* Receive the word back from the server */
  fprintf(stdout, "Received: ");
  fprintf(stdout, "\n");
  close(sock_l);
   close(sock_r);
   free(buffer);
   exit(0);
}

