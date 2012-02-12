#ifndef CACHE_SET_H
#define CACHE_SET_H

#include <string.h>

#include "fixed_types.h"
#include "cache_block_info.h"
#include "cache_base.h"

// Everything related to cache sets
class CacheSet
{
   public:
      
      static CacheSet* createCacheSet(std::string replacement_policy, CacheBase::cache_t cache_type, UInt32 associativity, UInt32 blocksize);
      static CacheBase::ReplacementPolicy parsePolicyType(std::string policy);

   protected:
      CacheBlockInfo** m_cache_block_info_array;
      char* m_blocks;
      UInt32 m_associativity;
      UInt32 m_blocksize;

   public:

      CacheSet(CacheBase::cache_t cache_type,
            UInt32 associativity, UInt32 blocksize);
      virtual ~CacheSet();

      UInt32 getBlockSize() { return m_blocksize; }

      UInt32 getAssociativity() { return m_associativity; }

      void read_line(UInt32 line_index, UInt32 offset, Byte *out_buff, UInt32 bytes);
      void write_line(UInt32 line_index, UInt32 offset, Byte *in_buff, UInt32 bytes);
      CacheBlockInfo* find(IntPtr tag, UInt32* line_index = NULL);
      bool invalidate(IntPtr& tag);
      void insert(CacheBlockInfo* cache_block_info, Byte* fill_buff, bool* eviction, CacheBlockInfo* evict_block_info, Byte* evict_buff);

      virtual UInt32 getReplacementIndex() = 0;
      virtual void updateReplacementIndex(UInt32) = 0;
};

class CacheSetRoundRobin : public CacheSet
{
   public:
      CacheSetRoundRobin(CacheBase::cache_t cache_type, 
            UInt32 associativity, UInt32 blocksize);
      ~CacheSetRoundRobin();

      UInt32 getReplacementIndex();
      void updateReplacementIndex(UInt32 accessed_index);

   private:
      UInt32 m_replacement_index;
};

class CacheSetLRU : public CacheSet
{
   public:
      CacheSetLRU(CacheBase::cache_t cache_type,
            UInt32 associativity, UInt32 blocksize);
      ~CacheSetLRU();

      UInt32 getReplacementIndex();
      void updateReplacementIndex(UInt32 accessed_index);

   private:
      UInt8* m_lru_bits;
};


#endif /* CACHE_SET_H */
