#include "socast.h"


long long getSystemTime() {
    struct timeb t;
    ftime(&t);
    return 1000 * t.time + t.millitm;
}

void print_timestamp(){
  struct timeval tv;
  int ret;
  ret = gettimeofday(&tv,NULL);
  #ifdef DEBUG
    if(!ret){
      printf("seconds = %ld ,useconds = %ld\n",(long) tv.tv_sec, (long)tv.tv_usec);
    }
  #endif
}

void close_playback(snd_pcm_t *PCM_HANDLE){
    snd_pcm_prepare(PCM_HANDLE); 
    snd_pcm_drain(PCM_HANDLE); 
    snd_pcm_close(PCM_HANDLE);
    #ifdef DEBUG
       printf("close_playback\n");
    #endif
}


void Die(char *mess) { perror(mess); exit(1); }

