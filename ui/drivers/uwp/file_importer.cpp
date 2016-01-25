#include "file_importer.h"
#include <ppltasks.h>

using namespace concurrency;
using namespace RetroArch_Win10;
using namespace Windows::Storage;

#define MAX_ACTIVE_TASKS 2

float FileImportEntry::Progress::get()
{
   return 0.0f;
}

FileImportStatus FileImportEntry::Status::get()
{
   if (m_async)
   {
      auto status = m_async->Status;
      switch (status)
      {
      case Windows::Foundation::AsyncStatus::Canceled:
         return FileImportStatus::Canceled;
      case Windows::Foundation::AsyncStatus::Completed:
         return FileImportStatus::Completed;
      case Windows::Foundation::AsyncStatus::Error:
         return FileImportStatus::Error;
      case Windows::Foundation::AsyncStatus::Started:
         return FileImportStatus::Working;
      }
   }
   return FileImportStatus::Pending;
}

RetroArch_Win10::FileImportManager::FileImportManager() :
   ActiveTasks(0)
{
}

FileImportEntry^ RetroArch_Win10::FileImportManager::QueueImportTask(Windows::Storage::StorageFile^ SourceFile, Windows::Storage::StorageFile^ TargetFile)
{
   auto entry = ref new FileImportEntry(SourceFile, TargetFile);

   if (ActiveTasks < MAX_ACTIVE_TASKS)
   {
      ActiveTasks++;
      entry->Start();
   }
   else
   {
      m_queue.push_back(entry);
   }

   entry->Completed += ref new Windows::Foundation::EventHandler<Windows::Storage::StorageFile ^>([this](Platform::Object^, StorageFile^)
   {
      ActiveTasks--;
      if (m_queue.size() > 0)
      {
         auto queued_entry = *m_queue.begin();
         m_queue.pop_front();
         ActiveTasks++;
         queued_entry->Start();
      }
   });

   return entry;
}

RetroArch_Win10::FileImportEntry::FileImportEntry(Windows::Storage::StorageFile^ SourceFile, Windows::Storage::StorageFile^ TargetFile) :
   SourceFile(SourceFile),
   TargetFile(TargetFile)
{
}

void RetroArch_Win10::FileImportEntry::Start()
{
   m_async = SourceFile->CopyAndReplaceAsync(TargetFile);
   create_task(m_async).then([=]()
   {
      this->Completed(this, TargetFile);
   });
}

void RetroArch_Win10::FileImportEntry::Cancel()
{
   if (m_async)
   {
      m_async->Cancel();
      this->Completed(this, nullptr);
   }
}
