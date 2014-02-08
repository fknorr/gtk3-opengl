## Process this file with automake to produce Makefile.in


lib_LTLIBRARIES =  \
	libgtk3-opengl.la

if PLATFORM_WIN32
libgtk3_opengl_la_SOURCES = \
	wgl.c
else
libgtk3_opengl_la_SOURCES = \
	xgl.c
endif

libgtk3_opengl_la_CPPFLAGS = @GTK_CFLAGS@
libgtk3_opengl_la_LIBADD = @GTK_LIBS@

include_HEADERS = \
	gtk3-opengl.h
