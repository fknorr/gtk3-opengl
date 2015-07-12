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


check_PROGRAMS = $(top_builddir)/example

__top_builddir__example_SOURCES = \
	main.c

__top_builddir__example_LDADD = \
	$(top_builddir)/libgtkglcanvas.la \
	-lm \
	$(GTK_LIBS) \
	$(OpenGL_LIBS) \
	$(GLEW_LIBS)

__top_builddir__example_CPPFLAGS = \
	$(OpenGL_CFLAGS) \
	$(GLEW_CFLAGS) \
    $(GTK_CFLAGS)

if PLATFORM_WIN32
__top_builddir__example_LDFLAGS = \
	-Wl,--export-all-symbols
else
__top_builddir__example_CFLAGS = \
	-rdynamic
endif

$(top_builddir)/example.ui:
	cp $(srcdir)/example.ui $(top_builddir)

__top_builddir__example_DEPENDENCIES = \
	$(top_builddir)/example.ui
