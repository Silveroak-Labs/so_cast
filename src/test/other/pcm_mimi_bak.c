/*
 *  This extra small demo sends a random samples to your speakers.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <alsa/asoundlib.h>
#define MAX_SIZE 16*1024

static char *device = "default";                        /* playback device */
snd_output_t *output = NULL;
unsigned char *buffer;                          /* some random data */

int main(int argc,char *argv[])
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
                                      1,
                                      96000,
                                      1,
                                      500000)) < 0) {   /* 0.5sec */
                printf("Playback open error: %s\n", snd_strerror(err));
                exit(EXIT_FAILURE);
        }
        //open audio file
        int fd;
        int ret;
        fd = open(argv[1],O_RDONLY);
        printf("stdout to pcm : filename=%s\n",argv[1]);
        if ( fd < 0 ) {
            printf("open file %s error\n",argv[1]);
        }else{
            buffer = (unsigned char *)malloc(MAX_SIZE);
            for (i = 0; i < 1000; i++) {
                //data num_frames
                if(ret = pread(fd,buffer,MAX_SIZE,MAX_SIZE*i)<0){
                    printf("pread error\n");
                    exit(1);
                }   
                printf("i=%d\n",i);
                while ((frames = snd_pcm_writei(handle, buffer, MAX_SIZE/2)) < 0) {
                      snd_pcm_prepare(handle);
                      fprintf(stderr, "<<<<<<<<<<<<<<< Buffer Underrun >>>>>>>>>>>>>>>\n");
                }
              printf("Test %d ret:%d, frames %d\n",i,ret,frames);
                
            }
        }
        close(fd);
        snd_pcm_close(handle);
        return 0;
}
