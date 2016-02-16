#include "game_library.h"
#include "system_library.h"
#include "file_importer.h"
#include "uwp.h"

using namespace RetroArch_Win10;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Data;

RetroArch_Win10::Game::Game()
{
   Title = "Unknown Title";
   BoxArt = "ms-appx:///media/assets/xmb/flatui/png/Sega - Mega Drive - Genesis-content.png";
   System = ESystemId::None;
}

Platform::String^ GetDllPath()
{
#if defined(_ARM_)
   return "bin/win_ARM/";
#elif defined(_X86_)
   return "bin/win_x86/";
#elif defined(_AMD64_)
   return "bin/win_x64/";
#endif
}

void RetroArch_Win10::Game::Play()
{
   if (System == ESystemId::None || Path == nullptr)
   {
      return;
   }

   auto system = SystemLibrary::Get()->GetSystem(System);

   Platform::String^ core = GetDllPath() + system->Core + "_libretro.dll";

   if (RetroarchMain::Instance.get())
   {
      RetroarchMain::Instance->StopUpdateThread(true);
   }

   RetroarchMain::Instance = std::unique_ptr<RetroarchMain>(new RetroarchMain(core, Path));
   RetroarchMain::Instance->StartUpdateThread();

   GameLibrary::Get()->GetDispatcher()->DispatchGameStarted(this);
}

void RetroArch_Win10::Game::ImportFrom(Windows::Storage::StorageFile ^ file)
{
   auto LibraryFolder = Windows::Storage::ApplicationData::Current->LocalFolder;
   m_importTask = FileImportManager::Get()->QueueImportTask(file, LibraryFolder);
   
   m_importTask->Progress += ref new Windows::Foundation::EventHandler<float>([this](Platform::Object ^sender, float progress)
   {
      PropertyChanged(this, ref new PropertyChangedEventArgs("Progress"));
   });

   m_importTask->Completed += ref new Windows::Foundation::EventHandler<Windows::Storage::StorageFile ^>([this](Platform::Object ^sender, Windows::Storage::StorageFile ^args)
   {
      if (args)
      {
         this->Path = args->Path;
      }
      else
      {
         GameLibrary::Get()->RemoveGame(this);

      }

      PropertyChanged(this, ref new PropertyChangedEventArgs("Importing"));
      PropertyChanged(this, ref new PropertyChangedEventArgs("Status"));
      PropertyChanged(this, ref new PropertyChangedEventArgs("Progress"));
   });

   PropertyChanged(this, ref new PropertyChangedEventArgs("Importing"));
   PropertyChanged(this, ref new PropertyChangedEventArgs("Status"));
   PropertyChanged(this, ref new PropertyChangedEventArgs("Progress"));
}

FileImportStatus RetroArch_Win10::Game::Status::get()
{
   if (m_importTask)
   {
      return m_importTask->Status;
   }
   return FileImportStatus::Completed;
}

float RetroArch_Win10::Game::Progress::get()
{
   if (m_importTask)
   {
      return m_importTask->CurrentProgress;
   }
   return 0.0f;
}


bool RetroArch_Win10::Game::Importing::get()
{
   if (m_importTask)
   {
      return m_importTask->Status != FileImportStatus::Completed;
   }
   return false;
}


bool RetroArch_Win10::Game::FileError::get()
{
   if (m_importTask)
   {
      return m_importTask->Status == FileImportStatus::Error ||
         m_importTask->Status == FileImportStatus::Canceled;
   }
   return false;
}


RetroArch_Win10::GameLibrary::GameLibrary()
{
   m_library = ref new GameVector();
   m_dispatcher = ref new GameLibraryDispatcher();

   auto game = ref new Game();
   AddGame(game);
   AddGame(game);
   AddGame(game);
}

GameLibrary * RetroArch_Win10::GameLibrary::Get()
{
   static GameLibrary * singleton = nullptr;

   if (singleton == nullptr)
   {
      singleton = new GameLibrary();
   }

   return singleton;
}

void RetroArch_Win10::GameLibrary::AddGame(Game ^ game)
{
   m_library->Append(game);
   GetGamesBySystem(game->System)->Append(game);
}

void RetroArch_Win10::GameLibrary::RemoveGame(Game ^ game)
{
   unsigned int index = 0;
   if (m_library->IndexOf(game, &index))
   {
      m_library->RemoveAt(index);
   }
   if (GetGamesBySystem(game->System)->IndexOf(game, &index))
   {
      GetGamesBySystem(game->System)->RemoveAt(index);
   }

}

GameVector ^ RetroArch_Win10::GameLibrary::GetGamesBySystem(ESystemId system)
{
   auto itr = m_library_system.find(system);
   if (itr == m_library_system.end())
   {
      auto newVec = ref new GameVector();
      m_library_system[system] = newVec;
      return newVec;
   }
   return itr->second;
}

void RetroArch_Win10::GameLibraryDispatcher::DispatchGameStarted(Game ^ game)
{
   GameStarted(game);
}
