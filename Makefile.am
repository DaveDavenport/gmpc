AUTOMAKE_OPTIONS = foreign 1.6

if WIN32
SUBDIRS = src pixmaps glade data po pixmaps doc
else
SUBDIRS = src pixmaps glade data po pixmaps doc remote
endif

if INTLTOOL_HIGHER_04000
else
EXTRA_DIST=	intltool-extract.in\
		intltool-update.in\
		intltool-merge.in


DISTCLEANFILES=\
	       intltool-update\
	       intltool-extract\
	       intltool-merge
endif
