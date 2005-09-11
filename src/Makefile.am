bin_PROGRAMS=gmpc

gmpc_SOURCES=\
	main.c\
	mpdinteraction.c\
	player.c\
	preferences.c\
	id3info.c\
	eggtrayicon.c\
	tray-icon.c\
	misc.c\
	strfsong.c\
	open-location.c\
	mm-keys.c\
	playlist3.c\
	vfs_download.c\
	osb_browser.c\
	edit_markup.c\
	tag-browser.c\
	playlist3-custom-stream-browser.c\
	playlist3-file-browser.c\
	playlist3-current-playlist-browser.c\
	playlist3-artist-browser.c\
	playlist3-find-browser.c\
	config1.c

#INCLUDES= @gmpc_CFLAGS@ -DPIXMAP_PATH=\""$(datadir)/gmpc/"\" -Wall -DGLADE_PATH=\""$(datadir)/gmpc/"\" -DDEBUG  
INCLUDES= @gmpc_CFLAGS@ $(gmpc_gvfs_CFLAGS) $(gmpc_libnotify_CFLAGS) -Wall   
LIBS = @gmpc_LIBS@ @LIBS@ $(gmpc_gvfs_LIBS) $(gmpc_libnotify_LIBS)


EXTRA_DIST = 	playlist3.h\
	    	main.h\
	   	mm-keys.h\
		open-location.h\
		strfsong.h\
		misc.h\
		eggtrayicon.h\
		vfs_download.h\
		mpdinteraction.h\
		osb_browser.h\
		config-defaults.h\
		tag-browser.h\
		playlist3-custom-stream-browser.h\
		playlist3-file-browser.h\
		playlist3-current-playlist-browser.h\
		playlist3-artist-browser.h\
		playlist3-find-browser.h\
		config1.h
		
