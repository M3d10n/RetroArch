//
// game_list_item.xaml.cpp
// Implementation of the game_list_item class
//

#include "pch.h"
#include "game_list_item.xaml.h"
#include "ui/drivers/uwp/game_library.h"

using namespace RetroArch_Win10;

using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Navigation;

// The User Control item template is documented at http://go.microsoft.com/fwlink/?LinkId=234236

DependencyProperty^ game_list_item::_TitleProperty = DependencyProperty::Register("Title", Platform::String::typeid, game_list_item::typeid, nullptr);

game_list_item::game_list_item()
{
	InitializeComponent();
}



