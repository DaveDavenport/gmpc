bin_PROGRAMS=gmpc

gmpc_SOURCES=\
	main.c\
	plugin.c\
	mpdinteraction.c\
	player.c\
	preferences.c\
	id3info.c\
	eggtrayicon.c\
	tray-icon.c\
	misc.c\
	strfsong.c\
	playlist3.c\
	edit_markup.c\
	playlist3-tag-browser.c\
	playlist3-file-browser.c\
	playlist3-current-playlist-browser.c\
	playlist3-artist-browser.c\
	playlist3-find-browser.c\
	mm-keys.c\
	config1.c

#	playlist3-custom-stream-browser.c
#	playlist3-osb-browser.c

INCLUDES= @gmpc_CFLAGS@ @EXTRA_CFLAGS@   -Wall -Werror \
	-DPIXMAP_PATH=\""$(pkgdatadir)"\" \
	-DGLADE_PATH=\""$(pkgdatadir)"\" \
	-DVERSION=\"@VERSION@\"
LIBS = @gmpc_LIBS@ @LIBS@ 
LDFLAGS = @EXTRA_LDFLAGS@

EXTRA_DIST = 	playlist3.h\
	    	main.h\
		plugin.h\
	   	mm-keys.h\
		strfsong.h\
		misc.h\
		eggtrayicon.h\
		mpdinteraction.h\
		config-defaults.h\
		playlist3-tag-browser.h\
		playlist3-file-browser.h\
		playlist3-current-playlist-browser.h\
		playlist3-artist-browser.h\
		playlist3-find-browser.h\
		config1.h

#		playlist3-osb-browser.h
#	playlist3-custom-stream-browser.h
