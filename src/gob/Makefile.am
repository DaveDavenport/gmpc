V = @
Q = $(V:1=)
QUIET_GEN =  $(Q:@=@echo '  GEN   '$@;)

noinst_LTLIBRARIES = libcwidgets.la

gmpc_GOBFILES = \
		gmpc-profiles.gob\
		gmpc-connection.gob\
		gmpc-meta-watcher.gob\
		gmpc-metaimage.gob\
		gmpc-meta-text-view.gob\
		gmpc-mpddata-model.gob\
		gmpc-mpddata-model-playlist.gob\
		gmpc-mpddata-model-sort.gob\
		gmpc-mpddata-treeview.gob\
		gmpc-clicklabel.gob\
		gmpc-stats-label.gob\
		gmpc-signals.gob

gmpc_GOBSTAMPFILES=\
	$(gmpc_GOBFILES:.gob=.gob.stamp)

gmpc_GOBCFILES=\
	$(gmpc_GOBFILES:.gob=.c)

gmpc_GOBHFILES=\
	$(gmpc_GOBFILES:.gob=.h)\
	$(gmpc_GOBFILES:.gob=-private.h)


nodist_libcwidgets_la_SOURCES=\
	$(gmpc_GOBCFILES)

libcwidgetsincludedir =$(includedir)/gmpc/

libcwidgetsinclude_HEADERS=\
						   $(gmpc_GOBHFILES)


BUILT_SOURCES=\
			  $(gmpc_GOBSTAMPFILES)

%.gob.stamp: %.gob
	$(QUIET_GE) echo "  GOB   $<"
	@GOB2@ --always-private-header $<
	@touch $@

EXTRA_DIST=\
	$(gmpc_GOBFILES)

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


CLEANFILES=\
	$(gmpc_GOBCFILES)\
	$(gmpc_GOBHFILES)\
	$(gmpc_GOBSTAMPFILES)
