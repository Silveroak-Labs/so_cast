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


#define BUFFSIZE 32
#define MAX_SIZE 16*1024

void Die(char *mess) { perror(mess); exit(1); }
int main(int argc, char *argv[]) {
  int sock;
  struct sockaddr_in echoserver;
  unsigned char *buffer;
  unsigned int echolen;
  int received = 0;

  if (argc != 4) {
    fprintf(stderr, "USAGE: TCPecho <server_ip> <port> <file>\n");
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


  int fd;
  fd = open(argv[3],O_RDONLY);
  buffer = (unsigned char *)malloc(MAX_SIZE);

  lseek(fd,0,SEEK_SET);
  /* Send the word to the server */
  int i;
  for(i=0;i<1000;i++){
    read(fd,buffer,MAX_SIZE);
    lseek(fd,MAX_SIZE*i,SEEK_SET);
    send(sock, buffer, MAX_SIZE, 0);
    printf("i = %d\n",i);
  }

  lseek(fd,0,SEEK_SET);
  close(fd);
  /* Receive the word back from the server */
  fprintf(stdout, "Received: ");
  fprintf(stdout, "\n");
   close(sock);
   exit(0);
}
