appicondir = $(datadir)/gmpc/icons/
dist_appicon_DATA =	\
	media-stream.png\
	media-artist.png\
	gmpc-no-cover.png\
	media-album.png\
	gmpc-loading-cover.png


SUBDIRS = icons


gtk_update_icon_cache = gtk-update-icon-cache -f -t $(DESTDIR)$(datadir)/icons/hicolor
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

EXTRA_DIST=gmpc.ico
