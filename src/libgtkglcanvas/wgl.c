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
#include <gtkgl/ext.h>
#include "canvas_impl.h"

#include <stdlib.h>
#include <assert.h>
#include <windows.h>

#include <gtk/gtk.h>
#include <gdk/gdkwin32.h>
#include <glib-object.h>

#include <epoxy/gl.h>
#include <epoxy/wgl.h>


// Returns a human-readable error message  based GetLastError. The result is
// owned by the caller.
static void
warn_last_error(void) {
	char *ptr;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
            NULL, GetLastError(), 0, (LPSTR) &ptr, 0, NULL);
	g_warning("GetLastError() = %s", ptr);
	HeapFree(GetProcessHeap(), 0, ptr);
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


struct _GtkGLVisual {
	HDC dc;
	gint pf;
};


static GtkGLVisual *
gtk_gl_visual_new(HDC dc, gint pf) {
    GtkGLVisual visual = { dc, pf };
    return g_memdup(&visual, sizeof visual);
}


void
gtk_gl_visual_free(GtkGLVisual *visual) {
    g_free(visual);
}


struct _GtkGLCanvas_NativePriv {
    HWND win;  // The canvas child window
	HDC parent_dc, win_dc;
    HGLRC dummy_glc, glc;
};


GtkGLCanvas_NativePriv*
gtk_gl_canvas_native_new() {
	return g_malloc0(sizeof(GtkGLCanvas_NativePriv));
}


void
gtk_gl_canvas_native_destroy_surface(GtkGLCanvas *canvas) {
	GtkGLCanvas_Priv *priv = GTK_GL_CANVAS_GET_PRIV(canvas);
	GtkGLCanvas_NativePriv *native = priv->native;

	if (native->win) {
		if (native->win_dc) {
            if (!ReleaseDC(native->win, native->win_dc)) {
                warn_last_error();
                g_warning("Unable to release surface DC");
            }
			native->win_dc = NULL;
		}
		if (!DestroyWindow(native->win)) {
            warn_last_error();
            g_warning("Unable to destroy surface window");
        }
		native->win = NULL;
	}
}


void
gtk_gl_canvas_native_resize_surface(GtkGLCanvas *canvas, unsigned width,
        unsigned height) {
    GtkGLCanvas_Priv *priv = GTK_GL_CANVAS_GET_PRIV(canvas);
    GtkGLCanvas_NativePriv *native = priv->native;

    g_assert(native->win);

    SetWindowPos(native->win, NULL, 0, 0, (int) width, (int) height, SWP_NOMOVE);
}


void
gtk_gl_canvas_native_realize(GtkGLCanvas *canvas) {
	GtkGLCanvas_Priv *priv = GTK_GL_CANVAS_GET_PRIV(canvas);
	GtkGLCanvas_NativePriv *native = priv->native;

    HWND hwnd = GDK_WINDOW_HWND(priv->win);
    native->parent_dc = GetDC(hwnd);
    if (!native->parent_dc) {
        warn_last_error();
        g_warning("Unable to get parent DC");
        return;
    }

    PIXELFORMATDESCRIPTOR pfd = { sizeof(PIXELFORMATDESCRIPTOR), 1,
        PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL, PFD_TYPE_RGBA, 32, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, PFD_MAIN_PLANE, 0, 0, 0, 0 };

    int pixel_format = ChoosePixelFormat(native->parent_dc, &pfd);
    if (!pixel_format || !SetPixelFormat(native->parent_dc,
            pixel_format, &pfd)) {
        warn_last_error();
        g_warning("Unable to choose/set dummy OpenGL pixel format");
        return;
    }

    native->dummy_glc = wglCreateContext(native->parent_dc);
    if (!native->dummy_glc) {
        warn_last_error();
        g_warning("Unable to wglCreate dummy context");
        return;
    }
}


void
gtk_gl_canvas_native_unrealize(GtkGLCanvas *canvas) {
	GtkGLCanvas_Priv *priv = GTK_GL_CANVAS_GET_PRIV(canvas);
	GtkGLCanvas_NativePriv *native = priv->native;

    if (native->parent_dc) {
        if (native->dummy_glc) {
            if (!wglMakeCurrent(native->parent_dc, NULL)) {
                warn_last_error();
                g_warning("Unable to detach dummy context");
            }
            if (!wglDeleteContext(native->dummy_glc)) {
                warn_last_error();
                g_warning("Unable to delete dummy context");
            }
            native->dummy_glc = NULL;
        }
        if (!ReleaseDC(GDK_WINDOW_HWND(priv->win), native->parent_dc)) {
            warn_last_error();
            g_warning("Unable to release parent window DC");
        }
        native->parent_dc = NULL;
    }
}


static void
gtk_gl_canvas_native_register_window_class(void) {
    WNDCLASSEX klass = { sizeof(WNDCLASSEX), CS_OWNDC, DefWindowProc, 0, 0,
            GetModuleHandle(NULL), NULL, NULL, NULL, NULL, "GtkGLCanvas",
            NULL };

    if (!RegisterClassEx(&klass)) {
        warn_last_error();
        g_warning("Unable to register surface window class");
    }
}


gboolean
gtk_gl_canvas_native_create_surface(GtkGLCanvas *canvas, const GtkGLVisual *visual) {
	GtkGLCanvas_Priv *priv = GTK_GL_CANVAS_GET_PRIV(canvas);
	GtkGLCanvas_NativePriv *native = priv->native;

    gtk_gl_canvas_native_register_window_class();

	HWND parent = GDK_WINDOW_HWND(priv->win);
	RECT client_rect;
    GetClientRect(parent, &client_rect);

	native->win = CreateWindowEx(0, "GtkGLCanvas", "", WS_VISIBLE | WS_CHILD,
        	client_rect.left, client_rect.top, client_rect.right,
        	client_rect.bottom-1, parent, NULL,
        	GetModuleHandle(NULL), NULL);
	if (!native->win) {
        warn_last_error();
		g_warning("Unable to create child window for canvas");
		goto fail;
	}

	native->win_dc = GetDC(native->win);
	if (!native->win_dc) {
        warn_last_error();
		g_warning("Unable to get DC for canvas child window");
		goto fail;
	}

	if (!SetPixelFormat(native->win_dc, visual->pf, NULL)) {
        warn_last_error();
		g_warning("Unable to set pixel format");
        goto fail;
	}

    return TRUE;

fail:
    gtk_gl_canvas_native_destroy_context(canvas);
    return FALSE;
}


#define MAX_NFORMATS 1000

GtkGLVisualList *
gtk_gl_canvas_enumerate_visuals(GtkGLCanvas *canvas) {
	static const gint iattribs[] = {
	 		WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
			WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
			WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB, // TODO
			0
	};
	static const float fattribs[] = { 0 };

	GtkGLCanvas_Priv *priv = GTK_GL_CANVAS_GET_PRIV(canvas);
	GtkGLCanvas_NativePriv *native = priv->native;
	gint *formats;
	UINT n_formats, i;
	GtkGLVisualList *list;

    HWND hwnd = GDK_WINDOW_HWND(priv->win);

	if (!native->parent_dc || !native->dummy_glc) {
		g_warning("Canvas does not have a dummy context, returning empty visual list");
		return gtk_gl_visual_list_new(FALSE, 0);
	}

    if (!wglMakeCurrent(native->parent_dc, native->dummy_glc)) {
        g_warning("Unable to make dummy context current, returning empty visual list");
		return gtk_gl_visual_list_new(FALSE, 0);
	}

	formats = g_malloc(MAX_NFORMATS * sizeof *formats);
	if (!epoxy_has_wgl_extension(native->parent_dc, "WGL_ARB_pixel_format")
            || !wglChoosePixelFormatARB(native->parent_dc, iattribs, fattribs,
            MAX_NFORMATS, formats, &n_formats)) {
		warn_last_error();
		g_warning("Unable to wglChoosePixelFormatARB(), returning "
				"empty visual list");
		return gtk_gl_visual_list_new(FALSE, 0);
	}

	list = gtk_gl_visual_list_new(TRUE, n_formats);
	for (i = 0; i < n_formats; ++i) {
		list->entries[i] = gtk_gl_visual_new(native->parent_dc, formats[i]);
	}
	return list;
}


void
gtk_gl_describe_visual(const GtkGLVisual *visual, GtkGLFramebufferConfig *out) {
    gint value;
	gint attr;
	gboolean ok;

    assert(visual);
    assert(out);

#define QUERY(a) \
    (attr = WGL_##a##_ARB, ok = wglGetPixelFormatAttribivARB(visual->dc, \
			visual->pf, 0, 1, &attr, &value), ok ? value : 0)

    out->accelerated = QUERY(ACCELERATION);

	out->color_types = QUERY(PIXEL_TYPE) == WGL_TYPE_RGBA_ARB
			? GTK_GL_COLOR_RGBA : GTK_GL_COLOR_INDEXED;

    out->color_bpp = QUERY(COLOR_BITS);
    out->fb_level = 0;
    out->double_buffered = QUERY(DOUBLE_BUFFER);
    out->stereo_buffered = QUERY(STEREO);
    out->aux_buffers = QUERY(AUX_BUFFERS);
    out->red_color_bpp = QUERY(RED_BITS);
    out->green_color_bpp = QUERY(GREEN_BITS);
    out->blue_color_bpp = QUERY(BLUE_BITS);
    out->alpha_color_bpp = QUERY(ALPHA_BITS);
    out->depth_bpp = QUERY(DEPTH_BITS);
    out->stencil_bpp = QUERY(STENCIL_BITS);
    out->red_accum_bpp = QUERY(ACCUM_RED_BITS);
    out->green_accum_bpp = QUERY(ACCUM_GREEN_BITS);
    out->blue_accum_bpp = QUERY(ACCUM_BLUE_BITS);
    out->alpha_accum_bpp = QUERY(ACCUM_ALPHA_BITS);

  	out->transparent_type = QUERY(TRANSPARENT) ? (
				out->color_types == GTK_GL_COLOR_RGBA
				? GTK_GL_TRANSPARENT_RGB : GTK_GL_TRANSPARENT_INDEX
			) : GTK_GL_TRANSPARENT_NONE;

    out->transparent_index = QUERY(TRANSPARENT_INDEX_VALUE);
    out->transparent_red = QUERY(TRANSPARENT_RED_VALUE);
    out->transparent_green = QUERY(TRANSPARENT_GREEN_VALUE);
    out->transparent_blue = QUERY(TRANSPARENT_BLUE_VALUE);
    out->transparent_alpha = QUERY(TRANSPARENT_ALPHA_VALUE);

    if (!epoxy_has_wgl_extension(visual->dc, "WGL_ARB_multisample")) {
        out->sample_buffers = QUERY(SAMPLE_BUFFERS);
        out->samples_per_pixel = QUERY(SAMPLES);
    } else {
        out->sample_buffers = out->samples_per_pixel = 0;
    }

	out->caveat = GTK_GL_CAVEAT_NONE;

#undef QUERY
}


static gboolean
gtk_gl_canvas_native_after_create_context(GtkGLCanvas *canvas,
        const GtkGLVisual *visual) {
	GtkGLCanvas_Priv *priv = GTK_GL_CANVAS_GET_PRIV(canvas);
	GtkGLCanvas_NativePriv *native = priv->native;
	gint attr, value;

	if(!native->glc) {
		warn_last_error();
		g_warning("Unable to create GL context");
		return FALSE;
	}

	if (!wglMakeCurrent(native->win_dc, native->glc)) {
		warn_last_error();
		g_warning("Unable to attach context");
		gtk_gl_canvas_native_destroy_context(canvas);
		return FALSE;
	}

	attr = WGL_DOUBLE_BUFFER_ARB;
	wglGetPixelFormatAttribivARB(visual->dc, visual->pf, 0, 1, &attr, &value);
	priv->double_buffered = !!value;
	return TRUE;
}


static gboolean
gtk_gl_canvas_native_before_create_context(GtkGLCanvas *canvas,
        const GtkGLVisual *visual) {
	GtkGLCanvas_Priv *priv = GTK_GL_CANVAS_GET_PRIV(canvas);
	GtkGLCanvas_NativePriv *native = priv->native;

    native->win_dc = GetDC(GDK_WINDOW_HWND(priv->win));
    if (!native->win_dc) {
        warn_last_error();
        g_warning("Unable to get DC for surface");
        return FALSE;
    }
    return TRUE;
}


gboolean
gtk_gl_canvas_native_create_context(GtkGLCanvas *canvas,
        const GtkGLVisual *visual) {
	GtkGLCanvas_Priv *priv = GTK_GL_CANVAS_GET_PRIV(canvas);
	GtkGLCanvas_NativePriv *native = priv->native;

    if (!gtk_gl_canvas_native_before_create_context(canvas, visual)) {
        return FALSE;
    }
    native->glc = wglCreateContext(native->win_dc);
    return gtk_gl_canvas_native_after_create_context(canvas, visual);
}


gboolean
gtk_gl_canvas_native_create_context_with_version(GtkGLCanvas *canvas,
        const GtkGLVisual *visual, guint ver_major, guint ver_minor,
        GtkGLProfile profile) {
    GtkGLCanvas_Priv *priv = GTK_GL_CANVAS_GET_PRIV(canvas);
    GtkGLCanvas_NativePriv *native = priv->native;

    if (!gtk_gl_canvas_native_before_create_context(canvas, visual)) {
        return FALSE;
    }

    const char *cxt_version;
    guint cxt_major, cxt_minor;


    if ((ver_major < 3 || (ver_major == 3 && ver_minor == 0))
            && (profile == GTK_GL_CORE_PROFILE
                || profile == GTK_GL_COMPATIBILITY_PROFILE)) {
        // Use legacy function for legacy contexts
        if (!gtk_gl_canvas_native_create_context(canvas, visual)) {
            return FALSE;
        }
    } else if (epoxy_has_wgl_extension(visual->dc, "WGL_ARB_create_context")) {
        /* (Core) contexts > 3.0 cannot be created via the legacy
         * wglCreateContext because the deprecation functionality requires
         * specification of the target version.
         */
        gint attrib_list[] = {
                WGL_CONTEXT_MAJOR_VERSION_ARB, ver_major,
                WGL_CONTEXT_MINOR_VERSION_ARB, ver_minor,
                0, 0, 0
        };
        /* OpenGL 3.1 does not know about compatibility profiles, so
         * compatibility is checked later via WGL_ARB_compatibility in that
         * case
         */
        if (epoxy_has_wgl_extension(visual->dc, "WGL_ARB_create_context_profile")) {
            attrib_list[4] = WGL_CONTEXT_PROFILE_MASK_ARB;
            switch (profile) {
                case GTK_GL_CORE_PROFILE:
                    attrib_list[5] = WGL_CONTEXT_CORE_PROFILE_BIT_ARB;
                    break;

                case GTK_GL_COMPATIBILITY_PROFILE:
                    attrib_list[5] = WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB;
                    break;

                case GTK_GL_ES_PROFILE:
                    if (!epoxy_has_wgl_extension(visual->dc,
                            "WGL_ARB_create_context_es_profile")) {
                        return FALSE;
                    }
                    attrib_list[5] = WGL_CONTEXT_ES_PROFILE_BIT_EXT;
                    break;

                default:
                    return FALSE;
            }
        };

        native->glc = wglCreateContextAttribsARB(native->win_dc, NULL,
                attrib_list);
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
    int version = epoxy_gl_version();
    if (version < ver_major * 10 + ver_minor) return FALSE;
    if (version == 31 && profile == GTK_GL_COMPATIBILITY_PROFILE
            && !epoxy_has_gl_extension("GL_ARB_compatibility")) {
        return FALSE;
    }
    return TRUE;
}


void
gtk_gl_canvas_native_destroy_context(GtkGLCanvas *canvas) {
	GtkGLCanvas_Priv *priv = GTK_GL_CANVAS_GET_PRIV(canvas);
	GtkGLCanvas_NativePriv *native = priv->native;

    if (!native->glc) return;
	assert(native->win_dc);

	if (!wglMakeCurrent(native->win_dc, NULL)) {
        warn_last_error();
        g_warning("Unable to detach context");
    }

	if (!wglDeleteContext(native->glc)) {
        warn_last_error();
        g_warning("Unable to delete context");
    }

	native->glc = NULL;
}


void
gtk_gl_canvas_native_make_current(GtkGLCanvas *canvas) {
	GtkGLCanvas_Priv *priv = GTK_GL_CANVAS_GET_PRIV(canvas);
    GtkGLCanvas_NativePriv *native = priv->native;
	if (native->glc) {
		assert(native->win_dc);
    	if (!wglMakeCurrent(native->win_dc, native->glc)) {
            warn_last_error();
            g_warning("Unable to make context current");
        }
	}
}


void
gtk_gl_canvas_native_swap_buffers(GtkGLCanvas *canvas) {
	GtkGLCanvas_Priv *priv = GTK_GL_CANVAS_GET_PRIV(canvas);
    GtkGLCanvas_NativePriv *native = priv->native;
	if (native->glc) {
		assert(native->win_dc);
        if (!wglSwapLayerBuffers(native->win_dc, WGL_SWAP_MAIN_PLANE)) {
            warn_last_error();
            g_warning("Unable to swap buffers");
        }
	}
}


GtkGLProc *
gtk_gl_get_proc_address(const char *name) {
    return (GtkGLProc*) wglGetProcAddress(name);
}

