/*
 *Copyright (C) 2004-2005 Qball Cow <Qball@qballcow.nl>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <stdlib.h>
#include <string.h>
#include <glade/glade.h>
#include <time.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

#include "main.h"
#include "strfsong.h"
#include "misc.h"
#include "playlist3.h"
#include "playlist3-file-browser.h"
#include "tag-browser.h"
#include "open-location.h"
#include "vfs_download.h"
#include "osb_browser.h"
#include "config1.h"
#include <regex.h>
#include "pl3_custom_stream.h"


extern config_obj *config;
extern GladeXML *pl3_xml;

void pl3_browser_file_add_folder()
{
	GtkTreeSelection *selec = gtk_tree_view_get_selection((GtkTreeView *)glade_xml_get_widget (pl3_xml, "cat_tree"));
	GtkTreeModel *model = GTK_TREE_MODEL(pl3_tree);
	GtkTreeIter iter;

	if(check_connection_state())
	{
		return;
	}
	if(gtk_tree_selection_get_selected(selec,&model, &iter))
	{
		char *path;
		char *message = NULL;
		gtk_tree_model_get(model, &iter, PL3_CAT_INT_ID, &path, -1);
		message = g_strdup_printf("Added folder '%s' recursively", path);
		pl3_push_statusbar_message(message);
		g_free(message);
		mpd_ob_playlist_queue_add(connection, path);
		mpd_ob_playlist_queue_commit(connection);
	}
}

void pl3_browser_file_update_folder()
{
	GtkTreeSelection *selec = gtk_tree_view_get_selection((GtkTreeView *)glade_xml_get_widget (pl3_xml, "cat_tree"));
	GtkTreeModel *model = GTK_TREE_MODEL(pl3_tree);
	GtkTreeIter iter;

	if(check_connection_state())
	{
		return;
	}
	if(gtk_tree_selection_get_selected(selec,&model, &iter))
	{
		char *path;
		gtk_tree_model_get(model, &iter, PL3_CAT_INT_ID, &path, -1);
		mpd_ob_playlist_update_dir(connection, path);
	}
}

void pl3_browser_file_replace_folder()
{
	pl3_clear_playlist();
	pl3_browser_file_add_folder();	
	mpd_ob_player_play(connection);
}


/* add's the toplevel entry for the file browser, it also add's a fantom child */
void pl3_browser_file_add()
{
	GtkTreeIter iter,child;
	gtk_tree_store_append(pl3_tree, &iter, NULL);
	gtk_tree_store_set(pl3_tree, &iter, 
			PL3_CAT_TYPE, PL3_BROWSE_FILE,
			PL3_CAT_TITLE, "Browse Filesystem",
			PL3_CAT_INT_ID, "/",
			PL3_CAT_ICON_ID, "gtk-open",
			PL3_CAT_PROC, FALSE,
			PL3_CAT_ICON_SIZE,GTK_ICON_SIZE_DND,-1);
	/* add fantom child for lazy tree */
	gtk_tree_store_append(pl3_tree, &child, &iter);
}


long unsigned pl3_browser_file_view_folder(GtkTreeIter *iter_cat)
{
	MpdData* data =NULL;
	char *path;
	int sub_folder = 0;
	GtkTreeIter iter;
	long  unsigned time=0;
	gtk_tree_model_get(GTK_TREE_MODEL(pl3_tree), iter_cat, 2 , &path, -1);

	/* check the connection state and when its valid proceed */
	if (check_connection_state ())
	{
		return 0;
	}

	data = mpd_ob_playlist_get_directory(connection, path);
	while (data != NULL)
	{
		if (data->type == MPD_DATA_TYPE_DIRECTORY)
		{
			gchar *basename = g_path_get_basename(data->value.directory);
			gtk_list_store_append (pl3_store, &iter);
			gtk_list_store_set (pl3_store, &iter,
					0, data->value.directory,
					1, PL3_ENTRY_DIRECTORY,
					2, basename,               
					5, "gtk-open",
					-1);
			g_free(basename);
			sub_folder++;
		}
		else if (data->type == MPD_DATA_TYPE_SONG)
		{
			gchar buffer[1024];
			char *markdata = cfg_get_single_value_as_string_with_default(config, "playlist", "browser_markup",DEFAULT_MARKUP_BROWSER);
			strfsong (buffer, 1024, markdata,data->value.song);
			cfg_free_string(markdata);
			if(data->value.song->time != MPD_SONG_NO_TIME)
			{
				time += data->value.song->time;			
			}

			gtk_list_store_append (pl3_store, &iter);
			gtk_list_store_set (pl3_store, &iter,
					0, data->value.song->file,
					1, PL3_ENTRY_SONG,
					2, buffer,               
					5, "media-audiofile",
					-1);

		}

		else if (data->type == MPD_DATA_TYPE_PLAYLIST)
		{
			gchar *basename = g_path_get_basename (data->value.playlist);
			gtk_list_store_append (pl3_store, &iter);
			gtk_list_store_set (pl3_store, &iter,
					0, data->value.playlist,
					1, PL3_ENTRY_PLAYLIST,
					2, basename,
					5, "media-playlist", 
					-1);
			g_free (basename);
		}
		data = mpd_ob_data_get_next(data);
	}
	/* remove the fantom child if there are no subfolders anyway. */
	if(!sub_folder)
	{
		if(gtk_tree_model_iter_children(GTK_TREE_MODEL(pl3_tree), &iter, iter_cat))
		{
			gtk_tree_store_remove(pl3_tree, &iter);      		
		}
	}
	return time;
}


void pl3_browser_file_fill_tree(GtkTreeIter *iter)
{
	char *path;
	MpdData *data = NULL;
	GtkTreeIter child,child2;
	gtk_tree_model_get(GTK_TREE_MODEL(pl3_tree),iter, 2, &path, -1);
	gtk_tree_store_set(pl3_tree, iter, 4, TRUE, -1);

	data = mpd_ob_playlist_get_directory(connection, path);
	while (data != NULL)
	{
		if (data->type == MPD_DATA_TYPE_DIRECTORY)
		{
			gchar *basename =
				g_path_get_basename (data->value.directory);
			gtk_tree_store_append (pl3_tree, &child, iter);
			gtk_tree_store_set (pl3_tree, &child,
					0, PL3_BROWSE_FILE,
					1, basename,
					2, data->value.directory,
					3, "gtk-open",
					4, FALSE,
					PL3_CAT_ICON_SIZE,1,
					-1);
			gtk_tree_store_append(pl3_tree, &child2, &child);

			g_free (basename);
		}
		data = mpd_ob_data_get_next(data);
	}
	if(gtk_tree_model_iter_children(GTK_TREE_MODEL(pl3_tree), &child, iter))
	{
		gtk_tree_store_remove(pl3_tree, &child); 
	}
}



void pl3_browser_file_cat_popup(GtkTreeView *tree, GdkEventButton *event)
{
		/* here we have:  Add. Replace, (update?)*/
		GtkWidget *item;
		GtkWidget *menu = gtk_menu_new();	
		/* add the add widget */
		item = gtk_image_menu_item_new_from_stock(GTK_STOCK_ADD,NULL);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
		g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(pl3_browser_file_add_folder), NULL);		

		/* add the replace widget */
		item = gtk_image_menu_item_new_with_label("Replace");
		gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
				gtk_image_new_from_stock(GTK_STOCK_REDO, GTK_ICON_SIZE_MENU));
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
		g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(pl3_browser_file_replace_folder), NULL);				

		/* add the update widget */
		item = gtk_image_menu_item_new_with_label("Update");
		gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
				gtk_image_new_from_stock(GTK_STOCK_REFRESH, GTK_ICON_SIZE_MENU));
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
		g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(pl3_browser_file_update_folder), NULL);				

		/* show everything and popup */
		gtk_widget_show_all(menu);                                                        		
		gtk_menu_popup(GTK_MENU(menu), NULL, NULL,NULL, NULL, event->button, event->time);
}

void pl3_browser_file_cat_key_press(GdkEventKey *event)
{
	if(event->state == GDK_CONTROL_MASK && event->keyval == GDK_Insert)
	{
		pl3_browser_file_replace_folder();		
	}
	else if(event->keyval == GDK_Insert)
	{
		pl3_browser_file_add_folder();		
	}

}

int pl3_browser_file_playlist_key_press(GdkEventKey *event)
{
	if(event->state == GDK_CONTROL_MASK && event->keyval == GDK_Insert)
	{
		pl3_browser_file_replace_folder();		
	}
	else if(event->keyval == GDK_Insert)
	{
		pl3_browser_file_add_folder();		
	}
	else if(event->keyval == GDK_i)
	{
		pl3_show_song_info ();
	}
	else
	{
		return FALSE;
	}
	return TRUE;
}



void pl3_browser_file_cat_sel_changed(GtkTreeView *tree,GtkTreeIter *iter)
{
	long unsigned time= 0;
	gchar *string;
	gtk_list_store_clear(pl3_store);	
	time = pl3_browser_file_view_folder(iter);
	gtk_tree_view_set_model(GTK_TREE_VIEW(tree), GTK_TREE_MODEL(pl3_store));
	string = format_time(time);
	gtk_statusbar_push(GTK_STATUSBAR(glade_xml_get_widget(pl3_xml, "statusbar2")),0, string);
	g_free(string);
}
void pl3_browser_file_show_info(GtkTreeIter *iter)
{
	if(mpd_ob_server_check_version(connection,0,12,0))
	{
		char *path;
		MpdData *data;
		gtk_tree_model_get (GTK_TREE_MODEL(pl3_store), iter, PL3_SONG_ID, &path, -1);
		data = mpd_ob_playlist_find_adv(connection,TRUE,MPD_TAG_ITEM_FILENAME,path,-1);
		while(data != NULL)
		{
			call_id3_window_song(mpd_songDup(data->value.song));
			data = mpd_ob_data_get_next(data);
		}                                                                              	
	}
}

