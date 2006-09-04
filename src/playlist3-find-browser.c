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
#include "playlist3-find-browser.h"
#include "config1.h"
#include "id3info.h"

static void pl3_find_browser_category_selection_changed(GtkWidget *, GtkTreeIter *);
static void pl3_find_browser_selected(GtkWidget *);
static void pl3_find_browser_unselected(GtkWidget *);
static void pl3_find_browser_add(GtkWidget *cat_tree);
static int pl3_find_browser_add_go_menu(GtkWidget *);
static void pl3_find_browser_search(void);
static void pl3_find_browser_row_activated(GtkTreeView *, GtkTreePath *);
static int pl3_find_browser_playlist_key_press(GtkWidget *, GdkEventKey *);
static void pl3_find_browser_add_selected(void);
static void pl3_find_browser_button_release_event(GtkWidget *but, GdkEventButton *event);
static void pl3_find_browser_connection_changed(MpdObj *mi, int connect, gpointer data);
static int pl3_find_browser_key_press_event(GtkWidget *mw, GdkEventKey *event, int type);
extern GladeXML *pl3_xml;

enum{
	PL3_FINDB_PATH,
	PL3_FINDB_TYPE,
	PL3_FINDB_TITLE,
	PL3_FINDB_ICON,
	PL3_FINDB_PID,
	PL3_FINDB_ROWS
};


#define PL3_FINDB_CB_ALL 88
#define	PL3_FINDB_CB_PLAYLIST 99
/**
 * Plugin structure
 */
gmpcPlBrowserPlugin find_browser_gbp = {
	pl3_find_browser_add,
	pl3_find_browser_selected,
	pl3_find_browser_unselected,
	pl3_find_browser_category_selection_changed,
	NULL,
	NULL,
	NULL,
	pl3_find_browser_add_go_menu,
	pl3_find_browser_key_press_event
};

gmpcPlugin find_browser_plug = {
	"File Browser",
	{1,1,1},
	GMPC_PLUGIN_PL_BROWSER,
	0,
	NULL,			/* path*/
	NULL,			/* init */
	&find_browser_gbp,		/* Browser */
	NULL,			/* status changed */
	pl3_find_browser_connection_changed, 		/* connection changed */
	NULL,		/* Preferences */
	NULL,			/* MetaData */
	NULL,
	NULL
};

extern GladeXML *pl3_xml;

/* internal */
GtkWidget 	*pl3_findb_tree 	= NULL;
GtkListStore 	*pl3_findb_store 	= NULL;
GtkWidget 	*pl3_findb_vbox 	= NULL;
GtkWidget	*pl3_findb_entry	= NULL;
GtkWidget 	*pl3_findb_combo	= NULL;
GtkListStore	*pl3_findb_combo_store 	= NULL;
GtkWidget	*pl3_findb_pb = NULL;

static int pl3_find_browser_button_press_event(GtkTreeView *tree, GdkEventButton *event)
{
	GtkTreeSelection *sel = gtk_tree_view_get_selection(tree);
	if(event->button != 3 || gtk_tree_selection_count_selected_rows(sel) < 2|| !mpd_check_connected(connection))	
	{
		return FALSE;
	}
	return TRUE;
}

static void pl3_find_fill_combo()
{
	GtkTreeIter iter;
	int i=0, max = 3;
	gtk_list_store_clear(pl3_findb_combo_store);

	if(mpd_server_check_version(connection,0,12,0))
	{
		max = MPD_TAG_NUM_OF_ITEM_TYPES;
	}
	for(i=0;i< max;i++)
	{
		gtk_list_store_append(pl3_findb_combo_store, &iter);
		gtk_list_store_set(pl3_findb_combo_store, &iter, 1, mpdTagItemKeys[i], 0,i, -1);	
	}

	gtk_list_store_append(pl3_findb_combo_store, &iter);
	gtk_list_store_set(pl3_findb_combo_store, &iter, 1, "All (slow)", 0,PL3_FINDB_CB_ALL,-1);	
	gtk_list_store_append(pl3_findb_combo_store, &iter);
	gtk_list_store_set(pl3_findb_combo_store, &iter, 1, "Playlist", 0,PL3_FINDB_CB_PLAYLIST, -1);

	gtk_combo_box_set_active(GTK_COMBO_BOX(pl3_findb_combo), 0);
}


static void pl3_find_browser_init()
{
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column = NULL;
	GtkWidget  *pl3_findb_sw = NULL;
	GtkWidget *hbox = NULL, *findbut = NULL;
	GtkEntryCompletion *compl = NULL;

	GValue value;
	pl3_findb_store = gtk_list_store_new (PL3_FINDB_ROWS, 
			G_TYPE_STRING, /* path to file */
			G_TYPE_INT,	/* type, FILE/PLAYLIST/FOLDER  */
			G_TYPE_STRING,	/* title to display */
			G_TYPE_STRING,
			G_TYPE_INT); /* icon type */



	renderer = gtk_cell_renderer_pixbuf_new ();

	column = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_column_pack_start (column, renderer, FALSE);
	gtk_tree_view_column_set_attributes (column,renderer,"stock-id", PL3_FINDB_ICON,NULL);
	memset(&value, 0, sizeof(value));
	/* set value for ALL */
	g_value_init(&value, G_TYPE_FLOAT);
	g_value_set_float(&value, 0.0);
	g_object_set_property(G_OBJECT(renderer), "yalign", &value); 

	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_set_attributes (column,renderer,"text", PL3_FINDB_TITLE, NULL);


	/* set up the tree */
	pl3_findb_tree= gtk_tree_view_new_with_model(GTK_TREE_MODEL(pl3_findb_store));
	/* insert the column in the tree */
	gtk_tree_view_append_column (GTK_TREE_VIEW (pl3_findb_tree), column);                                         	
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(pl3_findb_tree), FALSE);
	gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(pl3_findb_tree), TRUE);
	gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW(pl3_findb_tree)), GTK_SELECTION_MULTIPLE);

	/* setup signals */
	g_signal_connect(G_OBJECT(pl3_findb_tree), "row-activated",G_CALLBACK(pl3_find_browser_row_activated), NULL); 
	g_signal_connect(G_OBJECT(pl3_findb_tree), "button-press-event", G_CALLBACK(pl3_find_browser_button_press_event), NULL);
	g_signal_connect(G_OBJECT(pl3_findb_tree), "button-release-event", G_CALLBACK(pl3_find_browser_button_release_event), NULL);
	g_signal_connect(G_OBJECT(pl3_findb_tree), "key-press-event", G_CALLBACK(pl3_find_browser_playlist_key_press), NULL);

	/* set up the scrolled window */
	pl3_findb_sw = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(pl3_findb_sw), GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(pl3_findb_sw), GTK_SHADOW_ETCHED_IN);
	gtk_container_add(GTK_CONTAINER(pl3_findb_sw), pl3_findb_tree);

	/* set initial state */

	pl3_findb_vbox = gtk_vbox_new(FALSE, 6);
	hbox = gtk_hbox_new(FALSE,6);


	pl3_findb_pb = gtk_progress_bar_new();
	gtk_box_pack_start(GTK_BOX(pl3_findb_vbox), pl3_findb_sw, TRUE, TRUE,0);
	gtk_box_pack_start(GTK_BOX(pl3_findb_vbox), hbox, FALSE, TRUE,0);


	pl3_findb_entry = gtk_combo_box_entry_new();
	gtk_combo_box_set_model(GTK_COMBO_BOX(pl3_findb_entry), 
			GTK_TREE_MODEL(gtk_list_store_new(1, G_TYPE_STRING)));
	gtk_combo_box_entry_set_text_column(GTK_COMBO_BOX_ENTRY(pl3_findb_entry), 0);
	compl = gtk_entry_completion_new();
	gtk_entry_completion_set_model(GTK_ENTRY_COMPLETION(compl), gtk_combo_box_get_model(GTK_COMBO_BOX(pl3_findb_entry)));
	gtk_entry_completion_set_text_column(GTK_ENTRY_COMPLETION(compl), 0);

	gtk_entry_set_completion(GTK_ENTRY(GTK_BIN(pl3_findb_entry)->child), GTK_ENTRY_COMPLETION(compl));

	pl3_findb_combo_store = gtk_list_store_new(2,G_TYPE_INT, G_TYPE_STRING);
	pl3_findb_combo = gtk_combo_box_new_with_model(GTK_TREE_MODEL(pl3_findb_combo_store));

	renderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(pl3_findb_combo), renderer, TRUE);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(pl3_findb_combo), renderer, "text", 1, NULL);

	findbut = gtk_button_new_from_stock(GTK_STOCK_FIND);

	gtk_box_pack_start(GTK_BOX(hbox), pl3_findb_pb, TRUE, TRUE, 0);

	gtk_box_pack_start(GTK_BOX(hbox), pl3_findb_entry, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), pl3_findb_combo, TRUE, TRUE, 0);
	gtk_box_pack_end(GTK_BOX(hbox), findbut, FALSE, TRUE, 0);

	pl3_find_fill_combo();

	g_signal_connect(G_OBJECT(GTK_BIN(pl3_findb_entry)->child), "activate", pl3_find_browser_search, NULL);
	g_signal_connect(G_OBJECT(findbut), "clicked", pl3_find_browser_search, NULL);

	gtk_widget_show_all(pl3_findb_vbox);
	gtk_widget_hide(pl3_findb_pb);	
	g_object_ref(G_OBJECT(pl3_findb_vbox));
}

static void pl3_find_browser_selected(GtkWidget *container)
{
	if(pl3_findb_tree == NULL)
	{
		pl3_find_browser_init();
	}

	gtk_container_add(GTK_CONTAINER(container),pl3_findb_vbox);
	gtk_widget_grab_focus(pl3_findb_tree);
	gtk_widget_show(pl3_findb_vbox);
}
static void pl3_find_browser_unselected(GtkWidget *container)
{
	gtk_container_remove(GTK_CONTAINER(container), pl3_findb_vbox);
}




/*****************************************************************
 * Find Browser
 */
/* add's the toplevel entry for the current playlist view */
static void pl3_find_browser_add(GtkWidget *cat_tree)
{
	GtkTreeIter iter;
	gtk_tree_store_append(pl3_tree, &iter, NULL);
	gtk_tree_store_set(pl3_tree, &iter, 
			PL3_CAT_TYPE, find_browser_plug.id,
			PL3_CAT_TITLE, _("Search"),
			PL3_CAT_INT_ID, "",
			PL3_CAT_ICON_ID, "gtk-find",
			PL3_CAT_PROC, TRUE,
			PL3_CAT_ICON_SIZE,GTK_ICON_SIZE_DND,-1);
}

static unsigned long pl3_find_browser_view_browser()
{
	GtkTreeIter iter;
	char *markdata = cfg_get_single_value_as_string_with_default(config, "playlist", "browser_markup",DEFAULT_MARKUP_BROWSER);
	int time=0;
	gtk_tree_view_set_model(GTK_TREE_VIEW(pl3_findb_tree), NULL);
	gtk_list_store_clear(pl3_findb_store);
	if(TRUE)
	{
		gchar *name=NULL;
		gint num_field=0;
		GtkTreeIter child;
		GtkTreeIter cc_iter;
		GtkTreeIter combo_iter;

		MpdData *data = NULL;
		name = (gchar *)gtk_combo_box_get_active_text(GTK_COMBO_BOX(pl3_findb_entry));
		gtk_combo_box_get_active_iter(GTK_COMBO_BOX(pl3_findb_combo), &cc_iter);
		if(*name)
		{
			int skip = 0;
			GtkTreeModel *model = gtk_combo_box_get_model(GTK_COMBO_BOX(pl3_findb_entry));
			if(gtk_tree_model_get_iter_first(model, &combo_iter))
			{
				do{
					char *oldname = NULL;
					gtk_tree_model_get(model, &combo_iter, 0,&oldname,-1);
					if(!strcmp(name, oldname))
					{
						skip=1;
					}
				}while(gtk_tree_model_iter_next(model, &combo_iter) && !skip);
			} 
			if(!skip)
			{
				gtk_list_store_insert(GTK_LIST_STORE(model), &combo_iter,0);
				gtk_list_store_set(GTK_LIST_STORE(model),
						&combo_iter,
						0,name,
						-1);
			}
		}

		gtk_tree_model_get(GTK_TREE_MODEL(pl3_findb_combo_store),&cc_iter , 0, &num_field, -1);
		if(name == NULL || !strlen(name))
		{
		}
		/* do the actual search */
		else if(num_field < 50)
		{
			if(mpd_server_check_version(connection,0,12,0))
			{
				data = mpd_database_find_adv(connection,FALSE, num_field, name, -1);
			}
			else
			{
				data = mpd_database_find(connection, num_field, name, FALSE);
			}
		}
		else if(num_field == PL3_FINDB_CB_ALL){
			data = mpd_database_token_find(connection, name);
		}
		else if (num_field == PL3_FINDB_CB_PLAYLIST)
		{
			regex_t **filter_test = NULL;
			int songs = 0;
			int total_songs = mpd_playlist_get_playlist_length(connection);
			int step = total_songs/50;
			filter_test = mpd_misc_tokenize(name);
			/*
			   if(filter_test == NULL)
			   {
			   }
			   */
			if(playlist_list_get_loaded(PLAYLIST_LIST(playlist)) < 1)
			{
				gtk_widget_hide(pl3_findb_entry);
				gtk_widget_show(pl3_findb_pb);
				gtk_widget_set_sensitive(glade_xml_get_widget(pl3_xml, "pl3_win"),FALSE);
			}
			else{
				total_songs = 0;
			}

			if(gtk_tree_model_get_iter_first(GTK_TREE_MODEL(playlist), &iter) && filter_test != NULL)
			{
				do
				{
					int loop = TRUE;
					int i = 0;
					mpd_Song *song = NULL;
					gtk_tree_model_get(GTK_TREE_MODEL(playlist), &iter, PLAYLIST_LIST_COL_MPDSONG, &song, -1);

					songs++;
					if(step && total_songs && (songs % step) == 0)
					{
						gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(pl3_findb_pb), songs/(gdouble)total_songs);
						while(gtk_events_pending()) gtk_main_iteration();
					}

					if(filter_test != NULL &&song)
					{
						loop = FALSE;
						for(i=0;filter_test[i]!= NULL && !loop;i++)
						{
							
							loop = TRUE;
							/**
							 * TODO check using a look, not with hardcoded fields
							 */
							/* Artist */
							if(song->artist && regexec(filter_test[i], song->artist,0, NULL, 0) == 0) {
								loop =  FALSE;
							}
							/* Title */
							if(song->title && regexec(filter_test[i], song->title,0, NULL, 0) == 0) {
								loop =  FALSE;
							}
							/* File */
							if(song->file && regexec(filter_test[i], song->file,0, NULL, 0) == 0) {
								loop =  FALSE;
							}
							/* Album */
							if(song->album && regexec(filter_test[i], song->album,0, NULL, 0) == 0) {
								loop =  FALSE;
							}
							if(song->genre && regexec(filter_test[i], song->genre,0, NULL, 0) == 0) {
								loop =  FALSE;
							}
							if(song->track && regexec(filter_test[i], song->track,0, NULL, 0) == 0) {
								loop =  FALSE;
							}                                                                 
							if(song->name && regexec(filter_test[i], song->name,0, NULL, 0) == 0) {
								loop =  FALSE;
							}                                                                 
							if(song->composer && regexec(filter_test[i], song->composer,0, NULL, 0) == 0) {
								loop =  FALSE;
							}                                        
							if(song->date && regexec(filter_test[i], song->date,0, NULL, 0) == 0) {
								loop =  FALSE;
							}                     
							if(song->disc && regexec(filter_test[i], song->disc,0, NULL, 0) == 0) {
								loop =  FALSE;
							}
							if(song->comment && regexec(filter_test[i], song->comment,0, NULL, 0) == 0) {
								loop =  FALSE;
							}
						}	
					}
					if(!loop)
					{
						GtkTreeIter piter;
						int id = 0, ttime = 0;;
						char *temp = NULL;
						gtk_tree_model_get(GTK_TREE_MODEL(playlist), &iter,
								PLAYLIST_LIST_COL_SONG_ID, &id,
								PLAYLIST_LIST_COL_MARKUP, &temp,
								-1);
						gtk_list_store_append(pl3_findb_store, &piter);
						gtk_list_store_set(pl3_findb_store, &piter,
								PL3_FINDB_PATH, "", 
								PL3_FINDB_TYPE, PL3_CUR_PLAYLIST,
								PL3_FINDB_PID, id,
								PL3_FINDB_TITLE, temp,
								PL3_FINDB_ICON, "media-audiofile",
								-1); 	
						g_free(temp);                      		
						time+=ttime;
					}

				}
				while(gtk_tree_model_iter_next(GTK_TREE_MODEL(playlist), &iter));
			}
			mpd_misc_tokens_free(filter_test);
			if(total_songs)
			{
				gtk_widget_show(pl3_findb_entry);
				gtk_widget_hide(pl3_findb_pb);   		   
				gtk_widget_set_sensitive(glade_xml_get_widget(pl3_xml, "pl3_win"),TRUE);
				gtk_widget_grab_focus(pl3_findb_entry);
			}
		}

		while (data != NULL)
		{
			gchar buffer[1024];
			if(data->type == MPD_DATA_TYPE_SONG)
			{ 
				if(data->song->time != MPD_SONG_NO_TIME)
				{
					time += data->song->time;
				}

				mpd_song_markup(buffer, 1024, markdata,
						data->song);

				/* add as child of the above created parent folder */
				gtk_list_store_append (pl3_findb_store, &child);
				gtk_list_store_set (pl3_findb_store, &child,
						PL3_FINDB_PATH, data->song->file,
						PL3_FINDB_TITLE, buffer,
						PL3_FINDB_TYPE, PL3_ENTRY_SONG, 
						PL3_FINDB_ICON, "media-audiofile", 
						-1);
			}
			else if (data->type == MPD_DATA_TYPE_TAG && data->tag_type == MPD_TAG_ITEM_ARTIST)
			{
				gtk_list_store_prepend (pl3_findb_store, &child);
				gtk_list_store_set (pl3_findb_store, &child,
						PL3_FINDB_PATH, data->tag,
						PL3_FINDB_TITLE, data->tag,
						PL3_FINDB_TYPE, PL3_ENTRY_ARTIST, 
						PL3_FINDB_ICON, "media-artist", 			  
						-1);
			}
			else if (data->type == MPD_DATA_TYPE_TAG && data->tag_type == MPD_TAG_ITEM_ALBUM)
			{
				char *buffer = NULL;
				if(data->tag)
				{
					buffer = g_strdup_printf("%s - %s", data->tag, data->tag);
				}
				else
				{
					buffer = g_strdup(data->tag);
				}
				gtk_list_store_prepend (pl3_findb_store, &child);
				gtk_list_store_set (pl3_findb_store, &child,
						PL3_FINDB_PATH, data->tag,
						PL3_FINDB_TITLE, buffer,
						PL3_FINDB_TYPE, PL3_ENTRY_ALBUM,
						PL3_FINDB_ICON, "media-album",
						-1);
				g_free(buffer);
			}

			data =  mpd_data_get_next(data);
		}

	}
	gtk_tree_view_set_model(GTK_TREE_VIEW(pl3_findb_tree), GTK_TREE_MODEL(pl3_findb_store));
	cfg_free_string(markdata);
	return time;
}


static void pl3_find_browser_search()
{

	pl3_find_browser_view_browser();
	return;	
}


static void pl3_find_browser_show_info()
{
	GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW(pl3_findb_tree));
	GtkTreeSelection *selection =gtk_tree_view_get_selection (GTK_TREE_VIEW(pl3_findb_tree));
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

			GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(pl3_findb_tree));
			int type,id;
			MpdData *data;
			gtk_tree_model_get_iter (model, &iter, (GtkTreePath *) list->data);
			gtk_tree_model_get (model, &iter, PL3_FINDB_TYPE, &type,-1);
			if(type == PL3_CUR_PLAYLIST)
			{
				gtk_tree_model_get(model,&iter,PL3_FINDB_PID, &id,-1);
				call_id3_window (id);
			}
			else
			{
				char *path;
				gtk_tree_model_get(model,&iter,PL3_FINDB_PATH, &path,-1);
				data = mpd_database_find_adv(connection,TRUE,MPD_TAG_ITEM_FILENAME,path,-1);
				while(data != NULL)
				{
					call_id3_window_song(mpd_songDup(data->song));
					data = mpd_data_get_next(data);
				}
				g_free(path);
			}
		}
		while ((list = g_list_previous (list)) && mpd_check_connected(connection));
		/* free list */
		g_list_foreach (list, (GFunc) gtk_tree_path_free, NULL);
		g_list_free (list);
	}
}

static void pl3_find_browser_row_activated(GtkTreeView *tree, GtkTreePath *tp)
{
	GtkTreeIter iter;
	gchar *song_id;
	gint r_type;
	int id=-1;
	gtk_tree_model_get_iter(gtk_tree_view_get_model(tree), &iter, tp);
	gtk_tree_model_get(gtk_tree_view_get_model(tree), &iter, PL3_FINDB_PATH,&song_id, PL3_FINDB_TYPE, &r_type, -1);
	switch(r_type)
	{
		case PL3_CUR_PLAYLIST:

			gtk_tree_model_get(gtk_tree_view_get_model(tree), &iter, PL3_FINDB_PID,&id, -1);
			mpd_player_play_id(connection, id);
			break;
		default:
			{
				int playlist_length = mpd_playlist_get_playlist_length(connection);
				pl3_push_statusbar_message("Added a song");
				mpd_playlist_add(connection, song_id);
				/* if there was no song in the playlist, play it */
				if(playlist_length == 0)
				{
					mpd_player_play(connection);
				}
			}
			break;
	}

	g_free(song_id);
}

static void pl3_find_browser_category_selection_changed(GtkWidget *tree, GtkTreeIter *iter)
{
	long unsigned time = 0;
	gchar *string;	
	gtk_list_store_clear(pl3_findb_store);
	time = pl3_find_browser_view_browser();
	string = format_time(time);
	gtk_statusbar_push(GTK_STATUSBAR(glade_xml_get_widget(pl3_xml, "statusbar2")),0, string);
	g_free(string);
}

static void pl3_find_browser_replace_selected()
{
	mpd_playlist_clear(connection);
	pl3_find_browser_add_selected();
	mpd_player_play(connection);	

}

static int pl3_find_browser_playlist_key_press(GtkWidget *tree, GdkEventKey *event)
{
	if(event->state == GDK_CONTROL_MASK && event->keyval == GDK_Insert)
	{
		pl3_find_browser_replace_selected();		
	}
	else if(event->keyval == GDK_Insert)
	{
		pl3_find_browser_add_selected();		
	}
	else if(event->keyval == GDK_i)
	{
		pl3_find_browser_show_info();
	}
	else if(event->keyval == GDK_f)
	{
		gtk_widget_grab_focus(pl3_findb_entry);
	}
	else
	{
		return pl3_window_key_press_event(tree,event);
	}
	return TRUE;
}



static void pl3_find_browser_add_selected()
{
	GtkTreeIter iter;
	GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW(pl3_findb_tree));
	GtkTreeModel *model = GTK_TREE_MODEL (pl3_findb_store);
	GList *rows = gtk_tree_selection_get_selected_rows (selection, &model);
	int songs=0;
	if(rows != NULL)
	{
		gchar *name;
		gint type;
		GList *node = g_list_first(rows);
		do
		{
			GtkTreePath *path = node->data;
			gtk_tree_model_get_iter (model, &iter, path);
			gtk_tree_model_get (model, &iter, PL3_FINDB_PATH,&name, PL3_FINDB_TYPE, &type, -1);	  
			/* does this bitmask thingy works ok? I think it hsould */
			if(type&(PL3_ENTRY_SONG))
			{
				/* add them to the add list */
				mpd_playlist_queue_add(connection, name);
			}
			songs++;
			g_free(name);
		}while((node = g_list_next(node)) != NULL);
	}
	/* if there are items in the add list add them to the playlist */
	mpd_playlist_queue_commit(connection);
	if(songs != 0)
	{
		gchar * message = g_strdup_printf("Added %i song%s", songs, (songs != 1)? "s":"");
		pl3_push_statusbar_message(message);
		g_free(message);
	}

	g_list_foreach (rows, (GFunc) gtk_tree_path_free, NULL);
	g_list_free (rows);
}


static void pl3_find_browser_button_release_event(GtkWidget *but, GdkEventButton *event)
{
	if(event->button != 3) return;
	else if(gtk_tree_selection_count_selected_rows(gtk_tree_view_get_selection(GTK_TREE_VIEW(pl3_findb_tree))) > 0)
	{
		GtkTreeIter iter;
		int type = 0;
		gtk_combo_box_get_active_iter(GTK_COMBO_BOX(pl3_findb_combo), &iter);
		gtk_tree_model_get(GTK_TREE_MODEL(pl3_findb_combo_store),&iter, 0,&type, -1);
		if(type == PL3_FINDB_CB_PLAYLIST && mpd_server_check_version(connection, 0,12,0))
		{
			GtkWidget *item;
			GtkWidget *menu = gtk_menu_new();
			item = gtk_image_menu_item_new_from_stock(GTK_STOCK_DIALOG_INFO,NULL);
			gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
			g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(pl3_find_browser_show_info), NULL);
			gtk_widget_show_all(menu);
			gtk_menu_popup(GTK_MENU(menu), NULL, NULL,NULL, NULL, event->button, event->time);
		}
		else
		{
			GtkWidget *item;
			GtkWidget *menu = gtk_menu_new();
			item = gtk_image_menu_item_new_from_stock(GTK_STOCK_ADD,NULL);
			gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
			g_signal_connect(G_OBJECT(item), "activate",
					G_CALLBACK(pl3_find_browser_add_selected), NULL);
			/* add the replace widget */
			item = gtk_image_menu_item_new_with_label("Replace");
			gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
					gtk_image_new_from_stock(GTK_STOCK_REDO, GTK_ICON_SIZE_MENU));
			gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
			g_signal_connect(G_OBJECT(item), "activate",
					G_CALLBACK(pl3_find_browser_replace_selected), NULL);

			if(mpd_server_check_version(connection,0,12,0))
			{
				item = gtk_image_menu_item_new_from_stock(GTK_STOCK_DIALOG_INFO,NULL);
				gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
				g_signal_connect(G_OBJECT(item), "activate",
						G_CALLBACK(pl3_find_browser_show_info), NULL);
			}
			gtk_widget_show_all(menu);
			gtk_menu_popup(GTK_MENU(menu), NULL, NULL,NULL, NULL, event->button, event->time);
		}
	}
	return;
}

static void pl3_find_browser_search_playlist(void)
{
	if(pl3_findb_tree)
	{
		GtkTreeIter iter;
		gtk_tree_model_get_iter_first(GTK_TREE_MODEL(pl3_findb_combo_store), &iter);
		do{
			int id=0;
			gtk_tree_model_get(GTK_TREE_MODEL(pl3_findb_combo_store), &iter, 0, &id, -1);
			if(id == PL3_FINDB_CB_PLAYLIST)
			{
				gtk_combo_box_set_active_iter(GTK_COMBO_BOX(pl3_findb_combo), &iter);
			}
		}while(gtk_tree_model_iter_next(GTK_TREE_MODEL(pl3_findb_combo_store), &iter));

		gtk_widget_grab_focus(pl3_findb_entry);
	}
}

static void pl3_find_browser_disconnect()
{
	if(pl3_findb_store) gtk_list_store_clear(pl3_findb_store);
}


static void pl3_find_browser_activate()
 {
 	GtkTreeSelection *selec = gtk_tree_view_get_selection((GtkTreeView *)
 			glade_xml_get_widget (pl3_xml, "cat_tree"));
 
 	/**
 	 * Fix this to be nnot static
 	 */	
 	GtkTreePath *path = gtk_tree_path_new_from_string("3"); 
 	if(path)
 	{
 		gtk_tree_selection_select_path(selec, path);
 	}
 }
 
/**
 * Trigger Playlist search
 * This switches to the search window set focus on entry and set searh on playlist.
 * TODO: Move to search plugin?
 */
static void pl3_playlist_search()
{
	if(!mpd_check_connected(connection))
	{
		return;
	}

	if(pl3_xml)
	{
		gtk_window_present(GTK_WINDOW(glade_xml_get_widget(pl3_xml, "pl3_win")));
		GtkTreePath *path = gtk_tree_path_new_from_string("3");
		GtkTreeSelection *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(glade_xml_get_widget(pl3_xml, "cat_tree")));
		gtk_tree_selection_select_path(sel, path);
		gtk_tree_view_set_cursor(GTK_TREE_VIEW(glade_xml_get_widget(pl3_xml, "cat_tree")), path, NULL, FALSE);
		gtk_tree_path_free(path);
		pl3_find_browser_search_playlist();
	}
}

static int pl3_find_browser_add_go_menu(GtkWidget *menu)
{
 	GtkWidget *item = NULL;
 
 	item = gtk_image_menu_item_new_with_label(_("Search"));
 	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item), 
 			gtk_image_new_from_stock("gtk-find", GTK_ICON_SIZE_MENU));
 	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
 	g_signal_connect(G_OBJECT(item), "activate", 
 			G_CALLBACK(pl3_find_browser_activate), NULL);
	/**
	 * Find menu
	 */
 
 	item = gtk_image_menu_item_new_with_label(_("Search in playlist"));
 	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item), 
 			gtk_image_new_from_stock("gtk-find", GTK_ICON_SIZE_MENU));
 	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
 	g_signal_connect(G_OBJECT(item), "activate", 
 			G_CALLBACK(pl3_playlist_search), NULL);

 	return 1;
 }

static void pl3_find_browser_connection_changed(MpdObj *mi, int connect, gpointer data)
{
	if(!connect)
	{
		pl3_find_browser_disconnect();
	}
}
static int pl3_find_browser_key_press_event(GtkWidget *mw, GdkEventKey *event, int type)
{
	if (event->keyval == GDK_F4)
	{
		pl3_find_browser_activate();
		return TRUE;
	}                                           	
	else if(event->state&GDK_CONTROL_MASK && event->keyval == GDK_j)
	{
			pl3_playlist_search();
			return TRUE;
	}

	return FALSE;
}
