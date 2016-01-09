#pragma once

#include <collection.h> 
#include "systems.h"
#include "common.h"

namespace RetroArch_Win10
{
   public interface class IGame
   {
      // The game title
      property Platform::String^ Title;

      // Box art image path
      property Platform::String^ BoxArt;

      // The path to the game content (ROM, CD image, etc.)
      property Platform::String^ Path;

      // The game's system
      property ESystemId System;
   };

   [Windows::UI::Xaml::Data::Bindable]
   public ref class Game sealed : public IGame
   {
   public:
      // Inherited via IGame
      virtual property Platform::String ^ Title;
      virtual property Platform::String ^ BoxArt;
      virtual property Platform::String ^ Path;
      virtual property ESystemId System;
   };

   class GameLibrary
   {
   public:
      typedef Platform::Collections::Vector<IGame^> GameVector;

      typedef Platform::Collections::Map<ESystemId, GameVector^> GameVector;


      GameLibrary();

      static GameLibrary* Get();

      void AddGame(IGame^ game);

      GameVector^ GetGames() { return m_library; }

   private:
      GameVector^ m_library;
   };
}