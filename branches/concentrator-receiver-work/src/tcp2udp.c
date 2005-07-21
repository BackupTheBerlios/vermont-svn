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

typedef enum { UDP, TCP } socket_type;

#define DEFAULT_LISTEN_PORT 1501
#define DEFAULT_IMPORT_TYPE TCP

#define DEFAULT_EXPORT_PORT 1501
#define DEFAULT_EXPORT_TYPE UDP


int export_socket;

void sig_int() {}

int export_packet(IpfixParser* ipfixParser, byte* message, uint16_t len) {
	debug("Forwarding packet");
	write(export_socket, message, len);
	return 0;
}

void usage(char* progname) {
	fprintf(stderr, "%s: [-i tcp|udp ] [-e tcp|udp]\n", progname);
}

int main(int argc, char** argv) {
	int lport = DEFAULT_LISTEN_PORT;
	int eport = DEFAULT_EXPORT_PORT;

	socket_type import_type = DEFAULT_IMPORT_TYPE;
	socket_type export_type = DEFAULT_EXPORT_TYPE;

	int c;

	struct sockaddr_in servaddr;

	while (-1 != (c = getopt(argc, argv, "i:e:"))) {
		switch (c) {
		case 'i':
			if (strcmp("tcp", optarg) == 0){
				import_type = TCP;
				break;
			}
			if (strcmp("udp", optarg) == 0){
				import_type = UDP;
				break;
			}
			usage(argv[0]);
			exit(1);
		case 'e':
			if (strcmp("tcp", optarg) == 0) {
				export_type = TCP;
				break;
			}
			if (strcmp("udp", optarg) == 0) {
				export_type = UDP;
				break;
			}
			usage(argv[0]);
			exit(1);
		}
	}

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
	servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	servaddr.sin_port = htons(eport);

	if (-1 == connect(export_socket, (struct sockaddr*)&servaddr,
			  sizeof(servaddr))) {
		error("Could not connect to server");
		exit(1);
	}

	/* set up receiver */
	initializeIpfixCollectors();
	IpfixCollector* ipfixCollector = createIpfixCollector();
	
	switch (import_type) {
	case TCP:
		debug("Importing TCP stream");
		setReceiverType(ipfixCollector, TCP_IPV4, lport);
		break;
	case UDP:
		debug("Importing UDP stream");
		setReceiverType(ipfixCollector, UDP_IPV4, lport);
		break;
	default:
		error("Unkown import protocol");
		exit(1);
	}

	PacketProcessor* packetProcessor = createPacketProcessor();
	packetProcessor->processPacketCallbackFunction = export_packet;

	addPacketProcessor(ipfixCollector, packetProcessor);

	startIpfixCollector(ipfixCollector);

	debugf("Listening on port %i, exporting to port %i", lport, eport);
	pause();
	debug("Cleaning up");

	stopIpfixCollector(ipfixCollector);

	destroyIpfixCollector(ipfixCollector);
	deinitializeIpfixCollectors();

	close(export_socket);

	return 0;
}
