if EXTRA_VERSION
REVISION=@EXTRA_VERSION@
else
REVISION=`git rev-parse --short master`
endif
CONF_VERSION=0
-include Makefile.config

SUBDIRS=gob smclient

bin_PROGRAMS=gmpc

gmpc_SOURCES=\
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
	metadata.c\
    metadata-cache.c\
	browsers/playlist3-tag2-browser.c\
	browsers/playlist3-file-browser.c\
	browsers/playlist3-current-playlist-browser.c\
	browsers/playlist3-find2-browser.c\
	browsers/playlist3-playlist-editor.c\
	vala/gmpc-image.c\
	vala/gmpc-progress.c\
	vala/gmpc-rating.c\
	vala/gmpc_menu_item_rating.c\
	vala/gmpc-easy-command.c\
	vala/gmpc-song-links.c\
	vala/gmpc-plugin.c\
	vala/gmpc-test-plugin.c\
    vala/gmpc-mpddata-treeview-tooltip.c\
	vala/gmpc-metadata-browser2.c\
	vala/gmpc-favorites.c\
	vala/gmpc-paned-size-group.c\
    vala/gmpc-liststore-sort.c\
    vala/gmpc-connection.c\
    vala/gmpc-metadata-prefetcher.c\
	gmpc_easy_download.c\
	setup-assistant.c\
	url-fetcher.c\
	egg/eggcolumnchooserdialog.c\
	egg/eggcolumnmodel.c\
	bacon/bacon-message-connection.c\
	advanced-search.c\
	browsers/server-information.c\
	bug-information.c

gmpc_LDADD=\
			   gob/libcwidgets.la\
               smclient/libeggsmclient.la


if WIN32 
gmpc_LDADD += gmpc-win32.o
endif

BUILT_SOURCES=\
		revisiont.h

gmpc-win32.o: gmpc-win32.rc
	$(LIBTOOL) --mode=compile --tag=RC $(RC) -i $^ -o $@

.PHONY: revisiont.h
revisiont.h:
	@if test "$(REVISION)" != "$(CONF_VERSION)"; then\
		echo "Writing new Makefile.config file";\
		echo "CONF_VERSION=$(REVISION)" > Makefile.config;\
		echo "static const char *revision = \"$(REVISION)""\"; " > revision.h;\
	fi;

revision.h:	
	echo "static const char *revision = \"$(REVISION)""\"; " > revision.h;

INCLUDES=\
	-DPIXMAP_PATH=\""$(pkgdatadir)/icons/"\" \
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
	-I$(top_srcdir)/src/vala\
	-I$(top_builddir)/src/vala



LIBS= \
	@glib_LIBS@ @LIBS@ \
	@libmpd_LIBS@ \
	@gobject_LIBS@\
	@gtk_LIBS@\
	@gmodule_LIBS@\
	@libglade_LIBS@\
	@gthread_LIBS@\
	@libsoup_LIBS@\
	@SPIFF_LIBS@\
	@libxspf_LIBS@\
    @sqlite3_LIBS@

if USE_SYSTEM_LIBSEXY
LIBS += @libsexy_LIBS@
endif

if OSX
LIBS+=@macint_LIBS@
endif



AM_LDFLAGS = \
	@EXTRA_LDFLAGS@\
	@LDFLAGS@



AM_CFLAGS = @CFLAGS@\
	@glib_CFLAGS@\
	@libmpd_CFLAGS@ \
	@gobject_CFLAGS@\
	@gtk_CFLAGS@\
	@gmodule_CFLAGS@\
    @libxspf_CFLAGS@\
	@libglade_CFLAGS@\
	@gthread_CFLAGS@\
	@libsoup_CFLAGS@\
    @sqlite3_CFLAGS@\
	@EXTRA_CFLAGS@\
	-DPACKAGE_LIB_DIR=\""$(pkglibdir)"\"\
	-DPACKAGE_DATA_DIR=\""$(datarootdir)"\"

if USE_SYSTEM_LIBSEXY
AM_CFLAGS += @libsexy_CFLAGS@
endif

if OSX
AM_CFLAGS+=@macint_CFLAGS@
INCLUDES+=-DOSX=1
endif

if MAINTAINER_MODE
AM_CFLAGS+=-Wall -Wdeclaration-after-statement -Wshadow -fno-common -Wextra -Wno-unused-parameter -Wno-missing-field-initializers -Wold-style-definition -Wmissing-declarations -Wredundant-decls -Wmissing-noreturn -Wpointer-arith -Wcast-align -Wwrite-strings -Winline -Wnested-externs -Wmissing-format-attribute -Waggregate-return -Wundef -Wmissing-include-dirs -Winit-self -Wswitch-default -Wswitch-enum
endif


gmpcinclude_HEADERS = \
		plugin.h	\
		config1.h	\
		misc.h\
		gmpc_easy_download.h\
		vala/gmpc-easy-command.h\
		vala/gmpc-plugin.h\
        vala/gmpc-connection.h\
        vala/gmpc-mpddata-treeview-tooltip.h\
        vala/gmpc-paned-size-group.h\
        playlist3-messages.h\
		gmpc-version.h\
		metadata.h

gmpcincludedir =$(includedir)/gmpc/

EXTRA_DIST=\
		playlist3.h\
		playlist3-keybindings.h\
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
		vala/gmpc-image.h\
		vala/gmpc-rating.h\
		vala/gmpc-progress.h\
		vala/gmpc_menu_item_rating.h\
		vala/gmpc-easy-command.h\
		vala/gmpc-song-links.h\
		vala/gmpc-test-plugin.h\
        vala/gmpc-metadata-browser2.h\
        vala/gmpc-favorites.h\
        vala/gmpc-liststore-sort.h\
        vala/gmpc-connection.h\
        vala/gmpc-mpddata-treeview-tooltip.h\
        vala/gmpc-metadata-prefetcher.h\
	vala/gmpc-paned-size-group.h\
        gtktransition.h\
        plugin-internal.h\
		setup-assistant.h\
		metadata.h\
        metadata-cache.h\
		egg/eggcolumnchooserdialog.h\
		egg/eggcolumnmodel.h\
		revision.h\
		bacon/bacon-message-connection.h\
		advanced-search.h\
		gmpc-version.h.in\
		bug-information.h\
		gmpc-win32.rc

DISTCLEANFILES=\
			   revision.h\
			   Makefile.config
