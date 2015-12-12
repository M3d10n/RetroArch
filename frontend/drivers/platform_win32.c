/* RetroArch - A frontend for libretro.
 * Copyright (C) 2011-2015 - Daniel De Matteis
 *
 * RetroArch is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Found-
 * ation, either version 3 of the License, or (at your option) any later version.
 *
 * RetroArch is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE. See the GNU General Public License for more details.
 * * You should have received a copy of the GNU General Public License along with RetroArch.
 * If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include <windows.h>

#include <boolean.h>
#include <retro_miscellaneous.h>
#include <dynamic/dylib.h>
#include <file/file_list.h>
#include <file/file_path.h>

#include "../frontend_driver.h"
#include "../../general.h"
#include "../../verbosity.h"
#include "../../menu/menu.h"

/* We only load this library once, so we let it be 
 * unloaded at application shutdown, since unloading 
 * it early seems to cause issues on some systems.
 */

static dylib_t dwmlib;

static bool dwm_composition_disabled;

static void gfx_dwm_shutdown(void)
{
   if (dwmlib)
      dylib_close(dwmlib);
   dwmlib = NULL;
}

static bool gfx_init_dwm(void)
{
   static bool inited = false;

   if (inited)
      return true;

   dwmlib = dylib_load("dwmapi.dll");
   if (!dwmlib)
   {
      RARCH_LOG("Did not find dwmapi.dll.\n");
      return false;
   }
   atexit(gfx_dwm_shutdown);

   HRESULT (WINAPI *mmcss)(BOOL) = 
      (HRESULT (WINAPI*)(BOOL))dylib_proc(dwmlib, "DwmEnableMMCSS");
   if (mmcss)
   {
      RARCH_LOG("Setting multimedia scheduling for DWM.\n");
      mmcss(TRUE);
   }

   inited = true;
   return true;
}

static void gfx_set_dwm(void)
{
   HRESULT ret;
   settings_t *settings = config_get_ptr();

   if (!gfx_init_dwm())
      return;

   if (settings->video.disable_composition == dwm_composition_disabled)
      return;

   HRESULT (WINAPI *composition_enable)(UINT) = 
      (HRESULT (WINAPI*)(UINT))dylib_proc(dwmlib, "DwmEnableComposition");
   if (!composition_enable)
   {
      RARCH_ERR("Did not find DwmEnableComposition ...\n");
      return;
   }

   ret = composition_enable(!settings->video.disable_composition);
   if (FAILED(ret))
      RARCH_ERR("Failed to set composition state ...\n");
   dwm_composition_disabled = settings->video.disable_composition;
}

static void frontend_win32_get_os(char *s, size_t len, int *major, int *minor)
{
#if defined(WINAPI_FAMILY_PARTITION) && WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP)
	strlcpy(s, "Windows 10", len);
#else
	uint32_t version = GetVersion();

	*major   = (DWORD)(LOBYTE(LOWORD(version)));
	*minor   = (DWORD)(HIBYTE(LOWORD(version)));

   switch (*major)
   {
      case 6:
         switch (*minor)
         {
            case 3:
               strlcpy(s, "Windows 8.1", len);
               break;
            case 2:
               strlcpy(s, "Windows 8", len);
               break;
            case 1:
               strlcpy(s, "Windows 7/2008 R2", len);
               break;
            case 0:
               strlcpy(s, "Windows Vista/2008", len);
               break;
            default:
               break;
         }
         break;
      case 5:
         switch (*minor)
         {
            case 2:
               strlcpy(s, "Windows 2003", len);
               break;
            case 1:
               strlcpy(s, "Windows XP", len);
               break;
            case 0:
               strlcpy(s, "Windows 2000", len);
               break;
         }
         break;
      case 4:
         switch (*minor)
         {
            case 0:
               strlcpy(s, "Windows NT 4.0", len);
               break;
            case 90:
               strlcpy(s, "Windows ME", len);
               break;
            case 10:
               strlcpy(s, "Windows 98", len);
               break;
         }
         break;
      default:
         break;
   }

#endif
}

static void frontend_win32_init(void *data)
{
	typedef BOOL (WINAPI *isProcessDPIAwareProc)();
	typedef BOOL (WINAPI *setProcessDPIAwareProc)();
	HMODULE handle                         = GetModuleHandle(TEXT("User32.dll"));
	isProcessDPIAwareProc  isDPIAwareProc  = (isProcessDPIAwareProc)dylib_proc(handle, "IsProcessDPIAware");
	setProcessDPIAwareProc setDPIAwareProc = (setProcessDPIAwareProc)dylib_proc(handle, "SetProcessDPIAware");

	if (isDPIAwareProc)
	{
		if (!isDPIAwareProc())
		{
			if (setDPIAwareProc)
				setDPIAwareProc();
		}
	}
   
}

enum frontend_powerstate frontend_win32_get_powerstate(int *seconds, int *percent)
{
#if defined(WINAPI_FAMILY_PARTITION) && WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP)
	auto report = Windows::Devices::Power::Battery::AggregateBattery->GetReport();
	enum frontend_powerstate ret = FRONTEND_POWERSTATE_NONE;

	BOOL isUsingAC = (Windows::System::Power::PowerManager::PowerSupplyStatus == Windows::System::Power::PowerSupplyStatus::Adequate);
	
	auto status = report->Status;
	switch (status)
	{
	case Windows::System::Power::BatteryStatus::NotPresent:
		ret = isUsingAC ? FRONTEND_POWERSTATE_ON_POWER_SOURCE : FRONTEND_POWERSTATE_NO_SOURCE;		
		break;
	case Windows::System::Power::BatteryStatus::Idle:
		ret = isUsingAC ? FRONTEND_POWERSTATE_ON_POWER_SOURCE : FRONTEND_POWERSTATE_CHARGED;
		break;
	case Windows::System::Power::BatteryStatus::Charging:
		ret = FRONTEND_POWERSTATE_CHARGING;
		break;
	case Windows::System::Power::BatteryStatus::Discharging:
		ret = FRONTEND_POWERSTATE_NONE;
		break;
	default:
		break;
	}
#else
    SYSTEM_POWER_STATUS status;
	enum frontend_powerstate ret = FRONTEND_POWERSTATE_NONE;

	if (!GetSystemPowerStatus(&status))
		return ret;

	if (status.BatteryFlag == 0xFF)
		ret = FRONTEND_POWERSTATE_NONE;
	if (status.BatteryFlag & (1 << 7))
		ret = FRONTEND_POWERSTATE_NO_SOURCE;
	else if (status.BatteryFlag & (1 << 3))
		ret = FRONTEND_POWERSTATE_CHARGING;
	else if (status.ACLineStatus == 1)
		ret = FRONTEND_POWERSTATE_CHARGED;
	else
		ret = FRONTEND_POWERSTATE_ON_POWER_SOURCE;

	*percent  = (int)status.BatteryLifePercent;
	*seconds  = (int)status.BatteryLifeTime;
#endif

	return ret;
}

enum frontend_architecture frontend_win32_get_architecture(void)
{
   /* stub */
   return FRONTEND_ARCH_NONE;
}

static int frontend_win32_parse_drive_list(void *data)
{
#if defined(WINAPI_FAMILY_PARTITION) && WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP)
	file_list_t *list = (file_list_t*)data;
	menu_entries_push(list,
		"ms-appdata:///local/", "", MENU_FILE_DIRECTORY, 0, 0);
#else
   size_t i = 0;
   unsigned drives = GetLogicalDrives();
   char    drive[] = " :\\";
   file_list_t *list = (file_list_t*)data;

   for (i = 0; i < 32; i++)
   {
      drive[0] = 'A' + i;
      if (drives & (1 << i))
         menu_entries_push(list,
               drive, "", MENU_FILE_DIRECTORY, 0, 0);
   }
#endif

   return 0;
}

static void frontend_win32_environment_get(int *argc, char *argv[],
      void *args, void *params_data)
{
   gfx_set_dwm();

#if defined(WINAPI_FAMILY_PARTITION) && WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP)

   char applocal[1024];
   auto local_folder = Windows::Storage::ApplicationData::Current->LocalFolder->Path;
   pstringtocs(applocal, local_folder, sizeof(applocal));

   char appdata[1024];
   auto roaming_folder = Windows::Storage::ApplicationData::Current->RoamingFolder->Path;
   pstringtocs(appdata, roaming_folder, sizeof(appdata));

   const char* app = "ms-appx:///";
   retro_main_log_file_init("ms-appdata:///local/retroarch-log.txt");
   
   // Store in roaming folder to sync across devices
   fill_pathname_join(g_defaults.path.config, appdata, "retroarch.cfg", sizeof(g_defaults.path.config));
   fill_pathname_join(g_defaults.dir.sram, appdata, "sram", sizeof(g_defaults.dir.sram));
   fill_pathname_join(g_defaults.dir.savestate, appdata, "savestates", sizeof(g_defaults.dir.savestate));
   fill_pathname_join(g_defaults.dir.playlist, appdata, "playlists", sizeof(g_defaults.dir.playlist));

   // Local, no need to roam
   fill_pathname_join(g_defaults.dir.cache, applocal, "cache", sizeof(g_defaults.dir.cache));
   
   // Read-only, comes in the app package
   fill_pathname_join(g_defaults.dir.core, app, "cores", sizeof(g_defaults.dir.core));
   fill_pathname_join(g_defaults.dir.core_info, app, "cores/info", sizeof(g_defaults.dir.core_info));
   fill_pathname_join(g_defaults.dir.core_assets, app, "cores/assets", sizeof(g_defaults.dir.core_assets));
#endif
}

frontend_ctx_driver_t frontend_ctx_win32 = {
   frontend_win32_environment_get,
   frontend_win32_init,
   NULL,                           /* deinit */
   NULL,                           /* exitspawn */
   NULL,                           /* process_args */
   NULL,                           /* exec */
   NULL,                           /* set_fork */
   NULL,                           /* shutdown */
   NULL,                           /* get_name */
   frontend_win32_get_os,
   NULL,                           /* get_rating */
   NULL,                           /* load_content */
   frontend_win32_get_architecture,
   frontend_win32_get_powerstate,
   frontend_win32_parse_drive_list,
   "win32",
};
