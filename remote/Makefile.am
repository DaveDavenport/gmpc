if OS_WIN32

else
bin_PROGRAMS=gmpc-remote
gmpc_remote_SOURCES=\
		main.c\
		bacon-message-connection.c


INCLUDES= @EXTRA_CFLAGS@ -Wall\
	-DVERSION=\"@VERSION@\"\
	-DGMPC_BUILD=1

LIBS= \
	@glib_LIBS@ @LIBS@ \
	@gobject_LIBS@\
	@gtk_LIBS@\
	@gmodule_LIBS@


AM_LDFLAGS = \
	@EXTRA_LDFLAGS@\
	@LDFLAGS@

AM_CFLAGS = @CFLAGS@\
	@glib_CFLAGS@\
	@gobject_CFLAGS@\
	@gtk_CFLAGS@\
	@gmodule_CFLAGS@


EXTRA_DIST = \
		bacon-message-connection.h
endif		
