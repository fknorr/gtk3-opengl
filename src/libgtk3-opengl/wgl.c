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

#include "canvas.h"
#include "canvas_impl.h"
#include <stdlib.h>
#include <windows.h>
#include <gtk/gtk.h>
#include <gdk/gdkwin32.h>
#include <glib-object.h>
#include <GL/gl.h>
#include <GL/wglext.h>


struct _GtkGLCanvas_NativePriv 
{	
	HDC dc;
    HGLRC gl;
};


GtkGLCanvas_NativePriv*
gtk_gl_canvas_native_new()
{
	GtkGLCanvas_NativePriv *native = malloc(sizeof(GtkGLCanvas_NativePriv));
    native->gl = NULL;
	native->dc = NULL;
	return native;
}


char *format_last_error(void)
{
	char *ptr;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL,
	              GetLastError(), 0, &ptr, 0, NULL);
	return ptr;
}


typedef BOOL WINAPI (*wgl_pixel_format_chooser_t) (HDC hdc, const int *piAttribIList, 
         const FLOAT *pfAttribFList, UINT nMaxFormats, int *piFormats, UINT *nNumFormats);


static gboolean
choose_pixel_format_arb(HDC dc, const GtkGLAttributes *attrs, wgl_pixel_format_chooser_t choose)
{
	GLint iattribs[] = {
		WGL_DOUBLE_BUFFER_ARB, ((attrs->flags & GTK_GL_DOUBLE_BUFFERED) ? GL_TRUE : GL_FALSE),
		WGL_STEREO_ARB, ((attrs->flags & GTK_GL_STEREO) ? GL_TRUE : GL_FALSE),
		WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
		WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
		WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
		WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
		WGL_COLOR_BITS_ARB, attrs->color_buffer_bits,
		WGL_DEPTH_BITS_ARB, attrs->depth_buffer_bits,
		WGL_STENCIL_BITS_ARB, attrs->stencil_buffer_bits,
		WGL_SAMPLE_BUFFERS_ARB, ((attrs->flags & GTK_GL_SAMPLE_BUFFERS) ? GL_TRUE : GL_FALSE),
 		WGL_SAMPLES_ARB, attrs->num_samples,
		0
	};

	GLuint n_formats;
	GLint format;
	GLfloat fattribs[] = {0};

	return choose(dc, iattribs, fattribs, 1, &format, &n_formats)
			&& SetPixelFormat(dc, format, NULL);
}


static gboolean
choose_pixel_format_legacy(HDC dc, const GtkGLAttributes *attrs)
{	
	PIXELFORMATDESCRIPTOR pfd =
    {
        sizeof(PIXELFORMATDESCRIPTOR),
        1,
        PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
        PFD_TYPE_RGBA,
        attrs->color_buffer_bits,
        0, 0, 0, 0, 0, 0,
        0,
        0,
        0,
        0, 0, 0, 0,
        attrs->depth_buffer_bits
        attrs->stencil_buffer_bits
        0,
        PFD_MAIN_PLANE,
        0,
        0, 0, 0
    };
	int pf;

	if (attrs->flags & GTK_GL_SAMPLE_BUFFERS)
		g_warning("Target does not support sample buffers");
		          
	return (pf = ChoosePixelFormat(dc, &pfd)) && SetPixelFormat(dc, pf, &pfd);
}


gboolean
gtk_gl_canvas_native_create_context(GtkGLCanvas *canvas, const GtkGLAttributes *attrs)
{
	GtkGLCanvas_Priv *priv = GTK_GL_CANVAS_GET_PRIV(canvas);
	GtkGLCanvas_NativePriv *native = priv->native;
	HWND hwnd;
	HDC dc;
	GLuint n_formats;
	GLint format;
	wgl_pixel_format_chooser_t chooser;
	gboolean success;
	
    hwnd = GDK_WINDOW_HWND(gtk_widget_get_window(GTK_WIDGET(canvas)));
	dc = GetDC(hwnd);
	native->dc = dc;

	chooser = (wgl_pixel_format_chooser_t) wglGetProcAddress("wglChoosePixelFormatARB");
	if (chooser) success = choose_pixel_format_arb(dc, attrs, chooser);
	else success = choose_pixel_format_legacy(dc, attrs);

	success = success
			&& (native->gl = wglCreateContext(dc)) 
			&& wglMakeCurrent(dc, native->gl);
	
	if (!success) priv->error_msg = format_last_error();
	return success;
}


void 
gtk_gl_canvas_native_destroy_context(GtkGLCanvas *canvas)
{
	GtkGLCanvas_Priv *priv = GTK_GL_CANVAS_GET_PRIV(canvas);
	GtkGLCanvas_NativePriv *native = priv->native;
	
    if (native->dc && native->gl) {
		HWND hwnd = GDK_WINDOW_HWND(gtk_widget_get_window(GTK_WIDGET(canvas)));
		wglMakeCurrent(native->dc, NULL);
		wglDeleteContext(native->gl);
		ReleaseDC(hwnd, native->dc);
		native->gl = NULL;
		native->dc = NULL;
    }
}


void
gtk_gl_canvas_native_make_current(GtkGLCanvas *canvas)
{
	GtkGLCanvas_Priv *priv = GTK_GL_CANVAS_GET_PRIV(canvas);

    GtkGLCanvas_NativePriv *native = priv->native;
    wglMakeCurrent(native->dc, native->gl);
}


void
gtk_gl_canvas_native_swap_buffers(GtkGLCanvas *canvas)
{
	GtkGLCanvas_Priv *priv = GTK_GL_CANVAS_GET_PRIV(canvas);

    GtkGLCanvas_NativePriv *native = priv->native;
    wglSwapLayerBuffers(native->dc, WGL_SWAP_MAIN_PLANE);
}

