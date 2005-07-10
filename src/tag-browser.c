#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <glade/glade.h>
#include "libmpdclient.h"
#include "strfsong.h"
#include "main.h"
#include "playlist3.h"
#include "tag-browser.h"
#include "debug_printf.h"
#include "config1.h"

extern config_obj *config;
extern GladeXML *pl3_xml;
/************************************************************************************************/
/**************************** CUSTROM TAG BROWSER ***********************************************/
/************************************************************************************************/

void pl3_custom_tag_browser_reload()
{
	GtkTreeIter iter;
	if(pl3_tree == NULL || pl3_xml == NULL )
	{
		return;
	}

	if(gtk_tree_model_get_iter_first(GTK_TREE_MODEL(pl3_tree),&iter))
	{
		gchar *title = NULL;
		do{
			gtk_tree_model_get(GTK_TREE_MODEL(pl3_tree), &iter, PL3_CAT_TITLE, &title, -1);
			if(!strcmp(title, _("Tag Browser")))
			{
				GtkTreeIter child;
				if(gtk_tree_model_iter_children(GTK_TREE_MODEL(pl3_tree), &child, &iter))
				{
					do{
						gtk_tree_store_remove(pl3_tree, &child);

					}while(gtk_tree_store_iter_is_valid(pl3_tree, &child));
				}
				pl3_custom_tag_browser_list_add(&iter);
				return;
			}
		}while(gtk_tree_model_iter_next(GTK_TREE_MODEL(pl3_tree), &iter));
	}
}

void pl3_custom_tag_browser_list_add(GtkTreeIter *iter)
{
	conf_mult_obj *list;
	/* get the configured tag browsers */
	list = cfg_get_multiple_as_string(config, "playlist", "advbrows");
	if(list != NULL)
	{
		conf_mult_obj *data = list;
		do{
			if(strlen(data->key) && strlen(data->value))
			{
				pl3_custom_tag_browser_add_single(iter,data->key, data->value);
			}
			data = data->next;
		}while(data != NULL);
		cfg_free_multiple(list);
	}

}
void pl3_custom_tag_browser_add()
{
	if(mpd_ob_server_check_version(connection,0,12,0))
	{
		GtkTreeIter iter;


		/* add the root node to tag-browser */
		gtk_tree_store_append(pl3_tree, &iter, NULL);
		gtk_tree_store_set(pl3_tree, &iter, 
				PL3_CAT_TYPE, PL3_BROWSE_CUSTOM_TAG,
				PL3_CAT_TITLE, _("Tag Browser"),        	
				PL3_CAT_INT_ID, "",
				PL3_CAT_ICON_ID, "media-artist",
				PL3_CAT_PROC, TRUE,
				PL3_CAT_ICON_SIZE,GTK_ICON_SIZE_DND,
				PL3_CAT_BROWSE_FORMAT, "",
				-1);
		pl3_custom_tag_browser_list_add(&iter);
	}
	else
	{
		debug_printf(DEBUG_INFO, "pl3_custom_tag_browser_add(): Option not supported for mpd version < 0.12");
	}
}


void pl3_custom_tag_browser_add_single(GtkTreeIter *piter, char *title, char *format)
{
	GtkTreeIter iter,child;
	gtk_tree_store_append(pl3_tree, &iter, piter);
	gtk_tree_store_set(pl3_tree, &iter, 
			PL3_CAT_TYPE, PL3_BROWSE_CUSTOM_TAG,
			PL3_CAT_TITLE, title,        	
			PL3_CAT_INT_ID, "",
			PL3_CAT_ICON_ID, "media-artist",
			PL3_CAT_PROC, FALSE,
			PL3_CAT_ICON_SIZE,GTK_ICON_SIZE_DND,
			PL3_CAT_BROWSE_FORMAT, format,
			-1);
	/* add fantom child for lazy tree */
	gtk_tree_store_append(pl3_tree, &child, &iter);
}


void pl3_custom_tag_browser_fill_tree(GtkTreeIter *iter)
{
	char *first_tag, *second_tag;
	char *format;
	char **tk_format= NULL;
	GtkTreePath *path = NULL;
	int depth = 0;
	int len=0;
	GtkTreeIter child,child2;
	gtk_tree_model_get(GTK_TREE_MODEL(pl3_tree),iter, 1, &first_tag,2,&second_tag,PL3_CAT_BROWSE_FORMAT, &format, -1);
	gtk_tree_store_set(pl3_tree, iter, 4, TRUE, -1);

	path = gtk_tree_model_get_path(GTK_TREE_MODEL(pl3_tree), iter);
	if(path == NULL)
	{
		printf("Failed to get path\n");
		return;
	}
	depth = gtk_tree_path_get_depth(path) -2;
	gtk_tree_path_free(path);	

	if (!mpd_ob_check_connected(connection) || path < 0)
	{
		return;
	}

	tk_format = g_strsplit(format, "|",0);
	if(tk_format ==NULL)
	{
		printf("failed to split\n");
		return;
	}
	for(len=0;tk_format[len] != NULL;len++)	
	{
		if(mpd_misc_get_tag_by_name(tk_format[len])== -1)
		{
			printf("invallid tag: %s\n", tk_format[len]);
			g_strfreev(tk_format);
			return;
		}
	}
	/* the things we do when on level 0 */
	if(depth == 0)
	{
		MpdData *data = mpd_ob_playlist_get_unique_tags(connection,mpd_misc_get_tag_by_name(tk_format[0]),-1);

		while(data != NULL)
		{	
			gtk_tree_store_append (pl3_tree, &child, iter);
			gtk_tree_store_set (pl3_tree, &child,
					0, PL3_BROWSE_CUSTOM_TAG,
					1, data->value.tag, /* the field */
					2, data->value.tag, /* the artist name, if(1 and 2 together its an artist field) */
					3, "media-album",
					4, FALSE,
					PL3_CAT_ICON_SIZE,1,
					PL3_CAT_BROWSE_FORMAT, format,
					-1);
			if(len > 1)
			{
				gtk_tree_store_append(pl3_tree, &child2, &child);
			}

			data = mpd_ob_data_get_next(data);
		}
		if(gtk_tree_model_iter_children(GTK_TREE_MODEL(pl3_tree), &child, iter))
		{
			gtk_tree_store_remove(pl3_tree, &child); 
		}
	}
	/* if where inside a artist */
	else if(depth == 1)
	{
		MpdData *data = mpd_ob_playlist_get_unique_tags(connection,mpd_misc_get_tag_by_name(tk_format[1]),
				mpd_misc_get_tag_by_name(tk_format[0]),first_tag,-1 );
		if(data == NULL)
		{
			printf("no sub data\n");
		}
		while(data != NULL){
			gtk_tree_store_append (pl3_tree, &child, iter);
			gtk_tree_store_set (pl3_tree, &child,
					0, PL3_BROWSE_CUSTOM_TAG,
					1, data->value.tag,
					2, first_tag,
					3, "media-artist", 
					4, FALSE, 
					PL3_CAT_ICON_SIZE,1,
					PL3_CAT_BROWSE_FORMAT, format,
					-1);
			data = mpd_ob_data_get_next(data);
			if(len > 2)
			{
				gtk_tree_store_append(pl3_tree, &child2, &child);			
			}                                                        		
		}
		if(gtk_tree_model_iter_children(GTK_TREE_MODEL(pl3_tree), &child, iter))
		{
			gtk_tree_store_remove(pl3_tree, &child); 
		}
	}
	else if(depth == 2 && len >= 2)
	{
		MpdData *data = mpd_ob_playlist_get_unique_tags(connection,
				mpd_misc_get_tag_by_name(tk_format[2]),
				mpd_misc_get_tag_by_name(tk_format[1]),first_tag,
				mpd_misc_get_tag_by_name(tk_format[0]),second_tag,
				-1);
		if(data == NULL)
		{
			debug_printf(DEBUG_WARNING, "pl3_custom_tag_browser_fill_tree: no sub data %s %s %s %s %s\n",
					tk_format[2],
					tk_format[1],first_tag,
					tk_format[0],second_tag);
		}

		while(data != NULL){
			gtk_tree_store_append (pl3_tree, &child, iter);
			gtk_tree_store_set (pl3_tree, &child,
					0, PL3_BROWSE_CUSTOM_TAG,
					1, data->value.tag,
					2, first_tag,
					3, "media-artist", 
					4, TRUE, 
					PL3_CAT_ICON_SIZE,1,
					PL3_CAT_BROWSE_FORMAT, format,
					-1);
			data = mpd_ob_data_get_next(data);
		}
		if(gtk_tree_model_iter_children(GTK_TREE_MODEL(pl3_tree), &child, iter))
		{
			gtk_tree_store_remove(pl3_tree, &child); 
		}                                                                                       		
	}
	g_strfreev(tk_format);
}

long unsigned pl3_custom_tag_browser_view_folder(GtkTreeIter *iter_cat)
{
	char *first_tag, *second_tag, *format;
	char **tk_format;
	int i = 0, depth;
	GtkTreePath *path;
	GtkTreeIter iter;
	long unsigned time =0;
	gtk_tree_model_get(GTK_TREE_MODEL(pl3_tree), iter_cat, 2 , &first_tag, 1,&second_tag,PL3_CAT_BROWSE_FORMAT, &format, -1);

	if (check_connection_state ())
	{
		return 0;
	}

	path = gtk_tree_model_get_path(GTK_TREE_MODEL(pl3_tree), iter_cat);
	if(path == NULL)
	{
		printf("Failed to get path\n");
		return 0;
	}
	depth = gtk_tree_path_get_depth(path) -2;


	if(depth <= 0)
	{
		/*lowest level, do nothing */
		gtk_tree_path_free(path);	
		return 0;
	}                                    	

	tk_format = g_strsplit(format, "|",0);
	if(tk_format ==NULL)
	{
		printf("failed to split\n");
		gtk_tree_path_free(path);	
		return 0;
	}


	for(i=0;tk_format[i] != NULL;i++)	
	{

		if(mpd_misc_get_tag_by_name(tk_format[i])== -1)
		{

			g_strfreev(tk_format);
			gtk_tree_path_free(path);	
			return 0;
		}

	}

	if(depth == 1)
	{
		MpdData *data = mpd_ob_playlist_find_adv(connection,TRUE, 
				mpd_misc_get_tag_by_name(tk_format[0]), first_tag, -1);
		/* lowest level selected*/
		while(data != NULL)
		{
			gchar buffer[1024];
			char *markdata = cfg_get_single_value_as_string_with_default(config, "playlist", "browser_markup",DEFAULT_MARKUP_BROWSER);
			strfsong (buffer, 1024,markdata,data->value.song);
			cfg_free_string(markdata);
			if(data->value.song->time != MPD_SONG_NO_TIME)
			{
				time += data->value.song->time;
			}
			if(data->value.song->file == NULL)
			{
				debug_printf(DEBUG_WARNING,"pl3_browser_view_folder: crap mpdSong has no file attribute.\n");
			}
			gtk_list_store_append (pl3_store, &iter);
			gtk_list_store_set (pl3_store, &iter,
					2, buffer,
					0, data->value.song->file,
					1, PL3_ENTRY_SONG,
					5,"media-audiofile",
					-1);
			data = mpd_ob_data_get_next(data);
		}
	}
	else if(depth == 2)
	{
		/* second level */
		MpdData *data = mpd_ob_playlist_find_adv(connection,TRUE,
				mpd_misc_get_tag_by_name(tk_format[0]),first_tag,
				mpd_misc_get_tag_by_name(tk_format[1]), second_tag, -1);
		while (data != NULL)
		{
			gchar buffer[1024];
			char *markdata = cfg_get_single_value_as_string_with_default(config, "playlist", "browser_markup",DEFAULT_MARKUP_BROWSER);
			strfsong (buffer, 1024,markdata,data->value.song);
			cfg_free_string(markdata);
			if(data->value.song->time != MPD_SONG_NO_TIME)
			{
				time += data->value.song->time;
			}
			if(data->value.song->file == NULL)
			{
				debug_printf(DEBUG_WARNING,"pl3_browser_view_folder: crap mpdSong has no file attribute.\n");
			}
			gtk_list_store_append (pl3_store, &iter);
			gtk_list_store_set (pl3_store, &iter,
					2, buffer,
					0, data->value.song->file,
					1, PL3_ENTRY_SONG,
					5,"media-audiofile",
					-1);

			data = mpd_ob_data_get_next(data);
		}

	}
	else if (depth == 3)
	{
		char *first;
		/* take the upper one */
		/* go 2 up */
		if(gtk_tree_path_up(path) && gtk_tree_path_up(path))
		{
			MpdData *data  = NULL;
			gtk_tree_model_get_iter(GTK_TREE_MODEL(pl3_tree), &iter, path);
			gtk_tree_model_get(GTK_TREE_MODEL(pl3_tree), &iter, 1, &first, -1);

			/* artist and album is selected */
			data = mpd_ob_playlist_find_adv(connection,TRUE,
					mpd_misc_get_tag_by_name(tk_format[0]),	first,
					mpd_misc_get_tag_by_name(tk_format[1]), first_tag,
					mpd_misc_get_tag_by_name(tk_format[2]), second_tag,-1);
			while (data != NULL)
			{
				gchar buffer[1024];
				char *markdata = cfg_get_single_value_as_string_with_default(config, "playlist", "browser_markup",
						DEFAULT_MARKUP_BROWSER);
				strfsong (buffer, 1024,markdata,data->value.song);
				cfg_free_string(markdata);
				if(data->value.song->time != MPD_SONG_NO_TIME)
				{
					time += data->value.song->time;
				}
				if(data->value.song->file == NULL)
				{
					debug_printf(DEBUG_WARNING,"pl3_browser_view_folder: crap mpdSong has no file attribute.\n");
				}
				gtk_list_store_append (pl3_store, &iter);
				gtk_list_store_set (pl3_store, &iter,
						2, buffer,
						0, data->value.song->file,
						1, PL3_ENTRY_SONG,
						5,"media-audiofile",
						-1);

				data = mpd_ob_data_get_next(data);
			}
		}
	}
	gtk_tree_path_free(path);	
	g_strfreev(tk_format);
	return time;
}
