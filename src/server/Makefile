LOCAL_I=../include
//INCLUDES=-I/usr/include -I../include
INCLUDES=-I../include

SOCAST_LDFLAGS=-pthread -lasound -ldl -lrt -lm 

CC=mipsel-openwrt-linux-gcc
GCC=gcc


CFLAGS=$(SOCAST_LDFLAGS) $(INCLUDES)

HEADS=$(LOCAL_I)/socast.h
SRCS=play_server.c $(LOCAL_I)/socast.c $(LOCAL_I)/ialsalib.c

OBJS=$(SRCS:.c=.o)


PROGRAME=socast


all:$(PROGRAME)

debug:$(OBJS)
	$(CC) -o $(PROGRAME) $(SRCS) $(CFLAGS) -D DEBUG

$(PROGRAME):$(OBJS)
	$(CC) -o $(PROGRAME) $(SRCS) $(CFLAGS)

gcc:$(OBJS)
	$(GCC) -o $(PROGRAME) $(SRCS) $(CFLAGS)

$(OBJS):$(HEADS)
           
.PHONY : clean
clean:
	-rm $(PROGRAME) $(OBJS)


