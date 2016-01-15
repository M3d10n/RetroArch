//
// content.xaml.cpp
// Implementation of the content class
//

#include "pch.h"
#include "content.xaml.h"
#include "in_game_page.xaml.h"


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

// The Blank Page item template is documented at http://go.microsoft.com/fwlink/?LinkId=234238

content::content()
{
	InitializeComponent();
}

void RetroArch_Win10::content::Add_Clicked(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
   auto content_vm = static_cast<ContentViewModel^>(DataContext);
   content_vm->PickGamesToAdd();
}


void RetroArch_Win10::content::ItemClick(Platform::Object^ sender, Windows::UI::Xaml::Controls::ItemClickEventArgs^ e)
{
   auto game = static_cast<Game^>(e->ClickedItem);
   game->Play();
   auto frame = static_cast<Windows::UI::Xaml::Controls::Frame^>(Parent);
   bool ok = frame->Navigate(in_game_page::typeid);
}

