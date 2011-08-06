man_MANS = gmpc.1 gmpc-remote.1 gmpc-remote-stream.1
EXTRA_DIST = $(man_MANS)

SUBDIRS=Mallard

.PHONY: doc
doc: Manual.txt
	asciidoc --backend xhtml11 -a toc -a iconsdir=icons/ -a icons  Manual.txt 
	asciidoc --backend xhtml11 -a toc -a iconsdir=icons/ -a icons  Manual_DE.txt 
	asciidoc --backend xhtml11 -a toc -a iconsdir=icons/ -a icons  Manual_NL.txt 
