CC=gcc
#CFLAGS=-O -Wall
TARGET=libipfixlo.a

# flags for debugging:
CFLAGS=-g -O0 -Wall 

all: $(TARGET)
clean:
	rm -f $(TARGET) *.o

$(TARGET): ipfixlolib.o encoding.o
	ar cru $(TARGET) ipfixlolib.o encoding.o
	ranlib $(TARGET)

ipfixlolib.o: ipfixlolib.c ipfixlolib.h encoding.h
encoding.o: encoding.c encoding.h

