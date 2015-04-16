#include <stdio.h>
#include <sys/time.h>
#include <sys/timeb.h>

unsigned long long getSystemTime() {
    struct timeb t;
    ftime(&t);
    return 1000 * t.time + t.millitm;
}
void print_time(){
	struct timeval tv;
	int ret;
	ret = gettimeofday(&tv,NULL);
	if(!ret){
		printf("seconds = %ld ,useconds = %ld\n",(long) tv.tv_sec, (long)tv.tv_usec);
	}
}

int main(int arcv, char *argv[]){
	print_time();
	struct timespec spec ={
		.tv_sec = 0,
		.tv_nsec = 100000000
	};
	nanosleep(&spec,NULL);
	print_time();

	return 0;
}