﻿#include "uwp.h"
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

	m_window = window;
	if (m_deviceResources.get() != NULL)
	{
		m_deviceResources->SetWindow(window);
	}
}

// Initializes scene resources, or loads a previously saved app state.
void App::Load(Platform::String^ entryPoint)
{
	if (m_main == nullptr)
	{

		// Convert the entry point from wchar* to char*
		size_t buffer_size = entryPoint->Length() + 1;
		char * args = (char *)malloc(buffer_size);
		size_t i;
		wcstombs_s(&i, args, buffer_size, entryPoint->Data(), buffer_size);

		// Initialize
		rarch_main(1, &args, NULL);

		// Free the arguments
		free(args);

		// Set the video driver window
		m_deviceResources.reset((d3d11::DeviceResources*)video_driver_get_ptr(false));
		if (m_deviceResources.get() != NULL)
		{
			m_main = std::unique_ptr<RetroarchMain>(new RetroarchMain(m_deviceResources));
			m_deviceResources->SetWindow(m_window.Get());
		}
	}
}

// This method is called after the window becomes active.
void App::Run()
{
	while (!m_windowClosed)
	{
		if (m_windowVisible)
		{
			CoreWindow::GetForCurrentThread()->Dispatcher->ProcessEvents(CoreProcessEventsOption::ProcessAllIfPresent);

			m_main->Update();

			/*
			if (m_main->Render())
			{
				m_deviceResources->Present();
			}
			*/
		}
		else
		{
			CoreWindow::GetForCurrentThread()->Dispatcher->ProcessEvents(CoreProcessEventsOption::ProcessOneAndAllPending);
		}
	}
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
	CoreWindow::GetForCurrentThread()->Activate();
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
        m_deviceResources->Trim();

		// Insert your code here.

		deferral->Complete();
	});
}

void App::OnResuming(Platform::Object^ sender, Platform::Object^ args)
{
	// Restore any data or state that was unloaded on suspend. By default, data
	// and state are persisted when resuming from suspend. Note that this event
	// does not occur if the app was previously terminated.

	// Insert your code here.
}

// Window event handlers.

void App::OnWindowSizeChanged(CoreWindow^ sender, WindowSizeChangedEventArgs^ args)
{
	m_deviceResources->SetLogicalSize(Size(sender->Bounds.Width, sender->Bounds.Height));
	m_main->CreateWindowSizeDependentResources();
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
	m_deviceResources->SetDpi(sender->LogicalDpi);
	m_main->CreateWindowSizeDependentResources();
}

void App::OnOrientationChanged(DisplayInformation^ sender, Object^ args)
{
	m_deviceResources->SetCurrentOrientation(sender->CurrentOrientation);
	m_main->CreateWindowSizeDependentResources();
}

void App::OnDisplayContentsInvalidated(DisplayInformation^ sender, Object^ args)
{
	m_deviceResources->ValidateDevice();
}



// Loads and initializes application assets when the application is loaded.
RetroarchMain::RetroarchMain(const std::shared_ptr<d3d11::DeviceResources>& deviceResources) :
	m_deviceResources(deviceResources)
{
	// Register to be notified if the Device is lost or recreated
	m_deviceResources->RegisterDeviceNotify(this);
		
	// TODO: Replace this with your app's content initialization.
	//m_sceneRenderer = std::unique_ptr<Sample3DSceneRenderer>(new Sample3DSceneRenderer(m_deviceResources));

	//m_fpsTextRenderer = std::unique_ptr<SampleFpsTextRenderer>(new SampleFpsTextRenderer(m_deviceResources));

	// TODO: Change the timer settings if you want something other than the default variable timestep mode.
	// e.g. for 60 FPS fixed timestep update logic, call:
	/*
	m_timer.SetFixedTimeStep(true);
	m_timer.SetTargetElapsedSeconds(1.0 / 60);
	*/
}

RetroarchMain::~RetroarchMain()
{
	// Deregister device notification
	m_deviceResources->RegisterDeviceNotify(nullptr);
}

// Updates application state when the window size changes (e.g. device orientation change)
void RetroarchMain::CreateWindowSizeDependentResources()
{
	// TODO: Replace this with the size-dependent initialization of your app's content.
	//m_sceneRenderer->CreateWindowSizeDependentResources();
}

// Updates the application state once per frame.
void RetroarchMain::Update()
{	
	unsigned sleep_ms = 0;
	int ret = rarch_main_iterate(&sleep_ms);
	if (ret == 1 && sleep_ms > 0)
		retro_sleep(sleep_ms);
	rarch_main_data_iterate();
	if (ret != -1)
		return; // TODO: quit the app?	
}

// Renders the current frame according to the current application state.
// Returns true if the frame was rendered and is ready to be displayed.
bool RetroarchMain::Render()
{
	auto context = m_deviceResources->GetD3DDeviceContext();

	// Reset the viewport to target the whole screen.
	auto viewport = m_deviceResources->GetScreenViewport();
	context->RSSetViewports(1, &viewport);

	// Reset render targets to the screen.
	ID3D11RenderTargetView *const targets[1] = { m_deviceResources->GetBackBufferRenderTargetView() };
	context->OMSetRenderTargets(1, targets, m_deviceResources->GetDepthStencilView());

	// Clear the back buffer and depth stencil view.
	context->ClearRenderTargetView(m_deviceResources->GetBackBufferRenderTargetView(), DirectX::Colors::CornflowerBlue);
	context->ClearDepthStencilView(m_deviceResources->GetDepthStencilView(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	// Render the scene objects.
	// TODO: Replace this with your app's content rendering functions.
	//m_sceneRenderer->Render();
	//m_fpsTextRenderer->Render();

	return true;
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
	CreateWindowSizeDependentResources();
}
