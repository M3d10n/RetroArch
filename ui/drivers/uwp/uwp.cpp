#include "uwp.h"
#include "frontend/frontend.h"
#include "runloop.h"
#include "runloop_data.h"
#include "gfx/video_driver.h"

#include <ppltasks.h>

using namespace Retroarch;

using namespace concurrency;
using namespace Windows::ApplicationModel;
using namespace Windows::ApplicationModel::Core;
using namespace Windows::ApplicationModel::Activation;
using namespace Windows::UI::Core;
using namespace Windows::UI::Input;
using namespace Windows::System;
using namespace Windows::Foundation;
using namespace Windows::Graphics::Display;
using namespace Windows::System::Threading;

// The main function is only used to initialize our IFrameworkView class.
[Platform::MTAThread]
int main(Platform::Array<Platform::String^>^)
{
	auto direct3DApplicationSource = ref new Direct3DApplicationSource();
	CoreApplication::Run(direct3DApplicationSource);
	return 0;
}

IFrameworkView^ Direct3DApplicationSource::CreateView()
{
	return ref new App();
}

App::App() :
	m_windowClosed(false),
	m_windowVisible(true)
{
}

// The first method called when the IFrameworkView is being created.
void App::Initialize(CoreApplicationView^ applicationView)
{
	// Register event handlers for app lifecycle. This example includes Activated, so that we
	// can make the CoreWindow active and start rendering on the window.
	applicationView->Activated +=
		ref new TypedEventHandler<CoreApplicationView^, IActivatedEventArgs^>(this, &App::OnActivated);

	CoreApplication::Suspending +=
		ref new EventHandler<SuspendingEventArgs^>(this, &App::OnSuspending);

	CoreApplication::Resuming +=
		ref new EventHandler<Platform::Object^>(this, &App::OnResuming);

	// At this point we have access to the device. 
	// We can create the device-dependent resources.
	//m_deviceResources = std::make_shared<d3d11::DeviceResources>();
}

// Called when the CoreWindow object is created (or re-created).
void App::SetWindow(CoreWindow^ window)
{
	window->SizeChanged += 
		ref new TypedEventHandler<CoreWindow^, WindowSizeChangedEventArgs^>(this, &App::OnWindowSizeChanged);

	window->VisibilityChanged +=
		ref new TypedEventHandler<CoreWindow^, VisibilityChangedEventArgs^>(this, &App::OnVisibilityChanged);

	window->Closed += 
		ref new TypedEventHandler<CoreWindow^, CoreWindowEventArgs^>(this, &App::OnWindowClosed);

	DisplayInformation^ currentDisplayInformation = DisplayInformation::GetForCurrentView();

	currentDisplayInformation->DpiChanged +=
		ref new TypedEventHandler<DisplayInformation^, Object^>(this, &App::OnDpiChanged);

	currentDisplayInformation->OrientationChanged +=
		ref new TypedEventHandler<DisplayInformation^, Object^>(this, &App::OnOrientationChanged);

	DisplayInformation::DisplayContentsInvalidated +=
		ref new TypedEventHandler<DisplayInformation^, Object^>(this, &App::OnDisplayContentsInvalidated);

}

// Initializes scene resources, or loads a previously saved app state.
void App::Load(Platform::String^ entryPoint)
{
	if (m_main == nullptr)
	{
		// Create our "main"
		m_main = std::unique_ptr<RetroarchMain>(new RetroarchMain(entryPoint));

		// Set the UI dispatcher
		d3d11::ui_dispatcher = CoreWindow::GetForCurrentThread()->Dispatcher;
	}
}

// This method is called after the window becomes active.
void App::Run()
{
	m_main->StartUpdateThread();
	
	CoreWindow::GetForCurrentThread()->Dispatcher->ProcessEvents(CoreProcessEventsOption::ProcessUntilQuit);
}

// Required for IFrameworkView.
// Terminate events do not cause Uninitialize to be called. It will be called if your IFrameworkView
// class is torn down while the app is in the foreground.
void App::Uninitialize()
{
}

// Application lifecycle event handlers.

void App::OnActivated(CoreApplicationView^ applicationView, IActivatedEventArgs^ args)
{
	// Run() won't start until the CoreWindow is activated.
	applicationView->CoreWindow->Activate();
}

void App::OnSuspending(Platform::Object^ sender, SuspendingEventArgs^ args)
{
	// Save app state asynchronously after requesting a deferral. Holding a deferral
	// indicates that the application is busy performing suspending operations. Be
	// aware that a deferral may not be held indefinitely. After about five seconds,
	// the app will be forced to exit.
	SuspendingDeferral^ deferral = args->SuspendingOperation->GetDeferral();

	create_task([this, deferral]()
	{
		critical_section::scoped_lock lock(m_main->GetCriticalSection());
		GetResources()->Trim();

		m_main->StopUpdateThread();

		deferral->Complete();
	});
}

void App::OnResuming(Platform::Object^ sender, Platform::Object^ args)
{
	// Restore any data or state that was unloaded on suspend. By default, data
	// and state are persisted when resuming from suspend. Note that this event
	// does not occur if the app was previously terminated.

	// Insert your code here.
	m_main->StartUpdateThread();
}

// Window event handlers.

void App::OnWindowSizeChanged(CoreWindow^ sender, WindowSizeChangedEventArgs^ args)
{
	critical_section::scoped_lock lock(m_main->GetCriticalSection());
	GetResources()->SetLogicalSize(Size(sender->Bounds.Width, sender->Bounds.Height));
}

void App::OnVisibilityChanged(CoreWindow^ sender, VisibilityChangedEventArgs^ args)
{
	m_windowVisible = args->Visible;
}

void App::OnWindowClosed(CoreWindow^ sender, CoreWindowEventArgs^ args)
{
	m_windowClosed = true;
}

// DisplayInformation event handlers.

void App::OnDpiChanged(DisplayInformation^ sender, Object^ args)
{
	critical_section::scoped_lock lock(m_main->GetCriticalSection());
	GetResources()->SetDpi(sender->LogicalDpi);
}

void App::OnOrientationChanged(DisplayInformation^ sender, Object^ args)
{
	critical_section::scoped_lock lock(m_main->GetCriticalSection());
	GetResources()->SetCurrentOrientation(sender->CurrentOrientation);
}

void App::OnDisplayContentsInvalidated(DisplayInformation^ sender, Object^ args)
{
	critical_section::scoped_lock lock(m_main->GetCriticalSection());
	GetResources()->ValidateDevice();
}



// Loads and initializes application assets when the application is loaded.
RetroarchMain::RetroarchMain(Platform::String^ entryPoint)
{	
	m_entryPoint = entryPoint;
}

RetroarchMain::~RetroarchMain()
{
	// Deregister device notification
	auto resources = GetResources();
	if (resources)
		resources->RegisterDeviceNotify(nullptr);
}

// Updates the application state once per frame.
void RetroarchMain::Update()
{	
	critical_section::scoped_lock lock(m_criticalSection);

	// This needs to be done on every update in case the driver changes
	auto resources = GetResources();
	if (resources)
	{
		resources->RegisterDeviceNotify(this);
	}

	unsigned sleep_ms = 0;
	int ret = rarch_main_iterate(&sleep_ms);
	if (ret == 1 && sleep_ms > 0)
		retro_sleep(sleep_ms);
	rarch_main_data_iterate();
	if (ret != -1)
		return; // TODO: quit the app?	
}

static DWORD WINAPI UpdateThreadFunc(void *data)
{
	RetroarchMain* main = (RetroarchMain*)data;

	{
		critical_section::scoped_lock lock(main->GetCriticalSection());

		// Convert the entry point from wchar* to char*
		size_t buffer_size = main->GetEntryPoint()->Length() + 1;
		char * args = (char *)malloc(buffer_size);
		size_t i;
		wcstombs_s(&i, args, buffer_size, main->GetEntryPoint()->Data(), buffer_size);

		// Initialize
		rarch_main(1, &args, NULL);

		auto async = d3d11::ui_dispatcher->RunAsync(CoreDispatcherPriority::Normal, ref new DispatchedHandler([=]() 
		{
			GetResources()->SetWindow(CoreWindow::GetForCurrentThread(), DisplayInformation::GetForCurrentView());
		}, Platform::CallbackContext::Any));
		while (async->Status != AsyncStatus::Completed);

		// Free the arguments
		free(args);
	}

	while (true)
	{
		main->Update();
	}
}


void RetroarchMain::StartUpdateThread()
{
	if (!m_updateThread)
	{
		m_updateThread = CreateThread(NULL, 0, UpdateThreadFunc, this, 0, NULL);
	}
}

void RetroarchMain::StopUpdateThread()
{
	//m_renderLoopWorker->Cancel();
}

// Notifies renderers that device resources need to be released.
void RetroarchMain::OnDeviceLost()
{
	//m_sceneRenderer->ReleaseDeviceDependentResources();
	//m_fpsTextRenderer->ReleaseDeviceDependentResources();
}

// Notifies renderers that device resources may now be recreated.
void RetroarchMain::OnDeviceRestored()
{
	//m_sceneRenderer->CreateDeviceDependentResources();
	//m_fpsTextRenderer->CreateDeviceDependentResources();
}

d3d11::DeviceResources * Retroarch::GetResources()
{
	return (d3d11::DeviceResources*)video_driver_get_ptr(false);
}
