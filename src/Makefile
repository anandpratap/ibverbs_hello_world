.PHONY: clean

LDLIBS = -lrdmacm -libverbs -lpthread -lrt
CC = g++
CFLAGS = -Wall -O3 -std=c++11 -g
OBJS = server.o shared.o
OBJC = client.o shared.o

%.o: %.cpp
	${CC} ${CFLAGS} -c -o $@ $<

server: ${OBJS}
	${CC} -o $@ $^ ${LDLIBS} ${CFLAGS}
	mv server ../bin/.
client: ${OBJC}
	${CC} -o $@ $^ ${LDLIBS} ${CFLAGS}
	mv client ../bin/.

clean:
	rm *.o server client ../bin/*