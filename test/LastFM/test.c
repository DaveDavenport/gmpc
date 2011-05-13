#include <glib.h>
#include <gtk/gtk.h>
#include "plugin.h"

extern gmpcPlugin lastfm_plugin;
typedef gpointer _config_obj;
config_obj *config = NULL;
GmpcMetaWatcher *gmw = NULL;
MpdObj *connection = NULL;
void pixbuf_cache_invalidate_pixbuf_entry(const gchar *url)
{
}

void show_error_message(void)
{

}
void gmpc_meta_watcher_data_changed (GmpcMetaWatcher * self, mpd_Song * song, MetaDataType type, MetaDataResult result, MetaData * met)
{
}
gboolean 
gmpc_meta_watcher_match_data (MetaDataType type, mpd_Song * song1, mpd_Song * song2)
{
return TRUE;
}
void playlist3_show_error_message(const char *message, int el)
{

}
/**
 * Dummy config system 
 */
int cfg_get_single_value_as_int_with_default(config_obj *config, const gchar *a, const gchar *b, int def)
{
    return def;
}
char * cfg_get_single_value_as_string_with_default(config_obj *config, const gchar *a, const gchar *b, const gchar* def)
{
    return g_strdup(def);
}
char *cfg_get_single_value_as_string(config_obj *config, const gchar *a, const gchar *b)
{
    return NULL;
}
void cfg_set_single_value_as_string(config_obj *config, const gchar *a, const gchar *b, const gchar *def)
{
}

void cfg_set_single_value_as_int(config_obj * cfg, const char *class,
                                 const char *key, int value)
{
}

static int queries = 6;
void callback(GList *list, gpointer data)
{
    GMainLoop *lop = (GMainLoop *)data;
    printf("got result\n");
    if(list)
    {
        GList *iter;
        for(iter = g_list_first(list); iter != NULL; iter = g_list_next(iter))
        {
            MetaData *m = iter->data;
            if(meta_data_is_uri(m))
            {
                const char *uri = meta_data_get_uri(m);
                printf("uri: %s\n", uri);
            }
        }
        g_list_foreach(list, meta_data_free, NULL);
        g_list_free(list);
    }
    queries--;
    printf("done\n");
    if(queries ==0) {
        g_idle_add(g_main_loop_quit,lop);
    }
}

int main (int argc, char **argv)
{
    GMainLoop *l = NULL;
    g_type_init_with_debug_flags(G_TYPE_DEBUG_MASK);
    if(!g_thread_supported())
        g_thread_init(NULL);
    l = g_main_loop_new(NULL, TRUE); 
 
    mpd_Song *song = mpd_newSong();
    song->title = g_strdup("Layla");
    song->genre = g_strdup("Rock");
    song->artist = g_strdup("Eric Clapton");
    song->album = g_strdup("Unplugged");
    lastfm_plugin.metadata->get_metadata(song,  META_ALBUM_ART, callback, l);
    lastfm_plugin.metadata->get_metadata(song,  META_ARTIST_ART, callback, l);
    lastfm_plugin.metadata->get_metadata(song,  META_ARTIST_SIMILAR, callback, l);
    lastfm_plugin.metadata->get_metadata(song,  META_SONG_SIMILAR, callback, l);
    lastfm_plugin.metadata->get_metadata(song,  META_GENRE_SIMILAR, callback, l);
    lastfm_plugin.metadata->get_metadata(song,  META_ARTIST_TXT, callback, l);
    mpd_freeSong(song);

    g_main_loop_run(l);
    gmpc_easy_async_quit();
    g_main_loop_unref(l);
    xmlCleanupParser();
    return EXIT_SUCCESS;
}
