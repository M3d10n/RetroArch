//
// cores.xaml.h
// Declaration of the cores class
//

#pragma once

#include "cores.h"
#include "ui/drivers/uwp/cores.g.h"

namespace RetroArch_Win10
{
	/// <summary>
	/// An empty page that can be used on its own or navigated to within a Frame.
	/// </summary>
	[Windows::Foundation::Metadata::WebHostHidden]
	public ref class cores sealed
	{
	public:
		cores();

      virtual void OnNavigatedTo(Windows::UI::Xaml::Navigation::NavigationEventArgs^ e) override;
      virtual void OnNavigatedFrom(Windows::UI::Xaml::Navigation::NavigationEventArgs^ e) override;
   private:
      void Item_Clicked(Platform::Object^ sender, Windows::UI::Xaml::Controls::ItemClickEventArgs^ e);
   };
}
