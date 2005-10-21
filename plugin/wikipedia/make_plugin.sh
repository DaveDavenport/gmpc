gcc plugin.c -o plugin_wp.so -shared -ldl `pkg-config --libs --cflags gmodule-2.0 libxml-2.0 gtk+-2.0 glib-2.0 firefox-gtkmozembed`

cp plugin_wp.so ~/.gmpc/plugins/


