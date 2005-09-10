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
#include "playlist3-current-playlist-browser.h"
#include "tag-browser.h"
#include "open-location.h"
#include "vfs_download.h"
#include "osb_browser.h"
#include "config1.h"
#include <regex.h>
#include "pl3_custom_stream.h"


extern config_obj *config;
extern GladeXML *pl3_xml;
extern GtkListStore *pl2_store;

void pl3_browser_current_playlist_scroll_to_current_song()
{
	/* scroll to the playing song */
	if(mpd_ob_player_get_current_song_pos(connection) >= 0 && mpd_ob_playlist_get_playlist_length(connection)  > 0)
	{
		gchar *str = g_strdup_printf("%i", mpd_ob_player_get_current_song_pos(connection));
		GtkTreePath *path = gtk_tree_path_new_from_string(str);
		if(path != NULL)
		{
			gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(
						glade_xml_get_widget(pl3_xml, "playlist_tree")), 
					path,
					NULL,
					TRUE,0.5,0);
		}
		gtk_tree_path_free(path);
		g_free(str);
	}      
}

/* add's the toplevel entry for the current playlist view */
void pl3_browser_current_playlist_add()
{
	GtkTreeIter iter;
	gtk_tree_store_append(pl3_tree, &iter, NULL);
	gtk_tree_store_set(pl3_tree, &iter, 
			PL3_CAT_TYPE, PL3_CURRENT_PLAYLIST,
			PL3_CAT_TITLE, "Current Playlist",
			PL3_CAT_INT_ID, "",
			PL3_CAT_ICON_ID, "media-playlist",
			PL3_CAT_PROC, TRUE,
			PL3_CAT_ICON_SIZE,GTK_ICON_SIZE_DND,
			-1);
}

/* delete all selected songs,
 * if no songs select ask the user if he want's to clear the list 
 */
void pl3_browser_current_playlist_delete_selected_songs ()
{
	/* grab the selection from the tree */
	GtkTreeSelection *selection =
		gtk_tree_view_get_selection (GTK_TREE_VIEW
				(glade_xml_get_widget (pl3_xml, "playlist_tree")));
	/* check if where connected */
	/* see if there is a row selected */
	if (gtk_tree_selection_count_selected_rows (selection) > 0)
	{
		GList *list = NULL, *llist = NULL;
		GtkTreeModel *model = GTK_TREE_MODEL(pl2_store);
		/* start a command list */
		/* grab the selected songs */
		list = gtk_tree_selection_get_selected_rows (selection, &model);
		/* grab the last song that is selected */
		llist = g_list_first (list);
		/* remove every selected song one by one */
		do{
			GtkTreeIter iter;
			int value;
			gtk_tree_model_get_iter (model, &iter,(GtkTreePath *) llist->data);
			gtk_tree_model_get (model, &iter, SONG_ID, &value, -1);
			mpd_ob_playlist_queue_delete_id(connection, value);			
		} while ((llist = g_list_next (llist)));

		/* close the list, so it will be executed */
		mpd_ob_playlist_queue_commit(connection);
		/* free list */
		g_list_foreach (list, (GFunc) gtk_tree_path_free, NULL);
		g_list_free (list);
	}
	else
	{
		/* create a warning message dialog */
		GtkWidget *dialog =
			gtk_message_dialog_new (GTK_WINDOW
					(glade_xml_get_widget
					 (pl3_xml, "pl3_win")),
					GTK_DIALOG_MODAL,
					GTK_MESSAGE_WARNING,
					GTK_BUTTONS_NONE,
					_
					("Are you sure you want to clear the playlist?"));
		gtk_dialog_add_buttons (GTK_DIALOG (dialog), GTK_STOCK_CANCEL,
				GTK_RESPONSE_CANCEL, GTK_STOCK_OK,
				GTK_RESPONSE_OK, NULL);
		gtk_dialog_set_default_response (GTK_DIALOG (dialog),
				GTK_RESPONSE_CANCEL);

		switch (gtk_dialog_run (GTK_DIALOG (dialog)))
		{
			case GTK_RESPONSE_OK:
				/* check if where still connected */
				mpd_ob_playlist_clear(connection);
		}
		gtk_widget_destroy (GTK_WIDGET (dialog));
	}
	/* update everything if where still connected */
	gtk_tree_selection_unselect_all(selection);

	mpd_ob_status_queue_update(connection);
}

void pl3_browser_current_playlist_crop_selected_songs()
{
	/* grab the selection from the tree */
	GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW(glade_xml_get_widget (pl3_xml, "playlist_tree")));

	/* see if there is a row selected */	
	if (gtk_tree_selection_count_selected_rows (selection) > 0)
	{
		GtkTreeIter iter;


		/* start a command list */
		/* remove every selected song one by one */
		if(gtk_tree_model_get_iter_first(GTK_TREE_MODEL(pl2_store), &iter))
		{
			do{
				int value=0;
				if(!gtk_tree_selection_iter_is_selected(selection, &iter))
				{
					gtk_tree_model_get (GTK_TREE_MODEL(pl2_store), &iter, SONG_ID, &value, -1);
					mpd_ob_playlist_queue_delete_id(connection, value);				
				}
			} while (gtk_tree_model_iter_next(GTK_TREE_MODEL(pl2_store),&iter));
			mpd_ob_playlist_queue_commit(connection);
		}

	}
	/* update everything if where still connected */
	gtk_tree_selection_unselect_all(selection);

	mpd_ob_status_queue_update(connection);
}

/* should this be here? */
void pl3_current_playlist_row_changed(GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter)
{
	gint pos, new_pos;
	gchar *str = NULL;         	
	gint type = pl3_cat_get_selected_browser();
	if(type != PL3_CURRENT_PLAYLIST) return;
	str = gtk_tree_path_to_string(path);

	gtk_tree_model_get(model, iter,SONG_POS, &pos, -1);
	new_pos = atoi(str);
	if(new_pos > pos ) new_pos --;
	/* if there wasn't a move action we don't do anything, because this signal is trigged on every row change */
	if(new_pos == pos)
	{
		g_free(str);
		return;
	}


	mpd_ob_playlist_move_pos(connection, pos, new_pos);
	gtk_list_store_set(pl2_store,iter, SONG_POS, new_pos, -1);
	g_free(str);
}

void pl3_browser_current_playlist_playlist_popup(GtkTreeView *tree, GdkEventButton *event)
{
		/* del, crop */
		GtkWidget *item;
		GtkWidget *menu = gtk_menu_new();	
		/* add the delete widget */
		item = gtk_image_menu_item_new_from_stock(GTK_STOCK_REMOVE,NULL);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
		g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(pl3_browser_current_playlist_delete_selected_songs), NULL);


		/* add the delete widget */
		item = gtk_image_menu_item_new_with_label("Crop");
		gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
				gtk_image_new_from_stock(GTK_STOCK_CUT, GTK_ICON_SIZE_MENU));
		g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(pl3_browser_current_playlist_crop_selected_songs), NULL);		
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

		gtk_menu_shell_append(GTK_MENU_SHELL(menu),gtk_separator_menu_item_new());
		/* add the clear widget */
		item = gtk_image_menu_item_new_from_stock(GTK_STOCK_CLEAR,NULL);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
		g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(pl3_clear_playlist), NULL);		


		/* add the shuffle widget */
		item = gtk_image_menu_item_new_with_label("Shuffle");
		gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
				gtk_image_new_from_stock(GTK_STOCK_REFRESH, GTK_ICON_SIZE_MENU));
		g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(pl3_shuffle_playlist), NULL);		
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

		gtk_menu_shell_append(GTK_MENU_SHELL(menu),gtk_separator_menu_item_new());

		item = gtk_image_menu_item_new_from_stock(GTK_STOCK_DIALOG_INFO,NULL);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
		g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(pl3_show_song_info), NULL);		




		gtk_widget_show_all(menu);
		gtk_menu_popup(GTK_MENU(menu), NULL, NULL,NULL, NULL, event->button, event->time);	
}

void pl3_browser_current_playlist_row_activated(GtkTreeView *tree, GtkTreePath *path, GtkTreeViewColumn *col)
{
	GtkTreeIter iter;
	gint song_id;
	gtk_tree_model_get_iter(gtk_tree_view_get_model(tree), &iter, path);
	gtk_tree_model_get(gtk_tree_view_get_model(tree), &iter, PL3_SONG_ID,&song_id, -1);
	mpd_ob_player_play_id(connection, song_id);
}

void pl3_browser_current_playlist_show_info(GtkTreeView *tree, GtkTreeIter *iter)
{
	gint value;
	gtk_tree_model_get (gtk_tree_view_get_model(tree), iter, SONG_ID, &value, -1);
	call_id3_window (value);

}


