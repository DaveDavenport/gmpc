if ENABLE_TEST
noinst_PROGRAMS = \
	pixbuf_cache_test

pixbuf_cache_test_CFLAGS = @CFLAGS@\
		@glib_CFLAGS@\
		@libmpd_CFLAGS@\
		@gtk_CFLAGS@\
		-I$(top_srcdir)/src\
		-I$(top_srcdir)/src/vala\
        -I$(top_srcdir)/src/MetaData\
		-DPIXMAP_PATH=\""$(pkgdatadir)/icons/"\" \
		-DGLADE_PATH=\""$(pkgdatadir)"\" 

pixbuf_cache_test_LDADD = @LIBS@\
		@libmpd_LIBS@\
		@glib_LIBS@\
		@gtk_LIBS@

pixbuf_cache_test_SOURCES = \
		pixbuf_cache_test.c 

nodist_pixbuf_cache_test_SOURCES = \        
        $(top_srcdir)/src/Widgets/pixbuf-cache.c

endif
