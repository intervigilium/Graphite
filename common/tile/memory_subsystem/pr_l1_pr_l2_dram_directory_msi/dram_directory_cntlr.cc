using namespace std;

#include "dram_directory_cntlr.h"
#include "log.h"
#include "memory_manager.h"

namespace PrL1PrL2DramDirectoryMSI
{

DramDirectoryCntlr::DramDirectoryCntlr(MemoryManager* memory_manager,
      DramCntlr* dram_cntlr,
      UInt32 dram_directory_total_entries,
      UInt32 dram_directory_associativity,
      UInt32 cache_block_size,
      UInt32 dram_directory_max_num_sharers,
      UInt32 dram_directory_max_hw_sharers,
      string dram_directory_type_str,
      UInt64 dram_directory_cache_access_delay_in_ns,
      UInt32 num_dram_cntlrs,
      ShmemPerfModel* shmem_perf_model):
   m_memory_manager(memory_manager),
   m_dram_cntlr(dram_cntlr),
   m_cache_block_size(cache_block_size),
   m_shmem_perf_model(shmem_perf_model)
{
   m_dram_directory_cache = new DirectoryCache(m_memory_manager->getTile(),
                                               dram_directory_type_str,
                                               dram_directory_total_entries,
                                               dram_directory_associativity,
                                               cache_block_size,
                                               dram_directory_max_hw_sharers,
                                               dram_directory_max_num_sharers,
                                               num_dram_cntlrs,
                                               dram_directory_cache_access_delay_in_ns,
                                               m_shmem_perf_model);

   LOG_PRINT("Instantiated Dram Directory Cache");

   m_dram_directory_req_queue_list = new ReqQueueList();
}

DramDirectoryCntlr::~DramDirectoryCntlr()
{
   delete m_dram_directory_cache;
   delete m_dram_directory_req_queue_list;
}

void
DramDirectoryCntlr::handleMsgFromL2Cache(tile_id_t sender, ShmemMsg* shmem_msg)
{
   ShmemMsg::msg_t shmem_msg_type = shmem_msg->getMsgType();
   UInt64 msg_time = getShmemPerfModel()->getCycleCount();

   switch (shmem_msg_type)
   {
      case ShmemMsg::EX_REQ:
      case ShmemMsg::SH_REQ:

         {
            IntPtr address = shmem_msg->getAddress();
            
            // Add request onto a queue
            ShmemReq* shmem_req = new ShmemReq(shmem_msg, msg_time);
            m_dram_directory_req_queue_list->enqueue(address, shmem_req);
            if (m_dram_directory_req_queue_list->size(address) == 1)
            {
               if (shmem_msg_type == ShmemMsg::EX_REQ)
                  processExReqFromL2Cache(shmem_req);
               else if (shmem_msg_type == ShmemMsg::SH_REQ)
                  processShReqFromL2Cache(shmem_req);
               else
                  LOG_PRINT_ERROR("Unrecognized Request(%u)", shmem_msg_type);
            }
         }
         break;

      case ShmemMsg::INV_REP:
         processInvRepFromL2Cache(sender, shmem_msg);
         break;

      case ShmemMsg::FLUSH_REP:
         processFlushRepFromL2Cache(sender, shmem_msg);
         break;

      case ShmemMsg::WB_REP:
         processWbRepFromL2Cache(sender, shmem_msg);
         break;

      default:
         LOG_PRINT_ERROR("Unrecognized Shmem Msg Type: %u", shmem_msg_type);
         break;
   }
}

void
DramDirectoryCntlr::processNextReqFromL2Cache(IntPtr address)
{
   LOG_PRINT("Start processNextReqFromL2Cache(0x%x)", address);

   assert(m_dram_directory_req_queue_list->size(address) >= 1);
   ShmemReq* completed_shmem_req = m_dram_directory_req_queue_list->dequeue(address);
   delete completed_shmem_req;

   if (! m_dram_directory_req_queue_list->empty(address))
   {
      LOG_PRINT("A new shmem req for address(0x%x) found", address);
      ShmemReq* shmem_req = m_dram_directory_req_queue_list->front(address);

      // Update the Shared Mem Cycle Counts appropriately
      getShmemPerfModel()->setCycleCount(shmem_req->getTime());

      if (shmem_req->getShmemMsg()->getMsgType() == ShmemMsg::EX_REQ)
         processExReqFromL2Cache(shmem_req);
      else if (shmem_req->getShmemMsg()->getMsgType() == ShmemMsg::SH_REQ)
         processShReqFromL2Cache(shmem_req);
      else
         LOG_PRINT_ERROR("Unrecognized Request(%u)", shmem_req->getShmemMsg()->getMsgType());
   }
   LOG_PRINT("End processNextReqFromL2Cache(0x%x)", address);
}

DirectoryEntry*
DramDirectoryCntlr::processDirectoryEntryAllocationReq(ShmemReq* shmem_req)
{
   IntPtr address = shmem_req->getShmemMsg()->getAddress();
   tile_id_t requester = shmem_req->getShmemMsg()->getRequester();
   UInt64 msg_time = getShmemPerfModel()->getCycleCount();

   std::vector<DirectoryEntry*> replacement_candidate_list;
   m_dram_directory_cache->getReplacementCandidates(address, replacement_candidate_list);

   std::vector<DirectoryEntry*>::iterator it;
   std::vector<DirectoryEntry*>::iterator replacement_candidate = replacement_candidate_list.end();
   for (it = replacement_candidate_list.begin(); it != replacement_candidate_list.end(); it++)
   {
      if ( ( (replacement_candidate == replacement_candidate_list.end()) ||
             ((*replacement_candidate)->getNumSharers() > (*it)->getNumSharers()) 
           )
           &&
           (m_dram_directory_req_queue_list->size((*it)->getAddress()) == 0)
         )
      {
         replacement_candidate = it;
      }
   }

   LOG_ASSERT_ERROR(replacement_candidate != replacement_candidate_list.end(),
         "Cant find a directory entry to be replaced with a non-zero request list");

   IntPtr replaced_address = (*replacement_candidate)->getAddress();

   // We get the entry with the lowest number of sharers
   DirectoryEntry* directory_entry = m_dram_directory_cache->replaceDirectoryEntry(replaced_address, address);

   ShmemMsg nullify_msg(ShmemMsg::NULLIFY_REQ, MemComponent::DRAM_DIR, MemComponent::DRAM_DIR, requester, replaced_address, NULL, 0);

   ShmemReq* nullify_req = new ShmemReq(&nullify_msg, msg_time);
   m_dram_directory_req_queue_list->enqueue(replaced_address, nullify_req);

   assert(m_dram_directory_req_queue_list->size(replaced_address) == 1);
   processNullifyReq(nullify_req);

   return directory_entry;
}

void
DramDirectoryCntlr::processNullifyReq(ShmemReq* shmem_req)
{
   IntPtr address = shmem_req->getShmemMsg()->getAddress();
   tile_id_t requester = shmem_req->getShmemMsg()->getRequester();
   
   DirectoryEntry* directory_entry = m_dram_directory_cache->getDirectoryEntry(address);
   assert(directory_entry);

   DirectoryBlockInfo* directory_block_info = directory_entry->getDirectoryBlockInfo();
   DirectoryState::dstate_t curr_dstate = directory_block_info->getDState();

   switch (curr_dstate)
   {
      case DirectoryState::MODIFIED:
         getMemoryManager()->sendMsg(ShmemMsg::FLUSH_REQ, 
               MemComponent::DRAM_DIR, MemComponent::L2_CACHE, 
               requester /* requester */, 
               directory_entry->getOwner() /* receiver */, 
               address);
         break;
   
      case DirectoryState::SHARED:

         {
            pair<bool, vector<SInt32> > sharers_list_pair = directory_entry->getSharersList();
            if (sharers_list_pair.first == true)
            {
               // Broadcast Invalidation Request to all tiles 
               // (irrespective of whether they are sharers or not)
               getMemoryManager()->broadcastMsg(ShmemMsg::INV_REQ, 
                     MemComponent::DRAM_DIR, MemComponent::L2_CACHE, 
                     requester /* requester */, 
                     address);
            }
            else
            {
               // Send Invalidation Request to only a specific set of sharers
               for (UInt32 i = 0; i < sharers_list_pair.second.size(); i++)
               {
                  getMemoryManager()->sendMsg(ShmemMsg::INV_REQ, 
                        MemComponent::DRAM_DIR, MemComponent::L2_CACHE, 
                        requester /* requester */, 
                        sharers_list_pair.second[i] /* receiver */, 
                        address);
               }
            }
         }
         break;
   
      case DirectoryState::UNCACHED:

         {
            m_dram_directory_cache->invalidateDirectoryEntry(address);
      
            // Process Next Request
            processNextReqFromL2Cache(address);
         }
         break;
   
      default:
         LOG_PRINT_ERROR("Unsupported Directory State: %u", curr_dstate);
         break;
   }

}

void
DramDirectoryCntlr::processExReqFromL2Cache(ShmemReq* shmem_req, Byte* cached_data_buf)
{
   IntPtr address = shmem_req->getShmemMsg()->getAddress();
   tile_id_t requester = shmem_req->getShmemMsg()->getRequester();
   
   DirectoryEntry* directory_entry = m_dram_directory_cache->getDirectoryEntry(address);
   if (directory_entry == NULL)
   {
      directory_entry = processDirectoryEntryAllocationReq(shmem_req);
   }

   DirectoryBlockInfo* directory_block_info = directory_entry->getDirectoryBlockInfo();
   DirectoryState::dstate_t curr_dstate = directory_block_info->getDState();

   switch (curr_dstate)
   {
      case DirectoryState::MODIFIED:
         assert(cached_data_buf == NULL);
         getMemoryManager()->sendMsg(ShmemMsg::FLUSH_REQ, 
               MemComponent::DRAM_DIR, MemComponent::L2_CACHE, 
               requester /* requester */, 
               directory_entry->getOwner() /* receiver */, 
               address);
         break;
   
      case DirectoryState::SHARED:

         {
            assert(cached_data_buf == NULL);
            pair<bool, vector<SInt32> > sharers_list_pair = directory_entry->getSharersList();
            if (sharers_list_pair.first == true)
            {
               // Broadcast Invalidation Request to all tiles 
               // (irrespective of whether they are sharers or not)
               getMemoryManager()->broadcastMsg(ShmemMsg::INV_REQ, 
                     MemComponent::DRAM_DIR, MemComponent::L2_CACHE, 
                     requester /* requester */, 
                     address);
            }
            else
            {
               // Send Invalidation Request to only a specific set of sharers
               for (UInt32 i = 0; i < sharers_list_pair.second.size(); i++)
               {
                  getMemoryManager()->sendMsg(ShmemMsg::INV_REQ, 
                        MemComponent::DRAM_DIR, MemComponent::L2_CACHE, 
                        requester /* requester */, 
                        sharers_list_pair.second[i] /* receiver */, 
                        address);
               }
            }
         }
         break;
   
      case DirectoryState::UNCACHED:

         {
            // Modifiy the directory entry contents
            bool add_result = directory_entry->addSharer(requester);
            assert(add_result == true);
            directory_entry->setOwner(requester);
            directory_block_info->setDState(DirectoryState::MODIFIED);

            retrieveDataAndSendToL2Cache(ShmemMsg::EX_REP, requester, address, cached_data_buf);

            // Process Next Request
            processNextReqFromL2Cache(address);
         }
         break;
   
      default:
         LOG_PRINT_ERROR("Unsupported Directory State: %u", curr_dstate);
         break;
   }
}

void
DramDirectoryCntlr::processShReqFromL2Cache(ShmemReq* shmem_req, Byte* cached_data_buf)
{
   IntPtr address = shmem_req->getShmemMsg()->getAddress();
   tile_id_t requester = shmem_req->getShmemMsg()->getRequester();

   DirectoryEntry* directory_entry = m_dram_directory_cache->getDirectoryEntry(address);
   if (directory_entry == NULL)
   {
      directory_entry = processDirectoryEntryAllocationReq(shmem_req);
   }

   DirectoryBlockInfo* directory_block_info = directory_entry->getDirectoryBlockInfo();
   DirectoryState::dstate_t curr_dstate = directory_block_info->getDState();

   switch (curr_dstate)
   {
      case DirectoryState::MODIFIED:
         {
            assert(cached_data_buf == NULL);
            getMemoryManager()->sendMsg(ShmemMsg::WB_REQ, 
                  MemComponent::DRAM_DIR, MemComponent::L2_CACHE, 
                  requester /* requester */, 
                  directory_entry->getOwner() /* receiver */, 
                  address);
         }
         break;
   
      case DirectoryState::SHARED:
         {
            bool add_result = directory_entry->addSharer(requester);
            if (add_result == false)
            {
               tile_id_t sharer_id = directory_entry->getOneSharer();
               // Send a message to another sharer to invalidate that
               getMemoryManager()->sendMsg(ShmemMsg::INV_REQ, 
                     MemComponent::DRAM_DIR, MemComponent::L2_CACHE, 
                     requester /* requester */, 
                     sharer_id /* receiver */, 
                     address);
            }
            else
            {
               retrieveDataAndSendToL2Cache(ShmemMsg::SH_REP, requester, address, cached_data_buf);
      
               // Process Next Request
               processNextReqFromL2Cache(address);
            }
         }
         break;

      case DirectoryState::UNCACHED:
         {
            // Modifiy the directory entry contents
            bool add_result = directory_entry->addSharer(requester);
            assert(add_result == true);
            directory_block_info->setDState(DirectoryState::SHARED);

            retrieveDataAndSendToL2Cache(ShmemMsg::SH_REP, requester, address, cached_data_buf);
      
            // Process Next Request
            processNextReqFromL2Cache(address);
         }
         break;
   
      default:
         LOG_PRINT_ERROR("Unsupported Directory State: %u", curr_dstate);
         break;
   }
}

void
DramDirectoryCntlr::retrieveDataAndSendToL2Cache(ShmemMsg::msg_t reply_msg_type,
      tile_id_t receiver, IntPtr address, Byte* cached_data_buf)
{
   if (cached_data_buf != NULL)
   {
      // I already have the data I need cached
      getMemoryManager()->sendMsg(reply_msg_type, 
            MemComponent::DRAM_DIR, MemComponent::L2_CACHE, 
            receiver /* requester */, 
            receiver /* receiver */, 
            address, 
            cached_data_buf, getCacheBlockSize());
   }
   else
   {
      // Get the data from DRAM
      // This could be directly forwarded to the cache or passed
      // through the Dram Directory Controller

      // I have to get the data from DRAM
      Byte data_buf[getCacheBlockSize()];
      
      m_dram_cntlr->getDataFromDram(address, receiver, data_buf);
      getMemoryManager()->sendMsg(reply_msg_type, 
            MemComponent::DRAM_DIR, MemComponent::L2_CACHE, 
            receiver /* requester */,
            receiver /* receiver */,
            address, 
            data_buf, getCacheBlockSize());
   }
}

void
DramDirectoryCntlr::processInvRepFromL2Cache(tile_id_t sender, ShmemMsg* shmem_msg)
{
   IntPtr address = shmem_msg->getAddress();

   DirectoryEntry* directory_entry = m_dram_directory_cache->getDirectoryEntry(address);
   assert(directory_entry);

   DirectoryBlockInfo* directory_block_info = directory_entry->getDirectoryBlockInfo();
   assert(directory_block_info->getDState() == DirectoryState::SHARED);

   directory_entry->removeSharer(sender);
   if (directory_entry->getNumSharers() == 0)
   {
      directory_block_info->setDState(DirectoryState::UNCACHED);
   }

   if (m_dram_directory_req_queue_list->size(address) > 0)
   {
      ShmemReq* shmem_req = m_dram_directory_req_queue_list->front(address);

      // Update Times in the Shmem Perf Model and the Shmem Req
      shmem_req->updateTime(getShmemPerfModel()->getCycleCount());
      getShmemPerfModel()->updateCycleCount(shmem_req->getTime());

      if (shmem_req->getShmemMsg()->getMsgType() == ShmemMsg::EX_REQ)
      {
         // An ShmemMsg::EX_REQ caused the invalidation
         if (directory_block_info->getDState() == DirectoryState::UNCACHED)
         {
            processExReqFromL2Cache(shmem_req);
         }
      }
      else if (shmem_req->getShmemMsg()->getMsgType() == ShmemMsg::SH_REQ)
      {
         // A ShmemMsg::SH_REQ caused the invalidation
         processShReqFromL2Cache(shmem_req);
      }
      else // shmem_req->getShmemMsg()->getMsgType() == ShmemMsg::NULLIFY_REQ
      {
         if (directory_block_info->getDState() == DirectoryState::UNCACHED)
         {
            processNullifyReq(shmem_req);
         }
      }
   }
}

void
DramDirectoryCntlr::processFlushRepFromL2Cache(tile_id_t sender, ShmemMsg* shmem_msg)
{
   IntPtr address = shmem_msg->getAddress();

   DirectoryEntry* directory_entry = m_dram_directory_cache->getDirectoryEntry(address);
   assert(directory_entry);

   DirectoryBlockInfo* directory_block_info = directory_entry->getDirectoryBlockInfo();
   assert(directory_block_info->getDState() == DirectoryState::MODIFIED);

   directory_entry->removeSharer(sender);
   directory_entry->setOwner(INVALID_TILE_ID);
   directory_block_info->setDState(DirectoryState::UNCACHED);

   if (m_dram_directory_req_queue_list->size(address) != 0)
   {
      ShmemReq* shmem_req = m_dram_directory_req_queue_list->front(address);

      // Update times
      shmem_req->updateTime(getShmemPerfModel()->getCycleCount());
      getShmemPerfModel()->updateCycleCount(shmem_req->getTime());

      // An involuntary/voluntary Flush
      if (shmem_req->getShmemMsg()->getMsgType() == ShmemMsg::EX_REQ)
      {
         processExReqFromL2Cache(shmem_req, shmem_msg->getDataBuf());
      }
      else if (shmem_req->getShmemMsg()->getMsgType() == ShmemMsg::SH_REQ)
      {
         // Write Data to Dram
         sendDataToDram(address, shmem_msg->getRequester(), shmem_msg->getDataBuf());
         processShReqFromL2Cache(shmem_req, shmem_msg->getDataBuf());
      }
      else // shmem_req->getShmemMsg()->getMsgType() == ShmemMsg::NULLIFY_REQ
      {
         // Write Data To Dram
         sendDataToDram(address, shmem_msg->getRequester(), shmem_msg->getDataBuf());
         processNullifyReq(shmem_req);
      }
   }
   else
   {
      // This was just an eviction
      // Write Data to Dram
      sendDataToDram(address, shmem_msg->getRequester(), shmem_msg->getDataBuf());
   }
}

void
DramDirectoryCntlr::processWbRepFromL2Cache(tile_id_t sender, ShmemMsg* shmem_msg)
{
   IntPtr address = shmem_msg->getAddress();

   DirectoryEntry* directory_entry = m_dram_directory_cache->getDirectoryEntry(address);
   assert(directory_entry);
   
   DirectoryBlockInfo* directory_block_info = directory_entry->getDirectoryBlockInfo();

   assert(directory_block_info->getDState() == DirectoryState::MODIFIED);
   assert(directory_entry->hasSharer(sender));
   
   directory_entry->setOwner(INVALID_TILE_ID);
   directory_block_info->setDState(DirectoryState::SHARED);

   if (m_dram_directory_req_queue_list->size(address) != 0)
   {
      ShmemReq* shmem_req = m_dram_directory_req_queue_list->front(address);

      // Update Time
      shmem_req->updateTime(getShmemPerfModel()->getCycleCount());
      getShmemPerfModel()->updateCycleCount(shmem_req->getTime());

      // Write Data to Dram
      sendDataToDram(address, shmem_msg->getRequester(), shmem_msg->getDataBuf());

      LOG_ASSERT_ERROR(shmem_req->getShmemMsg()->getMsgType() == ShmemMsg::SH_REQ,
            "Address(0x%x), Req(%u)",
            address, shmem_req->getShmemMsg()->getMsgType());
      processShReqFromL2Cache(shmem_req, shmem_msg->getDataBuf());
   }
   else
   {
      LOG_PRINT_ERROR("Should not reach here");
   }
}

void
DramDirectoryCntlr::sendDataToDram(IntPtr address, tile_id_t requester, Byte* data_buf)
{
   // Write data to Dram
   m_dram_cntlr->putDataToDram(address, requester, data_buf);
}

}
