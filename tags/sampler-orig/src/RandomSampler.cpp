/*
 * PSAMP Reference Implementation
 *
 * RandomSampler.cpp
 *
 * Random n-out-of-N sampling of packets
 *
 * Author: Michael Drueing <michael@drueing.de>
 *
 */
 
#include <cstdlib>
#include <ctime>

#include "RandomSampler.h"

RandomSampler::RandomSampler(int n, int N) : samplingSize(N), acceptSize(n), currentPos(0)
{
  int pos;
  
  if (n > N)
  {
    fprintf(stderr, "RandomSampler: %d-out-of%d makes no sense!\n", n, N);
    exit(3);
  }
  
  sampleMask.clear();
  sampleMask.insert(sampleMask.begin(), N, false);
  
  srand(time(0));
  
  // setup sampling bitfield
  // TODO: There might be a more elegant solution to this...
  for (int i = 0; i < acceptSize; i++)
  {
    // find a free spot in the sampleMask (i.e. a position
    // with FALSE in it) and set it to TRUE
    do
    {
      pos = rand() % samplingSize;
    } while (sampleMask[pos]);
    sampleMask[pos] = true;
  }
};

bool RandomSampler::processPacket(const Packet *p)
{
  bool accepted = sampleMask[currentPos];
  
  currentPos = (currentPos + 1) % samplingSize;
  
  return accepted;
}
