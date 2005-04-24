#
# Makefile for the IPFIX concentrator project
# Copyright (C) 2004 Christoph Sommer
# http://www.deltadevelopment.de/users/christoph/ipfix
#
# This is a gmake (GNU make) Makefile, don't even try pmake (BSD make)
#

ARCH ?= BSD

# -- compiler and linker switches ----

CFLAGS += -pg -Wall -Werror
LDFLAGS += -pg 

ifeq ($(ARCH),BSD)
CFLAGS += -pthread
LDFLAGS += -pthread
else
LDLIBS += -l pthread 
endif

# -- programs, archives and their components ----

PROGS = testRules testCollector testHashing testConcentrator
ARCHS = ipfixCollector.a

testConcentrator = testConcentrator.o concentrator.o rules.o common.o ipfix.o crc16.o hashing.o config.o sndIpfix.o \
	ipfixCollector.a ../ipfixlolib/libipfixlo.a

testHashing = testHashing.o rules.o common.o ipfix.o crc16.o hashing.o config.o sndIpfix.o \
	ipfixCollector.a ../ipfixlolib/libipfixlo.a

testRules = testRules.o rules.o common.o ipfix.o \
	ipfixCollector.a

testCollector = testCollector.o common.o \
	ipfixCollector.a

ipfixCollector.a = rcvIpfix.o templateBuffer.o common.o ipfix.o

# -- generic and phony targets ----

.PHONY: all clean

all: $(PROGS)

build/%.o: src/%.c src/%.h
	-@tput setf 1
	-@echo "["$@"]" 
	-@tput sgr0
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	rm -f $(PROGS) gmon.out build/*

# -- macro-generated targets ----

define PROGSMACRO
$(1): $$(patsubst %, build/%, $$($(1)))
	-@tput setf 1
	-@echo "["$$@"]" 
	-@tput sgr0
	$(CC) $(LDFLAGS) -o $$@ $$^ $(LDLIBS)
endef

define ARCHSMACRO
build/$(1): $$(patsubst %, build/$(1)(build/%), $$($(1)))
endef

$(foreach prog,$(PROGS),$(eval $(call PROGSMACRO,$(prog))))
$(foreach arch,$(ARCHS),$(eval $(call ARCHSMACRO,$(arch))))

# -- EOF: here be dragons ----
