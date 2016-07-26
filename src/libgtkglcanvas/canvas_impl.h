/**
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


#pragma once

#include <gtkgl/canvas.h>


typedef struct _GtkGLCanvas_Priv GtkGLCanvas_Priv;
typedef struct _GtkGLCanvas_NativePriv GtkGLCanvas_NativePriv;

struct _GtkGLCanvas_Priv {
    GdkWindow *win, *surface;
	GtkGLCanvas_NativePriv *native;
	gboolean is_dummy;
    gboolean double_buffered;
};


GtkGLCanvas_NativePriv *gtk_gl_canvas_native_new(void);

GdkWindow *gtk_gl_canvas_native_create_surface(GtkGLCanvas *canvas,
        const GtkGLVisual *visual);

void gtk_gl_canvas_native_destroy_surface(GtkGLCanvas *canvas);

gboolean gtk_gl_canvas_native_create_context(GtkGLCanvas *canvas,
        const GtkGLVisual *visual);

gboolean gtk_gl_canvas_native_create_context_with_version(GtkGLCanvas *canvas,
       const GtkGLVisual *visual, guint ver_major, guint ver_minor,
       GtkGLProfile profile);

void gtk_gl_canvas_native_destroy_context(GtkGLCanvas *canvas);

void gtk_gl_canvas_native_swap_buffers(GtkGLCanvas *canvas);

void gtk_gl_canvas_native_make_current(GtkGLCanvas *canvas);


#define GTK_GL_CANVAS_GET_PRIV(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE((obj), GTK_GL_TYPE_CANVAS, \
        GtkGLCanvas_Priv))

