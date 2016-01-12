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

#include <stdlib.h>
#include <string.h>

#include <compat/strl.h>
#include <compat/posix_string.h>
#include <boolean.h>
#include <gfx/math/matrix_4x4.h>

#include "../../general.h"
#include "../video_state_tracker.h"
#include "../../dynamic.h"

#ifdef HAVE_CONFIG_H
#include "../../config.h"
#endif

#ifdef HAVE_OPENGL
#include "../common/gl_common.h"
#endif

#include "../video_shader_driver.h"

static void shader_d3d11_hlsl_deinit(void) { }
static bool shader_d3d11_hlsl_init(void *data, const char *path) { return true; }

static void shader_d3d11_hlsl_set_params(void *data, unsigned width, unsigned height, 
      unsigned tex_width, unsigned tex_height, 
      unsigned out_width, unsigned out_height,
      unsigned frame_count,
      const void *info, 
      const void *prev_info, 
      const void *feedback_info,
      const void *fbo_info, unsigned fbo_info_cnt)
{
}

static bool shader_d3d11_hlsl_set_mvp(void *data, const math_matrix_4x4 *mat)
{
#ifdef HAVE_OPENGL
#ifndef NO_GL_FF_MATRIX
   if (!strcmp(video_driver_get_ident(), "gl"))
      gl_ff_matrix(mat);
#endif
#endif
   return false;
}

static bool shader_d3d11_hlsl_set_coords(const void *data)
{
#ifdef HAVE_OPENGL
#ifndef NO_GL_FF_VERTEX
   if (!strcmp(video_driver_get_ident(), "gl"))
   {
      const struct gfx_coords *coords = (const struct gfx_coords*)data;
      gl_ff_vertex(coords);
   }
#endif
#endif
   return false;
}

static void shader_d3d11_hlsl_use(void *data, unsigned idx)
{
   (void)data;
   (void)idx;
}

static unsigned shader_d3d11_hlsl_num(void)
{
   return 0;
}

static bool shader_d3d11_hlsl_filter_type(unsigned idx, bool *smooth)
{
   (void)idx;
   (void)smooth;
   return false;
}

static enum gfx_wrap_type shader_d3d11_hlsl_wrap_type(unsigned idx)
{
   (void)idx;
   return RARCH_WRAP_BORDER;
}

static void shader_d3d11_hlsl_shader_scale(unsigned idx,
      struct gfx_fbo_scale *scale)
{
   (void)idx;
   (void)scale;
}

static unsigned shader_d3d11_hlsl_get_prev_textures(void)
{
   return 0;
}

static bool shader_d3d11_hlsl_mipmap_input(unsigned idx)
{
   (void)idx;
   return false;
}

static bool shader_d3d11_hlsl_get_feedback_pass(unsigned *idx)
{
   (void)idx;
   return false;
}

static struct video_shader *shader_d3d11_hlsl_get_current_shader(void)
{
   return NULL;
}

const shader_backend_t d3d11_hlsl_backend = {
   shader_d3d11_hlsl_init,
   shader_d3d11_hlsl_deinit,
   shader_d3d11_hlsl_set_params,
   shader_d3d11_hlsl_use,
   shader_d3d11_hlsl_num,
   shader_d3d11_hlsl_filter_type,
   shader_d3d11_hlsl_wrap_type,
   shader_d3d11_hlsl_shader_scale,
   shader_d3d11_hlsl_set_coords,
   shader_d3d11_hlsl_set_mvp,
   shader_d3d11_hlsl_get_prev_textures,
   shader_d3d11_hlsl_get_feedback_pass,
   shader_d3d11_hlsl_mipmap_input,
   shader_d3d11_hlsl_get_current_shader,

   RARCH_SHADER_NONE,
   "d3d11"
};