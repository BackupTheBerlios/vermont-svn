#include "rcvIpfix.h"
#include "common.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>

typedef enum { UNSPEC, UDP, TCP } socket_type;

#define DEFAULT_LISTEN_PORT 1501
#define DEFAULT_IMPORT_TYPE UDP

#define DEFAULT_EXPORT_PORT 1501
#define DEFAULT_EXPORT_TYPE TCP

#define DEFAULT_FROM "127.0.0.1"
#define DEFAULT_TO   "127.0.0.1"


int export_socket;

void sig_int() {}

int export_packet(IpfixParser* ipfixParser, byte* message, uint16_t len) {
	debug("Forwarding packet");
	write(export_socket, message, len);
	return 0;
}

void usage(char* progname) {
	fprintf(stderr, "%s: [ <from proto> [ <to proto> [ <from port> [ <to port> [ <from server> [ <to server> ] ] ] ] ] ]", progname);
}

int main(int argc, char** argv) {
	int lport = DEFAULT_LISTEN_PORT;
	int eport = DEFAULT_EXPORT_PORT;

	socket_type import_type = DEFAULT_IMPORT_TYPE;
	socket_type export_type = DEFAULT_EXPORT_TYPE;

	char* from_host = DEFAULT_FROM;
	char* to_host   = DEFAULT_TO;


	struct sockaddr_in servaddr;

	if (argc > 1) {
		if (!strcmp("udp", argv[1])) {
			import_type = UDP;
		} else if (!strcmp("tcp", argv[1])) {
			import_type = TCP;
		}
	}
	if (argc > 2) {
		if (!strcmp("udp", argv[2])) {
			export_type = UDP;
		} else if (!strcmp("tcp", argv[2])) {
			export_type = TCP;
		}
	}

	if (argc > 3) 
		lport = atoi(argv[3]);
	if (argc > 4) 
		eport = atoi(argv[4]);
	if (argc > 5)
		from_host = argv[5];
	if (argc > 6)
		to_host = argv[6];

	signal(SIGINT, sig_int);

	/* set up new exporter */
	switch (export_type) {
	case UDP:
		debug("Exporting UDP stream");
		if (-1 == (export_socket = socket(AF_INET, SOCK_DGRAM, 0))) {
			error("Could not create socket");
			exit(1);
		}
		break;
	case TCP:
		debug("Exporting TCP stream");
		if (-1 == (export_socket = socket(AF_INET, SOCK_STREAM, 0))) {
			error("Could not create socket");
			exit(1);
		}
		break;
	default:
		error("Unknown protocol");
		exit(1);
	}

	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	if (!inet_aton(from_host, &servaddr.sin_addr)) {
		error("Error translating source host");
		exit(1);
	}
	servaddr.sin_port = htons(eport);

	if (-1 == connect(export_socket, (struct sockaddr*)&servaddr,
			  sizeof(servaddr))) {
		error("Could not connect to server");
		exit(1);
	}

	/* set up receiver */
	initializeIpfixCollectors();
	IpfixCollector* ipfixCollector;
	
	switch (import_type) {
	case TCP:
		debug("Importing TCP stream");
		ipfixCollector = createIpfixCollector(TCP_IPV4, lport);
		break;
	case UDP:
		debug("Importing UDP stream");
		ipfixCollector = createIpfixCollector(UDP_IPV4, lport);
		break;
	default:
		error("Unkown import protocol");
		exit(1);
	}

	PacketProcessor* packetProcessor = createPacketProcessor();
	packetProcessor->processPacketCallbackFunction = export_packet;
	
	addPacketProcessor(ipfixCollector, packetProcessor);
	
	startIpfixCollector(ipfixCollector);

	debugf("Listening on %s:%i, exporting to %s:%i", import_type==TCP?"TCP":"UDP", lport, export_type==TCP?"TCP":"UDP", eport);
	pause();
	debug("Cleaning up");

	stopIpfixCollector(ipfixCollector);

	destroyIpfixCollector(ipfixCollector);
	deinitializeIpfixCollectors();

	close(export_socket);

	return 0;
}
