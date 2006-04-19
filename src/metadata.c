#include <string.h>
#include "main.h"

#include "metadata.h"
#include "cover-art.h"

extern config_obj *cover_index;

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
			char *temp = g_strdup_printf("biography:%s", song->title);                   		
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
			return META_DATA_AVAILABLE;
		}
	}
	else if (type == META_ARTIST_ART)
	{


	}
	else if (type == META_ARTIST_TXT)
	{



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
		printf("Meta Retrieval Thread: [%u] Got command\n",data->id);
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
		data->result = meta_data_get_from_cache(data->song,data->type, &(data->result_path));
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
				data->result = meta_plugins[i]->metadata->get_image(data->song, data->type, &path);
				data->result_path = path;
			}

		}
		/** 
		 * update cache 
		 */
		meta_data_set_cache(data->song, data->type, data->result, data->result_path);

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


gboolean meta_data_handle_results()
{
	meta_thread_data *data = NULL;

	/**
	 *  Check if there are results to handle
	 *  do this until the list is clear
	 */
	for(data = g_async_queue_try_pop(meta_results);data;
			data = g_async_queue_try_pop(meta_results))
	{	
		printf("Meta Data: [%u] Handling results\n",data->id);
		printf("Had: %s\n", data->result_path);
		data->callback(data->song, data->result,data->result_path, data->data);
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
	/**
	 * The command queue
	 */
	meta_commands = g_async_queue_new();
	/**
	 * the result queue
	 */
	meta_results = g_async_queue_new();
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
void meta_data_get_path_callback(mpd_Song *song, MetaDataType type, MetaDataCallback callback, gpointer data)
{
	MetaDataResult ret;
	char *path = NULL;

	/**
	 * if there is no callback, it's a programming error.
	 */
	g_assert(callback != NULL);

	/**
	 * If there is no song, then the same.
	 */
	g_return_if_fail(song != NULL);

	/**
	 * Check cache for result.
	 */
	ret = meta_data_get_from_cache(song, type, &path);

	/**
	 * If the data is know. (and doesn't need fectching) 
	 * call the callback and stop
	 */
	if(ret != META_DATA_FETCHING)
	{
		/* Call the callback function */
		callback(song, ret, path,data);
		/* clean up path if exists */
		if(path) g_free(path);
		/* return */
		return;
	}


	/**
	 * If no result, start a thread and start fetching the data from there
	 */

	meta_thread_data *mtd = g_malloc0(sizeof(*mtd));
	mtd->id = g_random_int();
	mtd->song = mpd_songDup(song);
	mtd->callback = callback;
	mtd->data = data;
	mtd->type = type;
	g_async_queue_push(meta_commands, mtd);
	mtd = NULL;

	/**
	 * Call the callback to let the client know where are going todo a 
	 * background fetch
	 */


	callback(song, META_DATA_FETCHING,NULL, data);
	/*	if(path)g_free(path);*/
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
			if(meta_plugins[i]->metadata->get_priority() < meta_plugins[i+1]->metadata->get_priority())
			{
				gmpcPlugin *temp = meta_plugins[i];
				changed=1;
				meta_plugins[i] = meta_plugins[i+1];
				meta_plugins[i+1] = temp;
			}
		}
	}while(changed);
}

