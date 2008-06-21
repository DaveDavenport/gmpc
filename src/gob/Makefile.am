noinst_LTLIBRARIES = libcwidgets.la

nodist_libcwidgets_la_SOURCES =\
	gmpc-connection.c\
	gmpc-metaimage.c\
	gmpc-meta-watcher.c\
	gmpc-meta-text-view.c\
	gmpc-mpddata-model.c\
	gmpc-mpddata-model-sort.c\
	gmpc-liststore-sort.c\
	gmpc-mpddata-model-playlist.c\
	gmpc-mpddata-treeview.c\
	gmpc-clicklabel.c\
	gmpc-stats-label.c\
	gmpc-signals.c\
	gmpc-idle.c\
	gmpc-profiles.c

BUILT_SOURCES=\
			  $(nodist_libcwidgets_la_SOURCES)

%.c %.h %-private.h: %.gob
	@GOB2@ --no-touch-headers --always-private-header $<
	@touch $@

EXTRA_DIST=\
		gmpc-profiles.gob\
		gmpc-connection.gob\
		gmpc-meta-watcher.gob\
		gmpc-metaimage.gob\
		gmpc-meta-text-view.gob\
		gmpc-mpddata-model.gob\
		gmpc-mpddata-model-playlist.gob\
		gmpc-mpddata-model-sort.gob\
		gmpc-liststore-sort.gob\
		gmpc-mpddata-treeview.gob\
		gmpc-clicklabel.gob\
		gmpc-stats-label.gob\
		gmpc-signals.gob\
		gmpc-idle.gob

INCLUDES=\
	@EXTRA_CFLAGS@\
	-I$(top_srcdir)\
	-I$(top_builddir)\
	-I$(top_srcdir)/src\
	-I$(top_builddir)/src\
	-I$(top_srcdir)/src/egg\
	-I$(top_builddir)/src/egg\
	-I$(top_srcdir)/src/bacon\
	-I$(top_builddir)/src/bacon\
	-DPIXMAP_PATH=\""$(pkgdatadir)"\" \
	-DGLADE_PATH=\""$(pkgdatadir)"\" \
	-DVERSION=\"@VERSION@\"\
	-DGMPC_BUILD=1\
	@CFLAGS@\
	@glib_CFLAGS@\
	@libmpd_CFLAGS@ \
	@gobject_CFLAGS@\
	@gtk_CFLAGS@\
	@gmodule_CFLAGS@\
	@libglade_CFLAGS@\
	@gthread_CFLAGS@\
	@sm_CFLAGS@


clean-local:
	-rm *.c
	-rm *.h
