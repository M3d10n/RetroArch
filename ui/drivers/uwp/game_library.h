#pragma once

#include <collection.h> 
#include "systems.h"
#include "common.h"

namespace RetroArch_Win10
{
   [Windows::UI::Xaml::Data::Bindable]
   public ref class Game sealed : public Common::BindableBase
   {
   public:
      Game();

      property Platform::String ^ Title;
      property Platform::String ^ BoxArt;
      property Platform::String ^ Path;
      property ESystemId System;

      property FileImportStatus Status
      {
         FileImportStatus get();
      }

      property float Progress
      {
         float get();
      }

      property bool Importing
      {
         bool get();
      }

      property bool FileError
      {
         bool get();
      }

      void Play();

      void ImportFrom(Windows::Storage::StorageFile^ file);

   private:
      ref class FileImportEntry^ m_importTask;
   };
   
   public delegate void GameDelegate(Game^);


   [Windows::Foundation::Metadata::WebHostHiddenAttribute]
   public ref class GameLibraryDispatcher sealed
   {
   public:
      event GameDelegate^ GameStarted;
      void DispatchGameStarted(Game^ game);
   };


   typedef Platform::Collections::Vector<Game^> GameVector;
   typedef std::map<ESystemId, GameVector^> SystemGameMap;

   class GameLibrary
   {
   public:

      GameLibrary();
      ~GameLibrary();

      static GameLibrary* Get();

      void AddGame(Game^ game);

      void RemoveGame(Game^ game);

      void AddGameToDb(Game^ game);

      void LoadGamesFromDb();

      GameVector^ GetGames() { return m_library; }

      GameVector^ GetGamesBySystem(ESystemId System);

      GameLibraryDispatcher^ GetDispatcher() { return m_dispatcher; }
   private:
      GameVector^ m_library;

      SystemGameMap m_library_system;

      GameLibraryDispatcher^ m_dispatcher;

      ref class SQLite::Core::Database^ m_db;
   };
}