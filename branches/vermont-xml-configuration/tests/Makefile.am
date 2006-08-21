bin_PROGRAMS=vtests

vtests_SOURCES=vermont_tests.cpp vermont_tests.h configuration_tests.cpp configuration_tests.h ../msg.h \
../msg.c ../vermont_configuration.cc ../vermont_configuration.h ../collector_configuration.cc \
../collector_configuration.h ../exporter_configuration.cc ../exporter_configuration.h ../metering_configuration.cc \
../metering_configuration.h ../observer_configuration.cc ../observer_configuration.h

AM_CFLAGS=-I$(top_srcdir) `xml2-config --cflags`
AM_CXXFLAGS=$(AM_CFLAGS)
AM_LDFLAGS=-L$(top_builddir)/concentrator -L$(top_builddir)/ipfixlolib -L$(topbuilddir)/sampler/ -lpthread -lpcap `xml2-config --libs`
vtests_LDADD=$(top_builddir)/concentrator/libconcentrator.a $(top_builddir)/ipfixlolib/libipfixlo.a $(top_builddir)/sampler/libsampler.a
vtests_DEPENDENCIES=$(top_builddir)/concentrator/libconcentrator.a $(top_builddir)/ipfixlolib/libipfixlo.a $(top_builddir)/sampler/libsampler.a
