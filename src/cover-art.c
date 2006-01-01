#include <stdio.h>
#include <stdlib.h>
#include <gtk/gtk.h>
#include "main.h"
#include "plugin.h"


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


void cover_art_fetch_image(mpd_Song *song, GSourceFunc function){
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
					if(temp_path)g_free(temp_path);
					return;
				}
				else if(retv == COVER_ART_NOT_FETCHED)
				{
					if(plugins[i]->coverart->fetch_image)
					{
						plugins[i]->coverart->fetch_image(song, function);
					}
					return;
				}
			}
		}	                                                                           	
	}
}
