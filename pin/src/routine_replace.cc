#include "routine_replace.h"
#include "user_space_wrappers.h"

bool replaceUserAPIFunction(RTN& rtn, string& name)
{

   AFUNPTR msg_ptr = NULL;
   PROTO proto = NULL;

   if (name == "CarbonInitializeThread")
   {
      msg_ptr = AFUNPTR(SimInitializeThread);
   }
   else if (name == "CAPI_Initialize")
   {
      msg_ptr = AFUNPTR(SimInitializeCommId);
   }
   else if (name == "CAPI_rank")
   {
      msg_ptr = AFUNPTR(SimGetCoreID);
   }
   else if (name == "CAPI_message_send_w")
   {
      msg_ptr = AFUNPTR(SimSendW);
   }
   else if (name == "CAPI_message_receive_w")
   {
      msg_ptr = AFUNPTR(SimRecvW);
   }
   else if (name == "mutexInit")
   {
      msg_ptr = AFUNPTR(SimMutexInit);
   }
   else if (name == "mutexLock")
   {
      msg_ptr = AFUNPTR(SimMutexLock);
   }
   else if (name == "mutexUnlock")
   {
      msg_ptr = AFUNPTR(SimMutexUnlock);
   }
   else if (name == "condInit")
   {
      msg_ptr = AFUNPTR(SimCondInit);
   }
   else if (name == "condWait")
   {
      msg_ptr = AFUNPTR(SimCondWait);
   }
   else if (name == "condSignal")
   {
      msg_ptr = AFUNPTR(SimCondSignal);
   }
   else if (name == "condBroadcast")
   {
      msg_ptr = AFUNPTR(SimCondBroadcast);
   }
   else if (name == "barrierInit")
   {
      msg_ptr = AFUNPTR(SimBarrierInit);
   }
   else if (name == "barrierWait")
   {
      msg_ptr = AFUNPTR(SimBarrierWait);
   }

   if (msg_ptr == AFUNPTR(SimGetCoreID)
         || (msg_ptr == AFUNPTR(SimMutexInit))
         || (msg_ptr == AFUNPTR(SimMutexLock))
         || (msg_ptr == AFUNPTR(SimMutexUnlock))
         || (msg_ptr == AFUNPTR(SimCondInit))
         || (msg_ptr == AFUNPTR(SimCondSignal))
         || (msg_ptr == AFUNPTR(SimCondBroadcast))
         || (msg_ptr == AFUNPTR(SimBarrierWait))
      )
   {
      proto = PROTO_Allocate(PIN_PARG(CAPI_return_t),
                             CALLINGSTD_DEFAULT,
                             name.c_str(),
                             PIN_PARG(int*),
                             PIN_PARG_END());
      RTN_ReplaceSignature(rtn, msg_ptr,
                           IARG_PROTOTYPE, proto,
                           IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
                           IARG_END);
      //RTN_Close(rtn);
      PROTO_Free(proto);
      return true;
   }
   else if ((msg_ptr == AFUNPTR(SimSendW)) || (msg_ptr == AFUNPTR(SimRecvW)))
   {
      proto = PROTO_Allocate(PIN_PARG(CAPI_return_t),
                             CALLINGSTD_DEFAULT,
                             name.c_str(),
                             PIN_PARG(CAPI_endpoint_t),
                             PIN_PARG(CAPI_endpoint_t),
                             PIN_PARG(char*),
                             PIN_PARG(int),
                             PIN_PARG_END());
      RTN_ReplaceSignature(rtn, msg_ptr,
                           IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
                           IARG_FUNCARG_ENTRYPOINT_VALUE, 1,
                           IARG_FUNCARG_ENTRYPOINT_VALUE, 2,
                           IARG_FUNCARG_ENTRYPOINT_VALUE, 3,
                           IARG_END);
      //RTN_Close(rtn);
      PROTO_Free(proto);
      return true;
   }
   else if ((msg_ptr == AFUNPTR(SimCondWait)))
   {
      proto = PROTO_Allocate(PIN_PARG(void),
                             CALLINGSTD_DEFAULT,
                             name.c_str(),
                             PIN_PARG(int*),
                             PIN_PARG(int*),
                             PIN_PARG_END());
      RTN_ReplaceSignature(rtn, msg_ptr,
                           IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
                           IARG_FUNCARG_ENTRYPOINT_VALUE, 1,
                           IARG_END);
      //RTN_Close(rtn);
      PROTO_Free(proto);
      return true;
   }
   else if ((msg_ptr == AFUNPTR(SimBarrierInit)))
   {
      proto = PROTO_Allocate(PIN_PARG(void),
                             CALLINGSTD_DEFAULT,
                             name.c_str(),
                             PIN_PARG(int*),
                             PIN_PARG(int),
                             PIN_PARG_END());
      RTN_ReplaceSignature(rtn, msg_ptr,
                           IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
                           IARG_FUNCARG_ENTRYPOINT_VALUE, 1,
                           IARG_END);
      //RTN_Close(rtn);
      PROTO_Free(proto);
      return true;
   }
   else if ((msg_ptr == AFUNPTR(SimInitializeCommId)))
   {
      proto = PROTO_Allocate(PIN_PARG(CAPI_return_t),
                             CALLINGSTD_DEFAULT,
                             name.c_str(),
                             PIN_PARG(int),
                             PIN_PARG_END());
      RTN_ReplaceSignature(rtn, msg_ptr,
                           IARG_PROTOTYPE, proto,
                           IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
                           IARG_END);
      //RTN_Close(rtn);
      PROTO_Free(proto);
      return true;
   }
   else if ((msg_ptr == AFUNPTR(SimInitializeThread)))
   {
      proto = PROTO_Allocate(PIN_PARG(CAPI_return_t),
                             CALLINGSTD_DEFAULT,
                             name.c_str(),
                             PIN_PARG_END());
      RTN_ReplaceSignature(rtn, msg_ptr,
                           IARG_PROTOTYPE, proto,
                           IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
                           IARG_END);
      //RTN_Close(rtn);
      PROTO_Free(proto);
      return true;
   }

   // If the function didn't replace a routine...
   return false;
}