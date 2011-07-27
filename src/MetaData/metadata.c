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
#include "metadata-cache.h"
#include "preferences.h"

#include "gmpc_easy_download.h"

#define LOG_DOMAIN "MetaData"

int meta_num_plugins=0;
gmpcPluginParent **meta_plugins = NULL;
static void meta_data_sort_plugins(void);
GList *process_queue = NULL;
/**
 * This queue is used to send replies back.
 */
GQueue *meta_results = NULL;


/**
 * Structure holding a metadata query */
typedef struct {
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
	/* The index of the plugin being queried */
	int index;
	int do_rename;
	int rename_done;
	/* List with temporary result from plugin index */
	GList *list;
	/* The current position in the list */
	GList *iter;
} meta_thread_data;

gboolean meta_compare_func(meta_thread_data *mt1, meta_thread_data *mt2);
static gboolean meta_data_handle_results(void);


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
 * Initialize
 */
void meta_data_init(void)
{
	g_assert(meta_results == NULL );

	metadata_cache_init();

	/**
	 * the result queue
	 */
	meta_results = g_queue_new();

	/**
	 * Create the retrieval thread
	 */
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
		metadata_cache_cleanup();
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

/**
 * new things *
 */
static int counter = 0;
static gboolean process_itterate(void);
/**
 * This function actually propagets the result.
 * It is called (during idle time) when a request is done.
 * The queue might hold one or more items 
 */

static gboolean meta_data_handle_results(void)
{
	meta_thread_data *data = NULL;
	/**
	 *  Check if there are results to handle
	 *  do this until the list is clear
	 */
	while((data = g_queue_pop_tail(meta_results)))
	{	
		/* Signal the result to the gmpc-meta-watcher object, any listening client is interrested in the result. */
		gmpc_meta_watcher_data_changed(gmw,data->song, (data->type)&META_QUERY_DATA_TYPES, data->result,data->met);
		/* If a specific callback is set, call that  */
		if(data->callback)
		{
			data->callback(data->song,data->result,data->met, data->data);
		}
		/**
		 * Start cleaning up the meta_thead_data 
		 */
		 /* Free any possible plugin results */
		if(data->list){
			g_list_foreach(data->list, (GFunc)meta_data_free, NULL);
			g_list_free(data->list);
		}
		/* Free the result data */
		if(data->met)
			meta_data_free(data->met);
		/* Free the copie and edited version of the songs */
		if(data->song)
			mpd_freeSong(data->song);
		if(data->edited)
			mpd_freeSong(data->edited);

		/* Free the Request struct */
		g_free(data);
	}
	/* Make the idle handler stop */
	return FALSE;
}

/**
 * This function handles it when data needs to be downloaded (in the cache) for now that are images.
 */
/* TODO REMOVE */
static void metadata_download_handler(const GEADAsyncHandler *handle, GEADStatus status, gpointer user_data)
{
	meta_thread_data *d = user_data;

	if(status == GEAD_PROGRESS) {
		/* If we are still downloading, do nothing */
		return;
	}
	counter --;
	if(status == GEAD_DONE)
	{
		/* If success, start processing  result */
		/* Get filename where we need to store the result. */
		gchar *filename = gmpc_get_metadata_filename(d->type&(~META_QUERY_NO_CACHE), d->edited, NULL);
		goffset length;
		/* Get the data we downloaded */
		const gchar *data = gmpc_easy_handler_get_data(handle, &length);
		GError *error = NULL;
		/* Try to store the data in the file */
		if(length > 0)
		{
			g_file_set_contents(filename, data, length, &error);
			if(error == NULL)
			{
				MetaData *md = d->iter->data;
				/* Set result successs */
				d->result = META_DATA_AVAILABLE;
				/* If success create a MetaData object */
				/* Create Metadata */
				d->met = meta_data_new();
				d->met->type = md->type;
				d->met->plugin_name = md->plugin_name;
				d->met->content_type = META_DATA_CONTENT_URI;
				d->met->content = filename;
				d->met->size = -1;
				/* Convert ownership of string to d->met */
				filename = NULL;
				/* Free any remaining results */ 
				g_list_foreach(d->list,(GFunc) meta_data_free, NULL);
				g_list_free(d->list);
				/* Set to NULL */
				d->list = NULL;
				d->iter = NULL;
				/* by setting index to the last, the process_itterate knows it should not look further */
				d->index = meta_num_plugins;
				/* Iterate the */

				g_idle_add((GSourceFunc)process_itterate, NULL);
				//process_itterate();
				/* we trown it back, quit */
				return;
			}
		}

		if(error){
			/* Ok we failed to store it, now we cleanup, and try the next entry. */
			g_log(LOG_DOMAIN, G_LOG_LEVEL_WARNING, "Failed to store file: %s: '%s'", filename, error->message);
			g_error_free(error); error = NULL;
		}
		else
			g_log(LOG_DOMAIN, G_LOG_LEVEL_WARNING, "Failed to store file: %s: '%i'", gmpc_easy_handler_get_uri(handle),(int)length);
		/* If we fail, clean up and try next one */
		g_free(filename);
	}
	/**
	 * It seems we somehow _failed_ to get data, lets try the next result
	 */
	if(d->iter)
	{
		/* If there is a result entry, advanced it to  the next entry.*/
		d->iter = g_list_next(d->iter);
		if(d->iter){
			/* Get the MetaData belonging to this result */
			MetaData *md = (MetaData *)d->iter->data;
			/* If it is an uri, we probly want to make a local copy of it. */
			if(meta_data_is_uri(md))
			{
				const char *path = meta_data_get_uri(md);// d->iter->data;
				if(path && path[0] == '/') {
					/* if it is allready a local copy, store that */ 
					/* set the result as final result */
					d->met = md;
					/* Remove it from the result list, so it does not get deleted twice */
					d->iter->data= NULL;
					/* set result available */
					d->result = META_DATA_AVAILABLE;
				}else if(path) {
					/* Setup a new async downloader */

					GEADAsyncHandler *new_handle = gmpc_easy_async_downloader((const gchar *)path, 
							metadata_download_handler, d);
					/* If it is a remote one, download it */ 
					if(new_handle != NULL)
					{
						/* if it is a success nothing left todo until the result comes in */
						counter++;
						return;
					}
				}
			}else {
				/* if it is not something we need to download, se the result as the final result */
				d->result = META_DATA_AVAILABLE;
				d->met = md;
				/* remove result from list */
				d->iter->data= NULL;
			}	

		}
	}
	/* There are no more items to query, or we have a hit.
	 * Cleanup the results by this plugin and advanced to the next 
	 */
	if(d->list){
		g_list_foreach(d->list,(GFunc) meta_data_free, NULL);
		g_list_free(d->list);
	}
	d->list = NULL;
	d->iter = NULL;
	/* if we have a result, skip the rest of the plugins */
	if(d->result == META_DATA_AVAILABLE)
		d->index = meta_num_plugins;
	else 
		d->index++;
	/* itterate the next step */
	g_idle_add((GSourceFunc)process_itterate, NULL);
	//process_itterate();
}
/**
 * this function handles the result from the plugins
 */
static void result_itterate(GList *list, gpointer user_data)
{
	MetaData *md=NULL;
	meta_thread_data *d = (meta_thread_data *)user_data;

	/**
	 * This plugin didn't have a result
	 * Skip to next plugin, 
	 * retry.
	 **/
	if(list == NULL){
		d->index++;
		/* retry */

		g_idle_add((GSourceFunc)process_itterate, NULL);
		//process_itterate();
		return;
	}

	/**
	 * Plugin has a result, try the downloading the first, if that fails
	 */
	d->list = list;
	/* Set the first one as the current one */
	d->iter = g_list_first(list);

	/* Get the result of the first query */ 
	md  = d->iter->data;
	/* if the first is an uri, make a local cache copy */
	if(meta_data_is_uri(md))
	{
		const char *path = meta_data_get_uri(md);
		if(path && path[0] == '/') {
			/* if it is a local copy, just set the result as final result */
			d->result = META_DATA_AVAILABLE;
			d->met = md;
			d->iter->data= NULL;
		}else if (path){
			/* if it is a remote uri, download it */

			GEADAsyncHandler *handle = gmpc_easy_async_downloader(path, metadata_download_handler, d);
			if(handle != NULL)
			{
				/* if download is a success wait for result */
				counter++;
				return;
			}
		}
	}else if (meta_data_is_raw(md))
	{
		gchar *filename = gmpc_get_metadata_filename(d->type&(~META_QUERY_NO_CACHE), d->edited, NULL);
		g_file_set_contents(filename, md->content, md->size, NULL);
		g_free(md->content);
		md->size = -1;
		md->content_type = META_DATA_CONTENT_URI;
		md->content = filename;
		d->result = META_DATA_AVAILABLE;
		d->met = md;
		d->iter->data= NULL;
	}else {
		/* if it is not something to download, set the result as final */
		d->result = META_DATA_AVAILABLE;
		d->met = md;
		d->iter->data= NULL;
	}	

	/* Free the remaining results. */
	if(d->list)
	{
		g_list_foreach(d->list, (GFunc)meta_data_free, NULL);
		g_list_free(d->list);
	}
	d->list = NULL;
	d->iter = NULL;

	/**
	 * If still no result, try next plugin
	 */
	if(d->result != META_DATA_AVAILABLE)
	{
		d->index++;
	}
	/**
	 * If we have a result.
	 * Make it not check other plugins
	 */
	else
		d->index = meta_num_plugins;
	/* Continue processing */

	g_idle_add((GSourceFunc)process_itterate,NULL);
	//process_itterate();
}
/**
 * This functions processes the requests, one by one 
 */
static gboolean process_itterate(void)
{
	int length1;

	meta_thread_data *d = NULL;
	/* If queue is empty, do nothing */
	if(process_queue == NULL){
		return FALSE;
	}


	/**
	 * Get the first entry in the queue (this is the active one, or will be the active request 
	 */
	d = process_queue->data;
	/**
	 * Check if there are still plugins left to process
	 */
	if(d->index < meta_num_plugins)
	{
		/**
		 * Before checking all the plugins, query cache
		 * Except when user asks to bypass cache
		 */
		if(d->index == 0)
		{
			/* if the user requested a bypass of the cache, don't query it */
			if(d->type&META_QUERY_NO_CACHE)
			{
				d->result = META_DATA_FETCHING;
			} else {
				MetaData *met = NULL;
				d->result = meta_data_get_from_cache(d->edited,d->type&META_QUERY_DATA_TYPES, &met);
				if(d->result != META_DATA_FETCHING){
					/* Copy the result to the final result */
					d->met = met;
					met = NULL;
				}
				if(met)meta_data_free(met);
			}
		}
		/**
		 * If cache has no reesult, query plugin
		 * Only query when the cache returns
		 */
		if(d->result == META_DATA_FETCHING)
		{
			/* Get the current plugin to query */
			gmpcPluginParent *plug = meta_plugins[d->index];
			/**
			 * Query plugins
			 */
			if(gmpc_plugin_get_enabled(plug))
			{
				gmpc_plugin_metadata_query_metadata_list(plug, 
					d->edited,
					d->type&META_QUERY_DATA_TYPES,
					result_itterate, 
					(gpointer)d);
				return FALSE;
			}
			else
			{
				/* advance to the next plugin */
				d->index++;
				/* do the next itteration */
				//process_itterate();
				return TRUE;
			}
			return FALSE;
		}
	}
	/* A hack to also query non modified named. Only used with rename option enabled */
	if(d->do_rename && d->result == META_DATA_FETCHING )
	{
		mpd_Song *song = d->edited;
		d->edited = d->song;
		d->song = song;
		d->do_rename = FALSE;
		d->index = 0;
		d->rename_done = TRUE;
		return TRUE;
	}
	/* revert changes */
	if(d->rename_done)
	{
		mpd_Song *song = d->edited;
		d->edited = d->song;
		d->song = song;
	}
	/**
	 * If nothing found, set unavailable
	 */
	if(d->result == META_DATA_FETCHING ) {
		d->result = META_DATA_UNAVAILABLE;
	}
	/* Create empty metadata object */
	if(d->met == NULL)
	{
		d->met = meta_data_new();
		d->met->type = (d->type&META_QUERY_DATA_TYPES);
		d->met->content_type = META_DATA_CONTENT_EMPTY;
		d->met->content = NULL; d->met->size = -1;
	}
	/**
	 * Store result (or lack off) 
	 **/
	meta_data_set_cache_real(d->edited, d->result, d->met);
	if(d->edited->artist && d->song->artist)
	{
		if(strcmp(d->edited->artist, d->song->artist)!=0)
			meta_data_set_cache_real(d->song, /*d->type&META_QUERY_DATA_TYPES,*/ d->result, d->met);
	}
	/**
	 * Remove from queue
	 */
	length1 = g_list_length(process_queue);
	/* Remove top */
	process_queue = g_list_delete_link(process_queue, process_queue);
	if(process_queue){
		GList *iter = g_list_first(process_queue);
		for(;iter;iter = g_list_next(iter))
		{
			meta_thread_data *d2 = iter->data;
			if(!meta_compare_func(d,d2)){
				/* Remove from intput list */
				iter = process_queue = g_list_delete_link(process_queue, iter);
				if(d2->callback)
				{
					/* If old fasion query, copy result and push to result-handling */
					d2->result = d->result;
					d2->met = meta_data_dup(d->met);
					/* put result back */
					g_queue_push_tail(meta_results, d2);		
				}else{
					/* if there is no old-type callback, We can remove it completely */
					/* Free result */
					if(d2->met)
						meta_data_free(d2->met);
					/* free stored song copies */
					if(d2->edited) 
						mpd_freeSong(d2->edited);
					if(d2->song)
						mpd_freeSong(d2->song);

					q_free(d2);
				}
			}

		}

	}
	/**
	 * push on handle result
	 */
	g_queue_push_tail(meta_results, d);		
	/**
	 * Call resuult handler 
	 */
	g_idle_add((GSourceFunc)meta_data_handle_results,NULL);

	/**
	 * Next in queue 
	 */

	if(process_queue){
		/* Make it be called again, if there are still items in the queue*/
		return TRUE;
	}
	//	g_idle_add((GSourceFunc)process_itterate, __FUNCTION__);
	//process_itterate();
	return FALSE;
}

/**
 * Function called by the "client" 
 */
MetaDataResult meta_data_get_path(mpd_Song *tsong, MetaDataType type, MetaData **met,MetaDataCallback callback, gpointer data)
{
	static int test_id = 0;
	MetaDataResult ret;
	meta_thread_data *mtd = NULL;
	mpd_Song *song =NULL;
	/* TODO: Validate request */

	g_assert(met != NULL);
	g_assert(*met == NULL);
	/**
	 * If there is no song
	 * return there is not metadata available;
	 */
	if(tsong == NULL)
	{
		return META_DATA_UNAVAILABLE;	
	}

	if(type&META_QUERY_NO_CACHE)
	{
		/**
		 * If the users did request a bypass,
		 * Don't Check cache for result.
		 * But signal an update 
		 */
		gmpc_meta_watcher_data_changed(gmw,tsong, (type)&META_QUERY_DATA_TYPES,META_DATA_FETCHING, NULL); 
		if(callback)
		{
			callback(song,META_DATA_FETCHING,NULL,data);
		}
		ret = META_DATA_FETCHING;
	}
	else
	{
		ret = meta_data_get_from_cache(tsong, type&META_QUERY_DATA_TYPES, met);
		/**
		 * If the data is know. (and doesn't need fectching) 
		 * call the callback and stop
		 */
		if(ret != META_DATA_FETCHING)
		{
			return ret;	
		}
		if(*met) meta_data_free(*met);
		*met = NULL;
	}

	mtd = g_malloc0(sizeof(*mtd));
	/**
	 * Get the 'edited' version we prefer to use when requesting metadata 
	 */
	mtd->edited = rewrite_mpd_song(tsong, type);

	/** 
	 * Query cache, but for changed artist name 
	 */
	if((type&META_QUERY_NO_CACHE) == 0)
	{
		/* Query the cache for the edited version of the cache, if no bypass is requested */
		ret = meta_data_get_from_cache(mtd->edited, type&META_QUERY_DATA_TYPES, met);
		/**
		 * If the data is know. (and doesn't need fectching) 
		 * call the callback and stop
		 */
		if(ret != META_DATA_FETCHING)
		{
			/* store it under the original */
			meta_data_set_cache_real(tsong, ret,*met);
			/* Cleanup what we had so far */
			mpd_freeSong(mtd->edited);
			g_free(mtd);
			/* Steal result */
			return ret;	
		}
		/* If it is fetching free the result */
		if(*met) meta_data_free(*met);
		*met = NULL;
	}


	/**
	 * If no result, start a thread and start fetching the data from there
	 */


	/**
	 * unique id 
	 * Not needed, but can be usefull for debugging
	 */
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
	mtd->index = 0;
	mtd->do_rename = cfg_get_single_value_as_int_with_default(config, "metadata", "rename", FALSE);
	mtd->rename_done = FALSE;
	/* Set that we are fetching */
	mtd->result = META_DATA_FETCHING;
	/* set result NULL */
	mtd->met = NULL;
	
	if(process_queue == NULL) {
		/* If queue is empty, add it, and call the processing function. */
		process_queue = g_list_append(process_queue, mtd);

		g_idle_add((GSourceFunc)process_itterate, NULL);
		//process_itterate();
		return META_DATA_FETCHING;
	}
	/* if queue not empy, append itand do nothing else */
	process_queue = g_list_append(process_queue, mtd);

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

/**
 * Get a list of urls, used by f.e. selector 
 */
typedef struct MLQuery{
	int index;
	int cancel;
	void (*callback)(gpointer handle,const gchar *plugin_name, GList *list, gpointer data);
	gpointer userdata;
	MetaDataType type;
	mpd_Song *song;
	int calls;
}MLQuery;

static void metadata_get_list_itterate(GList *list, gpointer data);
static gboolean metadata_get_list_itterate_idle(gpointer data)
{
	g_log("MetaData", G_LOG_LEVEL_DEBUG, "List itterate idle");
	metadata_get_list_itterate(NULL, data);
	return FALSE;
}

static void metadata_get_list_itterate(GList *list, gpointer data)
{
	MLQuery *q = (MLQuery *)data;
	g_log("MetaData", G_LOG_LEVEL_DEBUG, "List itterate");
	q->calls--;
	if(q->cancel){
		if(list){
			g_list_foreach(list,(GFunc) meta_data_free, NULL);
			g_list_free(list);
		}
		if(q == 0)
		{
			mpd_freeSong(q->song);
			g_free(q);
		}
		/*  clean up */
		return; 
	}
	if(list) {
		gmpcPluginParent *plug = meta_plugins[q->index-1]; 
		q->callback(q,gmpc_plugin_get_name(plug), list,q->userdata);
		g_list_foreach(list,(GFunc) meta_data_free, NULL);
		g_list_free(list);
	}
	if(q->index < meta_num_plugins)
	{
		gmpcPluginParent *plug = meta_plugins[q->index]; 
		q->index++;
		if(gmpc_plugin_get_enabled(plug)){
			q->calls++;
			g_log("MetaData", G_LOG_LEVEL_DEBUG, "Query: %s", gmpc_plugin_get_name(plug));
			gmpc_plugin_metadata_query_metadata_list(plug, q->song, q->type&META_QUERY_DATA_TYPES,metadata_get_list_itterate, (gpointer)q); 
		}
		else g_idle_add(metadata_get_list_itterate_idle, q);
		return;
	}
	/* indicate done, by calling with NULL handle */
	q->callback(q,NULL, NULL, q->userdata);

	mpd_freeSong(q->song);
	g_free(q);
}
void metadata_get_list_cancel(gpointer data)
{
	MLQuery *q = (MLQuery *)data;
	q->cancel = TRUE;
}
gpointer metadata_get_list(mpd_Song  *song, MetaDataType type, void (*callback)(gpointer handle,const gchar *plugin_name, GList *list, gpointer data), gpointer data)
{
	MLQuery *q = g_malloc0(sizeof(*q));
	q->cancel =FALSE;
	q->index = 0;
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
	.pref           = &metadata_pref_plug
};

/* vim: set noexpandtab ts=4 sw=4 sts=4 tw=120: */
