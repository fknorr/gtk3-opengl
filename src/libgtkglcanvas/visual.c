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

#include <gtkgl/visual.h>
#include <assert.h>
#include <stdlib.h>


static gboolean
suits_range(gint value1, gint value2, GtkGLRequirementType restr) {
    switch (restr) {
        case GTK_GL_PREFERABLY: return TRUE;
        case GTK_GL_EXACTLY: return value1 == value2;
        case GTK_GL_AT_MOST: return value1 <= value2;
        case GTK_GL_AT_LEAST: return value1 >= value2;
    }
}


static gboolean
suits_bool(gint value1, gint value2, GtkGLRequirementType restr) {
    switch (restr) {
        case GTK_GL_PREFERABLY: return TRUE;
        case GTK_GL_EXACTLY: return !value1 == !value2;
        case GTK_GL_AT_MOST: return !value1 >= !value2;
        case GTK_GL_AT_LEAST: return !value1 <= !value2;
    }
}



static gboolean
is_suitable_configuration(const GtkGLFramebufferConfig *config,
        const GtkGLRequirement *r) {
    while (r->attr != GTK_GL_NONE) {

#define CASE_TEST(attr, compar, expr) \
    case GTK_GL_##attr: \
        if (!(compar)((expr), r->value, r->req)) return FALSE; \
        break;

        switch (r->attr) {
            CASE_TEST(ACCELERATED, suits_bool, config->accelerated)
            CASE_TEST(COLOR_TYPES, suits_bool, config->color_types & r->value)
            CASE_TEST(FB_LEVEL, suits_range, config->fb_level)
            CASE_TEST(COLOR_BPP, suits_range, config->color_bpp)
            CASE_TEST(DOUBLE_BUFFERED, suits_bool, config->double_buffered)
            CASE_TEST(STEREO_BUFFERED, suits_bool, config->stereo_buffered)
            CASE_TEST(AUX_BUFFERS, suits_range, config->aux_buffers)
            CASE_TEST(RED_COLOR_BPP, suits_range, config->red_color_bpp)
            CASE_TEST(GREEN_COLOR_BPP, suits_range, config->green_color_bpp)
            CASE_TEST(BLUE_COLOR_BPP, suits_range, config->blue_color_bpp)
            CASE_TEST(ALPHA_COLOR_BPP, suits_range, config->alpha_color_bpp)
            CASE_TEST(DEPTH_BPP, suits_range, config->depth_bpp)
            CASE_TEST(STENCIL_BPP, suits_range, config->depth_bpp)
            CASE_TEST(RED_ACCUM_BPP, suits_range, config->red_accum_bpp)
            CASE_TEST(GREEN_ACCUM_BPP, suits_range, config->green_accum_bpp)
            CASE_TEST(BLUE_ACCUM_BPP, suits_range, config->blue_accum_bpp)
            CASE_TEST(ALPHA_ACCUM_BPP, suits_range, config->alpha_accum_bpp)
            CASE_TEST(TRANSPARENT_TYPE, suits_range, config->transparent_type)
            CASE_TEST(TRANSPARENT_INDEX_VALUE, suits_range,
                    config->transparent_index)
            CASE_TEST(TRANSPARENT_RED, suits_range, config->transparent_red)
            CASE_TEST(TRANSPARENT_GREEN, suits_range,
                    config->transparent_green)
            CASE_TEST(TRANSPARENT_BLUE, suits_range, config->transparent_blue)
            CASE_TEST(TRANSPARENT_ALPHA, suits_range,
                    config->transparent_alpha)
            CASE_TEST(SAMPLE_BUFFERS, suits_range, config->sample_buffers)
            CASE_TEST(SAMPLES_PER_PIXEL, suits_range,
                    config->samples_per_pixel)
            CASE_TEST(CAVEAT, suits_range, config->caveat)
            default: break;
        }

#undef CASE_TEST

        ++r;
    }

    return TRUE;
}


static gint
compare_configurations(const GtkGLFramebufferConfig *lhs,
        const GtkGLFramebufferConfig *rhs,
        const GtkGLRequirement *r) {

#define CASE_ORDER_BY(attr, var) \
    case GTK_GL_##attr: \
        if (r->req == GTK_GL_PREFERABLY && (gint) lhs->var == r->value \
                && (gint) rhs->var != r->value) { \
            return -1; \
        } else if (r->req == GTK_GL_PREFERABLY && (gint) lhs->var != r->value \
                && (gint) rhs->var == r->value) { \
            return +1; \
        } else if (lhs->var < rhs->var) { \
            return r->req == GTK_GL_AT_MOST /* ascending */ ? -1 : +1; \
        } else if (lhs->var > rhs->var) { \
            return r->req == GTK_GL_AT_LEAST /* descending */ ? -1 : +1; \
        } \
        break;
#define PREFER_BOOL(attr, v) \
    if (!lhs->attr == !v && !rhs->attr != !v) return -1; \
    else if (!lhs->attr != !v && !rhs->attr == !v) return +1;
#define PREFER_ORDER(attr, order) \
    if (lhs->attr < rhs->attr) return order; \
    if (lhs->attr > rhs->attr) return -(order);
#define PREFER_LESS(attr) PREFER_ORDER(attr, -1)
#define PREFER_GREATER(attr) PREFER_ORDER(attr, +1)

    while (r->attr != GTK_GL_NONE) {
        if (r->req != GTK_GL_EXACTLY) {
            switch (r->attr) {
                CASE_ORDER_BY(ACCELERATED, accelerated)
                CASE_ORDER_BY(COLOR_TYPES, color_types)
                CASE_ORDER_BY(FB_LEVEL, fb_level)
                CASE_ORDER_BY(COLOR_BPP, color_bpp)
                CASE_ORDER_BY(DOUBLE_BUFFERED, double_buffered)
                CASE_ORDER_BY(STEREO_BUFFERED, stereo_buffered)
                CASE_ORDER_BY(AUX_BUFFERS, aux_buffers)
                CASE_ORDER_BY(RED_COLOR_BPP, red_color_bpp)
                CASE_ORDER_BY(GREEN_COLOR_BPP, green_color_bpp)
                CASE_ORDER_BY(BLUE_COLOR_BPP, blue_color_bpp)
                CASE_ORDER_BY(ALPHA_COLOR_BPP, alpha_color_bpp)
                CASE_ORDER_BY(DEPTH_BPP, depth_bpp)
                CASE_ORDER_BY(STENCIL_BPP, stencil_bpp)
                CASE_ORDER_BY(RED_ACCUM_BPP, red_accum_bpp)
                CASE_ORDER_BY(GREEN_ACCUM_BPP, green_accum_bpp)
                CASE_ORDER_BY(BLUE_ACCUM_BPP, blue_accum_bpp)
                CASE_ORDER_BY(ALPHA_ACCUM_BPP, alpha_accum_bpp)
                CASE_ORDER_BY(TRANSPARENT_TYPE, transparent_type)
                CASE_ORDER_BY(TRANSPARENT_INDEX_VALUE, transparent_index)
                CASE_ORDER_BY(TRANSPARENT_RED, transparent_red)
                CASE_ORDER_BY(TRANSPARENT_GREEN, transparent_green)
                CASE_ORDER_BY(TRANSPARENT_BLUE, transparent_blue)
                CASE_ORDER_BY(TRANSPARENT_ALPHA, transparent_alpha)
                CASE_ORDER_BY(SAMPLE_BUFFERS, sample_buffers)
                CASE_ORDER_BY(SAMPLES_PER_PIXEL, samples_per_pixel)
                CASE_ORDER_BY(CAVEAT, caveat)
                default: break;
            }

        }
        ++r;
    }

    PREFER_BOOL(caveat, FALSE)
    PREFER_BOOL(accelerated, TRUE)
    PREFER_LESS(aux_buffers)
    PREFER_LESS(red_accum_bpp)
    PREFER_LESS(green_accum_bpp)
    PREFER_LESS(blue_accum_bpp)
    PREFER_LESS(alpha_accum_bpp)
    PREFER_LESS(sample_buffers)
    PREFER_LESS(samples_per_pixel)
    PREFER_BOOL(stereo_buffered, FALSE)
    PREFER_BOOL(double_buffered, FALSE)
    PREFER_LESS(stencil_bpp)
    PREFER_LESS(depth_bpp)
    PREFER_LESS(color_bpp)

    return 0;

#undef CASE_TEST
#undef PREFER_BOOL
#undef PREFER_ORDER
#undef PREFER_GREATER
#undef PREFER_LESS
}


static gboolean
dump_visuals(GtkGLFramebufferConfig *config, GtkGLVisual *visual,
        GtkGLVisual ***array_pptr) {
    *(*array_pptr)++ = visual;
    return FALSE;
}


GtkGLVisualList *
gtk_gl_visual_list_new(gboolean is_owner, size_t count) {
    GtkGLVisualList *list = g_malloc(sizeof *list);
    list->is_owner = is_owner;
    list->count = count;
    list->entries = g_malloc0(sizeof(GtkGLVisual*) * count);
    return list;
}


GtkGLVisualList *
gtk_gl_choose_visuals(const GtkGLVisualList *pool,
        const GtkGLRequirement *requirements) {
    GTree *suitable_configs;
    GtkGLVisualList *list;
    GtkGLVisual **list_ptr;
    size_t i;

    assert(pool);
    assert(requirements);

    suitable_configs = g_tree_new_full(
            (GCompareDataFunc) compare_configurations, (void*) requirements,
            g_free, NULL);
    for (i = 0; i < pool->count; ++i) {
        GtkGLFramebufferConfig *config = g_malloc(sizeof *config);
        gtk_gl_describe_visual(pool->entries[i], config);
        if (is_suitable_configuration(config, requirements)) {
            g_tree_insert(suitable_configs, config, pool->entries[i]);
        }
    }

    list = gtk_gl_visual_list_new(FALSE, g_tree_nnodes(suitable_configs));
    list_ptr = list->entries;
    g_tree_foreach(suitable_configs, (GTraverseFunc) dump_visuals, &list_ptr);
    g_tree_unref(suitable_configs);

    return list;
}


void
gtk_gl_visual_list_free(GtkGLVisualList *list) {
    if (!list) return;

    if (list->is_owner) {
        size_t i;
        for (i = 0; i < list->count; ++i) {
            gtk_gl_visual_free(list->entries[i]);
        }
    }
    g_free(list->entries);
    g_free(list);
}
