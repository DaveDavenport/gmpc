if ENABLE_TEST
noinst_PROGRAMS = \
   discogs 

discogs_CFLAGS = @CFLAGS@\
		@glib_CFLAGS@\
		@libmpd_CFLAGS@\
		@gtk_CFLAGS@\
        @libsoup_CFLAGS@\
		-I$(top_srcdir)/src\
		-I$(top_srcdir)/src/vala\
		-I$(top_srcdir)/src/Tools\
		-I$(top_srcdir)/src/Widgets\
		-I$(top_srcdir)/src/gob\
		-I$(top_srcdir)/src/MetaData\
        -DPACKAGE_DATA_DIR="\"\""\
        -DPIXMAP_PATH="\"\""\
        -DGLADE_PATH="\"\""

discogs_LDADD = @LIBS@\
		@libmpd_LIBS@\
		@glib_LIBS@\
		@gtk_LIBS@\
        @libsoup_LIBS@\
        @sqlite3_LIBS@

discogs_SOURCES = \
		test.c 

nodist_discogs_SOURCES = \
		$(top_srcdir)/src/plugin.c\
		$(top_srcdir)/src/Tools/misc.c\
		$(top_srcdir)/src/MetaData/metadata.c\
		$(top_srcdir)/src/Tools/gmpc_easy_download.c\
		$(top_srcdir)/src/Providers/DiscoGS.c\
        $(top_srcdir)/src/vala/gmpc-plugin.c\
		$(top_srcdir)/src/MetaData/metadata-cache-sqlite.c\
		$(top_srcdir)/src/MetaData/metadata-cache.c

endif
