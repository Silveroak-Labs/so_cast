/* 
 * This example reads standard from input and writes to  
 * the default PCM device for 5 seconds of data. 
 */
/* Use the newer ALSA API */   
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/fcntl.h> 
#include <alsa/asoundlib.h>  

#define MAX_SIZE 2*1024
 static char *device = "default";                        /* playback device */
snd_output_t *output = NULL;
unsigned char *buffer;   

int main()  
{  
    int err;
        unsigned int i;
        snd_pcm_t *handle;
        
        snd_pcm_sframes_t frames;

        if ((err = snd_pcm_open(&handle, device, SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
                printf("Playback open error: %s\n", snd_strerror(err));
                exit(EXIT_FAILURE);
        }
        if ((err = snd_pcm_set_params(handle,
                                      SND_PCM_FORMAT_S16_LE,
                                      SND_PCM_ACCESS_RW_INTERLEAVED,
                                      2,
                                      44100,
                                      1,
                                      500000)) < 0) {   /* 0.5sec */
                printf("Playback open error: %s\n", snd_strerror(err));
                exit(EXIT_FAILURE);
        }
        //open audio file
        int ret;
            buffer = (unsigned char *)malloc(MAX_SIZE);
            frames = MAX_SIZE/4;
            for (i = 0; 1; i++) {
                //data num_frames
                ret = read(0, buffer, MAX_SIZE);
                if(ret<=0){
                    printf("muisc play end\n");
                    break;
                }   
                printf("i=%d\n",i);
                while ((ret = snd_pcm_writei(handle, buffer, frames)) < 0) {
                      snd_pcm_prepare(handle);
                      fprintf(stderr, "<<<<<<<<<<<<<<< Buffer Underrun >>>>>>>>>>>>>>>\n");
                }
              printf("Test %d ret:%d, frames %d\n",i,ret,frames);
                
            }
        snd_pcm_drain(handle); 
        snd_pcm_close(handle);
        free(buffer);
        return 0;
}


