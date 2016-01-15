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

#include "../common/d3d11_common.h"
#include "../../general.h"
#include "../../driver.h"
#include "../video_context_driver.h"

struct d3d11_settings {
   bool menu_enabled;
} g_d3d11_settings;

static const gfx_ctx_driver_t * d3d11_get_context(void *data)
{
	unsigned minor = 2;
	unsigned major = 11;
	enum gfx_ctx_api api = GFX_CTX_DIRECT3D11_API;

	settings_t *settings = config_get_ptr();
	return gfx_ctx_init_first(video_driver_get_ptr(false),
		settings->video.context_driver,
		api, major, minor, false);
}

static void *d3d11_gfx_init(const video_info_t *video,
      const input_driver_t **input, void **input_data)
{
   RARCH_ERR("Using the null video driver. RetroArch will not be visible.");

   settings_t *settings = config_get_ptr();
   driver_t   *driver = driver_get_ptr();

   memset(&g_d3d11_settings, 0, sizeof(g_d3d11_settings));

   d3d11::DeviceResources* vid = NULL;
   const gfx_ctx_driver_t *ctx = NULL;
   *input = NULL;
   *input_data = NULL;

   // Create the video resources
   vid = new d3d11::DeviceResources(video);
   
   // Create the context
   ctx = d3d11_get_context(vid);
   if (!ctx)
	   goto error;
   gfx_ctx_set(ctx);

   // Initialize font renderer
   if (!font_init_first((const void**)&driver->font_osd_driver, &driver->font_osd_data,
	   vid, settings->video.font_path, 0, FONT_DRIVER_RENDER_DIRECT3D_API))
   {
	   goto error;
   }
   

   return vid;

error:
   if (vid)
	   delete vid;
   gfx_ctx_destroy(ctx);
   return NULL;
}

static D2D1_MATRIX_3X2_F d3d11_get_display_matrix(void *data, unsigned width, unsigned height)
{
	settings_t *settings = config_get_ptr();
	auto d3d11res = (d3d11::DeviceResources*)data;
	int x = 0;
	int y = 0;

	float desired_aspect = video_driver_get_aspect_ratio();
	video_driver_get_size(&width, &height);
	float device_aspect = (float)width / height;
	
	D2D1_MATRIX_3X2_F result;

	if (device_aspect > desired_aspect)
	{ 
		const float scale = desired_aspect / device_aspect;
		result = D2D1::Matrix3x2F::Scale(scale, 1 );
		result = result * D2D1::Matrix3x2F::Translation((1 - scale)*0.5f*width, 0);
	}
	else
	{
		const float scale = device_aspect / desired_aspect;
		result = D2D1::Matrix3x2F::Scale( 1, scale);
	}

	return result;
}

static bool d3d11_gfx_frame(void *data, const void *frame,
      unsigned width, unsigned height, uint64_t frame_count,
      unsigned pitch, const char *msg)
{
   static struct retro_perf_counter d3d_frame = { 0 };

   driver_t *driver = driver_get_ptr();
   settings_t *settings = config_get_ptr();
   const font_renderer_t *font_ctx = driver->font_osd_driver;

   rarch_perf_init(&d3d_frame, "d3d_frame");
   retro_perf_start(&d3d_frame);

   auto d3d11res = (d3d11::DeviceResources*)data;
   auto d2dctx = d3d11res->GetD2DDeviceContext();
   d2dctx->BeginDraw();
   d2dctx->Clear();

   d2dctx->SetTransform(d3d11_get_display_matrix(data, width, height));

   auto viewport = d3d11res->GetScreenViewport();
   D2D1_RECT_F rect = { 0, 0, viewport.Width, viewport.Height };

   d3d11res->SetFrameTexture(frame, d3d11res->GetVideoInfo()->rgb32, width, height, pitch);

   d2dctx->DrawBitmap(d3d11res->GetD2DFrameBitmap(), rect);

#ifdef HAVE_MENU
   auto menu_bitmap = d3d11res->GetD2DMenuBitmap();
   if (menu_bitmap && g_d3d11_settings.menu_enabled)
   {
	   d2dctx->SetTransform(d3d11_get_display_matrix(data, menu_bitmap->GetPixelSize().width, menu_bitmap->GetPixelSize().height));	   
	   d2dctx->DrawBitmap(menu_bitmap, rect);
   }
#endif
   
#ifdef HAVE_OVERLAY
   if (settings->input.overlay_enable)
   {
      d3d11res->RenderOverlays();
   }
#endif

   d3d11::ThrowIfFailed( d2dctx->EndDraw() );

   if (font_ctx->render_msg && msg)
   {
	   struct font_params font_parms = { 0 };
	   font_ctx->render_msg(driver->font_osd_data, msg, &font_parms);
   }


#ifdef HAVE_MENU
   if (menu_driver_alive())
	   menu_driver_frame();
#endif

   retro_perf_stop(&d3d_frame);

   gfx_ctx_update_window_title(data);
   gfx_ctx_swap_buffers(data);

   return true;
}

static void d3d11_gfx_set_nonblock_state(void *data, bool toggle)
{
   (void)data;
   (void)toggle;
}

static bool d3d11_gfx_alive(void *data)
{
   auto d3d11res = (d3d11::DeviceResources*)data;

   unsigned temp_width = 0, temp_height = 0;
   auto vp = d3d11res->GetScreenViewport();
   temp_width = vp.Width;
   temp_height = vp.Height;
   if (temp_width != 0 && temp_height != 0)
   {
	   video_driver_set_size(&temp_width, &temp_height);
   }

   return true;
}

static bool d3d11_gfx_focus(void *data)
{
   (void)data;
   return true;
}

static bool d3d11_gfx_suppress_screensaver(void *data, bool enable)
{
   (void)data;
   (void)enable;
   return false;
}

static bool d3d11_gfx_has_windowed(void *data)
{
   (void)data;
   return true;
}

static void d3d11_gfx_free(void *data)
{
   auto d3d11res = (d3d11::DeviceResources*)data;
   delete d3d11res;
}

static void d3d11_set_viewport(void *data,
	unsigned width, unsigned height,
	bool force_full,
	bool allow_rotate)
{
	
}

static bool d3d11_gfx_set_shader(void *data,
      enum rarch_shader_type type, const char *path)
{
   (void)data;
   (void)type;
   (void)path;

   return false; 
}

static void d3d11_gfx_set_rotation(void *data,
      unsigned rotation)
{
   (void)data;
   (void)rotation;
}

static void d3d11_gfx_viewport_info(void *data,
      struct video_viewport *vp)
{
   auto d3d11res = (d3d11::DeviceResources*)data;
   auto viewport = d3d11res->GetScreenViewport();

   vp->x = viewport.TopLeftX;
   vp->y = viewport.TopLeftY;
   vp->width = vp->full_width = viewport.Width;
   vp->height = vp->full_height = viewport.Height;
}

static bool d3d11_gfx_read_viewport(void *data, uint8_t *buffer)
{
   (void)data;
   (void)buffer;

   return true;
}

static void d3d11_set_aspect_ratio(void *data, unsigned aspect_ratio_idx)
{
	auto d3d11res = (d3d11::DeviceResources*)data;
	enum rarch_display_ctl_state cmd = RARCH_DISPLAY_CTL_NONE;

	switch (aspect_ratio_idx)
	{
	case ASPECT_RATIO_SQUARE:
		cmd = RARCH_DISPLAY_CTL_SET_VIEWPORT_SQUARE_PIXEL;
		break;

	case ASPECT_RATIO_CORE:
		cmd = RARCH_DISPLAY_CTL_SET_VIEWPORT_CORE;
		break;

	case ASPECT_RATIO_CONFIG:
		cmd = RARCH_DISPLAY_CTL_SET_VIEWPORT_CONFIG;
		break;

	default:
		break;
	}

	if (cmd != RARCH_DISPLAY_CTL_NONE)
		video_driver_ctl(cmd, NULL);

	video_driver_set_aspect_ratio_value(aspectratio_lut[aspect_ratio_idx].value);

	// TODO: implement aspect ratio 
	/*
	if (!d3d11res)
		return;

	d3d->keep_aspect = true;
	d3d->should_resize = true;
	*/
}

static void d3d11_apply_state_changes(void *data)
{
	// TODO: see if this is needed
	/*
	d3d_video_t *d3d = (d3d_video_t*)data;
    if (d3d)
       d3d->should_resize = true;
	*/
}

static void d3d11_set_osd_msg(void *data, const char *msg,
	const struct font_params *params, void *font)
{
	auto       d3d11res = (d3d11::DeviceResources*)data;
	driver_t    *driver = driver_get_ptr();
	const font_renderer_t *font_ctx = driver->font_osd_driver;

	// TODO: set d3d11res font rect?

	if (font_ctx->render_msg)
		font_ctx->render_msg(driver->font_osd_data, msg, params);
}

static void d3d11_show_mouse(void *data, bool state)
{
	gfx_ctx_show_mouse(data, state);
}

#ifdef HAVE_MENU
static void d3d11_set_menu_texture_frame(void *data,
	const void *frame, bool rgb32, unsigned width, unsigned height,
	float alpha)
{
	auto d3d11res = (d3d11::DeviceResources*)data;
	d3d11res->SetMenuTextureFrame(frame, rgb32, width, height, alpha);
	
}

static void d3d11_set_menu_texture_enable(void *data,
	bool state, bool full_screen)
{
   g_d3d11_settings.menu_enabled = state;
}
#endif

static const video_poke_interface_t d3d11_poke_interface = {
	NULL,
	NULL,
	NULL, /* get_video_output_size */
	NULL, /* get_video_output_prev */
	NULL, /* get_video_output_next */
	NULL, /* get_current_framebuffer */
	NULL, /* get_proc_address */
	d3d11_set_aspect_ratio,
	d3d11_apply_state_changes,
#ifdef HAVE_MENU
	d3d11_set_menu_texture_frame,
	d3d11_set_menu_texture_enable,
#else
	NULL,
#endif
	d3d11_set_osd_msg,
	d3d11_show_mouse,
};

static void d3d11_gfx_get_poke_interface(void *data,
	const video_poke_interface_t **iface)
{
	(void)data;
	*iface = &d3d11_poke_interface;
}

#ifdef HAVE_OVERLAY
static void d3d11_overlay_enable(void *data, bool state)
{

}

static bool d3d11_overlay_load(void *data,
   const void *image_data, unsigned num_images) 
{
   if (!num_images)
   {
      return false;
   }

   auto d3d11res = (d3d11::DeviceResources*)data;

   const struct texture_image *images =
      (const struct texture_image*)image_data;

   d3d11res->InitOverlays(images, num_images);

   return true;
}

static void d3d11_overlay_full_screen(void *data, bool enable)
{

}

static void d3d11_overlay_tex_geom(void *data, unsigned image,
   float x, float y, float w, float h)
{
   auto d3d11res = (d3d11::DeviceResources*)data;
   auto overlay = d3d11res->GetOverlay(image);
   if (overlay)
   {
      overlay->TexCoord = D2D_RECT_F{ x, y, w, h };
   }
}

static void d3d11_overlay_vertex_geom(void *data, unsigned image,
   float x, float y, float w, float h)
{
   auto d3d11res = (d3d11::DeviceResources*)data;
   auto overlay = d3d11res->GetOverlay(image);
   if (overlay)
   {
      overlay->Geometry = D2D_RECT_F{ x, y, w, h };
   }
}

static void d3d11_overlay_set_alpha(void *data, unsigned image, float mod)
{
   auto d3d11res = (d3d11::DeviceResources*)data;

}

static const video_overlay_interface_t d3d11_overlay_interface = {
   d3d11_overlay_enable,
   d3d11_overlay_load,
   d3d11_overlay_tex_geom,
   d3d11_overlay_vertex_geom,
   d3d11_overlay_full_screen,
   d3d11_overlay_set_alpha,
};

static void d3d11_get_overlay_interface(void *data,
   const video_overlay_interface_t **iface)
{
   (void)data;
   *iface = &d3d11_overlay_interface;
}
#endif

video_driver_t video_d3d11 = {
   d3d11_gfx_init,
   d3d11_gfx_frame,
   d3d11_gfx_set_nonblock_state,
   d3d11_gfx_alive,
   d3d11_gfx_focus,
   d3d11_gfx_suppress_screensaver,
   d3d11_gfx_has_windowed,
   d3d11_gfx_set_shader,
   d3d11_gfx_free,
   "d3d11",
   d3d11_set_viewport,
   d3d11_gfx_set_rotation,
   d3d11_gfx_viewport_info,
   d3d11_gfx_read_viewport,
   NULL, /* read_frame_raw */

#ifdef HAVE_OVERLAY
   d3d11_get_overlay_interface,
#endif
  d3d11_gfx_get_poke_interface,
};
