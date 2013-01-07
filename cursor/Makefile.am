
MAINTAINERCLEANFILES = Makefile.in

lib_LTLIBRARIES = libxcb-cursor.la

xcbinclude_HEADERS = xcb_cursor.h

AM_CFLAGS = $(CWARNFLAGS)

libxcb_cursor_la_SOURCES = cursor.c shape_to_id.c load_cursor.c parse_cursor_file.c
libxcb_cursor_la_CPPFLAGS = $(XCB_CFLAGS) $(XPROTO_CFLAGS)
libxcb_cursor_la_LIBADD = $(XCB_LIBS) $(XPROTO_LIBS)
libxcb_cursor_la_LDFLAGS = -version-info 0:0:0 -no-undefined -export-symbols-regex '^xcb_cursor_'

pkgconfig_DATA = xcb-cursor.pc

EXTRA_DIST = xcb-cursor.pc.in

# TODO: We cannot use --pic because then the gperf-generated code does not compile :-/
GPERFFLAGS = --includes --struct-type --language=ANSI-C --switch=1
CLEANFILES = shape_to_id.c

shape_to_id.c: shape_to_id.gperf
	if $(GPERF) $(GPERFFLAGS) shape_to_id.gperf >$@t; then \
	  mv $@t $@; \
	elif $(GPERF) --version >/dev/null 2>&1; then \
	  rm $@t; \
	  exit 1; \
	else \
	  rm $@t; \
	  touch $@; \
	fi
