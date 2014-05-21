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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <gtkgl/canvas.h>
#include <GL/gl.h>
#include <math.h>
#include <gio/gio.h>


static gboolean running = TRUE;
static GtkWidget *window;
static GtkGLCanvas *canvas;


static void 
message_box_response(GtkDialog *dialog)
{
	gtk_widget_destroy(GTK_WIDGET(dialog));
}


static void 
message_box(int type, const char *msg)
{
	GtkWidget *dlg = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_MODAL, 
	                                        type, GTK_BUTTONS_OK, "%s", msg);
	g_signal_connect(G_OBJECT(dlg), "response", G_CALLBACK(message_box_response), NULL);
	gtk_dialog_run(GTK_DIALOG(dlg));
}


gboolean
example_start_animation(void)
{
	if (!gtk_gl_canvas_has_context(canvas))
		message_box(GTK_MESSAGE_ERROR, "No context present");
	else
		running = TRUE;
	return TRUE;
}


gboolean 
example_stop_animation(void) {
    running = FALSE;
	return TRUE;
}


gboolean 
example_click(GtkWidget *wid, GdkEvent *ev, gpointer user_data) {
    char *msg;
	asprintf(&msg, "Clicked at %.3fx%.3f with button %d\n",
        ev->button.x, ev->button.y, ev->button.button);
	message_box(GTK_MESSAGE_INFO, msg);
    return TRUE;
}

static float s = 0.f;

static gboolean 
example_animate(gpointer ud) 
{
    if (running)
	{
		// some triangle rotation stuff
		s += 0.03f;
        
		gtk_widget_queue_draw(GTK_WIDGET(canvas));
	}
	return TRUE;
}


gboolean
example_draw_gl(void)
{
	if (!gtk_gl_canvas_has_context(canvas))
		return FALSE;
	
	// this is very important, as OpenGL has a somewhat global state. 
	// this will set the OpenGL state to this very widget.
	gtk_gl_canvas_make_current(canvas);
	
    // more bureaucracy
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    glViewport(0, 0, 200, 200);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glClearColor(0, 0, 1.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);

    glRotatef(cos(s)*360.f-180.f, 0.f, 0.f, 1.0);
    glColor4ub(0xed, 0xb9, 0x1e, 0xff);
    glBegin(GL_TRIANGLES);
    glVertex2f(-0.7f, -0.5f);
    glVertex2f(0.4f, -0.5f);
    glVertex2f(0.f, 0.8f);
    glEnd();

    // this is also very important
    gtk_gl_canvas_swap_buffers(canvas);
    return TRUE;
}


gboolean
example_create_context(void)
{
	GtkGLAttributes attrs = {
        GTK_GL_DOUBLE_BUFFERED | GTK_GL_SAMPLE_BUFFERS, 4
		24, 24, 0
    };

	if (gtk_gl_canvas_has_context(canvas))
		message_box(GTK_MESSAGE_ERROR, "Context exists already");
	else if (!gtk_gl_canvas_create_context(canvas, &attrs))
		message_box(GTK_MESSAGE_ERROR, gtk_gl_canvas_get_error(canvas));
	return TRUE;
}


gboolean 
example_destroy_context(void)
{
	if (!gtk_gl_canvas_has_context(canvas))
		message_box(GTK_MESSAGE_ERROR, "No context present");
	else
	{
		running = FALSE;
		gtk_gl_canvas_destroy_context(canvas);
	}
	return TRUE;
}


gboolean
example_check_context(void)
{
	const char *msg = gtk_gl_canvas_has_context(canvas) 
		? "I have a context" : "I don't have a context";
	message_box(GTK_MESSAGE_INFO, msg);
}


int 
main(int argc, char *argv[]) 
{
	GtkBuilder *builder;
	GError *error = NULL;
	GFile *exe_file, *path_file;
	char *directory, *ui_path;

	exe_file = g_file_new_for_path(argv[0]);
	path_file = g_file_get_parent(exe_file);
	directory = g_file_get_path(path_file);
	ui_path = g_malloc(strlen(directory) + 12);
	strcpy(ui_path, directory);
	strcat(ui_path, "/example.ui");
	g_object_unref(exe_file);
	g_object_unref(path_file);	

	puts(ui_path);
	
    gtk_init(&argc, &argv);
	
	builder = gtk_builder_new();
	gtk_builder_add_from_file(builder, ui_path, &error);
	if (error) g_error(error->message);
    free(ui_path);
	
	gtk_builder_connect_signals(builder, NULL);
	
	window = GTK_WIDGET(gtk_builder_get_object(builder, "window"));
	canvas = GTK_GL_CANVAS(gtk_builder_get_object(builder, "canvas"));

	gtk_widget_show_all(window);
	g_object_unref(builder);

	running = FALSE;
    g_timeout_add_full(1000, 10, example_animate, 0, 0);

    gtk_main();
    return 0;
}

