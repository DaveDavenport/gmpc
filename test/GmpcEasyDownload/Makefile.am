if ENABLE_TEST
noinst_PROGRAMS = \
    gmpc_easy_download	

gmpc_easy_download_CFLAGS = @CFLAGS@\
		@glib_CFLAGS@\
		@libmpd_CFLAGS@\
		@gtk_CFLAGS@\
        @libsoup_CFLAGS@\
		-I$(top_srcdir)/src\
		-I$(top_srcdir)/src/vala\
		-I$(top_srcdir)/src/gob\
		-I$(top_srcdir)/src/MetaData\
		-DPIXMAP_PATH=\""$(pkgdatadir)/icons/"\" \
		-DGLADE_PATH=\""$(pkgdatadir)"\" 

gmpc_easy_download_LDADD = @LIBS@\
		@libmpd_LIBS@\
		@glib_LIBS@\
		@gtk_LIBS@\
        @libsoup_LIBS@

gmpc_easy_download_SOURCES = \
		ged.c 

nodist_gmpc_easy_download_SOURCES = \
		$(top_srcdir)/src/misc.c\
		$(top_srcdir)/src/gmpc_easy_download.c

endif
