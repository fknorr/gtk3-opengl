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

#include <gtk/gtk.h>
#include <gtkgl/visual.h>
#include <gtkgl/canvas.h>
#include <GL/glew.h>
#include <math.h>


// Widgets as fetched from the GtkBuilder
static GtkWidget *window;
static GtkListStore *visual_list_store;
static GtkDialog *chooser;
GtkDialog *example_filter_dialog;
GtkGrid *example_filter_grid;
static GtkGLCanvas *canvas;
static GtkLabel *context_info_label, *mouse_info_label;
static GtkTreeSelection *visual_selection;
static GtkAdjustment *major_adjust, *minor_adjust;
static GtkComboBox *profile_combo;
static GtkButton *create_button, *destroy_button, *start_button, *stop_button;

GtkGLRequirement *example_requirements;
GtkGLVisualList *example_visuals, *example_choice;

// Whether the animation is running
static gboolean running = TRUE;
// The current object rotation
static float angle = 0.f;

// Drawing modes supported by the active context
// direct_mode: Drawing a triangle with glBegin() / glEnd()
// shaders: Drawing a rectangle with shaders and VBOs
// vaos: Drawing a hexagon with shaders, VBOs and Vertex Array Objects
static gboolean has_direct_mode, has_shaders, has_vaos;

// The shader program used by draw_with_shaders() and draw_with_vaos()
static GLuint program;
// Buffers for draw_with_shaders()
static GLuint rect_vertex_buffer, rect_index_buffer;
// Buffers for draw_with_vaos()
static GLuint hex_vertex_buffer, hex_index_buffer, hex_vao;
// Uniform / attribute locations in "program"
static GLuint projection_loc, modelview_loc, pos_loc, color_loc;


static void
message_box(int type, const char *msg) {
	GtkWidget *dlg = gtk_message_dialog_new(GTK_WINDOW(window),
            GTK_DIALOG_MODAL, type, GTK_BUTTONS_OK, "%s", msg);
	gtk_dialog_run(GTK_DIALOG(dlg));
	gtk_widget_destroy(dlg);
}


// Handler for the "start animation" button
gboolean
example_start_animation(void) {
	if (!gtk_gl_canvas_has_context(canvas)) {
		message_box(GTK_MESSAGE_ERROR, "No context present");
    } else {
		running = TRUE;
    }
	return TRUE;
}


// Handler for the "stop animation" button
gboolean
example_stop_animation(void) {
    running = FALSE;
	return TRUE;
}


// Animation function called by g_timeout_add_full()
static gboolean
example_animate(gpointer ud) {
    if (running) {
		angle += 2;
		gtk_widget_queue_draw(GTK_WIDGET(canvas));
	}
	return TRUE;
}


gboolean example_destroy_context(void);


// Loads a shader from file, compiles it, attaches it to a shader program
// and reports any errors via a message box.
static void
compile_attach_shader(GLuint program, GLenum type, const char *file) {
    GLuint shader = glCreateShader(type);
    GLint status;
	char *source;

	if (!g_file_get_contents(file, &source, NULL, NULL)) {
		message_box(GTK_MESSAGE_ERROR, "Unable to load shader source");
		example_destroy_context();
		return;
	}

    glShaderSource(shader, 1, (const char *const *) &source, NULL);
    glCompileShader(shader);

    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (!status) {
        GLsizei len;
        GLchar *log = g_malloc(10000);
        glGetShaderInfoLog(shader, 10000, &len, log);
		char *msg = g_strdup_printf("Error compiling shader:\n\n%s\n", log);
        message_box(GTK_MESSAGE_ERROR, msg);
		g_free(msg);
		example_destroy_context();
		return;
    }

    glAttachShader(program, shader);
    glDeleteShader(shader);
	g_free(source);
}


// Initializes a fresh OpenGL context, deciding available drawing modes and
// loading all required buffers
static void
init_context(void) {
	GLint mask;

	/* We're in a compatibility context if either
	 * 	 - Version < 3.1 or
	 *   - Version = 3.1 and GL_ARB_compatibility is supported or
	 *   - Version > 3.1 and GL_CONTEXT_COMPATIBILITY_PROFILE_BIT is set
	 */
	gboolean compatibility_context
	 		= !GLEW_VERSION_3_1 || GLEW_ARB_compatibility;
	if (!compatibility_context) {
		glGetIntegerv(GL_CONTEXT_PROFILE_MASK, &mask);
		compatibility_context = !!(mask &GL_CONTEXT_COMPATIBILITY_PROFILE_BIT);
	}

	glEnable(GL_MULTISAMPLE);

	has_direct_mode = compatibility_context;
	has_shaders = GLEW_VERSION_2_0 && compatibility_context;
	has_vaos = GLEW_VERSION_3_0;

	if (has_shaders || has_vaos) {
	    program = glCreateProgram();
	    compile_attach_shader(program, GL_VERTEX_SHADER,
				"res/example/vertex.glsl");
	    compile_attach_shader(program, GL_FRAGMENT_SHADER,
				"res/example/fragment.glsl");
	    glLinkProgram(program);
	    glValidateProgram(program);

		pos_loc = glGetAttribLocation(program, "pos");
		color_loc = glGetAttribLocation(program, "color");
		modelview_loc = glGetUniformLocation(program, "modelview");
		projection_loc = glGetUniformLocation(program, "projection");
	}

	if (has_shaders) {
		// Create VBOSs for rectangle
		static const GLfloat rect_verts[] = {
	 	//       x   y      R  G  B
				-1,  1,		1, 0, 0,
				-1, -1,     1, 1, 0,
				 1, -1,     0, 1, 0,
			 	 1,  1,     0, 0, 1
		};
		static const GLuint rect_inds[] = { 0, 1, 2, 2, 3, 0 };

		glGenBuffers(1, &rect_index_buffer);
		glBindBuffer(GL_ARRAY_BUFFER, rect_index_buffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof rect_inds, rect_inds,
				GL_STATIC_DRAW);

		glGenBuffers(1, &rect_vertex_buffer);
		glBindBuffer(GL_ARRAY_BUFFER, rect_vertex_buffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof rect_verts, rect_verts,
				GL_STATIC_DRAW);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}

	if (has_vaos) {
		// Create VBOs / VAO for hexagon
		static const GLfloat
#define SN 0.866025f
#define CS 0.5f
			hex_verts[] = {
			        0,    0,     1, 1, 1,
				    0,    1,     1, 1, 0,
				   SN,   CS,     0, 1, 0,
				   SN,  -CS,     0, 1, 1,
				    0,   -1,     0, 0, 1,
				  -SN,  -CS,     1, 0, 1,
				  -SN,   CS,     1, 0, 0
			};
		static const GLuint
			hex_inds[] = { 0, 1, 2, 3, 4, 5, 6, 1 };

		glGenVertexArrays(1, &hex_vao);
		glBindVertexArray(hex_vao);

		glGenBuffers(1, &hex_index_buffer);
		glBindBuffer(GL_ARRAY_BUFFER, hex_index_buffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof hex_inds, hex_inds,
				GL_STATIC_DRAW);

		glGenBuffers(1, &hex_vertex_buffer);
		glBindBuffer(GL_ARRAY_BUFFER, hex_vertex_buffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof hex_verts, hex_verts,
				GL_STATIC_DRAW);

		glEnableVertexAttribArray(pos_loc);
		glVertexAttribPointer(pos_loc, 2, GL_FLOAT, GL_FALSE, 20,
				(const void*) 0);
		glEnableVertexAttribArray(color_loc);
		glVertexAttribPointer(color_loc, 3, GL_FLOAT, GL_FALSE, 20,
				(const void*) 8);

		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}
}


// Frees any buffers and shaders allocated by init_context()
static void
cleanup_context(void) {
	if (has_vaos) {
		glBindVertexArray(0);
		glDeleteVertexArrays(1, &hex_vao);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		glDeleteBuffers(1, &hex_index_buffer);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glDeleteBuffers(1, &hex_vertex_buffer);
	}
	if (has_shaders) {
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		glDeleteBuffers(1, &rect_index_buffer);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glDeleteBuffers(1, &rect_vertex_buffer);
	}
	if (has_shaders || has_vaos) {
		glUseProgram(0);
		glDeleteProgram(program);
	}
}


// Draws a triangle with glBegin() / glEnd()
static void
draw_direct_mode(float aspect) {
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(-aspect, aspect, -1, 1, -1, 1);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glRotatef(angle, 0, 0, 1);
	glScalef(0.8, 0.8, 0.8);

	glBegin(GL_TRIANGLES);
		glColor3f(1, 0, 0);
		glVertex2f(0, 1);
		glColor3f(0, 1, 0);
		glVertex2f(-sinf(M_PI/3), -cosf(M_PI/3));
		glColor3f(0, 0, 1);
		glVertex2f(sinf(M_PI/3), -cosf(M_PI/3));
	glEnd();
}


// Loads the shader program and initializes uniforms
static void
begin_draw_with_shaders(float aspect, float scale) {
	float projection[16] = {
		1.f/aspect,   0,   0,   0,
		         0,   1,   0,   0,
				 0,   0,  -1,   0,
				-0,  -0,  -0,   1
	};
	float sin_angle = sin(angle*M_PI/180),
	      cos_angle = cos(angle*M_PI/180);
	float modelview[16] = {
		 cos_angle*scale, sin_angle*scale,     0,   0,
		-sin_angle*scale, cos_angle*scale,     0,   0,
		               0,               0, scale,   0,
		               0,               0,     0,   1
	};

	glUseProgram(program);

	glUniformMatrix4fv(modelview_loc, 1, GL_FALSE, modelview);
	glUniformMatrix4fv(projection_loc, 1, GL_FALSE, projection);
}


// Unloads the shader program
static void
end_draw_with_shaders(void) {
	glUseProgram(0);
}


// Drawas a rectangle with shaders VBOs
static void
draw_with_shaders(float aspect) {
	begin_draw_with_shaders(aspect, 0.6f);

	glBindBuffer(GL_ARRAY_BUFFER, rect_vertex_buffer);
	glEnableVertexAttribArray(pos_loc);
	glVertexAttribPointer(pos_loc, 2, GL_FLOAT, GL_FALSE, 20,
			(const void*) 0);
	glEnableVertexAttribArray(color_loc);
	glVertexAttribPointer(color_loc, 3, GL_FLOAT, GL_FALSE, 20,
			(const void*) 8);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, rect_index_buffer);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	end_draw_with_shaders();
}


// Draws a hexagon with VBOs and a vertex array object
static void
draw_with_vaos(float aspect) {
	begin_draw_with_shaders(aspect, 0.75f);

	glBindVertexArray(hex_vao);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, hex_index_buffer);
	glDrawElements(GL_TRIANGLE_FAN, 8, GL_UNSIGNED_INT, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	end_draw_with_shaders();
}


// Handler for the canvas' "draw" event
gboolean
example_draw(void) {
	GtkAllocation alloc;
	float aspect;

	if (!gtk_gl_canvas_has_context(canvas))
		return FALSE;

	// Bind the global OpenGL state to the canvas
	gtk_gl_canvas_make_current(canvas);

	// Set the viewport to the entire window (scaling)
	gtk_widget_get_allocation(GTK_WIDGET(canvas), &alloc);
	aspect = (float) alloc.width / alloc.height;

	glClearColor(0.1f, 0.1f, 0.1f, 1);
	glClear(GL_COLOR_BUFFER_BIT);

	if (has_direct_mode) {
		glViewport(0, alloc.height/2, alloc.width/2,
				alloc.height-alloc.height/2);
		draw_direct_mode(aspect);
	}

	if (has_shaders) {
		glViewport(alloc.width/2, alloc.height/2, alloc.width-alloc.width/2,
				alloc.height/2);
		draw_with_shaders(aspect);
	}

	if (has_vaos) {
		glViewport(0, 0, alloc.width/2, alloc.height/2);
		draw_with_vaos(aspect);
	}

	// Either swap buffers or flush, depending on the existance of a back buffer
	gtk_gl_canvas_display_frame(canvas);
	return TRUE;
}


// Updates the context-info-label on the bottom left of the window
static void
update_context_info(void) {
	gboolean present = gtk_gl_canvas_has_context(canvas);
	if (present) {
		char *text = g_strdup_printf("OpenGL Version %s",
				(const char*) glGetString(GL_VERSION));
		gtk_label_set_text(context_info_label, text);
		g_free(text);
	} else {
		gtk_label_set_text(context_info_label, "No context present");
	}

	gtk_widget_set_sensitive(GTK_WIDGET(create_button), !present);
	gtk_widget_set_sensitive(GTK_WIDGET(destroy_button), present);
	gtk_widget_set_sensitive(GTK_WIDGET(start_button), present);
	gtk_widget_set_sensitive(GTK_WIDGET(stop_button), present);
}


static void
update_visual_list(void) {
	// Initialize the context chooser dialog with existing visuals
	static const GtkGLRequirement no_requirements[] = { GTK_GL_LIST_END };
	size_t i;

	if (example_choice) gtk_gl_visual_list_free(example_choice);
	example_choice = gtk_gl_choose_visuals(example_visuals,
			example_requirements ? example_requirements : no_requirements);

	gtk_list_store_clear(visual_list_store);
	for (i = 0; i < example_choice->count; ++i) {
		GtkGLFramebufferConfig cfg;
		gtk_gl_describe_visual(example_choice->entries[i], &cfg);
		gtk_list_store_insert_with_values(visual_list_store, NULL, -1,
			0, cfg.accelerated,
			1, cfg.color_types & GTK_GL_COLOR_RGBA ? "RGBA" : "Indexed",
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
			18, cfg.sample_buffers,
			19, cfg.samples_per_pixel,
			20, cfg.caveat == GTK_GL_CAVEAT_SLOW ? "Slow"
				: cfg.caveat == GTK_GL_CAVEAT_NONCONFORMANT
					? "Non-conformant": "-",
			-1);
	}
}


// Handler for the "Create context..." button
gboolean
example_create_context(void) {
	if (gtk_gl_canvas_has_context(canvas)) {
		message_box(GTK_MESSAGE_ERROR, "Context exists already");
    } else {
		GtkTreeIter iter;
		unsigned ver_major, ver_minor;
		GtkGLProfile profile;
		size_t i;

		example_visuals = gtk_gl_canvas_enumerate_visuals(canvas);
		update_visual_list();

		// Show the chooser dialog
		gtk_dialog_run(chooser);
		gtk_widget_hide(GTK_WIDGET(chooser));

		// Get the entry of the selected index, or 0 if none was selected
		i = 0;
		if (gtk_tree_selection_get_selected(visual_selection, NULL, &iter)) {
			char *path = gtk_tree_model_get_string_from_iter(GTK_TREE_MODEL(
					visual_list_store), &iter);
			i = g_ascii_strtoll(path, NULL, 10);
			g_free(path);
		}

		// Get the version and profile values from the spin / combo boxes
		ver_major = (unsigned) gtk_adjustment_get_value(major_adjust);
		ver_minor = (unsigned) gtk_adjustment_get_value(minor_adjust);
		switch (gtk_combo_box_get_active(profile_combo)) {
			case 0: profile = GTK_GL_CORE_PROFILE; break;
			case 1: profile = GTK_GL_COMPATIBILITY_PROFILE; break;
			case 2: profile = GTK_GL_ES_PROFILE; break;
		}

		// Try creating and initializing a context
		if (gtk_gl_canvas_create_context_with_version(canvas,
				example_choice->entries[i], ver_major, ver_minor, profile)) {
			init_context();
			example_draw();
		} else {
			message_box(GTK_MESSAGE_ERROR, "Error creating context");
		}

		gtk_gl_visual_list_free(example_choice);
		example_choice = NULL;
		gtk_gl_visual_list_free(example_visuals);
		example_visuals = NULL;
    }

	update_context_info();

	return TRUE;
}


// Handler for the "Destroy context" button
gboolean
example_destroy_context(void) {
	if (!gtk_gl_canvas_has_context(canvas)) {
		message_box(GTK_MESSAGE_ERROR, "No context present");
    } else {
		running = FALSE;
		cleanup_context();
		gtk_gl_canvas_destroy_context(canvas);
	}

	update_context_info();

	return TRUE;
}



// Handler for the canvas' motion-notify event
gboolean
example_mouse_move(GtkWidget *widget, GdkEventMotion *ev) {
	// Update the bottom-right mouse-info-label
	char *text = g_strdup_printf("Mouse at (%d, %d)", (int) ev->x, (int) ev->y);
	gtk_label_set_text(mouse_info_label, text);
	g_free(text);
	return FALSE;
}


// Handler for the canvas' leave-notify event
gboolean
example_mouse_leave(GtkWidget *widget, GdkEventCrossing *ev) {
	// Clear the bottom-right mouse-info-label
	gtk_label_set_text(mouse_info_label, "");
	return FALSE;
}


void example_init_filter_dialog(void);
void example_filter_dialog_run(void);


gboolean
example_filter_visuals(GtkWidget *widget, GdkEvent *ev) {
	example_filter_dialog_run();
	update_visual_list();
	return FALSE;
}


int
main(int argc, char *argv[]) {
	GtkBuilder *builder;
	GError *error = NULL;

	gtk_init(&argc, &argv);

	builder = gtk_builder_new();
	gtk_builder_add_from_file(builder, "res/example/example.ui", &error);
	if (error) g_error("%s", error->message);

	gtk_builder_connect_signals(builder, NULL);

#define GET(name) gtk_builder_get_object(builder, name)

	window = GTK_WIDGET(GET("window"));
	context_info_label = GTK_LABEL(GET("context-info-label"));
	mouse_info_label = GTK_LABEL(GET("mouse-info-label"));
	chooser = GTK_DIALOG(GET("context-chooser"));
	example_filter_dialog = GTK_DIALOG(GET("filter-dialog"));
	example_filter_grid = GTK_GRID(GET("filter-grid"));
	visual_list_store = GTK_LIST_STORE(GET("visual-list-store"));
	canvas = GTK_GL_CANVAS(GET("canvas"));
	visual_selection = gtk_tree_view_get_selection(
			GTK_TREE_VIEW(GET("visual-list")));
	major_adjust = GTK_ADJUSTMENT(GET("ver-major"));
	minor_adjust = GTK_ADJUSTMENT(GET("ver-minor"));
	profile_combo = GTK_COMBO_BOX(GET("profile-combobox"));
	create_button = GTK_BUTTON(GET("create-button"));
	destroy_button = GTK_BUTTON(GET("destroy-button"));
	start_button = GTK_BUTTON(GET("start-anim-button"));
	stop_button = GTK_BUTTON(GET("stop-anim-button"));

#undef GET

	gtk_tree_selection_set_mode(visual_selection, GTK_SELECTION_SINGLE);
	update_context_info();

	example_init_filter_dialog();

	gtk_widget_show_all(window);
	g_object_unref(builder);

	// Start animation function, "running" determines if anything is actually
	// animated
	running = FALSE;
	g_timeout_add_full(1000, 10, example_animate, 0, 0);

	gtk_main();
	return 0;
}
