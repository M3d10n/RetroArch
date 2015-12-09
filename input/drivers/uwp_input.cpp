/*  RetroArch - A frontend for libretro.
 *  Copyright (C) 2010-2014 - Hans-Kristian Arntzen
 *  Copyright (C) 2011-2015 - Daniel De Matteis
 * 
 *  RetroArch is free software: you can redistribute it and/or modify it under the terms
 *  of the GNU General Public License as published by the Free Software Found-
 *  ation, either version 3 of the License, or (at your option) any later version.
 *
 *  RetroArch is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with RetroArch.
 *  If not, see <http://www.gnu.org/licenses/>.
 */

#include "gfx/common/d3d11_common.h"


#include "../../general.h"
#include "../../driver.h"
#include "../input_keymaps.h"

using namespace Windows::Foundation;
using namespace Windows::UI::Core;

static void OnKeyEvent(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::KeyEventArgs^ args)
{
	bool is_down = !args->KeyStatus.IsKeyReleased;

	input_keyboard_event(is_down, (unsigned int)args->VirtualKey, args->KeyStatus.ScanCode, 0,
		RETRO_DEVICE_KEYBOARD);
}

static void *uwp_input_input_init(void)
{
   RARCH_ERR("Using the null input driver. RetroArch will ignore you.");

   input_keymaps_init_keyboard_lut(rarch_key_map_uwp);

   auto window = Windows::ApplicationModel::Core::CoreApplication::MainView->CoreWindow;
   if (window)
   {
	   window->KeyDown += ref new TypedEventHandler<CoreWindow^, KeyEventArgs^>(&OnKeyEvent);
	   window->KeyUp += ref new TypedEventHandler<CoreWindow^, KeyEventArgs^>(&OnKeyEvent);
   }

   return (void*)-1;
}

static void uwp_input_input_poll(void *data)
{
   (void)data;
}

static bool uwp_input_keyboard_pressed(unsigned key)
{
	settings_t *settings = config_get_ptr();

	key = settings->input.binds[0][key].key;

	if (key >= RETROK_LAST)
		return false;

	Windows::System::VirtualKey tk = (Windows::System::VirtualKey)input_keymaps_translate_rk_to_keysym((enum retro_key)key);

	auto window = Windows::ApplicationModel::Core::CoreApplication::MainView->CoreWindow;
	if (window)
	{
		bool isDown = (bool)(window->GetKeyState(tk) & Windows::UI::Core::CoreVirtualKeyStates::Down);
		if (isDown)
		{
			return true;
		} 
	}

	return false;
}

static int16_t uwp_input_input_state(void *data,
      const struct retro_keybind **retro_keybinds, unsigned port,
      unsigned device, unsigned idx, unsigned id)
{
   settings_t *settings = config_get_ptr();

   switch (device)
   {
   case RETRO_DEVICE_JOYPAD:
	   return 0;

   case RETRO_DEVICE_KEYBOARD:
	   return uwp_input_keyboard_pressed(id);

   case RETRO_DEVICE_ANALOG:
	   return 0;

   case RETRO_DEVICE_MOUSE:
	   return 0;

   case RARCH_DEVICE_MOUSE_SCREEN:
	   return 0;

   case RETRO_DEVICE_POINTER:
   case RARCH_DEVICE_POINTER_SCREEN:
	   return 0;

   case RETRO_DEVICE_LIGHTGUN:
	   return 0;
   }

   return 0;
}

static bool uwp_input_input_key_pressed(void *data, int key)
{
	return uwp_input_keyboard_pressed(key);
}

static bool uwp_input_input_meta_key_pressed(void *data, int key)
{
   (void)data;
   (void)key;

   return false;
}

static void uwp_input_input_free_input(void *data)
{
   (void)data;
}

static uint64_t uwp_input_get_capabilities(void *data)
{
   uint64_t caps = 0;

   caps |= (1 << RETRO_DEVICE_JOYPAD);
   caps |= (1 << RETRO_DEVICE_KEYBOARD);

   return caps;
}

static bool uwp_input_set_sensor_state(void *data,
      unsigned port, enum retro_sensor_action action, unsigned event_rate)
{
   return false;
}

static void uwp_input_grab_mouse(void *data, bool state)
{
   (void)data;
   (void)state;
}

static bool uwp_input_set_rumble(void *data, unsigned port,
      enum retro_rumble_effect effect, uint16_t strength)
{
   (void)data;
   (void)port;
   (void)effect;
   (void)strength;

   return false;
}

input_driver_t input_uwp = {
   uwp_input_input_init,
   uwp_input_input_poll,
   uwp_input_input_state,
   uwp_input_input_key_pressed,
   uwp_input_input_meta_key_pressed,
   uwp_input_input_free_input,
   uwp_input_set_sensor_state,
   NULL,
   uwp_input_get_capabilities,
   "uwp",
   uwp_input_grab_mouse,
   NULL,
   uwp_input_set_rumble,
   NULL,
   NULL,
   NULL,
};
