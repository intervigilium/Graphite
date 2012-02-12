#ifndef __DRAM_PERF_MODEL_H__
#define __DRAM_PERF_MODEL_H__

#include "queue_model.h"
#include "fixed_types.h"
#include "moving_average.h"

// Note: Each Dram Controller owns a single DramModel object
// Hence, m_dram_bandwidth is the bandwidth for a single DRAM controller
// Total Bandwidth = m_dram_bandwidth * Number of DRAM controllers
// Number of DRAM controllers presently = Number of Cores
// m_dram_bandwidth is expressed in GB/s
// Assuming the frequency of a core is 1GHz, 
// m_dram_bandwidth is also expressed in 'Bytes per clock cycle'
// This DRAM model is not entirely correct.
// It sort of increases the queueing delay to a huge value if
// the arrival times of adjacent packets are spread over a large
// simulated time period
class DramPerfModel
{
   private:
      // Dram Model Parameters
      UInt64 m_dram_access_cost;
      volatile float m_dram_bandwidth;

      UInt32 m_cache_block_size;

      // Queue Model
      QueueModel* m_queue_model;
      std::string m_queue_model_type;
      bool m_queue_model_enabled;
      
      bool m_enabled;

      // Performance Counters
      UInt64 m_num_accesses;
      volatile double m_total_access_latency;
      volatile double m_total_queueing_delay;

      void createQueueModels();
      void destroyQueueModels();
      void resetQueueModels();
      void initializePerformanceCounters();

   public:
      DramPerfModel(float dram_access_cost, 
            float dram_bandwidth,
            bool queue_model_enabled,
            std::string queue_model_type, 
            UInt32 cache_block_size);

      ~DramPerfModel();

      UInt64 getAccessLatency(UInt64 pkt_time, UInt64 pkt_size, tile_id_t requester);
      void enable();
      void disable();
      void reset();

      UInt64 getTotalAccesses() { return m_num_accesses; }
      void outputSummary(ostream& out);

      static void dummyOutputSummary(ostream& out);
};

#endif /* __DRAM_PERF_MODEL_H__ */
