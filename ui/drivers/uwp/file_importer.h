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

      property float Progress
      {
         float get();
      }

      property FileImportStatus Status
      {
         FileImportStatus get();
      }

      void Start();
      void Cancel();

   private:
      Windows::Foundation::IAsyncOperation<Windows::Storage::StorageFile^>^ m_async;

      Windows::Storage::Streams::IInputStream^ Input;
      Windows::Storage::Streams::IOutputStream^ Output;

      Windows::Storage::StorageFile^ SourceFile;
      Windows::Storage::StorageFolder^ TargetFolder;

      Windows::Storage::Streams::Buffer^ Buffer;
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