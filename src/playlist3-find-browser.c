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

#include "main.h"
#include "strfsong.h"
#include "misc.h"
#include "playlist3.h"
#include "playlist3-current-playlist-browser.h"
#include "open-location.h"
#include "vfs_download.h"
#include "config1.h"

void pl3_find_browser_show_info(GtkTreeView *tree,GtkTreeIter *iter);









extern config_obj *config;
extern GladeXML *pl3_xml;
extern GtkListStore *pl2_store;

enum{
	PL3_FINDB_PATH,
	PL3_FINDB_TYPE,
	PL3_FINDB_TITLE,
	PL3_FINDB_ICON,
	PL3_FINDB_ROWS
};

enum {
	PL3_FINDB_CB_PLAYLIST


};

extern config_obj *config;
extern GladeXML *pl3_xml;

/* internal */
GtkWidget 	*pl3_findb_tree 	= NULL;
GtkListStore 	*pl3_findb_store 	= NULL;
GtkWidget 	*pl3_findb_vbox 	= NULL;
GtkWidget	*pl3_findb_entry	= NULL;
GtkWidget 	*pl3_findb_combo	= NULL;
GtkListStore	*pl3_findb_combo_store 	= NULL;

int pl3_find_browser_button_press_event(GtkTreeView *tree, GdkEventButton *event)
{
	GtkTreeSelection *sel = gtk_tree_view_get_selection(tree);
	if(event->button != 3 || gtk_tree_selection_count_selected_rows(sel) < 2|| !mpd_check_connected(connection))	
	{
		return FALSE;                                                                                           	
	}
	return TRUE;
}

void pl3_find_fill_combo()
{
	GtkTreeIter iter;
	gtk_list_store_clear(pl3_findb_combo_store);


	gtk_list_store_append(pl3_findb_combo_store, &iter);
	gtk_list_store_set(pl3_findb_combo_store, &iter, 1, "Playlist", 0,PL3_FINDB_CB_PLAYLIST, -1);	
	gtk_list_store_append(pl3_findb_combo_store, &iter);
	gtk_list_store_set(pl3_findb_combo_store, &iter, 1, "Playlist", 0,PL3_FINDB_CB_PLAYLIST, -1);
}







void pl3_find_browser_init()
{
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column = NULL;
	GtkWidget  *pl3_findb_sw = NULL;
	GtkWidget *hbox = NULL, *findbut = NULL;
	
	GValue value;
	pl3_findb_store = gtk_list_store_new (PL3_FINDB_ROWS, 
			GTK_TYPE_STRING, /* path to file */
			GTK_TYPE_INT,	/* type, FILE/PLAYLIST/FOLDER  */
			GTK_TYPE_STRING,	/* title to display */
			GTK_TYPE_STRING); /* icon type */



	renderer = gtk_cell_renderer_pixbuf_new ();

	column = gtk_tree_view_column_new ();
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
//	g_signal_connect(G_OBJECT(pl3_findb_tree), "row-activated",G_CALLBACK(pl3_find_browser_row_activated), NULL); 
	g_signal_connect(G_OBJECT(pl3_findb_tree), "button-press-event", G_CALLBACK(pl3_find_browser_button_press_event), NULL);
//	g_signal_connect(G_OBJECT(pl3_findb_tree), "button-release-event", G_CALLBACK(pl3_find_browser_button_release_event), NULL);
//	g_signal_connect(G_OBJECT(pl3_findb_tree), "key-press-event", G_CALLBACK(pl3_find_browser_playlist_key_press), NULL);

	/* set up the scrolled window */
	pl3_findb_sw = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(pl3_findb_sw), GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(pl3_findb_sw), GTK_SHADOW_ETCHED_IN);
	gtk_container_add(GTK_CONTAINER(pl3_findb_sw), pl3_findb_tree);

	/* set initial state */
	printf("initialized find playlist treeview\n");

	pl3_findb_vbox = gtk_vbox_new(FALSE, 6);
	hbox = gtk_hbox_new(FALSE,6);


	gtk_box_pack_start(GTK_BOX(pl3_findb_vbox), pl3_findb_sw, TRUE, TRUE,0);
	gtk_box_pack_start(GTK_BOX(pl3_findb_vbox), hbox, FALSE, TRUE,0);
	pl3_findb_entry = gtk_entry_new();

	pl3_findb_combo_store = gtk_list_store_new(3,GTK_TYPE_INT, GTK_TYPE_STRING, GTK_TYPE_STRING);
	pl3_findb_combo = gtk_combo_box_new_with_model(GTK_TREE_MODEL(pl3_findb_combo_store));

	renderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(pl3_findb_combo), renderer, TRUE);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(pl3_findb_combo), renderer, "text", 1, NULL);
	
	findbut = gtk_button_new_from_stock(GTK_STOCK_FIND);
	gtk_box_pack_start(GTK_BOX(hbox), pl3_findb_entry, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), pl3_findb_combo, TRUE, TRUE, 0);
	gtk_box_pack_end(GTK_BOX(hbox), findbut, FALSE, TRUE, 0);

	pl3_find_fill_combo();

	g_object_ref(G_OBJECT(pl3_findb_vbox));
}







void pl3_find_browser_selected(GtkWidget *container)
{
	if(pl3_findb_tree == NULL)
	{
		pl3_find_browser_init();
	}

	gtk_container_add(GTK_CONTAINER(container),pl3_findb_vbox);
	gtk_widget_show_all(pl3_findb_vbox);
}
void pl3_find_browser_unselected(GtkWidget *container)
{
	gtk_container_remove(GTK_CONTAINER(container), pl3_findb_vbox);
}




/*****************************************************************
 * Find Browser
 */
/* add's the toplevel entry for the current playlist view */
void pl3_find_browser_add()
{
   GtkTreeIter iter;
   gtk_tree_store_append(pl3_tree, &iter, NULL);
   gtk_tree_store_set(pl3_tree, &iter, 
	 PL3_CAT_TYPE, PL3_FIND,
	 PL3_CAT_TITLE, "Search",
	 PL3_CAT_INT_ID, "",
	 PL3_CAT_ICON_ID, "gtk-find",
	 PL3_CAT_PROC, TRUE,
	 PL3_CAT_ICON_SIZE,GTK_ICON_SIZE_DND,-1);
}

unsigned long pl3_find_browser_view_browser()
{
   GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW(glade_xml_get_widget (pl3_xml, "cat_tree")));
   GtkTreeModel *model = GTK_TREE_MODEL(pl3_tree);
   GtkTreeIter iter;
   char *markdata = cfg_get_single_value_as_string_with_default(config, "playlist", "browser_markup",DEFAULT_MARKUP_BROWSER);
   int time=0;
   gtk_list_store_clear(pl3_findb_store);
   if(gtk_tree_selection_get_selected(selection,&model, &iter))
   {
	   gchar *name=NULL;
	   gint num_field=0;
	   GtkTreeIter child;

	   MpdData *data = NULL;
	   name = (gchar *)gtk_entry_get_text(pl3_findb_entry);
	   num_field = gtk_combo_box_get_active(GTK_COMBO_BOX(pl3_findb_combo));
	   if(name == NULL || !strlen(name)) return 0 ;
	   /* do the actual search */
	   if(num_field < 4)
	   {
		   data = mpd_playlist_find(connection, num_field, name, FALSE);
	   }
	   else if(num_field == 4){
		   data = mpd_playlist_token_find(connection, name);
	   }
	   else if (num_field == 5)
	   {
		   regex_t **filter_test = NULL;
		   filter_test = mpd_misc_tokenize(name);
		   if(filter_test == NULL)
		   {
			   printf("crap: %s\n",name);
		   }

		   if(gtk_tree_model_get_iter_first(GTK_TREE_MODEL(pl2_store), &iter) && filter_test != NULL)
			   do
			   {
				   gchar *temp = NULL;
				   int loop = FALSE;
				   int i = 0;
				   gtk_tree_model_get(GTK_TREE_MODEL(pl2_store), &iter, 2, &temp, -1);

				   if(filter_test != NULL)
				   {
					   loop = TRUE;
					   for(i=0;filter_test[i]!= NULL && loop;i++)
					   {
						   if(regexec(filter_test[i], temp,0, NULL, 0) == REG_NOMATCH)
						   {
							   loop =  FALSE;
						   }

					   }	
				   }
				   if(loop)
				   {
					   GtkTreeIter piter;
					   int id = 0, ttime = 0;;
					   char *icon;
					   gtk_tree_model_get(GTK_TREE_MODEL(pl2_store), &iter, SONG_ID, &id, SONG_STOCK_ID, &icon, -1);
					   gtk_list_store_append(pl3_findb_store, &piter);
					   gtk_list_store_set(pl3_findb_store, &piter,
							   PL3_SONG_ID, "", 
							   PL3_SONG_POS, PL3_CUR_PLAYLIST,
							   PL3_UNKOWN, id,
							   PL3_SONG_TITLE, temp,
							   PL3_SONG_STOCK_ID, icon, 
							   -1); 	
					   g_free(icon);
					   time+=ttime;
				   }
				   g_free(temp);                      		
			   }
			   while(gtk_tree_model_iter_next(GTK_TREE_MODEL(pl2_store), &iter));
		   mpd_misc_tokens_free(filter_test);

	   }
	   
	   while (data != NULL)
	   {
		   gchar buffer[1024];
		  if(data->type == MPD_DATA_TYPE_SONG)
		  { 
			  if(data->value.song->time != MPD_SONG_NO_TIME)
			  {
				  time += data->value.song->time;
			  }

			  strfsong (buffer, 1024, markdata,
					  data->value.song);

			  /* add as child of the above created parent folder */
			  gtk_list_store_append (pl3_findb_store, &child);
			  gtk_list_store_set (pl3_findb_store, &child,
					  PL3_SONG_ID, data->value.song->file,
					  PL3_SONG_TITLE, buffer,
					  PL3_SONG_POS, PL3_ENTRY_SONG, 
					  PL3_SONG_STOCK_ID, "media-audiofile", 
					  -1);
		  }
		  else if (data->type == MPD_DATA_TYPE_ARTIST)
		  {
			  gtk_list_store_prepend (pl3_findb_store, &child);
			  gtk_list_store_set (pl3_findb_store, &child,
					  PL3_SONG_ID, data->value.artist,
					  PL3_SONG_TITLE, data->value.artist,
					  PL3_SONG_POS, PL3_ENTRY_ARTIST, 
					  PL3_SONG_STOCK_ID, "media-artist", 			  
					  -1);
		  }
		  else if (data->type == MPD_DATA_TYPE_ALBUM)
		  {
			  char *buffer = NULL;
			  if(data->value.artist)
			  {
				buffer = g_strdup_printf("%s - %s", data->value.artist, data->value.album);
			  }
			  else
			  {
				buffer = g_strdup(data->value.album);
			  }
			  gtk_list_store_prepend (pl3_findb_store, &child);                             		  
			  gtk_list_store_set (pl3_findb_store, &child,                                  		  
					  PL3_FINDB_PATH, data->value.album,
					  PL3_FINDB_TITLE, buffer,
					  PL3_FINDB_TYPE, PL3_ENTRY_ALBUM, 
					  PL3_FINDB_ICON, "media-album", 			  
					  -1);
			  g_free(buffer);
		  }
                                                                                                  		  
		  data =  mpd_data_get_next(data);
	   }

   }
   cfg_free_string(markdata);
   return time;
}


void pl3_find_browser_entry_change(GtkEntry *entry)
{
	gtk_widget_set_sensitive(glade_xml_get_widget(pl3_xml, "find_button"), (strlen(gtk_entry_get_text(pl3_findb_entry)) > 0)?1:0);

}

void pl3_find_browser_search()
{

	pl3_find_browser_view_browser();
	return;	
}


void pl3_find_browser_show_info(GtkTreeView *tree, GtkTreeIter *iter)
{
	char *path;
	GtkTreeModel *model = gtk_tree_view_get_model(tree);
	int type,id;
	MpdData *data;
	gtk_tree_model_get (model, iter, SONG_POS, &type,-1);
	if(type == PL3_CUR_PLAYLIST)
	{
		gtk_tree_model_get(model,iter,PL3_UNKOWN, &id,-1);
		call_id3_window (id);
	}
	else
	{
		gtk_tree_model_get(model,iter,PL3_SONG_ID, &path,-1);
		data = mpd_playlist_find_adv(connection,TRUE,MPD_TAG_ITEM_FILENAME,path,-1);
		while(data != NULL)                                                            	
		{
			call_id3_window_song(mpd_songDup(data->value.song));
			data = mpd_data_get_next(data);                                        
		}
	}
}

void pl3_find_browser_row_activated(GtkTreeView *tree, GtkTreePath *tp)
{
	GtkTreeIter iter;
	gchar *song_id;
	gint r_type;
	gtk_tree_model_get_iter(gtk_tree_view_get_model(tree), &iter, tp);
	gtk_tree_model_get(gtk_tree_view_get_model(tree), &iter, PL3_SONG_ID,&song_id, PL3_SONG_POS, &r_type, -1);
	if(song_id == NULL) return;
	if (r_type == PL3_CUR_PLAYLIST)
	{
		int id=-1;
		gtk_tree_model_get(gtk_tree_view_get_model(tree), &iter, PL3_UNKOWN,&id, -1);
		mpd_player_play_id(connection, id);
	}
	else
	{
		pl3_push_statusbar_message("Added a song");
		mpd_playlist_queue_add(connection, song_id);
	}
	mpd_playlist_queue_commit(connection);
}

void pl3_find_browser_category_selection_changed(GtkTreeView *tree, GtkTreeIter *iter)
{
	long unsigned time = 0;
	gchar *string;	
	gtk_list_store_clear(pl3_findb_store);
	time = pl3_find_browser_view_browser();
	string = format_time(time);
	gtk_statusbar_push(GTK_STATUSBAR(glade_xml_get_widget(pl3_xml, "statusbar2")),0, string);
	g_free(string);
}

