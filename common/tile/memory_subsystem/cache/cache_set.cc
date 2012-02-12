#include "cache_set.h"
#include "cache_base.h"
#include "log.h"

CacheSet::CacheSet(CacheBase::cache_t cache_type,
      UInt32 associativity, UInt32 blocksize):
      m_associativity(associativity), m_blocksize(blocksize)
{
   m_cache_block_info_array = new CacheBlockInfo*[m_associativity];
   for (UInt32 i = 0; i < m_associativity; i++)
   {
      m_cache_block_info_array[i] = CacheBlockInfo::create(cache_type);
   }
   m_blocks = new char[m_associativity * m_blocksize];
   
   memset(m_blocks, 0x00, m_associativity * m_blocksize);
}

CacheSet::~CacheSet()
{
   for (UInt32 i = 0; i < m_associativity; i++)
      delete m_cache_block_info_array[i];
   delete [] m_cache_block_info_array;
   delete [] m_blocks;
}

void 
CacheSet::read_line(UInt32 line_index, UInt32 offset, Byte *out_buff, UInt32 bytes)
{
   assert(offset + bytes <= m_blocksize);
   assert((out_buff == NULL) == (bytes == 0));

   if (out_buff != NULL)
      memcpy((void*) out_buff, &m_blocks[line_index * m_blocksize + offset], bytes);

   updateReplacementIndex(line_index);
}

void 
CacheSet::write_line(UInt32 line_index, UInt32 offset, Byte *in_buff, UInt32 bytes)
{
   assert(offset + bytes <= m_blocksize);
   assert((in_buff == NULL) == (bytes == 0));

   if (in_buff != NULL)
      memcpy(&m_blocks[line_index * m_blocksize + offset], (void*) in_buff, bytes);

   updateReplacementIndex(line_index);
}

CacheBlockInfo* 
CacheSet::find(IntPtr tag, UInt32* line_index)
{
   for (SInt32 index = m_associativity-1; index >= 0; index--)
   {
      if (m_cache_block_info_array[index]->getTag() == tag)
      {
         if (line_index != NULL)
            *line_index = index;
         return (m_cache_block_info_array[index]);
      }
   }
   return NULL;
}

bool 
CacheSet::invalidate(IntPtr& tag)
{
   for (SInt32 index = m_associativity-1; index >= 0; index--)
   {
      if (m_cache_block_info_array[index]->getTag() == tag)
      {
         m_cache_block_info_array[index]->invalidate();
         return true;
      }
   }
   return false;
}

void 
CacheSet::insert(CacheBlockInfo* cache_block_info, Byte* fill_buff, bool* eviction, CacheBlockInfo* evict_block_info, Byte* evict_buff)
{
   // This replacement strategy does not take into account the fact that
   // cache blocks can be voluntarily flushed or invalidated due to another write request
   const UInt32 index = getReplacementIndex();
   assert(index < m_associativity);

   assert(eviction != NULL);
         
   if (m_cache_block_info_array[index]->isValid())
   {
      *eviction = true;
      // FIXME: This is a hack. I dont know if this is the best way to do
      evict_block_info->clone(m_cache_block_info_array[index]);
      if (evict_buff != NULL)
         memcpy((void*) evict_buff, &m_blocks[index * m_blocksize], m_blocksize);
   }
   else
   {
      *eviction = false;
   }

   // FIXME: This is a hack. I dont know if this is the best way to do
   m_cache_block_info_array[index]->clone(cache_block_info);
   
   if (fill_buff != NULL)
      memcpy(&m_blocks[index * m_blocksize], (void*) fill_buff, m_blocksize);
}

CacheSet* 
CacheSet::createCacheSet (std::string replacement_policy,
      CacheBase::cache_t cache_type,
      UInt32 associativity, UInt32 blocksize)
{
   CacheBase::ReplacementPolicy policy = parsePolicyType(replacement_policy);
   switch(policy)
   {
      case CacheBase::ROUND_ROBIN:
         return new CacheSetRoundRobin(cache_type, associativity, blocksize);

      case CacheBase::LRU:
         return new CacheSetLRU(cache_type, associativity, blocksize);

      default:
         LOG_PRINT_ERROR("Unrecognized Cache Replacement Policy: %i",
               policy);
         break;
   }

   return (CacheSet*) NULL;
}

CacheBase::ReplacementPolicy 
CacheSet::parsePolicyType(string policy)
{
   if (policy == "round_robin")
      return CacheBase::ROUND_ROBIN;
   if (policy == "lru")
      return CacheBase::LRU;
   else
      return (CacheBase::ReplacementPolicy) -1;
}
