#pragma once

#include <collection.h> 
#include "common.h"
#include "system_library.h"

namespace RetroArch_Win10
{
   public delegate void SystemSelectedDelegate(System^);

   [Windows::UI::Xaml::Data::Bindable]
   [Windows::Foundation::Metadata::WebHostHiddenAttribute]
   public ref class CoresViewModel : public Common::BindableBase
   {
   internal:
      CoresViewModel();

   public:
      property Windows::Foundation::Collections::IObservableVector<System^>^ Systems
      {
         Windows::Foundation::Collections::IObservableVector<System^>^ get();
      }

      property System^ SelectedSystem
      {
         System^ get();
      }

      event SystemSelectedDelegate^ ItemSelected;

      void SetSelectedSystem(System^ Value);

   private:
   };
}