CC=gcc
CFLAGS=-O -Wall
INCLUDES=-I../src

TARGET=init_deinit template_generation identifier_naming

all: $(TARGET)

clean:
	rm -f $(TARGET) *.o

testrun:
# loop over all targets and run them
# not there yet
	echo "not implemented target"

init_deinit: init_deinit.c
	$(CC) $(INCLUDES) $(CFLAGS) -o  init_deinit init_deinit.c ../src/libipfixlo.a

template_generation: template_generation.c
	$(CC) $(INCLUDES) $(CFLAGS)  -o template_generation template_generation.c ../src/libipfixlo.a

identifier_naming: identifier_naming.c
	$(CC) $(INCLUDES) $(CFLAGS)  -o identifier_naming identifier_naming.c ../src/libipfixlo.a
