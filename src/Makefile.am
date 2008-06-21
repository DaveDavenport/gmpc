REVISION=$(shell git rev-parse --short master)
CONF_VERSION=0
-include Makefile.config

SUBDIRS=gob

bin_PROGRAMS=gmpc

sources=\
	main.c\
	plugin.c\
	mpdinteraction.c\
	preferences.c\
	tray-icon2.c\
	misc.c\
	playlist3.c\
	playlist3-messages.c\
	mm-keys.c\
	config1.c\
	sm.c\
	metadata.c\
	bacon/bacon-message-connection.c\
	browsers/playlist3-tag2-browser.c\
	browsers/playlist3-file-browser.c\
	browsers/playlist3-current-playlist-browser.c\
	browsers/playlist3-find2-browser.c\
	browsers/playlist3-metadata-browser.c\
	browsers/playlist3-playlist-editor.c\
	gmpc_easy_download.c\
	url_fetcher.c\
	egg/eggcolumnchooserdialog.c\
	egg/eggcolumnmodel.c\
	egg/eggcellrendererkeys.c\
	egg/eggaccelerators.c\
	TreeSearchWidget.c\
	qlib/qasyncqueue.c\
	sexy-icon-entry.c\
	edit_markup.c

gmpc_LDADD=\
			   gob/libcwidgets.la

#nodist_gmpc_SOURCES=\
#	gob/gmpc-connection.c\
#	gob/gmpc-metaimage.c\
#	gob/gmpc-meta-watcher.c\
#	gob/gmpc-meta-text-view.c\
#	gob/gmpc-mpddata-model.c\
#	gob/gmpc-mpddata-model-sort.c\
#	gob/gmpc-liststore-sort.c\
#	gob/gmpc-mpddata-model-playlist.c\
#	gob/gmpc-mpddata-treeview.c\
#	gob/gmpc-clicklabel.c\
#	gob/gmpc-stats-label.c\
#	gob/gmpc-signals.c\
#	gob/gmpc-idle.c\
#	gob/gmpc-profiles.c

if HAVE_RC
gmpc_SOURCES=$(sources) gmpc.rc
else
gmpc_SOURCES=$(sources)
endif

BUILT_SOURCES= revisiont.h
#	gob/gmpc-connection.c\
#	gob/gmpc-profiles.c\
#	gob/gmpc-mpddata-model.c\
#	gob/gmpc-mpddata-model-sort.c\
#	gob/gmpc-liststore-sort.c\
#	gob/gmpc-mpddata-model-playlist.c\
#	gob/gmpc-mpddata-treeview.c\
#	gob/gmpc-stats-label.c\
#	gob/gmpc-clicklabel.c\
#	gob/gmpc-meta-text-view.c\
#	gob/gmpc-meta-watcher.c\
#	gob/gmpc-metaimage.c\
#	gob/gmpc-idle.c\
#	gob/gmpc-signals.c

%.c %.h %-private.h: %.gob
	@GOB2@ -o gob/ --no-touch-headers --always-private-header $<
	@touch $@

%.o: %.rc
	$(LIBTOOL) --mode=compile --tag=RC $(RC) -i $^ -o $@

%.rc.o:
	$(LIBTOOL) --mode=compile --tag=RC $(RC) -i $^ -o $@

revisiont.h:
	@if test '$(REVISION)' != '$(CONF_VERSION)'; then\
		echo "Writing new Makefile.config file";\
		echo "static char *revision = \"$(REVISION)""\"; "> revision.h;\
		echo "CONF_VERSION=$(REVISION)" > Makefile.config;\
	fi;

INCLUDES= @EXTRA_CFLAGS@ -Wall\
	-DPIXMAP_PATH=\""$(pkgdatadir)"\" \
	-DGLADE_PATH=\""$(pkgdatadir)"\" \
	-DVERSION=\"@VERSION@\"\
	-DGMPC_BUILD=1\
	-I$(top_srcdir)\
	-I$(top_builddir)\
	-I$(top_srcdir)/src\
	-I$(top_builddir)/src\
	-I$(top_srcdir)/src/egg\
	-I$(top_builddir)/src/egg\
	-I$(top_srcdir)/src/gob\
	-I$(top_builddir)/src/gob\
	-I$(top_srcdir)/src/bacon\
	-I$(top_builddir)/src/bacon

LIBS= \
	@glib_LIBS@ @LIBS@ \
	@libmpd_LIBS@ \
	@gobject_LIBS@\
	@gtk_LIBS@\
	@gmodule_LIBS@\
	@libglade_LIBS@\
	@gthread_LIBS@\
	@sm_LIBS@


AM_LDFLAGS = \
	@EXTRA_LDFLAGS@\
	@LDFLAGS@

AM_CFLAGS = @CFLAGS@\
	@glib_CFLAGS@\
	@libmpd_CFLAGS@ \
	@gobject_CFLAGS@\
	@gtk_CFLAGS@\
	@gmodule_CFLAGS@\
	@libglade_CFLAGS@\
	@gthread_CFLAGS@\
	@sm_CFLAGS@\
	-DPACKAGE_LIB_DIR=\""$(pkglibdir)"\" 

gmpcinclude_HEADERS = \
		plugin.h	\
		config1.h	\
		misc.h\
		gmpc_easy_download.h\
		TreeSearchWidget.h \
		playlist3-messages.h\
		metadata.h



#gob/gmpc-profiles.h\
#gob/gmpc-mpddata-model.h\
#gob/gmpc-mpddata-model-sort.h\
#gob/gmpc-mpddata-treeview.h\
#gob/gmpc-meta-watcher.h\
#gob/gmpc-clicklabel.h\
#gob/gmpc-metaimage.h\
#gob/gmpc-connection.h\
#gob/gmpc-signals.h\
#gob/gmpc-idle.h


gmpcincludedir =$(includedir)/gmpc/

EXTRA_DIST=\
		playlist3.h\
		main.h\
		mm-keys.h\
		misc.h\
		mpdinteraction.h\
		config-defaults.h\
		browsers/playlist3-tag2-browser.h\
		browsers/playlist3-file-browser.h\
		browsers/playlist3-current-playlist-browser.h\
		browsers/playlist3-find2-browser.h\
		browsers/playlist3-playlist-editor.h\
		bacon/bacon-message-connection.h\
		sm.h\
		metadata.h\
		egg/eggcolumnchooserdialog.h\
		egg/eggcolumnmodel.h\
		revision.h\
		qlib/qasyncqueue.h\
		sexy-icon-entry.h\
		egg/eggcellrendererkeys.h\
		egg/eggaccelerators.h

DISTCLEANFILES=\
			   revision.h\
			   Makefile.config
