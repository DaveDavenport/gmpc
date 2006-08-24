#include <string.h>
#include <glib.h>
#include <glib/gstdio.h>

#include "main.h"

#include "metadata.h"

config_obj *cover_index= NULL;
int meta_num_plugins=0;
gmpcPlugin **meta_plugins = NULL;


/**
 * This is queue is used to send commands to the retrieval queue
 */
GAsyncQueue *meta_commands = NULL;
/**
 * This queue is used to send replies back.
 */
GAsyncQueue *meta_results = NULL;

GQueue *meta_remove = NULL;

/**
 * TODO: Make the config system thread safe 
 */
typedef struct {
	guint id;
	/* Data */
	mpd_Song *song;
	MetaDataCallback callback;
	gpointer data;
	MetaDataType type;
	/* Resuls  */
	MetaDataResult result;
	char *result_path;
} meta_thread_data;



void meta_data_set_cache(mpd_Song *song, MetaDataType type, MetaDataResult result, char *path)
{
	if(!song) return;
	/**
	 * Save the path for the album art
	 */
	if(type == META_ALBUM_ART)
	{
		if(song->artist && song->album)
		{
			char *temp = g_strdup_printf("album:%s", song->album);
			if(result == META_DATA_AVAILABLE)
			{
				cfg_set_single_value_as_string(cover_index, song->artist, temp,path);
			}
			else
			{
				cfg_set_single_value_as_string(cover_index, song->artist, temp,"");
			}
			g_free(temp);
		}
	}
	else if(type == META_ALBUM_TXT)
	{
		if(song->artist && song->album)
		{
			char *temp = g_strdup_printf("albumtxt:%s", song->album);                   		
			if(result == META_DATA_AVAILABLE)
			{
				cfg_set_single_value_as_string(cover_index, song->artist, temp,path);
			}
			else
			{
				cfg_set_single_value_as_string(cover_index, song->artist, temp,"");
			}                                                                        		
			g_free(temp);
		}
	}
	else if (type == META_ARTIST_ART)
	{
		if(song->artist)
		{
			char *temp = g_strdup("image");                   		
			if(result == META_DATA_AVAILABLE)                                                   		
			{
				cfg_set_single_value_as_string(cover_index, song->artist, temp,path);
			}
			else
			{
				cfg_set_single_value_as_string(cover_index, song->artist, temp,"");
			}                                                                        		
			g_free(temp);
		}
	}
	else if (type == META_ARTIST_TXT)
	{
		if(song->artist)
		{
			char *temp = g_strdup("biography");                   		
			if(result == META_DATA_AVAILABLE)                                                   		
			{
				cfg_set_single_value_as_string(cover_index, song->artist, temp,path);
			}
			else
			{
				cfg_set_single_value_as_string(cover_index, song->artist, temp,"");
			}                                                                        		
			g_free(temp);
		}
	}
	else if (type == META_SONG_TXT)
	{
		if(song->artist && song->title)
		{
			char *temp = g_strdup_printf("lyrics:%s", song->title);                   		
			if(result == META_DATA_AVAILABLE)                                                   		
			{
				cfg_set_single_value_as_string(cover_index, song->artist, temp,path);
			}
			else
			{
				cfg_set_single_value_as_string(cover_index, song->artist, temp,"");
			}                                                                        		
			g_free(temp);
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
		g_free(temp);
		if(*path)
		{
			/* if path length is NULL, then data unavailible */
			if(strlen(*path) == 0)
			{
				g_free(*path);

				*path = NULL;
				return META_DATA_UNAVAILABLE;	
			}
			/* return that data is availible */
			if(!g_file_test(*path, G_FILE_TEST_EXISTS))
			{
				temp = g_strdup_printf("album:%s", song->album);
				cfg_del_single_value(cover_index, song->artist, temp);
				g_free(temp);
				g_free(*path);
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
		g_free(temp);
		if(*path)
		{
			/* if path length is NULL, then data unavailible */
			if(strlen(*path) == 0)
			{
				g_free(*path);                                                  		
				*path = NULL;
				return META_DATA_UNAVAILABLE;	
			}
			/* return that data is availible */
			if(!g_file_test(*path, G_FILE_TEST_EXISTS))
			{
				temp = g_strdup_printf("albumtxt:%s", song->album);
				cfg_del_single_value(cover_index, song->artist, temp);
				g_free(temp);
				g_free(*path);
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
		g_free(temp);
		if(*path)
		{
			/* if path length is NULL, then data unavailible */
			if(strlen(*path) == 0)
			{
				g_free(*path);                                                  		
				*path = NULL;
				return META_DATA_UNAVAILABLE;	
			}
			/* return that data is availible */
			if(!g_file_test(*path, G_FILE_TEST_EXISTS))
			{
				temp = g_strdup("image");
				cfg_del_single_value(cover_index, song->artist, temp);
				g_free(temp);                                         			
				g_free(*path);
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
		g_free(temp);
		if(*path)
		{
			/* if path length is NULL, then data unavailible */
			if(strlen(*path) == 0)
			{
				g_free(*path);                                                  		
				*path = NULL;
				return META_DATA_UNAVAILABLE;	
			}
			/* return that data is availible */
			if(!g_file_test(*path, G_FILE_TEST_EXISTS))
			{
				temp = g_strdup("biography");
				cfg_del_single_value(cover_index, song->artist, temp);
				g_free(temp);
				g_free(*path);
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
		g_free(temp);
		if(*path)
		{
			/* if path length is NULL, then data unavailible */
			if(strlen(*path) == 0)
			{
				g_free(*path);                                                  		
				*path = NULL;
				return META_DATA_UNAVAILABLE;	
			}
			/* return that data is availible */
			if(!g_file_test(*path, G_FILE_TEST_EXISTS))
			{
				temp = g_strdup_printf("lyrics:%s",song->title);
				cfg_del_single_value(cover_index, song->artist, temp);
				g_free(temp);
				g_free(*path);
				*path = NULL;
				return META_DATA_FETCHING;	
			}
			/* return that data is availible */
			return META_DATA_AVAILABLE;
		}	
	}
	return META_DATA_FETCHING;	
}

void meta_data_retrieve_thread()
{
	meta_thread_data *data = NULL;
	/**
	 * A continues loop, waiting for new commands to arrive
	 */
	do{
		/**
		 * Get command from queue
		 */
		data = g_async_queue_pop(meta_commands);	
		/* 
		 * Set default return values
		 * TODO: Is this needed, because the cache will "init" them.
		 */

		data->result = META_DATA_UNAVAILABLE;
		data->result_path = NULL;

		/*
		 * Check cache *again*
		 * because between the time this command was commited, and the time 
		 * we start processing it, the result may allready been retrieved
		 * TODO: Make an option to _force_ it to recheck (aka bypass cache 
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
			int i = 0;
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

		}
		/** 
		 * update cache 
		 */
		meta_data_set_cache(data->song, data->type&META_QUERY_DATA_TYPES, data->result, data->result_path);

		/**
		 * Push the result back
		 */	
		g_async_queue_push(meta_results, data);		
		/**
		 * clear our reference to the object
		 */
		data = NULL;
	}while(1);
}




void meta_data_handle_remove_request(guint id)
{
	g_queue_push_head(meta_remove, GINT_TO_POINTER(id));
}

gboolean meta_data_handle_results()
{
	meta_thread_data *data = NULL;


	if(g_async_queue_length(meta_results) == 0 &&
		g_async_queue_length(meta_commands) == 0)
		{
			while(g_queue_pop_head(meta_remove));

		}



	/**
	 *  Check if there are results to handle
	 *  do this until the list is clear
	 */
	for(data = g_async_queue_try_pop(meta_results);data;
			data = g_async_queue_try_pop(meta_results))
	{	
		int test = 0, i = 0;
		for(i=g_queue_get_length(meta_remove)-1; i>=0;i--)
		{
			int num = GPOINTER_TO_INT(g_queue_peek_nth(meta_remove,i));
			if(num == data->id)
			{ 
				test = 1;
				g_queue_pop_nth(meta_remove, i);	

			}
		}
			
		if(test == 0&& data->callback)
		{
			data->callback(data->song, data->result,data->result_path, data->data);
		}
		if(data->result_path)g_free(data->result_path);
		mpd_freeSong(data->song);
		g_free(data);
	}
	/**
	 * Keep the timer running
	 */
	return TRUE;
}

/**
 * Initialize
 */

void meta_data_init()
{
	gchar *url = g_strdup_printf("%s/.covers/", g_get_home_dir());
	if(!g_file_test(url,G_FILE_TEST_IS_DIR)){
		if(g_mkdir(url, 0700)<0){
			g_error("Cannot make %s\n", url);
		}
	}
	g_free(url);
	url = g_strdup_printf("%s/.covers/covers.db", g_get_home_dir());
	cover_index = cfg_open(url);
	g_free(url);

	/**
	 * The command queue
	 */
	meta_commands = g_async_queue_new();
	/**
	 * the result queue
	 */
	meta_results = g_async_queue_new();
	/**
 	*  remove callbacks...
 	*  not thread save
 	*/
	meta_remove = g_queue_new();
	/**
	 * Create the retrieval thread
	 */
	g_thread_create((GThreadFunc)meta_data_retrieve_thread, NULL, FALSE, NULL);
	/**
	 * Set a timer on checking the results
	 * for now every 50 ms?
	 */
	g_timeout_add(50,(GSourceFunc)meta_data_handle_results, NULL);

}
/**
 * Function called by the "client" 
 */
guint meta_data_get_path_callback(mpd_Song *tsong, MetaDataType type, MetaDataCallback callback, gpointer data)
{
	MetaDataResult ret;
	meta_thread_data *mtd = NULL;
	mpd_Song *song =NULL;
	char *path = NULL;
	guint id = 0;

	/**
	 * if there is no callback, it's a programming error.
	 */
	g_assert(callback != NULL);

	/**
	 * If there is no song
	 * return;
	 */
	/*	g_return_if_fail(tsong != NULL); */
	if(tsong == NULL)
	{
		return 0;	
	}

	/**
	 * Check cache for result.
	 */
	if(type&META_QUERY_NO_CACHE)
	{
		ret = META_DATA_FETCHING;
	}
	else
	{
		ret = meta_data_get_from_cache(tsong, type&META_QUERY_DATA_TYPES, &path);
	}

	/**
	 * If the data is know. (and doesn't need fectching) 
	 * call the callback and stop
	 */
	if(ret != META_DATA_FETCHING)
	{
		/* Call the callback function */
		callback(tsong, ret, path,data);
		/* clean up path if exists */
		if(path) g_free(path);
		/* return */

		return 0;
	}

	/**
	 * Make a copy
	 */
	song = mpd_songDup(tsong);

	/**
	 * Check if the song is complete, (has file) so we can actually use it with all plugins
	 * If not, f.e. only artist, or only album, get a song from mpd
	 * so plugins based on path can work with it.
	 */
	/** For speed reason and mpd stressing disabled */
//	if(song->file == NULL)
	if(FALSE){
		/* Only mpd 0.12 supports this */
		if(mpd_server_check_version(connection, 0,12,0))
		{
			MpdData *data  = NULL;
			/** We need new libmpd data here.
			 * The we don't need the check what type of search
			 */
			/**
			 * this should be done faster, it can now cause extra slowdown
			 * because of the mpd roundtrips
			 */
			if(song->artist && song->album)
			{
				data= mpd_database_find_adv(connection,TRUE, 
						MPD_TAG_ITEM_ARTIST,
						song->artist,
						MPD_TAG_ITEM_ALBUM,
						song->album,
						-1);
			}
			else if(song->artist)
			{
				data= mpd_database_find_adv(connection, TRUE,
						MPD_TAG_ITEM_ARTIST,
						song->artist,
						-1);
			}
			if(data)
			{
				if(data->type == MPD_DATA_TYPE_SONG)
				{
					song->file = g_strdup(data->song->file);
				}

				mpd_data_free(data);
			}
		}
	}
	/**
	 * If no result, start a thread and start fetching the data from there
	 */

	mtd = g_malloc0(sizeof(*mtd));
	/**
	 * unique id 
	 * Not needed, but can be usefull for debugging
	 */
	id = mtd->id = g_random_int_range(1,2147483647);
	mtd->song = mpd_songDup(song);
	mtd->callback = callback;
	mtd->data = data;
	mtd->type = type;
	/** push it to the other thread */
	g_async_queue_push(meta_commands, mtd);
	/** clean reference to pointer, it's now to the other thread */
	mtd = NULL;

	/**
	 * Call the callback to let the client know where are going todo a 
	 * background fetch
	 */

	/**
	 * Tell the calling part we are fetching */
	callback(song, META_DATA_FETCHING,NULL, data);
	mpd_freeSong(song);
	return id;
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

void meta_data_check_plugin_changed()
{
	conf_mult_obj *class = NULL;
	int old_amount= cfg_get_single_value_as_int_with_default(config, "metadata", "num_plugins", 0);
	if(old_amount < meta_num_plugins)
	{
		GtkWidget *dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
				_("A new metadata plugin was added, gmpc will now purge all missing metadata from the cache"));
		g_signal_connect(G_OBJECT(dialog), "response", G_CALLBACK(gtk_widget_destroy), NULL);
		gtk_widget_show_all(GTK_WIDGET(dialog));
		while(gtk_events_pending()) gtk_main_iteration();
		class = cfg_get_class_list(cover_index);
		if(class)
		{
			while(class){
				conf_mult_obj *keys = cfg_get_key_list(cover_index,class->key);
				while(keys)
				{
					if(keys->value == NULL || keys->value[0] == '\0' )
					{
						cfg_del_single_value(cover_index, class->key, keys->key);
					}
					if(keys->next) keys = keys->next;
					else{
						cfg_free_multiple(keys);
						keys = NULL;
					}
				}
				if(class->next) class = class->next;
				else {
					cfg_free_multiple(class);
					class = NULL;
				}
			}
		}
	}
	if(old_amount != meta_num_plugins)
	{
		cfg_set_single_value_as_int(config, "metadata", "num_plugins", meta_num_plugins);
	}
}

