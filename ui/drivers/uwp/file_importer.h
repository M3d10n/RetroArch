#pragma once

#include <collection.h> 

namespace RetroArch_Win10
{
   public enum class FileImportStatus
   {
      Pending,
      Working,
      Completed,
      Canceled,
      Error,
   };

   public ref class FileImportEntry sealed
   {
   public:
      FileImportEntry(Windows::Storage::StorageFile^ SourceFile, Windows::Storage::StorageFile^ TargetFile);

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
      Windows::Foundation::IAsyncAction^ m_async;

      Windows::Storage::Streams::IInputStream^ Input;
      Windows::Storage::Streams::IOutputStream^ Output;

      Windows::Storage::StorageFile^ SourceFile;
      Windows::Storage::StorageFile^ TargetFile;

      Windows::Storage::Streams::Buffer^ Buffer;
   };

   class FileImportManager
   {
   public:
      FileImportManager();

      FileImportEntry^ QueueImportTask(Windows::Storage::StorageFile^ SourceFile, Windows::Storage::StorageFile^ TargetFile);

   private:
      unsigned int ActiveTasks;
      std::list<FileImportEntry^> m_queue;
   };
}