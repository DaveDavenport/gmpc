#include <string.h>
#include <glib.h>
#include <glib/gstdio.h>
#include "qlib/qasyncqueue.h"
#include "main.h"

#include "metadata.h"


config_obj *cover_index= NULL;
int meta_num_plugins=0;
gmpcPlugin **meta_plugins = NULL;

GThread *meta_thread = NULL;
/**
 * This is queue is used to send commands to the retrieval queue
 */
QAsyncQueue *meta_commands = NULL;
/**
 * This queue is used to send replies back.
 */
QAsyncQueue *meta_results = NULL;

GQueue *meta_remove = NULL;
GMutex *meta_processing = NULL;

typedef struct {
	guint id;
	/* Data */
	mpd_Song *song;
	MetaDataType type;
	/* Resuls  */
	MetaDataResult result;
	char *result_path;
	/* Callback */
	MetaDataCallback callback;
	gpointer data;
} meta_thread_data;

gboolean meta_compare_func(meta_thread_data *mt1, meta_thread_data *mt2);
static gboolean meta_data_handle_results(void);
/*
static gboolean meta_data_handler_data_match(meta_thread_data *data, gpointer data2);
*/
void meta_data_set_cache(mpd_Song *song, MetaDataType type, MetaDataResult result, char *path)
{
	if(!song) return;
	/**
	 * Save the path for the album art
	 */
	if(type == META_ALBUM_ART) {
		if(song->artist && song->album) {
			char *temp = g_strdup_printf("album:%s", song->album);
			if(result == META_DATA_AVAILABLE) {
				cfg_set_single_value_as_string(cover_index, song->artist, temp,path);
			} else {
				cfg_set_single_value_as_string(cover_index, song->artist, temp,"");
			}
			q_free(temp);
		}
	} else if(type == META_ALBUM_TXT) {
		if(song->artist && song->album)	{
			char *temp = g_strdup_printf("albumtxt:%s", song->album);                   		
			if(result == META_DATA_AVAILABLE) {
				cfg_set_single_value_as_string(cover_index, song->artist, temp,path);
			} else {
				cfg_set_single_value_as_string(cover_index, song->artist, temp,"");
			}                                                                        		
			q_free(temp);
		}
	} else if (type == META_ARTIST_ART) {
		if(song->artist) {
			char *temp = g_strdup("image");                   		
			if(result == META_DATA_AVAILABLE) {
				cfg_set_single_value_as_string(cover_index, song->artist, temp,path);
			} else {
				cfg_set_single_value_as_string(cover_index, song->artist, temp,"");
			}                                                                        		
			q_free(temp);
		}
	} else if (type == META_ARTIST_TXT) {
		if(song->artist) {
			char *temp = g_strdup("biography");                   		
			if(result == META_DATA_AVAILABLE) {
				cfg_set_single_value_as_string(cover_index, song->artist, temp,path);
			} else {
				cfg_set_single_value_as_string(cover_index, song->artist, temp,"");
			}                                                                        		
			q_free(temp);
		}
	} else if (type == META_SONG_TXT)	{
		if(song->artist && song->title) {
			char *temp = g_strdup_printf("lyrics:%s", song->title);                   		
			if(result == META_DATA_AVAILABLE) {
				cfg_set_single_value_as_string(cover_index, song->artist, temp,path);
			} else {
				cfg_set_single_value_as_string(cover_index, song->artist, temp,"");
			}                                                                        		
			q_free(temp);
		}
	}
}

/**
 * Checking the cache 
 * !!NEEDS TO BE THREAD SAFE !!
 */


MetaDataResult meta_data_get_from_cache(mpd_Song *song, MetaDataType type, char **path)
{
	if(!song)
	{
		return META_DATA_UNAVAILABLE;	
	}
	debug_printf(DEBUG_INFO,"request for: %s, %i\n", song->title, type);
	/* Get values acording to type */
	if(type == META_ALBUM_ART)
	{
		gchar *temp = NULL;
		if(!song->artist || !song->album)
		{
			return META_DATA_UNAVAILABLE;	
		}
		temp = g_strdup_printf("album:%s", song->album);
		*path = cfg_get_single_value_as_string(cover_index,song->artist,  temp);
		q_free(temp);
		if(*path)
		{
			/* if path length is NULL, then data unavailible */
			if(strlen(*path) == 0)
			{
				q_free(*path);

				*path = NULL;
				return META_DATA_UNAVAILABLE;	
			}
			/* return that data is availible */
			if(!g_file_test(*path, G_FILE_TEST_EXISTS))
			{
				temp = g_strdup_printf("album:%s", song->album);
				cfg_del_single_value(cover_index, song->artist, temp);
				q_free(temp);
				q_free(*path);
				*path = NULL;
				return META_DATA_FETCHING;	
			}
			return META_DATA_AVAILABLE;
		}

		/* else default to fetching */
	}
	else if(type == META_ALBUM_TXT)
	{
		gchar *temp = NULL;
		if(!song->artist || !song->album)
		{
			return META_DATA_UNAVAILABLE;	
		}
		temp = g_strdup_printf("albumtxt:%s", song->album);
		*path = cfg_get_single_value_as_string(cover_index,song->artist,  temp);
		q_free(temp);
		if(*path)
		{
			/* if path length is NULL, then data unavailible */
			if(strlen(*path) == 0)
			{
				q_free(*path);                                                  		
				*path = NULL;
				return META_DATA_UNAVAILABLE;	
			}
			/* return that data is availible */
			if(!g_file_test(*path, G_FILE_TEST_EXISTS))
			{
				temp = g_strdup_printf("albumtxt:%s", song->album);
				cfg_del_single_value(cover_index, song->artist, temp);
				q_free(temp);
				q_free(*path);
				*path = NULL;
				return META_DATA_FETCHING;	
			}
			/* return that data is availible */
			return META_DATA_AVAILABLE;
		}
	}
	else if (type == META_ARTIST_ART)
	{
		gchar *temp = NULL;
		if(!song->artist)
		{
			return META_DATA_UNAVAILABLE;	
		}
		temp = g_strdup("image");
		*path = cfg_get_single_value_as_string(cover_index,song->artist,  temp);
		q_free(temp);
		if(*path)
		{
			/* if path length is NULL, then data unavailible */
			if(strlen(*path) == 0)
			{
				q_free(*path);                                                  		
				*path = NULL;
				return META_DATA_UNAVAILABLE;	
			}
			/* return that data is availible */
			if(!g_file_test(*path, G_FILE_TEST_EXISTS))
			{
				temp = g_strdup("image");
				cfg_del_single_value(cover_index, song->artist, temp);
				q_free(temp);                                         			
				q_free(*path);
				*path = NULL;
				return META_DATA_FETCHING;	
			}
			/* return that data is availible */
			return META_DATA_AVAILABLE;
		}
	}
	else if (type == META_ARTIST_TXT)
	{
		gchar *temp = NULL;
		if(!song->artist)
		{
			return META_DATA_UNAVAILABLE;	
		}
		temp = g_strdup("biography");
		*path = cfg_get_single_value_as_string(cover_index,song->artist,  temp);
		q_free(temp);
		if(*path)
		{
			/* if path length is NULL, then data unavailible */
			if(strlen(*path) == 0)
			{
				q_free(*path);                                                  		
				*path = NULL;
				return META_DATA_UNAVAILABLE;	
			}
			/* return that data is availible */
			if(!g_file_test(*path, G_FILE_TEST_EXISTS))
			{
				temp = g_strdup("biography");
				cfg_del_single_value(cover_index, song->artist, temp);
				q_free(temp);
				q_free(*path);
				*path = NULL;
				return META_DATA_FETCHING;	
			}
			/* return that data is availible */
			return META_DATA_AVAILABLE;
		}

	}
	if(type == META_SONG_TXT)
	{
		gchar *temp = NULL;
		if(!song->artist || !song->title)
		{
			return META_DATA_UNAVAILABLE;	
		}
		temp = g_strdup_printf("lyrics:%s", song->title);
		*path = cfg_get_single_value_as_string(cover_index,song->artist,  temp);
		q_free(temp);
		if(*path)
		{
			/* if path length is NULL, then data unavailible */
			if(strlen(*path) == 0)
			{
				q_free(*path);                                                  		
				*path = NULL;
				return META_DATA_UNAVAILABLE;	
			}
			/* return that data is availible */
			if(!g_file_test(*path, G_FILE_TEST_EXISTS))
			{
				temp = g_strdup_printf("lyrics:%s",song->title);
				cfg_del_single_value(cover_index, song->artist, temp);
				q_free(temp);
				q_free(*path);
				*path = NULL;
				return META_DATA_FETCHING;	
			}
			/* return that data is availible */
			return META_DATA_AVAILABLE;
		}	
	}
	return META_DATA_FETCHING;	
}

static void meta_data_retrieve_thread()
{
	meta_thread_data *data = NULL;
	/**
	 * A continues loop, waiting for new commands to arrive
	 */
	do{
		/**
		 * Get command from queue
		 */
		
		data = q_async_queue_pop(meta_commands);	
		/* check if quit signal */
		if(data->id == 0)
		{
			return;
		}
		g_mutex_lock(meta_processing);
		/* 
		 * Set default return values
		 * Just to be sure, init them.
		 */

		data->result = META_DATA_UNAVAILABLE;
		data->result_path = NULL;

		/*
		 * Check cache *again*
		 * because between the time this command was commited, and the time 
		 * we start processing it, the result may allready been retrieved
		 */
		if(data->type&META_QUERY_NO_CACHE)
		{
			data->result = META_DATA_FETCHING;
		}
		else
		{
			data->result = meta_data_get_from_cache(data->song,data->type&META_QUERY_DATA_TYPES, &(data->result_path));
		}
		/**
		 * Handle cache result.
		 * If the cache returns it doesn't have anything (that it needs fetching)
		 * Start fetching 
		 */
		if(data->result == META_DATA_FETCHING)
		{
			char *path = NULL;
			char *old = NULL;
			int i = 0;
			

			if(data->song->artist && cfg_get_single_value_as_int_with_default(config, "metadata", "rename", FALSE))
			{
				gchar **str = g_strsplit(data->song->artist, ",", 2);
				old = data->song->artist;
				if(str[1]) {
					data->song->artist = g_strdup_printf("%s %s", g_strstrip(str[1]), g_strstrip(str[0]));
				}else{
					data->song->artist = g_strdup(old);
				}					
				g_strfreev(str);
				debug_printf(DEBUG_INFO, "string converted to: '%s'", data->song->artist);
			}
			/* 
			 * Set default return values
			 * Need to be reset, because of cache fetch
			 */
			data->result = META_DATA_UNAVAILABLE;
			data->result_path = NULL;            			
			/* 
			 * start fetching the results 
			 */
			/**
			 * Loop through all the plugins until we don't have plugins anymore, or we have a result.
			 */
			for(i=0;i<(meta_num_plugins) && data->result != META_DATA_AVAILABLE;i++)
			{
				/* *
				 * Get image function is only allowed to return META_DATA_AVAILABLE or META_DATA_UNAVAILABLE
				 */
				if(meta_plugins[i]->get_enabled())
				{
					data->result = meta_plugins[i]->metadata->get_image(data->song, data->type&META_QUERY_DATA_TYPES, &path);
					data->result_path = path;
				}
			}
			if(old)
			{
				g_free(data->song->artist);
				data->song->artist = old;
			}

		}
		/** 
		 * update cache 
		 */
		meta_data_set_cache(data->song, data->type&META_QUERY_DATA_TYPES, data->result, data->result_path);

		/**
		 * Push the result back
		 */	
		q_async_queue_push(meta_results, data);		
		g_idle_add((GSourceFunc)meta_data_handle_results,NULL);
		/**
		 * clear our reference to the object
		 */
		data = NULL;
		g_mutex_unlock(meta_processing);
	}while(1);
}
/*

static gboolean meta_data_handler_data_match(meta_thread_data *data, gpointer data2)
{
	if(data && data->id == GPOINTER_TO_INT(data2))
	{
		return 0;
	}
	return 1;
}

void meta_data_handle_remove_request(guint id)
{
	meta_thread_data *data = NULL;
	gboolean found = FALSE;
	
	if(!meta_commands)	
		return;

	q_async_queue_lock(meta_commands);
	if((data = q_async_queue_remove_data_unlocked(meta_commands, (GCompareFunc)meta_data_handler_data_match, GINT_TO_POINTER(id))))
	{
		if(data->result_path)q_free(data->result_path);
		mpd_freeSong(data->song);
		q_free(data);
		found = TRUE;
	}
	q_async_queue_unlock(meta_commands);
	debug_printf(DEBUG_ERROR, "(no error) Removing id: %u", id);

	if(!found)
		g_queue_push_head(meta_remove, GINT_TO_POINTER(id));
}
*/
static gboolean meta_data_handle_results(void)
{
	meta_thread_data *data = NULL;
	if(meta_thread == g_thread_self())
	{
		printf("Crap, handled in wrong thread\n");

	}
	/**
	 * Should check is one is being processed  (implemented)
	 */
	if(q_async_queue_length(meta_results) == 0 &&
		q_async_queue_length(meta_commands) == 0)
	{
		/** if the fetching thread is busy, don't do anything,
		 * if not, then lock it and clear it
		 */
		if(g_mutex_trylock(meta_processing)){
			debug_printf(DEBUG_INFO, "Clearing callback remove list");
			while(g_queue_pop_head(meta_remove));
			g_mutex_unlock(meta_processing);
		}
	}



	/**
	 *  Check if there are results to handle
	 *  do this until the list is clear
	 */
	for(data = q_async_queue_try_pop(meta_results);data;
			data = q_async_queue_try_pop(meta_results)) {	
		int test = 0, i = 0;

		gmpc_meta_watcher_data_changed(gmw,data->song, (data->type)&META_QUERY_DATA_TYPES, data->result,data->result_path);
		if(data->callback)
		{
			data->callback(data->song,data->result,data->result_path, data->data);
		}

		for(i=g_queue_get_length(meta_remove)-1; i>=0;i--) {
			int num = GPOINTER_TO_INT(g_queue_peek_nth(meta_remove,i));
			if(num == data->id) { 
				test = 1;
				g_queue_pop_nth(meta_remove, i);	
				debug_printf(DEBUG_INFO, "Removing callback: %u\n", num);
			}
		}
	
	
		if(data->result_path)q_free(data->result_path);
		mpd_freeSong(data->song);
		q_free(data);
	}
	/**
	 * Keep the timer running
	 */
	return FALSE;
}

/**
 * Initialize
 */
void meta_data_init()
{
	gchar *url = gmpc_get_covers_path(NULL); 
	if(!g_file_test(url,G_FILE_TEST_IS_DIR)){
		if(g_mkdir(url, 0700)<0){
			g_error("Cannot make %s\n", url);
		}
	}
	q_free(url);
	url = gmpc_get_covers_path("covers.db");
	cover_index = cfg_open(url);
	q_free(url);

	/**
	 * The command queue
	 */
	meta_commands = q_async_queue_new();
	/**
	 * the result queue
	 */
	meta_results = q_async_queue_new();
	/**
 	*  remove callbacks...
 	*  not thread save
 	*/
	meta_remove = g_queue_new();


	meta_processing = g_mutex_new();
	/**
	 * Create the retrieval thread
	 */
	meta_thread = g_thread_create((GThreadFunc)meta_data_retrieve_thread, NULL, TRUE, NULL);

}

void meta_data_add_plugin(gmpcPlugin *plug)
{
	int i=0;
	int changed = FALSE;	
	meta_num_plugins++;
	meta_plugins = g_realloc(meta_plugins,(meta_num_plugins+1)*sizeof(gmpcPlugin **));
	meta_plugins[meta_num_plugins-1] = plug;
	meta_plugins[meta_num_plugins] = NULL;

	do{	
		changed=0;
		for(i=0; i< (meta_num_plugins-1);i++)
		{
			if(meta_plugins[i]->metadata->get_priority() > meta_plugins[i+1]->metadata->get_priority())
			{
				gmpcPlugin *temp = meta_plugins[i];
				changed=1;
				meta_plugins[i] = meta_plugins[i+1];
				meta_plugins[i+1] = temp;
			}
		}
	}while(changed);
}

void meta_data_cleanup(void)
{
	cfg_do_special_cleanup(cover_index);
}

void meta_data_check_plugin_changed()
{
	int old_amount= cfg_get_single_value_as_int_with_default(config, "metadata", "num_plugins", 0);
	if(old_amount < meta_num_plugins)
	{
		GtkWidget *dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
				_("A new metadata plugin was added, gmpc will now purge all missing metadata from the cache"));
		g_signal_connect(G_OBJECT(dialog), "response", G_CALLBACK(gtk_widget_destroy), NULL);
		gtk_widget_show_all(GTK_WIDGET(dialog));
		meta_data_cleanup();
	}
	if(old_amount != meta_num_plugins)
	{
		cfg_set_single_value_as_int(config, "metadata", "num_plugins", meta_num_plugins);
	}
}

void meta_data_destroy(void)
{
	meta_thread_data *mtd = NULL;

	if(meta_thread)
	{
		debug_printf(DEBUG_INFO,"Waiting for meta thread to terminate...");
		/* remove old stuff */
		q_async_queue_lock(meta_commands);
		while((mtd = q_async_queue_try_pop_unlocked(meta_commands)))
		{
			mpd_freeSong(mtd->song);
                        q_free(mtd);
		}
		/* Create the quiet signal, this is just an empty request with id 0 */
		
		mtd = g_malloc0(sizeof(*mtd));
		mtd->id = 0;
		/* push the request to the thread */
		q_async_queue_push_unlocked(meta_commands, mtd);
		q_async_queue_unlock(meta_commands);
		/* wait for the thread to finish */
		g_thread_join(meta_thread);
		/* cleanup */
		g_free(mtd);
		debug_printf(DEBUG_INFO,"Done..");
	}
	cfg_close(cover_index);
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
 * Function called by the "client" 
 */
MetaDataResult meta_data_get_path(mpd_Song *tsong, MetaDataType type, gchar **path,MetaDataCallback callback, gpointer data)
{
	MetaDataResult ret;
	meta_thread_data *mtd = NULL;
	mpd_Song *song =NULL;
	guint id = 0;
	/* TODO: Validate request */

	/**
	 * If there is no song
	 * return;
	 */
	if(tsong == NULL)
	{
		return META_DATA_UNAVAILABLE;	
	}

	/**
	 * Check cache for result.
	 */
	if(type&META_QUERY_NO_CACHE)
	{
		/* For others */
		gmpc_meta_watcher_data_changed(gmw,tsong, (type)&META_QUERY_DATA_TYPES,META_DATA_FETCHING, NULL); 
		if(callback)
		{
			callback(song,META_DATA_FETCHING,NULL,data);
		}
		ret = META_DATA_FETCHING;
	}
	else
	{
		ret = meta_data_get_from_cache(tsong, type&META_QUERY_DATA_TYPES, path);
	}

	/**
	 * If the data is know. (and doesn't need fectching) 
	 * call the callback and stop
	 */
	if(ret != META_DATA_FETCHING)
	{
		return ret;	
	}


	/**
	 * Make a copy
	 */
	song = mpd_songDup(tsong);
	/**
	 * If no result, start a thread and start fetching the data from there
	 */

	mtd = g_malloc0(sizeof(*mtd));
	/**
	 * unique id 
	 * Not needed, but can be usefull for debugging
	 */
	id = mtd->id = g_random_int_range(1,2147483647);
	mtd->song = song;
	mtd->type = type;
	mtd->callback = callback;
	mtd->data = data;
	/**
	 * Check if request is allready in queue
	 */
	/* when using old api style, the request is commited anyway */
	if(!callback)
	{
		q_async_queue_lock(meta_commands);
		if(q_async_queue_has_data(meta_commands,(GCompareFunc)meta_compare_func, mtd))
		{
			q_async_queue_unlock(meta_commands);
			mpd_freeSong(song);
			g_free(mtd);
			return ret;
		}
		q_async_queue_unlock(meta_commands);
	}

	/** push it to the other thread */
	q_async_queue_push(meta_commands, mtd);
	/** clean reference to pointer, it's now to the other thread */
	mtd = NULL;

	return ret;
}
