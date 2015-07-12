/**
 * Copyright (c) 2014, Fabian Knorr
 *
 * This file is part of libgtkglcanvas.
 *
 * libgtkglcanvas is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * libgtkglcanvas is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libgtkglcanvas. If not, see <http://www.gnu.org/licenses/>.
 */


#pragma once

#include <glib.h>


G_BEGIN_DECLS



typedef enum _GtkGLState {

} GtkGLState;


typedef enum _GtkGLColorType {
    GTK_GL_COLOR_RGBA,
    GTK_GL_COLOR_INDEXED
} GtkGLColorType;


typedef enum _GtkGLTransparentType {
    GTK_GL_TRANSPARENT_RGB,
    GTK_GL_TRANSPARENT_INDEX
} GtkGLTransparentType;


typedef void *GtkGLConfigID;


typedef struct _GtkGLFramebufferConfig {
    gboolean accelerated;
    GtkGLColorType color_type;
    unsigned color_bpp;
    int fb_level;
    gboolean double_buffered;
    gboolean stereo_buffered;
    unsigned aux_buffers;
    unsigned red_color_bpp;
    unsigned green_color_bpp;
    unsigned blue_color_bpp;
    unsigned alpha_color_bpp;
    unsigned depth_bpp;
    unsigned stencil_bpp;
    unsigned red_accum_bpp;
    unsigned green_accum_bpp;
    unsigned blue_accum_bpp;
    unsigned alpha_accum_bpp;
    GtkGLTransparentType transparent_type;
    unsigned transparent_red;
    unsigned transparent_green;
    unsigned transparent_blue;
    unsigned transparent_alpha;
} GtkGLFramebufferConfig;


typedef enum _GtkGLAttribute {
    GTK_GL_ACCELERATED,
    GTK_GL_COLOR_TYPE,
    GTK_GL_COLOR_BPP,
    GTK_GL_FB_LEVEL,
    GTK_GL_DOUBLE_BUFFERED,
    GTK_GL_STEREO_BUFFERED,
    GTK_GL_AUX_BUFFERS,
    GTK_GL_RED_COLOR_BPP,
    GTK_GL_GREEN_COLOR_BPP,
    GTK_GL_BLUE_COLOR_BPP,
    GTK_GL_ALPHA_COLOR_BPP,
    GTK_GL_DEPTH_BPP,
    GTK_GL_STENCIL_BPP,
    GTK_GL_RED_ACCUM_BPP,
    GTK_GL_GREEN_ACCUM_BPP,
    GTK_GL_BLUE_ACCUM_BPP,
    GTK_GL_ALPHA_ACCUM_BPP,
    GTK_GL_TRANSPARENT_TYPE,
    GTK_GL_TRANSPARENT_RED,
    GTK_GL_TRANSPARENT_GREEN,
    GTK_GL_TRANSPARENT_BLUE,
    GTK_GL_TRANSPARENT_ALPHA
} GtkGLAttribute;


typedef enum _GtkGLRestraint {
    GTK_GL_PREFERABLY,
    GTK_GL_EXACTLY,
    GTK_GL_AT_MOST,
    GTK_GL_AT_LEAST
} GtkGLRestraint;


GtkGLVisual *gtk_gl_enumerate_visuals(size_t *out_count);

void gtk_gl_describe_visual(GtkGLConfigId id, GtkGLFramebufferConfig *out);

GtkGLVisual *gtk_gl_choose_visuals(const int *attrib_list, size_t *out_count);


G_END_DECLS
