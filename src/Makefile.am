if EXTRA_VERSION
REVISION=@EXTRA_VERSION@
else
REVISION=`git rev-parse --short master`
endif
CONF_VERSION=0
-include Makefile.config

bin_PROGRAMS=gmpc

gmpc_VALAFLAGS=--pkg=gmodule-2.0 --pkg=config --pkg=gtk+-2.0 --pkg=gmpc --pkg=libmpd --vapidir=$(top_srcdir)/src/vapi/ --basedir=$(top_builddir)/src/ -H gmpc-extras.h --library=gmpc-extras --use-header

VALA_VAPI_FILES=\
    vapi/gmpc.vapi\
    vapi/libmpd.vapi\
    vapi/config.vapi


gmpc_SOURCES=\
    Widgets/pixbuf-cache.c\
    vala/gmpc-connection.vala\
    Widgets/gmpc-progress.vala\
    vala/gmpc-easy-command.vala\
    vala/gmpc-favorites.vala\
    Widgets/gmpc-image-async.vala\
    Widgets/mpd-async-request.c\
    vala/gmpc-liststore-sort.vala\
    Widgets/gmpc-menu-item-rating.vala\
    browsers/gmpc-metadata-browser2.vala\
    vala/gmpc-metadata-prefetcher.vala\
    vala/gmpc-mpddata-treeview-tooltip.vala\
    browsers/gmpc-nowplaying2.vala\
    vala/gmpc-plugin.vala\
    vala/gmpc-rating.vala\
    vala/gmpc-song-links.vala\
    vala/gmpc-url-fetching-gui.vala\
    vala/gmpc-database-update-tracker.vala\
    vala/gmpc-test-plugin.vala\
    vala/gmpc-paned-size-group.vala\
    Providers/music-tree.vala\
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
	MetaData/metadata.c\
	MetaData/metadata-cache.c\
	gmpc_easy_download.c\
	setup-assistant.c\
	url-fetcher.c\
	advanced-search.c\
	bug-information.c\
    $(gmpc_bacon_FILES)\
    $(gmpc_egg_FILES)\
    $(gmpc_browser_FILES)

nodist_gmpc_SOURCES=\
    $(gmpc_GOBCFILES)

gmpc_LDADD=\
           libeggsmclient.la


if WIN32 
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
	-I$(top_srcdir)/src/vala\
	-I$(top_builddir)/src/vala\
	-I$(top_srcdir)/src/MetaData/\
	-I$(top_builddir)/src/MetaData/



LIBS= \
	@glib_LIBS@ @LIBS@ \
	@libmpd_LIBS@ \
	@gobject_LIBS@\
	@gtk_LIBS@\
	@gmodule_LIBS@\
	@libglade_LIBS@\
	@gthread_LIBS@\
	@libsoup_LIBS@\
	@libgio_LIBS@\
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
	@libgio_CFLAGS@\
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
		misc.h\
		mpdinteraction.h\
        Widgets/mpd-async-request.h\
		gmpc_easy_download.h\
        gmpc-extras.h\
        playlist3-messages.h\
		gmpc-version.h\
		MetaData/metadata.h

gmpcincludedir =$(includedir)/gmpc/

EXTRA_DIST=\
		playlist3.h\
		playlist3-keybindings.h\
		main.h\
		mm-keys.h\
		misc.h\
        Widgets/mpd-async-request.h\
		config-defaults.h\
		browsers/playlist3-tag2-browser.h\
		browsers/playlist3-file-browser.h\
		browsers/playlist3-current-playlist-browser.h\
		browsers/playlist3-find2-browser.h\
		browsers/playlist3-playlist-editor.h\
        gtktransition.h\
        plugin-internal.h\
		setup-assistant.h\
		MetaData/metadata.h\
		MetaData/metadata-cache.h\
		egg/eggcolumnchooserdialog.h\
		egg/eggcolumnmodel.h\
		revision.h\
		bacon/bacon-message-connection.h\
		advanced-search.h\
		gmpc-version.h.in\
		bug-information.h\
        gmpc-win32.rc\
        Widgets/pixbuf-cache.h\
        $(VALA_VAPI_FILES)\
        $(gmpc_GOBFILES)

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
		gmpc-clicklabel.gob\
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
# Browsers
##

gmpc_browser_FILES=\
	browsers/playlist3-tag2-browser.c\
	browsers/playlist3-file-browser.c\
	browsers/playlist3-current-playlist-browser.c\
	browsers/playlist3-find2-browser.c\
	browsers/server-information.c\
	browsers/playlist3-playlist-editor.c

##
# Vala files
##
#gmpc_vala_FILES=\
#	vala/gmpc-image.c\
#	vala/gmpc-rating.c\
#	vala/gmpc_menu_item_rating.c\
#	vala/gmpc-easy-command.c\
#	vala/gmpc-song-links.c\
#	vala/gmpc-plugin.c\
#	vala/gmpc-test-plugin.c\
#    vala/gmpc-mpddata-treeview-tooltip.c\
#	vala/gmpc-metadata-browser2.c\
#    vala/gmpc-nowplaying2.c\
#    vala/gmpc-favorites.c\
#    vala/gmpc-liststore-sort.c\
#    vala/gmpc-connection.c\
#    vala/gmpc-url-fetching-gui.c\
#    vala/gmpc-database-update-tracker.c\
#    vala/gmpc-metadata-prefetcher.c

##
# Embedded egg
##
gmpc_egg_FILES=\
	egg/eggcolumnchooserdialog.c\
	egg/eggcolumnmodel.c

##
# Bacon
##
gmpc_bacon_FILES=\
	bacon/bacon-message-connection.c


###
# SM CLIENT STUFF
###

if PLATFORM_WIN32
platform_sources = smclient/eggsmclient-win32.c
platform_logout_test_ldflags = -mwindows
else
if PLATFORM_OSX
platform_defines = -xobjective-c
platform_ldflags = -framework Carbon
platform_session_end_ldflags = -framework Carbon
platform_logout_test_ldflags = -framework Carbon
platform_sources = smclient/eggsmclient-osx.c
else
platform_defines = -DEGG_SM_CLIENT_BACKEND_XSMP
platform_ltlibraries =                   \
	libeggdesktopfile.la             
platform_libs = libeggdesktopfile.la -lSM -lICE
platform_sources = smclient/eggsmclient-xsmp.c
endif
endif

libeggsmclient_la_INCLUDES = \
	-DG_LOG_DOMAIN=\""EggSMClient"\" \
	$(platform_defines)              \
	$(glib_CFLAGS)           \
    $(gtk_CFLAGS)

noinst_LTLIBRARIES =                     \
	libeggsmclient.la                \
	$(platform_ltlibraries)

libeggsmclient_la_LIBADD =               \
	$(glib_LIBS)             \
	$(gtk_LIBS)             \
	$(platform_libs)

libeggsmclient_la_LDFLAGS =              \
	$(platform_ldflags)

libeggsmclient_la_SOURCES =              \
	smclient/eggsmclient.c                    \
	$(platform_sources)

libeggdesktopfile_la_LIBADD =            \
	$(EGG_LIBS)

libeggdesktopfile_la_SOURCES =           \
	smclient/eggdesktopfile.c                 

EXTRA_DIST+=\
	smclient/eggsmclient.h                    \
	smclient/eggsmclient-private.h            \
	smclient/eggdesktopfile.h


egg_session_end_LDADD =                  \
	libeggsmclient.la

egg_session_end_LDFLAGS =                \
	$(platform_session_end_ldflags)

egg_launch_SOURCES =                     \
	smclient/egg-launch.c

egg_launch_LDADD =                       \
	libeggdesktopfile.la

EXTRA_DIST +=                             \
	smclient/eggsmclient-osx.c                \
	smclient/eggsmclient-win32.c              \
	smclient/eggsmclient-xsmp.c
