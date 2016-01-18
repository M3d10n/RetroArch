//
// in_game_page.xaml.cpp
// Implementation of the in_game_page class
//

#include "pch.h"
#include "in_game_page.xaml.h"
#include "uwp.h"

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

in_game_page::in_game_page()
{
	InitializeComponent();
}

void RetroArch_Win10::in_game_page::OnNavigatedFrom(Windows::UI::Xaml::Navigation::NavigationEventArgs ^ e)
{

   if (Windows::Foundation::Metadata::ApiInformation::IsTypePresent("Windows.UI.ViewManagement.StatusBar"))
   {
      Windows::UI::ViewManagement::ApplicationView::GetForCurrentView()->ExitFullScreenMode();
      Windows::UI::ViewManagement::StatusBar::GetForCurrentView()->ShowAsync();
   }

   if (RetroarchMain::Instance.get())
   {
      RetroarchMain::Instance->StopUpdateThread();
      RetroarchMain::Instance.reset(nullptr);
   }
}

void RetroArch_Win10::in_game_page::OnNavigatedTo(Windows::UI::Xaml::Navigation::NavigationEventArgs ^ e)
{
   
   if (Windows::Foundation::Metadata::ApiInformation::IsTypePresent("Windows.UI.ViewManagement.StatusBar"))
   {
      Windows::UI::ViewManagement::ApplicationView::GetForCurrentView()->TryEnterFullScreenMode();
      Windows::UI::ViewManagement::StatusBar::GetForCurrentView()->HideAsync();
   }
}
