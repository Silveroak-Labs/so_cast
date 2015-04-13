/**


*/
#ifndef _SOCAST_INCLUDE_
#define _SOCAST_INCLUDE_

#include <stdio.h>

#include "list.h"


#define LISTEN_IP "0.0.0.0"
#define PORT 8883

#define PORT_C 8884


#define MAX_INDEX 65535*10

#define MAX_FRAMES 2048 //传输 的数据大小，2048 比较稳定

#define OK_STR "OK"
#define BC_IP "255.255.255.255"
//定义命令字符串

#define CMD_START "start"
#define CMD_STOP "stop"


typedef struct so_play_frame_
{
	int sequence;
	long long timestamp;
	unsigned char frames[MAX_FRAMES];
	/* data */
} so_play_frame;




extern list_t *FRAME_LIST;


#endif  // _SOCAST_INCLUDE_
