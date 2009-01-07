REVISION=$(shell git rev-parse --short master)
CONF_VERSION=0
-include Makefile.config

SUBDIRS=gob

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
	sm.c\
	metadata.c\
	browsers/playlist3-tag2-browser.c\
	browsers/playlist3-file-browser.c\
	browsers/playlist3-current-playlist-browser.c\
	browsers/playlist3-find2-browser.c\
	browsers/playlist3-metadata-browser.c\
	browsers/playlist3-playlist-editor.c\
	vala/gmpc_image.c\
	vala/gmpc-progress.c\
	gmpc_easy_download.c\
	setup-assistant.c\
	url_fetcher.c\
	egg/eggcolumnchooserdialog.c\
	egg/eggcolumnmodel.c\
	egg/eggcellrendererkeys.c\
	egg/eggaccelerators.c\
	TreeSearchWidget.c\
	qlib/qasyncqueue.c\
	edit_markup.c 

if EGGTRAYICON
gmpc_SOURCES+=egg/eggtrayicon.c
endif

if !USE_SYSTEM_LIBSEXY
gmpc_SOURCES+=sexy-icon-entry.c
endif

gmpc_LDADD=\
			   gob/libcwidgets.la


if WIN32 
gmpc_LDADD += gmpc-win32.o
endif

BUILT_SOURCES=\
		revisiont.h\
		revision.h

%.c %.h %-private.h: %.gob
	@GOB2@ -o gob/ --no-touch-headers --always-private-header $<
	@touch $@

%.o: %.rc
	$(LIBTOOL) --mode=compile --tag=RC $(RC) -i $^ -o $@

revisiont.h:
	@if test '$(REVISION)' != '$(CONF_VERSION)'; then\
		echo "Writing new Makefile.config file";\
		echo "CONF_VERSION=$(REVISION)" > Makefile.config;\
	fi;

revision.h:
	echo "static const char *revision = \"$(REVISION)""\"; " > revision.h

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
	-I$(top_builddir)/src/gob



LIBS= \
	@glib_LIBS@ @LIBS@ \
	@libmpd_LIBS@ \
	@gobject_LIBS@\
	@gtk_LIBS@\
	@gmodule_LIBS@\
	@libglade_LIBS@\
	@gthread_LIBS@\
	@sm_LIBS@ 

if USE_UNIQUE
LIBS += @unique_LIBS@
endif

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
	@libglade_CFLAGS@\
	@gthread_CFLAGS@\
	@sm_CFLAGS@\
	@EXTRA_CFLAGS@\
	-DPACKAGE_LIB_DIR=\""$(pkglibdir)"\"\ 
	-DPACKAGE_DATA_DIR=\""$(datarootdir)"\"\ 
	-DPACKAGE_DATA_PREFIX=\""$(prefix)"\" 

if USE_UNIQUE
AM_CFLAGS += @unique_CFLAGS@
endif

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
		TreeSearchWidget.h \
		playlist3-messages.h\
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
		vala/gmpc_image.h\
		vala/gmpc-progress.h\
		setup-assistant.h\
		sm.h\
		metadata.h\
		egg/eggcolumnchooserdialog.h\
		egg/eggcolumnmodel.h\
		egg/eggaccelerators.h\
		revision.h\
		qlib/qasyncqueue.h\
		sexy-icon-entry.h\
		egg/eggcellrendererkeys.h\
		egg/eggaccelerators.h\
		gmpc_unique.h\
		gmpc-win32.rc

if USE_SYSTEM_LIBSEXY
EXTRA_DIST+=sexy-icon-entry.c
endif

if EGGTRAYICON
EXTRA_DIST+=egg/eggtrayicon.h
else 
EXTRA_DIST+=egg/eggtrayicon.h egg/eggtrayicon.c
endif



DISTCLEANFILES=\
			   revision.h\
			   Makefile.config
