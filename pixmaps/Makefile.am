appicondir = $(datadir)/gmpc/icons/
appicon_DATA =	\
	media-audiofile.png\
	media-stream.png\
	media-artist.png\
	media-playback-start.png\
	gmpc-no-cover.png\
	media-album.png\
	gmpc-add-url.png\
	mpdBrowser_case.png\
	gmpc-loading-cover.png

EXTRA_DIST = $(appicon_DATA)

SUBDIRS = icons


gtk_update_icon_cache = gtk-update-icon-cache -f -t $(datadir)/icons/hicolor
install-data-hook: update-icon-cache
uninstall-hook: update-icon-cache
update-icon-cache:
	@-if test -z "$(DESTDIR)"; then \
		echo "Updating Gtk icon cache."; \
		$(gtk_update_icon_cache); \
		else \
		echo "*** Icon cache not updated.  After (un)install, run this:"; \
		echo "***   $(gtk_update_icon_cache)"; \
		fi

