//
// game_list_item.xaml.h
// Declaration of the game_list_item class
//

#pragma once

#include "ui/drivers/uwp/controls/game_list_item.g.h"

using namespace Windows::UI::Xaml;
using namespace Platform;

namespace RetroArch_Win10
{
   ref class Game;

	[Windows::Foundation::Metadata::WebHostHidden]
	public ref class game_list_item sealed
	{
   private:
      static DependencyProperty^ _TitleProperty;
	public:
		game_list_item();

      static property DependencyProperty^ TitleProperty
      {
         DependencyProperty^ get() { return _TitleProperty; }
      }
      property String^ Title
      {
         String^ get() { return (String^)GetValue(TitleProperty); }
         void set(String^ value) { SetValue(TitleProperty, value); }
      }
	};
}
