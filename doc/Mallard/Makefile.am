include ../../gnome-doc-utils.make
DOC_ID = @PACKAGE@
DOC_INCLUDES = legal.xml
DOC_LINGUAS=

DOC_PAGES = \
	AboutThisDocument.page\
	BrowserArtistBrowser.page\
	BrowserDatabase.page\
	BrowserGenreBrowser.page\
	BrowserMetadataBrowser.page\
	BrowserNowPlaying.page\
	BrowserPlaylistEditor.page\
	BrowserPlayQueue.page\
	BrowserSearch.page\
	Browsers.page\
	BuildInstallInstructions.page\
	EasyCommand.page\
	index.page\
	MainInterface.page\
	PreferencesConnection.page\
	PreferencesInterface.page\
	PreferencesInternalExtraPlaylistView.page\
	PreferencesMetadataAppearance.page\
	PreferencesMetadataHandler.page\
	PreferencesMultimediaKeys.page\
	PreferencesNotification.page\
	Preferences.page\
	PreferencesPlugins.page\
	PreferencesProxy.page\
	PreferencesServerSettings.page\
	PreferencesTagBasedBrowser.page\
	QuickStartGuide.page\
	ProblemSolving.page\
	TipsAndTricks.page\
	AutoMPD.page

DOC_FIGURES=\
Images/browser-pane-double.png\
Images/browser-pane-single.png\
Images/database-columns.png\
Images/database.png\
Images/gmpc-dev-install-guidestep_10.png\
Images/gmpc-dev-install-guidestep_11.png\
Images/gmpc-dev-install-guidestep_12.png\
Images/gmpc-dev-install-guidestep_13.png\
Images/gmpc-dev-install-guidestep_14.png\
Images/gmpc-dev-install-guidestep_15.png\
Images/gmpc-dev-install-guidestep_16.png\
Images/gmpc-dev-install-guidestep_9.png\
Images/Gmpc-easy-command.png\
Images/main-view-collapsed2.png\
Images/main-window-collapsed1.png\
Images/main-window.png\
Images/menu-bar.png\
Images/message-bar.png\
Images/meta-data-selector-artist.png\
Images/meta-data-selector-lyrics.png\
Images/nowplaying.png\
Images/pc-cover-plain.png\
Images/pc-cover.png\
Images/pc-favorite-hl.png\
Images/pc-favorite-nhl.png\
Images/pc-playback-control.png\
Images/pc-seek-bar.png\
Images/pc-song-info.png\
Images/playback-control.png\
Images/play-queue.png\
Images/play-queue-rightclick.png\
Images/preferences-connection.png\
Images/preferences-interface.png\
Images/preferences-metadata-browser.png\
Images/preferences-metadata-handler.png\
Images/preferences-multimedia-keys.png\
Images/preferences-notification.png\
Images/preferences-proxy.png\
Images/preferences-server-settings.png\
Images/preferences-tag-based-browser.png\
Images/server-information.png\
Images/sidebar-hori1.png\
Images/sidebar-hori2.png\
Images/sidebar.png\
Images/status-bar.png\
Images/status-icons.png\
Images/easy-command.png\
Images/browser-artist-browser.png\
Images/browser-database.png\
Images/browser-genre-browser.png\
Images/browser-metadata-browser.png\
Images/browser-nowplaying.png\
Images/browser-playlist-editor.png\
Images/browser-playqueue.png\
Images/browser-search.png\
Images/preferences-internal-extra-playlist-view.png\
Images/preferences-metadata-appearance.png\
Images/gmpc-preferences-autompd.png\
Images/preferences-plugins.png


dist-hook:doc-dist-hook


indent-help-pages:
	@for a in $(DOC_PAGES); do\
		xmllint -format C/"$$a" > C/"$$a.temp";\
		mv C/"$$a.temp" C/"$$a";\
	done

create-xhtml-pages:
	@mkdir html;\
	for a in $(DOC_PAGES);\
	do\
		bash gnome-doc-tool xhtml --copy-graphics --css-file="style.css" "C/$$a" -o html/;\
	done
