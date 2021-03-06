/*
 this is vermont.
 released under GPL v2

 (C) by Ronny T. Lampert

 */

/* own systems */
#include "vermont.h"
#include "iniparser.h"
#include "msg.h"
#include "uri.h"
#include "subsystems.h"
#include "config_concentrator.h"

#define CONC_POLL_MS 500
/* section in config we get our values from */
static char *CONF_SEC="concentrator";

/*
 this is a f*cking mess.
 this really needs to be split up, but I really dont care anymore
 */
int configure_concentrator(struct v_objects *v)
{
	char *poll_ms, *source_id;
	char *rules, *buffertime_min, *buffertime_max;

	char *listen;
        uint16_t listen_portn;

	int sID, exports=0;

	dictionary *conf=v->v_config;
        IpfixSender *ips=NULL;
        IpfixAggregator *ipa=NULL;

        IpfixCollector *ipc=NULL;
	IpfixReceiver  *ipr=NULL;
	IpfixPacketProcessor *ipp=NULL;
	IpfixParser *ipParser=NULL;

	char *l, *token=NULL;
	uri **export_uri=v->conc_uris;

        msg(MSG_DEBUG, "Config: now configuring the concentrator subsystem");


	if(!(poll_ms=iniparser_getvalue(conf, CONF_SEC, "poll_interval"))) {
		v->conc_poll_ms=CONC_POLL_MS;
		msg(MSG_ERROR,
		    "Config: parse error for poll_intervall - adjusting to %d ms",
		    v->conc_poll_ms
		   );
	} else {
		v->conc_poll_ms=atoi(poll_ms);
	}

	if(!(source_id=iniparser_getvalue(conf, CONF_SEC, "source_id"))) {
		msg(MSG_FATAL, "Config: %s source_id is missing", CONF_SEC);
                goto out;

	}
        sID=atoi(source_id);

	/* violating the original string is not nice, so copy */
	if(!(l=strdup(iniparser_getvalue(conf, CONF_SEC, "export_to")))) {
		msg(MSG_FATAL, "Config: %s export_to problem", CONF_SEC);
                goto out;
	}

	/*
	 loop over all given URIs
	 parse URI
	 */
	DPRINTF("IpfixSender URIs: %s\n", l);
	while((token=strsep(&l, ",")) && exports < MAX_URIS) {
		uri *u;

		DPRINTF("Trying to parse %s into URI\n", token);
		u=uri_parse(token);
		DPRINTF("Parsed %s into proto: %s, IP: %s, port: %d\n", token, u->scheme, u->host, u->port);

		export_uri[exports]=u;
		exports++;
	}
        exports--;

	free(l);

        /* Initialize concentrator subsystems reversely */

        /* make IPFIX exporter/sender */
        msg(MSG_DEBUG, "Config: now making IPFIX sender");
        initializeIpfixSenders();

	if(!(ips=createIpfixSender(sID, export_uri[0]->host, export_uri[0]->port))
          ) {
                goto out;
        }

	/*
	 loop over all exports and add to sender
         uri[0] is already added above at creation time!
	 */
	while(exports > 0) {
		msg(MSG_DEBUG, "Config: adding collector %s:%d to IpfixSender",
		    export_uri[exports]->host, export_uri[exports]->port
		   );
		if(ipfixSenderAddCollector(ips,
					   export_uri[exports]->host,
					   export_uri[exports]->port
					  )
		  ) {
			msg(MSG_ERROR, "Config: error adding collector %s:%d to IpfixSender",
			    export_uri[exports]->host, export_uri[exports]->port
			   );
		}
		exports--;
	}

        startIpfixSender(ips);
        subsys_on(&(v->v_subsystems), SUBSYS_CONC_EXPORT);

        /* make IPFIX aggregator */
        msg(MSG_DEBUG, "Config: now making IPFIX aggregator");
        initializeAggregators();
	rules=iniparser_getvalue(conf, CONF_SEC, "rules");
	buffertime_min=iniparser_getvalue(conf, CONF_SEC, "buffertime_min");
        buffertime_max=iniparser_getvalue(conf, CONF_SEC, "buffertime_max");

	if(!rules || !buffertime_min || !buffertime_max) {
		msg(MSG_FATAL, "Config: %s rules, buffertime_min/max are mandatory", CONF_SEC);
                goto out1;
	}

	if(!(ipa=createAggregator(
				  rules,
				  atoi(buffertime_min),
				  atoi(buffertime_max)
				 ))
	  ) {
		msg(MSG_FATAL, "Config: aggregator creation failure");
		goto out1;
        }

	addAggregatorCallbacks(ipa, getIpfixSenderCallbackInfo(ips));
        subsys_on(&(v->v_subsystems), SUBSYS_CONC_ACCOUNT);

        /* make IPFIX receiver/collector */
        initializeIpfixCollectors();
	if(!(listen=iniparser_getvalue(conf, CONF_SEC, "listen"))) {
		msg(MSG_FATAL, "Config: %s listen missing", CONF_SEC);
                goto out2;
	}
        /*
         check if we do want the receiving side; this makes sense if onlye
         the sampler part is used as input stream
         */
        if(strcasecmp("off", listen) != 0) {
                listen_portn=atoi(listen);

                if(!(ipc=createIpfixCollector())) {
                        msg(MSG_FATAL, "Config: IpfixCollector creation failure");
                        goto out2;
                }
		if (!(ipr=createIpfixReceiver(UDP_IPV4, listen_portn))) {
			msg(MSG_FATAL, "Config: IpfixReceiver creation failure for port %d", listen_portn);
			goto out3;
		}
		if (!(ipp=createIpfixPacketProcessor())) {
			msg(MSG_FATAL, "Config: IpfixPacketProcessor creation failure");
			goto out4;
		}
		if (!(ipParser=createIpfixParser())) {
			msg(MSG_FATAL, "Config: IpfixParser creation failure");
			goto out5;
		}
		
		addIpfixParserCallbacks(ipParser, getAggregatorCallbackInfo(ipa));
		setIpfixParser(ipp, ipParser);
		addIpfixReceiver(ipc, ipr);
		addIpfixPacketProcessor(ipc, ipp);
                subsys_on(&(v->v_subsystems), SUBSYS_CONC_RECEIVE);
        } else {
                msg(MSG_DEBUG, "Config: not running IpfixReceiver part of concentrator");
        }

        v->conc_collector=ipc;
        v->conc_exporter=ips;
        v->conc_aggregator=ipa;

	msg(MSG_INFO, "Config: now setting up periodic concentrator logging");
	if (v->conc_collector) {
		for (int i = 0; i != v->conc_collector->receiverCount; ++i) {
			msg_thread_add_log_function(statsIpfixReceiver, v->conc_collector->ipfixReceivers[i]);
		}
	}
	if (v->conc_aggregator) msg_thread_add_log_function(statsAggregator, v->conc_aggregator);
	if (v->conc_exporter) msg_thread_add_log_function(statsIpfixSender, v->conc_exporter);

        return 0;

out5:
	destroyIpfixPacketProcessor(ipp);
out4:
	destroyIpfixReceiver(ipr);
out3:
        destroyIpfixCollector(ipc);
out2:
        destroyAggregator(ipa);
out1:
        destroyIpfixSender(ips);
out:
        return 1;
}


/*
 this is the concentrator polling loop; this has to be done so flows expire
 and get exported etc
 */
void * concentrator_polling(void *arg)
{
        struct timespec req, rem;
        struct v_objects *v=(struct v_objects *)arg;

        IpfixCollector *ipc=v->conc_collector;
        IpfixAggregator *ipa=v->conc_aggregator;

        msg(MSG_DEBUG, "Aggregator: polling aggregator %p each %d ms", ipa, v->conc_poll_ms);
        /* break millisecond polltime into seconds and nanoseconds */
        req.tv_sec=(v->conc_poll_ms * 1000000) / 1000000000;
        req.tv_nsec=(v->conc_poll_ms * 1000000) % 1000000000;

        /*
        msg(MSG_DEBUG, "Concentrator: poll loop at %d seconds, %d nanoseconds (from %d ms)",
            req.tv_sec, req_tv_nsec, v->conc_poll_ms
           );
         */
        while(!v->conc_exitflag) {
                /* TODO: check for EINTR */
                nanosleep(&req, &rem);

                //DPRINTF("Polling aggregator %p\n", ipa);
                pollAggregator(ipa);
        }

        return (void *)1;
}
