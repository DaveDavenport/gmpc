if ENABLE_TEST
noinst_PROGRAMS = \
	async_image_test

async_image_test_CFLAGS = @CFLAGS@\
		@glib_CFLAGS@\
		@libmpd_CFLAGS@\
		@gtk_CFLAGS@\
		-I$(top_srcdir)/src\
		-I$(top_srcdir)/src/vala\
        -I$(top_srcdir)/src/MetaData\
        -I$(top_srcdir)/src/Tools\
        -I$(top_srcdir)/src/Widgets\
		-DPIXMAP_PATH=\""$(pkgdatadir)/icons/"\" \
		-DGLADE_PATH=\""$(pkgdatadir)"\"

async_image_test_LDADD = @LIBS@\
		@libmpd_LIBS@\
		@glib_LIBS@\
		@gtk_LIBS@

async_image_test_SOURCES = \
		async_image_test.c

nodist_async_image_test_SOURCES = \
        $(top_srcdir)/src/Widgets/gmpc-image-async.c\
        $(top_srcdir)/src/Tools/pixbuf-cache.c\
        $(top_srcdir)/src/Tools/misc.c

endif
