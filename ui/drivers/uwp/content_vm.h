#pragma once

#include <collection.h> 
#include "game_library.h"
#include "common.h"

namespace RetroArch_Win10
{
   public interface class IContentDisplay
   {
      property Platform::String^ Name;
      property Platform::String^ BoxArt;
   };

   [Windows::UI::Xaml::Data::Bindable]
   public ref class Content sealed : public IContentDisplay
   {
   public:
      // Inherited via IContentDisplay
      virtual property Platform::String ^ Name;
      virtual property Platform::String ^ BoxArt;
   };

   [Windows::UI::Xaml::Data::Bindable]
   [Windows::Foundation::Metadata::WebHostHiddenAttribute]
   public ref class ContentViewModel : public Common::BindableBase
   {
   internal:
      ContentViewModel();

   public:
      property Windows::Foundation::Collections::IObservableVector<IContentDisplay^>^ Cores
      {
         Windows::Foundation::Collections::IObservableVector<IContentDisplay^>^ get()
         {
            return m_contentList;
         }
      }

      property IContentDisplay^ SelectedItem
      {
         IContentDisplay^ get();
         void set(IContentDisplay^ Value);
      }

   private:
      Platform::Collections::Vector<IContentDisplay^>^ m_contentList;

      IContentDisplay^ m_selectedItem;
   };
}