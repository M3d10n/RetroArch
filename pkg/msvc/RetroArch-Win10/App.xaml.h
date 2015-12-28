//
// App.xaml.h
// Declaration of the App class.
//

#pragma once

#include "App.g.h"
#include "gfx/common/d3d11_common.h"
#include "ui/drivers/uwp/main_page.xaml.h"

namespace RetroArch_Win10
{
		/// <summary>
	/// Provides application-specific behavior to supplement the default Application class.
	/// </summary>
	ref class App sealed
	{
	public:
		App();
		virtual void OnLaunched(Windows::ApplicationModel::Activation::LaunchActivatedEventArgs^ e) override;

	private:
		void OnSuspending(Platform::Object^ sender, Windows::ApplicationModel::SuspendingEventArgs^ e);
		void OnResuming(Platform::Object ^sender, Platform::Object ^args);
		main_page^ m_mainPage;
	};
}
