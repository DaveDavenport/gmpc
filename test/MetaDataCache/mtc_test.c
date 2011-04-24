#include <stdio.h>
#include <stdlib.h>
#include <glib.h>
#include <libmpd/libmpd.h>
#include "gmpc-extras.h"
#include "metadata.h"
#include "metadata-cache.h"

GObject *config = NULL;
GmpcMetaWatcher *gmw = NULL;
GObject *connection = NULL;

void show_error_message(void)
{
}

void playlist3_show_error_message()
{
}


char *artists[] = {
    "Eric Clapton",
    "Clapton, Eric",
    "B.B. King",
    "Room Eleven",
    "James Brown",
    "Brown, James",
    "10cc",
    "2Pac",
    "Pantera",
    "Pärt, Arvo",
    "XERSDRXER"
};
int main(int argc, char **argv)
{
    long a;
    GTimeVal current,now;
    MetaDataResult result;
    int i=0,j=1000;
   // g_type_init();

    g_get_current_time(&current);
    metadata_cache_init();
    g_get_current_time(&now);
    a = now.tv_usec - current.tv_usec;
    printf("elapsed open db: %li us\n", (a<0)?1-a:a);
    current = now;
    /*
    for(;j>0;j--)
    {
        for(i=0;i<11;i++)
        {
            MetaData *met = NULL;
            mpd_Song *song = mpd_newSong();
            song->artist = g_strdup(artists[i]);
            result = meta_data_get_from_cache(song, META_ARTIST_ART, &met);
            if(met) meta_data_free(met);

            mpd_freeSong(song);
        }
    }

    g_get_current_time(&now);
    a = now.tv_usec - current.tv_usec;
    printf("elapsed 10.000 queries: %li us\n", (a<0)?1-a:a);
    current = now;


    for(j=10;j>0;j--)
    {
        for(i=0;i<11;i++)
        {
            MetaData *met = NULL;
            mpd_Song *song = mpd_newSong();
            song->artist = g_strdup(artists[i]);
            song->album = g_strdup("unplugged");
            result = meta_data_get_from_cache(song, META_ALBUM_ART, &met);
            if(met) meta_data_free(met);

            mpd_freeSong(song);
        }
    }
*/
    g_get_current_time(&now);
    a = now.tv_usec - current.tv_usec;
    printf("elapsed 100 queries: %li us\n", (a<0)?1-a:a);
    current = now;







    metadata_cache_destroy();

    g_get_current_time(&now);
    a = now.tv_usec - current.tv_usec;
    printf("elapsed closing: %li us\n", (a<0)?1-a:a);
    current = now;
    return EXIT_SUCCESS;
}

void pixbuf_cache_invalidate_pixbuf_entry(const gchar *url)
{
}
