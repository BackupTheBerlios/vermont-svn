/*
 * PSAMP Reference Implementation
 *
 * Filter.h
 *
 * Main filtering loop. Accepts a packet and applies filters on it
 *
 * Author: Michael Drueing <michael@drueing.de>
 *
 */
 
#ifndef FILTER_H
#define FILTER_H

#include <vector>
#include "Globals.h"
#include "ConcurrentQueue.h"
#include "Packet.h"
#include "Thread.h"

#include "PacketReceiver.h"
#include "PacketProcessor.h"
#include "SystematicSampler.h"
#include "RandomSampler.h"

class Filter : public PacketReceiver
{
public:
  Filter() : thread(Filter::filterProcess), exitFlag(false),pktIn(0), pktOut(0)
  {
  }
   
  ~Filter()
  {
  }
  
  void startFilter()
  {
    Filter::instance = this;
    thread.run(0);
  }
  
  void terminate()
  {
    exitFlag = true;
  }
  
  // adds a new output queue to the receivers
  void setReceiver(PacketReceiver *recv)
  {
    receiver = recv->getQueue();
  };
  
  // add a new filter or sampler
  void addProcessor(PacketProcessor *p)
  {
    processors.push_back(p);
  };

protected:
  Thread thread;
  static void *filterProcess(void *);
  std::vector<PacketProcessor *> processors;
  //std::vector<ConcurrentQueue<Packet> *> receivers;
  ConcurrentQueue<Packet> *receiver;
  
public:
  bool exitFlag;
  
  //debug variables
  unsigned long pktIn;
  unsigned long pktOut;
  
  static Filter *Filter::instance;
};

#endif
