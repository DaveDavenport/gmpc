bin_PROGRAMS=gmpc

gmpc_SOURCES=\
	cover-art.c\
	main.c\
	plugin.c\
	mpdinteraction.c\
	player.c\
	preferences.c\
	id3info.c\
	eggtrayicon.c\
	tray-icon.c\
	misc.c\
	playlist3.c\
	edit_markup.c\
	playlist3-tag-browser.c\
	playlist3-file-browser.c\
	playlist3-current-playlist-browser.c\
	playlist3-artist-browser.c\
	playlist3-find-browser.c\
	playlist-list.c\
	mm-keys.c\
	qthread.c\
	config1.c\
	cover-art-manager.c\
	TreeSearchWidget.c

INCLUDES= @EXTRA_CFLAGS@ -Wall\
	-DPIXMAP_PATH=\""$(pkgdatadir)"\" \
	-DGLADE_PATH=\""$(pkgdatadir)"\" \
	-DVERSION=\"@VERSION@\"

LIBS = \
	@glib_LIBS@ @LIBS@ \
	@libmpd_LIBS@ \
	@gobject_LIBS@\
	@libxml_LIBS@ \
	@gtk_LIBS@\
	@gmodule_LIBS@\
	@libglade_LIBS@\
	@gthread_LIBS@

LDFLAGS = \
	@EXTRA_LDFLAGS@

CFLAGS = @CFLAGS@\
	@glib_CFLAGS@\
	@libmpd_CFLAGS@ \
	@gobject_CFLAGS@\
	@libxml_CFLAGS@ \
	@gtk_CFLAGS@\
	@gmodule_CFLAGS@\
	@libglade_CFLAGS@\
	@gthread_CFLAGS@

gmpcinclude_HEADERS = \
		plugin.h	\
		cover-art.h	\
		config1.h	\
		qthread.h
gmpcincludedir =$(includedir)/gmpc/

EXTRA_DIST = 	playlist3.h\
	    	main.h\
		player.h\
	   	mm-keys.h\
		misc.h\
		eggtrayicon.h\
		mpdinteraction.h\
		config-defaults.h\
		playlist3-tag-browser.h\
		playlist3-file-browser.h\
		playlist3-current-playlist-browser.h\
		playlist3-artist-browser.h\
		playlist3-find-browser.h\
		playlist-list.h\
		TreeSearchWidget.h
