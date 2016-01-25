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

         char core[4096]      = { 0 };
         char content[4096]   = { 0 };

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

         global_t *global = global_get_ptr();
         global->perfcnt_enable = true;

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

         // Change the overlay
         UpdateOverlay();
                  
         unsigned sleep_ms = 0;
         int ret = rarch_main_iterate(&sleep_ms);
         if (ret == 1 && sleep_ms > 0)
            retro_sleep(sleep_ms);
         rarch_main_data_iterate();
      }
      
      // Shutdown
      {
         critical_section::scoped_lock lock(m_criticalSection);

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
      }
   });

   // Run task on a dedicated high priority background thread.
   m_updateWorker = ThreadPool::RunAsync(workItemHandler, WorkItemPriority::High, WorkItemOptions::TimeSliced);
   
}

void RetroarchMain::StopUpdateThread(bool wait)
{
   if (m_running && m_updateWorker)
   {
      m_shutdown = true;
      if (wait)
      {
         while (m_updateWorker->Status != AsyncStatus::Completed);
      }
   }
}

bool RetroArch_Win10::RetroarchMain::IsRunning()
{
   return m_running && !m_shutdown;
}

void RetroArch_Win10::RetroarchMain::ChangeOverlay(Platform::String ^ overlay)
{
   if (overlay->Equals(m_overlay))
   {
      return;
   }

   m_overlay = overlay;
   m_changeOverlay = true;
}

void RetroArch_Win10::RetroarchMain::SaveState()
{
   critical_section::scoped_lock lock(m_criticalSection);
   if (m_running && !m_shutdown)
   {
      event_command(EVENT_CMD_AUTOSAVE_STATE);
      event_command(EVENT_CMD_SAVEFILES);
   }
}

void RetroArch_Win10::RetroarchMain::ResetGame()
{
   critical_section::scoped_lock lock(m_criticalSection);
   event_command(EVENT_CMD_RESET);
}

void RetroArch_Win10::RetroarchMain::PauseGame()
{
   critical_section::scoped_lock lock(m_criticalSection);
   event_command(EVENT_CMD_PAUSE);
}

void RetroArch_Win10::RetroarchMain::ResumeGame()
{
   critical_section::scoped_lock lock(m_criticalSection);
   event_command(EVENT_CMD_UNPAUSE);
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

void RetroArch_Win10::RetroarchMain::UpdateOverlay()
{
   if (m_changeOverlay && input_overlay_status() == OVERLAY_STATUS_NONE)
   {
      settings_t *settings = config_get_ptr();
      if (m_overlay->IsEmpty())
      {
         settings->input.overlay_enable = false;
         settings->input.overlay_hide_in_menu = true;
         event_command(EVENT_CMD_OVERLAY_DEINIT);
      }
      else
      {
         std::wstring wide(m_overlay->Begin());
         std::string str(wide.begin(), wide.end());

         settings->input.overlay_enable = true;
         settings->input.overlay_hide_in_menu = false;
         fill_pathname_join(settings->input.overlay, g_defaults.dir.overlay, str.c_str(), sizeof(settings->input.overlay));
         event_command(EVENT_CMD_OVERLAY_INIT);
      }
      m_changeOverlay = false;
   }
}


d3d11::DeviceResources * RetroArch_Win10::GetResources()
{
   return (d3d11::DeviceResources*)video_driver_get_ptr(false);
}

