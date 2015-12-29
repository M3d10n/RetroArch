//
// main_page.xaml.h
// Declaration of the main_page class
//

#pragma once

#include "ui/drivers/uwp/main_page.g.h"
#include "uwp.h"

namespace RetroArch_Win10
{
	/// <summary>
	/// An empty page that can be used on its own or navigated to within a Frame.
	/// </summary>
	[Windows::Foundation::Metadata::WebHostHidden]
	public ref class main_page sealed
	{
	public:
		main_page();

   private:
      std::unique_ptr<RetroarchMain> m_main;
      void OnSizeChanged(Platform::Object ^sender, Windows::UI::Xaml::SizeChangedEventArgs ^e);
      void OnCompositionScaleChanged(Windows::UI::Xaml::Controls::SwapChainPanel ^sender, Platform::Object ^args);
   };
}
