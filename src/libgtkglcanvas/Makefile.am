# This file is part of libgtkglcanvas.
#
# libgtkglcanvas is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published
# by the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# libgtkglcanvas is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with libgtkglcanvas.  If not, see <http://www.gnu.org/licenses/>.


lib_LTLIBRARIES =  \
	$(top_builddir)/libgtkglcanvas.la

if PLATFORM_WIN32
platform_sources = wgl.c
platform_def = -DPLATFORM_WIN32
platform_cflags = $(X11_CFLAGS) $(GL_CFLAGS)
platform_libs =
else
platform_sources = glx.c
platform_def = -DPLATFORM_X11
platform_libs = $(X11_LIBS) $(GL_LIBS)
endif

__top_builddir__libgtkglcanvas_la_SOURCES = \
	visual.c \
	canvas.c \
	$(platform_sources)

gtkgldir = $(includedir)/gtkgl
gtkgl_HEADERS = \
    $(top_srcdir)/include/gtkgl/canvas.h \
    $(top_srcdir)/include/gtkgl/visual.h

if HAVE_GLADEUI
gladecatdir = $(GLADEUI_CATDIR)
gladecat_DATA = $(top_srcdir)/res/glade/catalogs/gtkglcanvas.xml
gladepix22dir = $(GLADEUI_PIXMAPDIR)/hicolor/22x22/actions/
gladepix22_DATA = $(top_srcdir)/res/glade/pixmaps/22x22/widget-gtkgl-glcanvas.png
gladepix16dir = $(GLADEUI_PIXMAPDIR)/hicolor/16x16/actions/
gladepix16_DATA = $(top_srcdir)/res/glade/pixmaps/16x16/widget-gtkgl-glcanvas.png
endif

gtkglcanvas.pc:
	sed "s!%!$(prefix)!" $(top_srcdir)/res/gtkglcanvas/gtkglcanvas.pc.tmpl > gtkglcanvas.pc

pkgconfigdir = $(PKG_CONFIG_DIR)
pkgconfig_DATA = gtkglcanvas.pc

__top_builddir__libgtkglcanvas_la_CFLAGS = \
    -I$(top_srcdir)/include \
	$(GTK_CFLAGS) \
	$(OpenGL_CFLAGS) \
	$(GLEW_CFLAGS) \
	$(platform_def) \
	$(platform_cflags)

__top_builddir__libgtkglcanvas_la_LIBADD = \
	$(GTK_LIBS) \
	$(OpenGL_LIBS) \
	$(GLEW_LIBS) \
	$(platform_libs)
