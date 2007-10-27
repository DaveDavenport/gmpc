all:

doc: Manual.txt
# Can this be changed to a ! test? 
	if test -d Manual; then\
		true;\
		else\
		mkdir Manual;\
	fi;
	a2x --icons-dir=images/ -f chunked  Manual.txt -d Manual/
	echo "Coping images"
	cp -ruf images/ Manual/Manual.chunked/

doc-simple: Manual.txt
	asciidoc Manual.txt

clean:
	if test -d Manual;\
	then\
		rm -Rf Manual/;\
	fi;
	if test -f Manual.html; \
	then\
		rm Manual.html;\
	fi;
	if test -f Manual.xml; \
	then\
		rm Manual.xml;\
	fi;

plugin-howto: PluginHowto.txt
		asciidoc $<
