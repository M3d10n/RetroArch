#pragma once

#include <collection.h> 
#include "game_library.h"
#include "common.h"

namespace RetroArch_Win10
{   
   [Windows::UI::Xaml::Data::Bindable]
   [Windows::Foundation::Metadata::WebHostHiddenAttribute]
   public ref class ContentViewModel : public Common::BindableBase
   {
   internal:
      ContentViewModel();

   public:
      property Windows::Foundation::Collections::IObservableVector<IGame^>^ Games
      {
         Windows::Foundation::Collections::IObservableVector<IGame^>^ get();
      }

      property IGame^ SelectedItem
      {
         IGame^ get();
         void set(IGame^ Value);
      }

      void PickGamesToAdd();

   private:
      Platform::Collections::Vector<IGame^>^ m_contentList;

      IGame^ m_selectedItem;
   };
}