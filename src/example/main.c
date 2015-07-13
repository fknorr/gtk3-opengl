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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <gtkgl/visual.h>
#include <gtkgl/canvas.h>
#include <GL/gl.h>
#include <math.h>
#include <gio/gio.h>


static gboolean running = TRUE;
static GtkWidget *window;
static GtkListStore *visual_list_store;
static GtkDialog *chooser;
static GtkGLCanvas *canvas;
static GtkLabel *info_label;
static GtkTreeSelection *visual_selection;
static GtkAdjustment *major_adjust, *minor_adjust;
static GtkComboBox *profile_combo;


static void
message_box_response(GtkDialog *dialog) {
	gtk_widget_destroy(GTK_WIDGET(dialog));
}


static void
message_box(int type, const char *msg) {
	GtkWidget *dlg = gtk_message_dialog_new(GTK_WINDOW(window),
            GTK_DIALOG_MODAL, type, GTK_BUTTONS_OK, "%s", msg);
	g_signal_connect(G_OBJECT(dlg), "response",
            G_CALLBACK(message_box_response), NULL);
	gtk_dialog_run(GTK_DIALOG(dlg));
}


gboolean
example_start_animation(void) {
	if (!gtk_gl_canvas_has_context(canvas)) {
		message_box(GTK_MESSAGE_ERROR, "No context present");
    } else {
		running = TRUE;
    }
	return TRUE;
}


gboolean
example_stop_animation(void) {
    running = FALSE;
	return TRUE;
}


gboolean
example_click(GtkWidget *wid, GdkEvent *ev, gpointer user_data) {
    char *msg = g_strdup_printf("Clicked at %.3fx%.3f with button %d\n",
        ev->button.x, ev->button.y, ev->button.button);
	message_box(GTK_MESSAGE_INFO, msg);
	g_free(msg);
    return TRUE;
}

static float s = 0.f;

static gboolean
example_animate(gpointer ud) {
    if (running) {
		// some triangle rotation stuff
		s += 0.03f;

		gtk_widget_queue_draw(GTK_WIDGET(canvas));
	}
	return TRUE;
}


gboolean
example_draw_gl(void) {
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

    gtk_gl_canvas_display_frame(canvas);
    return TRUE;
}


static void
update_context_info(void) {
	if (gtk_gl_canvas_has_context(canvas)) {
		char *text = g_strdup_printf("OpenGL Version %s",
				(const char*) glGetString(GL_VERSION));
		gtk_label_set_text(info_label, text);
		g_free(text);
	} else {
		gtk_label_set_text(info_label, "No context present");
	}
}


gboolean
example_create_context(void) {
	if (gtk_gl_canvas_has_context(canvas)) {
		message_box(GTK_MESSAGE_ERROR, "Context exists already");
    } else {
		GtkGLVisualList *visuals = gtk_gl_canvas_enumerate_visuals(canvas);
		size_t i;
		GtkTreeIter iter;
		unsigned ver_major, ver_minor;
		GtkGLProfile profile;

		gtk_list_store_clear(visual_list_store);
		for (i = 0; i < visuals->count; ++i) {
			GtkGLFramebufferConfig cfg;
			gtk_gl_describe_visual(visuals->entries[i], &cfg);
			gtk_list_store_insert_with_values(visual_list_store, NULL, -1,
				0, cfg.accelerated,
				1, cfg.color_type & GTK_GL_COLOR_RGBA ? "RGBA" : "Indexed",
				2, cfg.color_bpp,
				3, cfg.fb_level,
				4, cfg.double_buffered,
				5, cfg.stereo_buffered,
				6, cfg.aux_buffers,
				7, cfg.red_color_bpp,
				8, cfg.green_color_bpp,
				9, cfg.blue_color_bpp,
				10, cfg.alpha_color_bpp,
				11, cfg.depth_bpp,
				12, cfg.stencil_bpp,
				13, cfg.red_accum_bpp,
				14, cfg.green_accum_bpp,
				15, cfg.blue_accum_bpp,
				16, cfg.alpha_accum_bpp,
				17, cfg.transparent_type == GTK_GL_TRANSPARENT_NONE ? "None"
					: cfg.transparent_type == GTK_GL_TRANSPARENT_RGB ? "RGB"
					: "Indexed",
				18, cfg.transparent_index,
				19, cfg.transparent_red,
				20, cfg.transparent_green,
				21, cfg.transparent_blue,
				22, cfg.transparent_alpha,
				23, cfg.sample_buffers,
				24, cfg.samples_per_pixel,
				-1);
		}

		gtk_dialog_run(chooser);
		gtk_widget_hide(GTK_WIDGET(chooser));

		i = 0;
		if (gtk_tree_selection_get_selected(visual_selection, NULL, &iter)) {
			char *path = gtk_tree_model_get_string_from_iter(GTK_TREE_MODEL(
					visual_list_store), &iter);
			i = g_ascii_strtoll(path, NULL, 10);
			g_free(path);
		}

		ver_major = (unsigned) gtk_adjustment_get_value(major_adjust);
		ver_minor = (unsigned) gtk_adjustment_get_value(minor_adjust);
		profile = gtk_combo_box_get_active(profile_combo) == 0
				? GTK_GL_CORE_PROFILE : GTK_GL_COMPATIBILITY_PROFILE;

		if (!gtk_gl_canvas_create_context_with_version(canvas,
				visuals->entries[i], ver_major, ver_minor, profile)) {
			message_box(GTK_MESSAGE_ERROR, "Error creating context");
		}
		gtk_gl_visual_list_free(visuals);
    }

	update_context_info();

	return TRUE;
}


gboolean
example_destroy_context(void) {
	if (!gtk_gl_canvas_has_context(canvas)) {
		message_box(GTK_MESSAGE_ERROR, "No context present");
    } else {
		running = FALSE;
		gtk_gl_canvas_destroy_context(canvas);
	}

	update_context_info();

	return TRUE;
}


gboolean
example_check_context(void) {
	const char *msg = gtk_gl_canvas_has_context(canvas)
		? "I have a context" : "I don't have a context";
	message_box(GTK_MESSAGE_INFO, msg);
	return FALSE;
}


int
main(int argc, char *argv[]) {
	GtkBuilder *builder;
	GError *error = NULL;

    gtk_init(&argc, &argv);

	builder = gtk_builder_new();
	gtk_builder_add_from_file(builder, "src/example/example.ui", &error);
	if (error) g_error("%s", error->message);

	gtk_builder_connect_signals(builder, NULL);

	window = GTK_WIDGET(gtk_builder_get_object(builder, "window"));
	info_label = GTK_LABEL(gtk_builder_get_object(builder, "info-label"));
	chooser = GTK_DIALOG(gtk_builder_get_object(builder, "visual-chooser"));
	visual_list_store = GTK_LIST_STORE(gtk_builder_get_object(builder,
			"visual-list-store"));
	canvas = GTK_GL_CANVAS(gtk_builder_get_object(builder, "canvas"));
	visual_selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(
			gtk_builder_get_object(builder, "visual-list")));
	major_adjust = GTK_ADJUSTMENT(gtk_builder_get_object(builder,
			"ver-major"));
	minor_adjust = GTK_ADJUSTMENT(gtk_builder_get_object(builder,
			"ver-minor"));
	profile_combo = GTK_COMBO_BOX(gtk_builder_get_object(builder,
			"profile-combobox"));

	gtk_tree_selection_set_mode(visual_selection, GTK_SELECTION_SINGLE);

	gtk_widget_show_all(window);
	g_object_unref(builder);

	running = FALSE;
    g_timeout_add_full(1000, 10, example_animate, 0, 0);

    gtk_main();
    return 0;
}
