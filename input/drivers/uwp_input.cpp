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
#include "../input_common.h"

using namespace Windows::Foundation;
using namespace Windows::UI::Core;
using namespace concurrency;

#define MAX_TOUCH 16

struct key_state_t {
   uint8 state;
   unsigned int scan_code;
};

struct input_pointer
{
   int16_t x, y;
   int16_t full_x, full_y;
   unsigned int id;
};

struct key_states_t {
   key_state_t previous[256];
   key_state_t A[256];
   key_state_t B[256];
   key_state_t* current;
   input_pointer pointer[MAX_TOUCH];
   unsigned int curr_pointer;
} key_states;
Concurrency::critical_section input_critical_section;

static void OnKeyEvent(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::KeyEventArgs^ args)
{
   critical_section::scoped_lock lock(input_critical_section);

	unsigned int key = (unsigned int)args->VirtualKey;
	key_states.current[key].state = !args->KeyStatus.IsKeyReleased;
   key_states.current[key].scan_code = args->KeyStatus.ScanCode;
}

void OnPointerPressed(CoreWindow ^sender, PointerEventArgs ^args)
{
   critical_section::scoped_lock lock(input_critical_section);
   float x, y;

   auto currentPoint = args->CurrentPoint;
   x = currentPoint->RawPosition.X;
   y = currentPoint->RawPosition.Y;

   auto& pointer = key_states.pointer[key_states.curr_pointer];   
   if (!input_translate_coord_viewport(x, y, &pointer.x, &pointer.y, &pointer.full_x, &pointer.full_y))
   {
      return;
   }
   pointer.id = currentPoint->PointerId;

   //RARCH_LOG("PointerPressed: %u (%f,%f) at %u \n", pointer.id, x, y, key_states.curr_pointer);

   do {
      key_states.curr_pointer = (key_states.curr_pointer + 1) % MAX_TOUCH;
   } while (key_states.pointer[key_states.curr_pointer].id != 0);

}

void OnPointerReleased(CoreWindow ^sender, PointerEventArgs ^args)
{
   critical_section::scoped_lock lock(input_critical_section);

   auto currentPoint = args->CurrentPoint;
   unsigned int id = currentPoint->PointerId;
   for (unsigned int i = 0; i < MAX_TOUCH; i++)
   {
      auto& pointer = key_states.pointer[i];
      if (pointer.id == id)
      {
         //RARCH_LOG("PointerReleased: %u at %u \n", pointer.id, i);

         pointer.id = 0;
         if (i < key_states.curr_pointer)
         {
            key_states.curr_pointer = i;
         }
         break;
      }
   }
}


void OnPointerMoved(CoreWindow ^sender, PointerEventArgs ^args)
{
   critical_section::scoped_lock lock(input_critical_section);

   auto currentPoint = args->CurrentPoint;

   float x = currentPoint->RawPosition.X;
   float y = currentPoint->RawPosition.Y;


   unsigned int id = currentPoint->PointerId;
   for (unsigned int i = 0; i < MAX_TOUCH; i++)
   {
      auto& pointer = key_states.pointer[i];
      if (pointer.id == id)
      {
         //RARCH_LOG("PointerMoved: %u (%f,%f) at %u \n", pointer.id, x, y, i);

         input_translate_coord_viewport(x, y, &pointer.x, &pointer.y, &pointer.full_x, &pointer.full_y);
         break;
      }
   }
}

static void *uwp_input_input_init(void)
{
   memset(&key_states, 0, sizeof(key_states));
   key_states.current = key_states.A;

   input_keymaps_init_keyboard_lut(rarch_key_map_uwp);

   // Input events are handled on the UI thread
   static bool bound = false;
   if (!bound)
   {
      auto async = d3d11::ui_dispatcher->RunAsync(CoreDispatcherPriority::Normal, ref new DispatchedHandler([=]()
      {
         auto window = CoreWindow::GetForCurrentThread();
	      window->KeyDown += ref new TypedEventHandler<CoreWindow^, KeyEventArgs^>(&OnKeyEvent);
         window->KeyUp += ref new TypedEventHandler<CoreWindow^, KeyEventArgs^>(&OnKeyEvent);
         window->PointerPressed += ref new TypedEventHandler<CoreWindow ^, PointerEventArgs ^>(&OnPointerPressed);
         window->PointerReleased += ref new TypedEventHandler<CoreWindow ^, PointerEventArgs ^>(&OnPointerReleased);
         window->PointerMoved += ref new TypedEventHandler<CoreWindow ^, PointerEventArgs ^>(&OnPointerMoved);
         window->PointerExited += ref new TypedEventHandler<CoreWindow ^, PointerEventArgs ^>(&OnPointerReleased);
      }, Platform::CallbackContext::Any));
      bound = true;
   }
   
   return (void*)-1;
}

static void uwp_input_input_poll(void *data)
{
   int i;

   // Get a pointer to the current and next buffers
   key_state_t* next = key_states.current == key_states.A ? key_states.B : key_states.A;
   key_state_t* current = key_states.current;

   {
      critical_section::scoped_lock lock(input_critical_section);

      // Copy current into the next buffer
      memcpy(next, current, sizeof(key_states.previous));

      // Flip the buffer the UI thread writes to (this should be atomic)
      key_states.current = next;

      // Arrange the pointer array so it's contiguous
      int j = 0;
      for (i = 0; i < MAX_TOUCH, j < MAX_TOUCH; i++,j++)
      {
         while (key_states.pointer[j].id == 0 && j < (MAX_TOUCH - 1))
         {
            j++;
         }
         if (i != j)
         {
            key_states.pointer[i] = key_states.pointer[j];
            key_states.pointer[j].id = 0;
         }
      }
   }

   // Check for changed key state
   for (i = 0; i < ARRAY_SIZE(key_states.previous); i++)
   {
      if (current[i].state != key_states.previous[i].state)
      {
         input_keyboard_event(current[i].state, i, current[i].scan_code, 0, RETRO_DEVICE_KEYBOARD);
      }
   }

   // Copy the current values to the previous buffer
   memcpy(key_states.previous, current, sizeof(key_states.previous));

}

static bool uwp_input_keyboard_pressed(const struct retro_keybind *retro_keybinds, unsigned key)
{
	settings_t *settings = config_get_ptr();

	if (retro_keybinds[key].key >= RETROK_LAST)
		return false;

	Windows::System::VirtualKey tk = (Windows::System::VirtualKey)input_keymaps_translate_rk_to_keysym(retro_keybinds[key].key);

   // Read the state from where the UI thread is *not* currently writing to
   key_state_t* current = key_states.current == key_states.A ? key_states.B : key_states.A;

	return current[(unsigned int)tk].state;
}

static int16_t uwp_pointer_state(unsigned idx, unsigned id, bool screen)
{
   switch (id)
   {
   case RETRO_DEVICE_ID_POINTER_X:
      return screen ? key_states.pointer[idx].full_x : key_states.pointer[idx].x;
   case RETRO_DEVICE_ID_POINTER_Y:
      return screen ? key_states.pointer[idx].full_y : key_states.pointer[idx].y;
   case RETRO_DEVICE_ID_POINTER_PRESSED:
      return key_states.pointer[idx].id != 0;
   case RARCH_DEVICE_ID_POINTER_BACK:
      return 0; //TODO: get back button state
   default:
      break;
   }

   return 0;
}

static int16_t uwp_input_input_state(void *data,
      const struct retro_keybind **retro_keybinds, unsigned port,
      unsigned device, unsigned idx, unsigned id)
{
   settings_t *settings = config_get_ptr();

   switch (device)
   {
   case RETRO_DEVICE_JOYPAD:
      return uwp_input_keyboard_pressed(retro_keybinds[port], id);

   case RETRO_DEVICE_KEYBOARD:
	   return uwp_input_keyboard_pressed(retro_keybinds[port], id);

   case RETRO_DEVICE_ANALOG:
	   return 0;

   case RETRO_DEVICE_MOUSE:
	   return 0;

   case RARCH_DEVICE_MOUSE_SCREEN:
	   return 0;

   case RETRO_DEVICE_POINTER:
   case RARCH_DEVICE_POINTER_SCREEN:
      return uwp_pointer_state(idx, id, device == RARCH_DEVICE_POINTER_SCREEN);

   case RETRO_DEVICE_LIGHTGUN:
	   return 0;
   }

   return 0;
}

static bool uwp_input_input_key_pressed(void *data, int key)
{
   settings_t *settings = config_get_ptr();
	return uwp_input_keyboard_pressed(settings->input.binds[0], key);
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

