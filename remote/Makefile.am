if HAVE_UNIQUE
bin_PROGRAMS=gmpc-remote

bin_SCRIPTS=gmpc-remote-stream

gmpc_remote_SOURCES=\
		    $(top_srcdir)/src/Tools/ipc.c\
		    main.c



INCLUDES= @EXTRA_CFLAGS@ -Wall\
	-DVERSION=\"@VERSION@\"\
	-DGMPC_BUILD=1\
	-DCLIENT_ONLY\
	-I$(top_srcdir)\
	-I$(top_builddir)\
	-I$(top_srcdir)/src/Tools\
	-I$(top_builddir)/src/Tools\
	-I$(top_srcdir)/src\
	-I$(top_builddir)/src

LIBS+= \
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
else
EXTRA_DIST = \
    main.c\
    gmpc-remote-stream

endif
