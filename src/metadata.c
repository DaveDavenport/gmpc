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

#include <string.h>
#include <glib.h>
#include <glib/gstdio.h>
#include "main.h"

#include "metadata.h"
#include "gmpc_easy_download.h"

config_obj *cover_index= NULL;
int meta_num_plugins=0;
GMutex *meta_plugins_lock = NULL;
gmpcPluginParent **meta_plugins = NULL;
static void meta_data_sort_plugins(void);
GList *process_queue = NULL;
static MetaDataResult meta_data_get_from_cache(mpd_Song *song, MetaDataType type, char **path);
/**
 * This is queue is used to send commands to the retrieval queue
 */
GAsyncQueue *meta_commands = NULL;
/**
 * This queue is used to send replies back.
 */
GAsyncQueue *meta_results = NULL;


typedef struct {
	guint id;

	MetaDataCallback callback;
	/* Data */
	mpd_Song *song;
        mpd_Song *edited;
        MetaDataType type;
	/* Resuls  */
	MetaDataResult result;
	char *result_path;
	/* Callback */
	gpointer data;
    int index;
    GList *list;
    GList *iter;
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
            else if(tsong->albumartist)
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
    printf("saving: %i %s::%s\n", type, song->artist,path);
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


static MetaDataResult meta_data_get_from_cache(mpd_Song *song, MetaDataType type, char **path)
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


static gboolean meta_data_handle_results(void)
{
	meta_thread_data *data = NULL;
    printf("replying result\n");
    /**
	 *  Check if there are results to handle
	 *  do this until the list is clear
	 */
	for(data = g_async_queue_try_pop(meta_results);data;
			data = g_async_queue_try_pop(meta_results)) {	
        printf("result: %s::%i::%s\n",data->song->artist, data->result, data->result_path);
		gmpc_meta_watcher_data_changed(gmw,data->song, (data->type)&META_QUERY_DATA_TYPES, data->result,data->result_path);
 		if(data->callback)
		{
			data->callback(data->song,data->result,data->result_path, data->data);
		}

        if(data->result_path)q_free(data->result_path);
        if(data->song)
            mpd_freeSong(data->song);
        mpd_freeSong(data->edited);
        q_free(data);
	}

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
    meta_commands = g_async_queue_new();
    /**
     * the result queue
     */
    meta_results = g_async_queue_new();

    /**
     * Create the retrieval thread
     */
}

void meta_data_add_plugin(gmpcPluginParent *plug)
{
    if(!meta_plugins_lock){
        meta_plugins_lock = g_mutex_new();
    }
    g_mutex_lock(meta_plugins_lock);
    meta_num_plugins++;
    meta_plugins = g_realloc(meta_plugins,(meta_num_plugins+1)*sizeof(gmpcPluginParent **));
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
            if(gmpc_plugin_metadata_get_priority(meta_plugins[i]) > gmpc_plugin_metadata_get_priority(meta_plugins[i+1]))
            {
                gmpcPluginParent *temp = meta_plugins[i];
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
    
    {
        debug_printf(DEBUG_INFO,"Waiting for meta thread to terminate...");
        /* remove old stuff */
        g_async_queue_lock(meta_commands);
        while((mtd = g_async_queue_try_pop_unlocked(meta_commands)))
        {
        }
        /* Create the quit signal, this is just an empty request with id 0 */
        mtd = g_malloc0(sizeof(*mtd));
        mtd->id = 0;

        /* push the request to the thread */
        g_async_queue_push_unlocked(meta_commands, mtd);
        g_async_queue_unlock(meta_commands);
        /* cleanup */
        g_free(mtd);
        if(process_queue) {
            GList *iter;
            for(iter = g_list_first(process_queue); iter; iter = iter->next)
            {
                mtd = iter->data;
                g_list_foreach(mtd->list, (GFunc)g_free, NULL);
                g_list_free(mtd->list);
                mtd->list = mtd->iter = NULL;
                mpd_freeSong(mtd->song);
                mpd_freeSong(mtd->edited);
                g_free(mtd);
            }
            g_list_free(process_queue);
            process_queue = NULL;
        }

        debug_printf(DEBUG_INFO,"Done..");
    }
    if(meta_commands){
        g_async_queue_unref(meta_commands);
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
 * new things *
 */

static gboolean process_itterate(void);
/* TODO remove this and wrap this */
typedef struct _gmpcPluginParent {
    gmpcPlugin *old;
    GmpcPluginBase *new;
}_gmpcPluginParent;
static void metadata_download_handler(const GEADAsyncHandler *handle, GEADStatus status, gpointer user_data)
{
    meta_thread_data *d = user_data;
    if(status == GEAD_PROGRESS) {
        return;
    }
    if(status == GEAD_DONE)
    {
        gchar *filename = gmpc_get_metadata_filename(d->type&(~META_QUERY_NO_CACHE), d->song, NULL); 
        goffset length;
        const gchar *data = gmpc_easy_handler_get_data(handle, &length);
        g_file_set_contents(filename, data, length, NULL);
        d->result = META_DATA_AVAILABLE;
        d->result_path = filename;

        g_list_foreach(d->list,(GFunc) g_free, NULL);
        g_list_free(d->list);
        d->list = NULL;
        d->iter = NULL;
        d->index = meta_num_plugins;
        printf("Saving filename: %i::%s\n", d->result,filename);
        process_itterate();
    }else{
        if(d->iter)
        {
            d->iter = g_list_next(d->iter);
            if(d->iter){
                gmpc_easy_async_downloader((const gchar *)d->iter->data, metadata_download_handler, d);
                return;
            }
        }
        g_list_foreach(d->list,(GFunc) g_free, NULL);
        g_list_free(d->list);
        d->list = NULL;
        d->iter = NULL;
        d->index++;
        process_itterate();
    }
}
static void result_itterate(GList *list, gpointer user_data)
{
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
        return;
    }

    /**
     * Plugin has a result, try the downloading the first, if that fails
     */
    d->list = list;
    /* Store the first one in the list */
    d->iter = g_list_first(list);

    /* If type is ART we need to download something. */
    if(d->type&(META_ARTIST_ART|META_ALBUM_ART))
    {
        const char *path = d->iter->data;

        printf("New async downloading: %s\n", (const gchar *)d->iter->data);
        if(path[0] == '/') {
            d->result_path = (char *)path; 
            d->iter->data = NULL;
            d->result = META_DATA_AVAILABLE;
        }else{
            gmpc_easy_async_downloader((const gchar *)d->iter->data, metadata_download_handler, d);
            return;
        }
    
    }
    /* If type is text, store this in a file. */
    else if (d->type&(META_ALBUM_TXT|META_ARTIST_TXT|META_SONG_TXT))
    {
        GError *error = NULL;
        gchar *filename = gmpc_get_metadata_filename(d->type&(~META_QUERY_NO_CACHE), d->song, NULL); 
        g_file_set_contents(filename,(char *)d->iter->data, -1, &error);
        if(error)
        {
            printf("Error: %s\n", error->message);
            g_error_free(error);
            error = NULL;
        }
        else{
            /* If saved succesfull, pass filename back to handler */
            d->result_path = filename; 
            d->result = META_DATA_AVAILABLE;
        }
	/* similar are stored in db */
    }else {
	    char *path = d->iter->data;
	    d->result_path = (char *)path; 
        d->iter->data = NULL;
        d->result = META_DATA_AVAILABLE;
    }	

    /* Free the list. */
    if(d->list)
    {
        g_list_foreach(d->list, (GFunc)g_free, NULL);
        g_list_free(d->list);
    }
    d->list = NULL;
    d->iter = NULL;

    /**
     * If still no result, try next plugin
     */
    if(d->result == META_DATA_UNAVAILABLE)
    {
        d->index++;
    }
    /**
     * If we have a result.
     * Make it not check other plugins 
     */
    else
        d->index = meta_num_plugins;

    g_idle_add((GSourceFunc)process_itterate, NULL);
}

static gboolean process_itterate(void)
{

    meta_thread_data *d;
    printf("Process itterate\n");
    /* If queue is empty, do nothing */
    if(process_queue == NULL){ 
        printf("no more in queue\n");
        return FALSE;
    }

    /**
     * Get the top of the list
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
            if(d->type&META_QUERY_NO_CACHE)
            {
                d->result = META_DATA_UNAVAILABLE;
            } else {
                d->result = meta_data_get_from_cache(d->edited,d->type&META_QUERY_DATA_TYPES, &(d->result_path));
            }
        }
        /**
         * If cache has no reesult, query plugin
         */
        if(d->result != META_DATA_AVAILABLE) 
        {
            gmpcPluginParent *plug = meta_plugins[d->index];
    
            printf("Calling: %s\n", gmpc_plugin_get_name(plug));
            /* TODO WRAP THIS */
            /**
             * Query plugins, new type call in this thread.
             * old type, create new thread. 
             */
            if(gmpc_plugin_get_enabled(plug) && plug->old->metadata->get_uris != NULL)
            {
                gmpc_plugin_metadata_query_metadata_list(plug, d->edited, d->type&META_QUERY_DATA_TYPES, result_itterate, (gpointer)d);
            }
            else
            {
                d->index++;
                g_idle_add((GSourceFunc)process_itterate, NULL);
            }
            return FALSE;
        }
    }
    /**
     * If nothing found, set unavailable
     */
    if(d->result == META_DATA_FETCHING) {
        d->result = META_DATA_UNAVAILABLE;
    }
    /**
     * Store result (or lack off) 
     **/
    meta_data_set_cache_real(d->edited, d->type&META_QUERY_DATA_TYPES, d->result, d->result_path);
    if(d->edited->artist)
    {
        if(strcmp(d->edited->artist, "Various Artists")!=0)
            meta_data_set_cache_real(d->song, d->type&META_QUERY_DATA_TYPES, d->result, d->result_path);
    }
    /**
     * Remove from queue
     * TODO: try to match identical queries?
     */
    process_queue = g_list_remove(process_queue, d);
    if(process_queue){
        GList *iter = g_list_first(process_queue);
        for(;iter;iter = g_list_next(iter))
        {
            meta_thread_data *d2 = iter->data;
            if(!meta_compare_func(d,d2)){
                printf("removing double query \n");
                /* Remove from intput list */
                iter = process_queue = g_list_remove(process_queue, d2);
                /* if there is no old-type callback, only once is sufficient */
                if(d2->callback)
                {
                    printf("Old fashion callback, push back in meta_result\n");
                    /* Copy result */
                    d2->result = d->result;
                    d2->result_path = (d->result_path)?g_strdup(d->result_path):NULL;
                    /* put result back */
                    g_async_queue_push(meta_results, d2);		
                }else{
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
    g_async_queue_push(meta_results, d);		
    /**
     * Call resuult handler 
     */
    g_idle_add((GSourceFunc)meta_data_handle_results,NULL);

    /**
     * Next in queue 
     */
    g_idle_add((GSourceFunc)process_itterate, NULL);
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
    mtd->index = 0;
    mtd->result = META_DATA_UNAVAILABLE;
    mtd->result_path = NULL;
  
    printf("adding to queue\n");
    if(process_queue == NULL) {
        process_queue = g_list_append(process_queue, mtd);
        g_idle_add((GSourceFunc)process_itterate, NULL);
        return META_DATA_FETCHING;
    }
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
    const gchar *homedir = g_get_home_dir();
    g_assert(song->artist != NULL);
    g_assert(type < META_QUERY_DATA_TYPES); 
    {
        GError *error = NULL;
        gchar *filename = NULL, *dirname = NULL;
        const gchar *extention= (type&(META_ALBUM_TXT|META_ARTIST_TXT|META_SONG_TXT))?"txt":((ext == NULL)?((type&(META_ALBUM_ART|META_ARTIST_ART))?"jpg":""):ext);
        g_assert(song->artist != NULL);

        /* Convert it so the filesystem likes it */
        /* TODO: Add error checking */

        dirname = g_filename_from_utf8(song->artist, -1, NULL, NULL, NULL);
        /*
        if (g_get_charset (&charset))
        {
            debug_printf(DEBUG_INFO, "Locale is utf-8, just copying");
            dirname = g_strdup(song->artist);
        }else{
            debug_printf(DEBUG_INFO, "Locale is %s converting to UTF-8", charset);
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
            debug_printf(DEBUG_ERROR, "Failed to convert %s to file encoding. '%s'", song->artist, error->message);
            g_error_free(error);
            if(dirname) g_free(dirname);
            dirname = g_strdup("invalid");
        }
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
        if(gmpc_plugin_get_enabled(meta_plugins[i]))
        {
            gtk_list_store_insert_with_values(store, &iter, -1, 
                    0, meta_plugins[i],
                    1, gmpc_plugin_get_name(meta_plugins[i]),
                    2, gmpc_plugin_metadata_get_priority(meta_plugins[i]),
                    -1);
        }
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
    metadata_get_list_itterate(NULL, data);
    return FALSE;
}
static void metadata_get_list_itterate(GList *list, gpointer data)
{
    MLQuery *q = (MLQuery *)data;
    q->calls--;
    if(q->cancel){
            printf("Cancel\n");
            if(list){
                g_list_foreach(list,(GFunc) g_free, NULL);
                g_list_free(list);
            }
            if(q == 0)
            {
                printf("Cleaning\n");
                mpd_freeSong(q->song);
                g_free(q);
            }
            /*  clean up */
            return; 
    }
    if(list) {
        gmpcPluginParent *plug = meta_plugins[q->index-1]; 
        q->callback(q,gmpc_plugin_get_name(plug), list,q->userdata);
        g_list_foreach(list,(GFunc) g_free, NULL);
        g_list_free(list);
    }
    if(q->index < meta_num_plugins)
    {
        gmpcPluginParent *plug = meta_plugins[q->index]; 
        q->index++;
        printf("Query plugin: %s\n", gmpc_plugin_get_name(plug));
        if(gmpc_plugin_get_enabled(plug) && plug->old && plug->old->metadata && plug->old->metadata->get_uris){
            q->calls++;
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
    g_idle_add(metadata_get_list_itterate_idle, q);
    return q;
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
