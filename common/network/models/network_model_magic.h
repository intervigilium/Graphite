#ifndef NETWORK_MODEL_MAGIC_H
#define NETWORK_MODEL_MAGIC_H

#include "network.h"
#include "tile.h"
#include "lock.h"

class NetworkModelMagic : public NetworkModel
{
   private:
      Lock _lock;

   public:
      NetworkModelMagic(Network *net, SInt32 network_id);
      ~NetworkModelMagic();

      volatile float getFrequency() { return 1.0; }

      UInt32 computeAction(const NetPacket& pkt);
      void routePacket(const NetPacket &pkt, std::vector<Hop> &nextHops);
      void processReceivedPacket(NetPacket& pkt);

      void outputSummary(std::ostream &out);

      void reset()
      {}
};

#endif /* NETWORK_MODEL_MAGIC_H */
