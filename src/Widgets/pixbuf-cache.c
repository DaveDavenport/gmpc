/* Gnome Music Player Client (GMPC)
 * Copyright (C) 2004-2010 Qball Cow <qball@sarine.nl>
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

/* The hash table looking up the entries */
static GHashTable *pb_cache = NULL;

/* The structure holding the cache entry */
typedef struct {
    const gchar *key;
    GdkPixbuf *pb;
    time_t last_used;
    gboolean in_use;
} DCE;
static guint timeout = 0;
/* Creates a new cache entry */
static DCE * create_cache_entry(void)
{
    DCE *e = g_slice_new(DCE);
    e->last_used = time(NULL);
    return e;
}

static gboolean pixbuf_cache_timeout_passed(void)
{
    GHashTableIter iter;
    gchar *key;
    DCE *e;
    GList *liter, *list = NULL;
    g_log(LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Pixbuf cache timeout passed\n");
    g_hash_table_iter_init (&iter,pb_cache );
    while (g_hash_table_iter_next (&iter, (gpointer)&key, (gpointer)&e))
    {
        /* do something with key and value */
        if(e->in_use == FALSE)
        {
            list = g_list_prepend(list,(gpointer) e->key);
        }
    }
    for(liter = g_list_first(list); liter; liter= g_list_next(liter))
    {
        g_hash_table_remove(pb_cache, (gchar *)liter->data);
    }
    g_list_free(list);
    timeout = 0;
    return FALSE;
}

/* Called when the pixbuf looses it last entry */
static void pixbuf_cache_entry_toggle_ref(const gchar *key, GdkPixbuf *pb, gboolean is_last_ref)
{
    g_log(LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Toggle Reference called: %i:%s", is_last_ref,key);
    if(is_last_ref)
    {
        DCE *e = g_hash_table_lookup(pb_cache, key);
        if(e) e->in_use = FALSE;
        if(timeout > 0) g_source_remove(timeout);
        g_log(LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "set 10 seconds timeout\n");
        timeout = g_timeout_add_seconds(10, (GSourceFunc)pixbuf_cache_timeout_passed, NULL);
    }
}
/* Destroy cache entry */
static void destroy_cache_entry(DCE *entry)
{
    g_return_if_fail(entry != NULL);
    g_log(LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "%i: Destroy cache entry: %p", g_hash_table_size(pb_cache)-1,entry);

    g_object_remove_toggle_ref(G_OBJECT(entry->pb), (GToggleNotify)pixbuf_cache_entry_toggle_ref,(gpointer) entry->key);
    g_slice_free(DCE, entry);

}



void pixbuf_cache_invalidate_pixbuf_entry(gchar *url)
{
    gchar *key;
    DCE *e;
    GList *liter,*list= NULL;
    GHashTableIter iter;
    g_hash_table_iter_init (&iter,pb_cache );
    while (g_hash_table_iter_next (&iter, (gpointer)&key, (gpointer)&e))
    {
        int i;
        int length = strlen(key);
        for(i=0; i< length && key[i] != ':'; i++);
        if(key[i] == ':'){
            if(g_utf8_collate(&key[i+1],url) == 0) {
                g_log(LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Invalidate :%s", e->key);
                list = g_list_prepend(list,(gpointer) e->key);
            }
        }
    }
    for(liter = g_list_first(list); liter; liter= g_list_next(liter))
    {
        g_hash_table_remove(pb_cache, (gchar *)liter->data);
    }
    g_list_free(list);
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
    if(timeout > 0) g_source_remove(timeout);
    timeout = 0;
    g_hash_table_destroy(pb_cache);
    pb_cache = NULL;
}

void pixbuf_cache_add_icon(int size,const gchar *url, GdkPixbuf *pb)
{
    gchar *key;
    g_assert(pb_cache != NULL);

    key = g_strdup_printf("%i:%s", size, url);

    if(g_hash_table_lookup(pb_cache, key) == NULL)
    {
        DCE *e = create_cache_entry();
        e->key = key;
        e->pb = pb;
        e->in_use = TRUE;
        g_object_add_toggle_ref(G_OBJECT(pb), (GToggleNotify)pixbuf_cache_entry_toggle_ref,(gpointer) key);
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
        retv->in_use = TRUE;
        return g_object_ref(retv->pb);
    }
    return NULL;
}
