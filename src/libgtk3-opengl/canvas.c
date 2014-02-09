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

/**
 * This file is based on the work of André Diego Piske, 
 * see <https://github.com/andrepiske/tegtkgl>. To retain André's licensing
 * conditions on the parts of the software authored by him, the following 
 * copyright notice shall be included in this and all derived files:
 */

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
 **/


#include "canvas.h"
#include "canvas_impl.h"

#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <glib-object.h>
#include <stdlib.h>


G_DEFINE_TYPE(GtkGLCanvas, gtkgl_canvas, GTK_TYPE_WIDGET)

static void gtkgl_canvas_realize (GtkWidget *wid);
static void gtkgl_canvas_unrealize (GtkWidget *wid);
static void gtkgl_canvas_send_configure(GtkWidget *wid);
static void gtkgl_canvas_size_allocate(GtkWidget *wid, GtkAllocation *allocation);
static gboolean gtkgl_canvas_draw(GtkWidget *wid, cairo_t *cr);

static gboolean
gtkgl_canvas_draw(GtkWidget *wid, cairo_t *cr)
{
    return FALSE;
}



static void
gtkgl_canvas_finalize(GObject *obj)
{
	GtkGLCanvas *canvas = GTKGL_CANVAS(obj);
	GtkGLCanvas_Priv *priv = GTKGL_CANVAS_GET_PRIV(canvas);
	free(priv->native);
	
    G_OBJECT_CLASS(gtkgl_canvas_parent_class)->finalize(obj);
}


static void
gtkgl_canvas_class_init(GtkGLCanvasClass *klass)
{
    GtkWidgetClass *wklass;
    GObjectClass *oklass;

    g_type_class_add_private(klass, sizeof(GtkGLCanvas_Priv));
    wklass = GTK_WIDGET_CLASS(klass);

    wklass->realize = gtkgl_canvas_realize;
    wklass->unrealize = gtkgl_canvas_unrealize;
    wklass->size_allocate = gtkgl_canvas_size_allocate;
    wklass->draw = gtkgl_canvas_draw;

    oklass = (GObjectClass*) klass;
    oklass->finalize = gtkgl_canvas_finalize;
}


void
gtkgl_canvas_realize(GtkWidget *wid)
{
    GtkGLCanvas *gtkgl = GTKGL_CANVAS(wid);
    GtkGLCanvas_Priv *priv = GTKGL_CANVAS_GET_PRIV(gtkgl);
	GtkGLCanvas_NativePriv *native = GTKGL_CANVAS_GET_PRIV(gtkgl)->native;
    GdkWindowAttr attributes;
    GtkAllocation allocation;
    gint attributes_mask;

    gtk_widget_set_realized(wid, TRUE);
    gtk_widget_get_allocation(wid, &allocation);

    priv->disp = gdk_display_get_default();
	
	gtkgl_canvas_native_create_context(priv);
	
    attributes.window_type = GDK_WINDOW_CHILD;
    attributes.x = allocation.x;
    attributes.y = allocation.y;
    attributes.width = allocation.width;
    attributes.height = allocation.height;
    attributes.wclass = GDK_INPUT_OUTPUT;
    attributes.visual = gdk_visual_get_best_with_both(priv->effective_depth,
        GDK_VISUAL_DIRECT_COLOR);
    attributes.event_mask = gtk_widget_get_events(wid);
    attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL;

    priv->win = gdk_window_new(gtk_widget_get_parent_window(wid),
            &attributes, attributes_mask);
    gdk_window_set_user_data(priv->win, wid);
    gtk_widget_set_window(wid, priv->win);
    g_object_ref(wid);

	gtkgl_canvas_native_attach_context(priv);

    gtkgl_canvas_send_configure(wid);
}


void
gtkgl_canvas_unrealize(GtkWidget *wid)
{
    GtkGLCanvas *gtkgl = GTKGL_CANVAS(wid);
    GtkGLCanvas_Priv *priv = GTKGL_CANVAS_GET_PRIV(gtkgl);

	gtkgl_canvas_native_destroy_context(priv);

    GTK_WIDGET_CLASS(gtkgl_canvas_parent_class)->unrealize(wid);
}


static void
gtkgl_canvas_init(GtkGLCanvas *self)
{
    GtkWidget *wid = (GtkWidget*)self;
    GtkGLCanvas_Priv *priv = GTKGL_CANVAS_GET_PRIV(wid);
	
	priv->native = gtkgl_canvas_native_new();

    gtk_widget_set_can_focus(wid, TRUE);
    gtk_widget_set_receives_default(wid, TRUE);
    gtk_widget_set_has_window(wid, TRUE);
    gtk_widget_set_double_buffered(wid, FALSE);
}


static void
gtkgl_canvas_send_configure(GtkWidget *wid)
{
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
gtkgl_canvas_size_allocate(GtkWidget *wid, GtkAllocation *allocation)
{
    g_return_if_fail(GTKGL_IS_CANVAS(wid));
    g_return_if_fail(allocation != NULL);

    gtk_widget_set_allocation(wid, allocation);

    if (gtk_widget_get_realized(wid)) {
        if (gtk_widget_get_has_window(wid)) {

            gdk_window_move_resize(gtk_widget_get_window(wid),
                    allocation->x, allocation->y,
                    allocation->width, allocation->height);
        }

        gtkgl_canvas_send_configure(wid);
    }
}

GtkWidget*
gtkgl_canvas_new(const GtkGLAttributes *attrs)
{
    GtkGLCanvas* canvas = g_object_new(GTKGL_TYPE_CANVAS, NULL);
    GtkGLCanvas_Priv *priv = GTKGL_CANVAS_GET_PRIV(canvas);
	priv->attrs = *attrs;
	return GTK_WIDGET(canvas);
}

