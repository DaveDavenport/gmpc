#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "main.h"
#include "plugin.h"


typedef struct _ca_dl {
	struct _ca_dl *next;
	struct _ca_dl *previous;
	gmpcPlugin *plug;
	qthread *qt;
	GQueue *function;
	mpd_Song *song;
	int retval;
}ca_dl;

typedef struct _ca_callback{
	CoverArtCallback function;
	gpointer userdata;
} ca_callback;
GList *fetch_que_list = NULL;

config_obj *cover_index= NULL;


CoverArtResult cover_art_fetch_image_path_real(mpd_Song *song, gchar **path, gboolean cache)
{
	int i=0;
	int priority = 1000;
	int can_try = 0;
	if(!cfg_get_single_value_as_int_with_default(config, "cover-art", "enable",TRUE)) {
		return COVER_ART_NO_IMAGE;
	}
	if(song == NULL) {
		return COVER_ART_NO_IMAGE;
	}

	if(song->artist && song->album && cache){
		gchar *cipath = cfg_get_single_value_as_string(cover_index, song->artist, song->album);
		debug_printf(DEBUG_INFO,"query cover art cache");
		if(cipath)
		{
			if(strlen(cipath) == 0)
			{
				return COVER_ART_NO_IMAGE;
			}
			*path = g_strdup(cipath);
			return COVER_ART_OK_LOCAL;
		}
	}
	for(i =  0; plugins[i] != NULL; i++)
	{
		if(plugins[i]->plugin_type == GMPC_PLUGIN_COVER_ART)
		{
			if(plugins[i]->coverart->fetch_image_path != NULL)
			{
				char *temp_path= NULL;
				int retv = plugins[i]->coverart->fetch_image_path(song, &temp_path);
				if(retv == COVER_ART_OK_LOCAL || retv == COVER_ART_OK_REMOTE)
				{
					if(priority > plugins[i]->coverart->get_priority())
					{
						if(*path){
							g_free(*path);
							*path = NULL;
						}
						*path = temp_path;
						priority = plugins[i]->coverart->get_priority();
					}
					debug_printf(DEBUG_INFO,"%s has image \n", plugins[i]->name);
				}
				else if(retv == COVER_ART_NOT_FETCHED)
				{
					debug_printf(DEBUG_INFO,"%s can try \n", plugins[i]->name);
					can_try = 1;
				}
				else if (retv == COVER_ART_NO_IMAGE)
				{
					debug_printf(DEBUG_INFO,"%s has no image\n", plugins[i]->name);
				}
			}
		}	
	}
	if(*path)
	{
		if(song->artist && song->album && cache){
			cfg_set_single_value_as_string(cover_index, song->artist, song->album,*path);
		}
		debug_printf(DEBUG_INFO,"returned image: %s", *path);
		return 	COVER_ART_OK_LOCAL;

	}
	if(can_try)
	{
		return COVER_ART_NOT_FETCHED;
	}
	else{
		if(song->artist && song->album){
			cfg_set_single_value_as_string(cover_index, song->artist, song->album,"");
		}
	}
	return COVER_ART_NO_IMAGE;
}

CoverArtResult cover_art_fetch_image_path(mpd_Song *song, gchar **path)
{
	return cover_art_fetch_image_path_real(song, path,TRUE);
}

CoverArtResult cover_art_fetch_image_path_no_cache(mpd_Song *song, gchar **path)
{
	return cover_art_fetch_image_path_real(song, path,FALSE);
}

void __internall_fetch_cover_art(ca_dl *cd)
{
	debug_printf(DEBUG_INFO,"Starting cover art fetch with plugin: %s\n", cd->plug->name);
	cd->retval = cd->plug->coverart->fetch_image(cd->song,NULL);
}
void cover_art_execute_signal(ca_callback *function, mpd_Song *song)
{
	debug_printf(DEBUG_INFO,"Executing callback: %p %s-%s\n", function, song->artist, song->album);
	function->function(song,function->userdata);
	g_free(function);
}

int cover_art_check_fetch_done(ca_dl *cd)
{
	if(qthread_is_done(cd->qt))
	{
		fetch_que_list = g_list_remove(fetch_que_list,cd);
		/* when found execute signal */
		if(!cd->retval){
			/* update cache */
			if(cd->song->artist && cd->song->album){
				cfg_set_single_value_as_string(cover_index,
						cd->song->artist,
						cd->song->album,"");
			}
		}
		/* execute signals */
		g_queue_foreach(cd->function,(GFunc)cover_art_execute_signal, cd->song); 
		g_queue_free(cd->function);
		/* free song */
		mpd_freeSong(cd->song);
		/* cleanup thread */
		qthread_free(cd->qt);
		/* free cd */
		g_free(cd);
		return FALSE;
	}
	return TRUE;	
}
void cover_art_thread_fetch_image(gmpcPlugin *plug, mpd_Song *song, CoverArtCallback function, gpointer userdata)
{
	ca_dl *list = NULL;
	ca_callback *cc = g_malloc0(sizeof(ca_callback));
	cc->function = function;
	cc->userdata = userdata;
	if(fetch_que_list)
	{
		GList *first = g_list_first(fetch_que_list);
		do {
			ca_dl *list = first->data;
			if(!strcmp(list->song->artist, song->artist) &&
					!strcmp(list->song->album, song->album))
			{
				debug_printf(DEBUG_INFO,"Fetch allready in progress\n");
				g_queue_push_tail(list->function, cc);
				return;
			}
		}while((first = g_list_next(first))!= NULL);
	}
	list =  g_malloc(sizeof(ca_dl));
	list->plug = plug;
	list->song = mpd_songDup(song);
	list->qt = qthread_new((GSourceFunc)__internall_fetch_cover_art, list);
	list->function = g_queue_new();
	list->retval = FALSE;
	g_queue_push_tail(list->function, cc);
	fetch_que_list = g_list_append(fetch_que_list, list);
	qthread_run(list->qt);
	g_timeout_add(500, (GSourceFunc)cover_art_check_fetch_done,list);
}

void cover_art_fetch_image(mpd_Song *song, CoverArtCallback function,gpointer userdata){
	int i=0;
	int priority = 1000;
	gmpcPlugin *plugin = NULL;
	for(i =  0; plugins[i] != NULL; i++)
	{
		if(plugins[i]->plugin_type == GMPC_PLUGIN_COVER_ART)
		{
			if(plugins[i]->coverart->fetch_image_path != NULL)
			{
				char *temp_path= NULL;
				int retv = plugins[i]->coverart->fetch_image_path(song, &temp_path);
				if(retv == COVER_ART_OK_LOCAL || retv == COVER_ART_OK_REMOTE)
				{
					debug_printf(DEBUG_WARNING,"Not fetching image, allready availible %s\n",temp_path);
					if(temp_path)g_free(temp_path);

					return;
				}
				else if(retv == COVER_ART_NOT_FETCHED)
				{
					if(plugins[i]->coverart->fetch_image)
					{
						if(priority > plugins[i]->coverart->get_priority())
						{
							plugin = plugins[i];
							priority = plugins[i]->coverart->get_priority();
						}
					}
				}
			}
		}

	}
	if(plugin != NULL)
	{
		debug_printf(DEBUG_INFO,"Trying to fetch image from: %s\n", plugin->name);
		cover_art_thread_fetch_image(plugin,song,function,userdata);
	}
}

CoverArtResult cover_art_fetch_image_path_aa(gchar *artist,gchar *album, gchar **path)
{
	if(artist && album){
		gchar *cipath = cfg_get_single_value_as_string(cover_index, artist, album);
		debug_printf(DEBUG_INFO,"query cover art cache");
		if(cipath)
		{
			if(strlen(cipath) == 0)
			{
				return COVER_ART_NO_IMAGE;
			}
			*path = g_strdup(cipath);
			return COVER_ART_OK_LOCAL;
		}
	}

	if(!mpd_server_check_version(connection,0,12,0))return COVER_ART_NO_IMAGE;
	MpdData *data = mpd_database_find_adv(connection, FALSE, MPD_TAG_ITEM_ARTIST, artist,
			MPD_TAG_ITEM_ALBUM,album,-1);
	if(data){
		CoverArtResult ret =COVER_ART_NO_IMAGE;
		if(data->type == MPD_DATA_TYPE_SONG)
		{
			ret = cover_art_fetch_image_path(data->song,path);
		}
		mpd_data_free(data);
		return ret;
	}

	return COVER_ART_NO_IMAGE;
}


void cover_art_fetch_image_aa(gchar *artist, gchar *album, CoverArtCallback function,gpointer userdata)
{
	if(!mpd_server_check_version(connection,0,12,0))return;
	MpdData *data = mpd_database_find_adv(connection, FALSE, MPD_TAG_ITEM_ARTIST, artist,
			MPD_TAG_ITEM_ALBUM,album,-1);
	if(data){
		if(data->type == MPD_DATA_TYPE_SONG)
		{
			cover_art_fetch_image(data->song,function, userdata);
		}
		mpd_data_free(data);
	}

	return ;
}


void cover_art_init()
{
	gchar *url = g_strdup_printf("%s/.covers/", g_get_home_dir());
	if(!g_file_test(url,G_FILE_TEST_IS_DIR)){
		if(mkdir(url, 0700)<0){
			g_error("Cannot make %s\n", url);
		}
	}
	g_free(url);
	url = g_strdup_printf("%s/.covers/covers.db", g_get_home_dir());
	cover_index = cfg_open(url);
	g_free(url);
}

