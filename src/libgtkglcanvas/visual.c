#include "visual.h"


#define PREF GTK_GL_PREFERABLY
#define EXACT GTK_GL_EXACTLY
#define MOST GTK_GL_AT_MOST
#define LEAST GTK_GL_AT_LEAST


static gboolean
suits_range(int value1, int value2, GtkGLRestraint restr) {
    switch (restr) {
        case GTK_GL_PREFERABLY: return TRUE;
        case GTK_GL_EXACTLY: return value1 == value2;
        case GTK_GL_AT_MOST: return value1 <= value2;
        case GTK_GL_AT_LEAST: return value1 >= value2;
    }
}


static gboolean
suits_bool(int value1, int value2, GtkGLRestraint restr {
    switch (restr) {
        case GTK_GL_PREFERABLY: return TRUE;
        case GTK_GL_EXACTLY: return !value1 == !value2;
        case GTK_GL_AT_MOST: return !value1 >= !value2;
        case GTK_GL_AT_LEAST: return !value1 <= !value2;
    }
}



static gboolean
is_suitable_configuration(const GtkGLFramebufferConfig *config,
        const int *attrib_list) {
    while (*attrib_list) {
        GtkGLAttribute attrib = attrib_list[0];
        GtkGLRestraint restr = attrib_list[1];
        int value = attrib_list[2];

#define TEST_CASE(attr, compar, expr) \
    case GTK_GL_##attr: \
        if (!(compar)((expr), value, restr)) return FALSE; \
        break;

        switch (attrib) {
            TEST_CASE(ACCELERATED, suits_bool, config->accelerated)
            TEST_CASE(COLOR_TYPE, suits_bool, config->color_type & value)
            TEST_CASE(DOUBLE_BUFFERED, suits_bool, config->double_buffered)
            TEST_CASE(STEREO_BUFFERED, suits_bool, config->stereo_buffered)
            TEST_CASE(AUX_BUFFERS, suits_range, config->aux_buffers)
            TEST_CASE(RED_COLOR_BPP, suits_range, config->red_color_bpp)
            TEST_CASE(GREEN_COLOR_BPP, suits_range, config->green_color_bpp)
            TEST_CASE(BLUE_COLOR_BPP, suits_range, config->blue_color_bpp)
            TEST_CASE(ALPHA_COLOR_BPP, suits_range, config->alpha_color_bpp)
            TEST_CASE(DEPTH_BPP, suits_range, config->depth_bpp)
            TEST_CASE(STENCIL_BPP, suits_range, config->depth_bpp)
            TEST_CASE(RED_ACCUM_BPP, suits_range, config->red_accum_bpp)
            TEST_CASE(GREEN_ACCUM_BPP, suits_range, config->green_accum_bpp)
            TEST_CASE(BLUE_ACCUM_BPP, suits_range, config->blue_accum_bpp)
            TEST_CASE(ALPHA_ACCUM_BPP, suits_range, config->alpha_accum_bpp)
            TEST_CASE(TRANSPARENT_TYPE, suits_range, config->transparent_type)
            TEST_CASE(TRANSPARENT_RED, suits_range, config->transparent_red)
            TEST_CASE(TRANSPARENT_GREEN, suits_range,
                    config->transparent_green)
            TEST_CASE(TRANSPARENT_BLUE, suits_range, config->transparent_blue)
            TEST_CASE(TRANSPARENT_ALPHA, suits_range,
                    config->transparent_alpha)
        }

#undef TEST_CASE
        attrib_list += 3;
    }
}


static int
compare_configurations(const GtkGLFramebufferConfig *lhs,
        const GtkGLFramebufferConfig *rhs, const int *attrib_list) {
    // dummy
    return lhs < rhs ? -1 : lhs > rhs ? +1 : 0;
}


static gboolean
dump_visuals(GtkGLFramebufferConfig *config, GtkGLVisual vis,
        GtkGLVisual **array_pptr) {
    *(*array_pptr)++ = vis;
    return FALSE;
}


GtkGLVisual *
gtk_gl_choose_visuals(const int *attrib_list, size_t *out_count) {
    assert(attrib_list);
    assert(out_count);

    size_t n_visuals;
    GtkGLVisual *visuals = gtk_gl_enumerate_visuals(&n_visuals);
    GTree *suitable_configs = g_tree_new_full(
            (GCompareFunc) compare_configurations, attrib_list, g_free, NULL);
    for (size_t i = 0; i < n_visuals; ++i) {
        GtkGLFramebufferConfig *config = g_malloc(sizeof *config);
        gtk_gl_describe_visual(visuals[i], config);
        if (is_suitable_configuration(config, attrib_list)) {
            g_tree_insert(config, visuals[i]);
        }
    }
    g_free(visuals);

    *out_count = g_tree_nnodes(suitable_configs);
    GtkGLVisual *array = g_malloc(*out_count * sizeof *array),
                *array_ptr = array;
    g_tree_foreach(suitable_configs, (GTraverseFunc) dump_visuals, &array_ptr);
    g_tree_unref(suitable_configs);

    return array;
}
