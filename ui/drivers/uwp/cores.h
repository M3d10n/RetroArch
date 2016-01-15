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

      property ISystem^ SelectedSystem
      {
         ISystem^ get();
      }

      event SystemSelectedDelegate^ ItemSelected;

      void SetSelectedSystem(ISystem^ Value);

   private:
   };
}