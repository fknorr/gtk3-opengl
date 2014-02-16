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


check_PROGRAMS = example

example_SOURCES = \
	main.c

example_LDADD = \
	../libgtk3-opengl/libgtk3-opengl.la -lm -lGL \
	@GTK_LIBS@

example_CPPFLAGS = \
    @GTK_CFLAGS@

example_CFLAGS = \
	-rdynamic
