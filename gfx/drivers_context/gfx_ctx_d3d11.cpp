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

/* Null context. */

#include "../common/d3d11_common.h"
#include "../../driver.h"
#include "../video_context_driver.h"

static void gfx_ctx_d3d11_swap_interval(void *data, unsigned interval)
{
	(void)data;
	(void)interval;
}

static void gfx_ctx_d3d11_check_window(void *data, bool *quit,
	bool *resize, unsigned *width, unsigned *height, unsigned frame_count)
{
	(void)frame_count;
	(void)data;
	(void)quit;
	(void)width;
	(void)height;
	(void)resize;
}

static void gfx_ctx_d3d11_swap_buffers(void *data)
{
	auto d3d11res = (d3d11::DeviceResources*)data;
	d3d11res->Present();
}

static void gfx_ctx_d3d11_set_resize(void *data, unsigned width, unsigned height)
{
	(void)data;
	(void)width;
	(void)height;
}

static void gfx_ctx_d3d11_update_window_title(void *data)
{
	auto d3d11res = (d3d11::DeviceResources*)data;

	(void)data;
}

static void gfx_ctx_d3d11_get_video_size(void *data, unsigned *width, unsigned *height)
{
	(void)data;
	*width = 320;
	*height = 240;
}

static bool gfx_ctx_d3d11_set_video_mode(void *data,
	unsigned width, unsigned height,
	bool fullscreen)
{
	(void)data;
	(void)width;
	(void)height;
	(void)fullscreen;

	return true;
}

static void gfx_ctx_d3d11_destroy(void *data)
{
	(void)data;
}

static void gfx_ctx_d3d11_input_driver(void *data, const input_driver_t **input, void **input_data)
{
	(void)data;
	(void)input;
	(void)input_data;
}

static bool gfx_ctx_d3d11_has_focus(void *data)
{
	(void)data;
	return true;
}

static bool gfx_ctx_d3d11_suppress_screensaver(void *data, bool enable)
{
	(void)data;
	(void)enable;
	return false;
}

static bool gfx_ctx_d3d11_has_windowed(void *data)
{
	(void)data;
	return true;
}

static bool gfx_ctx_d3d11_bind_api(void *data, enum gfx_ctx_api api, unsigned major, unsigned minor)
{
	(void)data;
	(void)api;
	(void)major;
	(void)minor;

	return true;
}

static void gfx_ctx_d3d11_show_mouse(void *data, bool state)
{
	(void)data;
	(void)state;
}

static void gfx_ctx_d3d11_bind_hw_render(void *data, bool enable)
{
	(void)data;
	(void)enable;
}

static bool gfx_ctx_d3d11_init(void *data)
{
	(void)data;

	return true;
}

const gfx_ctx_driver_t gfx_ctx_d3d11 = {
	gfx_ctx_d3d11_init,
	gfx_ctx_d3d11_destroy,
	gfx_ctx_d3d11_bind_api,
	gfx_ctx_d3d11_swap_interval,
	gfx_ctx_d3d11_set_video_mode,
	gfx_ctx_d3d11_get_video_size,
	NULL, /* get_video_output_size */
	NULL, /* get_video_output_prev */
	NULL, /* get_video_output_next */
	NULL, /* get_metrics */
	NULL,
	gfx_ctx_d3d11_update_window_title,
	gfx_ctx_d3d11_check_window,
	gfx_ctx_d3d11_set_resize,
	gfx_ctx_d3d11_has_focus,
	gfx_ctx_d3d11_suppress_screensaver,
	gfx_ctx_d3d11_has_windowed,
	gfx_ctx_d3d11_swap_buffers,
	gfx_ctx_d3d11_input_driver,
	NULL,
	NULL,
	NULL,
	gfx_ctx_d3d11_show_mouse,
	"d3d11",
	gfx_ctx_d3d11_bind_hw_render,
};

