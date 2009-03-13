V = @
Q = $(V:1=)
QUIET_GEN =  $(Q:@=@echo '  GEN   '$@;)

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
	gmpc-profiles.c

libcwidgetsincludedir =$(includedir)/gmpc/
libcwidgetsinclude_HEADERS=\
						   gmpc-profiles.h\
						   gmpc-mpddata-model.h\
						   gmpc-mpddata-model-sort.h\
						   gmpc-mpddata-treeview.h\
						   gmpc-meta-watcher.h\
						   gmpc-clicklabel.h\
						   gmpc-metaimage.h\
						   gmpc-connection.h\
						   gmpc-signals.h



BUILT_SOURCES=\
			  $(nodist_libcwidgets_la_SOURCES)

%.c %.h %-private.h: %.gob
	$(QUIET_GE) echo "  GOB   $<"
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
		gmpc-signals.gob

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
	-I$(top_srcdir)/src/vala\
	-I$(top_builddir)/src/vala\
	-DPIXMAP_PATH=\""$(pkgdatadir)"\" \
	-DGLADE_PATH=\""$(pkgdatadir)"\" \
	-DVERSION=\"@VERSION@\"\
	-DPACKAGE_LIB_DIR=\""$(pkglibdir)"\" \
	-DGMPC_BUILD=1
if MAINTAINER_MODE
MAYBE_CFLAGS=-Wall -Werror -Wdeclaration-after-statement -Wshadow 
endif
AM_CFLAGS=\
	  @CFLAGS@\
	  @glib_CFLAGS@\
	@libmpd_CFLAGS@ \
	@gobject_CFLAGS@\
	@gtk_CFLAGS@\
	@gmodule_CFLAGS@\
	@libglade_CFLAGS@\
	@gthread_CFLAGS@\
	@sm_CFLAGS@\
	$(MAYBE_CFLAGS)


clean-local:
	-rm *.c
	-rm *.h
