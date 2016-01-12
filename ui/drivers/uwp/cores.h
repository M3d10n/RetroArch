#pragma once

#include <collection.h> 
#include "common.h"
#include "system_library.h"

namespace RetroArch_Win10
{
   public interface class ICoreInfoDisplay
   {
      property Platform::String^ Name;
      property Platform::String^ Icon;
      property Platform::String^ Background;
   };


   [Windows::UI::Xaml::Data::Bindable]
   public ref class Core sealed : public ICoreInfoDisplay
   {
   public:
      Core();

      // Inherited via ICoreInfoDisplay
      virtual property Platform::String ^ Name;
      virtual property Platform::String ^ Icon;
      virtual property Platform::String ^ Background;


   };


   [Windows::UI::Xaml::Data::Bindable]
   [Windows::Foundation::Metadata::WebHostHiddenAttribute]
   public ref class CoresViewModel : public Common::BindableBase
   {
   internal:
      CoresViewModel();

   public:
      property Windows::Foundation::Collections::IObservableVector<ICoreInfoDisplay^>^ Cores
      {
         Windows::Foundation::Collections::IObservableVector<ICoreInfoDisplay^>^ get()
         {
            return m_cores;
         }
      }

      property Windows::Foundation::Collections::IObservableVector<ISystem^>^ Systems
      {
         Windows::Foundation::Collections::IObservableVector<ISystem^>^ get();
      }


      property ICoreInfoDisplay^ SelectedItem
      {
         ICoreInfoDisplay^ get();
         void set(ICoreInfoDisplay^ Value);
      }

   private:
      Platform::Collections::Vector<ICoreInfoDisplay^>^ m_cores;

      ICoreInfoDisplay^ m_selectedItem;
   };
}