gcc plugin.c ../../src/strfsong.c ../../src/misc.c -o plugin_osd.so -shared -ldl `pkg-config --libs --cflags gmodule-2.0 libxml-2.0 gtk+-2.0 glib-2.0 "libmpd >= 0.0.9.6"` `xosd-config --libs --cflags`

cp plugin_osd.so ~/.gmpc/plugins/


