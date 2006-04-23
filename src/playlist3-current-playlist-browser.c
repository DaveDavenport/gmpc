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
#include "TreeSearchWidget.h"

static GtkTargetEntry drag_types[] =
{
   { "pm_data", GTK_TARGET_SAME_APP, 100},
};

GtkTreeModel *playlist = NULL;


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

/* internal */
GtkWidget *pl3_cp_tree = NULL;
GtkWidget *pl3_cp_sw = NULL;
GtkWidget *pl3_cp_vbox = NULL;
TreeSearch *tree_search = NULL;

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
		GtkTreeModel *model = GTK_TREE_MODEL(playlist);
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
			gtk_tree_model_get (model, &iter, PLAYLIST_LIST_COL_SONG_POS, &value, -1);
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

void pl3_current_playlist_search_activate()
{
	GtkTreeModel *model = GTK_TREE_MODEL(playlist);
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(pl3_cp_tree));
	if (gtk_tree_selection_count_selected_rows (selection) == 1)            
	{
		GList *list = gtk_tree_selection_get_selected_rows (selection, &model);
		pl3_current_playlist_browser_row_activated(GTK_TREE_VIEW(pl3_cp_tree),(GtkTreePath *)list->data, NULL);	
		/* free list */
		g_list_foreach (list, (GFunc) gtk_tree_path_free, NULL);                        	
		g_list_free (list);
	}
}

static GtkTreeViewColumn * pl3_current_playlist_add_column(GtkWidget *tree, char *columnname, int valuerow, int position)
{
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column = NULL;
	GValue value = {0,};
	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes (columnname, renderer,
			"text", valuerow, 
			"weight", PLAYLIST_LIST_COL_PLAYING_FONT_WEIGHT,
			NULL);
	gtk_tree_view_column_set_sizing(column,GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_column_set_fixed_width(column, 200);
	gtk_tree_view_column_set_resizable(column, TRUE);

	g_value_init(&value, G_TYPE_INT);
	g_value_set_int(&value, PANGO_ELLIPSIZE_END);
	g_object_set_property(G_OBJECT(renderer), "ellipsize", &value);	


//	gtk_tree_view_insert_column (GTK_TREE_VIEW (tree), column,position);                                         	  		
	gtk_tree_view_column_set_reorderable(column, TRUE);
	return column;
}

void pl3_current_playlist_column_changed(GtkTreeView *tree)
{
	int position = 0;
	GList *iter,*cols = gtk_tree_view_get_columns(tree);
	for(iter = cols; iter; iter = g_list_next(iter))
	{
		gpointer data = g_object_get_data(G_OBJECT(iter->data), "colid");
		int colid = GPOINTER_TO_INT(data);
		char *string = g_strdup_printf("%i", position);
		cfg_set_single_value_as_int(config, "current-playlist-column-pos", string, colid);
		position++;
		g_free(string);
	}
	g_list_free(cols);
}

void pl3_current_playlist_destroy()
{
	if(pl3_cp_tree)
	{
		GList *iter,*cols = gtk_tree_view_get_columns(GTK_TREE_VIEW(pl3_cp_tree));
		for(iter = cols; iter; iter = g_list_next(iter))
		{
			gpointer data = g_object_get_data(G_OBJECT(iter->data), "colid");
			int colid = GPOINTER_TO_INT(data);
			gchar *string = g_strdup_printf("%i", colid);
			int width = gtk_tree_view_column_get_width(GTK_TREE_VIEW_COLUMN(iter->data));
			cfg_set_single_value_as_int(config, "current-playlist-column-width", string,width);
			g_free(string);
		}
		g_list_free(cols);
	}
}

void pl3_current_playlist_browser_init()
{
	int position = 0;
	gchar smallstring[6];
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column = NULL;
	GtkTreeViewColumn *columns[PL_COLUMN_TOTAL];

	GValue value = {0,};
	/* set up the tree */
	pl3_cp_tree= gtk_tree_view_new_with_model(GTK_TREE_MODEL(playlist));
	renderer = gtk_cell_renderer_pixbuf_new ();

	column = gtk_tree_view_column_new ();
	gtk_tree_view_column_pack_start (column, renderer, FALSE);
	gtk_tree_view_column_set_attributes (column,renderer,"stock-id", PLAYLIST_LIST_COL_ICON_ID,NULL);
/*	memset(&value, 0, sizeof(value));*/

	/* set value for ALL */
	g_value_init(&value, G_TYPE_FLOAT);
	g_value_set_float(&value, 0.0);
	g_object_set_property(G_OBJECT(renderer), "yalign", &value); 
	gtk_tree_view_column_set_fixed_width(column, 20);
	gtk_tree_view_column_set_sizing(column,GTK_TREE_VIEW_COLUMN_FIXED);	

	//	gtk_tree_view_append_column (GTK_TREE_VIEW (pl3_cp_tree), column);                                         	  
	g_object_set_data(G_OBJECT(column), "colid", GINT_TO_POINTER(PL_COLUMN_ICON));
	columns[PL_COLUMN_ICON] = column;
	sprintf(smallstring,"%i", PL_COLUMN_ICON);
	if(!cfg_get_single_value_as_int_with_default(config, "current-playlist-column-enable", smallstring, FALSE))
	{
		gtk_tree_view_column_set_visible(column, FALSE);	
	}                                                                                                          	
	gtk_tree_view_column_set_reorderable(column, TRUE);

	/* markup column */
	column = pl3_current_playlist_add_column(pl3_cp_tree,_("Markup"), PLAYLIST_LIST_COL_MARKUP,-1);
	sprintf(smallstring,"%i", PL_COLUMN_MARKUP);
	if(!cfg_get_single_value_as_int_with_default(config, "current-playlist-column-enable", smallstring, FALSE))
	{
		gtk_tree_view_column_set_visible(column, FALSE);	
	}
	g_object_set_data(G_OBJECT(column), "colid", GINT_TO_POINTER(PL_COLUMN_MARKUP));

	columns[PL_COLUMN_MARKUP] = column;
	column = pl3_current_playlist_add_column(pl3_cp_tree, _("Artist"), PLAYLIST_LIST_COL_SONG_ARTIST,-1);
	sprintf(smallstring,"%i", PL_COLUMN_ARTIST);
	if(!cfg_get_single_value_as_int_with_default(config, "current-playlist-column-enable", smallstring, TRUE))
	{
		gtk_tree_view_column_set_visible(column, FALSE);	
	}
	g_object_set_data(G_OBJECT(column), "colid", GINT_TO_POINTER(PL_COLUMN_ARTIST));
	columns[PL_COLUMN_ARTIST] = column;

	column = pl3_current_playlist_add_column(pl3_cp_tree, _("Track"), PLAYLIST_LIST_COL_SONG_TRACK,-1);
	sprintf(smallstring,"%i", PL_COLUMN_TRACK);
	if(!cfg_get_single_value_as_int_with_default(config, "current-playlist-column-enable", smallstring, FALSE))
	{
		gtk_tree_view_column_set_visible(column, FALSE);	
	}                                                                                                     	
	g_object_set_data(G_OBJECT(column), "colid", GINT_TO_POINTER(PL_COLUMN_TRACK));
	columns[PL_COLUMN_TRACK] = column;

	column = pl3_current_playlist_add_column(pl3_cp_tree, _("Title"), PLAYLIST_LIST_COL_SONG_TITLEFILE,-1);
	sprintf(smallstring,"%i", PL_COLUMN_TITLEFILE);
	if(!cfg_get_single_value_as_int_with_default(config, "current-playlist-column-enable", smallstring, TRUE))
	{
		gtk_tree_view_column_set_visible(column, FALSE);	
	}
	g_object_set_data(G_OBJECT(column), "colid", GINT_TO_POINTER(PL_COLUMN_TITLEFILE));
	columns[PL_COLUMN_TITLEFILE] = column;

	column = pl3_current_playlist_add_column(pl3_cp_tree, _("Album"), PLAYLIST_LIST_COL_SONG_ALBUM,-1);
	sprintf(smallstring,"%i", PL_COLUMN_ALBUM);
	if(!cfg_get_single_value_as_int_with_default(config, "current-playlist-column-enable",smallstring, TRUE))
	{
		gtk_tree_view_column_set_visible(column, FALSE);	
	}
	g_object_set_data(G_OBJECT(column), "colid", GINT_TO_POINTER(PL_COLUMN_ALBUM));
	columns[PL_COLUMN_ALBUM] = column;

	column = pl3_current_playlist_add_column(pl3_cp_tree, _("Genre"), PLAYLIST_LIST_COL_SONG_GENRE,-1);
	sprintf(smallstring,"%i", PL_COLUMN_GENRE);
	if(!cfg_get_single_value_as_int_with_default(config, "current-playlist-column-enable", smallstring, TRUE))
	{
		gtk_tree_view_column_set_visible(column, FALSE);	
	}
	g_object_set_data(G_OBJECT(column), "colid", GINT_TO_POINTER(PL_COLUMN_GENRE));
	columns[PL_COLUMN_GENRE] = column;

	column = pl3_current_playlist_add_column(pl3_cp_tree, _("Composer"), PLAYLIST_LIST_COL_SONG_COMPOSER,-1);
	sprintf(smallstring,"%i", PL_COLUMN_COMPOSER);
	if(!cfg_get_single_value_as_int_with_default(config, "current-playlist-column-enable", smallstring, FALSE))
	{
		gtk_tree_view_column_set_visible(column, FALSE);	
	}                                                                                                     	
	g_object_set_data(G_OBJECT(column), "colid", GINT_TO_POINTER(PL_COLUMN_COMPOSER));
	columns[PL_COLUMN_COMPOSER] = column;

	for(position=0; position<PL_COLUMN_TOTAL;position++)
	{
		sprintf(smallstring, "%i", position);
		int size;
		int pos = cfg_get_single_value_as_int_with_default(config, "current-playlist-column-pos",smallstring, position);
		gtk_tree_view_append_column(GTK_TREE_VIEW(pl3_cp_tree), columns[pos]);
		sprintf(smallstring, "%i", pos);
		size = 	cfg_get_single_value_as_int_with_default(config, "current-playlist-column-width", smallstring,200);
		if(pos != PL_COLUMN_ICON)
		{
			gtk_tree_view_column_set_fixed_width(columns[pos], (size>0)?size:200);	
		}
	}
	/**
	 * Very dirty hack, to stop the last column from growing to far
	 */
	for(position=PL_COLUMN_TOTAL-1; position>=0;position--)
	{
		sprintf(smallstring, "%i", position);
		if(cfg_get_single_value_as_int_with_default(config, "current-playlist-column-enable", smallstring, FALSE))
		{
			gtk_tree_view_column_set_fixed_width(columns[position],1);			
			position = -1;
		}
	}

	gtk_tree_view_set_fixed_height_mode(GTK_TREE_VIEW(pl3_cp_tree), TRUE);
	g_signal_connect(G_OBJECT(pl3_cp_tree),"columns-changed", G_CALLBACK(pl3_current_playlist_column_changed), NULL);

	/* insert the column in the tree */


	gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(pl3_cp_tree), TRUE);
	gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW(pl3_cp_tree)), GTK_SELECTION_MULTIPLE);


	if(!cfg_get_single_value_as_int_with_default(config, "current-playlist", "header",TRUE))
	{
		gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(pl3_cp_tree), FALSE);
	}

	/* Disable search, we have a custom search */
	gtk_tree_view_set_enable_search(GTK_TREE_VIEW(pl3_cp_tree), FALSE);

	/* setup signals */
	g_signal_connect(G_OBJECT(pl3_cp_tree), "row-activated",G_CALLBACK(pl3_current_playlist_browser_row_activated), NULL); 
	g_signal_connect(G_OBJECT(pl3_cp_tree), "button-press-event", G_CALLBACK(pl3_current_playlist_browser_button_press_event), NULL);
	g_signal_connect(G_OBJECT(pl3_cp_tree), "button-release-event", G_CALLBACK(pl3_current_playlist_browser_button_release_event), NULL);
	g_signal_connect(G_OBJECT(pl3_cp_tree), "key-press-event", G_CALLBACK(pl3_current_playlist_browser_key_release_event), NULL);

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


	tree_search = (TreeSearch *)treesearch_new(GTK_TREE_VIEW(pl3_cp_tree), PLAYLIST_LIST_COL_MARKUP);
	g_signal_connect(G_OBJECT(tree_search), "result-activate", G_CALLBACK(pl3_current_playlist_search_activate),NULL);


	gtk_box_pack_end(GTK_BOX(pl3_cp_vbox), GTK_WIDGET(tree_search), FALSE, TRUE, 0);	
	/* set initial state */
	g_object_ref(G_OBJECT(pl3_cp_vbox));
}



void pl3_current_playlist_browser_scroll_to_current_song()
{
	if(pl3_cp_tree == NULL) return;
	/* scroll to the playing song */
	if(mpd_player_get_current_song_pos(connection) >= 0 && mpd_playlist_get_playlist_length(connection)  > 0&&
			gtk_tree_view_get_model(GTK_TREE_VIEW(pl3_cp_tree)))
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
		GtkTreeModel *model = GTK_TREE_MODEL(playlist);
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
			gtk_tree_model_get (model, &iter, PLAYLIST_LIST_COL_SONG_ID, &value, -1);
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
		if(gtk_tree_model_get_iter_first(GTK_TREE_MODEL(playlist), &iter))
		{
			do{
				int value=0;
				if(!gtk_tree_selection_iter_is_selected(selection, &iter))
				{
					gtk_tree_model_get (GTK_TREE_MODEL(playlist), &iter, PLAYLIST_LIST_COL_SONG_ID, &value, -1);
					mpd_playlist_queue_delete_id(connection, value);				
				}
			} while (gtk_tree_model_iter_next(GTK_TREE_MODEL(playlist),&iter));
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

	gtk_tree_model_get(model, iter,PLAYLIST_LIST_COL_SONG_POS, &pos, -1);
	new_pos = atoi(str);
	if(new_pos > pos ) new_pos --;
	/* if there wasn't a move action we don't do anything, because this signal is trigged on every row change */
	if(new_pos == pos)
	{
		g_free(str);
		return;
	}

	mpd_playlist_move_pos(connection, pos, new_pos);
	g_free(str);
}

void pl3_current_playlist_header_toggle(GtkCheckButton *cb)
{
	int active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(cb));
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(pl3_cp_tree),
			active);
	cfg_set_single_value_as_int(config, "current-playlist", "header", active);


}
void pl3_current_playlist_checkbox_selected(GtkCheckButton *cb)
{
	int active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(cb));
	GtkTreeViewColumn *column = GTK_TREE_VIEW_COLUMN(g_object_get_data(G_OBJECT(cb), "column"));
	int colid = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(column), "colid"));
	gchar  *string = g_strdup_printf("%i", colid);
	gtk_tree_view_column_set_visible(column, active);
	cfg_set_single_value_as_int(config, "current-playlist-column-enable",string, active);	
	g_free(string);
}

void pl3_current_playlist_enable_columns()
{
	GtkWidget *dialog = NULL;
	GList *cols, *iter;
	GtkWidget *vbox;
	GtkWidget *label;
	dialog = gtk_dialog_new_with_buttons("Select Columns", 
			NULL,
			GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_STOCK_CLOSE,
			GTK_RESPONSE_OK,
			NULL);

	vbox = gtk_vbox_new(FALSE,6);
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), vbox);

	label = gtk_label_new("Enable/Disable columns");
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, TRUE,0);
	gtk_container_set_border_width(GTK_CONTAINER(dialog), 3);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 9);


	cols = gtk_tree_view_get_columns(GTK_TREE_VIEW(pl3_cp_tree));
	for(iter = cols; iter; iter = g_list_next(iter))
	{
		GtkWidget *but = NULL;
		int colid = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(iter->data), "colid"));
		if(colid != PL_COLUMN_ICON)
		{
			but = gtk_check_button_new_with_label(gtk_tree_view_column_get_title(GTK_TREE_VIEW_COLUMN(iter->data)));		
		}else{
			but  = gtk_check_button_new_with_label(_("Status Icon"));			
		}
		g_object_set_data(G_OBJECT(but), "column", iter->data);
		gtk_box_pack_start(GTK_BOX(vbox), but, FALSE, TRUE,0);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(but), gtk_tree_view_column_get_visible(GTK_TREE_VIEW_COLUMN(iter->data)));
		g_signal_connect(G_OBJECT(but), "toggled", G_CALLBACK(pl3_current_playlist_checkbox_selected), NULL);

	}
	label = gtk_check_button_new_with_label(_("Show Column Headers"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(label),
			gtk_tree_view_get_headers_visible(GTK_TREE_VIEW(pl3_cp_tree)));
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, TRUE,0);
	g_signal_connect(G_OBJECT(label), "toggled", G_CALLBACK(pl3_current_playlist_header_toggle), NULL);
	
	g_list_free(cols);
	gtk_widget_show_all(dialog);
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
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



		/* add the shuffle widget */
		item = gtk_image_menu_item_new_with_label("Columns");
		gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
				gtk_image_new_from_stock(GTK_STOCK_REFRESH, GTK_ICON_SIZE_MENU));
		g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(pl3_current_playlist_enable_columns), NULL);		
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);









		


		gtk_widget_show_all(menu);
		gtk_menu_popup(GTK_MENU(menu), NULL, NULL,NULL, NULL, event->button, event->time);	
		return TRUE;
	}
	return FALSE;
}

void pl3_current_playlist_browser_row_activated(GtkTreeView *tree, GtkTreePath *path, GtkTreeViewColumn *col)
{
	GtkTreeIter iter;
	gint song_id;
	gtk_tree_model_get_iter(gtk_tree_view_get_model(tree), &iter, path);
	gtk_tree_model_get(gtk_tree_view_get_model(tree), &iter, PLAYLIST_LIST_COL_SONG_ID,&song_id, -1);
	mpd_player_play_id(connection, song_id);
}

void pl3_current_playlist_browser_show_info()
{
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
			mpd_Song *song = NULL;
			gtk_tree_model_get_iter (model, &iter, (GtkTreePath *) list->data);
			gtk_tree_model_get (model, &iter, PLAYLIST_LIST_COL_MPDSONG, &song, -1);
			call_id3_window_song (mpd_songDup(song));
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

	gtk_widget_grab_focus(pl3_cp_tree);
	/*
	   if(cfg_get_single_value_as_int_with_default(config, "playlist", "st_cur_song", 0))
	   {
	   pl3_current_playlist_browser_scroll_to_current_song();
	   }
	   */
}
void pl3_current_playlist_browser_unselected()
{
	gtk_container_remove(GTK_CONTAINER(glade_xml_get_widget(pl3_xml, "browser_container")), pl3_cp_vbox);
}


void pl3_current_playlist_browser_playlist_changed()
{

	gchar *string = format_time(playlist_list_get_playtime(PLAYLIST_LIST(playlist)));
	gchar *mesg = g_strdup_printf("%i Items, %s", PLAYLIST_LIST(playlist)->num_rows, string); 
	gtk_statusbar_push(GTK_STATUSBAR(glade_xml_get_widget(pl3_xml, "statusbar2")),0, "");	
	g_free(string);
	g_free(mesg);

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
		treesearch_start(tree_search);
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
					if(mpd_database_save_playlist(connection, str) == MPD_DATABASE_PLAYLIST_EXIST )
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
								mpd_database_delete_playlist(connection, str);
								mpd_database_save_playlist(connection,str);
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
