#include "memory_manager.h"
#include "cache_base.h"
#include "simulator.h"
#include "clock_converter.h"
#include "log.h"

namespace PrL1PrL2DramDirectoryMSI
{

MemoryManager::MemoryManager(Tile* tile, 
      Network* network, ShmemPerfModel* shmem_perf_model):
   MemoryManagerBase(tile, network, shmem_perf_model),
   m_dram_directory_cntlr(NULL),
   m_dram_cntlr(NULL),
   m_dram_cntlr_present(false),
   m_enabled(false)
{
   // Read Parameters from the Config file
   std::string l1_icache_type;
   UInt32 l1_icache_size = 0;
   UInt32 l1_icache_associativity = 0;
   std::string l1_icache_replacement_policy;
   UInt32 l1_icache_data_access_time = 0;
   UInt32 l1_icache_tags_access_time = 0;
   std::string l1_icache_perf_model_type;

   std::string l1_dcache_type;
   UInt32 l1_dcache_size = 0;
   UInt32 l1_dcache_associativity = 0;
   std::string l1_dcache_replacement_policy;
   UInt32 l1_dcache_data_access_time = 0;
   UInt32 l1_dcache_tags_access_time = 0;
   std::string l1_dcache_perf_model_type;

   std::string l2_cache_type;
   UInt32 l2_cache_size = 0;
   UInt32 l2_cache_associativity = 0;
   std::string l2_cache_replacement_policy;
   UInt32 l2_cache_data_access_time = 0;
   UInt32 l2_cache_tags_access_time = 0;
   std::string l2_cache_perf_model_type;

   UInt32 dram_directory_total_entries = 0;
   UInt32 dram_directory_associativity = 0;
   UInt32 dram_directory_max_num_sharers = 0;
   UInt32 dram_directory_max_hw_sharers = 0;
   std::string dram_directory_type_str;
   UInt32 dram_directory_home_lookup_param = 0;
   UInt32 dram_directory_cache_access_time = 0;

   volatile float dram_latency = 0.0;
   volatile float per_dram_controller_bandwidth = 0.0;
   bool dram_queue_model_enabled = false;
   std::string dram_queue_model_type;

   std::string directory_type;

   try
   {
      // L1 ICache
      l1_icache_type = "perf_model/l1_icache/" + Config::getSingleton()->getL1ICacheType(getTile()->getId());
      m_cache_block_size = Sim()->getCfg()->getInt(l1_icache_type + "/cache_block_size");
      l1_icache_size = Sim()->getCfg()->getInt(l1_icache_type + "/cache_size");
      l1_icache_associativity = Sim()->getCfg()->getInt(l1_icache_type + "/associativity");
      l1_icache_replacement_policy = Sim()->getCfg()->getString(l1_icache_type + "/replacement_policy");
      l1_icache_data_access_time = Sim()->getCfg()->getInt(l1_icache_type + "/data_access_time");
      l1_icache_tags_access_time = Sim()->getCfg()->getInt(l1_icache_type + "/tags_access_time");
      l1_icache_perf_model_type = Sim()->getCfg()->getString(l1_icache_type + "/perf_model_type");

      // L1 DCache
      l1_dcache_type = "perf_model/l1_dcache/" + Config::getSingleton()->getL1DCacheType(getTile()->getId());
      l1_dcache_size = Sim()->getCfg()->getInt(l1_dcache_type + "/cache_size");
      l1_dcache_associativity = Sim()->getCfg()->getInt(l1_dcache_type + "/associativity");
      l1_dcache_replacement_policy = Sim()->getCfg()->getString(l1_dcache_type + "/replacement_policy");
      l1_dcache_data_access_time = Sim()->getCfg()->getInt(l1_dcache_type + "/data_access_time");
      l1_dcache_tags_access_time = Sim()->getCfg()->getInt(l1_dcache_type + "/tags_access_time");
      l1_dcache_perf_model_type = Sim()->getCfg()->getString(l1_dcache_type + "/perf_model_type");

      // L2 Cache
      l2_cache_type = "perf_model/l2_cache/" + Config::getSingleton()->getL2CacheType(getTile()->getId());
      l2_cache_size = Sim()->getCfg()->getInt(l2_cache_type + "/cache_size");
      l2_cache_associativity = Sim()->getCfg()->getInt(l2_cache_type + "/associativity");
      l2_cache_replacement_policy = Sim()->getCfg()->getString(l2_cache_type + "/replacement_policy");
      l2_cache_data_access_time = Sim()->getCfg()->getInt(l2_cache_type + "/data_access_time");
      l2_cache_tags_access_time = Sim()->getCfg()->getInt(l2_cache_type + "/tags_access_time");
      l2_cache_perf_model_type = Sim()->getCfg()->getString(l2_cache_type + "/perf_model_type");

      // Dram Directory Cache
      dram_directory_total_entries = Sim()->getCfg()->getInt("perf_model/dram_directory/total_entries");
      dram_directory_associativity = Sim()->getCfg()->getInt("perf_model/dram_directory/associativity");
      dram_directory_max_num_sharers = Sim()->getConfig()->getTotalTiles();
      dram_directory_max_hw_sharers = Sim()->getCfg()->getInt("perf_model/dram_directory/max_hw_sharers");
      dram_directory_type_str = Sim()->getCfg()->getString("perf_model/dram_directory/directory_type");
      dram_directory_home_lookup_param = Sim()->getCfg()->getInt("perf_model/dram_directory/home_lookup_param");
      dram_directory_cache_access_time = Sim()->getCfg()->getInt("perf_model/dram_directory/directory_cache_access_time");

      // Dram Cntlr
      dram_latency = Sim()->getCfg()->getFloat("perf_model/dram/latency");
      per_dram_controller_bandwidth = Sim()->getCfg()->getFloat("perf_model/dram/per_controller_bandwidth");
      dram_queue_model_enabled = Sim()->getCfg()->getBool("perf_model/dram/queue_model/enabled");
      dram_queue_model_type = Sim()->getCfg()->getString("perf_model/dram/queue_model/type");

      // Directory Type
      directory_type = Sim()->getCfg()->getString("perf_model/dram_directory/directory_type");
   }
   catch(...)
   {
      LOG_PRINT_ERROR("Error reading memory system parameters from the config file");
   }

   if (getTile()->getId() == 0)
   {
      LOG_ASSERT_ERROR(directory_type != "limited_broadcast", \
            "Limited Broadcast directory scheme CANNOT be used with the MSI protocol.");
   }

   volatile float core_frequency = Config::getSingleton()->getCoreFrequency(getTile()->getMainCoreId());
   
   m_user_thread_sem = new Semaphore(0);
   m_network_thread_sem = new Semaphore(0);
   
   std::vector<tile_id_t> tile_list_with_dram_controllers = getTileListWithMemoryControllers();
   // if (getTile()->getId() == 0)
      // printTileListWithMemoryControllers(tile_list_with_dram_controllers);

   if (find(tile_list_with_dram_controllers.begin(), tile_list_with_dram_controllers.end(), getTile()->getId()) \
         != tile_list_with_dram_controllers.end())
   {
      m_dram_cntlr_present = true;

      m_dram_cntlr = new DramCntlr(this,
            dram_latency,
            per_dram_controller_bandwidth,
            dram_queue_model_enabled,
            dram_queue_model_type,
            getCacheBlockSize(),
            getShmemPerfModel());

      LOG_PRINT("Instantiated Dram Cntlr");

      m_dram_directory_cntlr = new DramDirectoryCntlr(this,
            m_dram_cntlr,
            dram_directory_total_entries,
            dram_directory_associativity,
            getCacheBlockSize(),
            dram_directory_max_num_sharers,
            dram_directory_max_hw_sharers,
            dram_directory_type_str,
            dram_directory_cache_access_time,
            tile_list_with_dram_controllers.size(),
            getShmemPerfModel());
      
      LOG_PRINT("Instantiated Dram Directory Cntlr");
   }

   m_dram_directory_home_lookup = new AddressHomeLookup(dram_directory_home_lookup_param, tile_list_with_dram_controllers, getCacheBlockSize());

   LOG_PRINT("Instantiated Dram Directory Home Lookup");

   m_l1_cache_cntlr = new L1CacheCntlr(getTile()->getId(),
         this,
         m_user_thread_sem,
         m_network_thread_sem,
         getCacheBlockSize(),
         l1_icache_size, l1_icache_associativity,
         l1_icache_replacement_policy,
         l1_icache_data_access_time,
         l1_dcache_size, l1_dcache_associativity,
         l1_dcache_replacement_policy,
         l1_dcache_data_access_time,
         core_frequency,
         getShmemPerfModel());
   
   LOG_PRINT("Instantiated L1 Cache Cntlr");

   m_l2_cache_cntlr = new L2CacheCntlr(getTile()->getId(),
         this,
         m_l1_cache_cntlr,
         m_dram_directory_home_lookup,
         m_user_thread_sem,
         m_network_thread_sem,
         getCacheBlockSize(),
         l2_cache_size, l2_cache_associativity,
         l2_cache_replacement_policy,
         l2_cache_data_access_time,
         core_frequency,
         getShmemPerfModel());

   LOG_PRINT("Instantiated L2 Cache Cntlr");

   m_l1_cache_cntlr->setL2CacheCntlr(m_l2_cache_cntlr);

   // Create Cache Performance Models
   m_l1_icache_perf_model = CachePerfModel::create(l1_icache_perf_model_type,
         l1_icache_data_access_time, l1_icache_tags_access_time, core_frequency);
   m_l1_dcache_perf_model = CachePerfModel::create(l1_dcache_perf_model_type,
         l1_dcache_data_access_time, l1_dcache_tags_access_time, core_frequency);
   m_l2_cache_perf_model = CachePerfModel::create(l2_cache_perf_model_type,
         l2_cache_data_access_time, l2_cache_tags_access_time, core_frequency);

   LOG_PRINT("Instantiated Cache Performance Models");

   // Register Call-backs
   getNetwork()->registerCallback(SHARED_MEM_1, MemoryManagerNetworkCallback, this);
   getNetwork()->registerCallback(SHARED_MEM_2, MemoryManagerNetworkCallback, this);
}

MemoryManager::~MemoryManager()
{
   getNetwork()->unregisterCallback(SHARED_MEM_1);
   getNetwork()->unregisterCallback(SHARED_MEM_2);

   // Delete the Models
   delete m_l1_icache_perf_model;
   delete m_l1_dcache_perf_model;
   delete m_l2_cache_perf_model;

   delete m_user_thread_sem;
   delete m_network_thread_sem;
   delete m_dram_directory_home_lookup;
   delete m_l1_cache_cntlr;
   delete m_l2_cache_cntlr;
   if (m_dram_cntlr_present)
   {
      delete m_dram_cntlr;
      delete m_dram_directory_cntlr;
   }
}

bool
MemoryManager::coreInitiateMemoryAccess(
      MemComponent::component_t mem_component,
      Core::lock_signal_t lock_signal,
      Core::mem_op_t mem_op_type,
      IntPtr address, UInt32 offset,
      Byte* data_buf, UInt32 data_length,
      bool modeled)
{
   return m_l1_cache_cntlr->processMemOpFromTile(mem_component, 
         lock_signal, 
         mem_op_type, 
         address, offset, 
         data_buf, data_length,
         modeled);
}

void
MemoryManager::handleMsgFromNetwork(NetPacket& packet)
{
   core_id_t sender = packet.sender;
   ShmemMsg* shmem_msg = ShmemMsg::getShmemMsg((Byte*) packet.data);
   UInt64 msg_time = packet.time;

   getShmemPerfModel()->setCycleCount(msg_time);

   MemComponent::component_t receiver_mem_component = shmem_msg->getReceiverMemComponent();
   MemComponent::component_t sender_mem_component = shmem_msg->getSenderMemComponent();

   if (m_enabled)
   {
      LOG_PRINT("Got Shmem Msg: type(%i), address(0x%x), sender_mem_component(%u), receiver_mem_component(%u), sender(%i,%i), receiver(%i,%i)", 
            shmem_msg->getMsgType(), shmem_msg->getAddress(), sender_mem_component, receiver_mem_component, sender.tile_id, sender.core_type, packet.receiver.tile_id, packet.receiver.core_type);    
   }

   switch (receiver_mem_component)
   {
      case MemComponent::L2_CACHE:
         switch(sender_mem_component)
         {
            case MemComponent::L1_ICACHE:
            case MemComponent::L1_DCACHE:
               assert(sender.tile_id == getTile()->getId());
               m_l2_cache_cntlr->handleMsgFromL1Cache(shmem_msg);
               break;

            case MemComponent::DRAM_DIR:
               m_l2_cache_cntlr->handleMsgFromDramDirectory(sender.tile_id, shmem_msg);
               break;

            default:
               LOG_PRINT_ERROR("Unrecognized sender component(%u)",
                     sender_mem_component);
               break;
         }
         break;

      case MemComponent::DRAM_DIR:
         switch(sender_mem_component)
         {
            LOG_ASSERT_ERROR(m_dram_cntlr_present, "Dram Cntlr NOT present");

            case MemComponent::L2_CACHE:
               m_dram_directory_cntlr->handleMsgFromL2Cache(sender.tile_id, shmem_msg);
               break;

            default:
               LOG_PRINT_ERROR("Unrecognized sender component(%u)",
                     sender_mem_component);
               break;
         }
         break;

      default:
         LOG_PRINT_ERROR("Unrecognized receiver component(%u)",
               receiver_mem_component);
         break;
   }

   // Delete the allocated Shared Memory Message
   // First delete 'data_buf' if it is present
   // LOG_PRINT("Finished handling Shmem Msg");

   if (shmem_msg->getDataLength() > 0)
   {
      assert(shmem_msg->getDataBuf());
      delete [] shmem_msg->getDataBuf();
   }
   delete shmem_msg;
}

void
MemoryManager::sendMsg(ShmemMsg::msg_t msg_type, MemComponent::component_t sender_mem_component, MemComponent::component_t receiver_mem_component, tile_id_t requester, tile_id_t receiver, IntPtr address, Byte* data_buf, UInt32 data_length)
{
   assert((data_buf == NULL) == (data_length == 0));
   ShmemMsg shmem_msg(msg_type, sender_mem_component, receiver_mem_component, requester, address, data_buf, data_length);

   Byte* msg_buf = shmem_msg.makeMsgBuf();
   UInt64 msg_time = getShmemPerfModel()->getCycleCount();

   if (m_enabled)
   {
      LOG_PRINT("Sending Msg: type(%u), address(0x%x), sender_mem_component(%u), receiver_mem_component(%u), requester(%i), sender(%i), receiver(%i)", msg_type, address, sender_mem_component, receiver_mem_component, requester, getTile()->getId(), receiver);
   }

   NetPacket packet(msg_time, SHARED_MEM_1,
         getTile()->getId(), receiver,
         shmem_msg.getMsgLen(), (const void*) msg_buf);
   getNetwork()->netSend(packet);

   // Delete the Msg Buf
   delete [] msg_buf;
}

void
MemoryManager::broadcastMsg(ShmemMsg::msg_t msg_type, MemComponent::component_t sender_mem_component, MemComponent::component_t receiver_mem_component, tile_id_t requester, IntPtr address, Byte* data_buf, UInt32 data_length)
{
   assert((data_buf == NULL) == (data_length == 0));
   ShmemMsg shmem_msg(msg_type, sender_mem_component, receiver_mem_component, requester, address, data_buf, data_length);

   Byte* msg_buf = shmem_msg.makeMsgBuf();
   UInt64 msg_time = getShmemPerfModel()->getCycleCount();

   if (m_enabled)
   {
      LOG_PRINT("Sending Msg: type(%u), address(0x%x), sender_mem_component(%u), receiver_mem_component(%u), requester(%i), sender(%i), receiver(%i)", msg_type, address, sender_mem_component, receiver_mem_component, requester, getTile()->getId(), NetPacket::BROADCAST);
   }

   NetPacket packet(msg_time, SHARED_MEM_1,
         getTile()->getId(), NetPacket::BROADCAST,
         shmem_msg.getMsgLen(), (const void*) msg_buf);
   getNetwork()->netSend(packet);

   // Delete the Msg Buf
   delete [] msg_buf;
}

void
MemoryManager::incrCycleCount(MemComponent::component_t mem_component, CachePerfModel::CacheAccess_t access_type)
{
   switch (mem_component)
   {
      case MemComponent::L1_ICACHE:
         getShmemPerfModel()->incrCycleCount(m_l1_icache_perf_model->getLatency(access_type));
         break;

      case MemComponent::L1_DCACHE:
         getShmemPerfModel()->incrCycleCount(m_l1_dcache_perf_model->getLatency(access_type));
         break;

      case MemComponent::L2_CACHE:
         getShmemPerfModel()->incrCycleCount(m_l2_cache_perf_model->getLatency(access_type));
         break;

      case MemComponent::INVALID_MEM_COMPONENT:
         break;

      default:
         LOG_PRINT_ERROR("Unrecognized mem component type(%u)", mem_component);
         break;
   }
}

void
MemoryManager::enableModels()
{
   m_enabled = true;

   m_l1_cache_cntlr->getL1ICache()->enable();
   m_l1_icache_perf_model->enable();
   
   m_l1_cache_cntlr->getL1DCache()->enable();
   m_l1_dcache_perf_model->enable();
   
   m_l2_cache_cntlr->getL2Cache()->enable();
   m_l2_cache_perf_model->enable();

   if (m_dram_cntlr_present)
   {
      m_dram_directory_cntlr->getDramDirectoryCache()->enable();
      m_dram_cntlr->getDramPerfModel()->enable();
   }
}

void
MemoryManager::disableModels()
{
   m_enabled = false;

   m_l1_cache_cntlr->getL1ICache()->disable();
   m_l1_icache_perf_model->disable();

   m_l1_cache_cntlr->getL1DCache()->disable();
   m_l1_dcache_perf_model->disable();

   m_l2_cache_cntlr->getL2Cache()->disable();
   m_l2_cache_perf_model->disable();

   if (m_dram_cntlr_present)
   {
      m_dram_directory_cntlr->getDramDirectoryCache()->disable();
      m_dram_cntlr->getDramPerfModel()->disable();
   }
}

void
MemoryManager::resetModels()
{
   m_l1_cache_cntlr->getL1ICache()->reset();
   m_l1_cache_cntlr->getL1DCache()->reset();
   m_l2_cache_cntlr->getL2Cache()->reset();

   if (m_dram_cntlr_present)
   {
      m_dram_cntlr->getDramPerfModel()->reset();
   }
}

void
MemoryManager::outputSummary(std::ostream &os)
{
   os << "Cache Summary:\n";
   m_l1_cache_cntlr->getL1ICache()->outputSummary(os);
   m_l1_cache_cntlr->getL1DCache()->outputSummary(os);
   m_l2_cache_cntlr->getL2Cache()->outputSummary(os);

   if (m_dram_cntlr_present)
   {      
      m_dram_cntlr->getDramPerfModel()->outputSummary(os);
      os << "Dram Directory Cache Summary:\n";
      m_dram_directory_cntlr->getDramDirectoryCache()->outputSummary(os);
   }
   else
   {
      DramPerfModel::dummyOutputSummary(os);
      os << "Dram Directory Cache Summary:\n";
      DirectoryCache::dummyOutputSummary(os);
   }
}

}
