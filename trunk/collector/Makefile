#
# Makefile for the IPFIX concentrator project
# Copyright (C) 2004 Christoph Sommer
# http://www.deltadevelopment.de/users/christoph/ipfix
#
# This is a gmake (GNU make) Makefile, don't even try pmake (BSD make)
#

# -- compiler and linker switches ----
CC=gcc
CFLAGS = -Wall
LDFLAGS = 
LDLIBS = -lpthread 

# -- programs, archives and their components ----

PROGS = testCollector
ARCHS = ipfixCollector.a

testCollector = testCollector.o tools.o \
	ipfixCollector.a

ipfixCollector.a = rcvMessage.o rcvIpfix.o templateBuffer.o

# -- generic and phony targets ----

.PHONY: all clean

all: $(PROGS)

build/%.o: src/%.c src/%.h
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	rm -f $(PROGS) build/*

# -- macro-generated targets ----

define PROGSMACRO
$(1): $$(patsubst %, build/%, $$($(1)))
	$(CC) $(LDFLAGS) -o $$@ $$^ $(LDLIBS)
endef

define ARCHSMACRO
build/$(1): $$(patsubst %, build/$(1)(build/%), $$($(1)))
endef

$(foreach prog,$(PROGS),$(eval $(call PROGSMACRO,$(prog))))
$(foreach arch,$(ARCHS),$(eval $(call ARCHSMACRO,$(arch))))

# -- EOF: here be dragons ----
