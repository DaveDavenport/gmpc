/* Gnome Music Player Client (GMPC)
 * Copyright (C) 2004-2012 Qball Cow <qball@gmpclient.org>
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
#include "preferences.h"

#define LOG_DOMAIN "MetaData"

#include <glyr/glyr.h>
#include <glyr/cache.h>

/**
 * GLYR
 */
static GAsyncQueue  *gaq           = NULL;
static GAsyncQueue  *return_queue  = NULL;
static GlyrDatabase *db            = NULL;


/**
 * This queue is used to send replies back.
 */
enum MTD_Action {
	MTD_ACTION_QUERY_METADATA,
	MTD_ACTION_CLEAR_ENTRY,
	MTD_ACTION_SET_ENTRY,
	MTD_ACTION_QUERY_LIST,
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

	/* Original (for sending the right signal) */
	mpd_Song *ori_song;

	/* The type of metadata */
	MetaDataType type;
	/* Result  */
	MetaDataResult result;
	/* The actual result data */
	MetaData *met;
	GList	*met_results;
} meta_thread_data;


// Validate if enough information is available for the query.
static gboolean meta_data_validate_query(mpd_Song *tsong, MetaDataType type)
{
	switch(type&META_QUERY_DATA_TYPES)
	{
		case META_GENRE_SIMILAR:
			if(tsong->genre == NULL || tsong->genre[0] == '\0')
				return FALSE;
			break;
		case META_SONG_GUITAR_TAB:
		case META_SONG_TXT:
		case META_SONG_SIMILAR:
			if(tsong->title == NULL || tsong->title[0] == '\0')
				return FALSE;
			if(tsong->artist == NULL || tsong->artist[0] == '\0')
				return FALSE;
			break;
		case META_ALBUM_ART:
		case META_ALBUM_TXT:
			if(tsong->album == NULL || tsong->album[0] == '\0')
				return FALSE;
		case META_ARTIST_TXT:
		case META_ARTIST_SIMILAR:
		case META_BACKDROP_ART:
		case META_ARTIST_ART:
			if(tsong->artist == NULL|| tsong->artist[0] == '\0')
				return FALSE;
			break;
		// other items.
		case META_QUERY_DATA_TYPES:
		case META_QUERY_NO_CACHE:
		default:
			return FALSE;

	}
	return TRUE;
}


static void meta_thread_data_free(meta_thread_data *mtd)
{
	/* Free the result data */
	if(mtd->met)
		meta_data_free(mtd->met);
	if(mtd->met_results) {
		g_list_foreach(mtd->met_results, (GFunc)meta_data_free, NULL);
		g_list_free(mtd->met_results);
		mtd->met_results = NULL;
	}
	/* Free the copie and edited version of the songs */
	if(mtd->song)
		mpd_freeSong(mtd->song);

	if(mtd->ori_song)
		mpd_freeSong(mtd->ori_song);

	/* Free the Request struct */
	g_slice_free(meta_thread_data, mtd);
}
gboolean meta_compare_func(meta_thread_data *mt1, meta_thread_data *mt2);
//static gboolean meta_data_handle_results(void);

mpd_Song *rewrite_mpd_song(mpd_Song *tsong, MetaDataType type, gboolean query_mpd)
{
	mpd_Song *edited = NULL;
	/* If it is not a mpd got song */
	if(tsong->file == NULL  && query_mpd)
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
		else if(type&(META_ARTIST_ART|META_ARTIST_TXT) && tsong->artist)
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
		else if(edited->album && edited->file && query_mpd)
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
	*/
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
		printf("Process results: %i\n", mtd->action);
		if(mtd->action == MTD_ACTION_QUERY_METADATA)
		{
			gmpc_meta_watcher_data_changed(gmw,mtd->ori_song, (mtd->type)&META_QUERY_DATA_TYPES, mtd->result,mtd->met);
			if(mtd->callback)
			{
				mtd->callback(mtd->ori_song, mtd->result, mtd->met, mtd->data);
			}
		} else if (mtd->action == MTD_ACTION_QUERY_LIST)
		{
			if(mtd->callback){
				MetaDataListCallback cb = (MetaDataListCallback)mtd->callback;
				// TODO: Update callback for current usecase.
				cb(NULL, "", mtd->met_results, mtd->data);
				// Send done.
				cb(NULL, "", NULL, mtd->data);
			}		
		}else if (mtd->action == MTD_ACTION_CLEAR_ENTRY) {
			printf("Signal no longer available.\n");
			// Signal that this item is now no longer available.
			gmpc_meta_watcher_data_changed(gmw, mtd->ori_song, (mtd->type)&META_QUERY_DATA_TYPES, META_DATA_UNAVAILABLE, NULL);
		}

		meta_thread_data_free(mtd);
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

	glyr_opt_parallel(query, 1);
	glyr_opt_number(query, 1);
	// timeout 5 seconds.
	glyr_opt_timeout(query, 5);
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
/**
 * Convert cache to MetaData object.
 */
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
                    mtd->plugin_name = g_strdup(cache->prov); 
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

/**
 * Convert cache to MetaData object.
 */
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
                    mtd->plugin_name = g_strdup(cache->prov); 
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
	// If more then one time called and we allready have a results
	// Push it in the results list.
	if(mtd->met != NULL) {
		mtd->met_results = g_list_prepend(mtd->met_results, (void*)mtd->met);
	}
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
			if(cache->cached)
			{
				(mtd->met)->plugin_name = g_strdup_printf("%s (cached)", cache->prov); 
			}else{
				(mtd->met)->plugin_name = g_strdup(cache->prov); 
			}
			(mtd->met)->content_type = content_type;

			// Steal the data.
			mtd->met->content = cache->data;
			cache->data = NULL;
			(mtd->met)->size = cache->size;
			cache->size = 0;
			mtd->result = META_DATA_AVAILABLE;
			// found something.
			retv = TRUE;
		}
		memcpy(&(mtd->met->md5sum), &(cache->md5sum), 16);
	}else { 
		// Explicitely not found.
		printf("Cache sais empty\n");
		retv = TRUE;
	}
	return retv;
}

static GlyrQuery *glyr_exit_handle = NULL;
static GStaticMutex exit_handle_lock = G_STATIC_MUTEX_INIT;

/**
 * Load a file from an URI
 * 
 * @param mtd A meta_thread_data.
 * 
 * Loads a file from a hard-drive. 
 *
 * @returns nothing.
 */
static GlyrMemCache *glyr_fetcher_thread_load_uri(meta_thread_data *mtd)
{
	GlyrMemCache *cache = NULL;
	const char *path = meta_data_get_uri(mtd->met);
	gchar *scheme = g_uri_parse_scheme(path);

	if(scheme == NULL || strcmp(scheme, "file") == 0)
	{
		char *content = NULL;
		gsize length =0;
		g_file_get_contents(path, &content,&length, NULL); 
		// set it to raw.
		mtd->met->content_type = META_DATA_CONTENT_RAW;
		cache = glyr_cache_new();
		cache->dsrc = g_strdup(path);
		glyr_cache_set_data(cache, content, length);


		// Clean old content.
		g_free(mtd->met->content);
		mtd->met->content = g_memdup(content, length);
		mtd->met->size = length;
		content = NULL;


		// Testing force image type.
		if(mtd->met->type == META_ALBUM_ART || mtd->met->type == META_ARTIST_ART ||
			mtd->met->type == META_BACKDROP_ART)
		{
			cache->is_image = TRUE;
			cache->img_format = g_strdup("jpeg");	
		}
		memcpy(&(mtd->met->md5sum), &(cache->md5sum), 16);

	}
	g_free(scheme);
	return cache;
}

static GlyrMemCache *glyr_fetcher_thread_load_raw(meta_thread_data *mtd)
{
	GlyrMemCache *cache = NULL;
	cache = glyr_cache_new();
	glyr_cache_set_data(cache, 
			g_memdup(mtd->met->content, mtd->met->size), 
			mtd->met->size);
	// Testing force image type.
	if(mtd->met->type == META_ALBUM_ART || mtd->met->type == META_ARTIST_ART ||
			mtd->met->type == META_BACKDROP_ART)
	{
		cache->is_image = TRUE;
		cache->img_format = g_strdup("jpeg");	
	}
	memcpy(mtd->met->md5sum, cache->md5sum, 16);
	return cache;
}

static GlyrMemCache *glyr_fetcher_thread_load_text(meta_thread_data *mtd)
{
	GlyrMemCache *cache = NULL;
	cache = glyr_cache_new();
	glyr_cache_set_data(cache, 
			g_strdup(mtd->met->content), 
			-1);
	memcpy(mtd->met->md5sum, cache->md5sum, 16);
	return cache;
}
/**
 * Thread that does the GLYR requests
 */
static void glyr_fetcher_thread(void *user_data)
{
	void *d;
	GlyrQuery query;

	g_static_mutex_lock(&exit_handle_lock);
	while((d = g_async_queue_pop(gaq)))
	{
		meta_thread_data    *mtd         = (meta_thread_data*)d;

		// Check if this is the quit command.
		if(mtd->action == MTD_ACTION_QUIT) {
			printf("Quitting....");
			g_static_mutex_unlock(&exit_handle_lock);
			/* Free the Request struct */
			meta_thread_data_free(mtd);		
			return;
		}
		else if (mtd->action == MTD_ACTION_CLEAR_ENTRY)
		{
			GlyrMemCache        *cache       = NULL;

			// Setup cancel lock
			glyr_exit_handle = &query;
			g_static_mutex_unlock(&exit_handle_lock);

			/* Set up the query */
			glyr_query_init(&query);
			setup_glyr_query(&query, mtd);
			glyr_opt_number(&query, 0);
			/* Set some random settings */
			glyr_opt_verbosity(&query,4);

			// Delete existing entries.
			glyr_db_delete(db, &query);

			// Set dummy entry in cache, so we know
			// we searched for this before.
			cache = glyr_cache_new();
			// TODO: Remove this randomize hack.
			glyr_cache_set_data(cache,
					g_strdup("GMPC Dummy"),
					-1);
			cache->rating = -1;

			// Add dummy entry
			printf("Inserting dummy item\n");
			glyr_db_insert(db,&query, cache);

			// Cleanup
			if(cache)glyr_free_list(cache);

			// Clear the query, and lock the handle again.
			g_static_mutex_lock(&exit_handle_lock);
			glyr_exit_handle = NULL;
			glyr_query_destroy(&query);

			printf("Push back result\n");
			// Push back result, and tell idle handle to handle it.
			g_async_queue_push(return_queue, mtd);
			// invalidate pointer.
			mtd = NULL;
			// Schedule the result thread in idle time.
			g_idle_add(glyr_return_queue, NULL);
		}
		else if (mtd->action == MTD_ACTION_QUERY_LIST)
		{
			// Check if this is thread safe.
			const char 			*md			 = connection_get_music_directory();
			GLYR_ERROR          err          = GLYRE_OK;
			MetaDataContentType content_type = META_DATA_CONTENT_RAW;
			GlyrMemCache        *cache       = NULL;

			glyr_exit_handle = &query;
			g_static_mutex_unlock(&exit_handle_lock);

			printf("new style query\n");

			glyr_query_init(&query);


			if(md != NULL && md[0] !=  '\0'&& mtd->song->file != NULL)
			{
				char *path = g_build_filename(md, mtd->song->file, NULL);
				glyr_opt_musictree_path(&query, path);
				g_free(path);
			}


			/* Set up the query */
			content_type = setup_glyr_query(&query, mtd);

			/* Set some random settings */
			glyr_opt_verbosity(&query,3);

			/* Tell libglyr to automatically lookup before searching the web */
			glyr_opt_lookup_db(&query, db);

			/* Also tell it not to write newly found items to the db */
			glyr_opt_db_autowrite(&query, FALSE);
			/* We want many results */
			glyr_opt_number(&query, 25);

			/* get metadata */
			cache = glyr_get(&query,&err,NULL);

			if(cache != NULL)
			{
				GlyrMemCache *iter = cache;
				while(iter){
					process_glyr_result(iter,content_type, mtd);
					iter = iter->next;
				}
				// Done. (most last item into list)
				process_glyr_result(NULL,content_type, mtd);
			}
			// Cleanup
			if(cache)glyr_free_list(cache);

			// Clear the query, and lock the handle again.
			g_static_mutex_lock(&exit_handle_lock);
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
			g_static_mutex_unlock(&exit_handle_lock);

			printf("new style query\n");

			glyr_query_init(&query);


			if(md != NULL && md[0] !=  '\0'&& mtd->song->file != NULL)
			{
				char *path = g_build_filename(md, mtd->song->file, NULL);
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
				// TODO: Remove this randomize hack.
				glyr_cache_set_data(cache,
						g_strdup("GMPC Dummy"),
						-1);
				cache->rating = -1;

				glyr_db_insert(db,&query, cache);
				printf("Cache is Empty\n");
				// Set unavailable 
				mtd->result = META_DATA_UNAVAILABLE; 
			}else{
				process_glyr_result(cache,content_type, mtd);
			}
			// Cleanup
			if(cache)glyr_free_list(cache);

			// Clear the query, and lock the handle again.
			g_static_mutex_lock(&exit_handle_lock);
			glyr_exit_handle = NULL;
			glyr_query_destroy(&query);

			// Push back result, and tell idle handle to handle it.
			g_async_queue_push(return_queue, mtd);
			// invalidate pointer.
			mtd = NULL;
			// Schedule the result thread in idle time.
			g_idle_add(glyr_return_queue, NULL);
		}
		else if (mtd->action == MTD_ACTION_SET_ENTRY)
		{
			GlyrMemCache        *cache       = NULL;

			// Setup cancel lock
			glyr_exit_handle = &query;
			g_static_mutex_unlock(&exit_handle_lock);

			/* Set up the query */
			glyr_query_init(&query);
			setup_glyr_query(&query, mtd);
			glyr_opt_number(&query, 0);
			/* Set some random settings */
			glyr_opt_verbosity(&query,3);

			// Delete existing entries.
			glyr_db_delete(db, &query);

			glyr_query_init(&query);
			setup_glyr_query(&query, mtd);
			glyr_opt_number(&query, 0);
			/* Set some random settings */
			glyr_opt_verbosity(&query,3);
			// load data
			if(meta_data_is_uri(mtd->met))
			{
				// try to load cache from file.
				cache = glyr_fetcher_thread_load_uri(mtd);
			}
			else if (meta_data_is_raw(mtd->met))
			{
				// try to load cache from raw.
				cache = glyr_fetcher_thread_load_raw(mtd);
			} else if (meta_data_is_text(mtd->met) || meta_data_is_html(mtd->met))
			{
				// try to load cache from text.
				cache = glyr_fetcher_thread_load_text(mtd);
			}	

			// Cache.
			if(cache)
			{
				cache->rating = 9;
				printf("Do DB insert\n");
				glyr_db_insert(db,&query, cache);
			}

			// Clear the query, and lock the handle again.
			g_static_mutex_lock(&exit_handle_lock);
			glyr_exit_handle = NULL;
			glyr_query_destroy(&query);

			// Push back result, and tell idle handle to handle it.
			// set it to query metadata to get the right handle behaviour.
			mtd->action = MTD_ACTION_QUERY_METADATA; 
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
		
	//g_mutex_init(&exit_handle_lock);
	/* Initialize..*/
	printf("open glyr db: %s\n", url);
	glyr_init();
	db = glyr_db_init(url);
	g_free(url);


	gaq = g_async_queue_new();
	return_queue = g_async_queue_new();
#if GLIB_CHECK_VERSION(2, 31, 0)
	gaq_fetcher_thread = g_thread_new("Glyr thread fetch", (GThreadFunc)glyr_fetcher_thread, NULL);
#else
	gaq_fetcher_thread = g_thread_create(glyr_fetcher_thread, NULL, TRUE, NULL);
#endif

}

/**
 * TODO: Can we guarantee that all the downloads are stopped? 
 */
void meta_data_destroy(void)
{
	meta_thread_data *mtd = NULL;
	/**
 	 * Clear the request queue, and tell thread to quit 
	 */
	g_async_queue_lock(gaq);
	while((mtd = g_async_queue_try_pop_unlocked(gaq))){
		/* Free */	
		meta_thread_data_free(mtd);
	}
	mtd = g_slice_new0(meta_thread_data);//g_malloc0(sizeof(*mtd));
	mtd->action = MTD_ACTION_QUIT;
	g_async_queue_push_unlocked(gaq, mtd);
	mtd = NULL;
	g_async_queue_unlock(gaq);
	// add lock? 
	g_static_mutex_lock(&exit_handle_lock);
	if(glyr_exit_handle) {
		printf("Sending quit signal\n");
		glyr_signal_exit(glyr_exit_handle);
	}
	g_static_mutex_unlock(&exit_handle_lock);

	printf("Waiting for glyr to finish.....\n");
	g_thread_join(gaq_fetcher_thread);
	//g_mutex_clear(&exit_handle_lock);

	glyr_db_destroy(db);
	glyr_cleanup();
	/**
 	 * Wait for thread to quit 
	 */
	g_async_queue_lock(return_queue);
	while((mtd = g_async_queue_try_pop_unlocked(return_queue))){
		 /* Free any possible plugin results */
		meta_thread_data_free(mtd);
	}
	g_async_queue_unlock(return_queue);
	g_async_queue_unref(gaq);
	g_async_queue_unref(return_queue);

}
gboolean meta_compare_func(meta_thread_data *mt1, meta_thread_data *mt2)
{
	if(mt1->action != mt2->action) return TRUE;
	
	if((mt1->type&META_QUERY_DATA_TYPES) != (mt2->type&META_QUERY_DATA_TYPES))
		return TRUE;
	if(!gmpc_meta_watcher_match_data(mt1->type&META_QUERY_DATA_TYPES, mt1->song, mt2->song))
	{
		return TRUE;
	}
	return FALSE;
}


static guint meta_data_thread_data_uid = 0;

void meta_data_set_entry ( mpd_Song *song, MetaData *met )
{
	meta_thread_data *mtd = NULL; 
	if(song == NULL || met == NULL || !meta_data_validate_query(song, met->type)) 
	{
		g_warning("Trying to set metadata entry with insufficient information");
		return;	
	}

	mtd = g_slice_new0(meta_thread_data);
	mtd->action = MTD_ACTION_SET_ENTRY;
	mtd->id = ++meta_data_thread_data_uid;
	/* Create a copy of the original song */
	mtd->song = rewrite_mpd_song(song, met->type, TRUE);
	mtd->ori_song = mpd_songDup(song);
	/* Set the type */
	mtd->type = met->type;
	/* set result NULL */
	mtd->met = meta_data_dup(met);;
	/* signal we are fetching. */
	gmpc_meta_watcher_data_changed(gmw,mtd->ori_song, (mtd->type)&META_QUERY_DATA_TYPES, META_DATA_FETCHING,NULL);
	/* Set entry */
	printf("Request setting entry\n");
	g_async_queue_push(gaq, mtd);
	mtd = NULL;
}

void meta_data_clear_entry(mpd_Song *song, MetaDataType type)
{
	meta_thread_data *mtd = NULL; 
	if(!meta_data_validate_query(song, type))
	{
		g_warning("Trying to clear metadata entry with insufficient information");
		return;	
	} 
	mtd = g_slice_new0(meta_thread_data);
	mtd->action = MTD_ACTION_CLEAR_ENTRY;
	mtd->id = ++meta_data_thread_data_uid;
	/* Create a copy of the original song */
	mtd->song = rewrite_mpd_song(song, type,TRUE);
	mtd->ori_song = mpd_songDup(song);
	/* Set the type */
	mtd->type = type;
	/* set result NULL */
	mtd->met = NULL;
	printf("Request clearing entry\n");
	g_async_queue_push(gaq, mtd);
	mtd = NULL;
}

/**
 * Function called by the "client" 
 */

MetaDataResult meta_data_get_path(mpd_Song *tsong, MetaDataType type, MetaData **met,MetaDataCallback callback, gpointer data)
{
	meta_thread_data *mtd = NULL;

	if(!meta_data_validate_query(tsong, type)) 
	{
		printf("Query invalid");
		*met = NULL;
		return META_DATA_UNAVAILABLE;
	}

    mtd = g_slice_new0(meta_thread_data);
	mtd->action = MTD_ACTION_QUERY_METADATA;
	mtd->id = ++meta_data_thread_data_uid;
	/* Create a copy of the original song */
	mtd->song = rewrite_mpd_song(tsong, type, FALSE);
	mtd->ori_song = mpd_songDup(tsong);
	/* Set the type */
	mtd->type = type;
	/* the callback */
	mtd->callback = callback;
	/* the callback data */
	mtd->data = data;
	/* Set that we are fetching */
	mtd->result = META_DATA_FETCHING;
	/* set result NULL */
	mtd->met = NULL;
	/**
     * If requested query the cache first 
	 */
	if((type&META_QUERY_NO_CACHE) == 0)
	{
		MetaDataResult mrd;
		MetaDataContentType content_type = META_DATA_CONTENT_RAW;
		GlyrQuery query;
		GlyrMemCache * cache = NULL;

		glyr_query_init(&query);
		/* Set some random settings */
		glyr_opt_verbosity(&query,3);

		content_type = setup_glyr_query(&query, mtd);
		cache = glyr_db_lookup(db, &query);
		if(process_glyr_result(cache,content_type, mtd))
		{
			// Cleanup
			if(cache)glyr_free_list(cache);
			glyr_query_destroy(&query);
			
			mrd = mtd->result;
			*met = mtd->met;
			mtd->met = NULL;
			// Free mtd
			meta_thread_data_free(mtd);

			return mrd;
		}
		if(cache)glyr_free_list(cache);
		glyr_query_destroy(&query);
	}
	else
	{
		printf("signal fetching\n");
		gmpc_meta_watcher_data_changed(gmw,mtd->ori_song, (mtd->type)&META_QUERY_DATA_TYPES, META_DATA_FETCHING,NULL);
		if(mtd->callback)
		{
			mtd->callback(mtd->ori_song, META_DATA_FETCHING, NULL, mtd->data);
		}
	}

	// Rewrite for query 
	if(mtd->song != NULL) mpd_freeSong(mtd->song);
	mtd->song = rewrite_mpd_song(tsong, type, TRUE);


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
		if(dirname == NULL)
		{
			const gchar *charset;
			g_get_charset (&charset);
			dirname = g_convert_with_fallback (song->artist, -1,
					charset, "UTF-8",(char *)"-", NULL, NULL, &error);
		}
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
/**
 * Set enabled
 */

void metadata_get_list_cancel(gpointer data)
{
}

gpointer metadata_get_list(mpd_Song  *song, MetaDataType type, void (*callback)(gpointer handle,const gchar *plugin_name, GList *list, gpointer data), gpointer data)
{
	meta_thread_data *mtd = NULL;
	
	mtd = g_slice_new0(meta_thread_data);
	mtd->action = MTD_ACTION_QUERY_LIST;
	mtd->id = ++meta_data_thread_data_uid;
	/* Create a copy of the original song */
	mtd->song = rewrite_mpd_song(song, type, TRUE);
	mtd->ori_song = mpd_songDup(song);
	/* Set the type */
	mtd->type = type;
	/* the callback */
	mtd->callback = callback;
	/* the callback data */
	mtd->data = data;
	/* Set that we are fetching */
	mtd->result = META_DATA_FETCHING;
	/* set result NULL */
	mtd->met = NULL;
	mtd->met_results = NULL;
	printf("start query\n");

	g_async_queue_push(gaq, mtd);
	mtd = NULL;
	return NULL;
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
	if(data->plugin_name) g_free(data->plugin_name);

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
	retv->plugin_name = g_strdup(data->plugin_name);
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

	memcpy(&(retv->md5sum),&(data->md5sum), 16); 

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
	retv->plugin_name = g_strdup(data->plugin_name);
	/* copy the content */
	retv->size = data->size;
	retv->content = data->content;
	data->size  = 0;
	data->content = NULL;
	retv->thumbnail_uri = data->thumbnail_uri;
	data->thumbnail_uri = NULL;

	memcpy(&(retv->md5sum),&(data->md5sum), 16); 

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

gmpcPlugin metadata_plug = {
	.name           = N_("Metadata Handler"),
	.version        = {1,1,1},
	.plugin_type    = GMPC_INTERNALL,
};

/* vim: set noexpandtab ts=4 sw=4 sts=4 tw=120: */
