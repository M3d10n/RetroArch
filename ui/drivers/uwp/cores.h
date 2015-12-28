#pragma once

#include <collection.h> 

namespace RetroArch_Win10
{
   [Windows::UI::Xaml::Data::Bindable]
   public ref class Core sealed
   {
   public:
      Core();

      property Platform::String^ Name;
      property Platform::String^ Icon;

   };

}