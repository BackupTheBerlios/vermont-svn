#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include "rcvMessage.h"

/***** Constants ************************************************************/

#define MAX_MSG_LEN	65536

/***** Data Types ************************************************************/


/***** Global Variables ******************************************************/

MessageCallbackFunction* messageCallbackFunction;
pthread_mutex_t receiverMutex = PTHREAD_MUTEX_INITIALIZER;

/***** Internal Functions ****************************************************/

static void * listenerUdpIpv4(void* handleP) {
	int handle = *(int*)handleP; free(handleP);
	struct sockaddr_in clientAddress;
	socklen_t clientAddressLen;
	byte* data = (byte*)malloc(sizeof(byte)*MAX_MSG_LEN);
	int n;

	while(1) {
		clientAddressLen = sizeof(struct sockaddr_in);
		n = recvfrom(handle, data, MAX_MSG_LEN, 0, (struct sockaddr*)&clientAddress, &clientAddressLen);

		if (n < 0) {
			debug("recvfrom returned without data, terminating listener thread");
			break;
			}
      
		pthread_mutex_lock(&receiverMutex);
		if (messageCallbackFunction) messageCallbackFunction(data, n);
		pthread_mutex_unlock(&receiverMutex);
		}

	close(handle);
	free(data);

	return 0;
	}

/***** Exported Functions ****************************************************/

void initializeRcvMessage() {
	messageCallbackFunction = 0;
	pthread_mutex_lock(&receiverMutex);
	}

void deinitializeRcvMessage() {
	pthread_mutex_unlock(&receiverMutex);
	}

void startRcvMessage() {
	pthread_mutex_unlock(&receiverMutex);
	}
	
void stopRcvMessage() {
	pthread_mutex_lock(&receiverMutex);
	}

void setMessageCallback(MessageCallbackFunction* messageCallbackFunction_) {
	messageCallbackFunction = messageCallbackFunction_;
	}

int rcvMessageUdpIpv4(uint16 port) {
	struct sockaddr_in serverAddress;
	int handle;
	int i;

	handle = socket(AF_INET, SOCK_DGRAM, 0);
	if(handle<0) {
		perror("socket");
		exit(1);
		}

	serverAddress.sin_family = AF_INET;
	serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
	serverAddress.sin_port = htons(port);
	i = bind(handle, (struct sockaddr*)&serverAddress, sizeof(struct sockaddr_in));
	if(i < 0) {
		perror("bind");
		exit(1);
		}

	int* handleP = (int*)malloc(sizeof(int)); 
	*handleP = handle;

	pthread_t thread; 
	pthread_create(&thread, 0, listenerUdpIpv4, handleP);

	return handle;
	}

void rcvMessageClose(int handle) {
	debug("closing handle");
	close(handle);
	}
