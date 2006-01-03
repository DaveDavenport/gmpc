#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include "main.h"
#include "plugin.h"

typedef struct _ca_dl {
	struct _ca_dl *next;
	struct _ca_dl *previous;
	gmpcPlugin *plug;
	qthread *qt;
	GQueue *function;
	mpd_Song *song;
}ca_dl;

typedef struct _ca_callback{
	CoverArtCallback function;
	gpointer userdata;
} ca_callback;
GList *fetch_que_list = NULL;


CoverArtResult cover_art_fetch_image_path(mpd_Song *song, gchar **path)
{
	int i=0;
	int can_try = 0;
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
					*path = temp_path;
					return retv;
				}
				else if(retv == COVER_ART_NOT_FETCHED)
				{
					can_try = 1;
				}
			}
		}	
	}
	if(can_try)
	{
		return COVER_ART_NOT_FETCHED;
	}
	return COVER_ART_NO_IMAGE;
}


void __internall_fetch_cover_art(ca_dl *cd)
{
	debug_printf(DEBUG_INFO,"Starting cover art fetch with plugin: %s\n", cd->plug->name);
	cd->plug->coverart->fetch_image(cd->song,NULL);
}
void cover_art_execute_signal(ca_callback *function, mpd_Song *song)
{
	debug_printf(DEBUG_INFO,"Executing callback: %p %s-%s\n", function, song->artist, song->album);
	function->function(song,function->userdata);
	g_free(function);
}
/*
void cover_art_free_signal(ca_callback *function)
{
	printf("Freeing: %p\n", function);
	g_free(function);
}
*/
int cover_art_check_fetch_done(ca_dl *cd)
{
	if(qthread_is_done(cd->qt))
	{
		fetch_que_list = g_list_remove(fetch_que_list,cd);
		/* execute signals */
		g_queue_foreach(cd->function,(GFunc)cover_art_execute_signal, cd->song); 
/*		g_queue_foreach(cd->function,(GFunc)cover_art_free_signal, NULL); 
*/		g_queue_free(cd->function);
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
	g_queue_push_tail(list->function, cc);
	fetch_que_list = g_list_append(fetch_que_list, list);
	qthread_run(list->qt);
	g_timeout_add(500, (GSourceFunc)cover_art_check_fetch_done,list);
}

void cover_art_fetch_image(mpd_Song *song, CoverArtCallback function,gpointer userdata){
	int i=0;
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
						cover_art_thread_fetch_image(plugins[i],song,function,userdata);
					}
					return;
				}
			}
		}	                                                                           	
	}
}
