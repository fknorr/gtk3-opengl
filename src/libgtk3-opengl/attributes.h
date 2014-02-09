/** 
 * Copyright (c) 2014, Fabian Knorr
 * 
 * This file is part of libgtk3-opengl.
 *
 * libgtk3-opengl is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * libgtk3-opengl is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libgtk3-opengl. If not, see <http://www.gnu.org/licenses/>.
 */


#pragma once

enum
{
	GTKGL_DOUBLE_BUFFERED = 1,
	GTKGL_STEREO = 2,
	GTKGL_SAMPLE_BUFFERS = 4
};

typedef struct _GtkGLAttributes GtkGLAttributes;

struct _GtkGLAttributes {
	unsigned flags;
	unsigned num_samples;
	unsigned depth_buffer_bits;
	unsigned stencil_buffer_bits;
};
