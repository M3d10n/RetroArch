//
// in_game_page.xaml.h
// Declaration of the in_game_page class
//

#pragma once

#include "ui/drivers/uwp/in_game_page.g.h"

namespace RetroArch_Win10
{
	/// <summary>
	/// An empty page that can be used on its own or navigated to within a Frame.
	/// </summary>
	[Windows::Foundation::Metadata::WebHostHidden]
	public ref class in_game_page sealed
	{
	public:
		in_game_page();

      virtual void OnNavigatedFrom(Windows::UI::Xaml::Navigation::NavigationEventArgs^ e) override;
      virtual void OnNavigatedTo(Windows::UI::Xaml::Navigation::NavigationEventArgs^ e) override;

      property bool IsGamePaused;

   private:
      Windows::Foundation::EventRegistrationToken BackRequestedToken;
      void OnBackRequested(Platform::Object ^sender, Windows::UI::Core::BackRequestedEventArgs ^args);


      void ResetClick(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
      void ResumeClicked(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
      void CommandBarClosing(Platform::Object^ sender, Platform::Object^ e);
   };
}
