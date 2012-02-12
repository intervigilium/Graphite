#include "address_home_lookup.h"
#include "log.h"

AddressHomeLookup::AddressHomeLookup(UInt32 ahl_param,
      vector<tile_id_t>& tile_list,
      UInt32 cache_block_size):
   m_ahl_param(ahl_param),
   m_tile_list(tile_list),
   m_cache_block_size(cache_block_size)
{

   // Each Block Address is as follows:
   // /////////////////////////////////////////////////////////// //
   //   block_num               |   block_offset                  //
   // /////////////////////////////////////////////////////////// //

   LOG_ASSERT_ERROR((1 << m_ahl_param) >= (SInt32) m_cache_block_size,
         "AHL param(%u) must be >= Cache Block Size(%u)",
         m_ahl_param, m_cache_block_size);
   m_total_modules = tile_list.size();
}

AddressHomeLookup::~AddressHomeLookup()
{
   // There is no memory to deallocate, so destructor has no function
}

tile_id_t AddressHomeLookup::getHome(IntPtr address) const
{
   SInt32 module_num = (address >> m_ahl_param) % m_total_modules;
   LOG_ASSERT_ERROR(0 <= module_num && module_num < (SInt32) m_total_modules, "module_num(%i), total_modules(%u)", module_num, m_total_modules);
   
   LOG_PRINT("address(0x%x), module_num(%i)", address, module_num);
   return (m_tile_list[module_num]);
}
