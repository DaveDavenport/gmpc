#include <glib.h>
#include <assert.h>
#include <gtk/gtk.h>


void colorshift_pixbuf(GdkPixbuf *dest, GdkPixbuf *src, int shift);
void decolor_pixbuf(GdkPixbuf *dest, GdkPixbuf *src);
void darken_pixbuf(GdkPixbuf * dest, guint factor);

/* Dummy */
gpointer config = NULL;

void show_error_message(void)
{
}

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


int main (int argc, char **argv)
{
    GList *node,*list = NULL; 
    int i;
    GMainLoop *l; 
    g_type_init_with_debug_flags(G_TYPE_DEBUG_MASK);

    GdkPixbuf *pb = gdk_pixbuf_new_from_file("test.jpg", NULL);
    assert(pb != NULL);
    GTimer *t = g_timer_new();
    for(i =0; i<1;i++)
        darken_pixbuf(pb, 1);    
    printf("elapsed: %f\n", g_timer_elapsed(t, NULL));
    g_timer_destroy(t);

    gdk_pixbuf_save(pb, "out.png", "png", NULL,NULL);

    g_object_unref(pb);
}
