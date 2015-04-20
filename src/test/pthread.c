#include <stdio.h>
#include <pthread.h>
void *test(void *msg){
	printf("test pthread \n");
	return NULL;
}

int main(){
	pthread_t testp;
	pthread_create(&testp,NULL,test,NULL);
	pthread_join(testp,NULL);

	return 0;
}