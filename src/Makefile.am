if EXTRA_VERSION
REVISION=@EXTRA_VERSION@
else
REVISION=`git rev-parse --short master`
endif
CONF_VERSION=0
-include Makefile.config

bin_PROGRAMS=gmpc

gmpc_VALAFLAGS=--pkg=gmodule-2.0 --pkg=config --pkg=gtk+-2.0 --pkg=gmpc --pkg=libmpd --pkg=libxml-2.0  --pkg=fix --vapidir=$(top_srcdir)/src/vapi/ --basedir=$(top_builddir)/src/ -H gmpc-extras.h --library=gmpc-extras --use-header

VALA_VAPI_FILES=\
    vapi/gmpc.vapi\
    vapi/libmpd.vapi\
    vapi/config.vapi


PLUGIN_FILES=\
    Plugins/extraplaylist.c\
    Plugins/AutoMPD.vala\
    Plugins/sidebar-search.vala\
    Plugins/sidebar-next-song.vala



gmpc_WIDGETS_FILES=\
    Widgets/GmpcVolume.vala\
    Widgets/gmpc-progress.vala\
    Widgets/mpd-async-request.c\
    Widgets/gmpc-song-list.vala\
    Widgets/gmpc-clicklabel.vala\
    Widgets/playlist3-messages.c\
    Widgets/gmpc-song-links.vala\
    Widgets/gmpc-image-async.vala\
    Widgets/advanced_settings.vala\
    Widgets/gmpc-widgets-qtable.vala\
    Widgets/gmpc-widgets-popupmenu.vala\
    Widgets/gmpc-metadata-backdrop.vala\
    Widgets/gmpc-metadata-text-label.vala\
    Widgets/gmpc-metadata-similarsongs.vala\
    Widgets/gmpc-metadata-similarartists.vala\
	Widgets/gmpc-sidebar-view.vala\
	Widgets/gmpc-widgets-overlay.vala\
    GUI/control_window.c\
    GUI/title_header.c\
    GUI/status_icon.c\
    GUI/cmd.c

#    GUI/thv.c

gmpc_TOOLS_FILES=\
    Tools/gmpc-database-update-tracker.vala\
    Tools/gmpc-metadata-prefetcher.vala\
    Tools/gmpc-liststore-sort.vala\
    Tools/gmpc-metadata-appearance.vala \
    Tools/gmpc_easy_download.c\
    Tools/mpd-easy-commands.c\
    Tools/advanced-search.c\
    Tools/setup-assistant.c\
    Tools/bug-information.c\
    Tools/mpdinteraction.c\
    Tools/pixbuf-cache.c\
    Tools/url-fetcher.c\
    Tools/plugin-man.c\
    Tools/mm-keys.c\
    Tools/misc.c\
    Tools/log.c\
    Tools/ipc.c

## 
# Browsers
##
gmpc_BROWSERS_FILES=\
	browsers/playlist3-current-playlist-browser.c\
    browsers/gmpc-metadata-browser2.vala\
	browsers/playlist3-playlist-editor.c\
	browsers/playlist3-find2-browser.c\
	browsers/playlist3-tag2-browser.c\
	browsers/playlist3-file-browser.c\
    browsers/gmpc-nowplaying2.vala\
	browsers/server-information.c

gmpc_SOURCES=\
    $(gmpc_WIDGETS_FILES)\
    $(gmpc_BROWSERS_FILES)\
    $(gmpc_TOOLS_FILES)\
	$(gmpc_egg_FILES)\
    $(PLUGIN_FILES)\
    vala/gmpc-connection.vala\
    vala/gmpc-easy-command.vala\
    vala/gmpc-favorites.vala\
    vala/gmpc-mpddata-treeview-tooltip.vala\
    vala/gmpc-plugin.vala\
    vala/gmpc-url-fetching-gui.vala\
    vala/gmpc-test-plugin.vala\
    vala/gmpc-paned-size-group.vala\
    vala/gmpc-sidebar-plugins.vala\
    MetaData/metadata.c\
    preferences.c\
    tray-icon2.c\
    playlist3.c\
    config1.c\
    options.c\
    plugin.c\
    main.c

#    MetaData/metadata-cache-sqlite.c\
#    MetaData/metadata-cache.c\
#


nodist_gmpc_SOURCES=\
    $(gmpc_GOBCFILES)

gmpc_LDADD=

if WIN32
gmpc_DEPENDENCIES = gmpc-win32.o
gmpc_LDADD += gmpc-win32.o
endif

BUILT_SOURCES=\
              $(gmpc_GOBSTAMPFILES)\
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
	-I$(top_srcdir)/src/Tools\
	-I$(top_builddir)/src/Tools\
	-I$(top_srcdir)/src/vala\
	-I$(top_builddir)/src/vala\
	-I$(top_srcdir)/src/Widgets\
	-I$(top_builddir)/src/Widgets\
	-I$(top_srcdir)/src/GUI\
	-I$(top_builddir)/src/GUI\
	-I$(top_srcdir)/src/MetaData/\
	-I$(top_builddir)/src/MetaData/



LIBS= \
	@glib_LIBS@ @LIBS@ \
	@libmpd_LIBS@ \
	@gobject_LIBS@\
	@gtk_LIBS@\
	@gmodule_LIBS@\
	@gthread_LIBS@\
	@libsoup_LIBS@\
	@libgio_LIBS@\
	@SPIFF_LIBS@\
	@libxspf_LIBS@\
	@libxml2_LIBS@\
	@libx11_LIBS@\
    @libglyr_LIBS@\
	@sqlite3_LIBS@\
	-lz


if HAVE_UNIQUE
    LIBS += @unique_LIBS@
endif

if HAVE_APP_INDICATOR
    LIBS += @APP_INDICATOR_LIBS@
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
	@gthread_CFLAGS@\
	@libsoup_CFLAGS@\
	@libgio_CFLAGS@\
    @sqlite3_CFLAGS@\
    @libxml2_CFLAGS@\
    @libx11_CFLAGS@\
    @EXTRA_CFLAGS@\
    @libglyr_CFLAGS@\
	-DPACKAGE_LIB_DIR=\""$(pkglibdir)"\"\
	-DPACKAGE_DATA_DIR=\""$(datarootdir)"\"


if HAVE_UNIQUE
    AM_CFLAGS += @unique_CFLAGS@ 
endif

if HAVE_APP_INDICATOR
    AM_CFLAGS += @APP_INDICATOR_CFLAGS@
endif

if MAINTAINER_MODE
AM_CFLAGS+=-Wall -Wdeclaration-after-statement -Wshadow -fno-common -Wextra -Wno-unused-parameter -Wno-missing-field-initializers -Wold-style-definition -Wmissing-declarations -Wredundant-decls -Wmissing-noreturn -Wpointer-arith -Wcast-align -Wwrite-strings -Winline -Wnested-externs -Wmissing-format-attribute -Waggregate-return -Wundef -Wmissing-include-dirs -Winit-self -Wswitch-default -Wswitch-enum -g -O1
endif


nodist_gmpcinclude_HEADERS=\
        gmpc-profiles.h\
        gmpc-meta-watcher.h\
        gmpc-mpddata-model.h\
        gmpc-mpddata-model-sort.h\
        gmpc-mpddata-treeview.h\
        gmpc-metaimage.h\
        gmpc-meta-text-view.h

gmpcinclude_HEADERS = \
		plugin.h	\
		config1.h	\
        Widgets/mpd-async-request.h\
        Widgets/playlist3-messages.h\
        Tools/gmpc_easy_download.h\
        Tools/mpdinteraction.h\
        Tools/misc.h\
        gmpc-extras.h\
        gmpc-version.h\
        GUI/status_icon.h\
		MetaData/metadata.h

gmpcincludedir =$(includedir)/gmpc/

EXTRA_DIST=\
		playlist3.h\
        tray-icon2.h\
		main.h\
        Widgets/mpd-async-request.h\
		config-defaults.h\
		browsers/playlist3-tag2-browser.h\
		browsers/playlist3-file-browser.h\
		browsers/playlist3-current-playlist-browser.h\
		browsers/playlist3-find2-browser.h\
		browsers/playlist3-playlist-editor.h\
        gtktransition.h\
        plugin-internal.h\
        Tools/mpd-easy-commands.h\
        Tools/setup-assistant.h\
		Tools/bug-information.h\
        Tools/plugin-man.h\
        Tools/log.h\
		Tools/mm-keys.h\
		Tools/misc.h\
		MetaData/metadata.h\
		egg/eggcolumnchooserdialog.h\
		egg/eggcolumnmodel.h\
		Tools/advanced-search.h\
		Tools/setup-assistant.h\
		Tools/ipc.h\
		revision.h\
		gmpc-version.h.in\
		Tools/bug-information.h\
		options.h\
		gmpc-win32.rc\
        Tools/pixbuf-cache.h\
        preferences.h\
        internal-plugins.h\
        GUI/control_window.h\
        GUI/title_header.h\
        GUI/cmd.h\
        $(VALA_VAPI_FILES)\
        $(gmpc_GOBFILES)

#        GUI/thv.h
#		MetaData/metadata-cache.h\
#		MetaData/metadata-cache-sqlite.h\
#

DISTCLEANFILES=\
			   revision.h\
			   Makefile.config

CLEANFILES=\
	$(gmpc_GOBCFILES)\
	$(gmpc_GOBHFILES)\
	$(gmpc_GOBSTAMPFILES)

## 
# GOB stuff
##
gmpc_GOBFILES = \
		gmpc-profiles.gob\
		gmpc-meta-watcher.gob\
		gmpc-metaimage.gob\
		gmpc-meta-text-view.gob\
		gmpc-mpddata-model.gob\
		gmpc-mpddata-model-playlist.gob\
		gmpc-mpddata-model-sort.gob\
		gmpc-mpddata-treeview.gob\
		gmpc-stats-label.gob

gmpc_GOBSTAMPFILES=\
	$(gmpc_GOBFILES:.gob=.gob.stamp)

gmpc_GOBCFILES=\
	$(gmpc_GOBFILES:.gob=.c)

gmpc_GOBHFILES=\
	$(gmpc_GOBFILES:.gob=.h)\
	$(gmpc_GOBFILES:.gob=-private.h)

%.gob.stamp: %.gob
	$(QUIET_GE) echo "  GOB   $< - $(top_builddir)/src/gob/"
	@GOB2@ --always-private-header $<
	@touch $(top_builddir)/src/$@


##
# Embedded egg
##
gmpc_egg_FILES=\
	egg/eggcolumnchooserdialog.c\
	egg/eggcolumnmodel.c
