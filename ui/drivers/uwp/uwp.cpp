#include "uwp.h"
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

// Loads and initializes application assets when the application is loaded.
RetroarchMain::RetroarchMain(Platform::String^ core, Platform::String^ content) :
   m_entryPoint(""),
   m_core(core),
   m_content(content),
   m_initialized(false),
   m_running(false),
   m_shutdown(false)
{	
}

std::unique_ptr<RetroarchMain> RetroarchMain::Instance;


RetroarchMain::~RetroarchMain()
{
   
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

         char core[4096] = { 0 }; //"bin/win_x86/genesis_plus_gx_libretro.dll"
         char content[4096] = { 0 }; //"bin/sonic2.smd"

         unsigned int argc = 1;
         if (m_core && !m_core->IsEmpty())
         {
            wcstombs_s(NULL, core, sizeof(core), m_core->Data(), sizeof(core));
            argc += 2;
         }
         if (m_content && !m_content->IsEmpty())
         {
            wcstombs_s(NULL, content, sizeof(content), m_content->Data(), sizeof(content));
            argc++;
         }

         char* arg_l[] = {
            "",
            "--libretro",
            core,
            content,
            NULL
         };
         
         // Initialize
         rarch_main(argc, arg_l, NULL);

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

         m_initialized = true;
         m_running = true;
      }

      // Update
      while (!m_shutdown)
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

      // Clear
      auto d2dctx = GetResources()->GetD2DDeviceContext();
      d2dctx->BeginDraw();
      d2dctx->Clear();
      d2dctx->EndDraw();
      GetResources()->Present();

      // Shutdown
      main_exit(NULL);

      m_initialized = false;
      m_running = false;
   });

   // Run task on a dedicated high priority background thread.
   m_updateWorker = ThreadPool::RunAsync(workItemHandler, WorkItemPriority::High, WorkItemOptions::TimeSliced);
   
}

void RetroarchMain::StopUpdateThread()
{
   //critical_section::scoped_lock lock(m_criticalSection);
   //event_command(EVENT_CMD_MENU_SAVE_CURRENT_CONFIG);
   m_shutdown = true;
   while (m_updateWorker->Status != AsyncStatus::Completed);
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

