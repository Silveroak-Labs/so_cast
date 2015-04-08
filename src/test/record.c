#include <sys/types.h>
#include <sys/stat.h>
#include <sys/fcntl.h> 
#include <alsa/asoundlib.h>  

#define MAX_SIZE 512 
static char *device = "default";                        /* playback device */
snd_output_t *output = NULL;

int main()  
{  
    unsigned char *buffer;   
    int err;
        unsigned int i;
        snd_pcm_t *handle;
        snd_pcm_t *handle_play;
        
        snd_pcm_uframes_t frames;
        snd_pcm_hw_params_t *params; 

        if ((err = snd_pcm_open(&handle, device, SND_PCM_STREAM_CAPTURE, 0)) < 0) {
                printf("Playback open error: %s\n", snd_strerror(err));
                exit(EXIT_FAILURE);
        }
        if ((err = snd_pcm_open(&handle_play, device, SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
                printf("Playback open error: %s\n", snd_strerror(err));
                exit(EXIT_FAILURE);
        }

        snd_pcm_hw_params_alloca(&params);

        unsigned int val;
        unsigned int latency;
        int dir;
        int rc;
        val = 32000;
        latency = 10000;

        frames = MAX_SIZE/4;

        //snd_pcm_hw_params_set_period_size_near(handle,params, &frames, &dir);
        //snd_pcm_hw_params_set_period_size_near(handle_play,params, &frames, &dir);

        rc = snd_pcm_hw_params(handle, params); 
        rc = snd_pcm_hw_params(handle_play, params); 

        if ((err = snd_pcm_set_params(handle,
	                              SND_PCM_FORMAT_S16_LE,
	                              SND_PCM_ACCESS_RW_INTERLEAVED,
	                              2,
	                              val,
	                              1,
	                              latency)) < 0) {	/* 0.5sec */
		printf("Playback open error: %s\n", snd_strerror(err));
		exit(EXIT_FAILURE);
	}

        if ((err = snd_pcm_set_params(handle_play,
	                              SND_PCM_FORMAT_S16_LE,
	                              SND_PCM_ACCESS_RW_INTERLEAVED,
	                              2,
	                              val,
	                              1,
	                              latency)) < 0) {	/* 0.5sec */
		printf("Playback open error: %s\n", snd_strerror(err));
		exit(EXIT_FAILURE);
	}


        snd_pcm_uframes_t period_size;
        snd_pcm_hw_params_get_period_size(params, &period_size,&dir);
        printf("frames  = %d\n",frames);
        printf("period size = %d\n",period_size);

        //open audio file
        int ret;
        buffer = (unsigned char *)malloc(MAX_SIZE);
        snd_pcm_hw_params_get_period_time(params,&val, &dir);

        printf("period time = %d\n",val);

        snd_pcm_hw_params_get_buffer_time(params, &val, &dir);    
        printf("buffer time = %d us\n", val); 
     
        snd_pcm_hw_params_get_buffer_size(params, (snd_pcm_uframes_t *) &val);    
        printf("buffer size = %d frames\n", val);   
   
        snd_pcm_hw_params_get_periods(params, &val, &dir);    
        printf("periods per buffer = %d frames\n", val);  
  
        unsigned int val2; 
        snd_pcm_hw_params_get_rate_numden(params, &val, &val2);    
        printf("exact rate=%d/%d bps\n", val, val2);
        for (i = 0; 1; i++) {
            //data num_frames
            ret = snd_pcm_readi(handle, buffer, frames); 
            if (ret == -EPIPE) {  
                /* EPIPE means overrun */  
                fprintf(stderr, "overrun occurred\n");  
                snd_pcm_prepare(handle);  
                continue;
            } else if (ret < 0) {  
                fprintf(stderr,  
                  "error from read: %s\n",  
                  snd_strerror(ret));  
            } else if (ret != (int)frames) {  
                fprintf(stderr, "short read, read %d frames\n", ret);  
            }  
            while((ret = snd_pcm_writei(handle_play, buffer, frames)) < 0){
              snd_pcm_prepare(handle_play);
              printf("error from writei: %s\n", snd_strerror(ret));    
            }
        }
        snd_pcm_drain(handle); 
        snd_pcm_close(handle);
        free(buffer);
        return 0;
}

