#include "core.h"
#include "log.h"
#include "magic_performance_model.h"
#include "branch_predictor.h"

using std::endl;

MagicPerformanceModel::MagicPerformanceModel(Core *core, float frequency)
    : CoreModel(core, frequency)
    , m_instruction_count(0)
{
}

MagicPerformanceModel::~MagicPerformanceModel()
{
}

void MagicPerformanceModel::outputSummary(std::ostream &os)
{
   os << "Core Performance Model Summary:" << endl;
   os << "    Instructions: " << getInstructionCount() << endl;
   CoreModel::outputSummary(os);
}

void MagicPerformanceModel::handleInstruction(Instruction *instruction)
{
   // compute cost
   UInt64 cost = 0;
  
   if (instruction->getAddress() != 0)
   { 
      // Instruction Cache modeling
      DynamicInstructionInfo &icache_info = getDynamicInstructionInfo();
      LOG_ASSERT_ERROR(icache_info.type == DynamicInstructionInfo::MEMORY_READ, "Info Expected(%u), Got(%u)",
                       DynamicInstructionInfo::MEMORY_READ, icache_info.type);
      // Just add the time taken to access the icache
      cost += icache_info.memory_info.latency;
      popDynamicInstructionInfo();
   }
   
   const OperandList &ops = instruction->getOperands();
   for (unsigned int i = 0; i < ops.size(); i++)
   {
      const Operand &o = ops[i];

      if (o.m_type == Operand::MEMORY)
      {
         DynamicInstructionInfo &info = getDynamicInstructionInfo();

         if (o.m_direction == Operand::READ)
         {
            LOG_ASSERT_ERROR(info.type == DynamicInstructionInfo::MEMORY_READ,
                             "Expected memory read info, got: %d.", info.type);

            cost += info.memory_info.latency;
            // ignore address
         }
         else
         {
            LOG_ASSERT_ERROR(info.type == DynamicInstructionInfo::MEMORY_WRITE,
                             "Expected memory write info, got: %d.", info.type);

            cost += info.memory_info.latency;
            // ignore address
         }

         popDynamicInstructionInfo();
      }
   }

   UInt64 instruction_cost = instruction->getCost();
   if (isModeled(instruction->getType()))
      cost += instruction_cost;
   else
      cost += 1;

   // update counters
   m_instruction_count++;
   m_cycle_count += cost;
}

bool MagicPerformanceModel::isModeled(InstructionType instruction_type)
{
   switch(instruction_type)
   {
      case INST_RECV:
      case INST_SYNC:
      case INST_SPAWN:
         return true;

      default:
         return false;
   }
}

void
MagicPerformanceModel::reset()
{
   CoreModel::reset();
   m_instruction_count = 0;
}
