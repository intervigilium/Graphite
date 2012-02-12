#ifndef __CACHE_BASE_H__
#define __CACHE_BASE_H__

#include <string>

using namespace std;

#include "fixed_types.h"

#define k_KILO 1024
#define k_MEGA (k_KILO*k_KILO)
#define k_GIGA (k_KILO*k_MEGA)

// Generic cache base class; 
// no allocate specialization, no cache set specialization
class CacheBase
{
   public:
      // types, constants
      enum access_t
      {
         INVALID_ACCESS_TYPE,
         MIN_ACCESS_TYPE,
         LOAD = MIN_ACCESS_TYPE,
         STORE,
         MAX_ACCESS_TYPE = STORE,
         NUM_ACCESS_TYPES = MAX_ACCESS_TYPE - MIN_ACCESS_TYPE + 1
      };

      enum cache_t
      {
         INVALID_CACHE_TYPE,
         MIN_CACHE_TYPE,
         PR_L1_CACHE = MIN_CACHE_TYPE,
         PR_L2_CACHE,
         MAX_CACHE_TYPE = PR_L2_CACHE,
         NUM_CACHE_TYPES = MAX_CACHE_TYPE - MIN_CACHE_TYPE + 1
      };

      enum ReplacementPolicy
      {
         ROUND_ROBIN = 0,
         LRU,
         NUM_REPLACEMENT_POLICIES
      };

   protected:
      // input params
      std::string m_name;
      UInt32 m_cache_size;
      UInt32 m_associativity;
      UInt32 m_blocksize;
      UInt32 m_num_sets;

      // computed params
      UInt32 m_log_blocksize;

   public:
      // constructors/destructors
      CacheBase(std::string name, UInt32 cache_size, UInt32 associativity, UInt32 cache_block_size);
      virtual ~CacheBase();

      // utilities
      void splitAddress(const IntPtr addr, IntPtr& tag, UInt32& set_index) const;
      void splitAddress(const IntPtr addr, IntPtr& tag, UInt32& set_index, UInt32& block_offset) const;
      IntPtr tagToAddress(const IntPtr tag);
      
      // Output Summary
      virtual void outputSummary(ostream& out) {}
};

#endif /* __CACHE_BASE_H__ */
