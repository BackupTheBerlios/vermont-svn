#ifndef VERMONT_H
#define VERMONT_H

#include <vector>

#include "iniparser.h"
#include "sampler/Template.h"
#include "sampler/Observer.h"
#include "sampler/Filter.h"
#include "sampler/PacketProcessor.h"
#include "sampler/ExporterSink.h"
#include "sampler/HookingFilter.h"

#include "concentrator/rcvIpfix.h"
#include "concentrator/aggregator.h"
#include "concentrator/sndIpfix.h"


/* holding all objects/handles/... for the subsystems like sampler and collector */
struct v_objects {

	/* main vermont: */
        /* the configuration struct, from iniparser */
	dictionary *v_config;
	/* initialized subsystems */
        unsigned int v_subsystems;
        /* starting time */
        time_t v_starttime;

	/* for sampler: */
	Template *templ;
	Observer *observer;
        Filter *filter;
        Sink *sink;

	/* and this is pragmatic */
        HookingFilter *hooking;

	/* for concentrator: */
        IpfixReceiver *conc_receiver;
        IpfixAggregator *conc_aggregator;
        IpfixSender *conc_exporter;

        /* poll aggregator this often */
        unsigned short conc_poll_ms;

        int conc_exitflag;

};

#endif
