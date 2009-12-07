if ENABLE_TEST
noinst_PROGRAMS = \
	mtc_test

mtc_test_CFLAGS = @CFLAGS@\
        @gtk_CFLAGS@\
		@glib_CFLAGS@\
		@libmpd_CFLAGS@\
        @sqlite3_CFLAGS@\
        @libsoup_CFLAGS@\
        -DPACKAGE_DATA_DIR="\"\""\
        -DPIXMAP_PATH="\"\""\
        -DGLADE_PATH="\"\""\
		-I$(top_srcdir)/src/gob\
		-I$(top_srcdir)/src/vala\
		-I$(top_srcdir)/src/Widgets\
		-I$(top_srcdir)/src/MetaData\
		-I$(top_srcdir)/src

mtc_test_LDADD = @LIBS@\
		@libmpd_LIBS@\
        @gtk_LIBS@\
        @sqlite3_LIBS@\
        @libsoup_LIBS@\
		@glib_LIBS@

mtc_test_SOURCES = \
		mtc_test.c 

nodist_mtc_test_SOURCES = \        
		$(top_srcdir)/src/config1.c\
		$(top_srcdir)/src/MetaData/metadata.c\
		$(top_srcdir)/src/plugin.c\
		$(top_srcdir)/src/misc.c\
        $(top_srcdir)/src/gmpc_easy_download.c\
        $(top_srcdir)/src/gmpc-meta-watcher.c\
        $(top_srcdir)/src/vala/gmpc-plugin.c\
		$(top_srcdir)/src/MetaData/metadata-cache.c

endif
