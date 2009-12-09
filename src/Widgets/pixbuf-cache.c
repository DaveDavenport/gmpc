/* Gnome Music Player Client (GMPC)
 * Copyright (C) 2004-2009 Qball Cow <qball@sarine.nl>
 * Project homepage: http://gmpc.wikia.com/
 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <strings.h>
#include <glib.h>
#include <gdk/gdk.h>
#include "Widgets/pixbuf-cache.h"

#define LOG_DOMAIN "PixbufCache"

static GHashTable *pb_cache = NULL;
typedef struct {
    const gchar *key;
    GdkPixbuf *pb;

} DCE;

static DCE * create_cache_entry()
{
    return g_slice_new(DCE);
}

static void pixbuf_cache_entry_toggle_ref(const gchar *key, GdkPixbuf *pb, gboolean is_last_ref)
{
    g_log(LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Toggle Reference called: %i:%s", is_last_ref,key);
    if(is_last_ref)
    {
        g_hash_table_remove(pb_cache, key);

    }
}
static void destroy_cache_entry(DCE *entry)
{
    g_return_if_fail(entry != NULL);
    g_log(LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "%i: Destroy cache entry: %p", g_hash_table_size(pb_cache)-1,entry);

    g_object_remove_toggle_ref(entry->pb, (GDestroyNotify)pixbuf_cache_entry_toggle_ref, entry->key);
    g_slice_free(DCE, entry);

}



void pixbuf_cache_create(void)
{
    g_assert(pb_cache == NULL);

    pb_cache = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, (GDestroyNotify)destroy_cache_entry);

    g_log(LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Create pixbuf cache");

}

void pixbuf_cache_destroy(void)
{
    g_log(LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Destroy pixbuf cache");
    g_hash_table_destroy(pb_cache);
    pb_cache = NULL;
}

void pixbuf_cache_add_icon(int size,const gchar *url, GdkPixbuf *pb)
{
    g_assert(pb_cache != NULL);

    gchar *key = g_strdup_printf("%i:%s", size, url);

    if(g_hash_table_lookup(pb_cache, key) == NULL)
    {
        DCE *e = create_cache_entry();
        e->key = key;
        e->pb = pb;
        g_object_add_toggle_ref(pb, (GToggleNotify)pixbuf_cache_entry_toggle_ref, key); 
        g_hash_table_insert(pb_cache, key, e);
        g_log(LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "%i Add entry: %s", g_hash_table_size(pb_cache),key);
    }
}

GdkPixbuf *pixbuf_cache_lookup_icon(int size, const gchar *url)
{
    gchar *key = g_strdup_printf("%i:%s", size, url);
    DCE*retv = NULL;
    retv = g_hash_table_lookup(pb_cache, key);
    g_free(key);

    g_log(LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Found entry: %p", key);
    if(retv)
    {
        return retv->pb;
    }
    return NULL;
}
