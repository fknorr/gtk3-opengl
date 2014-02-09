/** 
 * Copyright (c) 2014, Fabian Knorr
 * 
 * This file is part of libgtk3-opengl.
 *
 * libgtk3-opengl is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * libgtk3-opengl is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libgtk3-opengl. If not, see <http://www.gnu.org/licenses/>.
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
    native->xdis = NULL;
	return native;
}


void 
gtk_gl_canvas_native_create_context(GtkGLCanvas_Priv *priv)
{
	GtkGLCanvas_NativePriv *native = priv->native;
    XVisualInfo *vi;
    int att[] = { GLX_RGBA, GLX_DEPTH_SIZE, priv->attrs.depth_buffer_bits, 
		GLX_SAMPLES, (int) priv->attrs.num_samples,
		None, None, None, None };
	int *att_ptr = att+5;

	if (priv->attrs.flags & GTK_GL_DOUBLE_BUFFERED)
		*att_ptr++ = GLX_DOUBLEBUFFER;
	if (priv->attrs.flags & GTK_GL_STEREO)
		*att_ptr++ = GLX_STEREO;
	if (priv->attrs.flags & GTK_GL_SAMPLE_BUFFERS) 
		*att_ptr++ = GLX_SAMPLE_BUFFERS;
	
    native->xdis = gdk_x11_display_get_xdisplay(priv->disp);
    vi = glXChooseVisual(native->xdis, 0, att);
    native->glc = glXCreateContext(native->xdis, vi, 0, GL_TRUE);	
	priv->effective_depth = vi->depth;
}


void 
gtk_gl_canvas_native_attach_context(GtkGLCanvas_Priv *priv)
{
	GtkGLCanvas_NativePriv *native = priv->native;
    native->xwin = gdk_x11_window_get_xid(priv->win);
}


void 
gtk_gl_canvas_native_destroy_context(GtkGLCanvas_Priv *priv)
{
	GtkGLCanvas_NativePriv *native = priv->native;
	
    if (native->xdis) {
        glXDestroyContext(native->xdis, native->glc);
        // g_clear_object((volatile GObject**)&priv->win);

        native->xdis = NULL;
    }
}


void
gtk_gl_canvas_native_make_current(GtkGLCanvas_Priv *priv)
{
    GtkGLCanvas_NativePriv *native = priv->native;
    glXMakeCurrent(native->xdis, native->xwin, native->glc);
}


void
gtk_gl_canvas_native_swap_buffers(GtkGLCanvas_Priv *priv)
{
    GtkGLCanvas_NativePriv *native = priv->native;
    glXSwapBuffers(native->xdis, native->xwin);
}

