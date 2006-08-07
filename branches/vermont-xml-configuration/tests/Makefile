INCLUDES=-I. -I/usr/include/libxml2/
CC=gcc
CXX=g++
CFLAGS=-O2 -g $(INCLUDES) -pg
CXXFLAGS=$(CFLAGS)
LIBS=-lstdc++ -lxml2
LDFLAGS=$(LIBS) $(CFLAGS) -L/usr/lib/libxml2/
TARGET=vermont-tests
OBJS=vermont_tests.o configuration_tests.o ../vermont_configuration.o ../observer_configuration.o ../collector_configuration.o ../metering_configuration.o ../exporter_configuration.o
MODULES_LIBS=../sampler/libsampler.a ../ipfixlolib/libipfixlo.a ../concentrator/libconcentrator.a


all: $(TARGET)

$(TARGET): $(OBJS) $(MODULES_LIBS)
	g++ $(CFLAGS) -o $(TARGET) $(OBJS) $(MODULES_LIBS) $(LIBS)

clean:
	rm -f $(TARGET) *.o
	rm -f *~


vermont_tests.o: vermont_tests.h vermont_tests.cpp configuration_tests.h \
configuration_tests.cpp

configuration_tests.o: configuration_tests.h configuration_tests.cpp

