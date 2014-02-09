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
*
**/

#ifndef __GTK3_OPENGL_H__
#define __GTK3_OPENGL_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GTKGL_TYPE_CANVAS (gtkgl_canvas_get_type())
#define GTKGL_CANVAS(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), GTKGL_TYPE_CANVAS, GtkGLCanvas))
#define GTKGL_IS_CANVAS(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), GTKGL_TYPE_CANVAS))
#define GTKGL_CANVAS_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), GTKGL_TYPE_CANVAS, GtkGLCanvasClass)
#define GTKGL_IS_CANVAS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), GTKGL_TYPE_CANVAS_CLASS))
#define GTKGL_GET_CLASS(obj) (G_TYPE_INSTANCE((obj), GTKGL_TYPE_CANVAS, GtkGLCanvasClass))

typedef struct _GtkGLCanvas GtkGLCanvas;
typedef struct _GtkGLCanvasClass GtkGLCanvasClass;
typedef struct _GtkGLAttributes GtkGLAttributes;

struct _GtkGLCanvas {
    GtkWidget parent_instance;
};

struct _GtkGLCanvasClass {
    GtkWidgetClass parent_class;
};


enum
{
	GTKGL_DOUBLE_BUFFERED = 1,
	GTKGL_STEREO = 2,
	GTKGL_SAMPLE_BUFFERS = 4
};

struct _GtkGLAttributes {
	unsigned flags;
	unsigned num_samples;
	unsigned depth_buffer_bits;
};

GType gtkgl_canvas_get_type(void);
GtkWidget *gtkgl_canvas_new(const GtkGLAttributes *attrs);

void gtkgl_canvas_make_current(GtkGLCanvas*);
void gtkgl_canvas_swap_buffers(GtkGLCanvas*);

G_END_DECLS

#endif

