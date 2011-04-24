if ENABLE_TEST
noinst_PROGRAMS = \
	misc_test

misc_test_CFLAGS = @CFLAGS@ -g -O2\
		@glib_CFLAGS@\
		@libmpd_CFLAGS@\
		@gtk_CFLAGS@\
		-I$(top_srcdir)/src\
		-I$(top_srcdir)/src/vala\
        -I$(top_srcdir)/src/MetaData\
        -I$(top_srcdir)/src/Tools/\
        -I$(top_srcdir)/src/Widgets/\
		-DPIXMAP_PATH=\""$(pkgdatadir)/icons/"\" \
		-DGLADE_PATH=\""$(pkgdatadir)"\"\
        -D__MISC_TEST

misc_test_LDADD = @LIBS@\
		@libmpd_LIBS@\
		@glib_LIBS@\
		@gtk_LIBS@

misc_test_SOURCES = \
		misc_test.c

nodist_misc_test_SOURCES = \
        $(top_srcdir)/src/Tools/misc.c

endif
