/* 
  This example reads from the default PCM device 
  and writes to standard output for 5 seconds of data. 
 */  
  
/* Use the newer ALSA ALI */ 


/* 
 * This example reads standard from input and writes to  
 * the default PCM device for 5 seconds of data. 
 */
/* Use the newer ALSA API */   
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/fcntl.h> 
#include <alsa/asoundlib.h>  

#define MAX_SIZE 16*1024
 static char *device = "default";                        /* playback device */
snd_output_t *output = NULL;
unsigned char *buffer;   

int main()  
{  
    int err;
        unsigned int i;
        snd_pcm_t *handle;
        
        snd_pcm_sframes_t frames;

        if ((err = snd_pcm_open(&handle, device, SND_PCM_STREAM_CAPTURE, 0)) < 0) {
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
            }  
            ret = write(1, buffer, MAX_SIZE);  
        }
        snd_pcm_drain(handle); 
        snd_pcm_close(handle);
        free(buffer);
        return 0;
}

