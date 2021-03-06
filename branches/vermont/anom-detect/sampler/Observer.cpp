/*
 PSAMP Reference Implementation
 Observer.cpp
 Implementation of the packet capturing thread
 Author: Michael Drueing <michael@drueing.de>

 changed by: Ronny T. Lampert
             Gerhard Münz
 */


#include "Observer.h"

#include "common/msg.h"
#include "common/Thread.h"
#include "common/defs.h"

#include <pcap.h>
#include <unistd.h>
#include <iostream>
#include <sstream>


using namespace std;


InstanceManager<Packet> Observer::packetManager("Packet"); 

Observer::Observer(const std::string& interface) : thread(Observer::observerThread), allDevices(NULL),
	captureDevice(NULL), capturelen(PCAP_DEFAULT_CAPTURE_LENGTH), pcap_timeout(PCAP_TIMEOUT), 
	pcap_promisc(1), ready(false), filter_exp(0),
	receivedBytes(0), lastReceivedBytes(0), processedPackets(0), 
	lastProcessedPackets(0)

{
	captureInterface = (char*)malloc(interface.size() + 1);
	strcpy(captureInterface, interface.c_str());
	
	usedBytes += sizeof(Observer)+interface.size()+1;
	
	if(capturelen > PCAP_MAX_CAPTURE_LENGTH) {
		THROWEXCEPTION("compile-time parameter PCAP_DEFAULT_CAPTURE_LENGTH (%d) exceeds maximum capture length %d, " 
				"adjust compile-time parameter PCAP_MAX_CAPTURE_LENGTH!", capturelen, PCAP_DEFAULT_CAPTURE_LENGTH);
		
	}
};

Observer::~Observer()
{
	msg(MSG_DEBUG, "Observer: destructor called");

	shutdown(false);

	/* collect and output statistics */
	pcap_stat pstats;
	if (captureDevice && pcap_stats(captureDevice, &pstats)==0) {
		msg(MSG_DIALOG, "PCAP statistics (INFO: if statistics were activated, this information does not contain correct data!):");
		msg(MSG_DIALOG, "Number of packets received on interface: %u", pstats.ps_recv);
		msg(MSG_DIALOG, "Number of packets dropped by PCAP: %u", pstats.ps_drop);
	}

	msg(MSG_DEBUG, "freeing pcap/devices");
	if(captureDevice) {
		pcap_close(captureDevice);
	}

	/* no pcap_freecode here, is already done after attaching the filter */

	if(allDevices) {
		pcap_freealldevs(allDevices);
	}

	free(captureInterface);
	delete[] filter_exp;
	msg(MSG_DEBUG, "successful shutdown");
}
/*
 This is the main observer loop. It graps packets from libpcap and
 dispatches them to the registered receivers.
 */
void *Observer::observerThread(void *arg)
{
	/* first we need to get the instance back from the void *arg */
	Observer *obs=(Observer *)arg;
	InstanceManager<Packet>& packetManager = obs->packetManager;
	
	Packet *p = NULL;
	const unsigned char *pcapData;
	struct pcap_pkthdr packetHeader;
	bool have_send = false;
	obs->registerCurrentThread();
	
	if (!obs->isConnected()) {
		THROWEXCEPTION("Observer does not have any receiving modules to send packets to");
	}

	// start capturing packets
	msg(MSG_INFO, "now running capturing thread for device %s", obs->captureInterface);


	while(!obs->exitFlag) {
		// wait until data can be read from pcap file descriptor
		fd_set fd_wait;
		FD_ZERO(&fd_wait);
		FD_SET(pcap_fileno(obs->captureDevice), &fd_wait);
		struct timeval st;
		st.tv_sec = 1;
		st.tv_usec = 0;
		int result = select(FD_SETSIZE, &fd_wait, NULL, NULL, &st);
		if (result == -1) {
			msg(MSG_FATAL, "select() on pcap file descriptor returned -1");
			break;
		}
		if (result == 0) {
			continue;
		}

		/*
		 get next packet (no zero-copy possible *sigh*)
		 NOTICE: potential bottleneck, if pcap_next() is calling gettimeofday() at a high rate;
		 there is partially caching function described in an Sun or IBM (Developerworks) article
		 that can act as a via LD_PRELOAD used overlay function.
		 unfortunately I don't have an URL ready -Freek
		 */
		DPRINTFL(MSG_VDEBUG, "trying to get packet from pcap");
		pcapData=pcap_next(obs->captureDevice, &packetHeader);
		if(!pcapData)
			/* no packet data was available */
			continue;
		DPRINTFL(MSG_VDEBUG, "got new packet!");

		// show current packet as c-structure on stdout
		//for (unsigned int i=0; i<packetHeader.caplen; i++) {
			//printf("0x%02hhX, ", ((unsigned char*)pcapData)[i]);
		//}
		//printf("\n");

		// initialize packet structure (init copies packet data)
		p = packetManager.getNewInstance();
		p->init((char*)pcapData, packetHeader.caplen, packetHeader.ts);

		obs->receivedBytes += packetHeader.caplen;

		DPRINTF("received packet at %u.%04u, len=%d",
			(unsigned)p->timestamp.tv_sec,
			(unsigned)p->timestamp.tv_usec / 1000,
			packetHeader.caplen
			);

		// update statistics
		obs->receivedBytes += ntohs(*(uint16_t*)(p->netHeader+2));
		obs->processedPackets++;

		while (!obs->exitFlag) {
			DPRINTFL(MSG_VDEBUG, "trying to push packet to queue");
			if (have_send = obs->send(p)) {
				DPRINTFL(MSG_VDEBUG, "packet pushed");
				break;
			}
		}
	}

	// if we aren't connected to a destination, we grab the packet and and can't
	// send it to a destination. This would leak the packet, so clean it up... 
	if (!have_send && p != NULL)
		p->removeReference();
	
	msg(MSG_DEBUG, "exiting observer thread");
	obs->unregisterCurrentThread();
	pthread_exit((void *)1);
}


/*
 call after an Observer has been created
 error checking on pcap here, because it can't be done in the constructor
 and it may be too late, if done in the thread
 */
bool Observer::prepare(const std::string& filter)
{
	int dataLink = 0;

	// we need to store the filter expression, because pcap needs
	// a char* and doesn't accept a const char* ... nasty pcap-devs!!!
	if (!filter.empty()) {
		filter_exp = new char[filter.size() + 1];
		strcpy(filter_exp, filter.c_str());
		usedBytes += filter.size()+1;
	}
	struct in_addr i_netmask, i_network;

	// query all available capture devices
	msg(MSG_INFO, "Finding devices");
	if(pcap_findalldevs(&allDevices, errorBuffer) == -1) {
		msg(MSG_FATAL, "error getting list of interfaces: %s", errorBuffer);
		goto out;
	}

	for(pcap_if_t *dev = allDevices; dev != NULL; dev=dev->next) {
		msg(MSG_DEBUG, "PCAP: name=%s, desc=%s", dev->name, dev->description);
	}

	msg(MSG_INFO,
	    "pcap opening interface=%s, promisc=%d, snaplen=%d, timeout=%d",
	    captureInterface, pcap_promisc, capturelen, pcap_timeout
	   );
	captureDevice=pcap_open_live(captureInterface, capturelen, pcap_promisc, pcap_timeout, errorBuffer);
	// check for errors
	if(!captureDevice) {
		msg(MSG_FATAL, "Error initializing pcap interface: %s", errorBuffer);
		goto out1;
	}

	// make reads non-blocking
	if(pcap_setnonblock(captureDevice, 1, errorBuffer) == -1) {
		msg(MSG_FATAL, "Error setting pcap interface to non-blocking: %s", errorBuffer);
		goto out2;
	}

	dataLink = pcap_datalink(captureDevice);
	// IP_HEADER_OFFSET is set by the configure script
	switch (dataLink) {
	case DLT_EN10MB:
		if (IP_HEADER_OFFSET != 14 && IP_HEADER_OFFSET != 18) {
			msg(MSG_FATAL, "IP_HEADER_OFFSET on an ethernet device has to be 14 or 18 Bytes. Please adjust that value via configure --with-ipheader-offset");
			goto out2;
		}
		break;
	case DLT_LOOP:
	case DLT_NULL:
		if (IP_HEADER_OFFSET != 4) {
			msg(MSG_FATAL, "IP_HEADER_OFFSET on BSD loop back device has to be 4 Bytes. Please adjust that value via configure --with-ipheader-offset");
			goto out2;
		}
		break;
	default:
		msg(MSG_ERROR, "You are using an unkown IP_HEADER_OFFSET and data link combination. This can make problems. Please check if you use the correct IP_HEADER_OFFSET for your data link, if you see strange IPFIX/PSAMP packets.");
	}


	/* we need the netmask for the pcap_compile */
	if(pcap_lookupnet(captureInterface, &network, &netmask, errorBuffer) == -1) {
		msg(MSG_ERROR, "unable to determine netmask/network: %s", errorBuffer);
		network=0;
		netmask=0;
	}
	i_network.s_addr=network;
	i_netmask.s_addr=netmask;
	msg(MSG_DEBUG, "pcap seems to run on network %s", inet_ntoa(i_network));
	msg(MSG_INFO, "pcap seems to run on netmask %s", inet_ntoa(i_netmask));

	if(filter_exp) {
		msg(MSG_DEBUG, "compiling pcap filter code from: %s", filter_exp);
		if(pcap_compile(captureDevice, &pcap_filter, filter_exp, 1, netmask) == -1) {
			msg(MSG_FATAL, "unable to validate+compile pcap filter");
			goto out2;
		}

		if(pcap_setfilter(captureDevice, &pcap_filter) == -1) {
			msg(MSG_FATAL, "unable to attach filter to pcap: %s", pcap_geterr(captureDevice));
			goto out3;
		}
		/* you may free an attached code, see man-page */
		pcap_freecode(&pcap_filter);
	} else {
		msg(MSG_DEBUG, "using no pcap filter");
	}

	ready=true;

	return true;

out3:
	pcap_freecode(&pcap_filter);
out2:
	pcap_close(captureDevice);
	captureDevice=NULL;
out1:
	pcap_freealldevs(allDevices);
	allDevices=NULL;
out:
	return false;
}


/*
 this function is called by the logger timer thread and should dump
 some nice info using msg_stat
 */
void Observer::doLogging(void *arg)
{
	Observer *obs=(Observer *)arg;
	struct pcap_stat stats;

	 //pcap_stats() will set the stats to -1 if something goes wrong
	 //so it is okay if we dont check the return code
	obs->getPcapStats(&stats);
	msg_stat("%6d recv, %6d drop, %6d ifdrop", stats.ps_recv, stats.ps_drop, stats.ps_ifdrop);
}

/*
   call to get the main capture thread running
   open() has to be called before
*/
void Observer::performStart()
{
	if(!ready)
		THROWEXCEPTION("Can't start capturing, observer is not ready");

	msg(MSG_DEBUG, "now starting capturing thread");
	thread.run(this);
};

void Observer::performShutdown()
{
	/* be sure the thread is ending */
	msg(MSG_DEBUG, "joining the ObserverThread, may take a while (until next pcap data is received)");
	connected.shutdown();
	thread.join();
	msg(MSG_DEBUG, "ObserverThread joined");
}


/* you cannot change the caplen of an already running observer */
bool Observer::setCaptureLen(int x)
{
	/* we cant change pcap caplen if alredy pcap_open() called */
	if(ready) {
		THROWEXCEPTION("changing capture len on-the-fly is not supported by pcap");
	}
	if (x>PCAP_MAX_CAPTURE_LENGTH) {
		THROWEXCEPTION("maximum capture length is limited by constant PCAP_MAX_CAPTURE_LENGTH (%d), "
				"given value %d is too big", PCAP_MAX_CAPTURE_LENGTH, x);
	}
	capturelen=x;
	return true;
}

int Observer::getCaptureLen()
{
	return capturelen;
}


bool Observer::setPacketTimeout(int ms)
{
	if(ready) {
		msg(MSG_ERROR, "changing read timeout on-the-fly is not supported by pcap");
		return false;
	}
	pcap_timeout=ms;
	return true;
}


int Observer::getPacketTimeout()
{
	return pcap_timeout;
}

/*
   get some capturing statistics
   struct pcap_stat is defined in pcap.h and has at least 3 u_int variables:
   ps_recv, ps_drop, ps_ifdrop

   should return: -1 on failure, 0 on OK
   */
int Observer::getPcapStats(struct pcap_stat *out)
{
	return(pcap_stats(captureDevice, out));
}

/**
 * statistics function called by StatisticsManager
 */
std::string Observer::getStatisticsXML()
{
	ostringstream oss;
    pcap_stat pstats;
    if (captureDevice && pcap_stats(captureDevice, &pstats)==0) {
    	oss << "<pcap>";
    	oss << "<received type=\"packets\">" << pstats.ps_recv << "</received>";
    	oss << "<dropped type=\"packets\">" << pstats.ps_drop << "</dropped>";
    	oss << "</pcap>";
	}
	uint64_t diff = receivedBytes-lastReceivedBytes;
	lastReceivedBytes += diff;
	oss << "<observer>";
	oss << "<processed type=\"bytes\">" << diff << "</processed>";
	diff = processedPackets-lastProcessedPackets;
	lastProcessedPackets += diff;
	oss << "<processed type=\"packets\">" << diff << "</processed>";
	oss << "</observer>";
	return oss.str();
}


//static void plain_c_sucks_because_people_dont_seem_to_have_a_caps_key_and_like_to_type_stupid_underscores_alot(){}
