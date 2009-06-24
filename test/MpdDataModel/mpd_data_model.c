#include <stdio.h>
#include <stdlib.h>
#include <glib.h>
#include <gtk/gtk.h>
#include "gmpc-mpddata-model.h"


/** 
 * Stuff to make linker happy 
 */
GObject *config = NULL;
GObject *gmw = NULL;
GObject *gmpc_signals = NULL;

const gchar *meta_data_get_uri(gpointer input)
{
    return NULL;
}

int cfg_get_single_value_as_int_with_default(GObject *config, const gchar *a, const gchar *b, int def)
{
    return def;
}

gchar * cfg_get_single_value_as_string_with_default(GObject *config, const gchar *a, const gchar *b, gchar* def)
{
    return g_strdup(def);
}
char * 	gmpc_signals_get_browser_markup	(GObject * self)
{
    return NULL;
}
void gmpc_meta_watcher_get_meta_path_callback()
{
}
void playlist3_show_error_message()
{
}
/**
 * Create test list 
 */
MpdData * get_test_list(void)
{
    MpdData *retv = NULL;
    int i = 100000;
    for(;i>0;i--)
    {
        retv = mpd_new_data_struct_append(retv);
        retv->type = MPD_DATA_TYPE_SONG;
        retv->song = mpd_newSong();
        retv->song->file = g_strdup_printf("%i", i);
        retv->song->title = g_strdup_printf("%i", i);
    }
    return mpd_data_get_first(retv);
}

int main(int argc, char **argv)
{
    GtkTreeIter iter;
    g_type_init();
    GmpcMpdDataModel *model = gmpc_mpddata_model_new();
    gmpc_mpddata_model_set_mpd_data(GMPC_MPDDATA_MODEL(model), get_test_list());
    if(gtk_tree_model_get_iter_first(GTK_TREE_MODEL(model), &iter))
    {
        do{
            gchar*a, *b, *c;
            GtkTreePath *path = gtk_tree_model_get_path(GTK_TREE_MODEL(model), &iter);
            gtk_tree_model_get(GTK_TREE_MODEL(model), &iter, 
                    MPDDATA_MODEL_COL_SONG_TITLE, &b, 
                    MPDDATA_MODEL_COL_PATH, &a,
                    MPDDATA_MODEL_COL_SONG_ARTIST, &c, -1);


            g_free(a);
            g_free(b);
            g_free(c);
            gtk_tree_path_free(path);
        }while(gtk_tree_model_iter_next(GTK_TREE_MODEL(model), &iter));
    }
    g_object_unref(model);
    return EXIT_SUCCESS;
}
