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


dnl Process this file with autoconf to produce a configure script.
dnl Created by Anjuta application wizard.


AC_INIT(gtkglcanvas, 0.1)

AC_CONFIG_HEADERS([config.h])

AM_INIT_AUTOMAKE([1.11])

AM_SILENT_RULES([yes])

AC_PROG_CC


dnl ***************************************************************************
dnl Check for Windows
dnl ***************************************************************************
AC_CANONICAL_HOST

case $host_os in
  *mingw*)
    platform_win32=yes
    native_win32=yes
    ;;
  pw32* | *cygwin*)
    platform_win32=yes
    native_win32=no
    ;;
  *)
    platform_win32=no
    native_win32=no
    ;;
esac

PKG_CHECK_MODULES([GTK], [gtk+-3.0], [],
    [AC_MSG_ERROR([Missing dependency: GTK+3])])
PKG_CHECK_MODULES([GLEW], [glew], [],
    [AC_MSG_ERROR([Missing dependency: libGLEW])])

PKG_CHECK_MODULES([GLADEUI], [gladeui-2.0],
    [have_glade=yes;
        ac_gladeui_catdir=`$PKG_CONFIG --variable=catalogdir gladeui-2.0`;
        ac_gladeui_pixmapdir=`$PKG_CONFIG --variable=pixmapdir gladeui-2.0`],
    [ac_gladeui_catdir=; have_glade=no])

if test x"$platform_win32" = "xno"; then
    PKG_CHECK_MODULES([OpenGL], [gl], [],
    [AC_MSG_ERROR([Missing dependency: libGL])])
    PKG_CHECK_MODULES([X11], [x11], [],
    [AC_MSG_ERROR([Missing dependendy: X11])])
fi

AM_CONDITIONAL(PLATFORM_WIN32, test x"$platform_win32" = "xyes")

AM_CONDITIONAL(NATIVE_WIN32, test x"$native_win32" = "xyes")
AM_CONDITIONAL(HAVE_GLADEUI, test x"$have_glade" = "xyes")

srcdir=`readlink -f "$srcdir"`
builddir=`readlink -f "$top_builddir"`
AC_DEFINE_UNQUOTED([PREFIX], ["$prefix"], [Installation Prefix])
AC_DEFINE_UNQUOTED([SOURCE_DIR], ["$srcdir"], [Absolute Source Directory])

AC_SUBST(GLADEUI_CATDIR, "$ac_gladeui_catdir")
AC_SUBST(GLADEUI_PIXMAPDIR, "$ac_gladeui_pixmapdir")

pkgconfig_dir="`$PKG_CONFIG --debug 2>&1 | sed -ne "/Scanning directory /{s///;s/'//gp;q;}"`"
AC_SUBST(PKG_CONFIG_DIR, "$pkgconfig_dir")

LT_INIT([win32-dll])

AC_CONFIG_MACRO_DIR([m4])


AC_OUTPUT([
Makefile
src/Makefile
src/libgtkglcanvas/Makefile
src/example/Makefile
])
