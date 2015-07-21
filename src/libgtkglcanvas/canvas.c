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


#include <gtkgl/canvas.h>
#include "canvas_impl.h"
#include <GL/glew.h>


struct _GtkGLCanvas {
    GtkWidget parent_instance;
};

struct _GtkGLCanvasClass {
    GtkWidgetClass parent_class;
};


G_DEFINE_TYPE(GtkGLCanvas, gtk_gl_canvas, GTK_TYPE_WIDGET)

static void gtk_gl_canvas_realize (GtkWidget *wid);
static void gtk_gl_canvas_unrealize (GtkWidget *wid);
static void gtk_gl_canvas_send_configure(GtkWidget *wid);
static void gtk_gl_canvas_size_allocate(GtkWidget *wid,
        GtkAllocation *allocation);
static gboolean gtk_gl_canvas_draw(GtkWidget *wid, cairo_t *cr);


static gboolean
gtk_gl_canvas_draw(GtkWidget *wid, cairo_t *cr) {
	if (gtk_gl_canvas_has_context(GTK_GL_CANVAS(wid))) {
		return FALSE;
    }

    // Make a context-less canvas appear solid black
	GtkAllocation alloc;
	gtk_widget_get_allocation(wid, &alloc);
	cairo_set_source_rgb (cr, 0, 0, 0);
	cairo_rectangle(cr, 0, 0, alloc.width, alloc.height);
	cairo_fill(cr);
	return TRUE;
}


static void
gtk_gl_canvas_finalize(GObject *obj) {
	GtkGLCanvas *canvas = GTK_GL_CANVAS(obj);
	GtkGLCanvas_Priv *priv = GTK_GL_CANVAS_GET_PRIV(canvas);
	g_free(priv->native);

    G_OBJECT_CLASS(gtk_gl_canvas_parent_class)->finalize(obj);
}


static void
gtk_gl_canvas_class_init(GtkGLCanvasClass *klass) {
    GtkWidgetClass *wklass;
    GObjectClass *oklass;

    g_type_class_add_private(klass, sizeof(GtkGLCanvas_Priv));
    wklass = GTK_WIDGET_CLASS(klass);

    wklass->realize = gtk_gl_canvas_realize;
    wklass->unrealize = gtk_gl_canvas_unrealize;
    wklass->size_allocate = gtk_gl_canvas_size_allocate;
    wklass->draw = gtk_gl_canvas_draw;

    oklass = (GObjectClass*) klass;
    oklass->finalize = gtk_gl_canvas_finalize;
}


static void gtk_gl_canvas_create_window(GtkWidget *wid);

void
gtk_gl_canvas_realize(GtkWidget *wid) {
    GtkGLCanvas *canvas = GTK_GL_CANVAS(wid);
    GtkGLCanvas_Priv *priv = GTK_GL_CANVAS_GET_PRIV(canvas);
    GdkWindowAttr attributes;
    GtkAllocation allocation;
    gint attributes_mask;
	static GdkRGBA black = { 0, 0, 0, 1 };

    gtk_widget_set_realized(wid, TRUE);
    gtk_widget_get_allocation(wid, &allocation);

    // Create a GDK window to use as a parent for the actual context windows

    attributes.window_type = GDK_WINDOW_CHILD;
    attributes.x = allocation.x;
    attributes.y = allocation.y;
    attributes.width = allocation.width;
    attributes.height = allocation.height;
    attributes.wclass = GDK_INPUT_OUTPUT;

	attributes.event_mask = gtk_widget_get_events(wid)
            | GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK
            | GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK
            | GDK_POINTER_MOTION_HINT_MASK | GDK_SCROLL_MASK
            | GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK;
    attributes_mask = GDK_WA_X | GDK_WA_Y;
    priv->win = gdk_window_new(gtk_widget_get_parent_window(wid),
            &attributes, attributes_mask);
    gdk_window_set_user_data(priv->win, wid);
	gdk_window_set_background_rgba(priv->win, &black);
    gtk_widget_set_window(wid, priv->win);
    g_object_ref(wid);

    gtk_gl_canvas_send_configure(wid);

    gtk_gl_canvas_native_realize(canvas);
}


void
gtk_gl_canvas_unrealize(GtkWidget *wid) {
    GtkGLCanvas *canvas = GTK_GL_CANVAS(wid);
    GtkGLCanvas_Priv *priv = GTK_GL_CANVAS_GET_PRIV(canvas);

	if (!priv->is_dummy) 	{
		gtk_gl_canvas_native_destroy_context(canvas);
		priv->is_dummy = TRUE;
	}

    gtk_gl_canvas_native_unrealize(canvas);

    GTK_WIDGET_CLASS(gtk_gl_canvas_parent_class)->unrealize(wid);
}


static void
gtk_gl_canvas_init(GtkGLCanvas *self) {
    GtkWidget *wid = (GtkWidget*)self;
    GtkGLCanvas_Priv *priv = GTK_GL_CANVAS_GET_PRIV(wid);

	priv->native = gtk_gl_canvas_native_new();
	priv->is_dummy = TRUE;

    gtk_widget_set_can_focus(wid, TRUE);
    gtk_widget_set_receives_default(wid, TRUE);
    gtk_widget_set_has_window(wid, TRUE);
}


static void
gtk_gl_canvas_send_configure(GtkWidget *wid) {
    GtkAllocation allocation;
    GdkEvent *event = gdk_event_new(GDK_CONFIGURE);

    gtk_widget_get_allocation(wid, &allocation);

    event->configure.window = g_object_ref(gtk_widget_get_window(wid));
    event->configure.send_event = TRUE;
    event->configure.x = allocation.x;
    event->configure.y = allocation.y;
    event->configure.width = allocation.width;
    event->configure.height = allocation.height;

    gtk_widget_event(wid, event);
    gdk_event_free(event);
}


static void
gtk_gl_canvas_size_allocate(GtkWidget *wid, GtkAllocation *allocation) {
    g_return_if_fail(GTK_GL_IS_CANVAS(wid));
    g_return_if_fail(allocation != NULL);

    gtk_widget_set_allocation(wid, allocation);

    if (gtk_widget_get_realized(wid)) {
        if (gtk_widget_get_has_window(wid)) {

            gdk_window_move_resize(gtk_widget_get_window(wid),
                    allocation->x, allocation->y,
                    allocation->width, allocation->height);
        }

        gtk_gl_canvas_send_configure(wid);
    }
}

GtkWidget*
gtk_gl_canvas_new(void) {
    return GTK_WIDGET(g_object_new(GTK_GL_TYPE_CANVAS, NULL));
}


static void
gtk_gl_canvas_before_create_context(GtkGLCanvas *canvas) {
    GtkGLCanvas_Priv *priv = GTK_GL_CANVAS_GET_PRIV(canvas);
	if (!priv->is_dummy) {
		gtk_gl_canvas_native_destroy_context(canvas);
    }
}


static void
gtk_gl_canvas_after_create_context(GtkGLCanvas *canvas, gboolean success) {
    GtkGLCanvas_Priv *priv = GTK_GL_CANVAS_GET_PRIV(canvas);
    priv->is_dummy = !success;
	gtk_widget_queue_draw(GTK_WIDGET(canvas));
}


gboolean
gtk_gl_canvas_create_context(GtkGLCanvas *canvas, const GtkGLVisual *visual) {
    gboolean success;
    gtk_gl_canvas_before_create_context(canvas);
	success = gtk_gl_canvas_native_create_context(canvas, visual);
    gtk_gl_canvas_after_create_context(canvas, success);
	return success;
}


gboolean
gtk_gl_canvas_create_context_with_version(GtkGLCanvas *canvas,
       const GtkGLVisual *visual, guint ver_major, guint ver_minor,
       GtkGLProfile profile) {
    gboolean success;
    gtk_gl_canvas_before_create_context(canvas);
    success = gtk_gl_canvas_native_create_context_with_version(canvas, visual,
            ver_major, ver_minor, profile);
    gtk_gl_canvas_after_create_context(canvas, success);
    return success;
}


#define AUTO_CREATE_CONTEXT(create_expr) \
    GtkGLVisualList *visuals, *choice; \
    gboolean success; \
    visuals = gtk_gl_canvas_enumerate_visuals(canvas); \
    choice = gtk_gl_choose_visuals(visuals, requirements); \
    success = choice->count && (create_expr); \
    gtk_gl_visual_list_free(choice); \
    gtk_gl_visual_list_free(visuals); \
    return success;


gboolean
gtk_gl_canvas_auto_create_context(GtkGLCanvas *canvas,
        const GtkGLRequirement *requirements) {
    AUTO_CREATE_CONTEXT(gtk_gl_canvas_create_context(canvas,
            choice->entries[0]))
}


gboolean
gtk_gl_canvas_auto_create_context_with_version(GtkGLCanvas *canvas,
        const GtkGLRequirement *requirements, guint ver_major, guint ver_minor,
        GtkGLProfile profile) {
    AUTO_CREATE_CONTEXT(gtk_gl_canvas_create_context_with_version(canvas,
            choice->entries[0], ver_major, ver_minor, profile))
}


void
gtk_gl_canvas_destroy_context(GtkGLCanvas *canvas) {
    GtkGLCanvas_Priv *priv = GTK_GL_CANVAS_GET_PRIV(canvas);
	g_assert(!priv->is_dummy);
	gtk_gl_canvas_native_destroy_context(canvas);

	priv->is_dummy = TRUE;
	gtk_widget_queue_draw(GTK_WIDGET(canvas));
}


gboolean
gtk_gl_canvas_has_context(GtkGLCanvas *canvas) {
    GtkGLCanvas_Priv *priv = GTK_GL_CANVAS_GET_PRIV(canvas);
	return !priv->is_dummy;
}


void
gtk_gl_canvas_make_current(GtkGLCanvas *wid) {
    GtkGLCanvas_Priv *priv = GTK_GL_CANVAS_GET_PRIV(wid);
	g_assert(!priv->is_dummy);
	gtk_gl_canvas_native_make_current(wid);
}


void
gtk_gl_canvas_display_frame(GtkGLCanvas *wid) {
    GtkGLCanvas_Priv *priv = GTK_GL_CANVAS_GET_PRIV(wid);
	g_assert(!priv->is_dummy);

    // priv->double_buffered is set by the backend
    if (priv->double_buffered) {
        gtk_gl_canvas_native_swap_buffers(wid);
    } else {
        glFlush();
    }
}
