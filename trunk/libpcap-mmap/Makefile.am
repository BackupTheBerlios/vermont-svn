#  Copyright (c) 1993, 1994, 1995, 1996
#       The Regents of the University of California.  All rights reserved.
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that: (1) source code distributions
#  retain the above copyright notice and this paragraph in its entirety, (2)
#  distributions including binary code include the above copyright notice and
#  this paragraph in its entirety in the documentation or other materials
#  provided with the distribution, and (3) all advertising materials mentioning
#  features or use of this software display the following acknowledgement:
#  ``This product includes software developed by the University of California,
#  Lawrence Berkeley Laboratory and its contributors.'' Neither the name of
#  the University nor the names of its contributors may be used to endorse
#  or promote products derived from this software without specific prior
#  written permission.
#  THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED
#  WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
#  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
#
#  $Header: /n/CVS/sirt/libpcap/Makefile.am,v 0.8.3.1 2004/10/01 22:21:31 cpw Exp $

#
## Various configurable paths (remember to edit Makefile.am, not Makefile)
#

AUTOMAKE_OPTIONS	= foreign no-dependencies

# library versioning 
VCURRENT		= 1
VREVISION		= 0
VAGE			= 0
RMAJOR		= 0
RMINOR		= 8
RREVISION	= 3

LTRELEASE		= $(RMAJOR).$(RMINOR).$(RREVISION)
LTVERSION		= $(VCURRENT):$(VREVISION):$(VAGE)

PCAP			= @V_PCAP@
PSRC                    = pcap-@V_PCAP@.c
FSRC                    = fad-@V_FINDALLDEVS@.c
FADS			= @V_FADS@

LEX                     = @V_LEX@
YACC                    = @V_YACC@
OTHERS			= @V_OTHERS@
DEFS			= @DEFS@ @V_DEFS@
INCLUDES		= @V_INCLS@

EXTRA_DIST		= LICENSE Makefile.am INSTALL.txt FILES TODO CHANGES \
			  CREDITS README README.Win32 README.aix README.hpux \
			  README.linux README.ring README.tru64 pcap.3 \
			  VERSION REVISION configure.ac acinclude.m4 \
			  FADDERS PCAPPERS SUNOS4 bpf lbl packaging .warrantee \
			  scanner.l grammar.y bootstrap config Win32\
			  pcap-ring.c pcap-dag.c $(OTHERS) $(FADS)

MAINTAINERCLEANFILES    = Makefile.in Makefile.am.bak configure.ac.bak aclocal.m4 configure config.h.in

MOSTLYCLEANFILES        = bpf_filter.c scanner.c grammar.c version.c lex.yy.c \
                          tokdefs.h version.h net

HDR			= arcnet.h atmuni31.h config.h ethertype.h gencode.h \
			  llc.h nlpid.h pcap-bpf.h pcap-int.h pcap-namedb.h \
			  pcap-nit.h pcap-pf.h pcap-ring.h pcap-stdinc.h \
			  pcap.h ppp.h sll.h sunatmpos.h pf.h

GENSRC                  = bpf_filter.c scanner.c grammar.c version.c
CSRC                    = pcap.c inet.c gencode.c optimize.c  nametoaddr.c \
                          etherent.c savefile.c bpf_image.c bpf_dump.c

PCAPSRC			= $(GENSRC) $(PSRC) $(FSRC) $(CSRC)

lib_LTLIBRARIES		= libpcap.la

libpcap_la_LDFLAGS      = -release $(LTRELEASE)
#libpcap_la_LDFLAGS      = -version-info $(LTVERSION)

if USE_RING
libpcap_la_SOURCES	= $(PCAPSRC) $(HDR) pcap-ring.c
else
if USE_DAG
libpcap_la_SOURCES	= $(PCAPSRC) $(HDR) pcap-dag.c
else
libpcap_la_SOURCES      = $(PCAPSRC) $(HDR)
endif
endif

clean-local:
	rm -f libpcap.a

#all-local:
#	rm -f libpcap.a; ln -s .libs/libpcap.a libpcap.a
all-local: libpcap.a

libpcap.a:
	ln -s .libs/libpcap.a libpcap.a

install-data-local:
	rm -f $(DESTDIR)$(includedir)/pcap-bpf.h
	rm -f $(DESTDIR)$(includedir)/pcap.h
	rm -f $(DESTDIR)$(includedir)/pcap-namedb.h
	cp pcap.h $(DESTDIR)$(includedir)
	cp pcap-bpf.h $(DESTDIR)$(includedir)
	cp pcap-namedb.h $(DESTDIR)$(includedir)
	rm -f $(DESTDIR)$(libdir)/libpcap.la
	rm -f $(DESTDIR)$(libdir)/libpcap.so*
	(cd $(DESTDIR)$(libdir); ln -s libpcap-0.$(RMINOR).$(RREVISION).so libpcap.so)
	(cd $(DESTDIR)$(libdir); ln -s libpcap-0.$(RMINOR).$(RREVISION).so libpcap.so.0.$(RMINOR).$(RREVISION))
	(cd $(DESTDIR)$(libdir); ln -s libpcap-0.$(RMINOR).$(RREVISION).so libpcap.so.0.$(RMINOR))
	(cd $(DESTDIR)$(libdir); ln -s libpcap-0.$(RMINOR).$(RREVISION).so libpcap.so.0.7)
	(cd $(DESTDIR)$(libdir); ln -s libpcap-0.$(RMINOR).$(RREVISION).so libpcap.so.0)

# generated source

bpf_filter.c: bpf/net/bpf_filter.c
	rm -f bpf_filter.c
	ln -s bpf/net/bpf_filter.c bpf_filter.c

scanner.o: scanner.c tokdefs.h

scanner.c: scanner.l tokdefs.h
	@rm -f $@
	$(V_LEX) -t scanner.l > $$$$.$@; mv $$$$.$@ $@

tokdefs.h:
	rm -f grammar.c
	make grammar.o

grammar.o: grammar.c

grammar.c: grammar.y
	@rm -f grammar.c tokdefs.h
	$(YACC) -d grammar.y
	mv y.tab.c grammar.c
	mv y.tab.h tokdefs.h

version.o: version.c

version.c: REVISION version.h
	@rm -f $@
	echo `cat VERSION` | \
          sed -e 's/.*/char pcap_version[] = "&";/' > version.c

version.h: $(srcdir)/VERSION
	@rm -f $@
	sed -n -e 's/.*/static const char pcap_version_string[] = "libpcap version &";/p' $(srcdir)/VERSION > $@

