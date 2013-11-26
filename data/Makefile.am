rcfiledir= $(datadir)/gmpc/
dist_rcfile_DATA=\
	weblinks.list\
    gmpc.css\
	gmpc-gtk-win32.rc

desktopdir 	= $(datadir)/applications
desktop_in_files = gmpc.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = gmpc.pc


appdatadir= $(datadir)/appdata/
dist_appdata_DATA=\
    gmpc.appdata.xml

EXTRA_DIST = 		\
	gmpc.desktop.in 	\
	gmpc.pc.in


DISTCLEANFILES=\
 	gmpc.pc\
	gmpc.desktop
