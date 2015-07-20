/*
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


#pragma once

#include <glib.h>


/**
 * SECTION:visual
 * @Title: Visuals
 * @Short_Description: Platform independent framebuffer configuration
 *
 * Foo bar
 */


G_BEGIN_DECLS


/**
 * GtkGLBitSet:
 * A binary combination of flags defined in an enum
 */
typedef unsigned GtkGLBitSet;


/**
 * GtkGLColorType:
 * @GTK_GL_COLOR_RGBA: Each color is described by a RGBA triple (True Color
 *      and Direct Color)
 * @GTK_GL_COLOR_INDEXED: Colors are described by an index into a color table
 *
 * Color types as supported by a framebuffer configuration.
 */
typedef enum _GtkGLColorType {
    GTK_GL_COLOR_RGBA = 1,
    GTK_GL_COLOR_INDEXED = 2
} GtkGLColorType;


/**
 * GtkGLTransparentType:
 * @GTK_GL_TRANSPARENT_NONE: No transparent color is defined
 * @GTK_GL_TRANSPARENT_RGB: The transparent color is defined via a RGBA triple
 * @GTK_GL_TRANSPARENT_INDEX: The transparent color is defined via a color index
 *
 * The type of transparency color supported.
 */
typedef enum _GtkGLTransparentType {
    GTK_GL_TRANSPARENT_NONE,
    GTK_GL_TRANSPARENT_RGB,
    GTK_GL_TRANSPARENT_INDEX
} GtkGLTransparentType;


/**
 * GtkGLCaveat
 * @GTK_GL_CAVEAT_NONE: The configuration has no caveat
 * @GTK_GL_CAVEAT_SLOW: The configuration is expected to be slower than others
 * @GTK_GL_CAVEAT_NONCONFORMANT: The configuration is not standard-conformant
 *
 * Caveats of framebuffer configurations.
 */
typedef enum _GtkGLCaveat {
    GTK_GL_CAVEAT_NONE,
    GTK_GL_CAVEAT_SLOW,
    GTK_GL_CAVEAT_NONCONFORMANT
} GtkGLCaveat;


/**
 * GtkGLVisual:
 * The backend-dependent handle of a framebuffer configurations.
 */
typedef struct _GtkGLVisual GtkGLVisual;


/**
 * GtkGLVisualList:
 * @is_owner: Whether the list owns the entries pointer (used internally)
 * @count: The number of list entries
 * @entries: The array of list entries
 *
 * A list of backend handles to framebuffer configurations.
 */
typedef struct _GtkGLVisualList {
    gboolean is_owner;
    gsize count;
    GtkGLVisual **entries;
} GtkGLVisualList;


/**
 * GtkGLFramebufferConfig:
 * @accelerated: Whether the framebuffer supports hardware-acceleration
 * @color_types: The color types supported (combination of values in
 *         GtkGLColorType)
 * @color_bpp: The number of bits per pixel in the color buffer (for RGBA
 *         framebuffers, this is the sum of red/green/blue/alpha_color_bpp
 * @fb_level: The frame buffer level. Level zero is the default frame buffer.
 *         Positive levels correspond to frame buffers that overlay the default
 *         buffer, and negative levels correspond to frame buffers that underlie
 *         the default buffer
 * @double_buffered: Whether the framebuffer has separate front and back buffers
 * @stereo_buffered: Whether the framebuffer has separate left and right buffers
 * @aux_buffers: The number of auxiliary color buffers available
 * @red_color_bpp: The number of bits per pixel in the color buffer's red channel
 *         (only for RGBA contexts)
 * @green_color_bpp: The number of bits per pixel in the color buffer's green
 *         channel (only for RGBA contexts)
 * @blue_color_bpp: The number of bits per pixel in the color buffer's blue
 *         channel (only for RGBA contexts)
 * @alpha_color_bpp: The number of bits per pixel in the color buffer's alpha
 *         channel (only for RGBA contexts)
 * @depth_bpp: The number of bits per pixel in the depth buffer, or zero if the
 *         the framebuffer does not have a depth buffer
 * @stencil_bpp: The number of bits per pixel in the stencil buffer, or zero if
 *         the the framebuffer does not have a stencil buffer
 * @red_accum_bpp: The number of bits per pixel in the accumulation buffer's red
 *         channel, or zero if there is no accumulation buffer
 * @green_accum_bpp: The number of bits per pixel in the accumulation buffer's
 *         green channel, or zero if there is no accumulation buffer
 * @blue_accum_bpp: The number of bits per pixel in the accumulation buffer's
 *         blue channel, or zero if there is no accumulation buffer
 * @alpha_accum_bpp: The number of bits per pixel in the accumulation buffer's
 *         alpha channel, or zero if there is no accumulation buffer
 * @transparent_type: The type of transparent color used by the framebuffer
 * @transparent_index: The index of the transparent color (only in color index
 *         framebuffers)
 * @transparent_red: The red value of the transparent color (only in RGBA
 *         framebuffers)
 * @transparent_green: The green value of the transparent color (only in RGBA
 *         framebuffers)
 * @transparent_blue: The blue value of the transparent color (only in RGBA
 *         framebuffers)
 * @transparent_alpha: The alpha value of the transparent color (only in RGBA
 *         framebuffers)
 * @sample_buffers: The number of sample buffers available
 * @samples_per_pixel: The number of samples per pixel possible
 * @caveat: The framebuffer configuration's caveat, if any
 *
 * A structure describing framebuffer configurations platform-independently.
 */
typedef struct _GtkGLFramebufferConfig {
    gboolean accelerated;
    GtkGLBitSet color_types;
    guint color_bpp;
    gint fb_level;
    gboolean double_buffered;
    gboolean stereo_buffered;
    guint aux_buffers;
    guint red_color_bpp;
    guint green_color_bpp;
    guint blue_color_bpp;
    guint alpha_color_bpp;
    guint depth_bpp;
    guint stencil_bpp;
    guint red_accum_bpp;
    guint green_accum_bpp;
    guint blue_accum_bpp;
    guint alpha_accum_bpp;
    GtkGLTransparentType transparent_type;
    guint transparent_index;
    guint transparent_red;
    guint transparent_green;
    guint transparent_blue;
    guint transparent_alpha;
    guint sample_buffers;
    guint samples_per_pixel;
    GtkGLCaveat caveat;
} GtkGLFramebufferConfig;


/**
 * GtkGLAttribute:
 * @GTK_GL_NONE: Marks the end of the attribute list
 * @GTK_GL_ACCELERATED: Marks #GtkGLFramebufferConfig.accelerated
 * @GTK_GL_COLOR_TYPES: Marks #GtkGLFramebufferConfig.color_types
 * @GTK_GL_COLOR_BPP: Marks #GtkGLFramebufferConfig.color_bpp
 * @GTK_GL_FB_LEVEL: Marks #GtkGLFramebufferConfig.fb_level
 * @GTK_GL_DOUBLE_BUFFERED: Marks #GtkGLFramebufferConfig.double_buffered
 * @GTK_GL_STEREO_BUFFERED: Marks #GtkGLFramebufferConfig.stereo_buffered
 * @GTK_GL_AUX_BUFFERS: Marks #GtkGLFramebufferConfig.aux_buffers
 * @GTK_GL_RED_COLOR_BPP: Marks #GtkGLFramebufferConfig.red_color_bpp
 * @GTK_GL_GREEN_COLOR_BPP: Marks #GtkGLFramebufferConfig.green_color_bpp
 * @GTK_GL_BLUE_COLOR_BPP: Marks #GtkGLFramebufferConfig.blue_color_bpp
 * @GTK_GL_ALPHA_COLOR_BPP: Marks #GtkGLFramebufferConfig.alpha_color_bpp
 * @GTK_GL_DEPTH_BPP: Marks #GtkGLFramebufferConfig.depth_bpp
 * @GTK_GL_STENCIL_BPP: Marks #GtkGLFramebufferConfig.stencil_bpp
 * @GTK_GL_RED_ACCUM_BPP: Marks #GtkGLFramebufferConfig.red_accum_bpp
 * @GTK_GL_GREEN_ACCUM_BPP: Marks #GtkGLFramebufferConfig.green_accum_bpp
 * @GTK_GL_BLUE_ACCUM_BPP: Marks #GtkGLFramebufferConfig.blue_accum_bpp
 * @GTK_GL_ALPHA_ACCUM_BPP: Marks #GtkGLFramebufferConfig.alpha_accum_bpp
 * @GTK_GL_TRANSPARENT_TYPE: Marks #GtkGLFramebufferConfig.transparent_type
 * @GTK_GL_TRANSPARENT_INDEX_VALUE: Marks
 *         #GtkGLFramebufferConfig.transparent_index
 * @GTK_GL_TRANSPARENT_RED: Marks #GtkGLFramebufferConfig.transparent_red
 * @GTK_GL_TRANSPARENT_GREEN: Marks #GtkGLFramebufferConfig.transparent_green
 * @GTK_GL_TRANSPARENT_BLUE: Marks #GtkGLFramebufferConfig.transparent_blue
 * @GTK_GL_TRANSPARENT_ALPHA: Marks #GtkGLFramebufferConfig.transparent_alpha
 * @GTK_GL_SAMPLE_BUFFERS: Marks #GtkGLFramebufferConfig.sample_buffers
 * @GTK_GL_SAMPLES_PER_PIXEL: Marks #GtkGLFramebufferConfig.samples_per_pixel
 * @GTK_GL_CAVEAT: Marks #GtkGLFramebufferConfig.caveat
 *
 * Tokens to mark framebuffer attributes in #gtk_gl_choose_visuals()
 */
typedef enum _GtkGLAttribute {
    GTK_GL_NONE,
    GTK_GL_ACCELERATED,
    GTK_GL_COLOR_TYPES,
    GTK_GL_COLOR_BPP,
    GTK_GL_FB_LEVEL,
    GTK_GL_DOUBLE_BUFFERED,
    GTK_GL_STEREO_BUFFERED,
    GTK_GL_AUX_BUFFERS,
    GTK_GL_RED_COLOR_BPP,
    GTK_GL_GREEN_COLOR_BPP,
    GTK_GL_BLUE_COLOR_BPP,
    GTK_GL_ALPHA_COLOR_BPP,
    GTK_GL_DEPTH_BPP,
    GTK_GL_STENCIL_BPP,
    GTK_GL_RED_ACCUM_BPP,
    GTK_GL_GREEN_ACCUM_BPP,
    GTK_GL_BLUE_ACCUM_BPP,
    GTK_GL_ALPHA_ACCUM_BPP,
    GTK_GL_TRANSPARENT_TYPE,
    GTK_GL_TRANSPARENT_INDEX_VALUE,
    GTK_GL_TRANSPARENT_RED,
    GTK_GL_TRANSPARENT_GREEN,
    GTK_GL_TRANSPARENT_BLUE,
    GTK_GL_TRANSPARENT_ALPHA,
    GTK_GL_SAMPLE_BUFFERS,
    GTK_GL_SAMPLES_PER_PIXEL,
    GTK_GL_CAVEAT
} GtkGLAttribute;


/**
 * GtkGLRequirementType:
 * @GTK_GL_PREFERABLY: Configurations in which the attribute matches the
 *         reference value are preferred over other configurations
 * @GTK_GL_EXACTLY: The attribute value must match the reference value exactly
 * @GTK_GL_AT_MOST: The attribute value must not be greater than the reference
 *         value. Smaller values will be preferred
 * @GTK_GL_AT_LEAST: The attribute value must not be less than the reference
 *         value. Greater values will be preferred
 *
 * Defines the type of requirement imposed on a certain framebuffer attribute
 * in #gtk_gl_choose_visuals
 */
typedef enum _GtkGLRequirementType {
    GTK_GL_PREFERABLY,
    GTK_GL_EXACTLY,
    GTK_GL_AT_MOST,
    GTK_GL_AT_LEAST
} GtkGLRequirementType;


/**
 * GtkGLRequirement:
 * @attr: The attribute
 * @req: How the attribute should be compared to the reference value
 * @value: The reference value
 *
 * Defines a requirement for filtering and sorting framebuffer configurations
 * in #gtk_gl_choose_visuals()
 */
typedef struct _GtkGLRequirement {
    GtkGLAttribute attr;
    GtkGLRequirementType req;
    gint value;
} GtkGLRequirement;


/**
 * GTK_GL_LIST_END:
 * Terminator token for the requirement list in #gtk_gl_choose_visuals()
 */
#define GTK_GL_LIST_END { GTK_GL_NONE, GTK_GL_EXACTLY, 0 }


/**
 * gtk_gl_choose_visuals:
 * @pool: The list of visuals to choose from
 * @requirements: An array of #GtkGLRequirement, terminated by #GTK_GL_LIST_END
 *
 * Filters and sorts a GtkGLVisualList by the criteria provided in
 * the requirements parameter.
 * Returns: A filtered and sorted #GtkGLVisualList, owned by the caller
 */
GtkGLVisualList *gtk_gl_choose_visuals(const GtkGLVisualList *pool,
        const GtkGLRequirement *requirements);


/**
 * gtk_gl_visual_free:
 * @vis: The visual
 *
 * Frees a visual that was returned by #gtk_gl_canvas_enumerate_visuals()
 * before.
 *
 * You should consider using #gtk_gl_visual_list_free instead.
 */
void gtk_gl_visual_free(GtkGLVisual *vis);


/**
 * gtk_gl_visual_list_new:
 * @is_owner: Whether a subsequent call to #gtk_gl_visual_list_free() should
 *                 attempt to free the list entries
 * @count: The number of entries
 *
 * Creates a new, empty visual list of the desired size.
 * Returns: The new list
 */
GtkGLVisualList *gtk_gl_visual_list_new(gboolean is_owner, gsize count);


/**
 * gtk_gl_visual_list_free:
 * @visuals: The list
 *
 * Frees a #GtkGLVisualList.
 *
 * Optionally frees the entries if the list structure owns them.
 */
void gtk_gl_visual_list_free(GtkGLVisualList *visuals);


/**
 * gtk_gl_describe_visual:
 * @visual: The visual
 * @out: The framebuffer configuration to write to
 * Fills a #GtkGLFramebufferConfig structure from a backend-dependent visual.
 */
void gtk_gl_describe_visual(const GtkGLVisual *visual,
        GtkGLFramebufferConfig *out);


G_END_DECLS
