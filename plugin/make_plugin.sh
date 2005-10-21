gcc plugin.c -o plugin.so -shared -ldl `pkg-config --libs --cflags gmodule-2.0 libxml-2.0 gtk+-2.0 glib-2.0 mozilla-gtkmozembed`

cp plugin.so ~/.gmpc/plugins/


