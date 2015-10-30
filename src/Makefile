.PHONY: clean
1;3803;0cLDLIBS = -lrdmacm -libverbs -lpthread -lrt

CC = mpicxx

CFLAGS = -Wall -O3 -std=c++11 -g

OBJS = base/server.o base/process.o base/process_on_completion.o base/process_connection.o base/process_events.o base/process_comm_recv.o base/process_comm_send.o base/process_utils.o base/utils.o
OBJC = base/client.o base/process.o base/process_on_completion.o base/process_connection.o base/process_events.o base/process_comm_recv.o base/process_comm_send.o base/process_utils.o base/utils.o

OBJT = base/test.o base/process.o base/process_on_completion.o base/process_connection.o base/process_events.o base/process_comm_recv.o base/process_comm_send.o base/process_utils.o base/utils.o

all: server client

%.o: %.cpp
	${CC} ${CFLAGS} -c -o $@ $<

server: ${OBJS}
	${CC} -o $@ $^ ${LDLIBS} ${CFLAGS}
	mv server ../bin/.
client: ${OBJC}
	${CC} -o $@ $^ ${LDLIBS} ${CFLAGS}
	mv client ../bin/.
test: ${OBJT}
	${CC} -o $@ $^ ${LDLIBS} ${CFLAGS}
	mv test ../bin/.
clean:
	rm base/*.o ../bin/server ../bin/client
