bin_PROGRAMS=gmpc

gmpc_SOURCES=\
	main.c\
	mpdinteraction.c\
	callback.c\
	player.c\
	preferences.c\
	id3info.c\
	config.c\
	libmpdclient.c\
	eggtrayicon.c\
	tray-icon.c\
	misc.c\
	strfsong.c\
	playlist2.c\
	song-browser.c\
	open-location.c\
	list_tooltip.c\
	mm-keys.c\
	playlist3.c\
	vfs_download.c\
	config1.c

INCLUDES= @gmpc_CFLAGS@ -DPIXMAP_PATH=\""$(datadir)/gmpc/"\" -Wall -DGLADE_PATH=\""$(datadir)/gmpc/"\"
LIBS = @gmpc_LIBS@ @LIBS@ 


EXTRA_DIST = 	playlist3.h\
		playlist2.h\
	    	main.h\
	   	mm-keys.h\
		song-browser.h\
		open-location.h\ 
		strfsong.h\
		misc.h\
		eggtrayicon.h\
		libmpdclient.h\
		vfs_download.h\
		mpdinteraction.h\
		config1.h
		
