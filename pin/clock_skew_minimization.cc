#include "clock_skew_minimization.h"
#include "simulator.h"
#include "tile_manager.h"
#include "tile.h"
#include "clock_skew_minimization_object.h"

static bool enabled()
{
   std::string scheme = Sim()->getCfg()->getString("clock_skew_minimization/scheme", "none");
   return (scheme != "none");
}

void handlePeriodicSync()
{
   Tile* tile = Sim()->getTileManager()->getCurrentTile();
   assert(tile);
   if (tile->getId() >= (tile_id_t) Sim()->getConfig()->getApplicationTiles())
   {
      // Thread Spawner Tile / MCP
      return;
   }

   ClockSkewMinimizationClient *client = tile->getCurrentCore()->getClockSkewMinimizationClient();

   if (client)
      client->synchronize();
}

void addPeriodicSync(INS ins)
{
   if (!enabled())
      return;

   INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(handlePeriodicSync), IARG_END);
}
