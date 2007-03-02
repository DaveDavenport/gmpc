REVISION=$(shell svn info | grep "Last Changed Rev" | awk -F': ' '{ print $$2}')
CONF_VERSION=0
-include Makefile.config

bin_PROGRAMS=gmpc
gmpc_SOURCES=\
	main.c\
	plugin.c\
	mpdinteraction.c\
	preferences.c\
	id3info.c\
	eggtrayicon.c\
	tray-icon.c\
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
	playlist-list.c\
	mm-keys.c\
	config1.c\
	bacon-volume.c\
	sm.c\
	metadata.c\
	eggcellrendererkeys.c\
	eggaccelerators.c\
	bacon-message-connection.c\
	gmpc-connection.c\
	gmpc-metaimage.c\
	gmpc-mpddata-model.c\
	gmpc-mpddata-treeview.c\
	gmpc_easy_download.c\
	url_fetcher.c\
	eggcolumnchooserdialog.c\
	eggcolumnmodel.c\
	TreeSearchWidget.c\
	qlib/qasyncqueue.c

BUILT_SOURCES= revisiont.h\
	gmpc-connection.c\
	gmpc-mpddata-model.c\
	gmpc-mpddata-treeview.c\
	gmpc-metaimage.c

gmpc_connection_SOURCE=gmpc-connection.gob
gmpc-connection.c: $(gmpc_connection_SOURCE) 
	$(GOB2) --no-touch-headers $(gmpc_connection_SOURCE) 

gmpc_metaimage_SOURCE=gmpc-metaimage.gob
gmpc-metaimage.c: $(gmpc_metaimage_SOURCE)
	$(GOB2) --no-touch-headers $(gmpc_metaimage_SOURCE) 

gmpc_mpddata_model_SOURCE=gmpc-mpddata-model.gob
gmpc-mpddata-model.c: $(gmpc_mpddata_model_SOURCE)
	$(GOB2) --no-touch-headers $(gmpc_mpddata_model_SOURCE) 

gmpc_mpddata_treeview_SOURCE=gmpc-mpddata-treeview.gob
gmpc-mpddata-treeview.c: $(gmpc_mpddata_treeview_SOURCE)
	$(GOB2) --no-touch-headers $(gmpc_mpddata_treeview_SOURCE) 

revisiont.h:
	if test '$(REVISION)' != '$(CONF_VERSION)'; then\
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
	@sm_CFLAGS@

gmpcinclude_HEADERS = \
		plugin.h	\
		config1.h	\
		gmpc_easy_download.h\
		TreeSearchWidget.h \
		metadata.h
gmpcincludedir =$(includedir)/gmpc/

EXTRA_DIST = 	playlist3.h\
	    	main.h\
	   	mm-keys.h\
		misc.h\
		eggtrayicon.h\
		mpdinteraction.h\
		config-defaults.h\
		browsers/playlist3-tag-browser.h\
		browsers/playlist3-file-browser.h\
		browsers/playlist3-current-playlist-browser.h\
		browsers/playlist3-artist-browser.h\
		browsers/playlist3-find2-browser.h\
		browsers/playlist3-find3-browser.h\
		playlist-list.h\
		bacon-volume.h\
		sm.h\
		metadata.h\
		eggcellrendererkeys.h\
		eggaccelerators.h\
		bacon-message-connection.h\
		gmpc-connection.h\
		gmpc-metaimage.h\
		gmpc-metaimage-private.h\
		gmpc-mpddata-model.h\
		gmpc-mpddata-model-private.h\
		gmpc-mpddata-treeview.h\
		gmpc-mpddata-treeview-private.h\
		id3info.h\
		eggcolumnchooserdialog.h\
		eggcolumnmodel.h\
		revision.h\
		tray-icon.h\
		qlib/qasyncqueue.h
