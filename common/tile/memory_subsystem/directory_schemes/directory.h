#ifndef __DIRECTORY_H__
#define __DIRECTORY_H__

#include <string>

#include "directory_entry.h"
#include "fixed_types.h"

class Directory
{
   public:
      enum DirectoryType
      {
         FULL_MAP = 0,
         LIMITED_NO_BROADCAST,
         LIMITED_BROADCAST,
         ACKWISE,
         LIMITLESS,
         NUM_DIRECTORY_TYPES
      };

   private:
      DirectoryType m_directory_type;
      UInt32 m_num_entries;
      UInt32 m_max_hw_sharers;
      UInt32 m_max_num_sharers;

      // FIXME: Hack: Get me out of here
      UInt32 m_limitless_software_trap_penalty;

      DirectoryEntry** m_directory_entry_list;

   public:
      Directory(std::string directory_type_str, UInt32 num_entries, UInt32 max_hw_sharers, UInt32 max_num_sharers);
      ~Directory();

      DirectoryEntry* getDirectoryEntry(UInt32 entry_num);
      UInt32 getDirectoryEntrySize();
      void setDirectoryEntry(UInt32 entry_num, DirectoryEntry* directory_entry);
      DirectoryEntry* createDirectoryEntry();
      
      static DirectoryType parseDirectoryType(std::string directory_type_str);
};

#endif /* __DIRECTORY_H__ */
