CC=gcc
CFLAGS+=-O -Wall #-I/usr/include/sys/
TARGET=libipfixlo.a

all: $(TARGET)
clean:
	rm -f $(TARGET) *.o

$(TARGET): ipfixlolib.o encoding.o ipfix_names.o
	ar cru $(TARGET) ipfixlolib.o encoding.o ipfix_names.o
	ranlib $(TARGET)

ipfixlolib.o: ipfixlolib.c ipfixlolib.h encoding.h
encoding.o: encoding.c encoding.h
ipfix_names.o: ipfix_names.c ipfix_names.h

