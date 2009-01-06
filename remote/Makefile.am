bin_PROGRAMS=gmpc-remote

bin_SCRIPTS=gmpc-remote-stream

gmpc_remote_SOURCES=\
		main.c


INCLUDES= @EXTRA_CFLAGS@ -Wall\
	-DVERSION=\"@VERSION@\"\
	-DGMPC_BUILD=1\
	-I../src/

LIBS= \
	@glib_LIBS@ @LIBS@ \
	@gobject_LIBS@\
	@gtk_LIBS@\
	@unique_LIBS@\
	@gmodule_LIBS@


AM_LDFLAGS = \
	@EXTRA_LDFLAGS@\
	@LDFLAGS@

AM_CFLAGS = @CFLAGS@\
	@glib_CFLAGS@\
	@gobject_CFLAGS@\
	@gtk_CFLAGS@\
	@unique_CFLAGS@\
	@gmodule_CFLAGS@


EXTRA_DIST = \
			 gmpc-remote-stream
