Q=@
REVISION=$(shell svn info | grep "Last Changed Rev" | awk -F': ' '{ print $$2}')
CONF_VERSION=0
-include Makefile.config

bin_PROGRAMS=gmpc
sources=\
	main.c\
	plugin.c\
	mpdinteraction.c\
	preferences.c\
	tray-icon.c\
	tray-icon2.c\
	misc.c\
	playlist3.c\
	edit_markup.c\
	browsers/playlist3-tag-browser.c\
	browsers/playlist3-file-browser.c\
	browsers/playlist3-current-playlist-browser.c\
	browsers/playlist3-artist-browser.c\
	browsers/playlist3-find2-browser.c\
	browsers/playlist3-find3-browser.c\
	browsers/playlist3-metadata-browser.c\
	browsers/playlist3-information.c\
	browsers/playlist3-playlist-editor.c\
	playlist-list.c\
	mm-keys.c\
	config1.c\
	bacon/bacon-volume.c\
	sm.c\
	metadata.c\
	bacon/bacon-message-connection.c\
	gob/gmpc-connection.c\
	gob/gmpc-metaimage.c\
	gob/gmpc-meta-watcher.c\
	gob/gmpc-meta-text-view.c\
	gob/gmpc-mpddata-model.c\
	gob/gmpc-mpddata-model-sort.c\
	gob/gmpc-mpddata-treeview.c\
	gob/gmpc-clicklabel.c\
	gmpc_easy_download.c\
	gob/gmpc-profiles.c\
	url_fetcher.c\
	egg/eggcolumnchooserdialog.c\
	egg/eggcolumnmodel.c\
	egg/eggtrayicon.c\
	egg/eggcellrendererkeys.c\
	egg/eggaccelerators.c\
	TreeSearchWidget.c\
	qlib/qasyncqueue.c

if HAVE_RC
gmpc_SOURCES=$(sources) gmpc.rc
else
gmpc_SOURCES=$(sources)
endif

BUILT_SOURCES= revisiont.h\
	gob/gmpc-connection.c\
	gob/gmpc-profiles.c\
	gob/gmpc-mpddata-model.c\
	gob/gmpc-mpddata-model-sort.c\
	gob/gmpc-mpddata-treeview.c\
	gob/gmpc-clicklabel.c\
	gob/gmpc-meta-text-view.c\
	gob/gmpc-meta-watcher.c\
	gob/gmpc-metaimage.c

%.c %.h %-private.h: %.gob
	@GOB2@ -o gob/ --no-touch-headers $<
	@touch $@

%.o: %.rc
	$(LIBTOOL) --mode=compile --tag=RC $(RC) -i $^ -o $@

.rc.o:
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
	-DVERSION=\"@VERSION@\"

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
	-Igob/ \
	-Iegg/

gmpcinclude_HEADERS = \
		plugin.h	\
		config1.h	\
		gmpc_easy_download.h\
		gob/gmpc-profiles.h\
		gob/gmpc-mpddata-model.h\
		gob/gmpc-mpddata-model-sort.h\
		gob/gmpc-mpddata-treeview.h\
		gob/gmpc-meta-watcher.h\
		gob/gmpc-clicklabel.h\
		gob/gmpc-metaimage.h\
		TreeSearchWidget.h \
		metadata.h
gmpcincludedir =$(includedir)/gmpc/

EXTRA_DIST = 	playlist3.h\
	    	main.h\
	   	mm-keys.h\
		misc.h\
		mpdinteraction.h\
		config-defaults.h\
		browsers/playlist3-tag-browser.h\
		browsers/playlist3-file-browser.h\
		browsers/playlist3-current-playlist-browser.h\
		browsers/playlist3-artist-browser.h\
		browsers/playlist3-find2-browser.h\
		browsers/playlist3-find3-browser.h\
		browsers/playlist3-playlist-editor.h\
		playlist-list.h\
		bacon/bacon-volume.h\
		bacon/bacon-message-connection.h\
		sm.h\
		metadata.h\
		egg/eggcellrendererkeys.h\
		egg/eggaccelerators.h\
		gob/gmpc-profiles.h\
		gob/gmpc-profiles-private.h\
		gob/gmpc-connection.h\
		gob/gmpc-meta-watcher.h\
		gob/gmpc-meta-watcher-private.h\
		gob/gmpc-metaimage.h\
		gob/gmpc-metaimage-private.h\
		gob/gmpc-meta-text-view.h\
		gob/gmpc-meta-text-view-private.g\
		gob/gmpc-mpddata-model.h\
		gob/gmpc-mpddata-model-sort.h\
		gob/gmpc-mpddata-model-sort-private.c\
		gob/gmpc-mpddata-model-private.h\
		gob/gmpc-mpddata-treeview.h\
		gob/gmpc-mpddata-treeview-private.h\
		gob/gmpc-clicklabel.h\
		gob/gmpc-clicklabel-private.h\
		egg/eggcolumnchooserdialog.h\
		egg/eggcolumnmodel.h\
		egg/eggtrayicon.h\
		revision.h\
		tray-icon.h\
		qlib/qasyncqueue.h

#%.o: %.c Makefile.am
#	$(info ==> Building: $@)
#	$(Q)$(CC) $(AM_CFLAGS) $(INCLUDES) -I. -I../ -c $< -o $@
