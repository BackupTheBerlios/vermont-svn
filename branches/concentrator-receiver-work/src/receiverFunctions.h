#ifndef _RECEIVER_FUNCTIONS_H_
#define _RECEIVER_FUNCTIONS_H_

#include <stdint.h>

typedef uint8_t byte;

/**
 * TODO: make *blabla*
 */
typedef int(InitializeReceivers)();
typedef int(DeinitializeReceivers)();
typedef void*(CreateReceiver)(uint16_t);
typedef void(DestroyReceiver)(void*);
typedef int(StartReceiver)(void*);
typedef int(StopReceiver)(void*);
typedef int(SetPacketProcessor)(void*, void*, int);
typedef int(HasPacketProcessor)(void*);


/**
 * TODO: make *blabla*
 */
typedef struct {
	InitializeReceivers* initializeReceivers;
	DeinitializeReceivers* deinitializeReceivers;
	CreateReceiver* createReceiver;
	DestroyReceiver* destroyReceiver;
	StartReceiver* startReceiver;
	StopReceiver* stopReceiver;
	SetPacketProcessor* setPacketProcessor;
	HasPacketProcessor* hasPacketProcessor;

        } Receiver_Functions;


#endif
