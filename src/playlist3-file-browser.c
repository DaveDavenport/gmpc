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


#include "plugin.h"


#include "main.h"
#include "misc.h"
#include "playlist3.h"
#include "playlist3-file-browser.h"
#include "config1.h"

void pl3_file_browser_button_release_event(GtkWidget *but, GdkEventButton *event);
void pl3_file_browser_row_activated(GtkTreeView *tree, GtkTreePath *tp);
void pl3_file_browser_add_selected();
void pl3_file_browser_replace_selected();
int pl3_file_browser_playlist_key_press(GtkWidget *tree,GdkEventKey *event);
void pl3_file_browser_show_info();
long unsigned pl3_file_browser_view_folder(GtkTreeIter *iter_cat);
void pl3_file_browser_update_folder();
void pl3_file_browser_add_folder();
void pl3_file_browser_delete_playlist(GtkToggleButton *bt, char *string);
enum{
	PL3_FB_PATH,
	PL3_FB_TYPE,
	PL3_FB_TITLE,
	PL3_FB_ICON,
	PL3_FB_ROWS
};

extern config_obj *config;
extern GladeXML *pl3_xml;

/* internal */
GtkWidget *pl3_fb_tree = NULL;
GtkListStore *pl3_fb_store = NULL;
GtkWidget *pl3_fb_sw = NULL;

int pl3_file_browser_button_press_event(GtkTreeView *tree, GdkEventButton *event)
{
	GtkTreeSelection *sel = gtk_tree_view_get_selection(tree);
	if(event->button != 3 || gtk_tree_selection_count_selected_rows(sel) < 2|| !mpd_check_connected(connection))	
	{
		return FALSE;                                                                                           	
	}
	return TRUE;
}

void pl3_file_browser_init()
{
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column = NULL;
	GValue value;
	pl3_fb_store = gtk_list_store_new (PL3_FB_ROWS, 
			GTK_TYPE_STRING, /* path to file */
			GTK_TYPE_INT,	/* type, FILE/PLAYLIST/FOLDER  */
			GTK_TYPE_STRING,	/* title to display */
			GTK_TYPE_STRING); /* icon type */



	renderer = gtk_cell_renderer_pixbuf_new ();

	column = gtk_tree_view_column_new ();
	gtk_tree_view_column_pack_start (column, renderer, FALSE);
	gtk_tree_view_column_set_attributes (column,renderer,"stock-id", PL3_FB_ICON,NULL);
	memset(&value, 0, sizeof(value));
	/* set value for ALL */
	g_value_init(&value, G_TYPE_FLOAT);
	g_value_set_float(&value, 0.0);
	g_object_set_property(G_OBJECT(renderer), "yalign", &value); 

	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_set_attributes (column,renderer,"text", PL3_FB_TITLE, NULL);


	/* set up the tree */
	pl3_fb_tree= gtk_tree_view_new_with_model(GTK_TREE_MODEL(pl3_fb_store));
	/* insert the column in the tree */
	gtk_tree_view_append_column (GTK_TREE_VIEW (pl3_fb_tree), column);                                         	
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(pl3_fb_tree), FALSE);
	gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(pl3_fb_tree), TRUE);
	gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW(pl3_fb_tree)), GTK_SELECTION_MULTIPLE);

	/* setup signals */
	g_signal_connect(G_OBJECT(pl3_fb_tree), "row-activated",G_CALLBACK(pl3_file_browser_row_activated), NULL); 
	g_signal_connect(G_OBJECT(pl3_fb_tree), "button-press-event", G_CALLBACK(pl3_file_browser_button_press_event), NULL);
	g_signal_connect(G_OBJECT(pl3_fb_tree), "button-release-event", G_CALLBACK(pl3_file_browser_button_release_event), NULL);
	g_signal_connect(G_OBJECT(pl3_fb_tree), "key-press-event", G_CALLBACK(pl3_file_browser_playlist_key_press), NULL);

	/* set up the scrolled window */
	pl3_fb_sw = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(pl3_fb_sw), GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(pl3_fb_sw), GTK_SHADOW_ETCHED_IN);
	gtk_container_add(GTK_CONTAINER(pl3_fb_sw), pl3_fb_tree);

	/* set initial state */
	printf("initialized current playlist treeview\n");
	g_object_ref(G_OBJECT(pl3_fb_sw));
}

void pl3_file_browser_add_folder()
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
		mpd_playlist_queue_add(connection, path);
		mpd_playlist_queue_commit(connection);
		g_free(path);
	}
}

void pl3_file_browser_update_folder()
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
		mpd_playlist_update_dir(connection, path);
		g_free(path);
	}
}

void pl3_file_browser_replace_folder()
{
	mpd_playlist_clear(connection);
	pl3_file_browser_add_folder();	
	mpd_player_play(connection);
}


/* add's the toplevel entry for the file browser, it also add's a fantom child */
void pl3_file_browser_add()
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


long unsigned pl3_file_browser_view_folder(GtkTreeIter *iter_cat)
{
	MpdData* data =NULL;
	char *path;
	int sub_folder = 0;
	GtkTreeIter iter;
	long  unsigned time=0;
	

	/* check the connection state and when its valid proceed */
	if (check_connection_state ())
	{
		return 0;
	}
	gtk_tree_model_get(GTK_TREE_MODEL(pl3_tree), iter_cat, 2 , &path, -1);

	data = mpd_playlist_get_directory(connection, path);
	while (data != NULL)
	{
		if (data->type == MPD_DATA_TYPE_DIRECTORY)
		{
			gchar *basename = g_path_get_basename(data->directory);
			gtk_list_store_append (pl3_fb_store, &iter);
			gtk_list_store_set (pl3_fb_store, &iter,
					PL3_FB_PATH, data->directory,
					PL3_FB_TYPE, PL3_ENTRY_DIRECTORY,
					PL3_FB_TITLE, basename,               
					PL3_FB_ICON, "gtk-open",
					-1);
			g_free(basename);
			sub_folder++;
		}
		else if (data->type == MPD_DATA_TYPE_SONG)
		{
			gchar buffer[1024];
			char *markdata = cfg_get_single_value_as_string_with_default(config, "playlist", "browser_markup",DEFAULT_MARKUP_BROWSER);
			mpd_song_markup(buffer, 1024, markdata,data->song);
			cfg_free_string(markdata);
			if(data->song->time != MPD_SONG_NO_TIME)
			{
				time += data->song->time;			
			}

			gtk_list_store_append (pl3_fb_store, &iter);
			gtk_list_store_set (pl3_fb_store, &iter,
					PL3_FB_PATH, data->song->file,
					PL3_FB_TYPE, PL3_ENTRY_SONG,
					PL3_FB_TITLE, buffer,               
					PL3_FB_ICON, "media-audiofile",
					-1);

		}

		else if (data->type == MPD_DATA_TYPE_PLAYLIST)
		{
			gchar *basename = g_path_get_basename (data->playlist);
			gtk_list_store_append (pl3_fb_store, &iter);
			gtk_list_store_set (pl3_fb_store, &iter,
					PL3_FB_PATH, data->playlist,
					PL3_FB_TYPE, PL3_ENTRY_PLAYLIST,
					PL3_FB_TITLE, basename,
					PL3_FB_ICON, "media-playlist", 
					-1);
			g_free (basename);
		}
		data = mpd_data_get_next(data);
	}
	/* remove the fantom child if there are no subfolders anyway. */
	if(!sub_folder)
	{
		if(gtk_tree_model_iter_children(GTK_TREE_MODEL(pl3_tree), &iter, iter_cat))
		{
			gtk_tree_store_remove(pl3_tree, &iter);      		
		}
	}
	g_free(path);
	return time;
}


void pl3_file_browser_fill_tree(GtkTreeIter *iter)
{
	char *path;
	MpdData *data = NULL;
	GtkTreeIter child,child2;
	gtk_tree_model_get(GTK_TREE_MODEL(pl3_tree),iter, 2, &path, -1);
	gtk_tree_store_set(pl3_tree, iter, 4, TRUE, -1);

	data = mpd_playlist_get_directory(connection, path);
	while (data != NULL)
	{
		if (data->type == MPD_DATA_TYPE_DIRECTORY)
		{
			gchar *basename =
				g_path_get_basename (data->directory);
			gtk_tree_store_append (pl3_tree, &child, iter);
			gtk_tree_store_set (pl3_tree, &child,
					0, PL3_BROWSE_FILE,
					1, basename,
					2, data->directory,
					3, "gtk-open",
					4, FALSE,
					PL3_CAT_ICON_SIZE,1,
					-1);
			gtk_tree_store_append(pl3_tree, &child2, &child);

			g_free (basename);
		}
		data = mpd_data_get_next(data);
	}
	if(gtk_tree_model_iter_children(GTK_TREE_MODEL(pl3_tree), &child, iter))
	{
		gtk_tree_store_remove(pl3_tree, &child); 
	}
	g_free(path);
}



int pl3_file_browser_cat_popup(GtkWidget *menu, int type,GtkTreeView *tree, GdkEventButton *event)
{
	if(type == PL3_BROWSE_FILE)
	{
		/* here we have:  Add. Replace, (update?)*/
		GtkWidget *item;
		/* add the add widget */
		item = gtk_image_menu_item_new_from_stock(GTK_STOCK_ADD,NULL);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
		g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(pl3_file_browser_add_folder), NULL);		

		/* add the replace widget */
		item = gtk_image_menu_item_new_with_label("Replace");
		gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
				gtk_image_new_from_stock(GTK_STOCK_REDO, GTK_ICON_SIZE_MENU));
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
		g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(pl3_file_browser_replace_folder), NULL);				

		/* add the update widget */
		item = gtk_image_menu_item_new_with_label("Update");
		gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
				gtk_image_new_from_stock(GTK_STOCK_REFRESH, GTK_ICON_SIZE_MENU));
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
		g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(pl3_file_browser_update_folder), NULL);				

		/* show everything and popup */
		return 1;
	}
	return 0;
}

void pl3_file_browser_cat_key_press(GdkEventKey *event)
{
	if(event->state == GDK_CONTROL_MASK && event->keyval == GDK_Insert)
	{
		pl3_file_browser_replace_folder();		
	}
	else if(event->keyval == GDK_Insert)
	{
		pl3_file_browser_add_folder();		
	}

}

int pl3_file_browser_playlist_key_press(GtkWidget *tree, GdkEventKey *event)
{
	if(event->state == GDK_CONTROL_MASK && event->keyval == GDK_Insert)
	{
		pl3_file_browser_replace_selected();		
	}
	else if(event->keyval == GDK_Insert)
	{
		pl3_file_browser_add_selected();		
	}
	else if(event->keyval == GDK_i)
	{
		pl3_file_browser_show_info();
	}
	else
	{
		return pl3_window_key_press_event(tree,event);
	}
	return TRUE;
}



void pl3_file_browser_cat_sel_changed(GtkTreeView *tree,GtkTreeIter *iter)
{
	long unsigned time= 0;
	gchar *string;
	gtk_list_store_clear(pl3_fb_store);	
	time = pl3_file_browser_view_folder(iter);
	string = format_time(time);
	gtk_statusbar_push(GTK_STATUSBAR(glade_xml_get_widget(pl3_xml, "statusbar2")),0, string);
	g_free(string);
}

void pl3_file_browser_selected()
{
	if(pl3_fb_tree == NULL)
	{
		pl3_file_browser_init();
	}

	gtk_container_add(GTK_CONTAINER(glade_xml_get_widget(pl3_xml, "browser_container")), pl3_fb_sw);
	gtk_widget_show_all(pl3_fb_sw);
}
void pl3_file_browser_unselected()
{
	gtk_container_remove(GTK_CONTAINER(glade_xml_get_widget(pl3_xml, "browser_container")), pl3_fb_sw);
}


void pl3_file_browser_show_info()
{
	GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW(pl3_fb_tree));
	GtkTreeSelection *selection =gtk_tree_view_get_selection (GTK_TREE_VIEW(pl3_fb_tree));
	if(!mpd_server_check_version(connection,0,12,0))
	{
		return;
	}
	if (gtk_tree_selection_count_selected_rows (selection) > 0)
	{                                                                                     	
		GList *list = NULL;
		list = gtk_tree_selection_get_selected_rows (selection, &model);              	
		/* iterate over every row */
		list = g_list_last (list);
		do
		{
			GtkTreeIter iter;
			char *path;
			int type;
			MpdData *data;
			gtk_tree_model_get_iter (model, &iter, (GtkTreePath *) list->data);		      			
			gtk_tree_model_get (GTK_TREE_MODEL(pl3_fb_store), &iter,PL3_FB_TYPE,&type, PL3_FB_PATH, &path, -1);
			if(type == PL3_ENTRY_SONG)
			{
				data = mpd_playlist_find_adv(connection,TRUE,MPD_TAG_ITEM_FILENAME,path,-1);
				while(data != NULL)
				{
					call_id3_window_song(mpd_songDup(data->song));
					data = mpd_data_get_next(data);
				}
			}	
			g_free(path);
		}
		while ((list = g_list_previous (list)) && !check_connection_state ());
		/* free list */
		g_list_foreach (list, (GFunc) gtk_tree_path_free, NULL);              		      
		g_list_free (list);
	}
}


void pl3_file_browser_row_activated(GtkTreeView *tree, GtkTreePath *tp)
{
	GtkTreeIter iter;
	gchar *song_id;
	gint r_type;
	gtk_tree_model_get_iter(gtk_tree_view_get_model(tree), &iter, tp);
	gtk_tree_model_get(gtk_tree_view_get_model(tree), &iter, PL3_SONG_ID,&song_id, PL3_SONG_POS, &r_type, -1);
	if(song_id == NULL)
	{

		return;
	}
	if(r_type&PL3_ENTRY_PLAYLIST)
	{	
		pl3_push_statusbar_message("Loaded playlist");
		mpd_playlist_queue_load(connection, song_id);
	}
	else if (r_type&PL3_ENTRY_DIRECTORY)
	{
		GtkTreeSelection *selec = gtk_tree_view_get_selection((GtkTreeView *)glade_xml_get_widget (pl3_xml, "cat_tree"));
		GtkTreeModel *model = GTK_TREE_MODEL(pl3_tree);
		GtkTreeIter iter;

		if(gtk_tree_selection_get_selected(selec,&model, &iter))
		{
			GtkTreeIter citer;
			GtkTreePath *path = gtk_tree_model_get_path(model, &iter);
			gtk_tree_view_expand_row(GTK_TREE_VIEW(glade_xml_get_widget (pl3_xml, "cat_tree")), path, FALSE);
			gtk_tree_path_free(path);
			if(gtk_tree_model_iter_children(model, &citer, &iter))
			{
				do{
					char *name = NULL;
					gtk_tree_model_get(model, &citer, 2, &name, -1);
					if(strcmp(name, song_id) == 0)
					{
						gtk_tree_selection_select_iter(selec,&citer);						
						path = gtk_tree_model_get_path(model, &citer);
						gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(glade_xml_get_widget (pl3_xml, "cat_tree")), path,NULL,TRUE,0.5,0);
						gtk_tree_path_free(path);
						break;

					}
				}while(gtk_tree_model_iter_next(model, &citer));
			}	

		}
	}
	else
	{
		pl3_push_statusbar_message("Added a song");
		mpd_playlist_queue_add(connection, song_id);
	}
	mpd_playlist_queue_commit(connection);
	g_free(song_id);
}




void pl3_file_browser_button_release_event(GtkWidget *but, GdkEventButton *event)
{
	if(event->button != 3) return;
	GtkWidget *item;
	GtkWidget *menu = gtk_menu_new();
	GtkTreeSelection *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(pl3_fb_tree));
	/* don't show it when where listing custom streams... 
	 * show always when version 12..  or when searching in playlist.
	 */	
	if(gtk_tree_selection_count_selected_rows(sel) == 1)
	{	
		GtkTreeModel *model = GTK_TREE_MODEL(pl3_fb_store);	
		GList *list = gtk_tree_selection_get_selected_rows(sel, &model);
		if(list != NULL)
		{
			GtkTreeIter iter;
			int row_type;
			char *path;
			list = g_list_first(list);
			gtk_tree_model_get_iter(model, &iter, list->data);
			gtk_tree_model_get(model, &iter,PL3_FB_PATH,&path,PL3_FB_TYPE, &row_type, -1); 
			if(row_type&PL3_ENTRY_SONG)
			{
				if(mpd_server_check_version(connection,0,12,0))
				{
					item = gtk_image_menu_item_new_from_stock(GTK_STOCK_DIALOG_INFO,NULL);
					gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
					g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(pl3_file_browser_show_info), NULL);		
				}
			}
			if(row_type == PL3_ENTRY_PLAYLIST)
			{
				item = gtk_image_menu_item_new_from_stock(GTK_STOCK_DELETE,NULL);
				gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
				g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(pl3_file_browser_delete_playlist), path);
			}
			g_list_foreach (list,(GFunc) gtk_tree_path_free, NULL);
			g_list_free (list);
			g_free(path);
		}
	}
	/* add the delete widget */
	item = gtk_image_menu_item_new_from_stock(GTK_STOCK_ADD,NULL);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(pl3_file_browser_add_selected), NULL);

	/* add the replace widget */
	item = gtk_image_menu_item_new_with_label("Replace");
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
			gtk_image_new_from_stock(GTK_STOCK_REDO, GTK_ICON_SIZE_MENU));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(pl3_file_browser_replace_selected), NULL);

	gtk_widget_show_all(menu);
	gtk_menu_popup(GTK_MENU(menu), NULL, NULL,NULL, NULL, event->button, event->time);	
	return;
}
void pl3_file_browser_replace_selected()
{
	mpd_playlist_clear(connection);
	pl3_file_browser_add_selected();
	mpd_player_play(connection);	

}
void pl3_file_browser_add_selected()
{
	GtkTreeIter iter;
	GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW(pl3_fb_tree));
	GtkTreeModel *model = GTK_TREE_MODEL (pl3_fb_store);
	GList *rows = gtk_tree_selection_get_selected_rows (selection, &model);
	int songs=0;
	gchar *message;
	if(rows != NULL)
	{
		gchar *name;
		gint type;
		GList *node = g_list_first(rows);
		do
		{
			GtkTreePath *path = node->data;
			gtk_tree_model_get_iter (model, &iter, path);
			gtk_tree_model_get (model, &iter, PL3_FB_PATH,&name, PL3_FB_TYPE, &type, -1);	  
			/* does this bitmask thingy works ok? I think it hsould */
			if(type&(PL3_ENTRY_SONG|PL3_ENTRY_DIRECTORY))
			{
				/* add them to the add list */
				mpd_playlist_queue_add(connection, name);
			}
			else if (type == PL3_ENTRY_PLAYLIST)
			{
				mpd_playlist_queue_load(connection, name);
			}
			songs++;
			g_free(name);
		}while((node = g_list_next(node)) != NULL);
	}
	/* if there are items in the add list add them to the playlist */
	mpd_playlist_queue_commit(connection);
	if(songs != 0)
	{
		message = g_strdup_printf("Added %i song%s", songs, (songs != 1)? "s":"");
		pl3_push_statusbar_message(message);
		g_free(message);                                       	
	}

	g_list_foreach (rows, (GFunc) gtk_tree_path_free, NULL);
	g_list_free (rows);
}

void pl3_file_browser_delete_playlist(GtkToggleButton *bt, char *string)
{
	/* create a warning message dialog */
	GtkWidget *dialog = gtk_message_dialog_new (GTK_WINDOW
			(glade_xml_get_widget
			 (pl3_xml, "pl3_win")),
			GTK_DIALOG_MODAL,
			GTK_MESSAGE_WARNING,
			GTK_BUTTONS_NONE,
			_("Are you sure you want to clear the selected playlist?"));
	gtk_dialog_add_buttons (GTK_DIALOG (dialog), GTK_STOCK_NO,
			GTK_RESPONSE_CANCEL, GTK_STOCK_YES,
			GTK_RESPONSE_OK, NULL);
	gtk_dialog_set_default_response (GTK_DIALOG (dialog),
			GTK_RESPONSE_CANCEL);

	switch (gtk_dialog_run (GTK_DIALOG (dialog)))
	{
		case GTK_RESPONSE_OK:
			mpd_playlist_delete(connection, string);
			pl3_cat_sel_changed();

	}
	gtk_widget_destroy (GTK_WIDGET (dialog));
}
