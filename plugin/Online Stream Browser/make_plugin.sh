gcc plugin.c vfs_download.c open-location.c -o plugin_osb.so -shared -ldl `pkg-config --libs --cflags gmodule-2.0 libxml-2.0 gtk+-2.0 glib-2.0 libglade-2.0 gnome-vfs-2.0 gobject-2.0 "libmpd >= 0.0.9.6"`

cp plugin_osb.so ~/.gmpc/plugins/


