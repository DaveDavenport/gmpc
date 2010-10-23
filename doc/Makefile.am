man_MANS = gmpc.1 gmpc-remote.1
EXTRA_DIST = $(man_MANS)


.PHONY: doc
doc: Manual.txt
	asciidoc --backend xhtml11 -a toc -a iconsdir=Images/icons/ -a icons  Manual.txt 
