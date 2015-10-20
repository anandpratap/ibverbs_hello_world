.PHONY: clean

LDLIBS = -lrdmacm -libverbs -lpthread -lrt
CC = gcc
CFLAGS = -Wall
OBJS = server.o shared_ib.o
OBJC = client.o shared_ib.o

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

server: $(OBJS)
	$(CC) -o $@ $^ $(LDLIBS) 

client: $(OBJC)
	$(CC) -o $@ $^ $(LDLIBS)

clean:
	rm *.o server client