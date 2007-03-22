/*
 *Copyright (C) 2004-2007 Qball Cow <qball@sarine.nl>
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

#include "eggcolumnchooserdialog.h"

static void pl3_current_playlist_browser_playlist_changed(GtkWidget *tree, GtkTreeIter *iter);
static void pl3_current_playlist_browser_scroll_to_current_song(void);
static void pl3_current_playlist_browser_add(GtkWidget *cat_tree);

static void pl3_current_playlist_browser_selected(GtkWidget *container);
static void pl3_current_playlist_browser_unselected(GtkWidget *container);

static int pl3_current_playlist_browser_cat_menu_popup(GtkWidget *menu, int type, GtkWidget *tree, GdkEventButton *event);


static void pl3_current_playlist_status_changed(MpdObj *mi, ChangedStatusType what, void *userdata);
static int pl3_current_playlist_browser_add_go_menu(GtkWidget *menu);

GtkTreeModel *playlist = NULL;


/* just for here */
static void pl3_current_playlist_browser_row_activated(GtkTreeView *tree, GtkTreePath *path, GtkTreeViewColumn *col);
static int  pl3_current_playlist_browser_button_release_event(GtkTreeView *tree, GdkEventButton *event);
static int  pl3_current_playlist_browser_key_release_event(GtkTreeView *tree, GdkEventKey *event);
static void pl3_current_playlist_browser_show_info(void);
static void pl3_current_playlist_save_playlist(void);
static void pl3_current_playlist_browser_shuffle_playlist(void);
static void pl3_current_playlist_browser_clear_playlist(void);
static int pl3_current_playlist_key_press_event(GtkWidget *mw, GdkEventKey *event, int type);

gmpcPlBrowserPlugin current_playlist_gbp = {
	pl3_current_playlist_browser_add,
	pl3_current_playlist_browser_selected,
	pl3_current_playlist_browser_unselected,
	pl3_current_playlist_browser_playlist_changed,
	NULL,
	pl3_current_playlist_browser_cat_menu_popup,
	NULL,
	pl3_current_playlist_browser_add_go_menu,
	pl3_current_playlist_key_press_event
};

gmpcPlugin current_playlist_plug = {
	"Current Playlist Browser",
	{1,1,1},
	GMPC_PLUGIN_PL_BROWSER,
	0,
	NULL,			                /* path*/
	NULL,			                /* init */
        NULL,                                   /* Destroy */
	&current_playlist_gbp,		        /* Browser */
	pl3_current_playlist_status_changed,	/* status changed */
	NULL, 		                        /* connection changed */
	NULL,		                        /* Preferences */
	NULL,			                /* MetaData */
	NULL,                                   /* get_enabled */
	NULL                                    /* set_enabled */
};


/* external objects */
extern GladeXML *pl3_xml;

/* internal */
GtkWidget *pl3_cp_tree = NULL;
GtkWidget *pl3_cp_sw = NULL;
GtkWidget *pl3_cp_vbox = NULL;
TreeSearch *tree_search = NULL;

static int pl3_current_playlist_browser_button_press_event(GtkTreeView *tree, GdkEventButton *event)
{
	GtkTreeSelection *sel = gtk_tree_view_get_selection(tree);
	if(event->button != 3 || gtk_tree_selection_count_selected_rows(sel) < 2|| !mpd_check_connected(connection))	
	{
		return FALSE;                                                                                           	
	}
	return TRUE;
}

static void pl3_current_playlist_search_activate()
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


	gtk_tree_view_column_set_reorderable(column, TRUE);
	return column;
}

static void pl3_current_playlist_column_changed(GtkTreeView *tree)
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
		q_free(string);
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
			q_free(string);
		}
		g_list_free(cols);
	}
}

static void pl3_current_playlist_browser_init()
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
	gtk_tree_view_column_set_attributes (column,renderer,"icon-name",PLAYLIST_LIST_COL_ICON_ID,NULL);
/*	memset(&value, 0, sizeof(value));*/

	/* set value for ALL */
	g_value_init(&value, G_TYPE_FLOAT);
	g_value_set_float(&value, 0.0);
	g_object_set_property(G_OBJECT(renderer), "yalign", &value); 
	gtk_tree_view_column_set_fixed_width(column, 20);
	gtk_tree_view_column_set_sizing(column,GTK_TREE_VIEW_COLUMN_FIXED);	

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
	/* Composer column */
	column = pl3_current_playlist_add_column(pl3_cp_tree, _("Composer"), PLAYLIST_LIST_COL_SONG_COMPOSER,-1);
	sprintf(smallstring,"%i", PL_COLUMN_COMPOSER);
	if(!cfg_get_single_value_as_int_with_default(config, "current-playlist-column-enable", smallstring, FALSE))
	{
		gtk_tree_view_column_set_visible(column, FALSE);	
	}                                                                                                     	
	g_object_set_data(G_OBJECT(column), "colid", GINT_TO_POINTER(PL_COLUMN_COMPOSER));
	columns[PL_COLUMN_COMPOSER] = column;
	/* Performer Column */
	column = pl3_current_playlist_add_column(pl3_cp_tree, _("Performer"), PLAYLIST_LIST_COL_SONG_PERFORMER,-1);
	sprintf(smallstring,"%i", PL_COLUMN_PERFORMER);
	if(!cfg_get_single_value_as_int_with_default(config, "current-playlist-column-enable", smallstring, FALSE))
	{
		gtk_tree_view_column_set_visible(column, FALSE);	
	}                                                                                                     	
	g_object_set_data(G_OBJECT(column), "colid", GINT_TO_POINTER(PL_COLUMN_PERFORMER));
	columns[PL_COLUMN_PERFORMER] = column;

	/**
	 * Track length column
	 */
	column = pl3_current_playlist_add_column(pl3_cp_tree, _("Length"), PLAYLIST_LIST_COL_SONG_LENGTH_FORMAT,-1);
	sprintf(smallstring,"%i", PL_COLUMN_LENGTH);
	if(!cfg_get_single_value_as_int_with_default(config, "current-playlist-column-enable", smallstring, TRUE))
	{                                                                                                         	
		gtk_tree_view_column_set_visible(column, FALSE);	
	}
	g_object_set_data(G_OBJECT(column), "colid", GINT_TO_POINTER(PL_COLUMN_LENGTH));
	columns[PL_COLUMN_LENGTH] = column;

	/**
	 * Disc column
	 */

	column = pl3_current_playlist_add_column(pl3_cp_tree, _("Disc"), PLAYLIST_LIST_COL_SONG_DISC,-1);
	sprintf(smallstring,"%i", PL_COLUMN_DISC);
	if(!cfg_get_single_value_as_int_with_default(config, "current-playlist-column-enable", smallstring, TRUE))
	{                                                                                                         	
		gtk_tree_view_column_set_visible(column, FALSE);	
	}
	g_object_set_data(G_OBJECT(column), "colid", GINT_TO_POINTER(PL_COLUMN_DISC));
	columns[PL_COLUMN_DISC] = column;

	/**
	 * Comment Column
	 */
	column = pl3_current_playlist_add_column(pl3_cp_tree, _("Comment"), PLAYLIST_LIST_COL_SONG_COMMENT,-1);
	sprintf(smallstring,"%i", PL_COLUMN_COMMENT);
	if(!cfg_get_single_value_as_int_with_default(config, "current-playlist-column-enable", smallstring, TRUE))
	{                                                                                                         	
		gtk_tree_view_column_set_visible(column, FALSE);	
	}
	g_object_set_data(G_OBJECT(column), "colid", GINT_TO_POINTER(PL_COLUMN_COMMENT));
	columns[PL_COLUMN_COMMENT] = column;

	/**
	 * Date Column
	 */
	column = pl3_current_playlist_add_column(pl3_cp_tree, _("Date"), PLAYLIST_LIST_COL_SONG_DATE, -1);
	sprintf(smallstring,"%i", PL_COLUMN_DATE);
	if(!cfg_get_single_value_as_int_with_default(config, "current-playlist-column-enable", smallstring, TRUE))
	{                                                                                                         	
		gtk_tree_view_column_set_visible(column, FALSE);	
	}
	g_object_set_data(G_OBJECT(column), "colid", GINT_TO_POINTER(PL_COLUMN_DATE));
	columns[PL_COLUMN_DATE] = column;
	
	/**
	 * Songpos Column
	 */
	column = pl3_current_playlist_add_column(pl3_cp_tree, _("Pos"), PLAYLIST_LIST_COL_SONG_POS,-1);
	sprintf(smallstring,"%i", PL_COLUMN_SONGPOS);
	if(!cfg_get_single_value_as_int_with_default(config, "current-playlist-column-enable", smallstring, TRUE))
	{                                                                                                         	
		gtk_tree_view_column_set_visible(column, FALSE);	
	}
	g_object_set_data(G_OBJECT(column), "colid", GINT_TO_POINTER(PL_COLUMN_SONGPOS));
	columns[PL_COLUMN_SONGPOS] = column;                                                                            	




	

	
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
	 * Less dirty hack, to stop the last column from growing to far
	 */
	{
		for(position=PL_COLUMN_TOTAL-1; position>=0;position--)
		{
			GtkTreeViewColumn *col = gtk_tree_view_get_column(GTK_TREE_VIEW(pl3_cp_tree), position);
			if(col && gtk_tree_view_column_get_visible(col))
			{
				gtk_tree_view_column_set_fixed_width(col,50);			
				position = -1;
			}
		}
	}

	gtk_tree_view_set_fixed_height_mode(GTK_TREE_VIEW(pl3_cp_tree), TRUE);
	g_signal_connect(G_OBJECT(pl3_cp_tree),"columns-changed", G_CALLBACK(pl3_current_playlist_column_changed), NULL);

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

	gtk_tree_view_set_reorderable(GTK_TREE_VIEW(pl3_cp_tree),TRUE);


	tree_search = (TreeSearch *)treesearch_new(GTK_TREE_VIEW(pl3_cp_tree), PLAYLIST_LIST_COL_MARKUP);
	g_signal_connect(G_OBJECT(tree_search), "result-activate", G_CALLBACK(pl3_current_playlist_search_activate),NULL);


	gtk_box_pack_end(GTK_BOX(pl3_cp_vbox), GTK_WIDGET(tree_search), FALSE, TRUE, 0);	

	/**
	 * Set total time changed signal 
	 */
	g_signal_connect(G_OBJECT(playlist), "total-time-changed", G_CALLBACK(pl3_current_playlist_browser_playlist_changed), NULL);
	/* set initial state */
	g_object_ref(G_OBJECT(pl3_cp_vbox));
}

static void pl3_current_playlist_browser_select_current_song()
{
	if(pl3_cp_tree == NULL) return;
	/* scroll to the playing song */
	if(mpd_player_get_current_song_pos(connection) >= 0 && mpd_playlist_get_playlist_length(connection)  > 0&&
			gtk_tree_view_get_model(GTK_TREE_VIEW(pl3_cp_tree)))
	{
		GtkTreePath *path = gtk_tree_path_new_from_indices(mpd_player_get_current_song_pos(connection),-1);
		if(path != NULL)
		{
/*	gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(pl3_cp_tree),
					path,
					NULL,
					TRUE,0.5,0);
*/
			gtk_tree_view_set_cursor(GTK_TREE_VIEW(pl3_cp_tree), path, NULL, FALSE);
		}
		gtk_tree_path_free(path);
	}      
}

static void pl3_current_playlist_browser_scroll_to_current_song()
{
	if(pl3_cp_tree == NULL) return;
	/* scroll to the playing song */
	if(mpd_player_get_current_song_pos(connection) >= 0 && mpd_playlist_get_playlist_length(connection)  > 0&&
			gtk_tree_view_get_model(GTK_TREE_VIEW(pl3_cp_tree)))
	{
		GtkTreePath *path = gtk_tree_path_new_from_indices(mpd_player_get_current_song_pos(connection),-1);
		if(path != NULL)
		{
			gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(pl3_cp_tree),
					path,
					NULL,
					TRUE,0.5,0);
			gtk_tree_path_free(path);
		}
	
	}      
}

/* add's the toplevel entry for the current playlist view */
static void pl3_current_playlist_browser_add(GtkWidget *cat_tree)
{
	GtkTreeIter iter;
	gtk_tree_store_append(pl3_tree, &iter, NULL);
	gtk_tree_store_set(pl3_tree, &iter, 
			PL3_CAT_TYPE, current_playlist_plug.id,/*PL3_CURRENT_PLAYLIST,*/
			PL3_CAT_TITLE, _("Current Playlist"),
			PL3_CAT_INT_ID, "",
			PL3_CAT_ICON_ID, "media-playlist",
			PL3_CAT_PROC, TRUE,
			PL3_CAT_ICON_SIZE,GTK_ICON_SIZE_DND,
			-1);
}

/* delete all selected songs,
 * if no songs select ask the user if he want's to clear the list 
 */
static void pl3_current_playlist_browser_delete_selected_songs ()
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
					_("Are you sure you want to clear the playlist?"));
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

static void pl3_current_playlist_browser_crop_selected_songs()
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

static void pl3_current_playlist_enable_columns()
{
	GtkWidget *dialog = egg_column_chooser_dialog_new(GTK_TREE_VIEW(pl3_cp_tree));
	gtk_widget_show_all(dialog);
	gtk_dialog_run(GTK_DIALOG(dialog));
}
static int pl3_current_playlist_browser_button_release_event(GtkTreeView *tree, GdkEventButton *event)
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

static void pl3_current_playlist_browser_row_activated(GtkTreeView *tree, GtkTreePath *path, GtkTreeViewColumn *col)
{
	GtkTreeIter iter;
	gint song_id;
	gtk_tree_model_get_iter(gtk_tree_view_get_model(tree), &iter, path);
	gtk_tree_model_get(gtk_tree_view_get_model(tree), &iter, PLAYLIST_LIST_COL_SONG_ID,&song_id, -1);
	mpd_player_play_id(connection, song_id);
}

static void pl3_current_playlist_browser_show_info()
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

static void pl3_current_playlist_browser_selected(GtkWidget *container)
{
	if(pl3_cp_vbox == NULL)
	{
		pl3_current_playlist_browser_init();
	}
	gtk_container_add(GTK_CONTAINER(container), pl3_cp_vbox);
	gtk_widget_show(pl3_cp_vbox);
	pl3_current_playlist_browser_playlist_changed(NULL, NULL);

	gtk_widget_grab_focus(pl3_cp_tree);
}
static void pl3_current_playlist_browser_unselected(GtkWidget *container)
{
	gtk_container_remove(GTK_CONTAINER(container), pl3_cp_vbox);
}


static void pl3_current_playlist_browser_playlist_changed(GtkWidget *tree, GtkTreeIter *iter)
{
	if(pl3_cat_get_selected_browser() == current_playlist_plug.id)
	{
		if(mpd_playlist_get_playlist_length(connection))
		{
			guint playtime = playlist_list_get_playtime(PLAYLIST_LIST(playlist))*
				(1/playlist_list_get_loaded(PLAYLIST_LIST(playlist)));
			gchar *string = format_time(playtime);
			gchar *mesg = NULL;
			gdouble loaded =playlist_list_get_loaded(PLAYLIST_LIST(playlist));
			mesg = g_strdup_printf("%i Items%c %s %s", PLAYLIST_LIST(playlist)->num_rows,(string[0])?',':' ', string,
				(string[0] == 0 || loaded >= 1 || loaded <= 0.0)? "":_("(Estimation)")); 
			pl3_push_rsb_message(mesg);
			q_free(string);
			q_free(mesg);
		} else {
			pl3_push_rsb_message("");
		}
	}
}


static int pl3_current_playlist_browser_cat_menu_popup(GtkWidget *menu, int type, GtkWidget *tree, GdkEventButton *event)
{
	/* here we have:  Save, Clear*/
	GtkWidget *item;
	if(type != current_playlist_plug.id) return 0;
	/* add the save widget */
	item = gtk_image_menu_item_new_from_stock(GTK_STOCK_SAVE,NULL);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(pl3_current_playlist_save_playlist), NULL);

	/* add the clear widget */
	item = gtk_image_menu_item_new_from_stock(GTK_STOCK_CLEAR,NULL);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(pl3_current_playlist_browser_clear_playlist), NULL);


	return 1;
}

static int  pl3_current_playlist_browser_key_release_event(GtkTreeView *tree, GdkEventKey *event)
{
	if(event->keyval == GDK_Delete)
	{
		pl3_current_playlist_browser_delete_selected_songs ();
		return TRUE;                                          		
	}
	else if(event->keyval == GDK_i && event->state&GDK_MOD1_MASK)
	{
		pl3_current_playlist_browser_show_info();
		return TRUE;
	}
	else if (event->keyval == GDK_space)
	{
		pl3_current_playlist_browser_scroll_to_current_song();
		pl3_current_playlist_browser_select_current_song();
		return TRUE;			
	}
	else if (event->keyval == GDK_f && event->state&GDK_CONTROL_MASK)
	{
		treesearch_start(tree_search);
		return TRUE;
	}
	else if((event->state&(GDK_CONTROL_MASK|GDK_MOD1_MASK)) == 0 && 
			((event->keyval >= GDK_space && event->keyval <= GDK_z)))
	{
		char data[2];
		data[0] = (char)gdk_keyval_to_unicode(event->keyval);
		data[1] = '\0';
		treesearch_start(TREESEARCH(tree_search));
		gtk_entry_set_text(GTK_ENTRY(TREESEARCH(tree_search)->entry),data);
		gtk_editable_set_position(GTK_EDITABLE(TREESEARCH(tree_search)->entry),1);
		return TRUE;
	}
	return pl3_window_key_press_event(GTK_WIDGET(tree),event);
}

/* create a dialog that allows the user to save the current playlist */
static void pl3_current_playlist_save_playlist ()
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
	q_free(str);

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
								GmpcStatusChangedCallback(connection, MPD_CST_DATABASE, NULL);
								break;
							default:
								run = TRUE;
						}
						/* return to stare */
						gtk_widget_set_sensitive(GTK_WIDGET(glade_xml_get_widget(xml, "pl-entry")), TRUE);
						gtk_widget_hide(glade_xml_get_widget(xml, "hbox5"));

						q_free(errormsg);
					}
					else 
					{
						GmpcStatusChangedCallback(connection, MPD_CST_DATABASE, NULL);
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

static void pl3_current_playlist_browser_clear_playlist()
{
	mpd_playlist_clear(connection);
}

static void pl3_current_playlist_browser_shuffle_playlist()
{
	mpd_playlist_shuffle(connection);
}

static void pl3_current_playlist_highlight_song_change ()
{
/*	GtkTreeIter iter;
	gchar *temp;
*/	if (!mpd_check_connected (connection))
	{
		return;
	}

	/* check if we need to highlight a song */
	if (mpd_player_get_state(connection) > MPD_PLAYER_STOP && mpd_player_get_current_song_pos(connection) >= 0)
	{
/*
		temp = g_strdup_printf ("%i", mpd_player_get_current_song_pos(connection));
		if (gtk_tree_model_get_iter_from_string (GTK_TREE_MODEL (playlist), &iter, temp))
		{
			gint pos;
			gtk_tree_model_get (GTK_TREE_MODEL (playlist), &iter, PLAYLIST_LIST_COL_SONG_POS,
					&pos, -1);
*/			/* check if we have the right song, if not, print an error */
/*			if (pos != mpd_player_get_current_song_pos(connection))
			{
				debug_printf(DEBUG_ERROR,"Error %i "\
						" %i should be the same\n",
						pos,
						mpd_player_get_current_song_pos(connection));
			}
*/
			if(cfg_get_single_value_as_int_with_default(config, "playlist", "st_cur_song", 0) && pl3_cp_tree) 
			{
				pl3_current_playlist_browser_scroll_to_current_song();
			}
/*		}
		q_free (temp);
*/	}
}


static void pl3_current_playlist_status_changed(MpdObj *mi, ChangedStatusType what, void *userdata)
{
	if(what&MPD_CST_PLAYLIST)
	{
		playlist_list_data_update(PLAYLIST_LIST(playlist),mi,GTK_TREE_VIEW(pl3_cp_tree));
	}
	if(what&(MPD_CST_SONGPOS|MPD_CST_SONGID))
	{
		playlist_list_set_current_song_pos(PLAYLIST_LIST(playlist), mpd_player_get_current_song_pos(mi));
		pl3_current_playlist_highlight_song_change();
	}
	if(what&MPD_CST_STATE)
	{
		if(mpd_player_get_state(mi) == MPD_STATUS_STATE_STOP){
			playlist_list_set_current_song_pos(PLAYLIST_LIST(playlist), -1);
		}
		else if (mpd_player_get_state(mi) == MPD_STATUS_STATE_PLAY) {
			playlist_list_set_current_song_pos(PLAYLIST_LIST(playlist), mpd_player_get_current_song_pos(mi));
			pl3_current_playlist_browser_scroll_to_current_song();
		}
	}
}


static void pl3_current_playlist_browser_activate()
{
	GtkTreeSelection *selec = gtk_tree_view_get_selection((GtkTreeView *)
			glade_xml_get_widget (pl3_xml, "cat_tree"));

	/**
	 * Fix this to be nnot static
	 */	
	GtkTreePath *path = gtk_tree_path_new_from_indices(0,-1);
	if(path)
	{
		gtk_tree_selection_select_path(selec, path);
		gtk_tree_path_free(path);
	}
}


static int pl3_current_playlist_browser_add_go_menu(GtkWidget *menu)
{
	GtkWidget *item = NULL;

	item = gtk_image_menu_item_new_with_label(_("Current Playlist"));
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item), 
			gtk_image_new_from_icon_name("media-playlist", GTK_ICON_SIZE_MENU));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	g_signal_connect(G_OBJECT(item), "activate", 
			G_CALLBACK(pl3_current_playlist_browser_activate), NULL);
	return 1;
}


static int pl3_current_playlist_key_press_event(GtkWidget *mw, GdkEventKey *event, int type)
{
	/** Global keybinding */
	if (event->keyval == GDK_F1)
	{
		pl3_current_playlist_browser_activate();
		return TRUE;
	}

	return FALSE;
}
