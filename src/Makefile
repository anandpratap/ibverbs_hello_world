.PHONY: clean

LDLIBS = -lrdmacm -libverbs -lpthread -lrt
CC = g++
CFLAGS = -Wall -O0 -std=c++11
OBJS = server.o shared.o utils.o
OBJC = client.o shared.o utils.o

all: server client

%.o: %.cpp
	${CC} ${CFLAGS} -c -o $@ $<

server: ${OBJS}
	${CC} -o $@ $^ ${LDLIBS} ${CFLAGS}
	mv server ../bin/.
client: ${OBJC}
	${CC} -o $@ $^ ${LDLIBS} ${CFLAGS}
	mv client ../bin/.

clean:
	rm *.o ../bin/server ../bin/client