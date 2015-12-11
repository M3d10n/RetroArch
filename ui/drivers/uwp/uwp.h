#pragma once

#include "gfx/common/d3d11_common.h"

namespace Retroarch
{
	static d3d11::DeviceResources* GetResources();

	class RetroarchMain : public d3d11::IDeviceNotify
	{
	public:
		RetroarchMain(Platform::String^ entryPoint);
		~RetroarchMain();
		void CreateWindowSizeDependentResources();
		void Update();

		void StartUpdateThread();
		void StopUpdateThread();

		Concurrency::critical_section& GetCriticalSection() { return m_criticalSection; }
		Platform::String^ GetEntryPoint() { return m_entryPoint; }
		
		Windows::UI::Core::CoreWindow^ GetWindow() { return m_window.Get();  }
		void SetWindow(Windows::UI::Core::CoreWindow^ Window);

		Windows::Graphics::Display::DisplayInformation^ GetDisplayInformation() { return m_displayInformation; }
		void SetDisplayInformation(Windows::Graphics::Display::DisplayInformation^ DisplayInformation);

		// IDeviceNotify
		virtual void OnDeviceLost();
		virtual void OnDeviceRestored();

	private:
		Platform::Agile<Windows::UI::Core::CoreWindow> m_window;
		Windows::Graphics::Display::DisplayInformation^	m_displayInformation;

		void* m_updateThread;
		Platform::String^ m_entryPoint;


		Concurrency::critical_section m_criticalSection;
	};

	// Main entry point for our app. Connects the app with the Windows shell and handles application lifecycle events.
	ref class App sealed : public Windows::ApplicationModel::Core::IFrameworkView
	{
	public:
		App();

		// IFrameworkView Methods.
		virtual void Initialize(Windows::ApplicationModel::Core::CoreApplicationView^ applicationView);
		virtual void SetWindow(Windows::UI::Core::CoreWindow^ window);
		virtual void Load(Platform::String^ entryPoint);
		virtual void Run();
		virtual void Uninitialize();

	protected:
		// Application lifecycle event handlers.
		void OnActivated(Windows::ApplicationModel::Core::CoreApplicationView^ applicationView, Windows::ApplicationModel::Activation::IActivatedEventArgs^ args);
		void OnSuspending(Platform::Object^ sender, Windows::ApplicationModel::SuspendingEventArgs^ args);
		void OnResuming(Platform::Object^ sender, Platform::Object^ args);

		// Window event handlers.
		void OnWindowSizeChanged(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::WindowSizeChangedEventArgs^ args);
		void OnVisibilityChanged(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::VisibilityChangedEventArgs^ args);
		void OnWindowClosed(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::CoreWindowEventArgs^ args);

		// DisplayInformation event handlers.
		void OnDpiChanged(Windows::Graphics::Display::DisplayInformation^ sender, Platform::Object^ args);
		void OnOrientationChanged(Windows::Graphics::Display::DisplayInformation^ sender, Platform::Object^ args);
		void OnDisplayContentsInvalidated(Windows::Graphics::Display::DisplayInformation^ sender, Platform::Object^ args);

	private:
		Platform::Agile<Windows::UI::Core::CoreWindow> m_window;
		std::unique_ptr<RetroarchMain> m_main;
		bool m_windowClosed;
		bool m_windowVisible;
	};
}

ref class Direct3DApplicationSource sealed : Windows::ApplicationModel::Core::IFrameworkViewSource
{
public:
	virtual Windows::ApplicationModel::Core::IFrameworkView^ CreateView();
};
