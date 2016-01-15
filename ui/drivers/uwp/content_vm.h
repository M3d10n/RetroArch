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
      property Windows::Foundation::Collections::IObservableVector<Game^>^ Games
      {
         Windows::Foundation::Collections::IObservableVector<Game^>^ get();
      }

      property Game^ SelectedItem
      {
         Game^ get();
         void set(Game^ Value);
      }

      void PickGamesToAdd();

   private:
      Platform::Collections::Vector<Game^>^ m_contentList;

      Game^ m_selectedItem;
   };
}