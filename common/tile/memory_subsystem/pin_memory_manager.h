#ifndef __PIN_MEMORY_MANAGER_H__
#define __PIN_MEMORY_MANAGER_H__

#include "core.h"

class PinMemoryManager
{
   public:
      enum AccessType
      {
         ACCESS_TYPE_READ = 0,
         ACCESS_TYPE_READ2,
         ACCESS_TYPE_WRITE,
         NUM_ACCESS_TYPES
      };

   private:
      Core *m_core;
      //Core *m_core;

      // scratchpads are used to implement memory redirection for
      // all memory accesses that do not involve the stack, plus
      // pushf and popf
      static const unsigned int m_scratchpad_size = 4 * 1024;
      char *m_scratchpad [NUM_ACCESS_TYPES];
      
      // Used to redirect pushf and popf
      carbon_reg_t m_saved_esp;

   public:
      PinMemoryManager(Core* core);
      ~PinMemoryManager();

      // Functions for redirecting general memory accesses
      carbon_reg_t redirectMemOp (bool has_lock_prefix, IntPtr tgt_ea, IntPtr size, AccessType access_type);
      void completeMemWrite (bool has_lock_prefix, IntPtr tgt_ea, IntPtr size, AccessType access_type);

      // Functions for redirecting pushf
      carbon_reg_t redirectPushf ( IntPtr tgt_esp, IntPtr size );
      carbon_reg_t completePushf (IntPtr esp, IntPtr size);
      
      // Functions for redirecting popf
      carbon_reg_t redirectPopf (IntPtr tgt_esp, IntPtr size);
      carbon_reg_t completePopf (IntPtr esp, IntPtr size);

};

#endif /* __PIN_MEMORY_MANAGER_H__ */
