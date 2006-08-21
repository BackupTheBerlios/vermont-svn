SUBDIRS=concentrator ipfixlolib sampler tools

bin_programs=vermont

vermont_sources=collector_configuration.cc collector_configuration.h exporter_configuration.cc \
exporter_configuration.h metering_configuration.cc metering_configuration.h msg.c msg.h observer_configuration.cc \
observer_configuration.h subsystems.c subsystems.h uri.c uri.h vermont.cc vermont_configuration.cc \
vermont_configuration.h vermont.h

AM_CFLAGS=-I$(top_srcdir)
AM_CXXFLAGS=$(AM_CFLAGS)
AM_LDFLAGS=-L$(top_builddir)/concentrator -L$(top_builddir)/sampler -L$(top_builddir)/ipfixlolib
vermont_LDADD=$(top_builddir)/concentrator/libconcentrator.a $(top_builddir)/sampler/libsampler.a $(top_builddir)/ipfixlolib/libipfixlo.a
vermont_DEPENDENCIES=$(top_builddir)/concentrator/libconcentrator.a $(top_builddir)/sampler/libsampler.a $(top_builddir)/ipfixlolib/libipfixlo.a
