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


char *
format_last_error(void)
{
	char *ptr;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL,
	              GetLastError(), 0, (LPSTR) &ptr, 0, NULL);
	return ptr;
}


LRESULT CALLBACK
w32_canvas_wnd_proc(HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
    GtkWidget *widget;
	switch(msg)
	{
        case WM_NCHITTEST:
            return HTTRANSPARENT;
            
        default:
            return DefWindowProc(hWnd, msg, wParam, lParam);
    }
}


static BOOL (*dyn_wglChoosePixelFormatARB)(HDC, GLint*, GLfloat*, GLuint, GLint*, GLuint*);



static gboolean
set_pixel_format_legacy(HDC dc, const GtkGLAttributes *attrs)
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
        attrs->depth_buffer_bits,
        attrs->stencil_buffer_bits,
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


static void
w32_init(void)
{
    static GMutex mutex;
    static gboolean allocated = FALSE;

    if (allocated) return;
    
    g_mutex_lock(&mutex);
    if (!allocated)
    {
        WNDCLASSEX wnd_class;
        HWND dummy;
        gboolean success;
            
        ZeroMemory(&wnd_class, sizeof(WNDCLASSEX));
        wnd_class.cbClsExtra = 0;
        wnd_class.cbSize = sizeof(WNDCLASSEX);
        wnd_class.cbWndExtra = 0;
        wnd_class.hbrBackground = (HBRUSH) GetStockObject(DKGRAY_BRUSH);
        wnd_class.hCursor = LoadCursor(NULL,IDC_ARROW);
        wnd_class.hIcon = NULL;
        wnd_class.hIconSm = NULL;
        wnd_class.hInstance = GetModuleHandle(NULL);
        wnd_class.lpfnWndProc = w32_canvas_wnd_proc;
        wnd_class.lpszClassName = "GLCanvas";
        wnd_class.lpszMenuName = NULL;
        wnd_class.style = CS_OWNDC;

        if (!RegisterClassEx(&wnd_class)) goto fail;
        else allocated = TRUE;

        dummy = CreateWindowEx(0, //WS_EX_TRANSPARENT,
        "GLCanvas",
        "",
        WS_VISIBLE,
        0, 0, 10, 10,
        NULL,
        NULL,
        GetModuleHandle(NULL),
        NULL);

        if (dummy)
        {
            HDC dc;
            HGLRC gl;
            GtkGLAttributes attrs;

            attrs.flags = 0;
            attrs.num_samples = 0;
            attrs.color_buffer_bits = 16;
            attrs.depth_buffer_bits = 8;
            attrs.stencil_buffer_bits = 0;        

            success = (dc = GetDC(dummy))
                && set_pixel_format_legacy(dc, &attrs)
                && (gl = wglCreateContext(dc))
                && wglMakeCurrent(dc, gl);
                
            dyn_wglChoosePixelFormatARB
                = (void*) wglGetProcAddress("wglChoosePixelFormatARB");
                
            if (!success) goto fail;

            wglMakeCurrent(dc, NULL);
            wglDeleteContext(gl);
            ReleaseDC(dummy, dc);
        }

        DestroyWindow(dummy);
    }
    goto cleanup;

fail:
    {
        char *error = format_last_error();
        g_error(error);
        free(error);
    }

cleanup:
    g_mutex_unlock(&mutex);
}


struct _GtkGLCanvas_NativePriv 
{
    HWND canvas;
	HDC dc;
    HGLRC gl;
};


GtkGLCanvas_NativePriv*
gtk_gl_canvas_native_new()
{
	GtkGLCanvas_NativePriv *native = calloc(sizeof(GtkGLCanvas_NativePriv), 1);
	return native;
}


typedef BOOL WINAPI (*wgl_pixel_format_chooser_t) (HDC hdc, const int *piAttribIList, 
         const FLOAT *pfAttribFList, UINT nMaxFormats, int *piFormats, UINT *nNumFormats);


static gboolean
set_pixel_format_arb(HDC dc, const GtkGLAttributes *attrs)
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

    return dyn_wglChoosePixelFormatARB
        && dyn_wglChoosePixelFormatARB(dc, iattribs, fattribs, 1, &format, &n_formats)
        && n_formats
        && SetPixelFormat(dc, format, NULL);
}



void
on_size_allocate(GtkWidget *widget, GdkRectangle *rect, gpointer user)
{
	GtkGLCanvas_Priv *priv = GTK_GL_CANVAS_GET_PRIV(GTK_GL_CANVAS(widget));
    if (priv->native->canvas)
        SetWindowPos(priv->native->canvas, NULL, 0, 0, rect->width, rect->height-1, 0);
}


gboolean
gtk_gl_canvas_native_create_context(GtkGLCanvas *canvas, const GtkGLAttributes *attrs)
{
	GtkGLCanvas_Priv *priv = GTK_GL_CANVAS_GET_PRIV(canvas);
	GtkGLCanvas_NativePriv *native = priv->native;
	HWND hwnd = GDK_WINDOW_HWND(gtk_widget_get_window(GTK_WIDGET(canvas)));
	HDC dc = NULL;;
	GLuint n_formats;
	GLint format;
	wgl_pixel_format_chooser_t chooser;
	gboolean success = FALSE;
    RECT client_rect;

    w32_init();
    GetClientRect(hwnd, &client_rect);
    native->canvas = CreateWindowEx(0, //WS_EX_TRANSPARENT,
        "GLCanvas",
        "",
        WS_VISIBLE|WS_CHILD,
        client_rect.left,
        client_rect.top,
        client_rect.right,
        client_rect.bottom-1,
        hwnd,
        NULL,
        GetModuleHandle(NULL),
        NULL);

    if (!native->canvas)
    {
        g_error("Creating native canvas window failed");
        return FALSE;
    }
    ShowWindow(native->canvas, SW_SHOW);
    g_signal_connect(GTK_WIDGET(canvas), "size-allocate", G_CALLBACK(on_size_allocate), NULL);
    
    dc = GetDC(native->canvas);
	native->dc = dc;

    if (dc)
    {
        success =  (set_pixel_format_arb(dc, attrs)
                || set_pixel_format_legacy(dc, attrs))
            && (native->gl = wglCreateContext(dc))
            && wglMakeCurrent(dc, native->gl);
    }
    
	if (!success) priv->error_msg = format_last_error();
    
	return success;
}


void 
gtk_gl_canvas_native_destroy_context(GtkGLCanvas *canvas)
{
	GtkGLCanvas_Priv *priv = GTK_GL_CANVAS_GET_PRIV(canvas);
	GtkGLCanvas_NativePriv *native = priv->native;
	
    if (native->dc && native->gl)
    {
		wglMakeCurrent(native->dc, NULL);
		wglDeleteContext(native->gl);
		ReleaseDC(NULL, native->dc);
		native->gl = NULL;
		native->dc = NULL;
    }

    if (native->canvas)
    {
        DestroyWindow(native->canvas);
        native->canvas = NULL;
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


gboolean
gtk_gl_supports_feature(int feature)
{
    return FALSE;
}
