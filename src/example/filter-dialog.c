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
#include <math.h>


extern GtkDialog *example_filter_dialog;
extern GtkGrid *example_filter_grid;
extern GtkGLRequirement *example_requirements;


typedef enum _AttrType {
	ATTR_BOOL,
	ATTR_INT,
	ATTR_UNSIGNED,
	ATTR_COLOR_TYPES,
	ATTR_TRANSP_TYPE,
	ATTR_CAVEAT
} AttrType;


typedef struct AttrDescriptor {
	GtkGLAttribute attr;
	const char *name;
 	AttrType type;
} AttrDescriptor;


static const AttrDescriptor attrs[] = {
	{ GTK_GL_ACCELERATED, "Accelerated", ATTR_BOOL },
	{ GTK_GL_COLOR_TYPES, "Color types", ATTR_COLOR_TYPES },
	{ GTK_GL_COLOR_BPP, "Color BPP", ATTR_UNSIGNED },
	{ GTK_GL_FB_LEVEL, "Framebuffer Level", ATTR_INT },
	{ GTK_GL_DOUBLE_BUFFERED, "Double Buffered", ATTR_BOOL },
	{ GTK_GL_STEREO_BUFFERED, "Stereo Buffered", ATTR_BOOL },
	{ GTK_GL_AUX_BUFFERS, "Aux Buffers", ATTR_UNSIGNED },
	{ GTK_GL_RED_COLOR_BPP, "Red Color BPP", ATTR_UNSIGNED },
	{ GTK_GL_GREEN_COLOR_BPP, "Green Color BPP", ATTR_UNSIGNED },
	{ GTK_GL_BLUE_COLOR_BPP, "Blue Color BPP", ATTR_UNSIGNED },
	{ GTK_GL_ALPHA_COLOR_BPP, "Alpha Color BPP", ATTR_UNSIGNED },
	{ GTK_GL_DEPTH_BPP, "Depth BPP", ATTR_UNSIGNED },
	{ GTK_GL_STENCIL_BPP, "Stencil", ATTR_UNSIGNED },
	{ GTK_GL_RED_ACCUM_BPP, "Red Accum. BPP", ATTR_UNSIGNED },
	{ GTK_GL_GREEN_ACCUM_BPP, "Green Accum. BPP", ATTR_UNSIGNED },
	{ GTK_GL_BLUE_ACCUM_BPP, "Blue Accum. BPP", ATTR_UNSIGNED },
	{ GTK_GL_ALPHA_ACCUM_BPP, "Alpha Accum. BPP", ATTR_UNSIGNED },
	{ GTK_GL_TRANSPARENT_TYPE, "Transparency Type", ATTR_TRANSP_TYPE },
	{ GTK_GL_TRANSPARENT_INDEX_VALUE, "Transparent Index", ATTR_UNSIGNED },
	{ GTK_GL_TRANSPARENT_RED, "Transparent Red", ATTR_UNSIGNED },
	{ GTK_GL_TRANSPARENT_GREEN, "Transparent Green", ATTR_UNSIGNED },
	{ GTK_GL_TRANSPARENT_BLUE, "Transparent Blue", ATTR_UNSIGNED },
	{ GTK_GL_TRANSPARENT_ALPHA, "Transparent Alpha", ATTR_UNSIGNED },
	{ GTK_GL_SAMPLE_BUFFERS, "Sample Buffers", ATTR_UNSIGNED },
	{ GTK_GL_SAMPLES_PER_PIXEL, "Samples per Pixel", ATTR_UNSIGNED },
	{ GTK_GL_CAVEAT, "Caveat", ATTR_CAVEAT }
};


enum {
	ATTR_COUNT = sizeof(attrs)/sizeof(attrs[0])
};


void
example_init_filter_dialog(void) {
	size_t i;

	for (i = 0; i < ATTR_COUNT; ++i) {
		GtkLabel *name_label = GTK_LABEL(gtk_label_new(attrs[i].name));

		GtkComboBoxText *req_box = GTK_COMBO_BOX_TEXT(gtk_combo_box_text_new());
		gtk_combo_box_text_append_text(req_box, "Don't care");
		gtk_combo_box_text_append_text(req_box, "Preferably equals");
		gtk_combo_box_text_append_text(req_box, "Exactly equals");
		gtk_combo_box_text_append_text(req_box, "At most");
		gtk_combo_box_text_append_text(req_box, "At least");
		gtk_combo_box_set_active(GTK_COMBO_BOX(req_box), 0);

		GtkWidget *selector;
		switch (attrs[i].type) {
			case ATTR_BOOL: {
				GtkComboBoxText *bool_box = GTK_COMBO_BOX_TEXT(
						gtk_combo_box_text_new());
				gtk_combo_box_text_append_text(bool_box, "False");
				gtk_combo_box_text_append_text(bool_box, "True");
				gtk_combo_box_set_active(GTK_COMBO_BOX(bool_box), 0);
				selector = GTK_WIDGET(bool_box);
				break;
			}

			case ATTR_INT: {
				GtkAdjustment *adj = gtk_adjustment_new(0, -1000, 1000,
						1, 1, 1);
				GtkSpinButton *spin = GTK_SPIN_BUTTON(gtk_spin_button_new(
						adj, 1, 0));
				selector = GTK_WIDGET(spin);
				break;
			}

			case ATTR_UNSIGNED: {
				GtkAdjustment *adj = gtk_adjustment_new(0, 0, 1000,
						1, 1, 1);
				GtkSpinButton *spin = GTK_SPIN_BUTTON(gtk_spin_button_new(
						adj, 1, 0));
				selector = GTK_WIDGET(spin);
				break;
			}

			case ATTR_COLOR_TYPES: {
				GtkComboBoxText *color_box = GTK_COMBO_BOX_TEXT(
						gtk_combo_box_text_new());
				gtk_combo_box_text_append_text(color_box, "RGBA");
				gtk_combo_box_text_append_text(color_box, "Indexed");
				gtk_combo_box_text_append_text(color_box, "RGBA+Indexed");
				gtk_combo_box_set_active(GTK_COMBO_BOX(color_box), 0);
				selector = GTK_WIDGET(color_box);
				break;
			}

			case ATTR_TRANSP_TYPE: {
				GtkComboBoxText *transp_box = GTK_COMBO_BOX_TEXT(
						gtk_combo_box_text_new());
				gtk_combo_box_text_append_text(transp_box, "None");
				gtk_combo_box_text_append_text(transp_box, "RGB");
				gtk_combo_box_text_append_text(transp_box, "Index");
				gtk_combo_box_set_active(GTK_COMBO_BOX(transp_box), 0);
				selector = GTK_WIDGET(transp_box);
				break;
			}

			case ATTR_CAVEAT: {
				GtkComboBoxText *cave_box = GTK_COMBO_BOX_TEXT(
						gtk_combo_box_text_new());
				gtk_combo_box_text_append_text(cave_box, "None");
				gtk_combo_box_text_append_text(cave_box, "Slow");
				gtk_combo_box_text_append_text(cave_box, "Non-conformant");
				gtk_combo_box_set_active(GTK_COMBO_BOX(cave_box), 0);
				selector = GTK_WIDGET(cave_box);
				break;
			}
		}

		gtk_grid_attach(example_filter_grid, GTK_WIDGET(name_label), 0, i,
				1, 1);
		gtk_grid_attach(example_filter_grid, GTK_WIDGET(req_box), 1, i, 1, 1);
		gtk_grid_attach(example_filter_grid, selector, 2, i, 1, 1);
	}
	gtk_widget_show_all(GTK_WIDGET(example_filter_grid));
}


void
example_filter_dialog_run(void) {
	size_t i, j=0;

	gtk_dialog_run(example_filter_dialog);
	gtk_widget_hide(GTK_WIDGET(example_filter_dialog));

	g_free(example_requirements);
	example_requirements = g_malloc0(ATTR_COUNT * sizeof(GtkGLRequirement));

	for (i = 0; i < ATTR_COUNT; ++i) {
		int combo_req = gtk_combo_box_get_active(GTK_COMBO_BOX(
				gtk_grid_get_child_at(example_filter_grid, 1, i)));
		if (combo_req) {
			GtkWidget *selector = gtk_grid_get_child_at(example_filter_grid,
					2, i);
			example_requirements[j].attr = attrs[i].attr;
			example_requirements[j].req = combo_req - 1;
			if (attrs[i].type == ATTR_INT || attrs[i].type == ATTR_UNSIGNED) {
				example_requirements[j].value
						= gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(
							selector));
			} else {
				example_requirements[j].value = gtk_combo_box_get_active(
						GTK_COMBO_BOX(selector));
			}
			++j;
		}
	}
}
