SOURCES := $(wildcard *.page)
OBJECTS := $(patsubst %.page,%.xhtml,$(SOURCES))

all:	$(OBJECTS)

%.xhtml:	%.page
	bash gnome-doc-tool xhtml --css-file="style.css"  $<
    
clean:
	rm -rf *.xhtml index.cache
