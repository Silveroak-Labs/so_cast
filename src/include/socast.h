/**


*/
#ifndef _SOCAST_INCLUDE_
#define _SOCAST_INCLUDE_

#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <sys/timeb.h>
#include <pthread.h>
#include "list.h"


#define LISTEN_IP "0.0.0.0"
#define PORT 8883


#define MAX_INDEX 65535*10

#define MAX_FRAMES 1024


typedef struct so_play_frame_
{
	long long timestamp;
	unsigned char *frames;
	/* data */
} so_play_frame;


extern list_t *FRAME_LIST;


#endif  // _SOCAST_INCLUDE_
