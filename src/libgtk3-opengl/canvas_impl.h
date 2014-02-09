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


#pragma once

#include "canvas.h"


typedef struct _GtkGLCanvas_Priv GtkGLCanvas_Priv;
typedef struct _GtkGLCanvas_NativePriv GtkGLCanvas_NativePriv;

struct _GtkGLCanvas_Priv {
    GdkDisplay *disp;
    GdkWindow *win;
	GtkGLAttributes attrs;
	GtkGLCanvas_NativePriv *native;
	gboolean is_dummy;
	unsigned effective_depth;
};


GtkGLCanvas_NativePriv *gtk_gl_canvas_native_new(void);
void gtk_gl_canvas_native_create_context(GtkGLCanvas_Priv *priv);
void gtk_gl_canvas_native_attach_context(GtkGLCanvas_Priv *priv);
void gtk_gl_canvas_native_destroy_context(GtkGLCanvas_Priv *priv);
void gtk_gl_canvas_native_swap_buffers(GtkGLCanvas_Priv *priv);
void gtk_gl_canvas_native_make_current(GtkGLCanvas_Priv *priv);


#define GTK_GL_CANVAS_GET_PRIV(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE((obj), GTK_GL_TYPE_CANVAS, GtkGLCanvas_Priv))

