/*
 * PSAMP Reference Implementation
 *
 * Observer.h
 *
 * Declarations for observing process
 *
 * Author: Michael Drueing <michael@drueing.de>
 *
 */

#ifndef OBSERVER_H
#define OBSERVER_H

#include <vector>
#include "Globals.h"
#include "Thread.h"
#include "ConcurrentQueue.h"
#include "Packet.h"
#include "PacketReceiver.h"
#include <pcap.h>

class Observer
{
public:
  // ObserverThread constructor
  Observer() : thread(Observer::observerThread), exitFlag(false)
  {    
    // query all available capture devices
    LOG("Observer: Finding devices\n");
    pcap_findalldevs(&allDevices, errorBuffer);
    if (!allDevices)
    {
      LOG("Observer: error getting list of interfaces. Reason: %s\n", errorBuffer);
    }
    for (pcap_if_t *dev = allDevices; dev != NULL; dev=dev->next )
    {
      LOG("  Name=%s, DESC=%s\n", dev->name, dev->description);
    }
  };

  ~Observer()
  {
    LOG("Observer: freeing devices...\n");
    
    if (captureDevice)
      pcap_close(captureDevice);
    
    pcap_freealldevs(allDevices);
  };

  void startCapture(char *ifdesc)
  {
    captureInterface = ifdesc;
    thread.run(this);
  };

  void terminateCapture()
  {
    exitFlag = true;
  };

  void addReceiver(PacketReceiver *recv)
  {
    receivers.push_back(recv->getQueue());
  };

protected:
  Thread thread;
  
  // pointer to list of pcap-devices
  pcap_if_t *allDevices;
  pcap_t *captureDevice;
  char errorBuffer[PCAP_ERRBUF_SIZE];

  static void *observerThread(void *);

public:
  bool exitFlag;
  char *captureInterface;
  std::vector<ConcurrentQueue<Packet> *> receivers;
};

#endif
