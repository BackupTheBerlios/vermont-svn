CC=gcc
# default flags:
#CFLAGS=-O -Wall
# flags for debugging:
CFLAGS=-g -O0 -Wall

INCLUDES=-I../src

TARGET=init_deinit_test template_generation_test

all: $(TARGET)

clean:
	rm -f $(TARGET) *.o

testrun:
# loop over all targets and run them
# not there yet
	echo "not implemented target"

init_deinit_test: init_deinit_test.c
	$(CC) $(INCLUDES) $(CFLAGS) -o  init_deinit_test init_deinit_test.c ../src/libipfixlo.a
template_generation_test: template_generation_test.c
	$(CC) $(INCLUDES) $(CFLAGS)  -o template_generation_test template_generation_test.c ../src/libipfixlo.a

