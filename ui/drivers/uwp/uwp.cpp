﻿#include "uwp.h"
#include "frontend/frontend.h"
#include "runloop.h"
#include "runloop_data.h"
#include "gfx/video_driver.h"
#include "general.h"

#include <file/file_path.h>
#include <ppltasks.h>

using namespace RetroArch_Win10;

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
/*[Platform::MTAThread]
int main(Platform::Array<Platform::String^>^)
{
   auto direct3DApplicationSource = ref new Direct3DApplicationSource();
   CoreApplication::Run(direct3DApplicationSource);
   return 0;
}
*/

IFrameworkView^ Direct3DApplicationSource::CreateView()
{
   return ref new OldApp();
}

OldApp::OldApp() :
   m_windowVisible(true)
{
}

// The first method called when the IFrameworkView is being created.
void OldApp::Initialize(CoreApplicationView^ applicationView)
{
   // Register event handlers for app lifecycle. This example includes Activated, so that we
   // can make the CoreWindow active and start rendering on the window.
   applicationView->Activated +=
      ref new TypedEventHandler<CoreApplicationView^, IActivatedEventArgs^>(this, &OldApp::OnActivated);

   CoreApplication::Suspending +=
      ref new EventHandler<SuspendingEventArgs^>(this, &OldApp::OnSuspending);

   CoreApplication::Resuming +=
      ref new EventHandler<Platform::Object^>(this, &OldApp::OnResuming);

   // At this point we have access to the device. 
   // We can create the device-dependent resources.
   //m_deviceResources = std::make_shared<d3d11::DeviceResources>();
}

// Called when the CoreWindow object is created (or re-created).
void OldApp::SetWindow(CoreWindow^ window)
{
   window->SizeChanged += 
      ref new TypedEventHandler<CoreWindow^, WindowSizeChangedEventArgs^>(this, &OldApp::OnWindowSizeChanged);

   window->VisibilityChanged +=
      ref new TypedEventHandler<CoreWindow^, VisibilityChangedEventArgs^>(this, &OldApp::OnVisibilityChanged);

   window->Closed += 
      ref new TypedEventHandler<CoreWindow^, CoreWindowEventArgs^>(this, &OldApp::OnWindowClosed);

   DisplayInformation^ currentDisplayInformation = DisplayInformation::GetForCurrentView();

   currentDisplayInformation->DpiChanged +=
      ref new TypedEventHandler<DisplayInformation^, Object^>(this, &OldApp::OnDpiChanged);

   currentDisplayInformation->OrientationChanged +=
      ref new TypedEventHandler<DisplayInformation^, Object^>(this, &OldApp::OnOrientationChanged);

   DisplayInformation::DisplayContentsInvalidated +=
      ref new TypedEventHandler<DisplayInformation^, Object^>(this, &OldApp::OnDisplayContentsInvalidated);

}

// Initializes scene resources, or loads a previously saved app state.
void OldApp::Load(Platform::String^ entryPoint)
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
void OldApp::Run()
{
   m_main->StartUpdateThread();
   CoreWindow::GetForCurrentThread()->Dispatcher->ProcessEvents(CoreProcessEventsOption::ProcessUntilQuit);
}


// Required for IFrameworkView.
// Terminate events do not cause Uninitialize to be called. It will be called if your IFrameworkView
// class is torn down while the app is in the foreground.
void OldApp::Uninitialize()
{
}

// Application lifecycle event handlers.

void OldApp::OnActivated(CoreApplicationView^ applicationView, IActivatedEventArgs^ args)
{
   // Run() won't start until the CoreWindow is activated.
   applicationView->CoreWindow->Activate();
}

void OldApp::OnSuspending(Platform::Object^ sender, SuspendingEventArgs^ args)
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

void OldApp::OnResuming(Platform::Object^ sender, Platform::Object^ args)
{
   // Restore any data or state that was unloaded on suspend. By default, data
   // and state are persisted when resuming from suspend. Note that this event
   // does not occur if the app was previously terminated.

   // Insert your code here.
   m_main->StartUpdateThread();
}

// Window event handlers.

void OldApp::OnWindowSizeChanged(CoreWindow^ sender, WindowSizeChangedEventArgs^ args)
{
   if (!m_main->IsInitialized())
   {
      return;
   }
   critical_section::scoped_lock lock(m_main->GetCriticalSection());
   GetResources()->SetLogicalSize(Size(sender->Bounds.Width, sender->Bounds.Height));
}

void OldApp::OnVisibilityChanged(CoreWindow^ sender, VisibilityChangedEventArgs^ args)
{
   m_windowVisible = args->Visible;
}

void OldApp::OnWindowClosed(CoreWindow^ sender, CoreWindowEventArgs^ args)
{
}

// DisplayInformation event handlers.

void OldApp::OnDpiChanged(DisplayInformation^ sender, Object^ args)
{
   if (!m_main->IsInitialized())
   {
      return;
   }

   critical_section::scoped_lock lock(m_main->GetCriticalSection());
   GetResources()->SetDpi(sender->LogicalDpi);
}

void OldApp::OnOrientationChanged(DisplayInformation^ sender, Object^ args)
{
   return;
   if (!m_main->IsInitialized())
   {
      return;
   }

   critical_section::scoped_lock lock(m_main->GetCriticalSection());
   GetResources()->SetCurrentOrientation(sender->CurrentOrientation);
}

void OldApp::OnDisplayContentsInvalidated(DisplayInformation^ sender, Object^ args)
{
   if (!m_main->IsInitialized())
   {
      return;
   }

   critical_section::scoped_lock lock(m_main->GetCriticalSection());
   GetResources()->ValidateDevice();
}



// Loads and initializes application assets when the application is loaded.
RetroarchMain::RetroarchMain(Platform::String^ entryPoint) :
   m_entryPoint(entryPoint),
   m_initialized(false)
{	
}

RetroarchMain::~RetroarchMain()
{
   // Deregister device notification
   auto resources = GetResources();
   if (resources)
      resources->RegisterDeviceNotify(nullptr);
}

void RetroarchMain::StartUpdateThread()
{
   // Already running
   if (m_updateWorker && m_updateWorker->Status == AsyncStatus::Started)
   {
      return;
   }

   auto workItemHandler = ref new WorkItemHandler([this](IAsyncAction^)
   {
      // Initialize only once
      if (!m_initialized)
      {
         critical_section::scoped_lock lock(m_criticalSection);
         critical_section::scoped_lock init_loc(m_initCriticalSection);
         
         // Convert the entry point from wchar* to char*
         size_t buffer_size = m_entryPoint->Length() + 1;
         char * args = (char *)malloc(buffer_size);
         size_t i;
         wcstombs_s(&i, args, buffer_size, m_entryPoint->Data(), buffer_size);
         
         
         // Initialize
         rarch_main(1, &args, NULL);

         // Override the core folder based on the current architecture
         settings_t *settings = config_get_ptr();         
#if defined(_ARM_)
         fill_pathname_join(settings->libretro_directory, g_defaults.dir.core, "win_ARM", sizeof(settings->libretro_directory));
#elif defined(_X86_)
         fill_pathname_join(settings->libretro_directory, g_defaults.dir.core, "win_x86", sizeof(settings->libretro_directory));
#elif defined(_AMD64_)
         fill_pathname_join(settings->libretro_directory, g_defaults.dir.core, "win_x64", sizeof(settings->libretro_directory));
#endif

         // If there's no keyboard or gamepad attached, always show the overlay
         auto keyboardCaps = ref new Windows::Devices::Input::KeyboardCapabilities();
         auto touchCaps = ref new Windows::Devices::Input::TouchCapabilities();
         if (settings->input.overlay_hide_in_menu && keyboardCaps->KeyboardPresent == 0 && touchCaps->TouchPresent > 0)
         {
            settings->input.overlay_enable = true;
            settings->input.overlay_hide_in_menu = false;    
            fill_pathname_join(settings->input.overlay, g_defaults.dir.overlay, "gamepads/flat/retropad-fast.cfg", sizeof(settings->input.overlay));
            event_command(EVENT_CMD_OVERLAY_INIT);
         }

         // Free the arguments
         free(args);

         m_initialized = true;
      }

      // Update
      while (true)
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
      }
   });

   // Run task on a dedicated high priority background thread.
   m_updateWorker = ThreadPool::RunAsync(workItemHandler, WorkItemPriority::High, WorkItemOptions::TimeSliced);
   
}

void RetroarchMain::StopUpdateThread()
{
   //critical_section::scoped_lock lock(m_criticalSection);
   event_command(EVENT_CMD_MENU_SAVE_CURRENT_CONFIG);
   m_updateWorker->Cancel();
}

bool RetroarchMain::IsInitialized()
{
   if (!m_initCriticalSection.try_lock())
   {
      return false;
   }
   m_initCriticalSection.unlock();
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
}


d3d11::DeviceResources * RetroArch_Win10::GetResources()
{
   return (d3d11::DeviceResources*)video_driver_get_ptr(false);
}

