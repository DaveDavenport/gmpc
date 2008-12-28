#include <string.h>
#include <glib.h>
#include <glib/gstdio.h>
#include "qlib/qasyncqueue.h"
#include "main.h"

#include "metadata.h"

long unsigned num_queries = 0;

config_obj *cover_index= NULL;
int meta_num_plugins=0;
GMutex *meta_plugins_lock = NULL;
gmpcPlugin **meta_plugins = NULL;
static void meta_data_sort_plugins(void);
GThread *meta_thread = NULL;
/**
 * This is queue is used to send commands to the retrieval queue
 */
QAsyncQueue *meta_commands = NULL;
/**
 * This queue is used to send replies back.
 */
QAsyncQueue *meta_results = NULL;

GMutex *meta_processing = NULL;

typedef struct {
	guint id;
	/* Data */
	mpd_Song *song;
        mpd_Song *edited;
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
static mpd_Song *rewrite_mpd_song(mpd_Song *tsong, MetaDataType type)
{
    mpd_Song *edited = NULL;
    /* If it is not a mpd got song */
    if(tsong->file == NULL )
    {
        if(type&(META_ALBUM_ART|META_ALBUM_TXT))
        {
            MpdData *data2 = NULL;
            mpd_database_search_start(connection, TRUE);
            mpd_database_search_add_constraint(connection, MPD_TAG_ITEM_ARTIST, (tsong->artist)?tsong->artist:"");
            mpd_database_search_add_constraint(connection, MPD_TAG_ITEM_ALBUM,  (tsong->album)?tsong->album:""); 
            data2 = mpd_database_search_commit(connection);
            if(data2)
            {
                edited = data2->song;
                data2->song = NULL;
                mpd_data_free(data2);
            }
        }
        if(type&(META_ARTIST_ART|META_ARTIST_TXT))
        {
            MpdData *data2 = NULL;
            mpd_database_search_start(connection, TRUE);
            mpd_database_search_add_constraint(connection, MPD_TAG_ITEM_ARTIST, (tsong->artist)?tsong->artist:"");
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
        if(edited->album && edited->file)
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
        debug_printf(DEBUG_INFO, "string converted to: '%s'", edited->artist);
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

static void meta_data_set_cache_real(mpd_Song *song, MetaDataType type, MetaDataResult result, char *path)
{
	if(!song) return;
	/**
	 * Save the path for the album art
	 */
	if(type == META_ALBUM_ART) {
		if(song->artist && song->album) {
//			char *temp = g_strdup_printf("album:%s", song->album);
			if(result == META_DATA_AVAILABLE) {
				cfg_set_single_value_as_string_mm(cover_index, song->artist,song->album,NULL, "image",path);
			} else {
				cfg_set_single_value_as_string_mm(cover_index, song->artist,song->album,NULL, "image","");
			}
//			q_free(temp);
		}
	} else if(type == META_ALBUM_TXT) {
		if(song->artist && song->album)	{
//			char *temp = g_strdup_printf("albumtxt:%s", song->album);                   		
			if(result == META_DATA_AVAILABLE) {
				cfg_set_single_value_as_string_mm(cover_index, song->artist,song->album, NULL, "info",path);
			} else {
				cfg_set_single_value_as_string_mm(cover_index, song->artist,song->album, NULL, "info","");
			}                                                                        		
//			q_free(temp);
		}
	} else if (type == META_ARTIST_ART) {
		if(song->artist) {
			if(result == META_DATA_AVAILABLE) {
				cfg_set_single_value_as_string_mm(cover_index, song->artist,NULL, NULL, "image",path);
			} else {
				cfg_set_single_value_as_string_mm(cover_index, song->artist,NULL, NULL, "image","");
			}                                                                        		
		}
	} else if (type == META_ARTIST_TXT) {
		if(song->artist) {
			if(result == META_DATA_AVAILABLE) {
				cfg_set_single_value_as_string_mm(cover_index, song->artist,NULL, NULL, "biography",path);
			} else {
				cfg_set_single_value_as_string_mm(cover_index, song->artist,NULL, NULL, "biography","");
			}                                                                        		
		}
	} else if (type == META_ARTIST_SIMILAR) {
		if(song->artist) {
			if(result == META_DATA_AVAILABLE) {
				cfg_set_single_value_as_string_mm(cover_index, song->artist, NULL, NULL,"similar_artist",path);
			} else {
				cfg_set_single_value_as_string_mm(cover_index, song->artist,NULL, NULL, "similar_artist","");
			}                                                                        		
		}
	} else if (type == META_SONG_TXT)	{
		if(song->artist && song->title) {
			if(result == META_DATA_AVAILABLE) {
				cfg_set_single_value_as_string_mm(cover_index, song->artist,"lyrics", NULL, song->title,path);
			} else {
				cfg_set_single_value_as_string_mm(cover_index, song->artist, "lyrics", NULL, song->title,"");
			}                                                                        		
		}
	}else if (type == META_SONG_SIMILAR) {
		if(song->artist && song->title) {
			if(result == META_DATA_AVAILABLE) {
				cfg_set_single_value_as_string_mm(cover_index, song->artist,"similar_song",NULL, song->title,path);
			} else {
				cfg_set_single_value_as_string_mm(cover_index, song->artist,"similar_song",NULL, song->title,"");
			}                                                                        		
		}
	} 

}
void meta_data_set_cache(mpd_Song *song, MetaDataType type, MetaDataResult result, char *path)
{
    mpd_Song *edited = rewrite_mpd_song(song, type);
    meta_data_set_cache_real(edited, type, result, path);
    if(edited->artist)
    {
        if(strcmp(edited->artist, "Various Artists")!=0)
            meta_data_set_cache_real(song, type, result, path);
    }
    mpd_freeSong(edited);
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
		//gchar *temp = NULL;
		if(!song->artist || !song->album)
		{
			return META_DATA_UNAVAILABLE;	
		}
//		temp = g_strdup_printf("album:%s", song->album);
		*path = cfg_get_single_value_as_string_mm(cover_index,song->artist,song->album, NULL, "image");
//		q_free(temp);
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
//				temp = g_strdup_printf("album:%s", song->album);
				cfg_del_single_value_mm(cover_index, song->artist,song->album, NULL, "image");
//				q_free(temp);
				q_free(*path);
				*path = NULL;
				return META_DATA_FETCHING;	
			}
			return META_DATA_AVAILABLE;
		}

		/* else default to fetching */
	}
	/* Get values acording to type */
	else if(type == META_ARTIST_SIMILAR)
	{
		if(!song->artist) 
		{
			return META_DATA_UNAVAILABLE;	
		}
		*path = cfg_get_single_value_as_string_mm(cover_index,song->artist,NULL, NULL,  "similar_artist");
		if(*path)
		{
			/* if path length is NULL, then data unavailible */
			if(strlen(*path) == 0)
			{
				q_free(*path);

				*path = NULL;
				return META_DATA_UNAVAILABLE;	
			}
			return META_DATA_AVAILABLE;
		}

		/* else default to fetching */
	}

	else if(type == META_ALBUM_TXT)
	{
		//gchar *temp = NULL;
		if(!song->artist || !song->album)
		{
			return META_DATA_UNAVAILABLE;	
		}
//		temp = g_strdup_printf("albumtxt:%s", song->album);
		*path = cfg_get_single_value_as_string_mm(cover_index,song->artist, song->album, NULL, "info");
//		q_free(temp);
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
//				temp = g_strdup_printf("albumtxt:%s", song->album);
				cfg_del_single_value_mm(cover_index, song->artist,song->album, NULL, "info");
//				q_free(temp);
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
		//gchar *temp = NULL;
		if(!song->artist)
		{
			return META_DATA_UNAVAILABLE;	
		}
		*path = cfg_get_single_value_as_string_mm(cover_index,song->artist,NULL, NULL, "image");
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
				cfg_del_single_value_mm(cover_index, song->artist,NULL, NULL,"image");
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
		//gchar *temp = NULL;

		if(!song->artist)
		{
			return META_DATA_UNAVAILABLE;	
		}
		*path = cfg_get_single_value_as_string_mm(cover_index,song->artist,  NULL, NULL, "biography");
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
				cfg_del_single_value_mm(cover_index, song->artist, NULL, NULL,"biography");
				q_free(*path);
				*path = NULL;
				return META_DATA_FETCHING;	
			}
			/* return that data is availible */
			return META_DATA_AVAILABLE;
		}

	}
	else if(type == META_SONG_TXT)
	{
		//gchar *temp = NULL;
		if(!song->artist || !song->title)
		{
			return META_DATA_UNAVAILABLE;	
		}
		*path = cfg_get_single_value_as_string_mm(cover_index,song->artist,"lyrics",NULL, song->title);
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
				cfg_del_single_value_mm(cover_index, song->artist,"lyrics", NULL, song->title);
				q_free(*path);
				*path = NULL;
				return META_DATA_FETCHING;	
			}
			/* return that data is availible */
			return META_DATA_AVAILABLE;
		}	
	}
	else if(type == META_SONG_SIMILAR)
	{
        //char *temp;
		if(!song->artist && song->title) 
		{
			return META_DATA_UNAVAILABLE;	
		}

		*path = cfg_get_single_value_as_string_mm(cover_index,song->artist,"similar_song", NULL, song->title);
		if(*path)
		{
			/* if path length is NULL, then data unavailible */
			if(strlen(*path) == 0)
			{
				q_free(*path);

				*path = NULL;
				return META_DATA_UNAVAILABLE;	
			}
			return META_DATA_AVAILABLE;
		}

		/* else default to fetching */
	}
	return META_DATA_FETCHING;	
}

static void meta_data_retrieve_thread(void)
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
        debug_printf(DEBUG_INFO, "Got a request with id: %i, this is a request of type: %i",data->id, data->type);
		/* check if quit signal */
		if(data->id == 0)
		{
            debug_printf(DEBUG_INFO, "Quit command recieved.. quitting");
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
			data->result = meta_data_get_from_cache(data->edited/*song*/,data->type&META_QUERY_DATA_TYPES, &(data->result_path));
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
                gmpcPlugin *plug =  NULL;
                /* This is locked when sorting, because entry might be invalid 
                 * plug is always valid once obtained
                 */
                if(!meta_plugins_lock){
                    meta_plugins_lock = g_mutex_new();
                }
                g_mutex_lock(meta_plugins_lock);
                plug = meta_plugins[i];
                g_mutex_unlock(meta_plugins_lock);
                /* *
				 * Get image function is only allowed to return META_DATA_AVAILABLE or META_DATA_UNAVAILABLE
				 */
				if(gmpc_plugin_get_enabled(plug))
				{
                    debug_printf(DEBUG_INFO, "Query plugin: '%s'", plug->name);
					data->result = plug->metadata->get_image(data->edited, data->type&META_QUERY_DATA_TYPES, &path);
					data->result_path = path;
				}
			}
		}
		/** 
		 * update cache 
		 */
		meta_data_set_cache_real(data->edited, data->type&META_QUERY_DATA_TYPES, data->result, data->result_path);
        if(data->edited->artist)
        {
                if(strcmp(data->edited->artist, "Various Artists")!=0)
                    meta_data_set_cache_real(data->song, data->type&META_QUERY_DATA_TYPES, data->result, data->result_path);
        }

		/**
		 * Push the result back
		 */	
		q_async_queue_push(meta_results, data);		
        
		
		/**
		 * clear our reference to the object
		 */
		data = NULL;
		g_mutex_unlock(meta_processing);

        g_idle_add((GSourceFunc)meta_data_handle_results,NULL);
    }while(1);
}

static gboolean meta_data_handle_results(void)
{
	meta_thread_data *data = NULL;
    int test = 0;
	if(meta_thread == g_thread_self())
	{
		debug_printf(DEBUG_ERROR,"Crap, handled in wrong thread\n");

	}
    /**
	 *  Check if there are results to handle
	 *  do this until the list is clear
	 */
	for(data = q_async_queue_try_pop(meta_results);data;
			data = q_async_queue_try_pop(meta_results)) {	

		gmpc_meta_watcher_data_changed(gmw,data->song, (data->type)&META_QUERY_DATA_TYPES, data->result,data->result_path);
 		if(data->callback)
		{
			data->callback(data->song,data->result,data->result_path, data->data);
		}

        if(data->result_path)q_free(data->result_path);
        mpd_freeSong(data->song);
        mpd_freeSong(data->edited);
        q_free(data);
	}
	/**
	 * Keep the timer running
	 */
    /* update when handled */
    test = g_mutex_trylock(meta_processing);
    if(test)
        g_mutex_unlock(meta_processing);
    gmpc_meta_watcher_queue_size_changed(gmw, q_async_queue_true_length(meta_commands)+!test, num_queries);
    return FALSE;
}

/**
 * Initialize
 */
void meta_data_init(void)
{
    gchar *url = gmpc_get_covers_path(NULL); 
    if(!g_file_test(url,G_FILE_TEST_IS_DIR)){
        if(g_mkdir(url, 0700)<0){
            g_error("Cannot make %s\n", url);
        }
    }
    q_free(url);
    url = gmpc_get_covers_path("covers.db2");
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

    meta_processing = g_mutex_new();
    /**
     * Create the retrieval thread
     */
    meta_thread = g_thread_create((GThreadFunc)meta_data_retrieve_thread, NULL, TRUE, NULL);

}

void meta_data_add_plugin(gmpcPlugin *plug)
{
    if(!meta_plugins_lock){
        meta_plugins_lock = g_mutex_new();
    }
    g_mutex_lock(meta_plugins_lock);
    meta_num_plugins++;
    meta_plugins = g_realloc(meta_plugins,(meta_num_plugins+1)*sizeof(gmpcPlugin **));
    meta_plugins[meta_num_plugins-1] = plug;
    meta_plugins[meta_num_plugins] = NULL;
    g_mutex_unlock(meta_plugins_lock);
    meta_data_sort_plugins();
}

static void meta_data_sort_plugins(void)
{
    int i;
    int changed = FALSE;	
    if(!meta_plugins_lock){
        meta_plugins_lock = g_mutex_new();
    }
    g_mutex_lock(meta_plugins_lock);
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
    g_mutex_unlock(meta_plugins_lock);
}

void meta_data_cleanup(void)
{
    cfg_do_special_cleanup(cover_index);
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
    INIT_TIC_TAC();
    

    if(meta_thread)
    {
        debug_printf(DEBUG_INFO,"Waiting for meta thread to terminate...");
        /* remove old stuff */
        q_async_queue_lock(meta_commands);
        while((mtd = q_async_queue_try_pop_unlocked(meta_commands)))
        {
            mpd_freeSong(mtd->song);
            mpd_freeSong(mtd->edited);
            q_free(mtd);
        }
        /* Create the quit signal, this is just an empty request with id 0 */
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
    if(meta_processing) {
        g_mutex_free(meta_processing);
        meta_processing = NULL;
    }
    if(meta_commands){
        q_async_queue_unref(meta_commands);
        meta_commands = NULL;
    }
    /* Close the cover database  */
    TOC("test")
    cfg_close(cover_index);
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
 * Function called by the "client" 
 */
MetaDataResult meta_data_get_path(mpd_Song *tsong, MetaDataType type, gchar **path,MetaDataCallback callback, gpointer data)
{
    MetaDataResult ret;
    meta_thread_data *mtd = NULL;
    mpd_Song *song =NULL;
    guint id = 0;
    int test = 0;
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
        /**
         * If the data is know. (and doesn't need fectching) 
         * call the callback and stop
         */
        if(ret != META_DATA_FETCHING)
        {
            return ret;	
        }
    }

    mtd = g_malloc0(sizeof(*mtd));
    /**
     * Make a copy
     */
    mtd->edited = rewrite_mpd_song(tsong, type);

    /** 
     * Query cache, but for changed artist name 
     */
    if((type&META_QUERY_NO_CACHE) == 0)
    {
        ret = meta_data_get_from_cache(mtd->edited, type&META_QUERY_DATA_TYPES, path);
        /**
         * If the data is know. (and doesn't need fectching) 
         * call the callback and stop
         */
        if(ret != META_DATA_FETCHING)
        {
            /* store it under the original */
            meta_data_set_cache_real(tsong, type, ret, *path);
            mpd_freeSong(mtd->edited);
            q_free(mtd);
            return ret;	
        }
    }


     /**
     * If no result, start a thread and start fetching the data from there
     */

   
    /**
     * unique id 
     * Not needed, but can be usefull for debugging
     */
    mtd->song = mpd_songDup(tsong);

    id = mtd->id = g_random_int_range(1,2147483647);
    mtd->type = type;
    mtd->callback = callback;
    mtd->data = data;
    /**
     * Check if request is allready in queue
     */
    /* when using old api style, the request is commited anyway */
    if(!callback)
    {
        /* if it is allready in the queue, there is no need to push it again
         * because GmpcMetaWatcher signal will arrive at every widget
         */
        /*
         * TODO: this misses items currently in the queue, try to catch that too.
         */
        q_async_queue_lock(meta_commands);
        if(q_async_queue_has_data(meta_commands,(GCompareFunc)meta_compare_func, mtd))
        {
            q_async_queue_unlock(meta_commands);
            mpd_freeSong(mtd->song);
            mpd_freeSong(mtd->edited);
            g_free(mtd);
            return ret;
        }
        q_async_queue_unlock(meta_commands);
    }

    /** push it to the other thread */

    num_queries ++;
    /* I should fix this */
    test = g_mutex_trylock(meta_processing);
    if(test)
        g_mutex_unlock(meta_processing);
    gmpc_meta_watcher_queue_size_changed(gmw, q_async_queue_true_length(meta_commands)+!test, num_queries);

    q_async_queue_push(meta_commands, mtd);
    /** clean reference to pointer, it's now to the other thread */
    mtd = NULL;

    return ret;
}

static gchar * strip_invalid_chars(gchar *input)
{
    int i = 0;
    int length = strlen(input);
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
    const gchar *homedir = g_get_home_dir();
    g_assert(type < META_QUERY_DATA_TYPES); 
    {
        gchar *filename = NULL, *dirname = NULL;
        const gchar *extention= (type&(META_ALBUM_TXT|META_ARTIST_TXT|META_SONG_TXT))?"txt":((ext == NULL)?"":ext);
        g_assert(song->artist != NULL);

        /* Convert it so the filesystem likes it */
        /* TODO: Add error checking */
        dirname = g_filename_from_utf8(song->artist,-1,NULL,NULL,NULL); 
        dirname = strip_invalid_chars(dirname);
        retv = g_build_path(G_DIR_SEPARATOR_S, homedir,METADATA_DIR, dirname,NULL);
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
            filename = g_strdup_printf("%s.%s", temp,extention);
            g_free(temp);
        }else if(type&META_ARTIST_ART){
            filename = g_strdup_printf("artist_IMAGE.%s", extention);
        }else if (type&META_ARTIST_TXT){
            filename = g_strdup_printf("artist_BIOGRAPHY.%s", extention);
        }else if (type&META_SONG_TXT) {
            gchar *temp ;
            g_assert(song->title != NULL);
            temp =g_filename_from_utf8(song->title,-1,NULL,NULL,NULL); 
            filename = g_strdup_printf("%s_LYRIC.%s", temp,extention);
            g_free(temp);
        }
        filename = strip_invalid_chars(filename);
        retv = g_build_path(G_DIR_SEPARATOR_S, homedir,METADATA_DIR, dirname,filename,NULL);
        g_free(filename);
        g_free(dirname);
    }
    return retv;
}
static void metadata_pref_priority_changed(GtkCellRenderer *renderer, char *path, char *new_text, GtkListStore *store)
{
    GtkTreeIter iter;
    if(gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(store), &iter, path))
    {
        gmpcPlugin *plug;
        gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, 0, &plug, -1);
        if(plug)
        {
            plug->metadata->set_priority((gint)g_ascii_strtoull(new_text, NULL, 0));
            gtk_list_store_set(GTK_LIST_STORE(store), &iter, 2, plug->metadata->get_priority(),-1);
            meta_data_sort_plugins();
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
            2, meta_plugins[i]->metadata->get_priority(),
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



void metadata_import_old_db(char *url)
{
    config_obj *old = cfg_open(url);
    if(old)
    {
        conf_mult_obj *mult, *iter;

        mult = cfg_get_class_list(old);
        for(iter = mult; iter; iter = iter->next)
        {
            char *artist = iter->key;
            if(artist)
            {
                mpd_Song *song = mpd_newSong();
                conf_mult_obj *cm , *citer;
                cm = cfg_get_key_list(old, artist);
                song->artist = artist;
                for(citer = cm; citer; citer = citer->next)
                {
                    /* Artist only */
                    if(strcmp(citer->key, "image") == 0)
                    {
                        meta_data_set_cache_real(song, 
                                META_ARTIST_ART, 
                                (strlen(citer->value) > 0)?META_DATA_AVAILABLE:META_DATA_UNAVAILABLE, citer->value);  
                    }else if (strcmp(citer->key, "similar") == 0)
                    {
                        meta_data_set_cache_real(song, 
                                META_ARTIST_SIMILAR, 
                                (strlen(citer->value) > 0)?META_DATA_AVAILABLE:META_DATA_UNAVAILABLE, citer->value);  
                    }else if (strcmp(citer->key, "biography") == 0)
                    {
                        meta_data_set_cache_real(song, 
                                META_ARTIST_TXT, 
                                (strlen(citer->value) > 0)?META_DATA_AVAILABLE:META_DATA_UNAVAILABLE, citer->value);  
                    }
                    /* Album */
                    else if(strncmp(citer->key, "album:",6) == 0)
                    {
                        song->album = &((citer->key)[6]);
                        meta_data_set_cache_real(song, 
                                META_ALBUM_ART, 
                                (strlen(citer->value) > 0)?META_DATA_AVAILABLE:META_DATA_UNAVAILABLE, citer->value);  
                    }
                    else if (strncmp(citer->key, "albumtxt:", 9) == 0)
                    {
                        song->album = &((citer->key)[9]);
                        meta_data_set_cache_real(song, 
                                META_ALBUM_TXT, 
                                (strlen(citer->value) > 0)?META_DATA_AVAILABLE:META_DATA_UNAVAILABLE, citer->value);  
                    }
                    /* song */
                    else if (strncmp(citer->key, "lyrics:",7) == 0) 
                    {
                        song->title = &((citer->key)[7]);
                        meta_data_set_cache_real(song, 
                                META_SONG_TXT,
                                (strlen(citer->value) > 0)?META_DATA_AVAILABLE:META_DATA_UNAVAILABLE, citer->value);  
                    }else if (strncmp(citer->key, "similar:", 8) == 0)
                    {
                        song->title = &((citer->key)[8]);
                        meta_data_set_cache_real(song, 
                                META_SONG_SIMILAR,
                                (strlen(citer->value) > 0)?META_DATA_AVAILABLE:META_DATA_UNAVAILABLE, citer->value);  
                    }

                    song->album = NULL;
                    song->title = NULL;
                }
                song->artist = NULL;
                mpd_freeSong(song);
                cfg_free_multiple(cm);
            }
        }
        if(mult)
            cfg_free_multiple(mult);
        cfg_close(old); 
    }
}


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



