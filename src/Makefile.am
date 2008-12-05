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
	sexy-icon-entry.c\
	edit_markup.c

gmpc_LDADD=\
			   gob/libcwidgets.la


if HAVE_RC
gmpc_SOURCES=$(sources) gmpc-win32.rc
else
gmpc_SOURCES=$(sources)
endif

BUILT_SOURCES=\
		revisiont.h\
		revision.h

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
		echo "CONF_VERSION=$(REVISION)" > Makefile.config;\
	fi;

revision.h:
		echo "static char *revision = \"$(REVISION)""\"; "> revision.h;

INCLUDES= @EXTRA_CFLAGS@ \
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

if MAINTAINER_MODE
MAYBE_CFLAGS=-Wall -Werror -Wdeclaration-after-statement -Wshadow 
endif

AM_CFLAGS = @CFLAGS@\
	@glib_CFLAGS@\
	@libmpd_CFLAGS@ \
	@gobject_CFLAGS@\
	@gtk_CFLAGS@\
	@gmodule_CFLAGS@\
	@libglade_CFLAGS@\
	@gthread_CFLAGS@\
	@sm_CFLAGS@\
	-DPACKAGE_LIB_DIR=\""$(pkglibdir)"\" \
	$(MAYBE_CFLAGS)



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
		bacon/bacon-message-connection.h\
		vala/gmpc_image.h\
		vala/gmpc-progress.h\
		setup-assistant.h\
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
