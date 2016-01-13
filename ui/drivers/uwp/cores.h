#pragma once

#include <collection.h> 
#include "common.h"
#include "system_library.h"

namespace RetroArch_Win10
{
   public delegate void SystemSelectedDelegate(ISystem^);

   [Windows::UI::Xaml::Data::Bindable]
   [Windows::Foundation::Metadata::WebHostHiddenAttribute]
   public ref class CoresViewModel : public Common::BindableBase
   {
   internal:
      CoresViewModel();

   public:
      property Windows::Foundation::Collections::IObservableVector<ISystem^>^ Systems
      {
         Windows::Foundation::Collections::IObservableVector<ISystem^>^ get();
      }

      event SystemSelectedDelegate^ ItemSelected;


      property ISystem^ SelectedItem
      {
         ISystem^ get();
         void set(ISystem^ Value);
      }

   private:

      ISystem^ m_selectedItem;
   };
}