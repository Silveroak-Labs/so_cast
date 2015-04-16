#ifndef _SOCAST_INCLUDE_
#define _SOCAST_INCLUDE_

#include <stdio.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <sys/fcntl.h>

#include <sys/timeb.h>

#include <sys/types.h>
#include <unistd.h>

#include <pthread.h>
#include <alsa/asoundlib.h> 


#define LISTEN_IP "0.0.0.0"
#define PORT 8883

#define PORT_C 8884


#define MAX_INDEX 1024*8

#define MAX_FRAMES 2048 //传输 的数据大小，2048 比较稳定

#define OK_STR "OK"
#define BC_IP "255.255.255.255"
//定义命令字符串

#define CMD_START 1
#define CMD_STOP 2


typedef struct so_play_frame_
{
	int sequence;
	struct timeval timestamp;
	unsigned char frames[MAX_FRAMES];
	/* data */
} so_play_frame;

typedef struct so_play_cmd_
{
	int type;  //1.start 2.stop
	struct timeval current_t;

} so_play_cmd;

long long getSystemTime();
void close_playback(snd_pcm_t *PCM_HANDLE);
void print_timestamp();

void Die(char *mess);

extern so_play_frame FRAME_LIST[MAX_INDEX];


#endif  // _SOCAST_INCLUDE_
