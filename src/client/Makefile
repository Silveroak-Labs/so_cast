LOCAL_I=../include
LIB_I=../lib
#INCLUDES=-I/usr/include -I../include -I../lib
INCLUDES=-I/usr/include -I../include


#APR_CFG=$(apr-1-config --cflags --cppflags --includes --link-ld)

#SOCAST_LDFLAGS=-lpthread -rdynamic -ldl
SOCAST_LDFLAGS=-pthread -lasound

CC=gcc

CFLAGS=$(SOCAST_LDFLAGS) $(INCLUDES)

HEADS=$(LOCAL_I)/socast.h
SRCS=speaker.c $(LOCAL_I)/socast.c

OBJS=$(SRCS:.c=.o)

#LIBS_OBJS=$(LIB_I)/list.o $(LIB_I)/list_iterator.o $(LIB_I)/list_node.o

#LIBS+=-L $(LIB_I)

PROGRAME=speaker

#all:lib $(PROGRAME)
all:$(PROGRAME)
#lib:
	#make -C $(LIB_I)

$(PROGRAME):$(OBJS)
	#$(CC) $(LIBS_OBJS) -o $(PROGRAME) $(SRCS) $(CFLAGS)
	$(CC) -o $(PROGRAME) $(SRCS) $(CFLAGS)

$(OBJS):$(HEADS)
           
.PHONY : clean
clean:
	-rm $(PROGRAME) $(OBJS)


