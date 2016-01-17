#pragma once

#include <collection.h> 
#include "systems.h"
#include "common.h"

namespace RetroArch_Win10
{
   [Windows::UI::Xaml::Data::Bindable]
   public ref class System sealed
   {
   public:
      System();

      property ESystemId Id;
      property Platform::String ^ Name;
      property Platform::String ^ Maker;
      property Platform::String ^ Icon;
      property Platform::String ^ Background;
      property Platform::String ^ MediaIcon;

      property Platform::String ^ Core;
      property Windows::Foundation::Collections::IVector<Platform::String^>^ FileTypes;

      property Platform::String ^ LandscapeOverlay;
      property Platform::String ^ PortraitOverlay;
   };

   public delegate void SelectedSystemChangedDelegate(System^);

   [Windows::Foundation::Metadata::WebHostHiddenAttribute]
   public ref class SystemLibraryDispatcher sealed
   {
   public:
      event SelectedSystemChangedDelegate^ SelectedSystemChanged;
      void DispatchSelectedSystemChanged(System^ newSystem);
   };


   class SystemLibrary
   {
   public:
      SystemLibrary();

      static SystemLibrary* Get();

      typedef Platform::Collections::Vector<System^> SystemsVector;

      typedef Platform::Collections::Map<ESystemId, System^> SystemsMap;

      void RegisterSystem(System^ newSystem);

      SystemsVector^ GetSystems() { return m_library; }

      System^ GetSystem(ESystemId Id);

      void SetSelectedSystem(System^ system);

      System^ GetSelectedSystem() { return m_selectedSystem; }

      SystemLibraryDispatcher^ GetDispatcher() { return m_dispatcher; }
            
   private:
      SystemsVector^ m_library;
      SystemsMap^    m_library_map;
      System^       m_selectedSystem;

      SystemLibraryDispatcher^ m_dispatcher;
   };
}