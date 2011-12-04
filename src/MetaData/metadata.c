/* Gnome Music Player Client (GMPC)
 * Copyright (C) 2004-2011 Qball Cow <qball@gmpclient.org>
 * Project homepage: http://gmpclient.org/

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

#include <string.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <ctype.h>
#include "main.h"

#include "metadata.h"
//#include "metadata-cache.h"
#include "preferences.h"

#include "gmpc_easy_download.h"

#define LOG_DOMAIN "MetaData"

#include <glyr/glyr.h>
#include <glyr/cache.h>

#define LOG_SUBCLASS        "glyros"
#define LOG_COVER_NAME      "fetch-art-album"
#define LOG_ARTIST_ART      "fetch-art-artist"
#define LOG_SIMILIAR_ARTIST "fetch-similiar-artist"
#define LOG_SIMILIAR_SONG   "fetch-similiar-song"
#define LOG_SIMILIAR_GENRE  "fetch-similiar-genre"
#define LOG_ARTIST_TXT      "fetch-biography-artist"
#define LOG_SONG_TXT        "fetch-lyrics"
#define LOG_GUITARTABS      "fetch-guitartabs"
#define LOG_ALBUM_TXT       "fetch-album-txt"

// other
#define LOG_FUZZYNESS      "fuzzyness"
#define LOG_CMINSIZE       "cminsize"
#define LOG_CMAXSIZE       "cmaxsize"
#define LOG_MSIMILIARTIST  "msimiliartist"
#define LOG_MSIMILISONG    "msimilisong"
#define LOG_QSRATIO        "qsratio"
#define LOG_PARALLEL       "parallel"
#define LOG_USERAGENT      "useragent"
#define LOG_FROM           "from"
int meta_num_plugins=0;
gmpcPluginParent **meta_plugins = NULL;
static void meta_data_sort_plugins(void);
GList *process_queue = NULL;


/**
 * GLYR
 */

static GAsyncQueue *gaq = NULL;
static GAsyncQueue *return_queue = NULL;
const char *plug_name = "glyr";
static GlyrDatabase *db = NULL;


/**
 * This queue is used to send replies back.
 */
enum MTD_Action {
	MTD_ACTION_QUERY_METADATA,
	MTD_ACTION_CLEAR_ENTRY,
	MTD_ACTION_QUIT
};

/**
 * Structure holding a metadata query */
typedef struct {
	/* The type of action todo */
	enum MTD_Action action;
	/* unique id for the query (unused)*/
	guint id;
	/* The callback to call when the query is done, or NULL */
	MetaDataCallback callback;
	/* Callback user_data pointer*/
	gpointer data;
	/* The song the data is queries for */
	mpd_Song *song;
	/* The song modified by the search system.
	 * This does albumartist, tag cleanup etc */
	mpd_Song *edited;
	/* The type of metadata */
	MetaDataType type;
	/* Result  */
	MetaDataResult result;
	/* The actual result data */
	MetaData *met;
	int do_rename;
	int rename_done;
#if 0
	/* List with temporary result from plugin index */
	GList *list;
	/* The current position in the list */
	GList *iter;
#endif
} meta_thread_data;

gboolean meta_compare_func(meta_thread_data *mt1, meta_thread_data *mt2);
//static gboolean meta_data_handle_results(void);


/*
   static gboolean meta_data_handler_data_match(meta_thread_data *data, gpointer data2);
 */
mpd_Song *rewrite_mpd_song(mpd_Song *tsong, MetaDataType type)
{
	mpd_Song *edited = NULL;
	/* If it is not a mpd got song */
	if(tsong->file == NULL )
	{
		if(type&(META_ALBUM_ART|META_ALBUM_TXT) && tsong->artist && tsong->album)
		{
			MpdData *data2 = NULL;
			mpd_database_search_start(connection, TRUE);
			mpd_database_search_add_constraint(connection, MPD_TAG_ITEM_ARTIST, tsong->artist);
			mpd_database_search_add_constraint(connection, MPD_TAG_ITEM_ALBUM,  tsong->album);
			data2 = mpd_database_search_commit(connection);
			if(data2)
			{
				edited = data2->song;
				data2->song = NULL;
				mpd_data_free(data2);
			}
			else if(mpd_server_tag_supported(connection,MPD_TAG_ITEM_ALBUM_ARTIST) && tsong->albumartist)
			{
				mpd_database_search_start(connection, TRUE);
				mpd_database_search_add_constraint(connection, MPD_TAG_ITEM_ALBUM_ARTIST, tsong->albumartist);
				mpd_database_search_add_constraint(connection, MPD_TAG_ITEM_ALBUM,  tsong->album); 
				data2 = mpd_database_search_commit(connection);
				if(data2)
				{
					edited = data2->song;
					data2->song = NULL;
					mpd_data_free(data2);
				}


			}
		}
		if(type&(META_ARTIST_ART|META_ARTIST_TXT) && tsong->artist)
		{
			MpdData *data2 = NULL;
			mpd_database_search_start(connection, TRUE);
			mpd_database_search_add_constraint(connection, MPD_TAG_ITEM_ARTIST, tsong->artist);
			data2 = mpd_database_search_commit(connection);
			if(data2)
			{
				edited = data2->song;
				data2->song = NULL;
				mpd_data_free(data2);
			}
		}
	}
	if(!edited)
		edited = mpd_songDup(tsong);

	/**
	 * Collections detection
	 * Only do this for album related queries.
	 */
	if(type&(META_ALBUM_ART|META_ALBUM_TXT))
	{
		if(edited->albumartist)
		{
			if(edited->artist)
				g_free(edited->artist);
			edited->artist = g_strdup(edited->albumartist);

		}
		else if(edited->album && edited->file)
		{
			int i=0;
			MpdData *data2;
			char *dir = g_path_get_dirname(edited->file);
			mpd_database_search_start(connection,TRUE);
			mpd_database_search_add_constraint(connection, MPD_TAG_ITEM_ALBUM, edited->album);


			data2 = mpd_database_search_commit(connection);
			if(data2)
			{
				for(i=0;data2; data2 = mpd_data_get_next(data2))
				{
					if(strncasecmp(data2->song->file, dir, strlen(dir))==0)
					{
						/* Check for NULL pointers */
						if(data2->song->artist && edited->artist)
						{
							if(strcmp(data2->song->artist, edited->artist))
								i++;
						}
					}
				}
			}
			if(i >=3)
			{
				if(edited->artist)
					g_free(edited->artist);
				edited->artist = g_strdup("Various Artists");
			}
			g_free(dir);
		}
	}
	/**
	 * Artist renaming, Clapton, Eric -> Eric Clapton
	 */
	if(edited->artist && cfg_get_single_value_as_int_with_default(config, "metadata", "rename", FALSE))
	{
		gchar **str = g_strsplit(edited->artist, ",", 2);

		if(str[0] && str[1]) {
			g_free(edited->artist);
			edited->artist = g_strdup_printf("%s %s", g_strstrip(str[1]), g_strstrip(str[0]));
		}
		g_strfreev(str);
		g_log(LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "string converted to: '%s'", edited->artist);
	}

	/**
	 * Sanitize album name and so (remove () (
	 */
	if(cfg_get_single_value_as_int_with_default(config, "metadata", "sanitize", TRUE))
	{
		if(edited->album)
		{
			int i,j=0,depth=0;
			int length;
			char *album = edited->album;
			edited->album = g_malloc0((strlen(album)+1)*sizeof(char));
			length = strlen(album);
			for(i=0;i< length;i++)
			{
				if(album[i] == '(') depth++;
				else if (album[i] == ')')depth--;
				else if (depth  == 0) {
					edited->album[j] = album[i];
					j++;
				}
			}
			g_free(album);
			/* Remove trailing  and leading spaces */
			edited->album = g_strstrip(edited->album);
		}
		if(edited->title)
		{
			int i,j=0,depth=0;
			char *title = edited->title;
			int length;
			edited->title = g_malloc0((strlen(title)+1)*sizeof(char));
			length = strlen(title);
			for(i=0;i< length;i++)
			{
				if(title[i] == '(') depth++;
				else if (title[i] == ')')depth--;
				else if (depth  == 0) {
					edited->title[j] = title[i];
					j++;
				}
			}
			g_free(title);
			/* Remove trailing  and leading spaces */
			edited->title = g_strstrip(edited->title);
		}


	}
	return edited;
}
/**
 * If the metadata thread managed to handle a result this function
 * Will call the callback (from the main (gtk) thread)
 */
static gboolean glyr_return_queue(void *user_data)
{
	
	meta_thread_data *mtd = (meta_thread_data*)g_async_queue_try_pop(return_queue);
	if(mtd)
	{
		if(mtd->action == MTD_ACTION_QUERY_METADATA)
		{
			printf("%s: results\n", __FUNCTION__);

			gmpc_meta_watcher_data_changed(gmw,mtd->song, (mtd->type)&META_QUERY_DATA_TYPES, mtd->result,mtd->met);
			if(mtd->callback)
			{
				mtd->callback(mtd->song, mtd->result, mtd->met, mtd->data);
			}
		}else if (mtd->action == MTD_ACTION_CLEAR_ENTRY) {
			// Signal that this item is now no longer available.
			gmpc_meta_watcher_data_changed(gmw, mtd->song, (mtd->type)&META_QUERY_DATA_TYPES, META_DATA_UNAVAILABLE, NULL);
		}

		/* Free the result data */
		if(mtd->met)
			meta_data_free(mtd->met);
		/* Free the copie and edited version of the songs */
		if(mtd->song)
			mpd_freeSong(mtd->song);
		if(mtd->edited)
			mpd_freeSong(mtd->edited);

		/* Free the Request struct */
		g_free(mtd);
		return true;
	}
	return false;
}

static MetaDataContentType setup_glyr_query(GlyrQuery *query,
	const meta_thread_data *mtd)
{
	MetaDataContentType content_type = META_DATA_CONTENT_RAW;
	MetaDataType type = mtd->type&META_QUERY_DATA_TYPES;	

	/* Force UTF 8 */
	glyr_opt_force_utf8(query, TRUE);

	glyr_opt_parallel(query, 4);
	glyr_opt_number(query, 1);

	/* set metadata */
	glyr_opt_artist(query,(char*)mtd->song->artist);
	glyr_opt_album (query,(char*)mtd->song->album);
	glyr_opt_title (query,(char*)mtd->song->title);

	/* set default type */
	glyr_opt_type(query, GLYR_GET_UNSURE);

	if(type == META_ARTIST_ART )
	{
		glyr_opt_type(query, GLYR_GET_ARTIST_PHOTOS);
		content_type = META_DATA_CONTENT_RAW;
	}
	else if (type == META_BACKDROP_ART)
	{
		glyr_opt_type(query, GLYR_GET_BACKDROPS);
		content_type = META_DATA_CONTENT_RAW;
	}
	else if(type == META_ARTIST_TXT)
	{
		glyr_opt_lang_aware_only(query,TRUE);
		glyr_opt_type(query, GLYR_GET_ARTISTBIO);
		content_type = META_DATA_CONTENT_TEXT;
	}
	else if(type == META_ARTIST_SIMILAR)
	{
		glyr_opt_type(query, GLYR_GET_SIMILIAR_ARTISTS);
		// cfg_* is no longer thread safe 
		glyr_opt_number(query,20);
		content_type = META_DATA_CONTENT_TEXT;
	}
	else if(type == META_ALBUM_ART &&
			mtd->song->album != NULL)
	{
		glyr_opt_type(query, GLYR_GET_COVERART);
		content_type = META_DATA_CONTENT_RAW;
	}
	else if(type == META_ALBUM_TXT &&
			mtd->song->album != NULL)
	{
		glyr_opt_type(query, GLYR_GET_ALBUM_REVIEW);
		content_type = META_DATA_CONTENT_TEXT;
	}
	else if(type == META_SONG_TXT &&
			mtd->song->title != NULL)
	{
		glyr_opt_type(query, GLYR_GET_LYRICS);
		content_type = META_DATA_CONTENT_TEXT;
	}
	else if(type == META_SONG_SIMILAR &&
			mtd->song->title != NULL) 
	{
		glyr_opt_type(query, GLYR_GET_SIMILIAR_SONGS);
		// cfg_* is no longer thread safe 
		glyr_opt_number(query,20); 
	}
	else if (type == META_SONG_GUITAR_TAB &&
			mtd->song->title) 
	{
		glyr_opt_type(query, GLYR_GET_GUITARTABS);
		content_type = META_DATA_CONTENT_TEXT;
	}
	else {
		g_warning("Unsupported metadata type, or insufficient info");
	}
	return content_type;
}
static MetaData * glyr_get_similiar_song_names(GlyrMemCache * cache)
{
    MetaData * mtd = NULL;
    while(cache != NULL)
    {
        if(cache->data != NULL)
        {
            gchar ** split = g_strsplit(cache->data,"\n",0);
            if(split != NULL && split[0] != NULL)
            {
                gchar * buffer;
                if(!mtd) {
                    mtd = meta_data_new();
                    mtd->type = META_SONG_SIMILAR;
                    mtd->plugin_name = plug_name; 
                    mtd->content_type = META_DATA_CONTENT_TEXT_LIST;
                    mtd->size = 0;
                }

                buffer = g_strdup_printf("%s::%s",split[1],split[0]);
                g_log(LOG_DOMAIN,G_LOG_LEVEL_DEBUG, "%s\n", buffer);

                mtd->size++;
                mtd->content = g_list_append((GList*) mtd->content, buffer);
                g_strfreev(split);
            }
        }
        cache = cache->next;
    }
    return mtd;
}

static MetaData * glyr_get_similiar_artist_names(GlyrMemCache * cache)
{
    MetaData * mtd = NULL;
    while(cache != NULL)
    {
        if(cache->data != NULL)
        {
            gchar ** split = g_strsplit(cache->data,"\n",0);
            if(split != NULL)
            {
                if(!mtd) {
                    mtd = meta_data_new();
                    mtd->type = META_ARTIST_SIMILAR;
                    mtd->plugin_name = plug_name; 
                    mtd->content_type = META_DATA_CONTENT_TEXT_LIST;
                    mtd->size = 0;
                }
                mtd->size++;
                mtd->content = g_list_append((GList*) mtd->content,
						g_strdup((char *)split[0]));
				g_strfreev(split);
            }
        }
        cache = cache->next;
    }
    return mtd;
}
/**
 * Convert GLYR result into gmpc result 
 */
static gboolean process_glyr_result(GlyrMemCache *cache, 
	MetaDataContentType content_type,
	meta_thread_data *mtd)
{
	gboolean retv = FALSE;
	mtd->result = META_DATA_UNAVAILABLE;
	mtd->met = NULL;
	if(cache == NULL) return retv;
	if(cache->rating >= 0)
	{
        if(mtd->type == META_ARTIST_SIMILAR)
        {
            MetaData * cont = glyr_get_similiar_artist_names(cache);
            if(cont != NULL)
            {
				(mtd->met) = cont;
				mtd->result = META_DATA_AVAILABLE;
				retv = TRUE;
            }
        }
        else if (mtd->type == META_SONG_SIMILAR)
        {
            MetaData * cont;
            cont = glyr_get_similiar_song_names(cache);
            if (cont != NULL)
            {
				(mtd->met) = cont;
				mtd->result = META_DATA_AVAILABLE;
				retv = TRUE;
            }
        }
		else
		{
			(mtd->met) = meta_data_new();
			(mtd->met)->type = mtd->type;
			(mtd->met)->plugin_name = plug_name; 
			(mtd->met)->content_type = content_type;

			(mtd->met)->content = g_malloc0(cache->size);
			memcpy((mtd->met)->content, cache->data, cache->size);
			(mtd->met)->size = cache->size;
			mtd->result = META_DATA_AVAILABLE;
			// found something.
			retv = TRUE;
		}
	}else { 
		// Explicitely not found.
		printf("Cache sais empty\n");
		retv = TRUE;
	}
	return retv;
}

static GlyrQuery *glyr_exit_handle = NULL;
static GMutex *exit_handle_lock = NULL;

/**
 * Thread that does the GLYR requests
 */
void glyr_fetcher_thread(void *user_data)
{
	void *d;
	GlyrQuery query;

	g_mutex_lock(exit_handle_lock);
	while((d = g_async_queue_pop(gaq)))
	{
		meta_thread_data    *mtd         = (meta_thread_data*)d;

		// Check if this is the quit command.
		if(mtd->action == MTD_ACTION_QUIT) {
			printf("Quitting....");
			g_mutex_unlock(exit_handle_lock);
			/* Free the Request struct */
			g_free(mtd);
			return;
		}
		else if (mtd->action == MTD_ACTION_CLEAR_ENTRY)
		{
			GlyrMemCache        *cache       = NULL;
			// Setup cancel lock
			glyr_exit_handle = &query;
			g_mutex_unlock(exit_handle_lock);

			/* Set up the query */
			glyr_query_init(&query);
			setup_glyr_query(&query, mtd);

			// Delete existing entries.
			glyr_db_delete(db, &query);

			// Set dummy entry in cache, so we know
			// we searched for this before.
			cache = glyr_cache_new();
			glyr_cache_set_data(cache, g_strdup("GMPC Dummy data"), -1);
			cache->dsrc = g_strdup("GMPC dummy insert");
			cache->prov = g_strdup("none");
			cache->img_format = g_strdup("");
			cache->rating = -1;

			// Add dummy entry
			glyr_db_insert(db,&query, cache);

			// Cleanup
			if(cache)glyr_free_list(cache);

			// Clear the query, and lock the handle again.
			g_mutex_lock(exit_handle_lock);
			glyr_exit_handle = NULL;
			glyr_query_destroy(&query);

			// Push back result, and tell idle handle to handle it.
			g_async_queue_push(return_queue, mtd);
			// invalidate pointer.
			mtd = NULL;
			// Schedule the result thread in idle time.
			g_idle_add(glyr_return_queue, NULL);
		}
		// QUERY database
		else if (mtd->action == MTD_ACTION_QUERY_METADATA)
		{
			// Check if this is thread safe.
			const char 			*md			 = connection_get_music_directory();
			GLYR_ERROR          err          = GLYRE_OK;
			MetaDataContentType content_type = META_DATA_CONTENT_RAW;
			GlyrMemCache        *cache       = NULL;

			glyr_exit_handle = &query;
			g_mutex_unlock(exit_handle_lock);

			printf("new style query\n");

			glyr_query_init(&query);


			if(md != NULL && md[0] !=  '\0'&& mtd->song->file != NULL)
			{
				const char *path = g_build_filename(md, mtd->song->file, NULL);
				glyr_opt_musictree_path(&query, path);
				g_free(path);
			}


			/* Set up the query */
			content_type = setup_glyr_query(&query, mtd);

			/* If cache disabled, remove the entry in the db */
			if(((mtd->type)&META_QUERY_NO_CACHE) == META_QUERY_NO_CACHE)
			{
				printf("Disable cache\n");
				glyr_opt_from(&query, "all;-local");
				// Remove cache request.
				mtd->type&=META_QUERY_DATA_TYPES;	
				// Delete the entry.
				glyr_db_delete(db, &query);
			}

			/* Set some random settings */
			glyr_opt_verbosity(&query,3);

			/* Tell libglyr to automatically lookup before searching the web */
			glyr_opt_lookup_db(&query, db);

			/* Also tell it to write newly found items to the db */
			glyr_opt_db_autowrite(&query, TRUE);


			/* get metadata */
			cache = glyr_get(&query,&err,NULL);

			if(cache == NULL){
				// Set dummy entry in cache, so we know
				// we searched for this before.
				cache = glyr_cache_new();
				glyr_cache_set_data(cache, g_strdup("GMPC Dummy data"), -1);
				cache->dsrc = g_strdup("GMPC dummy insert");
				cache->prov = g_strdup("none");
				cache->img_format = g_strdup("");
				cache->rating = -1;

				glyr_db_insert(db,&query, cache);
				printf("Cache is Empty\n");
				// Set unavailable 
				mtd->result = META_DATA_UNAVAILABLE; 
			}else{
				printf("Provider: %s\n", cache->prov);
				process_glyr_result(cache,content_type, mtd);
			}
			// Cleanup
			if(cache)glyr_free_list(cache);

			// Clear the query, and lock the handle again.
			g_mutex_lock(exit_handle_lock);
			glyr_exit_handle = NULL;
			glyr_query_destroy(&query);

			// Push back result, and tell idle handle to handle it.
			g_async_queue_push(return_queue, mtd);
			// invalidate pointer.
			mtd = NULL;
			// Schedule the result thread in idle time.
			g_idle_add(glyr_return_queue, NULL);
		}else {
			g_error("Unknown type of query to perform");
			return;
		}
	}
}

/**
 * Initialize
 */
GThread *gaq_fetcher_thread = NULL;
void meta_data_init(void)
{
	gchar *url;

	/* Is this function thread safe? */
	url = gmpc_get_covers_path("");
		
	exit_handle_lock = g_mutex_new();
	/* Initialize..*/
	printf("open glyr db: %s\n", url);
	glyr_init();
	db = glyr_db_init(url);
	printf("db: %p\n", db);
	g_free(url);


	gaq = g_async_queue_new();
	return_queue = g_async_queue_new();
	gaq_fetcher_thread = g_thread_create(glyr_fetcher_thread, NULL, TRUE, NULL);

}

void meta_data_add_plugin(gmpcPluginParent *plug)
{
	g_assert(plug != NULL);

	meta_num_plugins++;
	meta_plugins = g_realloc(meta_plugins,(meta_num_plugins+1)*sizeof(gmpcPluginParent **));
	meta_plugins[meta_num_plugins-1] = plug;
	meta_plugins[meta_num_plugins] = NULL;
	meta_data_sort_plugins();
}

static void meta_data_sort_plugins(void)
{
	int i;
	int changed = FALSE;	
	do{	
		changed=0;
		for(i=0; i< (meta_num_plugins-1);i++)
		{
			if(gmpc_plugin_metadata_get_priority(meta_plugins[i]) > gmpc_plugin_metadata_get_priority(meta_plugins[i+1]))
			{
				gmpcPluginParent *temp = meta_plugins[i];
				changed=1;
				meta_plugins[i] = meta_plugins[i+1];
				meta_plugins[i+1] = temp;
			}
		}
	}while(changed);
}

static gboolean meta_data_check_plugin_changed_message(gpointer data)
{
	playlist3_show_error_message(_("A new metadata plugin was added, gmpc has purged all failed hits from the cache"), ERROR_INFO);
	return FALSE;
}
void meta_data_check_plugin_changed(void)
{
	int old_amount= cfg_get_single_value_as_int_with_default(config, "metadata", "num_plugins", 0);
	if(old_amount < meta_num_plugins)
	{
		gtk_init_add(meta_data_check_plugin_changed_message, NULL);
		//metadata_cache_cleanup();
	}
	if(old_amount != meta_num_plugins)
	{
		cfg_set_single_value_as_int(config, "metadata", "num_plugins", meta_num_plugins);
	}
}
/**
 * TODO: Can we guarantee that all the downloads are stopped? 
 */
void meta_data_destroy(void)
{
	meta_thread_data *mtd = NULL;
	INIT_TIC_TAC();
#if 0
	if(process_queue) {
		GList *iter;
		/* Iterate through the list and destroy all entries */
		for(iter = g_list_first(process_queue); iter; iter = iter->next)
		{
			mtd = iter->data;

			/* Free any possible plugin results */
			g_list_foreach(mtd->list, (GFunc)meta_data_free, NULL);
			g_list_free(mtd->list);
			mtd->list = mtd->iter = NULL;
			/* Destroy the result */
			if(mtd->met) 
				meta_data_free(mtd->met);
			/* destroy the copied and modified song */
			if(mtd->song)
				mpd_freeSong(mtd->song);
			if(mtd->edited)
				mpd_freeSong(mtd->edited);

			/* Free the Request struct */
			g_free(mtd);
		}
		g_list_free(process_queue);
		process_queue = NULL;
	}

	g_log(LOG_DOMAIN, G_LOG_LEVEL_DEBUG,"Done..");
	/* Close the cover database  */
	TOC("test")
	metadata_cache_destroy();
	TOC("Config saved")
#endif
	/**
 	 * Clear the request queue, and tell thread to quit 
	 */
	g_async_queue_lock(gaq);
	while((mtd = g_async_queue_try_pop_unlocked(gaq))){
		 /* Free any possible plugin results */
/*
		if(mtd->list){
			g_list_foreach(mtd->list, (GFunc)meta_data_free, NULL);
			g_list_free(mtd->list);
		}

*/		/* Free the result data */
		if(mtd->met)
			meta_data_free(mtd->met);
		/* Free the copie and edited version of the songs */
		if(mtd->song)
			mpd_freeSong(mtd->song);
		if(mtd->edited)
			mpd_freeSong(mtd->edited);

		/* Free the Request struct */
		g_free(mtd);
	}
	mtd = g_malloc0(sizeof(*mtd));
	mtd->action = MTD_ACTION_QUIT;
	g_async_queue_push_unlocked(gaq, mtd);
	mtd = NULL;
	g_async_queue_unlock(gaq);
	// add lock? 
	g_mutex_lock(exit_handle_lock);
	if(glyr_exit_handle) {
		printf("Sending quit signal\n");
		glyr_signal_exit(glyr_exit_handle);
	}
	g_mutex_unlock(exit_handle_lock);

	printf("Waiting for glyr to finish.....\n");
	g_thread_join(gaq_fetcher_thread);
	g_mutex_free(exit_handle_lock);

	glyr_db_destroy(db);
	glyr_cleanup();
	/**
 	 * Wait for thread to quit 
	 */
	g_async_queue_lock(return_queue);
	while((mtd = g_async_queue_try_pop_unlocked(return_queue))){
		 /* Free any possible plugin results */
/*		if(mtd->list){
			g_list_foreach(mtd->list, (GFunc)meta_data_free, NULL);
			g_list_free(mtd->list);
		}
*/		/* Free the result data */
		if(mtd->met)
			meta_data_free(mtd->met);
		/* Free the copie and edited version of the songs */
		if(mtd->song)
			mpd_freeSong(mtd->song);
		if(mtd->edited)
			mpd_freeSong(mtd->edited);

		/* Free the Request struct */
		g_free(mtd);
	}
	g_async_queue_unlock(return_queue);
	g_async_queue_unref(gaq);
	g_async_queue_unref(return_queue);

}
gboolean meta_compare_func(meta_thread_data *mt1, meta_thread_data *mt2)
{
	if((mt1->type&META_QUERY_DATA_TYPES) != (mt2->type&META_QUERY_DATA_TYPES))
		return TRUE;
	if(!gmpc_meta_watcher_match_data(mt1->type&META_QUERY_DATA_TYPES, mt1->song, mt2->song))
	{
		return TRUE;
	}
	return FALSE;
}


static int test_id = 0;

void meta_data_clear_entry(mpd_Song *song, MetaDataType type)
{
	meta_thread_data *mtd = g_malloc0(sizeof(*mtd));
	mtd->action = MTD_ACTION_CLEAR_ENTRY;
	mtd->id = ++test_id;
	/* Create a copy of the original song */
	mtd->song = mpd_songDup(song);
	/* Set the type */
	mtd->type = type;
	/* set result NULL */
	mtd->met = NULL;

	g_async_queue_push(gaq, mtd);
	mtd = NULL;
}
/**
 * Function called by the "client" 
 */

MetaDataResult meta_data_get_path(mpd_Song *tsong, MetaDataType type, MetaData **met,MetaDataCallback callback, gpointer data)
{
	meta_thread_data *mtd = NULL;
	
	INIT_TIC_TAC()

	/**
	 * unique id 
	 * Not needed, but can be usefull for debugging
	 */
	
	mtd = g_malloc0(sizeof(*mtd));
	mtd->action = MTD_ACTION_QUERY_METADATA;
	mtd->id = ++test_id;
	/* Create a copy of the original song */
	mtd->song = mpd_songDup(tsong);
	/* Set the type */
	mtd->type = type;
	/* the callback */
	mtd->callback = callback;
	/* the callback data */
	mtd->data = data;
	/* start at the first plugin */
	mtd->do_rename = cfg_get_single_value_as_int_with_default(config, "metadata", "rename", FALSE);
	mtd->rename_done = FALSE;
	/* Set that we are fetching */
	mtd->result = META_DATA_FETCHING;
	/* set result NULL */
	mtd->met = NULL;
	/**
     * If requested query the cache first 
	 */
	if((type&META_QUERY_NO_CACHE) == 0)
	{
		GLYR_ERROR          err          = GLYRE_OK;
		const char 			*md			 = connection_get_music_directory();
		MetaDataResult mrd;
		MetaDataContentType content_type = META_DATA_CONTENT_RAW;
		GlyrQuery query;
		GlyrMemCache * cache = NULL;
		printf("Try querying cache\n");

		glyr_query_init(&query);
		/* Set some random settings */
		glyr_opt_verbosity(&query,3);
		
		if(md != NULL && md[0] !=  '\0' && mtd->song->file != NULL)
		{
			const char *path = g_build_filename(md, mtd->song->file, NULL);
			glyr_opt_musictree_path(&query, path);
			g_free(path);
		}

		content_type = setup_glyr_query(&query, mtd);

		cache = glyr_db_lookup(db, &query);
		printf("cache: %p\n", cache);
		if(process_glyr_result(cache,content_type, mtd))
		{
			// Cleanup
			if(cache)glyr_free_list(cache);
			glyr_query_destroy(&query);
			
			//gmpc_meta_watcher_data_changed(gmw,mtd->song, (mtd->type)&META_QUERY_DATA_TYPES, mtd->result,mtd->met);
			mrd = mtd->result;
			*met = mtd->met;
			mtd->met = NULL;
			if(mtd->met)
				meta_data_free(mtd->met);
			/* Free the copie and edited version of the songs */
			if(mtd->song)
				mpd_freeSong(mtd->song);
			if(mtd->edited)
				mpd_freeSong(mtd->edited);
			g_free(mtd);
			printf("Got from cache\n");

			TEC("Got from cache");
			return mrd;
		}
		if(cache)glyr_free_list(cache);
		glyr_query_destroy(&query);
	}
	else
	{
		gmpc_meta_watcher_data_changed(gmw,mtd->song, (mtd->type)&META_QUERY_DATA_TYPES, META_DATA_FETCHING,NULL);
		if(mtd->callback)
		{
			mtd->callback(mtd->song, META_DATA_FETCHING, NULL, mtd->data);
		}
	}
	printf("start query\n");

	TEC("Pushing actual query");

	g_async_queue_push(gaq, mtd);
	mtd = NULL;
	return META_DATA_FETCHING;
}

static gchar * strip_invalid_chars(gchar *input)
{
	int i = 0;
	int length = 0;
	g_assert(input != NULL);
	length = strlen(input);
	if(input == NULL) return NULL;
	for(i=0;i<length;i++)
	{
		switch(input[i])
		{
			case ':':
			case '\\':
			case '/':
			case ';':
			case '*':
			case '?':
				case '\"':
			case '<':
			case '>':
			case '|':
					input[i] = ' ';
			default:
					break;
		}
	}
	return input;
}
/**
 * Helper function for storing metadata
 */
gchar * gmpc_get_metadata_filename(MetaDataType  type, mpd_Song *song, char *ext)
{
	gchar *retv= NULL;
	/* home dir */
	const gchar *homedir = g_get_user_cache_dir();
	g_assert(song->artist != NULL);
	g_assert(type < META_QUERY_DATA_TYPES); 

	{
		GError *error = NULL;
		gchar *filename = NULL, *dirname = NULL;
		const gchar *extension= (type&(META_ALBUM_TXT|META_ARTIST_TXT|META_SONG_TXT|META_SONG_GUITAR_TAB))?"txt":((ext == NULL)?((type&(META_ALBUM_ART|META_ARTIST_ART))?"jpg":""):ext);

		/* Convert it so the filesystem likes it */
		/* TODO: Add error checking */

		dirname = g_filename_from_utf8(song->artist, -1, NULL, NULL, NULL);
		/*
		   if (g_get_charset (&charset))
		   {
		   g_log(LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Locale is utf-8, just copying");
		   dirname = g_strdup(song->artist);
		   }else{
		   g_log(LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Locale is %s converting to UTF-8", charset);
		   dirname = g_convert_with_fallback (song->artist, -1,
		   charset, "UTF-8","-", NULL, NULL, &error);
		   }
		 */
		if(dirname == NULL)
		{
			const gchar *charset;
			g_get_charset (&charset);
			dirname = g_convert_with_fallback (song->artist, -1,
					charset, "UTF-8",(char *)"-", NULL, NULL, &error);
		}
		//dirname = g_filename_from_utf8(song->artist,-1,NULL,NULL,&error); 
		if(error) {
			g_log(LOG_DOMAIN, G_LOG_LEVEL_WARNING, "Failed to convert %s to file encoding. '%s'", song->artist, error->message);
			g_error_free(error);
			if(dirname) g_free(dirname);
			dirname = g_strdup("invalid");
		}
		dirname = strip_invalid_chars(dirname);
		retv = g_build_path(G_DIR_SEPARATOR_S, homedir,"gmpc","metadata", dirname,NULL);
		if(g_file_test(retv, G_FILE_TEST_EXISTS) == FALSE) {
			if(g_mkdir_with_parents(retv, 0755) < 0) {
				g_error("Failed to create: %s\n", retv);
				abort();
			}
		}
		if(!g_file_test(retv, G_FILE_TEST_IS_DIR)) {
			g_error("File exists but is not a directory: %s\n", retv);
			abort();
		}
		g_free(retv);
		if(type&(META_ALBUM_ART|META_ALBUM_TXT)) {
			gchar *temp ;
			g_assert(song->album != NULL);
			temp =g_filename_from_utf8(song->album,-1,NULL,NULL,NULL); 
			filename = g_strdup_printf("%s.%s", temp,extension);
			g_free(temp);
		}else if(type&META_ARTIST_ART){
			filename = g_strdup_printf("artist_IMAGE.%s", extension);
		}else if (type&META_ARTIST_TXT){
			filename = g_strdup_printf("artist_BIOGRAPHY.%s", extension);
		}else if (type&META_SONG_TXT) {
			gchar *temp ;
			g_assert(song->title != NULL);
			temp =g_filename_from_utf8(song->title,-1,NULL,NULL,NULL); 
			filename = g_strdup_printf("%s_LYRIC.%s", temp,extension);
			g_free(temp);
		}else if (type&META_SONG_GUITAR_TAB) {
			gchar *temp ;
			g_assert(song->title != NULL);
			temp =g_filename_from_utf8(song->title,-1,NULL,NULL,NULL); 
			filename = g_strdup_printf("%s_GUITAR_TAB.%s", temp,extension);
			g_free(temp);
		}
		filename = strip_invalid_chars(filename);
		retv = g_build_path(G_DIR_SEPARATOR_S, homedir,"gmpc", "metadata", dirname,filename,NULL);
		if(filename) g_free(filename);
		if(dirname) g_free(dirname);
	}
	return retv;
}
static void metadata_pref_priority_changed(GtkCellRenderer *renderer, char *path, char *new_text, GtkListStore *store)
{
	GtkTreeIter iter;
	if(gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(store), &iter, path))
	{
		gmpcPluginParent *plug;
		gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, 0, &plug, -1);
		if(plug)
		{
			gmpc_plugin_metadata_set_priority(plug,(gint)g_ascii_strtoull(new_text, NULL, 0));
			gtk_list_store_set(GTK_LIST_STORE(store), &iter, 2, gmpc_plugin_metadata_get_priority(plug),-1);
			meta_data_sort_plugins();
		}
	}
}
/**
 * Get the enabled state directly from the plugin
 */
static void __column_data_func_enabled(GtkTreeViewColumn *column, 
										GtkCellRenderer *cell,
										GtkTreeModel *model,
										GtkTreeIter *iter,
										gpointer data)
{
		gmpcPluginParent *plug;
		gtk_tree_model_get(GTK_TREE_MODEL(model), iter, 0, &plug, -1);
		if(plug)
		{
			gboolean active = gmpc_plugin_get_enabled(plug);
			g_object_set(G_OBJECT(cell), "active", active, NULL);
		}
}
/**
 * Set enabled
 */
static void __column_toggled_enabled(GtkCellRendererToggle *renderer,
										char *path,
										gpointer store)
{
	GtkTreeIter iter;
	if(gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(store), &iter, path))
	{
		gmpcPluginParent *plug;
		gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, 0, &plug, -1);
		if(plug)
		{
			gboolean state = !gtk_cell_renderer_toggle_get_active(renderer);
			gmpc_plugin_set_enabled(plug,state);
			preferences_window_update();
		}
	}
}
static void metadata_construct_pref_pane(GtkWidget *container)
{
	GtkObject *adjustment;
	int i = 0;
	GtkCellRenderer *renderer;
	GtkWidget *vbox, *sw;
	GtkWidget *treeview;
	GtkWidget *label = NULL;
	GtkListStore *store = gtk_list_store_new(3, 
			G_TYPE_POINTER, /* The GmpcPlugin */
			G_TYPE_STRING, /* Name */
			G_TYPE_INT /* The priority */
			);
	gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (store),
			2, GTK_SORT_ASCENDING);


	/* Create vbox */
	vbox = gtk_vbox_new(FALSE, 6);
	/* tree + container */
	sw = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw), GTK_SHADOW_ETCHED_IN);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
	gtk_container_add(GTK_CONTAINER(sw), treeview);


	/* enable column */
	renderer = gtk_cell_renderer_toggle_new();
	gtk_tree_view_insert_column_with_data_func(GTK_TREE_VIEW(treeview),
			-1,
			"Enabled",
			renderer,
			(GtkTreeCellDataFunc)__column_data_func_enabled,
			NULL,
			NULL);
	g_object_set(G_OBJECT(renderer), "activatable", TRUE, NULL);
	g_signal_connect(G_OBJECT(renderer), "toggled" ,
			G_CALLBACK(__column_toggled_enabled), store);

	/* Build the columns */
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(treeview),
			-1,
			"Name",
			renderer,
			"text", 1,
			NULL);
	renderer = gtk_cell_renderer_spin_new();

	adjustment = gtk_adjustment_new (0, 0, 100, 1, 0, 0);
	g_object_set(G_OBJECT(renderer), "editable", TRUE, NULL);
	g_object_set (renderer, "adjustment", adjustment, NULL);
	g_signal_connect(G_OBJECT(renderer), "edited", G_CALLBACK(metadata_pref_priority_changed), store);
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(treeview),
			-1,
			"Priority",
			renderer,
			"text", 2,
			NULL);


	/* Add the list to the vbox */
	gtk_box_pack_start(GTK_BOX(vbox), sw, TRUE, TRUE, 0);

	gtk_container_add(GTK_CONTAINER(container), vbox);
	/* add plugins to list */
	for(i=0; i< meta_num_plugins;i++)
	{
		GtkTreeIter iter;
		gtk_list_store_insert_with_values(store, &iter, -1, 
				0, meta_plugins[i],
				1, gmpc_plugin_get_name(meta_plugins[i]),
				2, gmpc_plugin_metadata_get_priority(meta_plugins[i]),
				-1);
	}

	label = gtk_label_new("Plugins are evaluated from low priority to high");
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
	gtk_widget_show_all(container);
}
static void metadata_destroy_pref_pane(GtkWidget *container)
{
	GtkWidget *child = gtk_bin_get_child(GTK_BIN(container));
	if(child)
	{
		gtk_widget_destroy(child);
	}
}

void metadata_get_list_cancel(gpointer data)
{
}
gpointer metadata_get_list(mpd_Song  *song, MetaDataType type, void (*callback)(gpointer handle,const gchar *plugin_name, GList *list, gpointer data), gpointer data)
{
	callback(NULL, NULL, NULL, data);
	return NULL;
#if 0
	MLQuery *q = g_malloc0(sizeof(*q));
	q->cancel =FALSE;
	q->callback = callback;
	q->userdata = data;
	q->type = type;
	q->calls =1;
	/**
	 * Create a copy, so song is guarantee to be valid during queries of plugins
	 */
	q->song = mpd_songDup(song);
	/* Check cache */
	{
		MetaData *met = NULL;
		int retv = meta_data_get_from_cache(q->song, q->type,&met); 
		g_log("MetaData", G_LOG_LEVEL_DEBUG, "Queried cache: %i",retv); 
		if(retv == META_DATA_AVAILABLE)
		{
			GList *list = g_list_append(NULL, met);

			g_log("MetaData", G_LOG_LEVEL_DEBUG, "Callback");
			q->callback(q, met->plugin_name,list, q->userdata);
			g_log("MetaData", G_LOG_LEVEL_DEBUG, "Cleanup");
			g_list_foreach(g_list_first(list),(GFunc) meta_data_free, NULL);
			g_list_free(list);
			g_log("MetaData", G_LOG_LEVEL_DEBUG, "Cleanup done");
			met = NULL;
		}
		if(met)
			meta_data_free(met);
	}
	
	g_log("MetaData", G_LOG_LEVEL_DEBUG, "Start first itteration idle");
	g_idle_add(metadata_get_list_itterate_idle, q);
	return q;
#endif
}
/**
 * MetaData 
 */
MetaData *meta_data_new(void)
{
	/* Create a new structure completely filled with 0's */
	MetaData *retv = g_new0(MetaData, 1);
	retv->content_type = META_DATA_CONTENT_EMPTY;
	return retv;
}

void meta_data_free(MetaData *data)
{
	if(data == NULL) return;
	if(data->content) {
		if(data->content_type == META_DATA_CONTENT_TEXT_VECTOR)
		{
			g_strfreev(data->content);
		}
		else if (data->content_type == META_DATA_CONTENT_TEXT_LIST)
		{
			g_list_foreach((GList *)data->content, (GFunc)g_free, NULL);
			g_list_free((GList *)data->content);
		}
		else if(data->content_type != META_DATA_CONTENT_EMPTY)
			g_free(data->content);
		data->content = NULL;
		data->size = 0;
	}
	if(data->thumbnail_uri) g_free(data->thumbnail_uri);
	data->thumbnail_uri = NULL;
	g_free(data);
}

MetaData *meta_data_dup(MetaData *data)
{
	MetaData *retv = meta_data_new();
	g_assert(data != NULL);
	/* Copy type of metadata */
	retv->type = data->type;
	/* Copy the type of the data */
	retv->content_type = data->content_type;
	/* Copy the name of the providing plugin. (const char * so only copy pointer ) */
	retv->plugin_name = data->plugin_name;
	/* copy the content */
	retv->size = data->size;
	if(retv->content_type == META_DATA_CONTENT_TEXT_VECTOR) {
		if(data->content) retv->content =(void *) g_strdupv((gchar **)data->content);
	}
	/* raw data always needs a length */
	else if (data->content_type == META_DATA_CONTENT_RAW)
	{
		if(data->size > 0 ) {
			retv->content = g_memdup(data->content, (guint)data->size);
		}
	}
	else if (data->content_type == META_DATA_CONTENT_TEXT_LIST)
	{
		GList *list = NULL;
		GList *iter = g_list_first((GList *)(data->content));
		while((iter = g_list_next(iter)))
		{
			list = g_list_append(list, g_strdup(iter->data));
		}
		data->content =(void *) g_list_reverse(list);
	}
	else if (data->content_type == META_DATA_CONTENT_EMPTY)
	{
		retv->content = NULL; retv->size = 0;
	}
	/* Text is NULL terminated */
	else
	{
		retv->content = NULL;
		if(data->content){
			retv->content = g_strdup((gchar *)data->content);
		}
	}
	if(data->thumbnail_uri != NULL) {
		retv->thumbnail_uri = g_strdup(data->thumbnail_uri);
	}
	return retv;
}
MetaData *meta_data_dup_steal(MetaData *data)
{
	MetaData *retv = meta_data_new();
	g_assert(data != NULL);
	/* Copy type of metadata */
	retv->type = data->type;
	/* Copy the type of the data */
	retv->content_type = data->content_type;
	/* Copy the name of the providing plugin. (const char * so only copy pointer ) */
	retv->plugin_name = data->plugin_name;
	/* copy the content */
	retv->size = data->size;
	retv->content = data->content;
	data->size  = 0;
	data->content = NULL;
	retv->thumbnail_uri = data->thumbnail_uri;
	data->thumbnail_uri = NULL;
	return retv;
}
gboolean meta_data_is_empty(const MetaData *data)
{
	return data->content_type == META_DATA_CONTENT_EMPTY;
}
gboolean meta_data_is_uri(const MetaData *data)
{
	return data->content_type == META_DATA_CONTENT_URI;
}
const gchar * meta_data_get_uri(const MetaData *data)
{
	g_assert(meta_data_is_uri(data));
	return (const gchar *)data->content;
}
void meta_data_set_uri(MetaData *data, const gchar *uri)
{
	g_assert(meta_data_is_uri(data));
	if(data->content) g_free(data->content);
	data->content = g_strdup(uri);
}
void meta_data_set_raw(MetaData *item, guchar *data, gsize len)
{
	g_assert(meta_data_is_raw(item));
	if(item->content) g_free(item->content);
	item->content = g_memdup(data, len);
	item->size = len;
}
void meta_data_set_raw_owned(MetaData *item, guchar **data, gsize *len)
{
	g_assert(meta_data_is_raw(item));
	if(item->content) g_free(item->content);
	item->content = *data;
	*data = NULL;
	item->size = *len;
	*len = 0;
}
void meta_data_set_thumbnail_uri(MetaData *data, const gchar *uri)
{
	g_assert(meta_data_is_uri(data));
	if(data->thumbnail_uri) g_free(data->thumbnail_uri);
	data->thumbnail_uri = g_strdup(uri);
}
const gchar * meta_data_get_thumbnail_uri(const MetaData *data)
{
	g_assert(meta_data_is_uri(data));
	/* Only valid for images. */
	g_assert((data->type&(META_ALBUM_ART|META_ARTIST_ART)) != 0);

	return (const gchar *)data->thumbnail_uri;
}

gboolean meta_data_is_text(const MetaData *data)
{
	return data->content_type == META_DATA_CONTENT_TEXT;
}

void meta_data_set_text(MetaData *data, const gchar *text)
{
	if(meta_data_is_text(data))
	{
		if(data->content) g_free(data->content);
		data->content = g_strdup(text);
		data->size = -1;
	}
}
const gchar * meta_data_get_text(const MetaData *data)
{
	g_assert(meta_data_is_text(data));
	return (const gchar *)data->content;
}

gboolean meta_data_is_html(const MetaData *data)
{
	return data->content_type == META_DATA_CONTENT_HTML;
}
const gchar * meta_data_get_html(const MetaData *data)
{
	g_assert(meta_data_is_html(data));
	return (const gchar *)data->content;
}


/**
 * Convert a html encoded token (like &amp; and &#030;
 * to the corresponding unichar.
 *
 * On early falire (i.e. empty string) returns 0.
 * If the encoding is wrong, it returns the unicode error value 0xFFFC.
 */
static gunichar htmlname2unichar( const gchar* htmlname, gint len )
{
        // * can be optimized by sorting
        // * and checking str(n)cmp results
        static struct _TT {
                const gchar *str;
                gint utf8;
        }
        html2utf8_table[] = {
                { "quot", 34 },
                { "amp", 38 },
                { "lt", 60 },
                { "gt", 62 },
                { "nbsp", 160 },
                { "iexcl", 161 },
                { "cent", 162 },
                { "pound", 163 },
                { "curren", 164 },
                { "yen", 165 },
                { "brvbar", 166 },
                { "sect", 167 },
                { "uml", 168 },
                { "copy", 169 },
                { "ordf", 170 },
                { "laquo", 171 },
                { "not", 172 },
                { "shy", 173 },
                { "reg", 174 },
                { "macr", 175 },
                { "deg", 176 },
                { "plusmn", 177 },
                { "sup2", 178 },
                { "sup3", 179 },
                { "acute", 180 },
                { "micro", 181 },
                { "para", 182 },
                { "middot", 183 },
                { "cedil", 184 },
                { "sup1", 185 },
                { "ordm", 186 },
                { "raquo", 187 },
                { "frac14", 188 },
                { "frac12", 189 },
                { "frac34", 190 },
                { "iquest", 191 },
                { "Agrave", 192 },
                { "Aacute", 193 },
                { "Acirc", 194 },
                { "Atilde", 195 },
                { "Auml", 196 },
                { "Aring", 197 },
                { "AElig", 198 },
                { "Ccedil", 199 },
                { "Egrave", 200 },
                { "Eacute", 201 },
                { "Ecirc", 202 },
                { "Euml", 203 },
                { "Igrave", 204 },
                { "Iacute", 205 },
                { "Icirc", 206 },
                { "Iuml", 207 },
                { "ETH", 208 },
                { "Ntilde", 209 },
                { "Ograve", 210 },
                { "Oacute", 211 },
                { "Ocirc", 212 },
                { "Otilde", 213 },
                { "Ouml", 214 },
                { "times", 215 },
                { "Oslash", 216 },
                { "Ugrave", 217 },
                { "Uacute", 218 },
                { "Ucirc", 219 },
                { "Uuml", 220 },
                { "Yacute", 221 },
                { "THORN", 222 },
                { "szlig", 223 },
                { "agrave", 224 },
                { "aacute", 225 },
                { "acirc", 226 },
                { "atilde", 227 },
                { "auml", 228 },
                { "aring", 229 },
                { "aelig", 230 },
                { "ccedil", 231 },
                { "egrave", 232 },
                { "eacute", 233 },
                { "ecirc", 234 },
                { "euml", 235 },
                { "igrave", 236 },
                { "iacute", 237 },
                { "icirc", 238 },
                { "iuml", 239 },
                { "eth", 240 },
                { "ntilde", 241 },
                { "ograve", 242 },
                { "oacute", 243 },
                { "ocirc", 244 },
                { "otilde", 245 },
                { "ouml", 246 },
                { "divide", 247 },
                { "oslash", 248 },
                { "ugrave", 249 },
                { "uacute", 250 },
                { "ucirc", 251 },
                { "uuml", 252 },
                { "yacute", 253 },
                { "thorn", 254 },
                { "yuml", 255 },
                { "Alpha", 913 },
                { "alpha", 945 },
                { "Beta", 914 },
                { "beta", 946 },
                { "Gamma", 915 },
                { "gamma", 947 },
                { "Delta", 916 },
                { "delta", 948 },
                { "Epsilon", 917 },
                { "epsilon", 949 },
                { "Zeta", 918 },
                { "zeta", 950 },
                { "Eta", 919 },
                { "eta", 951 },
                { "Theta", 920 },
                { "theta", 952 },
                { "Iota", 921 },
                { "iota", 953 },
                { "Kappa", 922 },
                { "kappa", 954 },
                { "Lambda", 923 },
                { "lambda", 955 },
                { "Mu", 924 },
                { "mu", 956 },
                { "Nu", 925 },
                { "nu", 957 },
                { "Xi", 926 },
                { "xi", 958 },
                { "Omicron", 927 },
                { "omicron", 959 },
                { "Pi", 928 },
                { "pi", 960 },
                { "Rho", 929 },
                { "rho", 961 },
                { "Sigma", 931 },
                { "sigmaf", 962 },
                { "sigma", 963 },
                { "Tau", 932 },
                { "tau", 964 },
                { "Upsilon", 933 },
                { "upsilon", 965 },
                { "Phi", 934 },
                { "phi", 966 },
                { "Chi", 935 },
                { "chi", 967 },
                { "Psi", 936 },
                { "psi", 968 },
                { "Omega", 937 },
                { "omega", 969 },
                { "thetasym", 977 },
                { "upsih", 978 },
                { "piv", 982 },
                { "forall", 8704 },
                { "part", 8706 },
                { "exist", 8707 },
                { "empty", 8709 },
                { "nabla", 8711 },
                { "isin", 8712 },
                { "notin", 8713 },
                { "ni", 8715 },
                { "prod", 8719 },
                { "sum", 8721 },
                { "minus", 8722 },
                { "lowast", 8727 },
                { "radic", 8730 },
                { "prop", 8733 },
                { "infin", 8734 },
                { "ang", 8736 },
                { "and", 8869 },
                { "or", 8870 },
                { "cap", 8745 },
                { "cup", 8746 },
                { "int", 8747 },
                { "there4", 8756 },
                { "sim", 8764 },
                { "cong", 8773 },
                { "asymp", 8776 },
                { "ne", 8800 },
                { "equiv", 8801 },
                { "le", 8804 },
                { "ge", 8805 },
                { "sub", 8834 },
                { "sup", 8835 },
                { "nsub", 8836 },
                { "sube", 8838 },
                { "supe", 8839 },
                { "oplus", 8853 },
                { "otimes", 8855 },
                { "perp", 8869 },
                { "sdot", 8901 },
                { "loz", 9674 },
                { "lceil", 8968 },
                { "rceil", 8969 },
                { "lfloor", 8970 },
                { "rfloor", 8971 },
                { "lang", 9001 },
                { "rang", 9002 },
                { "larr", 8592 },
                { "uarr", 8593 },
                { "rarr", 8594 },
                { "darr", 8595 },
                { "harr", 8596 },
                { "crarr", 8629 },
                { "lArr", 8656 },
                { "uArr", 8657 },
                { "rArr", 8658 },
                { "dArr", 8659 },
                { "hArr", 8660 },
                { "bull", 8226 },
                { "hellip", 8230 },
                { "prime", 8242 },
                { "oline", 8254 },
                { "frasl", 8260 },
                { "weierp", 8472 },
                { "image", 8465 },
                { "real", 8476 },
                { "trade", 8482 },
                { "euro", 8364 },
                { "alefsym", 8501 },
                { "spades", 9824 },
                { "clubs", 9827 },
                { "hearts", 9829 },
                { "diams", 9830 },
                { "ensp", 8194 },
                { "emsp", 8195 },
                { "thinsp", 8201 },
                { "zwnj", 8204 },
                { "zwj", 8205 },
                { "lrm", 8206 },
                { "rlm", 8207 },
                { "ndash", 8211 },
                { "mdash", 8212 },
                { "lsquo", 8216 },
                { "rsquo", 8217 },
                { "sbquo", 8218 },
                { "ldquo", 8220 },
                { "rdquo", 8221 },
                { "bdquo", 8222 },
                { "dagger", 8224 },
                { "Dagger", 8225 },
                { "permil", 8240 },
                { "lsaquo", 8249 },
                { "rsaquo", 8250 },
                { NULL, 0 }
        };
        gint i;

        g_return_val_if_fail( NULL != htmlname, 0 );

        if( '\0' == *htmlname )
                return 0;

        if( '#' == *htmlname ) {
                const gchar *iter = htmlname;
                gunichar c = 0;
                if( 0 > len )
                        i = 7;
                else
                        i = len - 1;
                iter++;
                while( isdigit( *iter ) && ( 0 <= i ) ) {
                        c = c * 10 + (*iter - '0');
                        iter++;
                        i--;
                }

                if( 0 >= len ) {
                        if( '\0' == *iter )
                                return c;
                }
                else if( 0 == i )
                        return c;

                return 0;

        }

        if( 0 > len ) {
                for( i = 0; NULL != html2utf8_table[ i ].str; i++ )
                        if( 0 == strcmp( htmlname, html2utf8_table[ i ].str ) )
                                return html2utf8_table[ i ].utf8;
        }
        else if( 0 < len ) {
                for( i = 0; NULL != html2utf8_table[ i ].str; i++ )
                        if( 0 == strncmp( htmlname, html2utf8_table[ i ].str, len ) )
                                return html2utf8_table[ i ].utf8;
        }

        return 0xFFFC;
}

/**
 * Convert a string containing HTML encoded unichars to valid UTF-8.
 */
static gchar *htmlstr2utf8( const gchar* str )
{
        const gchar *amp_pos, *colon_pos, *copy_pos;
        gunichar uni = 0;
        GString *result;
        gsize len;

        if( NULL == str )
                return NULL;

        result = g_string_new( NULL );
        colon_pos = str;
        copy_pos = colon_pos;

        do {
                amp_pos = strchr( colon_pos, '&' );
                if( NULL == amp_pos )
                        break;
                colon_pos = amp_pos;

                len = 0;
                while( (';' != *colon_pos)
                        && ('\0' != *colon_pos) )
                {
                        colon_pos++;
                        len++;
                }

                if( (9 > len) && (2 < len) ) {
                        uni = htmlname2unichar( amp_pos + 1, colon_pos - amp_pos - 1 );
                        if( (0 != uni) && (TRUE == g_unichar_validate( uni )) ) {
                                g_string_append_len( result, copy_pos, amp_pos - copy_pos );
                                colon_pos++;
                                copy_pos = colon_pos;
                                g_string_append_unichar( result, uni );
                        }
                }
                amp_pos = colon_pos;
        }
        while( NULL != amp_pos );

        if( NULL != copy_pos )
                g_string_append( result, copy_pos );

        return g_string_free( result, FALSE );
}

static gchar * strip_tags(gchar *html)
{
	gsize i = 0,j=0;
	unsigned depth = 0;
	while(html[i] != '\0') {
		if(html[i] == '<') depth++;
		else if(html[i] == '>') depth--;	
		else if(depth == 0) {
			html[j] = html[i];
			j++;
		}
		i++;
	}
	html[j] = '\0';
	return html;
}

gchar * meta_data_get_text_from_html(const MetaData *data)
{
	gchar *retv;
	g_assert(meta_data_is_html(data));
	retv = htmlstr2utf8((gchar *)data->content);
	/* need to strip tags */
	retv = strip_tags(retv);
	return retv;
}
gboolean meta_data_is_raw(const MetaData *data)
{
	return data->content_type == META_DATA_CONTENT_RAW;
}

const guchar * meta_data_get_raw(const MetaData *data, gsize *length)
{
	g_assert(meta_data_is_raw(data));
	if(length)
		*length = data->size;
	return (guchar *)data->content;
}
gboolean meta_data_is_text_vector(const MetaData *data)
{
	return data->content_type == META_DATA_CONTENT_TEXT_VECTOR;
}
const gchar ** meta_data_get_text_vector(const MetaData *data)
{
	g_assert(meta_data_is_text_vector(data));
	return (const gchar **)data->content;
}

gboolean meta_data_is_text_list(const MetaData *data)
{
	return data->content_type == META_DATA_CONTENT_TEXT_LIST;
}
const GList *meta_data_get_text_list(const MetaData *data)
{
	g_assert(meta_data_is_text_list(data));
	return (const GList *)data->content;
}
/**
 * Plugin structure
 */

gmpcPrefPlugin metadata_pref_plug = {
	.construct      = metadata_construct_pref_pane,
	.destroy        =  metadata_destroy_pref_pane
};
gmpcPlugin metadata_plug = {
	.name           = N_("Metadata Handler"),
	.version        = {1,1,1},
	.plugin_type    = GMPC_INTERNALL,
};

/* vim: set noexpandtab ts=4 sw=4 sts=4 tw=120: */
