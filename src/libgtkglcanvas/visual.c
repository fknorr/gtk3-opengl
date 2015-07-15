#include <gtkgl/visual.h>
#include <assert.h>


static gboolean
suits_range(int value1, int value2, GtkGLRequirementType restr) {
    switch (restr) {
        case GTK_GL_PREFERABLY: return TRUE;
        case GTK_GL_EXACTLY: return value1 == value2;
        case GTK_GL_AT_MOST: return value1 <= value2;
        case GTK_GL_AT_LEAST: return value1 >= value2;
    }
}


static gboolean
suits_bool(int value1, int value2, GtkGLRequirementType restr) {
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

#define TEST_CASE(attr, compar, expr) \
    case GTK_GL_##attr: \
        if (!(compar)((expr), r->value, r->req)) return FALSE; \
        break;

        switch (r->attr) {
            TEST_CASE(ACCELERATED, suits_bool, config->accelerated)
            TEST_CASE(COLOR_TYPES, suits_bool, config->color_types & r->value)
            TEST_CASE(FB_LEVEL, suits_range, config->fb_level)
            TEST_CASE(COLOR_BPP, suits_range, config->color_bpp)
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
            TEST_CASE(TRANSPARENT_INDEX_VALUE, suits_range,
                    config->transparent_index)
            TEST_CASE(TRANSPARENT_RED, suits_range, config->transparent_red)
            TEST_CASE(TRANSPARENT_GREEN, suits_range,
                    config->transparent_green)
            TEST_CASE(TRANSPARENT_BLUE, suits_range, config->transparent_blue)
            TEST_CASE(TRANSPARENT_ALPHA, suits_range,
                    config->transparent_alpha)
            TEST_CASE(SAMPLE_BUFFERS, suits_range, config->sample_buffers)
            TEST_CASE(SAMPLES_PER_PIXEL, suits_range,
                    config->samples_per_pixel)
            TEST_CASE(CAVEAT, suits_range, config->caveat)
            default: break;
        }

#undef TEST_CASE

        ++r;
    }

    return TRUE;
}


static int
compare_configurations(const GtkGLFramebufferConfig *lhs,
        const GtkGLFramebufferConfig *rhs,
        const GtkGLRequirement *requirements) {
    // dummy
    return lhs < rhs ? -1 : lhs > rhs ? +1 : 0;
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
