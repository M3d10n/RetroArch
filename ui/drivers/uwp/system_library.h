#pragma once

#include <collection.h> 
#include "systems.h"
#include "common.h"

namespace RetroArch_Win10
{
   public interface class ISystem
   {
      property ESystemId Id;

      property Platform::String^ Name;

      property Platform::String^ Maker;

      property Platform::String^ Icon;

      property Platform::String^ Background;

      property Platform::String^ Core;

   };

   [Windows::UI::Xaml::Data::Bindable]
   public ref class System sealed : public ISystem
   {
   public:

      // Inherited via ISystem
      virtual property ESystemId Id;
      virtual property Platform::String ^ Name;
      virtual property Platform::String ^ Maker;
      virtual property Platform::String ^ Icon;
      virtual property Platform::String ^ Background;
      virtual property Platform::String ^ Core;
   };

   class SystemLibrary
   {
   public:
      SystemLibrary();

      static SystemLibrary* Get();

      typedef Platform::Collections::Vector<ISystem^> SystemsVector;

      typedef Platform::Collections::Map<ESystemId, ISystem^> SystemsMap;

      void RegisterSystem(ISystem^ newSystem);

      SystemsVector^ GetSystems() { return m_library; }

      ISystem^ GetSystem(ESystemId Id);

   private:
      SystemsVector^ m_library;
      SystemsMap^    m_library_map;
   };
}