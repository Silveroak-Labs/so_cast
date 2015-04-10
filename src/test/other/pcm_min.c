/*
 *  This extra small demo sends a random samples to your speakers.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <alsa/asoundlib.h>
static char *device = "default";			/* playback device */

snd_output_t *output = NULL;
unsigned char buffer[16*1024];				/* some random data */

int main(int argc, char *argv[])
{
        int err;
        unsigned int i;
        snd_pcm_t *handle;
        snd_pcm_sframes_t frames;

        int fd;
        int ret;
        fd = open(argv[1],O_RDONLY);
        printf("stdout to pcm : filename=%s\n",argv[1]);
        if ( fd < 0 ) {
            printf("open file %s error\n",argv[1]);
            exit(1);
        }
        if(ret = pread(fd,buffer,16*1024,0)==-1){
             printf("pread error\n");
             exit(1);
        }   

        close(fd);
        printf("buffer %s\n",buffer);
	if ((err = snd_pcm_open(&handle, device, SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
		printf("Playback open error: %s\n", snd_strerror(err));
		exit(EXIT_FAILURE);
	}
	if ((err = snd_pcm_set_params(handle,
	                              SND_PCM_FORMAT_S16_LE,
	                              SND_PCM_ACCESS_RW_INTERLEAVED,
	                              1,
	                              48000,
	                              1,
	                              500000)) < 0) {	/* 0.5sec */
		printf("Playback open error: %s\n", snd_strerror(err));
		exit(EXIT_FAILURE);
	}

        for (i = 0; i < 16; i++) {
                frames = snd_pcm_writei(handle, buffer, sizeof(buffer));
                if (frames < 0)
                        frames = snd_pcm_recover(handle, frames, 0);
                if (frames > 0 && frames < (long)sizeof(buffer))
                        printf("Short write (expected %li, wrote %li)\n", (long)sizeof(buffer), frames);
        }

	snd_pcm_close(handle);
	return 0;
}
