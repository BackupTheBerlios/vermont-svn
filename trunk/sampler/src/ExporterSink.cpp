/*
 * PSAMP Reference Implementation
 *
 * ExporterSink.cpp
 *
 * Implementation of an IPFIX exporter packet sink
 * using Jan Petranek's ipfixlolib
 *
 * Author: Michael Drueing <michael@drueing.de>
 *
 */
 
#include "ExporterSink.h"
#include "Globals.h"
#include "ipfixlolib.h"

using namespace std;

void *ExporterSink::exporterSinkProcess(void *arg)
{
  ExporterSink *sink = (ExporterSink *)arg;
  ConcurrentQueue<Packet> *queue = sink->getQueue();
  Packet *p;
  int pckCount = 0;
  int deadline = 400; // timeout in msec after first packet has been added
  const int maxpackets = 10; // do not put more than this many packets in one IPFIX packet
  
  LOG("ExporterSink started\n");
  while (!sink->exitFlag)
  {
    pckCount = 1;
    
    // first we need to get a packet
    p = queue->pop();
    sink->StartNewPacketStream();
    sink->AddPacket(p);
    
    while (pckCount < maxpackets)
    {
      // TODO: add time constraint here (max. wait time)
      p = queue->pop(); 
      // if (timeout) break;
      sink->AddPacket(p);
      pckCount++;
    }
    // TODO: add packets here with time constraints
    sink->FlushPacketStream();
  }
}
