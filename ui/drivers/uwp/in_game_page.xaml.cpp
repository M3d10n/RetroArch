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
   IsGamePaused = false;
	InitializeComponent();
}

void RetroArch_Win10::in_game_page::OnNavigatedFrom(Windows::UI::Xaml::Navigation::NavigationEventArgs ^ e)
{

   if (Windows::Foundation::Metadata::ApiInformation::IsTypePresent("Windows.UI.ViewManagement.StatusBar"))
   {
	   using namespace Windows::UI::ViewManagement;
      ApplicationView::GetForCurrentView()->ExitFullScreenMode();
	  ApplicationView::GetForCurrentView()->SetDesiredBoundsMode(ApplicationViewBoundsMode::UseVisible);
      StatusBar::GetForCurrentView()->ShowAsync();
   }

   if (RetroarchMain::Instance.get())
   {
      RetroarchMain::Instance->StopUpdateThread(false);
   }

   Common::Dispatcher::Get()->BackRequested -= BackRequestedToken;

}

void RetroArch_Win10::in_game_page::OnNavigatedTo(Windows::UI::Xaml::Navigation::NavigationEventArgs ^ e)
{   
   if (Windows::Foundation::Metadata::ApiInformation::IsTypePresent("Windows.UI.ViewManagement.StatusBar"))
   {
	  using namespace Windows::UI::ViewManagement;
      ApplicationView::GetForCurrentView()->TryEnterFullScreenMode();	  
	  ApplicationView::GetForCurrentView()->SetDesiredBoundsMode(ApplicationViewBoundsMode::UseCoreWindow);
      StatusBar::GetForCurrentView()->HideAsync();
   }

   BackRequestedToken = Common::Dispatcher::Get()->BackRequested +=
      ref new Windows::Foundation::EventHandler<Windows::UI::Core::BackRequestedEventArgs ^>(this, &RetroArch_Win10::in_game_page::OnBackRequested);
}

void RetroArch_Win10::in_game_page::OnBackRequested(Platform::Object ^ sender, Windows::UI::Core::BackRequestedEventArgs ^ args)
{
   if (args->Handled)
   {
      return;
   }

   if (!IsGamePaused)
   {
      IsGamePaused = true;
      this->BottomAppBar->IsOpen = true;

      if (Windows::Foundation::Metadata::ApiInformation::IsTypePresent("Windows.UI.ViewManagement.StatusBar"))
      {
         Windows::UI::ViewManagement::ApplicationView::GetForCurrentView()->ExitFullScreenMode();
         Windows::UI::ViewManagement::StatusBar::GetForCurrentView()->ShowAsync();
      }

      args->Handled = true;
      if (RetroarchMain::Instance.get())
      {
         RetroarchMain::Instance->PauseGame();
      }
   }
}


void RetroArch_Win10::in_game_page::ResetClick(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
   if (RetroarchMain::Instance.get())
   {
      RetroarchMain::Instance->ResetGame();
   }
}


void RetroArch_Win10::in_game_page::ResumeClicked(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
   this->BottomAppBar->IsOpen = false;
}


void RetroArch_Win10::in_game_page::CommandBarClosing(Platform::Object^ sender, Platform::Object^ e)
{
   if (IsGamePaused && RetroarchMain::Instance.get())
   {
      IsGamePaused = false;
      RetroarchMain::Instance->ResumeGame();

      if (Windows::Foundation::Metadata::ApiInformation::IsTypePresent("Windows.UI.ViewManagement.StatusBar"))
      {
         Windows::UI::ViewManagement::ApplicationView::GetForCurrentView()->TryEnterFullScreenMode();
         Windows::UI::ViewManagement::StatusBar::GetForCurrentView()->HideAsync();
      }
   }
}
