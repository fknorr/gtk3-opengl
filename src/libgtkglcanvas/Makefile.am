# This file is part of libgtk3-opengl.
# 
# libgtk3-opengl is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published
# by the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# 
# libgtk3-opengl is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU Lesser General Public License
# along with libgtk3-opengl.  If not, see <http://www.gnu.org/licenses/>.


lib_LTLIBRARIES =  \
	$(top_builddir)/libgtkglcanvas.la

if PLATFORM_WIN32
platform_sources = wgl.c
platform_def = -DPLATFORM_WIN32
else
platform_sources = glx.c
platform_def = -DPLATFORM_X11
endif

__top_builddir__libgtkglcanvas_la_SOURCES = \
	canvas.c \
	$(platform_sources)

gtkgldir = $(includedir)/gtkgl
gtkgl_HEADERS = canvas.h attributes.h

if HAVE_GLADEUI
gladedir = $(GLADEUI_CATDIR)
glade_DATA = gtkglcanvas.xml
endif

__top_builddir__libgtkglcanvas_la_CFLAGS = \
	@GTK_CFLAGS@ $(platform_def)
__top_builddir__libgtkglcanvas_la_LIBADD = \
	@GTK_LIBS@
