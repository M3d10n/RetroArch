//
// content.xaml.h
// Declaration of the content class
//

#pragma once

#include "ui/drivers/uwp/content.g.h"

namespace RetroArch_Win10
{
	/// <summary>
	/// An empty page that can be used on its own or navigated to within a Frame.
	/// </summary>
	[Windows::Foundation::Metadata::WebHostHidden]
	public ref class content sealed
	{
	public:
		content();

   private:
      bool m_inGame;
     
      void Add_Clicked(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
      void ItemClick(Platform::Object^ sender, Windows::UI::Xaml::Controls::ItemClickEventArgs^ e);
      void OnBackRequested(Platform::Object ^sender, Windows::UI::Core::BackRequestedEventArgs ^args);

   };
}
