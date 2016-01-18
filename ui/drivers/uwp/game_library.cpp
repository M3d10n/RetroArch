#include "game_library.h"
#include "system_library.h"
#include "uwp.h"

using namespace RetroArch_Win10;

RetroArch_Win10::Game::Game()
{
   Title = "Unknown Title";
   BoxArt = "ms-appx:///media/assets/xmb/flatui/png/Sega - Mega Drive - Genesis-content.png";
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
