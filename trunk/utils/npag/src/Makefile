
CC = gcc
CFLAGS = -g -Wall
LDFLAGS = -lpthread
LD = gcc
OBJS = main.o sender.o checksum.o generate_raw.o generate_tcp.o automata.o states.o
PROG = npag

all: $(PROG)

$(PROG): $(OBJS)
	$(LD) $(LDFLAGS) $(OBJS) -o ../$(PROG)
	/bin/rm $(OBJS)

main.o: main.c config.h
	$(CC) $(CFLAGS) -c main.c

sender.o: sender.c config.h
	$(CC) $(CFLAGS) -c sender.c
	
generate_raw.o: generate_raw.c packet.h config.h
	$(CC) $(CFLAGS) -c generate_raw.c
	
generate_tcp.o: generate_tcp.c packet.h config.h
	$(CC) $(CFLAGS) -c generate_tcp.c

automata.o:	automata.c config.h
	$(CC) $(CFLAGS) -c automata.c
	
states.o:	states.c 
	$(CC) $(CFLAGS) -c states.c


checksum.o: checksum.c
	$(CC) $(CFLAGS) -c checksum.c
clean:
	/bin/rm $(PROG) $(OBJS)
	
