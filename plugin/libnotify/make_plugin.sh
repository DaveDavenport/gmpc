gcc plugin.c -o plugin_libnotify.so -shared -ldl `pkg-config --libs --cflags gmodule-2.0 libxml-2.0 gtk+-2.0 glib-2.0 libnotify "libmpd >= 0.0.9.6" gmpc` -fPIC

cp plugin_libnotify.so ~/.gmpc/plugins/


