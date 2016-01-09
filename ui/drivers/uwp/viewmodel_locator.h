#pragma once

#include "cores.h"
#include "content_vm.h"

namespace RetroArch_Win10
{
   [Windows::UI::Xaml::Data::Bindable]
   [Windows::Foundation::Metadata::WebHostHidden]
   public ref class ViewModelLocator sealed
   {
   public:
      ViewModelLocator();

      property CoresViewModel^ CoresVM { CoresViewModel^ get(); }
      property ContentViewModel^ ContentVM { ContentViewModel^ get(); }
   };
}