bin_PROGRAMS=gmpc

gmpc_SOURCES=\
	main.c\
	mpdinteraction.c\
	callback.c\
	player.c\
	preferences.c\
	playlist.c\
	id3info.c\
	config.c\
	libmpdclient.c\
	libmpdclient.h\
	eggtrayicon.h\
	eggtrayicon.c\
	tray-icon.c\
	popup.c\
	misc.c\
	misc.h\
	strfsong.c\
	strfsong.h\
	support.h\
	support.c\ 
	playlist2.c\
	playlist2.h\
	main.h

INCLUDES= @gmpc_CFLAGS@ -DPIXMAP_PATH=\""$(datadir)/gmpc/"\" -Wall -DGLADE_PATH=\""$(datadir)/gmpc/"\"
LIBS = @gmpc_LIBS@ @LIBS@ 
