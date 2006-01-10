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
#include <config.h>
#include <regex.h>
#include "plugin.h"


#include "main.h"
#include "misc.h"
#include "playlist3.h"
#include "playlist3-current-playlist-browser.h"
#include "config1.h"


static GtkTargetEntry drag_types[] =
{
   { "pm_data", GTK_TARGET_SAME_APP, 100},
};




/* just for here */
void pl3_current_playlist_browser_row_activated(GtkTreeView *tree, GtkTreePath *path, GtkTreeViewColumn *col);
int  pl3_current_playlist_browser_button_release_event(GtkTreeView *tree, GdkEventButton *event);
int  pl3_current_playlist_browser_key_release_event(GtkTreeView *tree, GdkEventKey *event);
void pl3_current_playlist_browser_show_info();
void pl3_current_playlist_row_changed(GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter);
void pl2_save_playlist ();
void pl3_current_playlist_browser_shuffle_playlist();
void pl3_current_playlist_browser_clear_playlist();
void pl3_current_playlist_browser_add_to_clipboard(int cut);


/* external objects */
extern GladeXML *pl3_xml;
extern GtkListStore *pl2_store;

/* internal */
GtkWidget *pl3_cp_tree = NULL;
GtkWidget *pl3_cp_sw = NULL;
GtkWidget *pl3_cp_vbox = NULL;
GtkWidget *pl3_cp_search_hbox=NULL;
GtkWidget *pl3_cp_search_entry=NULL;

int pl3_current_playlist_browser_button_press_event(GtkTreeView *tree, GdkEventButton *event)
{
	GtkTreeSelection *sel = gtk_tree_view_get_selection(tree);
	if(event->button != 3 || gtk_tree_selection_count_selected_rows(sel) < 2|| !mpd_check_connected(connection))	
	{
		return FALSE;                                                                                           	
	}
	return TRUE;
}

int pl3_cp_dnd(GtkTreeView *tree,GdkDragContext *drag_context,gint x,gint y,guint time)
{
	GtkTreePath *path=NULL;
	GtkTreeViewDropPosition pos = 0;
	GtkTreeSelection *selection = gtk_tree_view_get_selection(tree);
	gtk_tree_view_get_dest_row_at_pos(GTK_TREE_VIEW(tree),x,y, &path, &pos);
	int position = -1;

	if(path != NULL)
	{
		gchar *str = gtk_tree_path_to_string(path);
		position = atoi(str);
		if(pos == GTK_TREE_VIEW_DROP_AFTER)
		{
		}
		else if(pos == GTK_TREE_VIEW_DROP_BEFORE)
		{

		}
		g_free(str);
	}
	else
	{
	}
	if (gtk_tree_selection_count_selected_rows (selection) > 0 && position >=0)
	{
		GList *list = NULL, *llist = NULL;
		GtkTreeModel *model = GTK_TREE_MODEL(pl2_store);
		/* start a command list */
		/* grab the selected songs */
		list = gtk_tree_selection_get_selected_rows (selection, &model);
		/* grab the last song that is selected */
		llist = g_list_first (list);
		/* remove every selected song one by one */
		int test=0;
		do{
			GtkTreeIter iter;
			int value;
			int dest = position;
			gtk_tree_model_get_iter (model, &iter,(GtkTreePath *) llist->data);
			gtk_tree_model_get (model, &iter, SONG_POS, &value, -1);
			if(position < value && pos ==  GTK_TREE_VIEW_DROP_AFTER) dest++;
			mpd_playlist_move_pos(connection, value-test,dest);			
			if(position > value) test++;
			if(position < value)position++;
			
		} while ((llist = g_list_next (llist)));

		/* free list */
		g_list_foreach (list, (GFunc) gtk_tree_path_free, NULL);                        	
		g_list_free (list);
	}
	gtk_drag_finish(drag_context, TRUE, FALSE, time);
	gtk_tree_selection_unselect_all(selection);
	return TRUE;
}
void pl3_current_playlist_search_next_iter(GtkTreeModel *model,GtkTreeIter *iter) {
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(pl3_cp_tree));
	const char *text = gtk_entry_get_text(GTK_ENTRY(pl3_cp_search_entry));
	regex_t regt;
	if(strlen(text) == 0)
	{
		gtk_tree_selection_unselect_all(selection);       
		return;
	}
	if(regcomp(&regt, text, REG_EXTENDED|REG_ICASE|REG_NOSUB)) {
		gtk_tree_selection_unselect_all(selection);       
		return;
	}
		
	if(strlen(text) >0)
	{
		do{
			char *title;
			gtk_tree_model_get(model, iter,SONG_TITLE, &title, -1); 
			if(!regexec(&regt, title, 0,NULL,0))
			{
		
				GtkTreePath *path = gtk_tree_model_get_path(model, iter);
				gtk_tree_selection_unselect_all(selection);
				gtk_tree_selection_select_iter(selection, iter);
				gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(pl3_cp_tree), path,NULL,TRUE,0.5,0.5);
				g_free(title);
				gtk_tree_path_free(path);
				return;
			}
			g_free(title);
		} while(gtk_tree_model_iter_next(model,  iter));
		gtk_tree_selection_unselect_all(selection);       
	}
	return;
}
void pl3_current_playlist_search()
{
	GtkTreeIter iter;
	GtkTreeModel *model = GTK_TREE_MODEL(pl2_store);
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(pl3_cp_tree));
	if (gtk_tree_selection_count_selected_rows (selection) == 1)
	{

		GList *list = NULL;
		list = gtk_tree_selection_get_selected_rows (selection, &model);
		gtk_tree_model_get_iter (model, &iter,(GtkTreePath *) list->data);
		pl3_current_playlist_search_next_iter(model,&iter);
		/* free list */
		g_list_foreach (list, (GFunc) gtk_tree_path_free, NULL);                        	
		g_list_free (list);
	}
	else{
		if(gtk_tree_model_get_iter_first(model, &iter))
		{
			pl3_current_playlist_search_next_iter(model,&iter);
		}
	}

}
void pl3_current_playlist_search_hide()
{
	gtk_widget_hide(pl3_cp_search_hbox);
	gtk_widget_grab_focus(pl3_cp_tree);

}

void pl3_current_playlist_search_activate(GtkEntry *entry)
{
	const gchar *text = gtk_entry_get_text(entry);
	GtkTreeModel *model = GTK_TREE_MODEL(pl2_store);
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(pl3_cp_tree));
	if (gtk_tree_selection_count_selected_rows (selection) == 1 && strlen(text))            
	{
		GList *list = gtk_tree_selection_get_selected_rows (selection, &model);
		pl3_current_playlist_browser_row_activated(GTK_TREE_VIEW(pl3_cp_tree),(GtkTreePath *)list->data, NULL);	
		/* free list */
		g_list_foreach (list, (GFunc) gtk_tree_path_free, NULL);                        	
		g_list_free (list);
	}
	pl3_current_playlist_search_hide();
}

void pl3_current_playlist_browser_init()
{
	GtkWidget *wid = NULL;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column = NULL;
	GValue value;


	renderer = gtk_cell_renderer_pixbuf_new ();

	column = gtk_tree_view_column_new ();
	gtk_tree_view_column_pack_start (column, renderer, FALSE);
	gtk_tree_view_column_set_attributes (column,renderer,"stock-id", SONG_STOCK_ID,NULL);
	memset(&value, 0, sizeof(value));
	/* set value for ALL */
	g_value_init(&value, G_TYPE_FLOAT);
	g_value_set_float(&value, 0.0);
	g_object_set_property(G_OBJECT(renderer), "yalign", &value); 

	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	/* set value for ALL */
	memset(&value, 0, sizeof(value));
	g_value_init(&value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&value, TRUE);
	memset(&value, 0, sizeof(value));

#if GTK_CHECK_VERSION(2,6,0)
	g_value_init(&value, G_TYPE_INT);
	g_value_set_int(&value, PANGO_ELLIPSIZE_END);
	g_object_set_property(G_OBJECT(renderer), "ellipsize", &value);	
#endif
	gtk_tree_view_column_set_attributes (column,renderer,"text", SONG_TITLE, "weight", WEIGHT_INT,NULL);
	g_object_set_property(G_OBJECT(renderer), "weight-set", &value);                                    	


	/* set up the tree */
	pl3_cp_tree= gtk_tree_view_new_with_model(GTK_TREE_MODEL(pl2_store));
	/* insert the column in the tree */
	gtk_tree_view_append_column (GTK_TREE_VIEW (pl3_cp_tree), column);                                         	
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(pl3_cp_tree), FALSE);
	gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(pl3_cp_tree), TRUE);
	gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW(pl3_cp_tree)), GTK_SELECTION_MULTIPLE);
	gtk_tree_view_set_enable_search(GTK_TREE_VIEW(pl3_cp_tree), FALSE);

	/* setup signals */
	g_signal_connect(G_OBJECT(pl3_cp_tree), "row-activated",G_CALLBACK(pl3_current_playlist_browser_row_activated), NULL); 
	g_signal_connect(G_OBJECT(pl3_cp_tree), "button-press-event", G_CALLBACK(pl3_current_playlist_browser_button_press_event), NULL);
	g_signal_connect(G_OBJECT(pl3_cp_tree), "button-release-event", G_CALLBACK(pl3_current_playlist_browser_button_release_event), NULL);
	g_signal_connect(G_OBJECT(pl3_cp_tree), "key-press-event", G_CALLBACK(pl3_current_playlist_browser_key_release_event), NULL);


	g_signal_connect(pl2_store, "row-changed", G_CALLBACK(pl3_current_playlist_row_changed), NULL);


	/* set up the scrolled window */
	pl3_cp_sw = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(pl3_cp_sw), GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(pl3_cp_sw), GTK_SHADOW_ETCHED_IN);
	pl3_cp_vbox = gtk_vbox_new(FALSE,6);

	gtk_container_add(GTK_CONTAINER(pl3_cp_sw), pl3_cp_tree);
	gtk_box_pack_start(GTK_BOX(pl3_cp_vbox), pl3_cp_sw, TRUE, TRUE,0);
	gtk_widget_show_all(pl3_cp_sw);

	gtk_drag_source_set(GTK_WIDGET(pl3_cp_tree), GDK_BUTTON1_MASK, drag_types, 1, GDK_ACTION_COPY);
	gtk_tree_view_enable_model_drag_dest (GTK_TREE_VIEW(pl3_cp_tree), drag_types, 1, GDK_ACTION_COPY);
	/*	gtk_drag_source_set_icon_name(pl3_cp_tree, "gtk-dnd");*/

	g_signal_connect(G_OBJECT(pl3_cp_tree), "drag-drop", G_CALLBACK(pl3_cp_dnd), NULL);

	pl3_cp_search_hbox = gtk_hbox_new(FALSE, 6);
	wid = gtk_label_new("Find");	
	gtk_box_pack_start(GTK_BOX(pl3_cp_search_hbox), wid, FALSE, TRUE, 0);
	pl3_cp_search_entry = gtk_entry_new();
	g_signal_connect(G_OBJECT(pl3_cp_search_entry), "changed", G_CALLBACK(pl3_current_playlist_search), NULL);
	g_signal_connect(G_OBJECT(pl3_cp_search_entry), "activate", G_CALLBACK(pl3_current_playlist_search_activate), NULL);

	gtk_box_pack_start(GTK_BOX(pl3_cp_search_hbox), pl3_cp_search_entry, FALSE, TRUE, 0);
	wid = gtk_button_new_from_stock(GTK_STOCK_GO_UP);
	gtk_box_pack_start(GTK_BOX(pl3_cp_search_hbox), wid, FALSE, TRUE, 0);
	wid = gtk_button_new_from_stock(GTK_STOCK_GO_DOWN);
	gtk_box_pack_start(GTK_BOX(pl3_cp_search_hbox), wid, FALSE, TRUE, 0);
	wid = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
	gtk_box_pack_end(GTK_BOX(pl3_cp_search_hbox), wid, FALSE, TRUE, 0);
	g_signal_connect_swapped(G_OBJECT(wid), "clicked", G_CALLBACK(pl3_current_playlist_search_hide), pl3_cp_search_hbox);	
	gtk_box_pack_end(GTK_BOX(pl3_cp_vbox), pl3_cp_search_hbox, FALSE, TRUE, 0);	
	/* set initial state */
	g_object_ref(G_OBJECT(pl3_cp_vbox));
}



void pl3_current_playlist_browser_scroll_to_current_song()
{
	/* scroll to the playing song */
	if(mpd_player_get_current_song_pos(connection) >= 0 && mpd_playlist_get_playlist_length(connection)  > 0)
	{
		gchar *str = g_strdup_printf("%i", mpd_player_get_current_song_pos(connection));
		GtkTreePath *path = gtk_tree_path_new_from_string(str);
		if(path != NULL)
		{
			gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(pl3_cp_tree),
					path,
					NULL,
					TRUE,0.5,0);
		}
		gtk_tree_path_free(path);
		g_free(str);
	}      
}

/* add's the toplevel entry for the current playlist view */
void pl3_current_playlist_browser_add()
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
void pl3_current_playlist_browser_delete_selected_songs ()
{
	/* grab the selection from the tree */
	GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW(pl3_cp_tree));
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
			mpd_playlist_queue_delete_id(connection, value);			
		} while ((llist = g_list_next (llist)));

		/* close the list, so it will be executed */
		mpd_playlist_queue_commit(connection);
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
				mpd_playlist_clear(connection);
		}
		gtk_widget_destroy (GTK_WIDGET (dialog));
	}
	/* update everything if where still connected */
	gtk_tree_selection_unselect_all(selection);

	mpd_status_queue_update(connection);
}
void pl3_current_playlist_browser_clipboard_add_foreach(char *path, gpointer data)
{
	//int pos = GPOINTER_TO_INT(data);
	mpd_playlist_add(connection,path);
	g_free(path);
}
void pl3_current_playlist_browser_clipboard_paste()
{
	int id = 0;
	if(g_queue_get_length(pl3_queue) > 0)
	{
		g_queue_foreach(pl3_queue, (GFunc)(pl3_current_playlist_browser_clipboard_add_foreach), GINT_TO_POINTER(id));
		/* how do I remove all elements propperly at once? */
		g_queue_free(pl3_queue);
		pl3_queue = g_queue_new();
	}
}
void pl3_current_playlist_browser_clipboard_cut()
{
	pl3_current_playlist_browser_add_to_clipboard(1);
}
void pl3_current_playlist_browser_clipboard_copy()
{
	pl3_current_playlist_browser_add_to_clipboard(0);
}

void pl3_current_playlist_browser_add_to_clipboard(int cut)
{
	/* grab the selection from the tree */
	GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW(pl3_cp_tree));
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
			char *path = NULL;
			gtk_tree_model_get_iter (model, &iter,(GtkTreePath *) llist->data);
			gtk_tree_model_get (model, &iter, SONG_ID, &value,SONG_PATH,&path, -1);
			g_queue_push_tail(pl3_queue, path);
			if(cut)
			{
				mpd_playlist_queue_delete_id(connection, value);			
			}
		} while ((llist = g_list_next (llist)));

		/* close the list, so it will be executed */
		if(cut)
		{
			mpd_playlist_queue_commit(connection);
		}
		/* free list */
		g_list_foreach (list, (GFunc) gtk_tree_path_free, NULL);
		g_list_free (list);
	}

	/* update everything if where still connected */
	gtk_tree_selection_unselect_all(selection);
	if(cut)
	{
		mpd_status_queue_update(connection);
	}
}

void pl3_current_playlist_browser_crop_selected_songs()
{
	/* grab the selection from the tree */
	GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW(pl3_cp_tree));

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
					mpd_playlist_queue_delete_id(connection, value);				
				}
			} while (gtk_tree_model_iter_next(GTK_TREE_MODEL(pl2_store),&iter));
			mpd_playlist_queue_commit(connection);
		}

	}
	/* update everything if where still connected */
	gtk_tree_selection_unselect_all(selection);

	mpd_status_queue_update(connection);
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


	mpd_playlist_move_pos(connection, pos, new_pos);
	gtk_list_store_set(pl2_store,iter, SONG_POS, new_pos, -1);
	g_free(str);
}

int pl3_current_playlist_browser_button_release_event(GtkTreeView *tree, GdkEventButton *event)
{
	if(event->button == 3)
	{
		/* del, crop */
		GtkWidget *item;
		GtkWidget *menu = gtk_menu_new();	
		/* add the delete widget */
		item = gtk_image_menu_item_new_from_stock(GTK_STOCK_REMOVE,NULL);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
		g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(pl3_current_playlist_browser_delete_selected_songs), NULL);


		/* add the delete widget */
		item = gtk_image_menu_item_new_with_label("Crop");
		gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
				gtk_image_new_from_stock(GTK_STOCK_CUT, GTK_ICON_SIZE_MENU));
		g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(pl3_current_playlist_browser_crop_selected_songs), NULL);		
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

		gtk_menu_shell_append(GTK_MENU_SHELL(menu),gtk_separator_menu_item_new());
		/* add the clear widget */
		item = gtk_image_menu_item_new_from_stock(GTK_STOCK_CLEAR,NULL);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
		g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(pl3_current_playlist_browser_clear_playlist), NULL);		


		/* add the shuffle widget */
		item = gtk_image_menu_item_new_with_label("Shuffle");
		gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
				gtk_image_new_from_stock(GTK_STOCK_REFRESH, GTK_ICON_SIZE_MENU));
		g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(pl3_current_playlist_browser_shuffle_playlist), NULL);		
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

		gtk_menu_shell_append(GTK_MENU_SHELL(menu),gtk_separator_menu_item_new());

		item = gtk_image_menu_item_new_from_stock(GTK_STOCK_DIALOG_INFO,NULL);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
		g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(pl3_current_playlist_browser_show_info), NULL);		


		/*	
			item = gtk_image_menu_item_new_from_stock(GTK_STOCK_CUT,NULL);
			gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
			g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(pl3_current_playlist_browser_clipboard_cut), NULL);		
			item = gtk_image_menu_item_new_from_stock(GTK_STOCK_COPY,NULL);
			gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
			g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(pl3_current_playlist_browser_clipboard_copy), NULL);		


			if(g_queue_get_length(pl3_queue)>0)
			{
			item = gtk_image_menu_item_new_from_stock(GTK_STOCK_PASTE,NULL);
			gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
			g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(pl3_current_playlist_browser_clipboard_paste), NULL);		

			}
			*/	


		gtk_widget_show_all(menu);
		gtk_menu_popup(GTK_MENU(menu), NULL, NULL,NULL, NULL, event->button, event->time);	
	}
	return 1;
}

void pl3_current_playlist_browser_row_activated(GtkTreeView *tree, GtkTreePath *path, GtkTreeViewColumn *col)
{
	GtkTreeIter iter;
	gint song_id;
	gtk_tree_model_get_iter(gtk_tree_view_get_model(tree), &iter, path);
	gtk_tree_model_get(gtk_tree_view_get_model(tree), &iter, PL3_SONG_ID,&song_id, -1);
	mpd_player_play_id(connection, song_id);
}

void pl3_current_playlist_browser_show_info()
{
	gint value;
	GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW(pl3_cp_tree));
	GtkTreeSelection *selection =gtk_tree_view_get_selection (GTK_TREE_VIEW(pl3_cp_tree));
	if (gtk_tree_selection_count_selected_rows (selection) > 0)
	{
		GList *list = NULL;
		list = gtk_tree_selection_get_selected_rows (selection, &model);
		/* iterate over every row */
		list = g_list_last (list);
		do
		{
			GtkTreeIter iter;
			gtk_tree_model_get_iter (model, &iter, (GtkTreePath *) list->data);
			gtk_tree_model_get (model, &iter, SONG_ID, &value, -1);
			call_id3_window (value);
		}
		while ((list = g_list_previous (list)) && mpd_check_connected(connection));
		/* free list */
		g_list_foreach (list, (GFunc) gtk_tree_path_free, NULL);
		g_list_free (list);
	}
}

void pl3_current_playlist_browser_selected()
{
	if(pl3_cp_vbox == NULL)
	{
		pl3_current_playlist_browser_init();
	}

	gtk_container_add(GTK_CONTAINER(glade_xml_get_widget(pl3_xml, "browser_container")), pl3_cp_vbox);
	gtk_widget_show(pl3_cp_vbox);
	pl3_current_playlist_browser_playlist_changed();


	if(cfg_get_single_value_as_int_with_default(config, "playlist3", "st_cur_song", 0))
	{
		pl3_current_playlist_browser_scroll_to_current_song();
	}
}
void pl3_current_playlist_browser_unselected()
{
	gtk_container_remove(GTK_CONTAINER(glade_xml_get_widget(pl3_xml, "browser_container")), pl3_cp_vbox);
}


void pl3_current_playlist_browser_playlist_changed()
{
	gchar *string = format_time(info.playlist_playtime);
	gtk_statusbar_push(GTK_STATUSBAR(glade_xml_get_widget(pl3_xml, "statusbar2")),0, string);	
	g_free(string);
}


int pl3_current_playlist_browser_cat_menu_popup(GtkWidget *menu, int type, GtkTreeView *tree, GdkEventButton *event)
{
	/* here we have:  Save, Clear*/
	GtkWidget *item;
	if(type != PL3_CURRENT_PLAYLIST) return 0;
	/* add the save widget */
	item = gtk_image_menu_item_new_from_stock(GTK_STOCK_SAVE,NULL);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	/* TODO: Write own fun ction */
	g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(pl2_save_playlist), NULL);

	/* add the clear widget */
	item = gtk_image_menu_item_new_from_stock(GTK_STOCK_CLEAR,NULL);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(pl3_current_playlist_browser_clear_playlist), NULL);


	return 1;
}

int  pl3_current_playlist_browser_key_release_event(GtkTreeView *tree, GdkEventKey *event)
{
	if(event->keyval == GDK_Delete)
	{
		pl3_current_playlist_browser_delete_selected_songs ();
		return TRUE;                                          		
	}
	else if(event->keyval == GDK_i)
	{
		pl3_current_playlist_browser_show_info();
		return TRUE;
	}
	else if (event->keyval == GDK_space)
	{
		pl3_current_playlist_browser_scroll_to_current_song();
		return TRUE;			
	}
	else if (event->keyval == GDK_f && event->state&GDK_CONTROL_MASK)
	{
		gtk_widget_show_all(pl3_cp_search_hbox);
		gtk_widget_grab_focus(pl3_cp_search_entry);
		return TRUE;
	}
	return pl3_window_key_press_event(GTK_WIDGET(tree),event);
}

/* create a dialog that allows the user to save the current playlist */
void pl2_save_playlist ()
{
	gchar *str;
	GladeXML *xml = NULL;
	int run = TRUE;
	/* check if the connection is up */
	if (!mpd_check_connected(connection))
	{
		return;
	}
	/* create the interface */
	str = gmpc_get_full_glade_path("playlist3.glade");
	xml = glade_xml_new (str, "save_pl", NULL);
	g_free(str);

	/* run the interface */
	do
	{
		switch (gtk_dialog_run (GTK_DIALOG (glade_xml_get_widget (xml, "save_pl"))))
		{
			case GTK_RESPONSE_OK:
				run = FALSE;
				/* if the users agrees do the following: */
				/* get the song-name */
				str = (gchar *)	gtk_entry_get_text (GTK_ENTRY
						(glade_xml_get_widget (xml, "pl-entry")));
				/* check if the user entered a name, we can't do withouth */
				/* TODO: disable ok button when nothing is entered */
				/* also check if there is a connection */
				if (strlen (str) != 0 && mpd_check_connected(connection))
				{
					if(mpd_playlist_save(connection, str) == MPD_PLAYLIST_EXIST )
					{
						gchar *errormsg = g_strdup_printf(_("<i>Playlist <b>\"%s\"</b> already exists\nOverwrite?</i>"), str);
						gtk_label_set_markup(GTK_LABEL(glade_xml_get_widget(xml, "label_error")), errormsg);
						gtk_widget_show(glade_xml_get_widget(xml, "hbox5"));
						/* ask to replace */
						gtk_widget_set_sensitive(GTK_WIDGET(glade_xml_get_widget(xml, "pl-entry")), FALSE);
						switch (gtk_dialog_run (GTK_DIALOG (glade_xml_get_widget (xml, "save_pl"))))
						{
							case GTK_RESPONSE_OK:
								run = FALSE;
								mpd_playlist_delete(connection, str);
								mpd_playlist_save(connection,str);
								break;
							default:
								run = TRUE;
						}
						/* return to stare */
						gtk_widget_set_sensitive(GTK_WIDGET(glade_xml_get_widget(xml, "pl-entry")), TRUE);
						gtk_widget_hide(glade_xml_get_widget(xml, "hbox5"));

						g_free(errormsg);
					}
				}
				break;
			default:
				run = FALSE;
		}
	}while(run);
	/* destroy the window */
	gtk_widget_destroy (glade_xml_get_widget (xml, "save_pl"));

	/* unref the gui description */
	g_object_unref (xml);
}

void pl3_current_playlist_browser_clear_playlist()
{
	mpd_playlist_clear(connection);
}

void pl3_current_playlist_browser_shuffle_playlist()
{
	mpd_playlist_shuffle(connection);
}
