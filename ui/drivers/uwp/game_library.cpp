#include "game_library.h"

using namespace RetroArch_Win10;

RetroArch_Win10::GameLibrary::GameLibrary()
{
   m_library = ref new GameVector();

   // Test
   auto game = ref new Game();
   game->Title = "Sonic The Hedgehog 2";
   game->Path = "";

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

void RetroArch_Win10::GameLibrary::AddGame(IGame ^ game)
{
   m_library->Append(game);
}
