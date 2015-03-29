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


/**
 * Enumerates possible flags for GL context creation.
 * All values are orthogonal and can be combined via the '|' operator.
 */
typedef enum _GtkGLFeature
{
    /**
     * Whether the context uses two framebuffers to avoid tearing.
     * If this flag is used, @ref gtk_gl_canvas_swap_buffers must be called
     * after drawing a frame in order to display it.
     */
	GTK_GL_DOUBLE_BUFFERED = 1,

    /**
     * Whether the context should supply a stereo buffer.
     */
	GTK_GL_STEREO = 2,

    /**
     * Whether the context should support multisampling. If this flag is
     * set in the @ref GtkGLAttributes structure, its @code num_samples
     * field determines the number of samples used.
     */
	GTK_GL_SAMPLE_BUFFERS = 4
} GtkGLFeature;


/**
 * Enumerates "support levels" for @ref GtkGLAttributes configurations.
 * Those levels can be queried using @ref gtk_gl_query_feature_support
 * or @ref gtk_gl_query_configuration_support.
 */
typedef enum _GtkGLSupport
{
    /**
     * This feature is not supported, and there is no way to create a
     * suitable fallback configuration.
     */
    GTK_GL_UNSUPPORTED,

    /**
     * Parts of this feature set are supported, but some features had to be
     * disabled or reduced to lower quality in order to find a suitable
     * configuration.
     */
    GTK_GL_PARTIALLY_SUPPORTED,

    /**
     * All parts of this configuration are supported, creating a context
     * with the attributes supplied will yield a matching or better
     * configuration.
     */
    GTK_GL_FULLY_SUPPORTED
} GtkGLSupport;


/**
 * Describes minimum requirements to a created OpenGL context.
 *
 * This structure is subject to extension in the future.
 */
typedef struct _GtkGLAttributes {
    /**
     * A combination of values form @ref GtkGLFeature
     */
	unsigned flags;

    /**
     * The number of samples if multisampling is enabled via
     * @ref GTK_GL_SAMPLE_BUFFERS.
     */
	unsigned num_samples;

    /**
     * The color buffer's depth in bits. Usually 16, 24 or 32.
     */
	unsigned color_buffer_bits;

    /**
     * The depth buffer's depth in bits, if any. Usually 16 or 24, but may be 0
     * to indicate that no depth buffer is required.
     */
	unsigned depth_buffer_bits;

    /**
     * The stencil buffer's depth in bits, if required. Usually 0 or 8.
     */
	unsigned stencil_buffer_bits;
} GtkGLAttributes;


/**
 * Returns whether a single feature is supported by system.
 *
 * @param feature The flag to check
 * @return A @ref GtkGLSupport value indicating the level of support
 */
GtkGLSupport gtk_gl_query_feature_support(GtkGLFeature feature);


/**
 * Returns whether a combination of features, known as a @ref GtkGLAttributes
 * structure, is fully or partially supported by the system.
 *
 * @param attrs The attributes to check
 * @return A @ref GtkGLSupport value indicating the level of support
 */
GtkGLSupport gtk_gl_query_configuration_support(const GtkGLAttributes *attrs);


G_END_DECLS
