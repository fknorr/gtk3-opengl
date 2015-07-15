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

#include <gtkgl/canvas.h>
#include "canvas_impl.h"
#include <stdlib.h>
#include <windows.h>
#include <gtk/gtk.h>
#include <gdk/gdkwin32.h>
#include <glib-object.h>
#include <GL/gl.h>
#include <GL/glew.h>
#include <GL/wglew.h>


// Returns a human-readable error message  based GetLastError. The result is
// owned by the caller.
static void
warn_last_error(void) {
	char *ptr;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
            NULL, GetLastError(), 0, (LPSTR) &ptr, 0, NULL);
	g_warning("GetLastError() = %s", ptr);
	HeapFree(GetProcessHeap(), ptr);
}


// Custom WndProc for the canvas child window, making it transparent to mouse
// interaction
static LRESULT CALLBACK
w32_canvas_wnd_proc(HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam) {
    GtkWidget *widget;
	switch(msg) {
            case WM_NCHITTEST:
            return HTTRANSPARENT;

        default:
            return DefWindowProc(hWnd, msg, wParam, lParam);
    }
}


static gboolean wglew_initialized = FALSE;


// Creates the child-window class and resolves wglChoosePixelFormatARB.
// Global, a noop except for the first call, and thread-safe.
static gboolean
init_wglew(void) {
    WNDCLASSEX wnd_class;
    HWND dummy;
    gboolean success = FALSE;
    HDC dc;
    HGLRC glc;
	PIXELFORMATDESCRIPTOR pfd;
	int pf;

	if (wglew_initialized) return TRUE;

	// Register class for all GLCanvas windows
    ZeroMemory(&wnd_class, sizeof(WNDCLASSEX));
    wnd_class.cbSize = sizeof(WNDCLASSEX);
    wnd_class.hbrBackground = (HBRUSH) GetStockObject(BLACK_BRUSH);
    wnd_class.hCursor = LoadCursor(NULL,IDC_ARROW);
    wnd_class.hInstance = GetModuleHandle(NULL);
    wnd_class.lpfnWndProc = w32_canvas_wnd_proc;
    wnd_class.lpszClassName = "GLCanvas";
    wnd_class.style = CS_OWNDC; // WGL requires an owned DC

    if (!RegisterClassEx(&wnd_class)) {
		g_warning("Unable to register window class in WGLEW initialization");
		goto end;
	}

    // Create an invisible window with a dummy GL context in order to
    // be able to call glewInit()
    dummy = CreateWindowEx(0, "GLCanvas", "", 0, 0, 0, 10,
			10, NULL, NULL, GetModuleHandle(NULL), NULL);
	if (!dummy) {
		g_warning("Unable to create dummy window in WGLEW initialization");
		goto end;
	}

	// Create a GL context
    if (!(dc = GetDC(dummy))) {
		g_warning("Unable to retrieve device context in WGLEW initialization");
		goto free_window;
	}

	ZeroMemory(&pfd, sizeof pfd);
	pfd.nSize = sizeof pfd;
	pfd.nVersion = 1;
	pfd.dfFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL;
	pfd.iPixelType = PFD_TYPE_RGBA;
	if (!((pf = ChoosePixelFormat(dc, &pfd)) && SetPixelFormat(dc, pf, &pfd))) {
		g_warning("Unable to set pixel format in WGLEW initialization");
		goto free_dc;
	}

	if (!(glc = wglCreateContext(dc))) {
		g_warning("Unable to create GL context in WGLEW initialization");
		goto free_dc;
	}

	if (!wglMakeCurrent(dc, gl)) {
		g_warning("Unable to attach context in WGLEW initialization");
		goto free_glc;
	}

	if (glewInit() == GLEW_OK) {
		wglew_initialized = TRUE;
	} else {
		g_warning("glewInit() failed in WGLEW initialization");
	}

free_glc:
    wglMakeCurrent(dc, NULL);
    wglDeleteContext(glc);

free_dc:
    ReleaseDC(dummy, dc);

free_window:
    DestroyWindow(dummy);

end:
	if (!wglew_initialized) warn_last_error();
	return wglew_initialized;
}


struct _GtkGLVisual {
	int pf;
};


static GtkGLVisual *
gtk_gl_visual_new(int pf) {
    GtkGLVisual visual = { pf };
    return g_memdup(&visual, sizeof visual);
}


void
gtk_gl_visual_free(GtkGLVisual *visual) {
    g_free(visual);
}


struct _GtkGLCanvas_NativePriv {
    HWND win;  // The canvas child window
	HDC dc;
    HGLRC glc;
};


GtkGLCanvas_NativePriv*
gtk_gl_canvas_native_new() {
	return g_malloc0(sizeof(GtkGLCanvas_NativePriv));
}


// Sets a DC's pixel format via wglChoosePixelFromatARB if available
static gboolean
set_pixel_format_arb(HDC dc, const GtkGLAttributes *attrs) {
	GLint iattribs[] = {
		WGL_DOUBLE_BUFFER_ARB,
            ((attrs->flags & GTK_GL_DOUBLE_BUFFERED) ? GL_TRUE : GL_FALSE),
		WGL_STEREO_ARB, ((attrs->flags & GTK_GL_STEREO) ? GL_TRUE : GL_FALSE),
		WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
		WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
		WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
		WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
		WGL_COLOR_BITS_ARB, attrs->color_buffer_bits,
		WGL_DEPTH_BITS_ARB, attrs->depth_buffer_bits,
		WGL_STENCIL_BITS_ARB, attrs->stencil_buffer_bits,
		WGL_SAMPLE_BUFFERS_ARB,
            ((attrs->flags & GTK_GL_SAMPLE_BUFFERS) ? GL_TRUE : GL_FALSE),
 		WGL_SAMPLES_ARB, attrs->num_samples,
		0
	};

	GLuint n_formats;
    GLint format;
	GLfloat fattribs[] = {0};

    return dyn_wglChoosePixelFormatARB
        && dyn_wglChoosePixelFormatARB(dc, iattribs, fattribs, 1,
                &format, &n_formats)
        && n_formats
        && SetPixelFormat(dc, format, NULL);
}


// Event handler resizing the canvas child window when the canvas itself is
// resized
static void
on_size_allocate(GtkWidget *widget, GdkRectangle *rect, gpointer user) {
	GtkGLCanvas_Priv *priv = GTK_GL_CANVAS_GET_PRIV(GTK_GL_CANVAS(widget));

    // When the child window fills the widget, the widget does not receive
    // paint events. As a workaround, the child is made 1px smaller than
    // the parent. This is a workaround that should be resolved correctly.
    if (priv->native->win) {
        SetWindowPos(priv->native->win, NULL, 0, 0, rect->width,
                rect->height-1, 0);
    }
}


static void
destroy_child_window(GtkGLCanvas *canvas) {
	GtkGLCanvas_Priv *priv = GTK_GL_CANVAS_GET_PRIV(canvas);
	GtkGLCanvas_NativePriv *native = priv->native;

	if (native->dc) {
		ReleaseDC(native->dc);
		native->dc = NULL;
	}
	if (native->win) {
		DestroyWindow(native->win);
		native->win = NULL;
	}
}


static void
create_child_window(GtkGLCanvas *canvas) {
	GtkGLCanvas_Priv *priv = GTK_GL_CANVAS_GET_PRIV(canvas);
	GtkGLCanvas_NativePriv *native = priv->native;
	HWND parent;
	RECT client_rect;

	hwnd = GDK_WINDOW_HWND(gtk_widget_get_window(GTK_WIDGET(canvas)));
    GetClientRect(hwnd, &client_rect);
	native->win = CreateWindowEx(0, "GLCanvas", "", WS_VISIBLE | WS_CHILD,
        	client_rect.left, client_rect.top, client_rect.right,
        	client_rect.bottom-1, /*See on_size_allocate*/, parent, NULL,
        	GetModuleHandle(NULL), NULL);
	if (!native->win) {
		g_warning("Unable to create child window for canvas");
		warn_last_error();
		return;
	}

	native->dc = GetDC(native->win);
	if (!native->dc) {
		g_warning("Unable to get DC for canvas child window");
		warn_last_error();
		destroy_child_window();
	}
}



void
gtk_gl_canvas_native_realize(GtkGLCanvas *canvas) {
	native->win = NULL;
	native->dc = NULL;
	native->glc = NULL;
    g_signal_connect(GTK_WIDGET(canvas), "size-allocate",
        G_CALLBACK(on_size_allocate), NULL);
}


void
gtk_gl_canvas_native_unrealize(GtkGLCanvas *canvas) {
	destroy_child_window(canvas);
}


#define MAX_NFORMATS 1000

GtkGLVisualList *
gtk_gl_canvas_enumerate_visuals(GtkGLCanvas *canvas) {
	static const int iattribs[] = {
	 		WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
			WGL_SUPPORT_OPENGL_ARB, GL_TRUE
	};
	static const float fattribs[] = { 0 };

	GtkGLCanvas_Priv *priv = GTK_GL_CANVAS_GET_PRIV(canvas);
	GtkGLCanvas_NativePriv *native = priv->native;
	int *formats;
	UINT n_formats, i;
	GtkGLVisualList *list;

	if (!wglew_init()) {
		g_warning("Unable to initialize WGLEW, returning empty visual list");
		return gtk_gl_visual_list_new(FALSE, 0);
	}

	if (!native->win) {
		create_child_window(canvas);
	}

	if (!native->dc) {
		g_warning("Canvas does not have a child window DC, returning "
				"empty visual list");
		return gtk_gl_visual_list_new(FALSE, 0);
	}

	formats = g_malloc(MAX_NFORMATS * sizeof *formats);
	if (!WGLEW_ARB_pixel_format || !wglChoosePixelFormatARB(native->dc,
			iattribs, fattribs, MAX_NFORMATS, formats, &n_formats)) {
		g_warning("Unable to wglChoosePixelFormatARB(), returning "
				"empty visual list");
		warn_last_error();
		return gtk_gl_visual_list_new(FALSE, 0);
	}

	list = gtk_gl_visual_list_new(TRUE, n_formats);
	for (i = 0; i < n_formats; ++i) {
		list->entries[i] = gtk_gl_visual_new(formats[i]);
	}
	return list;
}


static gboolean
gtk_gl_canvas_native_before_create_context(GtkGLCanvas *canvas,
        GtkGLVisual *visual) {
	GtkGLCanvas_Priv *priv = GTK_GL_CANVAS_GET_PRIV(canvas);
	GtkGLCanvas_NativePriv *native = priv->native;
	assert(visual);

    if (wglew_initialized) {
		g_warning("WGLEW not initialized, aborting context creation");
		return FALSE;
	}

	if (!native->win) {
		create_child_window(canvas);
	}

	if (!native->dc) {
		g_warning("Canvas does not have a child window DC, aborting context "
				"creation");
		return FALSE;
	}

	if (!SetPixelFormat(native->dc, visual->pf, NULL)) {
		g_warning("Unable to set pixel format, aborting context creation");
		warn_last_error();
		return FALSE;
	}
	return TRUE;
}


static gboolean
gtk_gl_canvas_native_after_create_context(GtkGLCanvas *canvas,
        GtkGLVisual *visual) {
	GtkGLCanvas_Priv *priv = GTK_GL_CANVAS_GET_PRIV(canvas);
	GtkGLCanvas_NativePriv *native = priv->native;

	if(!native->glc) {
		g_warning("Unable to create GL context");
		warn_last_error();
		return FALSE;
	}

	if (wglMakeCurrent(native->dc, native->glc)) {
		g_warning("Unable to attach context");
		warn_last_error();
		gtk_gl_canvas_native_destroy_context(canvas);
		return FALSE;
	}
	return TRUE;
}


gboolean
gtk_gl_canvas_native_create_context(GtkGLCanvas *canvas,
        const GtkGLVisual *visual) {
	GtkGLCanvas_Priv *priv = GTK_GL_CANVAS_GET_PRIV(canvas);
	GtkGLCanvas_NativePriv *native = priv->native;
	gboolean success = FALSE;

	if (!gtk_gl_canvas_native_before_create_context(canvas, visual)) {
		return FALSE;
	}
	native->glc = wglCreateContext(dc);
	return gtk_gl_canvas_native_after_create_context(canvas, visual);
}


gboolean
gtk_gl_canvas_native_create_context_with_version(GtkGLCanvas *canvas,
        GtkGLVisual *visual, unsigned ver_major, unsigned ver_minor,
        GtkGLProfile profile) {
    GtkGLCanvas_Priv *priv = GTK_GL_CANVAS_GET_PRIV(canvas);
    GtkGLCanvas_NativePriv *native = priv->native;
    const char *cxt_version;
    unsigned cxt_major, cxt_minor;

    if ((ver_major < 3 || (ver_major == 3 && ver_minor == 0))
            && (profile == GTK_GL_CORE_PROFILE
                || profile == GTK_GL_COMPATIBILITY_PROFILE)) {
        // Use legacy function for legacy contexts
        if (!gtk_gl_canvas_native_create_context(canvas, visual)) {
            return FALSE;
        }
    } else if (WGLEW_ARB_create_context) {
        /* (Core) contexts > 3.0 cannot be created via the legacy
         * wglCreateContext because the deprecation functionality requires
         * specification of the target version.
         */
        int attrib_list[] = {
                WGL_CONTEXT_MAJOR_VERSION_ARB, ver_major,
                WGL_CONTEXT_MINOR_VERSION_ARB, ver_minor,
                0, 0, 0
        };
        /* OpenGL 3.1 does not know about compatibility profiles, so
         * compatibility is checked later via GLEW_ARB_compatibility in that
         * case
         */
        if (WGL_ARB_create_context_profile) {
            attrib_list[4] = WGL_CONTEXT_PROFILE_MASK_ARB;
            switch (profile) {
                case GTK_GL_CORE_PROFILE:
                    attrib_list[5] = WGL_CONTEXT_CORE_PROFILE_BIT_ARB;
                    break;

                case GTK_GL_COMPATIBILITY_PROFILE:
                    attrib_list[5] = WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB;
                    break;

                case GTK_GL_ES_PROFILE:
                    if (!WGL_EXT_create_context_es_profile) return FALSE;
                    attrib_list[5] = WGL_CONTEXT_ES_PROFILE_BIT_EXT;
                    break;

                default:
                    return FALSE;
            }
        };

        if (!gtk_gl_canvas_native_before_create_context(canvas, visual)) {
            return FALSE;
        }
        native->glc = wglCreateContextAttribsARB(native->dc, NULL, attrib_list
        if (!gtk_gl_canvas_native_after_create_context(canvas, visual)) {
            return FALSE;
        }
    } else {
        return FALSE;
    }

    /* Verify the correct context version (the legacy wglCreateContext may or
     * may not return a context with the required version, and an OpenGL 3.1
     * context may not support the compatibility mode even if requested (see
     * above).
     */
    cxt_version = (const char*) glGetString(GL_VERSION);
    if (sscanf(cxt_version, "%u.%u", &cxt_major, &cxt_minor) == 2) {
        if (cxt_major < ver_major
                || (cxt_major == ver_major && cxt_minor < ver_minor)) {
            return FALSE;
        }
        if (cxt_major == 3 && cxt_minor == 1
                && profile == GTK_GL_COMPATIBILITY_PROFILE
                && !GLEW_ARB_compatibility) {
            return FALSE;
        }
        return TRUE;
    }

    gtk_gl_canvas_native_destroy_context(canvas);
    return FALSE;
}


void
gtk_gl_canvas_native_destroy_context(GtkGLCanvas *canvas) {
	GtkGLCanvas_Priv *priv = GTK_GL_CANVAS_GET_PRIV(canvas);
	GtkGLCanvas_NativePriv *native = priv->native;

    if (!native->glc) return;
	assert(native->dc);

	wglMakeCurrent(native->dc, NULL);
	wglDeleteContext(native->glc);
	native->glc = NULL;

	destroy_child_window(canvas);
}


void
gtk_gl_canvas_native_make_current(GtkGLCanvas *canvas) {
	GtkGLCanvas_Priv *priv = GTK_GL_CANVAS_GET_PRIV(canvas);
    GtkGLCanvas_NativePriv *native = priv->native;
	if (native->glc) {
		assert(native->dc);
    	wglMakeCurrent(native->dc, native->glc);
	}
}


void
gtk_gl_canvas_native_swap_buffers(GtkGLCanvas *canvas) {
	GtkGLCanvas_Priv *priv = GTK_GL_CANVAS_GET_PRIV(canvas);
    GtkGLCanvas_NativePriv *native = priv->native;
	if (native->glc) {
		assert(native->dc);
    	wglSwapLayerBuffers(native->dc, WGL_SWAP_MAIN_PLANE);
	}
}
