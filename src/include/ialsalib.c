/*
 *  Latency test program
 *
 *     Author: Jaroslav Kysela <perex@perex.cz>
 *
 *     Author of bandpass filter sweep effect:
 *	       Maarten de Boer <mdeboer@iua.upf.es>
 *
 *  This small demo program can be used for measuring latency between
 *  capture and playback. This latency is measured from driver (diff when
 *  playback and capture was started). Scheduler is set to SCHED_RR.
 *
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sched.h>
#include <errno.h>
#include <getopt.h>
#include <alsa/asoundlib.h>
#include <sys/time.h>
#include <math.h>


int setparams_stream(snd_pcm_t *handle,
		     snd_pcm_hw_params_t *params,
			 snd_pcm_format_t format,
			 int channels,
			 int rate,
		     const char *id)
{
    int resample = 1;
	int err;
	unsigned int rrate;
	printf("setparams_stream\n");

	err = snd_pcm_hw_params_any(handle, params);
	if (err < 0) {
		printf("Broken configuration for %s PCM: no configurations available: %s\n", snd_strerror(err), id);
		return err;
	}
	err = snd_pcm_hw_params_set_rate_resample(handle, params, resample);
	if (err < 0) {
		printf("Resample setup failed for %s (val %i): %s\n", id, resample, snd_strerror(err));
		return err;
	}
	err = snd_pcm_hw_params_set_access(handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
	if (err < 0) {
		printf("Access type not available for %s: %s\n", id, snd_strerror(err));
		return err;
	}
	err = snd_pcm_hw_params_set_format(handle, params, format);
	if (err < 0) {
		printf("Sample format not available for %s: %s\n", id, snd_strerror(err));
		return err;
	}
	err = snd_pcm_hw_params_set_channels(handle, params, channels);
	if (err < 0) {
		printf("Channels count (%i) not available for %s: %s\n", channels, id, snd_strerror(err));
		return err;
	}
	rrate = rate;
	err = snd_pcm_hw_params_set_rate_near(handle, params, &rrate, 0);
	if (err < 0) {
		printf("Rate %iHz not available for %s: %s\n", rate, id, snd_strerror(err));
		return err;
	}
	if ((int)rrate != rate) {
		printf("Rate doesn't match (requested %iHz, get %iHz)\n", rate, err);
		return -EINVAL;
	}
	return 0;
}

int setparams_bufsize(snd_pcm_t *handle,
		      snd_pcm_hw_params_t *params,
		      snd_pcm_hw_params_t *tparams,
		      snd_pcm_uframes_t bufsize,
		      const char *id)
{
	int err;
	snd_pcm_uframes_t periodsize;

	snd_pcm_hw_params_copy(params, tparams);
	periodsize = bufsize * 2;
	err = snd_pcm_hw_params_set_buffer_size_near(handle, params, &periodsize);
	if (err < 0) {
		printf("Unable to set buffer size %li for %s: %s\n", bufsize * 2, id, snd_strerror(err));
		return err;
	}
	periodsize /= 2;
	err = snd_pcm_hw_params_set_period_size_near(handle, params, &periodsize, 0);
	if (err < 0) {
		printf("Unable to set period size %li for %s: %s\n", periodsize, id, snd_strerror(err));
		return err;
	}
	return 0;
}

int setparams_set(snd_pcm_t *handle,
		  snd_pcm_hw_params_t *params,
		  snd_pcm_sw_params_t *swparams,
		  const char *id)
{
	int err;
	snd_pcm_uframes_t val;

	err = snd_pcm_hw_params(handle, params);
	if (err < 0) {
		printf("Unable to set hw params for %s: %s\n", id, snd_strerror(err));
		return err;
	}
	err = snd_pcm_sw_params_current(handle, swparams);
	if (err < 0) {
		printf("Unable to determine current swparams for %s: %s\n", id, snd_strerror(err));
		return err;
	}
	err = snd_pcm_sw_params_set_start_threshold(handle, swparams, 0x7fffffff);
	if (err < 0) {
		printf("Unable to set start threshold mode for %s: %s\n", id, snd_strerror(err));
		return err;
	}
	val = 4;
	err = snd_pcm_sw_params_set_avail_min(handle, swparams, val);
	if (err < 0) {
		printf("Unable to set avail min for %s: %s\n", id, snd_strerror(err));
		return err;
	}
	err = snd_pcm_sw_params(handle, swparams);
	if (err < 0) {
		printf("Unable to set sw params for %s: %s\n", id, snd_strerror(err));
		return err;
	}
	return 0;
}

int setparams(snd_pcm_t *chandle, int *bufsize,snd_pcm_format_t format,int channels,int rate)
{
	int err;
	snd_pcm_hw_params_t *ct_params;	/* templates with rate, format and channels */
	snd_pcm_hw_params_t *c_params;
	snd_pcm_sw_params_t *c_swparams;
	snd_pcm_uframes_t  c_size , c_psize;
	unsigned int  c_time;
	unsigned int val;

	snd_pcm_hw_params_alloca(&c_params);
	snd_pcm_hw_params_alloca(&ct_params);
	snd_pcm_sw_params_alloca(&c_swparams);
	
	if ((err = setparams_stream(chandle, ct_params,format,channels, rate,"capture")) < 0) {
		printf("Unable to set parameters for playback stream: %s\n", snd_strerror(err));
		exit(0);
	}

	if ((err = setparams_bufsize(chandle, c_params, ct_params, *bufsize, "capture")) < 0) {
		printf("Unable to set sw parameters for playback stream: %s\n", snd_strerror(err));
		exit(0);
	}

	snd_pcm_hw_params_get_period_size(c_params, &c_psize, NULL);
	if (c_psize > (unsigned int)*bufsize)
		*bufsize = c_psize;
	snd_pcm_hw_params_get_period_time(c_params, &c_time, NULL);
	
	
	snd_pcm_hw_params_get_buffer_size(c_params, &c_size);
	if (c_psize * 2 < c_size) {
                snd_pcm_hw_params_get_periods_min(c_params, &val, NULL);
		if (val > 2 ) {
			printf("capture device does not support 2 periods per buffer\n");
			exit(0);
		}
	}
	
	if ((err = setparams_set(chandle, c_params, c_swparams, "capture")) < 0) {
		printf("Unable to set sw parameters for playback stream: %s\n", snd_strerror(err));
		exit(0);
	}

	printf("set params end\n");
	return 0;
}

void showstat(snd_pcm_t *handle, size_t frames)
{
	int err;
	snd_output_t *output = NULL;
	err = snd_output_stdio_attach(&output, stdout, 0);
	if (err < 0) {
		printf("Output failed: %s\n", snd_strerror(err));
		return;
	}
	snd_pcm_status_t *status;

	snd_pcm_status_alloca(&status);
	if ((err = snd_pcm_status(handle, status)) < 0) {
		printf("Stream status error: %s\n", snd_strerror(err));
		exit(0);
	}
	printf("*** frames = %li ***\n", (long)frames);
	snd_pcm_status_dump(status, output);
}

void showlatency(size_t latency,int rate)
{
	double d;
	latency *= 2;
	d = (double)latency / (double)rate;
	printf("Trying latency %li frames, %.3fus, %.6fms (%.4fHz)\n", (long)latency, d * 1000000, d * 1000, (double)1 / d);
}


int open_and_set_pcm(snd_pcm_t **chandle,char *cdevice,snd_pcm_stream_t stream,snd_pcm_format_t format,snd_pcm_access_t access,int rate,int channels,int latency,int frame_num,char *buffer){
	int err;
	if ((err = snd_pcm_open(&(*chandle), cdevice, SND_PCM_STREAM_CAPTURE, 0)) < 0) {
		printf("Record open error: %s\n", snd_strerror(err));
		return -1;
	}

	if (setparams(*chandle, &latency,format,channels,rate) < 0){
		fprintf(stderr, "set params failed\n");
	    return -2;	
	}
	if ((err = snd_pcm_set_params(*chandle,
	     format,
         access,
         channels,
	     rate,
	         1,
	     latency)) < 0) {   /* 0.5sec */
	    printf("Playback open error: %s\n", snd_strerror(err));
	    return -3;	
	}

	if (snd_pcm_format_set_silence(format, buffer, frame_num*channels) < 0) {
		fprintf(stderr, "silence error\n");
	    return -4;	
	}

	if ((err = snd_pcm_start(*chandle)) < 0) {
		printf("Go error: %s\n", snd_strerror(err));
	    return -5;	
	}
	return 0;

}
void close_pcm(snd_pcm_t *chandle){
	snd_pcm_drop(chandle);
	snd_pcm_hw_free(chandle);
	snd_pcm_close(chandle);
}
/**
int main(int argc, char *argv[])
{
	
	snd_pcm_t  *chandle;
    char *buffer;
	int frame_num,err; 
    ssize_t r;

    int buffer_size=2048;
    char *cdevice = "default";
	//setscheduler();

	printf("Capture device is %s\n", cdevice);
//open_and_set_pcm(snd_pcm_t *chandle,char *cdevice,snd_pcm_stream_t stream,snd_pcm_format_t format,snd_pcm_access_t access,int rate,int channels,int latency){
	frame_num = buffer_size/4;
	buffer = malloc(buffer_size);
    open_and_set_pcm(&chandle,cdevice,SND_PCM_STREAM_CAPTURE,SND_PCM_FORMAT_S16_LE,SND_PCM_ACCESS_RW_INTERLEAVED,44100,2,500000,frame_num,buffer);
    	

	int j=0;
	int fd = open("/tmp/a.wav",O_WRONLY|O_CREAT);


	printf("start read\n");
	while(j<10000){
		bzero(buffer,buffer_size);
	    do {
	      	r = snd_pcm_readi(chandle, buffer, frame_num);
	    } while (r == -EAGAIN);
		if(r<0){
			fprintf(stderr,"overrun occurred\n");
	   	    snd_pcm_prepare(chandle);
		}
		printf("j=%d\n",j);
		write(fd,buffer,buffer_size);
		j++;
	}
    close(fd);	
	printf("Capture:\n");
    close_pcm(chandle);	
	return 0;
}
**/
