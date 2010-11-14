#include <glib.h>
#include <gtk/gtk.h>
#include "Widgets/pixbuf-cache.h"
#include "gmpc-extras.h"

#define CYCLES 2

gpointer config = NULL;

void playlist3_show_error_message(const char *message, int el)
{

}
/**
 * Dummy config system 
 */
int cfg_get_single_value_as_int_with_default(gpointer *config, const gchar *a, const gchar *b, int def)
{
    return def;
}
gchar * cfg_get_single_value_as_string_with_default(gpointer *config, const gchar *a, const gchar *b, gchar* def)
{
    return g_strdup(def);
}
gchar *cfg_get_single_value_as_string(gpointer *config, const gchar *a, const gchar *b)
{
    return NULL;
}
void cfg_set_single_value_as_string(gpointer *config, const gchar *a, const gchar *b, gchar *def)
{
}

void cfg_set_single_value_as_int(gpointer *config, const gchar *a, const gchar *b, int def )
{
}
int main (int argc, char **argv)
{
    guint i = CYCLES;
    GList *node,*list = NULL; 
    GMainLoop *l; 
    g_type_init_with_debug_flags(G_TYPE_DEBUG_MASK);

    l = g_main_loop_new(NULL, TRUE); 
    pixbuf_cache_create();
    for(;i>0;i--){
        GtkWidget *pb = gmpc_meta_image_async_new();
        gmpc_meta_image_async_set_from_file(pb,"test.png", i ,FALSE);
        if(pb) {
            list = g_list_prepend(list, g_object_ref_sink(pb));
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
