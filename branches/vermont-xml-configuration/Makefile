INCLUDES=-I.
CC=gcc
CFLAGS=-O2 -g $(INCLUDES) -pg
CXXFLAGS=$(CFLAGS)
LIBS=-lstdc++ -lpcap -lpthread -lm
LDFLAGS=$(LIBS) $(CFLAGS)
TARGET=vermont
OBJS=vermont.o subsystems.o msg.o iniparser.o uri.o concentrator_configuration.o sampler_configuration.o vermont_configuration.o
MODULES=ipfixlolib sampler concentrator
MODULES_LIBS=sampler/libsampler.a ipfixlolib/libipfixlo.a concentrator/libconcentrator.a

all: modules $(TARGET)

modules:
	for dir in $(MODULES); do \
		(cd $$dir; $(MAKE) all); \
	done

$(TARGET): $(OBJS) $(MODULES_LIBS)
	g++ $(CFLAGS) -o $(TARGET) $(OBJS) $(MODULES_LIBS) $(LIBS)

clean:
	rm -f *.o $(TARGET) *~
	for dir in $(MODULES); do \
		(cd $$dir; $(MAKE) clean); \
	done

distclean: clean
	rm -f *~

concentrator_configuration.o: concentrator_configuration.cc vermont.h iniparser.h uri.h \
  sampler/Template.h sampler/Packet.h msg.h sampler/Lock.h \
  sampler/Observer.h sampler/Thread.h sampler/ConcurrentQueue.h \
  sampler/TimeoutSemaphore.h sampler/PacketReceiver.h sampler/Filter.h \
  sampler/PacketProcessor.h sampler/SystematicSampler.h \
  sampler/RandomSampler.h sampler/ExporterSink.h ipfixlolib/ipfixlolib.h \
  ipfixlolib/encoding.h ipfixlolib/ipfix_names.h sampler/Sink.h \
  sampler/HookingFilter.h concentrator/rcvIpfix.h \
  concentrator/aggregator.h concentrator/rules.h concentrator/hashing.h \
  concentrator/sndIpfix.h subsystems.h concentrator_configuration.h
sampler_configuration.o: sampler_configuration.cc ipfixlolib/ipfixlolib.h \
  ipfixlolib/encoding.h ipfixlolib/ipfix_names.h sampler/Packet.h msg.h \
  sampler/Lock.h sampler/Filter.h sampler/ConcurrentQueue.h \
  sampler/TimeoutSemaphore.h sampler/Thread.h sampler/PacketReceiver.h \
  sampler/PacketProcessor.h sampler/SystematicSampler.h \
  sampler/RandomSampler.h sampler/Observer.h sampler/PacketSink.h \
  sampler/Sink.h sampler/ExporterSink.h sampler/Template.h \
  sampler/IPHeaderFilter.h sampler/HookingFilter.h \
  concentrator/sampler_hook_entry.h vermont.h iniparser.h uri.h \
  concentrator/rcvIpfix.h concentrator/aggregator.h concentrator/rules.h \
  concentrator/hashing.h concentrator/sndIpfix.h subsystems.h \
  sampler_configuration.h
vermont_configuration.o: vermont_configuration.cc iniparser.h msg.h subsystems.h vermont.h uri.h \
  sampler/Template.h sampler/Packet.h sampler/Lock.h sampler/Observer.h \
  sampler/Thread.h sampler/ConcurrentQueue.h sampler/TimeoutSemaphore.h \
  sampler/PacketReceiver.h sampler/Filter.h sampler/PacketProcessor.h \
  sampler/SystematicSampler.h sampler/RandomSampler.h \
  sampler/ExporterSink.h ipfixlolib/ipfixlolib.h ipfixlolib/encoding.h \
  ipfixlolib/ipfix_names.h sampler/Sink.h sampler/HookingFilter.h \
  concentrator/rcvIpfix.h concentrator/aggregator.h concentrator/rules.h \
  concentrator/hashing.h concentrator/sndIpfix.h config_sampler.h \
  config_concentrator.h
iniparser.o: iniparser.c iniparser.h msg.h
msg.o: msg.c msg.h
subsystems.o: subsystems.c subsystems.h msg.h
uri.o: uri.c uri.h
