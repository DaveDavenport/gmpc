gcc plugin.c -o plugin_osb.so -shared -ldl `pkg-config --libs --cflags gmodule-2.0 libxml-2.0 gtk+-2.0 glib-2.0 libglade-2.0`

cp plugin_osb.so ~/.gmpc/plugins/


