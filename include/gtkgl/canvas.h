/*
 * Copyright (c) 2014-2015, Fabian Knorr
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

/*
 * This file is based on the work of André Diego Piske,
 * see <https://github.com/andrepiske/tegtkgl>. To retain André's licensing
 * conditions on the parts of the software authored by him, the following
 * copyright notice shall be included in this and all derived files:
 */

/*
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
 */


#pragma once

#include <gtk/gtk.h>
#include "visual.h"


/**
 * SECTION:canvas
 * @Title: GtkGLCanvas
 * @Short_Description: The GtkGLCanvas widget type
 *
 * A #GtkGLCanvas is a widget providing an OpenGL drawing surface. It can be
 * queried for its supported Visuals via #gtk_gl_canvas_enumerate_visuals(),
 * which can the be used to create an OpenGL rendering context inside the
 * canvas widget via #gtk_gl_canvas_create_context() . Legacy versions are
 * supported as well as modern contexts with using version and profile
 * specifications in #gtk_gl_canvas_create_context_with_version() .
 */

G_BEGIN_DECLS


#define GTK_GL_TYPE_CANVAS (gtk_gl_canvas_get_type())
#define GTK_GL_CANVAS(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), \
        GTK_GL_TYPE_CANVAS, GtkGLCanvas))
#define GTK_GL_IS_CANVAS(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), \
        GTK_GL_TYPE_CANVAS))
#define GTK_GL_CANVAS_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), \
        GTK_GL_TYPE_CANVAS, GtkGLCanvasClass))
#define GTK_GL_IS_CANVAS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), \
        GTK_GL_TYPE_CANVAS_CLASS))
#define GTK_GL_CANVAS_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS((obj), \
        GTK_GL_TYPE_CANVAS, GtkGLCanvasClass))


/**
 * GtkGLProfile:
 * @GTK_GL_CORE_PROFILE: The core profile, only supporting non-deprecated
 *      features
 * @GTK_GL_COMPATIBILITY_PROFILE: The compatibility profile, supporting both
 *      non-deprecated and deprecated features (not available for all OpenGL
 *      versions, depending on the driver)
 * @GTK_GL_ES_PROFILE: The OpenGL ES profile, supporting only features in the
 *      OpenGL ES specification. This is a subset of #GTK_GL_CORE_PROFILE
 *
 * OpenGL profiles as used in OpenGL versions > 3.0.
 */
typedef enum _GtkGLProfile {
    GTK_GL_CORE_PROFILE,
    GTK_GL_COMPATIBILITY_PROFILE,
    GTK_GL_ES_PROFILE
} GtkGLProfile;


/**
 * GtkGLCanvas:
 * The GLCanvas Widget type.
 */
typedef struct _GtkGLCanvas GtkGLCanvas;


/**
 * GtkGLCanvasClass:
 * Type information for #GtkGLCanvas
 */
typedef struct _GtkGLCanvasClass GtkGLCanvasClass;


GType gtk_gl_canvas_get_type(void);


/**
 * gtk_gl_canvas_new:
 *
 * Creates a new context-less canvas.
 */
GtkWidget *gtk_gl_canvas_new(void);


/**
 * gtk_gl_canvas_enumerate_visuals:
 * @canvas: The canvas
 *
 * Enumerates all visuals supported for context creation on this canvas.
 * If querying this information fails, an empty list is returned.
 *
 * Use #gtk_gl_choose_visuals() to filter and sort the visuals returned by this
 * function.
 *
 * Returns: A list of supported visuals
 */
GtkGLVisualList *gtk_gl_canvas_enumerate_visuals(GtkGLCanvas *canvas);


/**
 * gtk_gl_canvas_create_context:
 * @canvas: The canvas
 * @visual: The visual selecting the frame buffer configuration
 *
 * Creates a new OpenGL context on a #GtkGLCanvas, making the context
 * current to the calling thread. If the canvas already has a context, this
 * will fail.
 *
 * This function can not create OpenGL contexts > 3.0 reliably on all devices
 * as version 3.1 introduces deprecation, which requires explicit version
 * number handling - If you need recent OpenGL funcionality, use
 * #gtk_gl_canvas_create_context_with_version() instead.
 *
 * Returns: Whether the context was created successfully
 */
gboolean gtk_gl_canvas_create_context(GtkGLCanvas *canvas,
        const GtkGLVisual *visual);


/**
 * gtk_gl_canvas_create_context_with_version:
 * @canvas: The canvas
 * @visual: The visual selecting the frame buffer configuration
 * @ver_major: The OpenGL major version, e.g. 3
 * @ver_minor: The OpenGL minor version, e.g. 1
 * @profile: The OpenGL profile (ignored if the version is < 3.1)
 *
 * Creates a new OpenGL context on a #GtkGLCanvas, making the context
 * current to the calling thread. If the canvas already has a context, this
 * will fail.
 *
 * Unlike #gtk_gl_canvas_create_context(), this function allows the user to
 * specify the desired GL version and profile. Use this to create contexts
 * with an OpenGL version > 3.0.
 *
 * Returns: Whether the context was created successfully
 */
gboolean gtk_gl_canvas_create_context_with_version(GtkGLCanvas *canvas,
        const GtkGLVisual *visual, guint ver_major, guint ver_minor,
        GtkGLProfile profile);


/**
 * gtk_gl_canvas_auto_create_context:
 * @canvas: The canvas
 * @requirements: An array of #GtkGLRequirement, terminated by #GTK_GL_LIST_END
 *
 * Chooses a minimal #GtkGLVisual according to the requirements pareameter and
 * creates a new OpenGL context on a #GtkGLCanvas, making the context
 * current to the calling thread. If the canvas already has a context, this
 * will fail.
 *
 * For finer framebuffer control, use #gtk_gl_canvas_enumerate_visuals and
 * #gtk_gl_canvas_create_context() .
 *
 * This function can not create OpenGL contexts > 3.0 reliably on all devices
 * as version 3.1 introduces deprecation, which requires explicit version
 * number handling - If you need recent OpenGL funcionality, use
 * #gtk_gl_canvas_auto_create_context_with_version() instead.
 *
 * Returns: Whether the context was created successfully
 */
gboolean gtk_gl_canvas_auto_create_context(GtkGLCanvas *canvas,
        const GtkGLRequirement *requirements);


/**
 * gtk_gl_canvas_auto_create_context_with_version:
 * @canvas: The canvas
 * @requirements: An array of #GtkGLRequirement, terminated by #GTK_GL_LIST_END
 * @ver_major: The OpenGL major version, e.g. 3
 * @ver_minor: The OpenGL minor version, e.g. 1
 * @profile: The OpenGL profile (ignored if the version is < 3.1)
 *
 * Chooses a minimal #GtkGLVisual according to the requirements pareameter and
 * creates a new OpenGL context on a #GtkGLCanvas, making the context
 * current to the calling thread. If the canvas already has a context, this
 * will fail.
 *
 * For finer framebuffer control, use #gtk_gl_canvas_enumerate_visuals and
 * #gtk_gl_canvas_create_context_with_version() .
 *
 * Unlike #gtk_gl_canvas_create_context(), this function allows the user to
 * specify the desired GL version and profile. Use this to create contexts
 * with an OpenGL version > 3.0.
 *
 * Returns: Whether the context was created successfully
 */
gboolean gtk_gl_canvas_auto_create_context_with_version(GtkGLCanvas *canvas,
        const GtkGLRequirement *requirements, guint ver_major, guint ver_minor,
        GtkGLProfile profile);


/**
 * gtk_gl_canvas_destroy_context:
 * @canvas: The canvas
 *
 * Destroys an active context on a #GtkGLCanvas. If the canvas does not have
 * a context, this is a no-op.
 */
void gtk_gl_canvas_destroy_context(GtkGLCanvas *canvas);


/**
 * gtk_gl_canvas_has_context:
 * @canvas: The canvas
 *
 * Queries for an active OpenGL context.
 * Returns: Whether a #GtkGLCanvas has an active context
 */
gboolean gtk_gl_canvas_has_context(GtkGLCanvas *canvas);


/**
 * gtk_gl_canvas_make_current:
 * @canvas: The canvas
 *
 * Makes the OpenGL context of a #GtkGLCanvas current to the calling thread.
 * After this, any calls to gl* functions will refer to the context of this
 * canvas. If the canvas does not have a context, this is a no-op.
 */
void gtk_gl_canvas_make_current(GtkGLCanvas* canvas);


/**
 * gtk_gl_canvas_display_frame:
 * @canvas: The canvas
 *
 * Swaps the front and back buffer in a double-buffered GL context or flushes
 * all GL draw calls in a single-buffered context.
 *
 * Call this function after rendering a frame.
 */
void gtk_gl_canvas_display_frame(GtkGLCanvas* canvas);


G_END_DECLS
