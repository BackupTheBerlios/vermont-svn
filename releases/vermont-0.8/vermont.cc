/*
 this is vermont.
 released under GPL v2

 (C) by Ronny T. Lampert

 */
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <vector>

/* own systems */
#include "iniparser.h"
#include "msg.h"
#include "subsystems.h"
#include "vermont.h"
#include "config_sampler.h"
#include "config_concentrator.h"

using namespace std;

static void usage();
static void sig_handler(int x);
static int setup_signal(int signal, void (*handler)(int));
static int vermont_readconf(dictionary **conf, char *file);
static int vermont_configure(struct v_objects *v);
static int vermont_start_all(struct v_objects *v);
static int configure_logging(struct v_objects *v);
static int using_log_thread;


int main(int ac, char **dc)
{
	dictionary *config;
	int c, debug_level=MSG_DEFAULT;
	char *config_file=NULL;

	struct v_objects v_objects={0};

        /* parse command line */
	while((c=getopt(ac, dc, "hf:d")) != -1) {

                switch(c) {

                case 'f':
                        config_file=optarg;
                        break;

                case 'd':
                        debug_level++;
                        break;

                case 'h':
                default:
                        /* print usage and quit vermont, if unknow switch */
                        usage();
                        exit(1);
                }
        }

        /* setup verboseness */
        msg_setlevel(debug_level);

        setup_signal(SIGINT, sig_handler);

        /*
         read the main configuration file

         DO NOT-NOT-NEVER-NOT destroy this subsystem while vermont is running
         as all pointers used for configuration are pointing into internal structs

         DO NOT-NOT-NEVER-NOT free any pointers retrieved from the config via
         iniparser_get*() functions
         */
        if(vermont_readconf(&config, config_file)) {
                exit(-1);
        }
        /* attach the configuration struct into the main vermont objects */
        v_objects.v_config=config;
        subsys_on(&(v_objects.v_subsystems), SUBSYS_CONFIG);

        /*
         main configuration entry: all subsystems are here CONFIGURED
         STARTING them is done later
         */
        if(vermont_configure(&v_objects)) {
                exit(-1);
        }
        subsys_dump(v_objects.v_subsystems);

        vermont_start_all(&v_objects);

        /* record startup time */
        v_objects.v_starttime=time(NULL);
        msg(MSG_DIALOG, "up and running at %s", ctime(&(v_objects.v_starttime)));

        /*
         C H E E R I O W !
         WE ARE UP ! HOOORAY
        */

        /*
         main vermont now runs the periodic polling method for aggregator, if we have one
         if not, simply block on pause() until signalled
         */
        if(v_objects.conc_aggregator) {
                concentrator_polling(&v_objects);
        } else {
                pause();
        }

        return 0;
}


/*
 read the config from *file and attach the iniparser stuff to **conf
 perform basic checks

 this does not return the pointer and NULL in case of error, because it
 does also some kind of checking on the config and maybe one wants to report
 specific errors depending on what went wrong, so we stick to int.

 NOTE: the funny thing is, all returned strings are direct pointer into
 the dictionary-struct and as such valid until the dictionary is
 free()'d again.

 SO WE USUALLY DONT COPY!
 */
static int vermont_readconf(dictionary **conf, char *file)
{
        dictionary *d;

        /* read configuration */
        if(!(d=iniparser_new(file))) {
                if(file) {
                        msg(MSG_FATAL, "could not open config_file %s", file);
                } else {
                        msg(MSG_FATAL, "no config file given, but mandatory");
                        usage();
                }
                return(-1);
        }

        /* light check if all section we need are present */
        if((iniparser_find_entry(d, "concentrator") == 1) &&
           (iniparser_find_entry(d, "sampler") == 1) &&
           (iniparser_find_entry(d, "main") == 1)
          ) {
                *conf=d;
                return 0;

        } else {
                msg(MSG_FATAL, "Config: not all needed sections in config %s", file);
                return -1;
        }
}


/*
 this is the main configuration entry point
 configure all subsystems peu a peu
 */
static int vermont_configure(struct v_objects *v)
{
	dictionary *conf=v->v_config;
	int ret;

	/* if the sampler is not needed, interface will say "off" */
	char *run_sampler=iniparser_getvalue(conf, "sampler", "interface");
	char *run_concentrator=iniparser_getvalue(conf, "concentrator", "run");
	char *hooking=iniparser_getvalue(conf, "main", "packets");

	/* configure the msg subsystem */
        ret=configure_logging(v);
        if(ret==2) {
		/* OK, but not running */
	} else if(ret==0) {
                /* OK and running */
		subsys_on(&(v->v_subsystems), SUBSYS_LOGGING);
	} else {
		msg(MSG_FATAL, "Main: Configuring the logging subsystem failed");
		return -1;
	}

        /*
	 safety check for the hook
	 for the hook, BOTH subsystems have to be on
	 */
	if(strcasecmp(hooking, "off") != 0) {

		if((strcasecmp(run_sampler, "off") == 0) ||
		   (strcasecmp(run_concentrator, "off") == 0)
		  ) {
			msg(MSG_FATAL,
			    "Main: Hooking is used (%s), but either concentrator(%s) or sampler(%s) is off",
			    hooking,
			    run_concentrator,
			    run_sampler
			   );
			msg(MSG_FATAL, "Both must be enabled");
			return -1;
		}
	}

	/* check if we run the sampler */
	if(strcasecmp(run_sampler, "off") == 0) {
		msg(MSG_DIALOG, "not running sampler subsystem");
	} else {
		if(configure_sampler(v)) {
			msg(MSG_FATAL, "Main: Could not configure the sampler");
			return -1;
		}
	}

	/* check if we run the concentrator */
	if(strcasecmp(run_concentrator, "off") == 0) {
		msg(MSG_DIALOG, "not running concentrator subsystem");
	} else {

		if(configure_concentrator(v)) {
			msg(MSG_FATAL, "Main: Could not configure the concentrator");
			return -1;
		}
	}

	/* after we have a concentrator, we may fill the ctx for the HookingFilter */
	if(v->hooking) {
		msg(MSG_DEBUG, "Main: adding context pointer %p (aggregator) to HookingFilter", v->conc_aggregator);
		v->hooking->setContext(v->conc_aggregator);
	}

	return 0;
}


/*
 open log file and configure the msg_stat subsystem
 0 = all ok and running
 2 = OK, but not running
 1 or -1 = not OK, not running
 */
static int configure_logging(struct v_objects *v)
{
        FILE *FD;
        dictionary *conf=v->v_config;
        char *file;
	char *log_interval;

        file=iniparser_getvalue(conf, "main", "log");
        if(strcasecmp(file, "off") == 0) {
                msg(MSG_DEBUG, "Main: logging subsystem is off");
                return 2;
        }

        msg(MSG_DEBUG, "Main: now configuring the logging subsystem");
        if(!(FD=fopen(file, "a"))) {
                msg(MSG_FATAL, "Main: could not init message subsystem, opening log %s failed", file);
                return -1;
        }

        /*
         setting line buffering
         per default a file-stream is full-buffered, so the output won't show up
         too soon
         */
        setvbuf(FD, (char *)NULL, _IOLBF, 0);

        msg(MSG_INFO, "Logging: using %s as statistics log", file);

	/*
	 set up the logger thread
	 */
	log_interval = iniparser_getvalue(conf, "main", "log_interval");
	if((!log_interval) || (atoi(log_interval) == 0)) {
		msg(MSG_DEBUG, "Main: logging thread disabled");
	} else {
		/* set up logging thread */
		msg(MSG_DEBUG, "Main: logging all %d milliseconds", atoi(log_interval));
		msg_thread_set_timeout(atoi(log_interval));

		using_log_thread=1;
	}

	return(msg_stat_setup(MSG_SETUP_NEW, FD));
}


/* starts all configured subsystems */
static int vermont_start_all(struct v_objects *v)
{
        if(v->conc_aggregator) {
                /*
                 not nice; sender has to be started right before _creating_
		 the aggregator
                 so you won't start it here, but in configure_concentrator()
		 */
                //startIpfixSender(v->conc_exporter);
                startAggregator(v->conc_aggregator);
        }

        if(v->conc_receiver) {
                startIpfixReceiver(v->conc_receiver);
        }

        if(v->observer) {
                v->sink->runSink();
                v->filter->startFilter();
                v->observer->startCapture();
        }

	if(using_log_thread) {
		msg_thread_start();
	}

        /* DIRTY: to stabilize and wait for threads */
        sleep(1);

        return 0;
}



/* bla bla bla */
static void usage()
{
        printf(
               "VERsatile MONitoring Tool - VERMONT\n" \
               " mandatory:\n" \
               "    -f <inifile>     load config\n" \
               " optional:\n" \
               "    -d               increase debug level (specify multiple for even more)\n" \
              );
}


static int setup_signal(int signal, void (*handler)(int))
{
        struct sigaction sig;

        sig.sa_handler=sig_handler;
        sig.sa_flags=SA_RESTART;
        sigemptyset(&sig.sa_mask);
        return(sigaction(signal, &sig, NULL));
}


/* just shallow right now */
static void sig_handler(int x)
{
        msg(MSG_DIALOG, "got signal %d - exiting", x);
        exit(2);
}


static void __cplusplus_really_sucks_andPeopleUsingVeryStrangeNamingConventionsWithLongLongExplicitBlaBlaAndfUnNycasE()
{
	/*
	 The Paris crew at MOME Interop 2005

	 Christoph "C'est bon" "Das koennte beruehmt sein" Sommer
	 Lothar "Pragmatic" "Ai verdammt, das funktioniert nicht" Braun
	 Ronny T. "Agent Provocateur" "Le (SVN-)depot, c'est moi" Lampert


	 Thanks to

	 Falko "Ich hab da mal so ne Idee" Dressler
	 Gerhard "Man muesste mal" Muenz

	 who made it possible to fly to Paris.
	 */
};
