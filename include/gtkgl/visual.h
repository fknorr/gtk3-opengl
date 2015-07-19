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


#pragma once

#include <glib.h>


G_BEGIN_DECLS


/// A binary combination of flags defined in an enum
typedef unsigned GtkGLBitSet;


/// Color types as supported by a framebuffer configuration.
typedef enum _GtkGLColorType {
    /// Each color is described by a RGBA triple (True Color and Direct Color)
    GTK_GL_COLOR_RGBA = 1,
    /// Colors are described by an index into a color table
    GTK_GL_COLOR_INDEXED = 2
} GtkGLColorType;


/// The type of transparency color supported.
typedef enum _GtkGLTransparentType {
    /// No transparent color is defined
    GTK_GL_TRANSPARENT_NONE,
    /// The transparent color is defined via a RGBA triple
    GTK_GL_TRANSPARENT_RGB,
    /// The transparent color is defined via a color index
    GTK_GL_TRANSPARENT_INDEX
} GtkGLTransparentType;


/// Caveats of framebuffer configurations.
typedef enum _GtkGLCaveat {
    /// The configuration has no caveat
    GTK_GL_CAVEAT_NONE,
    /// The configuration is expected to be slower than others
    GTK_GL_CAVEAT_SLOW,
    /// The configuration is not standard-conformant
    GTK_GL_CAVEAT_NONCONFORMANT
} GtkGLCaveat;


/// The backend-dependent handle of a framebuffer configurations.
typedef struct _GtkGLVisual GtkGLVisual;


/// A list of backend handles to framebuffer configurations.
typedef struct _GtkGLVisualList {
    /// Whether the list owns the entries pointer (used internally)
    gboolean is_owner;

    /// The number of list entries
    size_t count;

    /// The list entries
    GtkGLVisual **entries;
} GtkGLVisualList;


/// A structure describing framebuffer configurations platform-independently
typedef struct _GtkGLFramebufferConfig {
    /// Whether the framebuffer supports hardware-acceleration
    gboolean accelerated;

    /// The color types supported (combination of values in GtkGLColorType)
    GtkGLBitSet color_types;

    /// The number of bits per pixel in the color buffer (for RGBA framebuffers,
    /// this is the sum of red/green/blue/alpha_color_bpp
    unsigned color_bpp;

    /* The frame buffer level. Level zero is the default frame buffer. Positive
     * levels correspond to frame buffers that overlay the default buffer,
     * and negative levels correspond to frame buffers that underlie the
     * default buffer
     */
    int fb_level;

    /// Whether the framebuffer has separate front and back buffers
    gboolean double_buffered;

    /// Whether the framebuffer has separate left and right buffers
    gboolean stereo_buffered;

    /// The number of auxiliary color buffers available
    unsigned aux_buffers;

    /// The number of bits per pixel in the color buffer's red channel
    /// (only for RGBA contexts)
    unsigned red_color_bpp;

    /// The number of bits per pixel in the color buffer's green channel
    /// (only for RGBA contexts)
    unsigned green_color_bpp;

    /// The number of bits per pixel in the color buffer's blue channel
    /// (only for RGBA contexts)
    unsigned blue_color_bpp;

    /// The number of bits per pixel in the color buffer's alpha channel
    /// (only for RGBA contexts)
    unsigned alpha_color_bpp;

    /// The number of bits per pixel in the depth buffer, or zero if the
    /// the framebuffer does not have a depth buffer
    unsigned depth_bpp;

    /// The number of bits per pixel in the stencil buffer, or zero if the
    /// the framebuffer does not have a stencil buffer
    unsigned stencil_bpp;

    /// The number of bits per pixel in the accumulation buffer's red channel,
    /// or zero if there is no accumulation buffer
    unsigned red_accum_bpp;

    /// The number of bits per pixel in the accumulation buffer's green channel,
    /// or zero if there is no accumulation buffer
    unsigned green_accum_bpp;

    /// The number of bits per pixel in the accumulation buffer's blue channel,
    /// or zero if there is no accumulation buffer
    unsigned blue_accum_bpp;

    /// The number of bits per pixel in the accumulation buffer's alpha channel,
    /// or zero if there is no accumulation buffer
    unsigned alpha_accum_bpp;

    /// The type of transparent color used by the framebuffer
    GtkGLTransparentType transparent_type;

    /// The index of the transparent color (only in color index framebuffers)
    unsigned transparent_index;

    /// The red value of the transparent color (only in RGBA framebuffers)
    unsigned transparent_red;

    /// The green value of the transparent color (only in RGBA framebuffers)
    unsigned transparent_green;

    /// The blue value of the transparent color (only in RGBA framebuffers)
    unsigned transparent_blue;

    /// The alpha value of the transparent color (only in RGBA framebuffers)
    unsigned transparent_alpha;

    /// The number of sample buffers available
    unsigned sample_buffers;

    /// The number of samples per pixel possible
    unsigned samples_per_pixel;

    /// The framebuffer configuration's caveat, if any
    GtkGLCaveat caveat;
} GtkGLFramebufferConfig;


/// Tokens to mark framebuffer attributes in gtk_gl_choose_visuals()
typedef enum _GtkGLAttribute {
    /// Marks the end of the attribute list
    GTK_GL_NONE,

    /// Marks GtkGLFramebufferConfig.accelerated
    GTK_GL_ACCELERATED,

    /// Marks GtkGLFramebufferConfig.color_types
    GTK_GL_COLOR_TYPES,

    /// Marks GtkGLFramebufferConfig.color_bpp
    GTK_GL_COLOR_BPP,

    /// Marks GtkGLFramebufferConfig.fb_level
    GTK_GL_FB_LEVEL,

    /// Marks GtkGLFramebufferConfig.double_buffered
    GTK_GL_DOUBLE_BUFFERED,

    /// Marks GtkGLFramebufferConfig.stereo_buffered
    GTK_GL_STEREO_BUFFERED,

    /// Marks GtkGLFramebufferConfig.aux_buffers
    GTK_GL_AUX_BUFFERS,

    /// Marks GtkGLFramebufferConfig.red_color_bpp
    GTK_GL_RED_COLOR_BPP,

    /// Marks GtkGLFramebufferConfig.green_color_bpp
    GTK_GL_GREEN_COLOR_BPP,

    /// Marks GtkGLFramebufferConfig.blue_color_bpp
    GTK_GL_BLUE_COLOR_BPP,

    /// Marks GtkGLFramebufferConfig.alpha_color_bpp
    GTK_GL_ALPHA_COLOR_BPP,

    /// Marks GtkGLFramebufferConfig.depth_bpp
    GTK_GL_DEPTH_BPP,

    /// Marks GtkGLFramebufferConfig.stencil_bpp
    GTK_GL_STENCIL_BPP,

    /// Marks GtkGLFramebufferConfig.red_accum_bpp
    GTK_GL_RED_ACCUM_BPP,

    /// Marks GtkGLFramebufferConfig.green_accum_bpp
    GTK_GL_GREEN_ACCUM_BPP,

    /// Marks GtkGLFramebufferConfig.blue_accum_bpp
    GTK_GL_BLUE_ACCUM_BPP,

    /// Marks GtkGLFramebufferConfig.alpha_accum_bpp
    GTK_GL_ALPHA_ACCUM_BPP,

    /// Marks GtkGLFramebufferConfig.transparent_type
    GTK_GL_TRANSPARENT_TYPE,

    /// Marks GtkGLFramebufferConfig.transparent_index
    GTK_GL_TRANSPARENT_INDEX_VALUE,

    /// Marks GtkGLFramebufferConfig.transparent_red
    GTK_GL_TRANSPARENT_RED,

    /// Marks GtkGLFramebufferConfig.transparent_green
    GTK_GL_TRANSPARENT_GREEN,

    /// Marks GtkGLFramebufferConfig.transparent_blue
    GTK_GL_TRANSPARENT_BLUE,

    /// Marks GtkGLFramebufferConfig.transparent_alpha
    GTK_GL_TRANSPARENT_ALPHA,

    /// Marks GtkGLFramebufferConfig.sample_buffers
    GTK_GL_SAMPLE_BUFFERS,

    /// Marks GtkGLFramebufferConfig.samples_per_pixel
    GTK_GL_SAMPLES_PER_PIXEL,

    /// Marks GtkGLFramebufferConfig.caveat
    GTK_GL_CAVEAT
} GtkGLAttribute;


/// Defines the type of requirement imposed on a certain framebuffer attribute
/// in gtk_gl_choose_visuals
typedef enum _GtkGLRequirementType {
    /// Configurations in which the attribute matches the reference value are
    /// preferred over other configurations
    GTK_GL_PREFERABLY,

    /// The attribute value must match the reference value exactly
    GTK_GL_EXACTLY,

    /// The attribute value must not be greater than the reference value.
    /// Smaller values will be preferred
    GTK_GL_AT_MOST,

    /// The attribute value must not be less than the reference value.
    /// Greater values will be preferred
    GTK_GL_AT_LEAST
} GtkGLRequirementType;


/// Defines a requirement for filtering and sorting framebuffer configurations
/// in gtk_gl_choose_visuals
typedef struct _GtkGLRequirement {
    GtkGLAttribute attr;
    GtkGLRequirementType req;
    int value;
} GtkGLRequirement;


/// Terminator token for the requirement list in gtk_gl_choose_visuals
#define GTK_GL_LIST_END { GTK_GL_NONE, GTK_GL_EXACTLY, 0 }


/**
 * Filters and sorts a GtkGLVisualList by the criteria provided in
 * the requirements parameter.
 * @param pool The list of visuals to choose from
 * @param requirements An array of GtkGLRequirements, terminated by
 * GTK_GL_LIST_END
 * @return A filtered GtkGLVisualList, owned by the caller
 */
GtkGLVisualList *gtk_gl_choose_visuals(const GtkGLVisualList *pool,
        const GtkGLRequirement *requirements);


/**
 * Frees a visual that was returned by gtk_gl_canvas_enumerate_visuals before.
 *
 * You should consider using gtk_gl_visual_list_free instead.
 * @param vis The visual
 */
void gtk_gl_visual_free(GtkGLVisual *vis);


/**
 * Creates a new, empty visual list of the desired size.
 * @param is_owner Whether a subsequent call to gtk_gl_visual_list_free should
 *                 attempt to free the list entries
 * @param count The number of entries
 * @return The new list
 */
GtkGLVisualList *gtk_gl_visual_list_new(gboolean is_owner, size_t count);


/**
 * Sorts a visual list ascending by relevance and estimated resource
 * requirement.
 *
 * Visuals without caveats are preferred over visuals with caveats, accelerated
 * visuals are preferred over unaccelerated ones, and visuals with less
 * features, buffers and buffer depth are preferred over visuals with more
 * resource requirements.
 * @param list The visual list
 */
void gtk_gl_visual_list_sort(GtkGLVisualList *list);


/**
 * Frees a GtkGLVisualList.
 *
 * Optionally frees the entries if the list structure owns them.
 * @param vis The list
 */
void gtk_gl_visual_list_free(GtkGLVisualList *visuals);


/**
 * Fills a GtkGLFramebufferConfig structure from a backend-dependent visual.
 * @param visual The visual
 * @param out The framebuffer configuration to write to
 */
void gtk_gl_describe_visual(const GtkGLVisual *visual,
        GtkGLFramebufferConfig *out);


G_END_DECLS
