#pragma once

#include "common.h"
#include <collection.h> 

namespace RetroArch_Win10
{
   
   public ref class FileImportEntry sealed
   {
   public:
      FileImportEntry(Windows::Storage::StorageFile^ SourceFile, Windows::Storage::StorageFolder^ TargetFolder);

      event Windows::Foundation::EventHandler<Windows::Storage::StorageFile^>^ Completed;
      event Windows::Foundation::EventHandler<float>^ Progress;

      property float CurrentProgress
      {
         float get() { return m_progress; }
      }

      property FileImportStatus Status
      {
         FileImportStatus get();
      }

      void Start();
      void Cancel();

   private:
      Windows::Foundation::IAsyncOperationWithProgress<uint64, uint64>^ m_async;
      Windows::Foundation::IAsyncAction^ m_progressWorker;

      Windows::Storage::Streams::IInputStream^ Input;
      Windows::Storage::Streams::IOutputStream^ Output;

      Windows::Storage::StorageFile^ SourceFile;
      Windows::Storage::StorageFile^ TargetFile;
      Windows::Storage::StorageFolder^ TargetFolder;

      Windows::UI::Core::CoreDispatcher^ Dispatcher;

      uint64 m_fileSize;
      float m_progress;
   };

   class FileImportManager
   {
   public:
      FileImportManager();

      FileImportEntry^ QueueImportTask(Windows::Storage::StorageFile^ SourceFile, Windows::Storage::StorageFolder^ TargetFolder);

      static FileImportManager* Get();
   private:
      unsigned int ActiveTasks;
      std::list<FileImportEntry^> m_queue;
   };
}