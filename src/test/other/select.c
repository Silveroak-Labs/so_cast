#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>


#define TIMEOUT 5
#define BUF_LEN 1024

int main(void){

	struct timeval tv;
	fd_set readfds;
	int ret;

	

	tv.tv_sec = TIMEOUT;
	tv.tv_usec = 0;
	while(1){
		FD_ZERO(&readfds);
		FD_SET(STDIN_FILENO,&readfds);
		ret = select (STDIN_FILENO+1,
				&readfds,
				NULL,
				NULL,
				&tv
		);

		if(ret==-1){
			perror("select error");
			return 1;
		} else if(!ret){
			printf("%d seconds elapsed. \n", TIMEOUT);
			return 0;
		}

		if(FD_ISSET(STDIN_FILENO, &readfds)){
			char buf[BUF_LEN];
			int len;
			len = read(STDIN_FILENO, buf,BUF_LEN);
			if(len == -1){
				perror("read error");
				return 1;
			}
			if(len){
				buf[len] = '\0';
				printf("read:%s\n",buf);
			}
			
		}
	}

	
	printf("test 1 \n");

	return 1;
}