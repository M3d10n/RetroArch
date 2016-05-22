#include "game_library.h"
#include "system_library.h"
#include "file_importer.h"
#include "sqlite_codes.h"
#include "uwp.h"

#include <vector>

using namespace RetroArch_Win10;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Data;
using namespace SQLite::Core;

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
         GameLibrary::Get()->AddGameToDb(this);
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

   // Initialize database
   auto db_path = Windows::Storage::ApplicationData::Current->LocalFolder->Path + "\\library_v0.db";
   int rc = Sqlite3::sqlite3_open(db_path, &m_db);
   if (rc) {
      Sqlite3::sqlite3_close(m_db);
      retro_assert(0);
   }

   char* db_err = nullptr;

   //rc = sqlite3_exec(m_db, "DROP TABLE IF EXISTS library", NULL, NULL, &db_err);
   //retro_assert(!rc);
   
   Statement^ stm = nullptr;
   rc = Sqlite3::sqlite3_prepare_v2(m_db, "CREATE TABLE IF NOT EXISTS library(id INTEGER PRIMARY KEY AUTOINCREMENT, path TEXT UNIQUE, title TEXT, boxart TEXT, system INTEGER)", &stm);
   retro_assert(!rc);   
   rc = Sqlite3::sqlite3_step(stm);
   retro_assert(rc == SQLITE_DONE);
   rc = Sqlite3::sqlite3_finalize(stm);
   retro_assert(!rc);

   LoadGamesFromDb();
}

RetroArch_Win10::GameLibrary::~GameLibrary()
{
   Sqlite3::sqlite3_close(m_db);
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

void RetroArch_Win10::GameLibrary::AddGameToDb(Game ^ game)
{
   Statement^ stmt = nullptr;
   int rc = 0;

   rc = Sqlite3::sqlite3_prepare_v2(m_db, "INSERT INTO library (path, title, boxart, system) VALUES (?, ?, ?, ?)", &stmt);
   retro_assert(!rc);

   rc = Sqlite3::sqlite3_bind_text(stmt, 1, game->Path, -1);
   retro_assert(!rc);

   rc = Sqlite3::sqlite3_bind_text(stmt, 2, game->Title, -1);
   retro_assert(!rc);

   rc = Sqlite3::sqlite3_bind_text(stmt, 3, game->BoxArt, -1);
   retro_assert(!rc);

   rc = Sqlite3::sqlite3_bind_int(stmt, 4, (int)game->System);
   retro_assert(!rc);

   do {
      rc = Sqlite3::sqlite3_step(stmt);
      if (rc == SQLITE_BUSY)
         continue;

      retro_assert(rc != SQLITE_ERROR);
   } while (false);

   rc = Sqlite3::sqlite3_finalize(stmt);
   retro_assert(!rc);
}

void RetroArch_Win10::GameLibrary::LoadGamesFromDb()
{
   char* db_err = nullptr;

   Statement^ stmt = nullptr;
   int rc = 0;
  
   rc = Sqlite3::sqlite3_prepare_v2(m_db, "SELECT ALL id, path, title, boxart, system FROM library", &stmt);
   retro_assert(!rc);
   // TODO: make this async
   do {
      rc = Sqlite3::sqlite3_step(stmt);
      switch (rc)
      {
         case SQLITE_ROW:
         {
            auto game = ref new Game();
            game->Path = Sqlite3::sqlite3_column_text(stmt, 1);
            game->Title = Sqlite3::sqlite3_column_text(stmt, 2);
            game->BoxArt = Sqlite3::sqlite3_column_text(stmt, 3);
            game->System = (ESystemId)Sqlite3::sqlite3_column_int(stmt, 4);
            AddGame(game);
         }
         case SQLITE_BUSY:
            continue;
      }
      retro_assert(rc == SQLITE_DONE);
   } while (rc != SQLITE_DONE);


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
