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

#include <GL/glxew.h>

#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <glib-object.h>

#include <gtkgl/visual.h>
#include <gtkgl/canvas.h>
#include "canvas_impl.h"


// Whether init_glxew has run (only required once)
static gboolean glxew_initialized = FALSE;


/* X Error handling routines:
 * X generates errors asynchronously, and a single error will usually terminate
 * the application. GLX functions may generate X Errors if a non-existing
 * context type was requested or a visual does not match the context - things
 * we cannot detect in advance.
 * To allow the gtk_gl_* functions to handle X Errors gracefully, they can be
 * "muted" via begin_capture_xerrors() / end_capture_xerrors().
 */
typedef int (*XErrorHandler)(Display *dpy, XErrorEvent *ev);
static XErrorHandler old_xerror_handler;
static XErrorEvent last_xerror;
static gboolean have_xerror_flag;
static GMutex xerror_mutex;


static int
silent_xerror_handler(Display *dpy, XErrorEvent *ev) {
    last_xerror = *ev;
    have_xerror_flag = TRUE;
    return 0;
}


static void
begin_capture_xerrors(void) {
    g_mutex_lock(&xerror_mutex);
    have_xerror_flag = FALSE;
    old_xerror_handler = XSetErrorHandler(silent_xerror_handler);
}


static gboolean
have_xerror(Display *dpy) {
    XSync(dpy, False);
    return have_xerror_flag;
}


// Ends muting of X Errors, returning whether there was an error
static gboolean
end_capture_xerrors(Display *dpy) {
    gboolean had_xerror = have_xerror(dpy);
    XSetErrorHandler(old_xerror_handler);
    g_mutex_unlock(&xerror_mutex);
    return had_xerror;
}


/* Creates a dummy context to call glewInit() on in order to initialize
 * GLEW variables and function for using GLX extensions.
 */
static gboolean
init_glxew(Display *dpy, int screen) {
    static int attr_sets[][3] = {
            { GLX_RGBA, GLX_DOUBLEBUFFER, None },
            { GLX_RGBA, None },
            { GLX_DOUBLEBUFFER, None },
            { None }
        };

    int erb, evb;
    XVisualInfo *vi = NULL;
    GLXContext cxt;
    Window wnd;
    size_t i;
    gboolean dummy_created = FALSE;

    if (glxew_initialized) return TRUE;

    assert(dpy);
    begin_capture_xerrors();

    if (!glXQueryExtension(dpy, &erb, &evb) || have_xerror(dpy)) goto end;

    for (i = 0; i < 4; ++i) {
        if ((vi = glXChooseVisual(dpy, screen, attr_sets[i]))
                || have_xerror(dpy)) {
            break;
        }
    }
    if (!vi) goto end;

    if (!(cxt = glXCreateContext(dpy, vi, None, True)) || have_xerror(dpy))
        goto end_vi;
    if (!(wnd = XCreateSimpleWindow(dpy, RootWindow(dpy, screen), 0, 0, 1, 1,
            1, 0, 0)) || have_xerror(dpy)) goto end_cxt;
    if (glXMakeCurrent(dpy, wnd, cxt) || have_xerror(dpy)) {
        dummy_created = TRUE;
        glewExperimental = GL_TRUE;
        if (glewInit() != GLEW_OK) {
            g_warning("Unable to initialize GLXEW");
        } else {
            glxew_initialized = TRUE;
        }
    }
    XDestroyWindow(dpy, wnd);
end_cxt:
    glXDestroyContext(dpy, cxt);
end_vi:
    XFree(vi);
end:
    if (end_capture_xerrors(dpy)) {
        g_warning("Received X window system error during GLXEW initialization");
        return FALSE;
    }
    if (!dummy_created) {
        g_warning("Unable to create dummy context for GLXEW initialization");
        return FALSE;
    }
    return TRUE;
}


// Describes a X visual for create_context / describe_visual.
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
    // Whether the struct has been initialized (in init_native())
    gboolean initialized;

    // "Location" of the GtkGLCanvas X window
    Display *dpy;
    int screen;

    // The GLX window (residing inside the GtkGLCanvas window)
    GLXWindow win;

    // The context once created
    GLXContext glc;

    // The XVisualInfo of the GtkGLCanvas window
    XVisualInfo visual_info;
};


GtkGLCanvas_NativePriv*
gtk_gl_canvas_native_new() {
	return g_malloc0(sizeof(GtkGLCanvas_NativePriv));
}


static gboolean
gtk_gl_canvas_init_native(GtkGLCanvas *canvas) {
	GtkGLCanvas_Priv *priv = GTK_GL_CANVAS_GET_PRIV(canvas);
    GtkGLCanvas_NativePriv *native = priv->native;
    XWindowAttributes xattrs;
    XVisualInfo template, *vi;
    int count;

    if (native->initialized) return TRUE;

    native->dpy = gdk_x11_display_get_xdisplay(gdk_window_get_display(
            priv->win));
	if (!native->dpy) {
        g_warning("Unable to get X11 display");
        return FALSE;
    }

    native->screen = gdk_x11_screen_get_screen_number(gdk_window_get_screen(
            priv->win));

    begin_capture_xerrors();

    // Get the XVIsualInfo of the GtkGLCanvas window in order to compare
    // the visual type later
    XGetWindowAttributes(native->dpy, gdk_x11_window_get_xid(priv->win),
            &xattrs);
    template.visualid = XVisualIDFromVisual(xattrs.visual);
    vi = XGetVisualInfo(native->dpy, VisualIDMask, &template, &count);
    assert(count == 1);
    native->visual_info = *vi;
    XFree(vi);

    if (end_capture_xerrors(native->dpy)) {
        g_warning("Received X window system error during native canvas"
            " initialization");
        return FALSE;
    }

    if (init_glxew(native->dpy, gdk_x11_screen_get_screen_number(
            gdk_window_get_screen(priv->win)))) {
        native->initialized = TRUE;
    }
    return native->initialized;
}


void
gtk_gl_canvas_native_realize(GtkGLCanvas *canvas) {
	GtkGLCanvas_Priv *priv = GTK_GL_CANVAS_GET_PRIV(canvas);
    GtkGLCanvas_NativePriv *native = priv->native;

    native->initialized = FALSE;
    native->dpy = NULL;
    native->screen = 0;
    native->win = 0;
    native->glc = NULL;
}


void
gtk_gl_canvas_native_unrealize(GtkGLCanvas *canvas) {

}


static gboolean
visual_type_matches(int glx, int x) {
    switch (glx) {
        case GLX_TRUE_COLOR: return x == TrueColor;
        case GLX_DIRECT_COLOR: return x == DirectColor;
        case GLX_PSEUDO_COLOR: return x == PseudoColor;
        case GLX_STATIC_COLOR: return x == StaticColor;
        case GLX_GRAY_SCALE: return x == GrayScale;
        case GLX_STATIC_GRAY: return x == StaticGray;
        default: return FALSE;
    }
}

GtkGLVisualList *
gtk_gl_canvas_enumerate_visuals(GtkGLCanvas *canvas) {
	GtkGLCanvas_Priv *priv = GTK_GL_CANVAS_GET_PRIV(canvas);
    GtkGLCanvas_NativePriv *native = priv->native;

    int fbconfig_count;
    GLXFBConfig *fbconfigs;
    GtkGLVisualList *list;
    size_t i, j;

    assert(canvas);

    if (!gtk_gl_canvas_init_native(canvas) || !GLXEW_VERSION_1_3) {
        return gtk_gl_visual_list_new(TRUE, 0);
    }

    begin_capture_xerrors();

    /* Get a list of GLXFBConfigs, check for:
     *   - Ability to render to an X window
     *   - Correct visual type (must match the parent GtkGLCanvas window)
     * and insert matching configs into the visual list
     */
    fbconfigs = glXGetFBConfigs(native->dpy, native->screen, &fbconfig_count);
    list = gtk_gl_visual_list_new(TRUE, fbconfig_count);
    for (i = 0, j = 0; i < list->count; ++i) {
        int targets, vtype;
        glXGetFBConfigAttrib(native->dpy, fbconfigs[i], GLX_DRAWABLE_TYPE,
            &targets);
        glXGetFBConfigAttrib(native->dpy, fbconfigs[i], GLX_X_VISUAL_TYPE,
            &vtype);
        if ((targets & GLX_WINDOW_BIT)
                && visual_type_matches(vtype, native->visual_info.class)) {
            list->entries[j++] = gtk_gl_visual_new(native->dpy, fbconfigs[i]);
        }
    }
    if (end_capture_xerrors(native->dpy)) {
        g_warning("Received X window system error during visual enumeration");
        return gtk_gl_visual_list_new(TRUE, 0);
    }
    list->count = j;
    return list;
}


void
gtk_gl_describe_visual(const GtkGLVisual *visual, GtkGLFramebufferConfig *out) {
    int value;

    assert(visual);
    assert(out);
    assert(glxew_initialized);
    assert(GLXEW_VERSION_1_3);

#define QUERY(attr) \
    (glXGetFBConfigAttrib(visual->dpy, visual->cfg, GLX_##attr, &value), value)

    out->accelerated = TRUE;

    QUERY(RENDER_TYPE);
    out->color_types = (
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

    QUERY(CONFIG_CAVEAT);
    out->caveat
            = value == GLX_SLOW_CONFIG ? GTK_GL_CAVEAT_SLOW
            : value == GLX_NON_CONFORMANT_CONFIG ? GTK_GL_CAVEAT_NONCONFORMANT
            : GTK_GL_CAVEAT_NONE;

#undef QUERY
}


static gboolean
gtk_gl_canvas_native_before_create_context(GtkGLCanvas *canvas,
        const GtkGLVisual *visual) {
    GtkGLCanvas_Priv *priv = GTK_GL_CANVAS_GET_PRIV(canvas);
    GtkGLCanvas_NativePriv *native = priv->native;

    assert(visual);
    assert(glxew_initialized);
    assert(native->initialized);
    assert(GLXEW_VERSION_1_3);

    begin_capture_xerrors();

    /* For each context creation a new GLX window must be constructed - once the
     * visual is pinned down, it cannot be changed. Just using the GtkGLCanvas
     * window would crash upon creating a second context with a different
     * lisual
     */
    native->win = glXCreateWindow(native->dpy, visual->cfg,
            gdk_x11_window_get_xid(priv->win), NULL);
    if (!native->win || have_xerror(native->dpy)) {
        g_warning("glXCreateWindow() failed");
        end_capture_xerrors(native->dpy);
        return FALSE;
    }
    return TRUE;
}


static gboolean
gtk_gl_canvas_native_after_create_context(GtkGLCanvas *canvas,
        const GtkGLVisual *visual) {
    GtkGLCanvas_Priv *priv = GTK_GL_CANVAS_GET_PRIV(canvas);
    GtkGLCanvas_NativePriv *native = priv->native;
    int attrib;
    gboolean failed = FALSE;

    if (!native->glc) return FALSE;

    // Required for deciding between glFlush() and swap_buffers() in
    // display_frame()
    glXGetFBConfigAttrib(native->dpy, visual->cfg, GLX_DOUBLEBUFFER,
            &attrib);
    priv->double_buffered = (unsigned) attrib;

    if (!glXMakeCurrent(native->dpy, native->win, native->glc)) {
        g_warning("glXMakeCurrent() failed after successful context creation");
        failed = TRUE;
    }

    if (end_capture_xerrors(native->dpy)) {
        g_warning("Received X error during context creation");
        failed = TRUE;
    }

    // Do glewInit() again in order to get the correct function pointers
    // for the created context
    if (glewInit() != GLEW_OK) {
        g_warning("glewInit() failed after context creation");
        failed = TRUE;
    }

    if (failed) {
        gtk_gl_canvas_native_destroy_context(canvas);
        native->glc = NULL;
    };
    return !failed;
}


gboolean
gtk_gl_canvas_native_create_context(GtkGLCanvas *canvas,
        const GtkGLVisual *visual) {
    GtkGLCanvas_Priv *priv = GTK_GL_CANVAS_GET_PRIV(canvas);
    GtkGLCanvas_NativePriv *native = priv->native;
    XVisualInfo *vi;

    if (!gtk_gl_canvas_native_before_create_context(canvas, visual)) {
        return FALSE;
    }

    vi = glXGetVisualFromFBConfig(visual->dpy, visual->cfg);
    if (!vi) {
        g_error("Unable to get X visual form GtkGLVisual");
        return FALSE;
    }
    // Create legacy context
    native->glc = glXCreateContext(native->dpy, vi, NULL, GL_TRUE);
    XFree(vi);

    return gtk_gl_canvas_native_after_create_context(canvas, visual);
}


gboolean
gtk_gl_canvas_native_create_context_with_version(GtkGLCanvas *canvas,
        const GtkGLVisual *visual, unsigned ver_major, unsigned ver_minor,
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
    } else if (GLXEW_ARB_create_context) {
        /* (Core) contexts > 3.0 cannot be created via the legacy
         * glXCreateContext because the deprecation functionality requires
         * specification of the target version.
         */
        int attrib_list[] = {
                GLX_CONTEXT_MAJOR_VERSION_ARB, ver_major,
                GLX_CONTEXT_MINOR_VERSION_ARB, ver_minor,
                None, None, None
        };
        /* OpenGL 3.1 does not know about compatibility profiles, so
         * compatibility is checked later via GLEW_ARB_compatibility in that
         * case
         */
        if (GLXEW_ARB_create_context_profile) {
            attrib_list[4] = GLX_CONTEXT_PROFILE_MASK_ARB;
            switch (profile) {
                case GTK_GL_CORE_PROFILE:
                    attrib_list[5] = GLX_CONTEXT_CORE_PROFILE_BIT_ARB;
                    break;

                case GTK_GL_COMPATIBILITY_PROFILE:
                    attrib_list[5] = GLX_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB;
                    break;

                case GTK_GL_ES_PROFILE:
                    if (!GLXEW_EXT_create_context_es_profile) return FALSE;
                    attrib_list[5] = GLX_CONTEXT_ES_PROFILE_BIT_EXT;
                    break;

                default:
                    return FALSE;
            }
        };

        if (!gtk_gl_canvas_native_before_create_context(canvas, visual)) {
            return FALSE;
        }
        native->glc = glXCreateContextAttribsARB(native->dpy, visual->cfg, NULL,
                GL_TRUE, attrib_list);
        if (!gtk_gl_canvas_native_after_create_context(canvas, visual)) {
            return FALSE;
        }
    } else {
        return FALSE;
    }

    /* Verify the correct context version (the legacy glXCreateContext may or
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

    begin_capture_xerrors();

    if (native->glc) {
        // Context is not destroyed until it is no longer current
        glXMakeCurrent(native->dpy, native->win, NULL);
		glXDestroyContext(native->dpy, native->glc);
		native->glc = NULL;
    }
    if (native->win) {
        // See the GLX_MESA_release_buffers docs
        if (GLXEW_MESA_release_buffers) {
            glXReleaseBuffersMESA(native->dpy, native->win);
        }
        glXDestroyWindow(native->dpy, native->win);
        native->win = 0;
    }

    if (end_capture_xerrors(native->dpy)) {
        g_warning("Received X window system error during context destruction");
    }
}


void
gtk_gl_canvas_native_make_current(GtkGLCanvas *canvas) {
	GtkGLCanvas_NativePriv *native = GTK_GL_CANVAS_GET_PRIV(canvas)->native;
    if (native->glc) {
        glXMakeCurrent(native->dpy, native->win, native->glc);
    }
}


void
gtk_gl_canvas_native_swap_buffers(GtkGLCanvas *canvas) {
	GtkGLCanvas_NativePriv *native = GTK_GL_CANVAS_GET_PRIV(canvas)->native;
    if (native->glc) {
        glXSwapBuffers(native->dpy, native->win);
    }
}
