if ENABLE_TEST
noinst_PROGRAMS = \
	read_conf

read_conf_CFLAGS = @CFLAGS@\
		@glib_CFLAGS@\
		@libmpd_CFLAGS@\
		-I$(top_srcdir)/src

read_conf_LDADD = @LIBS@\
		@libmpd_LIBS@\
		@glib_LIBS@

read_conf_SOURCES = \
		read_conf.c 

nodist_read_conf_SOURCES = \
		$(top_srcdir)/src/config1.c

endif
