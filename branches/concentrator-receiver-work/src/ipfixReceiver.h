#ifndef _IPFIX_RECEIVER_H_
#define _IPFIX_RECEIVER_H_

#include <pthread.h>
#include <stdint.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAX_MSG_LEN	65536

/**
 * Defines type of Receiver
 */
typedef enum {
	UDP_IPV4, UDP_IPV6, TCP_IPV4, TCP_IPV6, SCTP_IPV4, SCTP_IPV6
} Receiver_Type;


/**
 * Represents an IpfixReceiver
 */
typedef struct {
	int listen_socket;

	int* connected_sockets;
	int connection_count;

	pthread_mutex_t mutex; /**< Mutex to pause receiving thread */
	pthread_t thread;      /**< Thread ID for this particular instance, to sync against etc */

	int authCount;
	struct in_addr* authHosts;

	void* packetProcessor;
	int processorCount;
	
	Receiver_Type receiver_type;
} IpfixReceiver;


int initializeIpfixReceivers();
int deinitializeIpfixReceivers();

IpfixReceiver* createIpfixReceiver(Receiver_Type receiver_type, int port);
void destroyIpfixReceiver(IpfixReceiver* ipfixReceiver);

int startIpfixReceiver(IpfixReceiver* ipfixReceiver);
int stopIpfixReceiver(IpfixReceiver* ipfixReceiver);

int addAuthorizedHost(IpfixReceiver* ipfixReceiver, const char*);
int isHostAuthorized(IpfixReceiver* ipfixReceiver, struct in_addr* inaddr, int addrlen);
int setPacketProcessor(IpfixReceiver* ipfixReceiver, void* packetProcessor, int processorCount);



#endif
