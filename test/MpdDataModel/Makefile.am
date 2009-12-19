if ENABLE_TEST
noinst_PROGRAMS = \
	mpd_data_model

mpd_data_model_CFLAGS = @CFLAGS@\
		@glib_CFLAGS@\
		@libmpd_CFLAGS@\
		@gtk_CFLAGS@\
		-I$(top_srcdir)/src\
		-I$(top_srcdir)/src/vala\
        -I$(top_srcdir)/src/MetaData\
		-DPIXMAP_PATH=\""$(pkgdatadir)/icons/"\" \
		-DGLADE_PATH=\""$(pkgdatadir)"\" 

mpd_data_model_LDADD = @LIBS@\
		@libmpd_LIBS@\
		@glib_LIBS@\
		@gtk_LIBS@

mpd_data_model_SOURCES = \
		mpd_data_model.c 

nodist_mpd_data_model_SOURCES = \        
		$(top_srcdir)/src/misc.c\
		$(top_srcdir)/src/gmpc-mpddata-model.c\
        $(top_srcdir)/src/Widgets/gmpc-image-async.c\
        $(top_srcdir)/src/Widgets/pixbuf-cache.c

endif
