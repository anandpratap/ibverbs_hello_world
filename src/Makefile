.PHONY: clean

LDLIBS = -lrdmacm -libverbs -lpthread -lrt
CC = g++
CFLAGS = -Wall -O0 -std=c++11 -g
OBJS = base/server.o base/process.o base/process_on_completions.o base/process_connections.o base/process_events.o base/process_comm_recv.o base/process_comm_send.o base/utils.o
OBJC = base/client.o base/process.o base/process_on_completions.o base/process_connections.o base/process_events.o base/process_comm_recv.o base/process_comm_send.o base/utils.o

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
	rm base/*.o ../bin/server ../bin/client