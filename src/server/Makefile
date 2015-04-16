LOCAL_I=../include
INCLUDES=-I/usr/include -I../include

SOCAST_LDFLAGS=-pthread -lasound

CC=gcc


CFLAGS=$(SOCAST_LDFLAGS) $(INCLUDES)

HEADS=$(LOCAL_I)/socast.h
SRCS=play_server.c $(LOCAL_I)/socast.c

OBJS=$(SRCS:.c=.o)


PROGRAME=socast

all:$(PROGRAME)


$(PROGRAME):$(OBJS)
	$(CC) -o $(PROGRAME) $(SRCS) $(CFLAGS)

$(OBJS):$(HEADS)
           
.PHONY : clean
clean:
	-rm $(PROGRAME) $(OBJS)


