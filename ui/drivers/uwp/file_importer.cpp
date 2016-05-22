#include "file_importer.h"
#include <ppltasks.h>
#include <ppl.h>
#include <array>

using namespace concurrency;
using namespace RetroArch_Win10;
using namespace Windows::Foundation;
using namespace Windows::Storage;
using namespace Windows::System::Threading;
using namespace Windows::UI::Core;

#define MAX_ACTIVE_TASKS 2

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

FileImportEntry^ RetroArch_Win10::FileImportManager::QueueImportTask(Windows::Storage::StorageFile^ SourceFile, Windows::Storage::StorageFolder^ TargetFolder)
{
   auto entry = ref new FileImportEntry(SourceFile, TargetFolder);

   Windows::Storage::AccessCache::StorageApplicationPermissions::MostRecentlyUsedList->Add(SourceFile);

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

FileImportManager * RetroArch_Win10::FileImportManager::Get()
{
   static FileImportManager* manager = new FileImportManager();
   return manager;
}

RetroArch_Win10::FileImportEntry::FileImportEntry(Windows::Storage::StorageFile^ SourceFile, Windows::Storage::StorageFolder^ TargetFolder) :
   SourceFile(SourceFile),
   TargetFolder(TargetFolder),
   m_fileSize(0),
   m_progress(0)
{
}

void RetroArch_Win10::FileImportEntry::Start()
{
   // Grab the current thread window dispatcher
   auto window = CoreWindow::GetForCurrentThread();
   if (window)
   {
      Dispatcher = window->Dispatcher;
   }
   else
   {
      Dispatcher = nullptr;
   }

   // Get the output stream
   auto t0 = create_task(TargetFolder->CreateFileAsync(Platform::String::Concat(SourceFile->Name, ".tmp"), CreationCollisionOption::ReplaceExisting))
      .then([this](StorageFile^ targetFile)
   {
      TargetFile = targetFile;
      return targetFile->OpenAsync(FileAccessMode::ReadWrite);
   })
      .then([this](Streams::IRandomAccessStream^ stream)
   {
      Output = stream->GetOutputStreamAt(0);
   });

   // Get the input stream
   auto t1 = create_task(SourceFile->OpenSequentialReadAsync())
      .then([=](Streams::IInputStream^ stream)
   {
      Input = stream;
   });

   // Get the source file size
   auto t2 = create_task(SourceFile->GetBasicPropertiesAsync())
      .then([this](FileProperties::BasicProperties^ properties)
   {
      m_fileSize = properties->Size;
   });

   // Copy when both tasks complete
   (t0 && t1 && t2).then([this]()
   {
      m_async = Streams::RandomAccessStream::CopyAndCloseAsync(Input, Output);
      m_async->Progress = ref new AsyncOperationProgressHandler<uint64, uint64>([this](IAsyncOperationWithProgress<uint64, uint64>^ m_async, uint64 progress) {
         m_progress = float((double)progress / (double)m_fileSize);
         
         if (Dispatcher)
         {
            Dispatcher->RunAsync(CoreDispatcherPriority::Normal, ref new DispatchedHandler([this]()
            {
               Progress(this, m_progress);
            }));
         }
      });

      // Rename when done copying
      auto t3 = create_task(m_async).then([this](uint64)
      {
         return TargetFile->RenameAsync(SourceFile->Name, NameCollisionOption::ReplaceExisting);
      })
         .then([this](void)
      {
         Completed(this, TargetFile);
         m_async = nullptr;
      });
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
