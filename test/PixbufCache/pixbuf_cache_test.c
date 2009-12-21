#include <glib.h>
#include <gtk/gtk.h>
#include "Widgets/pixbuf-cache.h"

#define CYCLES 128

int main (int argc, char **argv)
{
    guint i = CYCLES;
    GList *node,*list = NULL; 
    GMainLoop *l; 
    g_type_init_with_debug_flags(G_TYPE_DEBUG_MASK);

    l = g_main_loop_new(NULL, TRUE); 
    pixbuf_cache_create();
    for(;i>0;i--){
        GdkPixbuf *pb = gdk_pixbuf_new_from_file("test.png", NULL);
        if(pb) {
            pixbuf_cache_add_icon(i, "test.png", pb);
            list = g_list_prepend(list, pb);
        }
    }
    printf("Filling done\n");
    for(node = g_list_first(list); node; node = g_list_next(node))
    {
        g_object_unref(node->data);
    }
    g_list_free(list);
    g_timeout_add_seconds(30, g_main_loop_quit,l);
    printf("quitting in 30 seconds\n");
    g_main_loop_run(l);

    pixbuf_cache_destroy();
}
