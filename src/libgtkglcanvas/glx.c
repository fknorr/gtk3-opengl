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
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is furnished to do
 * so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 **/


#include "canvas.h"
#include "canvas_impl.h"
#include <stdlib.h>

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>

#include <X11/extensions/xf86vmode.h>

#include <GL/glx.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <glib-object.h>


struct _GtkGLCanvas_NativePriv 
{	
    Display *xdis;
    Window xwin;
    GLXContext glc;
};


GtkGLCanvas_NativePriv*
gtk_gl_canvas_native_new()
{
	GtkGLCanvas_NativePriv *native = malloc(sizeof(GtkGLCanvas_NativePriv));
	native->glc = NULL;
	native->xwin = 0;
    native->xdis = NULL;
	return native;
}


gboolean
gtk_gl_canvas_native_create_context(GtkGLCanvas *canvas, const GtkGLAttributes *attrs)
{
	GtkGLCanvas_Priv *priv = GTK_GL_CANVAS_GET_PRIV(canvas);
	GtkGLCanvas_NativePriv *native = priv->native;
    XVisualInfo *vi;

    // GLX expects a key-value-array for its parameters. 
    int att[] = {
            GLX_RGBA, GLX_DEPTH_SIZE, attrs->depth_buffer_bits, 
            GLX_SAMPLES, (int) attrs->num_samples,
            None, None, None, None },
        fallback_att[] = {
            GLX_RGBA, None, None };
	int *att_ptr = att+5;

	if (attrs->flags & GTK_GL_DOUBLE_BUFFERED) {
		*att_ptr++ = GLX_DOUBLEBUFFER;
        fallback_att[1] = GLX_DOUBLEBUFFER;
    }
    
	if (attrs->flags & GTK_GL_STEREO)
		*att_ptr++ = GLX_STEREO;
	if (attrs->flags & GTK_GL_SAMPLE_BUFFERS) 
		*att_ptr++ = GLX_SAMPLE_BUFFERS;
	
    native->xwin = gdk_x11_window_get_xid(priv->win);
	if (!native->xwin) 
	{
		priv->error_msg = strdup("Unable to get X11 window for canvas");
		return FALSE;
	}	
	
    native->xdis = gdk_x11_display_get_xdisplay(priv->disp);
	if (!native->xdis) 
	{
		priv->error_msg = strdup("Unable to get X display");
		return FALSE;
	}	
	
    vi = glXChooseVisual(native->xdis, 0, att);
    if (!vi) {
        g_warning("GtkGLCanvas: Falling back to default X visual");
        vi = glXChooseVisual(native->xdis, 0, fallback_att);
    }
    
	if (!vi) {
		priv->error_msg = strdup("Unable to get X visual");
		return FALSE;
	}	
	
    native->glc = glXCreateContext(native->xdis, vi, 0, GL_TRUE);	
	if (!native->glc) 
	{
		priv->error_msg = strdup("Unable to create GLX context");
		return FALSE;
	}	
	
	priv->effective_depth = vi->depth;
	return TRUE;
}


void 
gtk_gl_canvas_native_destroy_context(GtkGLCanvas *canvas)
{
	GtkGLCanvas_Priv *priv = GTK_GL_CANVAS_GET_PRIV(canvas);
	GtkGLCanvas_NativePriv *native = priv->native;
	
    if (native->xdis) {		
		glXDestroyContext(native->xdis, native->glc);
		native->glc = NULL;
		native->xwin = 0;
        native->xdis = NULL;
    }
}


void
gtk_gl_canvas_native_make_current(GtkGLCanvas *canvas)
{
	GtkGLCanvas_Priv *priv = GTK_GL_CANVAS_GET_PRIV(canvas);
    GtkGLCanvas_NativePriv *native = priv->native;
    glXMakeCurrent(native->xdis, native->xwin, native->glc);
}


void
gtk_gl_canvas_native_swap_buffers(GtkGLCanvas *canvas)
{
	GtkGLCanvas_Priv *priv = GTK_GL_CANVAS_GET_PRIV(canvas);
    GtkGLCanvas_NativePriv *native = priv->native;
    glXSwapBuffers(native->xdis, native->xwin);
}


GtkGLSupport
gtk_gl_query_feature_support(GtkGLFeature feature)
{
    switch (feature)
    {
        case GTK_GL_DOUBLE_BUFFERED:
        case GTK_GL_STEREO:
        case GTK_GL_SAMPLE_BUFFERS:
            return GTK_GL_FULLY_SUPPORTED;

        default:
            return GTK_GL_UNSUPPORTED;
    }
}
