#include <stdio.h>
#include <glib.h>
#include <glib/gstdio.h>
#include "main.h"
#include "metadata.h"
#include "config1.h"
#include "metadata_cache.h"

#define CACHE_NAME "Metadata cache"

/**
TODO: Find a  way to store the type of the data in the config file. (met->content_type)
*/
config_obj *cover_index;

/**
 * Checking the cache

 */

static MetaDataResult meta_data_get_cache_uri(mpd_Song *song, MetaData **met)
{
    gchar *path = NULL;
    if((*met)->type == META_ALBUM_ART){
        path = cfg_get_single_value_as_string_mm(cover_index,song->artist,song->album, NULL, "image");
    }else if((*met)->type == META_ALBUM_TXT){
		path = cfg_get_single_value_as_string_mm(cover_index,song->artist, song->album, NULL, "info");
    }else if ((*met)->type == META_ARTIST_ART) {
		path = cfg_get_single_value_as_string_mm(cover_index,song->artist,NULL, NULL, "image");
    }else if ((*met)->type == META_ARTIST_TXT) {
		path = cfg_get_single_value_as_string_mm(cover_index,song->artist,  NULL, NULL, "biography");
    }else if ((*met)->type == META_SONG_TXT) {
		path = cfg_get_single_value_as_string_mm(cover_index,song->artist,"lyrics",NULL, song->title);
    }
    if(path)
    {
        /* if path length is NULL, then data unavailible */
        if(strlen(path) == 0)
        {
            q_free(path);
            return META_DATA_UNAVAILABLE;	
        }
        /* return that data is availible */
        if(!g_file_test(path, G_FILE_TEST_EXISTS))
        {
            if((*met)->type == META_ALBUM_ART){
                cfg_del_single_value_mm(cover_index,song->artist,song->album, NULL, "image");
              //  cfg_del_single_value_mm(cover_index,song->artist,song->album, NULL, "image_type");
            }else if((*met)->type == META_ALBUM_TXT){
                cfg_del_single_value_mm(cover_index,song->artist, song->album, NULL, "info");
              //  cfg_del_single_value_mm(cover_index,song->artist,song->album, NULL, "info_type");
            }else if ((*met)->type == META_ARTIST_ART) {
                cfg_del_single_value_mm(cover_index,song->artist,NULL, NULL, "image");
              //  cfg_del_single_value_mm(cover_index,song->artist,song->album, NULL, "image_type");
            }else if ((*met)->type == META_ARTIST_TXT) {
                cfg_del_single_value_mm(cover_index,song->artist,  NULL, NULL, "biography");
              //  cfg_del_single_value_mm(cover_index,song->artist,song->album, NULL, "biography_type");
            }else if ((*met)->type == META_SONG_TXT) {
                cfg_del_single_value_mm(cover_index,song->artist,"lyrics",NULL, song->title);
              //  cfg_del_single_value_mm(cover_index,song->artist,"lyrics_type",NULL, song->title);
            }
            q_free(path);
            return META_DATA_FETCHING;	
        }
        (*met)->content_type = META_DATA_CONTENT_URI;
        (*met)->content =path; (*met)->size = -1;
        return META_DATA_AVAILABLE;
    }
    return META_DATA_FETCHING;
}

static MetaDataResult meta_data_get_cache_list(mpd_Song *song, MetaData **met)
{
    gchar *list = cfg_get_single_value_as_string_mm(cover_index,song->artist,NULL, NULL,  "similar_artist");
    if(list)
    {
        GList *rlist = NULL;
        gchar **result = NULL;
        int i;
        if(strlen(list) == 0){
            g_free(list);
            return META_DATA_UNAVAILABLE;
        }
        result = g_strsplit(list, "\n", 0);
        for(i=0; result && result[i];i++)
        {
            rlist = g_list_prepend(rlist,g_strdup(result[i]));
        }
        if(result) g_strfreev(result);
        (*met)->content = rlist;
        (*met)->content_type = META_DATA_CONTENT_TEXT_LIST;
        return META_DATA_AVAILABLE;
    }
    return META_DATA_FETCHING;
}


MetaDataResult meta_data_get_from_cache(mpd_Song *song, MetaDataType type, MetaData **met)
{
    (*met) = meta_data_new();
    (*met)->type = type;
    (*met)->content_type = META_DATA_CONTENT_EMPTY;
    (*met)->plugin_name = CACHE_NAME;
    if(!song)
    {
		return META_DATA_UNAVAILABLE;	
	}
	/* Get values acording to type */
	if(type == META_ALBUM_ART)
	{
		if(!song->artist || !song->album)
		{
			return META_DATA_UNAVAILABLE; 
		}

        return meta_data_get_cache_uri(song, met);	
        /* else default to fetching */
	}
	/* Get values acording to type */
	else if(type == META_ARTIST_SIMILAR)
	{
    
		if(!song->artist)
		{
			return META_DATA_UNAVAILABLE;	
		}
        /* else default to fetching */
        return meta_data_get_cache_list(song, met);
	}

	else if(type == META_ALBUM_TXT)
	{
		if(!song->artist || !song->album)
		{
			return META_DATA_UNAVAILABLE;	
		}
        return meta_data_get_cache_uri(song, met);	
	}
	else if (type == META_ARTIST_ART)
	{
		if(!song->artist)
		{
			return META_DATA_UNAVAILABLE;	
		}
        return meta_data_get_cache_uri(song, met);	
    }
	else if (type == META_ARTIST_TXT)
	{
        if(!song->artist)
		{
			return META_DATA_UNAVAILABLE;	
		}
        return meta_data_get_cache_uri(song, met);	
	}
	else if(type == META_SONG_TXT)
	{
		if(!song->artist || !song->title)
		{
			return META_DATA_UNAVAILABLE;	
		}
        return meta_data_get_cache_uri(song, met);	
	}
	else if(type == META_SONG_SIMILAR)
	{
        char *path = NULL;
		if(!song->artist && song->title)
		{
			return META_DATA_UNAVAILABLE;	
		}

		path = cfg_get_single_value_as_string_mm(cover_index,song->artist,"similar_song", NULL, song->title);
		if(path)
		{
			/* if path length is NULL, then data unavailible */
			if(strlen(path) == 0)
			{
				q_free(path);
				return META_DATA_UNAVAILABLE;	
			}

            (*met)->content_type = META_DATA_CONTENT_RAW;
            (*met)->content = path; (*met)->size = -1;
			return META_DATA_AVAILABLE;
		}

		/* else default to fetching */
	}
	return META_DATA_FETCHING;	
}

static void meta_data_set_cache_meta_data(const char *a, const char *b, const char *c, const char*d, MetaData *met)
{
    /* Set unavailable */
    if(met == NULL || met->content_type == META_DATA_CONTENT_EMPTY)
    {
        cfg_set_single_value_as_string_mm(cover_index, a,b,c,d,"");
    }
    /* Set Available for different types */ 
    else if(met->content_type == META_DATA_CONTENT_URI) 
    {
        const char *data =meta_data_get_uri(met);
        cfg_set_single_value_as_string_mm(cover_index, a,b,c,d,data);
    }
    else if(met->content_type == META_DATA_CONTENT_TEXT_LIST) 
    {
        GString *string = g_string_new("");
        GList *iter = g_list_first((GList *)meta_data_get_text_list(met));
        for(;iter;iter = g_list_next(iter)){
            string = g_string_append(string, iter->data);
            if(iter->next) string = g_string_append(string, "\n");
        }
        cfg_set_single_value_as_string_mm(cover_index, a,b,c,d,string->str);
        g_string_free(string,TRUE);
    }else if (met->content_type == META_DATA_CONTENT_TEXT_VECTOR)
    {
        const gchar **vector = meta_data_get_text_vector(met);
        gchar *cvector = g_strjoinv("\n", (gchar **)vector);
        cfg_set_single_value_as_string_mm(cover_index, a,b,c,d,cvector);
        g_free(cvector);
    }else if (met->content_type == META_DATA_CONTENT_TEXT)
    {
        const char *data = meta_data_get_text(met);
        cfg_set_single_value_as_string_mm(cover_index, a,b,c,d,data);
    }else if (met->content_type == META_DATA_CONTENT_HTML)
    {
        const char *data = meta_data_get_html(met);
        cfg_set_single_value_as_string_mm(cover_index, a,b,c,d,data);
    }
    else if (met->content_type == META_DATA_CONTENT_RAW)
    {
        gsize length=0;
        const guchar *data = meta_data_get_raw(met, &length);
        gchar *encoded = g_base64_encode(data, length);
        cfg_set_single_value_as_string_mm(cover_index, a,b,c,d,encoded);
        g_free(encoded);
    }
    else
    {
        g_error("Unkown metadata type, cannot store");
    }
}

void meta_data_set_cache_real(mpd_Song *song, MetaDataResult result, MetaData *met)
{
	if(!song) return;
	/**
	 * Save the path for the album art
	 */
	if(met->type == META_ALBUM_ART) {
		if(song->artist && song->album) {
			if(result == META_DATA_AVAILABLE) {
                meta_data_set_cache_meta_data(song->artist, song->album, NULL, "image", met);
			} else {
                meta_data_set_cache_meta_data(song->artist, song->album, NULL, "image", NULL);
			}
		}
	} else if(met->type == META_ALBUM_TXT) {
		if(song->artist && song->album)	{
			if(result == META_DATA_AVAILABLE) {
				meta_data_set_cache_meta_data(song->artist,song->album, NULL, "info",met);
			} else {
				meta_data_set_cache_meta_data(song->artist,song->album, NULL, "info",NULL);
			}                                                                        		
		}
	} else if (met->type == META_ARTIST_ART) {
		if(song->artist) {
			if(result == META_DATA_AVAILABLE) {
				meta_data_set_cache_meta_data(song->artist,NULL, NULL, "image",met);
			} else {
				meta_data_set_cache_meta_data(song->artist,NULL, NULL, "image",NULL);
			}                                                                        		
		}
	} else if (met->type == META_ARTIST_TXT) {
		if(song->artist) {
			if(result == META_DATA_AVAILABLE) {
				meta_data_set_cache_meta_data(song->artist,NULL, NULL, "biography",met);
			} else {
                meta_data_set_cache_meta_data(song->artist,NULL, NULL, "biography",NULL);
            }                                                                        		
		}
	} else if (met->type == META_ARTIST_SIMILAR) {
		if(song->artist) {
			if(result == META_DATA_AVAILABLE) {
				meta_data_set_cache_meta_data(song->artist,NULL, NULL, "similar_artist",met);
			} else {
				meta_data_set_cache_meta_data(song->artist,NULL, NULL, "similar_artist",NULL);
			}                                                                        		
		}
	} else if (met->type == META_SONG_TXT)	{
		if(song->artist && song->title) {
			if(result == META_DATA_AVAILABLE) {
                meta_data_set_cache_meta_data(song->artist,"lyrics", NULL, song->title,met);
            } else {
                meta_data_set_cache_meta_data(song->artist, "lyrics", NULL, song->title,NULL);
            }                                                                        		
		}
	}else if (met->type == META_SONG_SIMILAR) {
		if(song->artist && song->title) {
			if(result == META_DATA_AVAILABLE) {
                /* TODO FIX THIS */
				cfg_set_single_value_as_string_mm(cover_index, song->artist,"similar_song",NULL, song->title,met->content);
			} else {
				cfg_set_single_value_as_string_mm(cover_index, song->artist,"similar_song",NULL, song->title,"");
			}                                                                        		
		}
	}

}
void meta_data_set_cache(mpd_Song *song, MetaDataResult result, MetaData *met)
{
    mpd_Song *edited = rewrite_mpd_song(song, met->type);
    meta_data_set_cache_real(edited, result, met);
    if(edited->artist)
    {
        if(strcmp(edited->artist, "Various Artists")!=0)
            meta_data_set_cache_real(song, result, met);
    }
    mpd_freeSong(edited);
}

void metadata_import_old_db(char *url)
{
/*
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
    */
}

void metadata_cache_init(void)
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

}


void metadata_cache_cleanup(void)
{
    cfg_do_special_cleanup(cover_index);
}
void metadata_cache_destroy(void)
{
    cfg_close(cover_index);
}
