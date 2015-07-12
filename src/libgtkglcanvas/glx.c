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

/**
 * This file is based on the work of André Diego Piske,
 * see <https://github.com/andrepiske/tegtkgl>. To retain André's licensing
 * conditions on the parts of the software authored by him, the following
 * copyright notice shall be included in this and all derived files:
 */

/**
 * Copyright (c) 2013 André Diego Piske
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 **/

#include <stdlib.h>
#include <assert.h>

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>

#include <X11/extensions/xf86vmode.h>

#include <GL/glxew.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <glib-object.h>

#include "visual.h"
#include "canvas.h"
#include "canvas_impl.h"


struct _GtkGLVisual {
    Display *dpy;
    GLXFBConfig cfg;
};


static GtkGLVisual *
gtk_gl_visual_new(Display *dpy, GLXFBConfig cfg) {
    GtkGLVisual visual = { dpy, cfg };
    return g_memdup(&visual, sizeof visual);
}


void
gtk_gl_visual_free(GtkGLVisual *visual) {
    g_free(visual);
}


struct _GtkGLCanvas_NativePriv {
    Display *dpy;
    int screen;
    Window win;
    GLXContext glc;
};


GtkGLCanvas_NativePriv*
gtk_gl_canvas_native_new() {
	return g_malloc0(sizeof(GtkGLCanvas_NativePriv));
}


void
gtk_gl_canvas_native_realize(GtkGLCanvas *canvas) {
	GtkGLCanvas_Priv *priv = GTK_GL_CANVAS_GET_PRIV(canvas);
    GtkGLCanvas_NativePriv *native = priv->native;

    native->win = gdk_x11_window_get_xid(priv->win);
	if (!native->win) g_critical("Unable to get X11 window for canvas");

    native->dpy = gdk_x11_display_get_xdisplay(gdk_window_get_display(
            priv->win));
	if (!native->dpy) g_critical("Unable to get X11 display");

    native->screen = gdk_x11_screen_get_screen_number(gdk_window_get_screen(
            priv->win));
    if (!native->screen) g_critical("Unable to get X11 screen");
}


GtkGLVisualList *
gtk_gl_canvas_enumerate_visuals(GtkGLCanvas *canvas) {
	GtkGLCanvas_Priv *priv = GTK_GL_CANVAS_GET_PRIV(canvas);
    GtkGLCanvas_NativePriv *native = priv->native;

    int fbconfig_count;
    GLXFBConfig *fbconfigs;
    GtkGLVisualList *list;
    size_t i;

    assert(native->dpy);

    fbconfigs = glXGetFBConfigs(native->dpy, native->screen, &fbconfig_count);
    list = gtk_gl_visual_list_new(TRUE, fbconfig_count);
    for (i = 0; i < list->count; ++i) {
        list->entries[i] = gtk_gl_visual_new(native->dpy, fbconfigs[i]);
    }
    return list;
}


void
gtk_gl_describe_visual(const GtkGLVisual *visual, GtkGLFramebufferConfig *out) {
    int value;

#define QUERY(attr) \
    (glXGetFBConfigAttrib(visual->dpy, visual->cfg, GLX_##attr, &value), value)

    out->accelerated = TRUE;

    QUERY(RENDER_TYPE);
    out->color_type = (
            (value & GLX_RGBA_BIT ? GTK_GL_COLOR_RGBA : 0)
            | (value & GLX_COLOR_INDEX_BIT ? GTK_GL_COLOR_INDEXED : 0));

    out->color_bpp = QUERY(BUFFER_SIZE);
    out->fb_level = QUERY(LEVEL);
    out->double_buffered = QUERY(DOUBLEBUFFER);
    out->stereo_buffered = QUERY(STEREO);
    out->aux_buffers = QUERY(AUX_BUFFERS);
    out->red_color_bpp = QUERY(RED_SIZE);
    out->green_color_bpp = QUERY(GREEN_SIZE);
    out->blue_color_bpp = QUERY(BLUE_SIZE);
    out->alpha_color_bpp = QUERY(ALPHA_SIZE);
    out->depth_bpp = QUERY(DEPTH_SIZE);
    out->stencil_bpp = QUERY(STENCIL_SIZE);
    out->red_accum_bpp = QUERY(ACCUM_RED_SIZE);
    out->green_accum_bpp = QUERY(ACCUM_GREEN_SIZE);
    out->blue_accum_bpp = QUERY(ACCUM_BLUE_SIZE);
    out->alpha_accum_bpp = QUERY(ACCUM_ALPHA_SIZE);

    QUERY(TRANSPARENT_TYPE);
    out->transparent_type
            = value == GLX_NONE ? GTK_GL_TRANSPARENT_NONE
            : value == GLX_TRANSPARENT_RGB ? GTK_GL_TRANSPARENT_INDEX
            : GTK_GL_TRANSPARENT_INDEX;

    out->transparent_index = QUERY(TRANSPARENT_INDEX_VALUE);
    out->transparent_red = QUERY(TRANSPARENT_RED_VALUE);
    out->transparent_green = QUERY(TRANSPARENT_GREEN_VALUE);
    out->transparent_blue = QUERY(TRANSPARENT_BLUE_VALUE);
    out->transparent_alpha = QUERY(TRANSPARENT_ALPHA_VALUE);

    if (GLXEW_VERSION_1_4) {
        out->sample_buffers = QUERY(SAMPLE_BUFFERS);
        out->samples_per_pixel = QUERY(SAMPLES);
    } else if (GLXEW_ARB_multisample) {
        out->sample_buffers = QUERY(SAMPLE_BUFFERS_ARB);
        out->samples_per_pixel = QUERY(SAMPLES_ARB);
    } else {
        out->sample_buffers = out->samples_per_pixel = 0;
    }

#undef QUERY
}


gboolean
gtk_gl_canvas_native_create_context(GtkGLCanvas *canvas,
        GtkGLVisual *visual) {
	GtkGLCanvas_Priv *priv = GTK_GL_CANVAS_GET_PRIV(canvas);
	GtkGLCanvas_NativePriv *native = priv->native;

    XVisualInfo *vi;
    int attrib;

    assert(visual);

    vi = glXGetVisualFromFBConfig(visual->dpy, visual->cfg);
    if (!vi) {
        g_error("Unable to get X visual form GtkGLVisual");
        return FALSE;
    }

    native->glc = glXCreateContext(native->dpy, vi, NULL, GL_TRUE);
	if (!native->glc)
	{
		g_error("Unable to create GLX context");
		return FALSE;
	}

    glXGetFBConfigAttrib(native->dpy, visual->cfg, GLX_DOUBLEBUFFER,
            &attrib);
    priv->double_buffered = (unsigned) attrib;
    priv->effective_depth = vi->depth;

    XFree(vi);
	return TRUE;
}


gboolean
gtk_gl_canvas_native_create_context_with_version(GtkGLCanvas *canvas,
        GtkGLVisual *visual, unsigned ver_major, unsigned ver_minor,
        GtkGLProfile profile) {
    return FALSE;
}


void
gtk_gl_canvas_native_destroy_context(GtkGLCanvas *canvas) {
	GtkGLCanvas_Priv *priv = GTK_GL_CANVAS_GET_PRIV(canvas);
	GtkGLCanvas_NativePriv *native = priv->native;

    if (native->dpy) {
		glXDestroyContext(native->dpy, native->glc);
		native->glc = NULL;
    }
}


void
gtk_gl_canvas_native_make_current(GtkGLCanvas *canvas) {
	GtkGLCanvas_Priv *priv = GTK_GL_CANVAS_GET_PRIV(canvas);
    GtkGLCanvas_NativePriv *native = priv->native;
    glXMakeCurrent(native->dpy, native->win, native->glc);
}


void
gtk_gl_canvas_native_swap_buffers(GtkGLCanvas *canvas) {
	GtkGLCanvas_Priv *priv = GTK_GL_CANVAS_GET_PRIV(canvas);
    GtkGLCanvas_NativePriv *native = priv->native;
    glXSwapBuffers(native->dpy, native->win);
}
