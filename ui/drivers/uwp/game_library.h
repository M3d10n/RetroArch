#pragma once

#include <collection.h> 
#include "systems.h"
#include "common.h"

namespace RetroArch_Win10
{
   [Windows::UI::Xaml::Data::Bindable]
   public ref class Game sealed 
   {
   public:
      Game();

      property Platform::String ^ Title;
      property Platform::String ^ BoxArt;
      property Platform::String ^ Path;
      property ESystemId System;

      void Play();
   };

   typedef Platform::Collections::Vector<Game^> GameVector;
   typedef std::map<ESystemId, GameVector^> SystemGameMap;

   class GameLibrary
   {
   public:

      GameLibrary();

      static GameLibrary* Get();

      void AddGame(Game^ game);

      GameVector^ GetGames() { return m_library; }

      GameVector^ GetGamesBySystem(ESystemId System);


   private:
      GameVector^ m_library;

      SystemGameMap m_library_system;
   };
}