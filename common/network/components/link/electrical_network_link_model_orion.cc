#include "electrical_network_link_model_orion.h"
#include "log.h"

ElectricalNetworkLinkModelOrion::ElectricalNetworkLinkModelOrion(LinkType link_type, volatile float link_frequency, volatile double link_length, UInt32 link_width):
   _frequency(link_frequency),
   _total_dynamic_energy(0)
{
   LOG_ASSERT_ERROR(link_type == ELECTRICAL_REPEATED, \
         "Orion only supports REPEATED_ELECTRICAL link models currently");
   _orion_link = new OrionLink(link_length / 1000, link_width, OrionConfig::getSingleton());
}

ElectricalNetworkLinkModelOrion::~ElectricalNetworkLinkModelOrion()
{
   delete _orion_link;
}

UInt64
ElectricalNetworkLinkModelOrion::getDelay()
{
   // FIXME: Calculate the proper delay - Using Vladimir's models
   return 0;
}

volatile double
ElectricalNetworkLinkModelOrion::getStaticPower()
{
   return _orion_link->get_static_power();
}

void
ElectricalNetworkLinkModelOrion::updateDynamicEnergy(UInt32 num_bit_flips, UInt32 num_flits)
{
   volatile double dynamic_energy = _orion_link->calc_dynamic_energy(num_bit_flips);
   _total_dynamic_energy += (num_flits * dynamic_energy);
}
