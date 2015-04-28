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

#include <net/if.h>
#include <net/if_arp.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#include <unistd.h>

#include <arpa/inet.h>
#include <errno.h>

#define LISTEN_IP "0.0.0.0"
#define PORT_B 18884

#define PORT 8883

#define PORT_C 8884


#define MAX_INDEX 1024*8

#define MAX_FRAMES 1024*2 //传输 的数据大小，2048 比较稳定

#define OK_STR "OK"
//定义命令字符串

#define CMD_START 1
#define CMD_STOP 2
#define CMD_ADD 0 //增加设备到server中

#define CMD_FIND "find_server"

#define MAXINTERFACES 16    /* 最大接口数 */


typedef struct so_play_frame_
{
	int sequence;
	long long timestamp;
	char frames[MAX_FRAMES];
	/* data */
} so_play_frame;

typedef struct so_play_cmd_
{
	int type;  //1.start 2.stop
	long long current_t;

} so_play_cmd;


typedef struct so_play_server_process_
{
	char msg[64];
	struct sockaddr_in from_server;
	int from_len;
} so_sp;

typedef struct so_play_speaker_
{
	struct sockaddr_in address;
	int addr_len;
} so_speaker;

typedef struct so_network_io_
{
	char name[64];
	struct in_addr ip;
	struct in_addr netmask;
	struct in_addr broadcast;
	int is_up;  //1 up 0 down
} so_network_io;


long long getSystemTime();
void close_playback(snd_pcm_t *PCM_HANDLE);
void print_timestamp();

int get_nio_nums();
int get_network_io(so_network_io *buffer[MAXINTERFACES]);

int get_ip(char *name,struct ifreq *ifr);

void Die(char *mess);

long unsigned long get_tsf(int *fd);

int open_and_set_pcm(snd_pcm_t **chandle,char *cdevice,snd_pcm_stream_t stream,snd_pcm_format_t format,snd_pcm_access_t access,int rate,int channels,int latency,int frame_num,char *buffer);
void close_pcm(snd_pcm_t *channel);

extern so_play_frame FRAME_LIST[MAX_INDEX];



#endif  // _SOCAST_INCLUDE_
