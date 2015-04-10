/**


*/
#ifndef _SOCAST_INCLUDE_
#define _SOCAST_INCLUDE_

#include <stdio.h>

#include "list.h"


#define LISTEN_IP "0.0.0.0"
#define PORT 8883


#define MAX_INDEX 65535*10

#define MAX_FRAMES 512


typedef struct so_play_frame_
{
	long long timestamp;
	unsigned char frames[MAX_FRAMES];
	/* data */
} so_play_frame;


extern list_t *FRAME_LIST;


#endif  // _SOCAST_INCLUDE_
