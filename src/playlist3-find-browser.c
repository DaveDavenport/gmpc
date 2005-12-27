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

void pl3_find_browser_search();
void pl3_find_browser_row_activated(GtkTreeView *tree, GtkTreePath *tp);
void pl3_find_browser_show_info();
unsigned long pl3_find_browser_view_browser();
int pl3_find_browser_playlist_key_press(GtkWidget *tree, GdkEventKey *event);
void pl3_find_browser_add_selected();
void pl3_find_browser_replace_selected();
void pl3_find_browser_button_release_event(GtkWidget *but, GdkEventButton *event);

extern GladeXML *pl3_xml;
extern GtkListStore *pl2_store;

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
	int i=0, max = 3;
	gtk_list_store_clear(pl3_findb_combo_store);

	if(mpd_server_check_version(connection,0,12,0))
	{
		max = MPD_TAG_NUM_OF_ITEM_TYPES;
	}
	for(i=0;i< max;i++)
	{
		if(i != MPD_TAG_ITEM_COMMENT) /* I Don't want COMMENT */
		{
			gtk_list_store_append(pl3_findb_combo_store, &iter);
			gtk_list_store_set(pl3_findb_combo_store, &iter, 1, mpdTagItemKeys[i], 0,i, -1);	
		}
	}

	gtk_list_store_append(pl3_findb_combo_store, &iter);
	gtk_list_store_set(pl3_findb_combo_store, &iter, 1, "All (slow)", 0,PL3_FINDB_CB_ALL,-1);	
	gtk_list_store_append(pl3_findb_combo_store, &iter);
	gtk_list_store_set(pl3_findb_combo_store, &iter, 1, "Playlist", 0,PL3_FINDB_CB_PLAYLIST, -1);

	gtk_combo_box_set_active(GTK_COMBO_BOX(pl3_findb_combo), 0);
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
			GTK_TYPE_STRING,
			GTK_TYPE_INT); /* icon type */



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
	printf("initialized find playlist treeview\n");

	pl3_findb_vbox = gtk_vbox_new(FALSE, 6);
	hbox = gtk_hbox_new(FALSE,6);


	gtk_box_pack_start(GTK_BOX(pl3_findb_vbox), pl3_findb_sw, TRUE, TRUE,0);
	gtk_box_pack_start(GTK_BOX(pl3_findb_vbox), hbox, FALSE, TRUE,0);
	pl3_findb_entry = gtk_entry_new();

	pl3_findb_combo_store = gtk_list_store_new(2,GTK_TYPE_INT, GTK_TYPE_STRING);
	pl3_findb_combo = gtk_combo_box_new_with_model(GTK_TREE_MODEL(pl3_findb_combo_store));

	renderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(pl3_findb_combo), renderer, TRUE);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(pl3_findb_combo), renderer, "text", 1, NULL);
	
	findbut = gtk_button_new_from_stock(GTK_STOCK_FIND);
	gtk_box_pack_start(GTK_BOX(hbox), pl3_findb_entry, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), pl3_findb_combo, TRUE, TRUE, 0);
	gtk_box_pack_end(GTK_BOX(hbox), findbut, FALSE, TRUE, 0);

	pl3_find_fill_combo();

	g_signal_connect(G_OBJECT(pl3_findb_entry), "activate", pl3_find_browser_search, NULL);
	g_signal_connect(G_OBJECT(findbut), "clicked", pl3_find_browser_search, NULL);
	
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
   GtkTreeIter iter;
   char *markdata = cfg_get_single_value_as_string_with_default(config, "playlist", "browser_markup",DEFAULT_MARKUP_BROWSER);
   int time=0;
   gtk_list_store_clear(pl3_findb_store);
   if(TRUE)
   {
	   gchar *name=NULL;
	   gint num_field=0;
	   GtkTreeIter child;
	   GtkTreeIter cc_iter;

	   MpdData *data = NULL;
	   name = (gchar *)gtk_entry_get_text(GTK_ENTRY(pl3_findb_entry));
	   gtk_combo_box_get_active_iter(GTK_COMBO_BOX(pl3_findb_combo), &cc_iter);

	   gtk_tree_model_get(GTK_TREE_MODEL(pl3_findb_combo_store),&cc_iter , 0, &num_field, -1);
	   if(name == NULL || !strlen(name))
	   {
		   cfg_free_string(markdata);
		   return 0 ;
	   }
	   /* do the actual search */
	   if(num_field < 50)
	   {
		   if(mpd_server_check_version(connection,0,12,0))
		   {
			   data = mpd_playlist_find_adv(connection,FALSE, num_field, name, -1);
		   }
		   else
		   {
			   data = mpd_playlist_find(connection, num_field, name, FALSE);
		   }
	   }
	   else if(num_field == PL3_FINDB_CB_ALL){
		   data = mpd_playlist_token_find(connection, name);
	   }
	   else if (num_field == PL3_FINDB_CB_PLAYLIST)
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
							   PL3_FINDB_PATH, "", 
							   PL3_FINDB_TYPE, PL3_CUR_PLAYLIST,
							   PL3_FINDB_PID, id,
							   PL3_FINDB_TITLE, temp,
							   PL3_FINDB_ICON, icon, 
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
   cfg_free_string(markdata);
   return time;
}


void pl3_find_browser_search()
{

	pl3_find_browser_view_browser();
	return;	
}


void pl3_find_browser_show_info()
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
				data = mpd_playlist_find_adv(connection,TRUE,MPD_TAG_ITEM_FILENAME,path,-1);
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





void pl3_find_browser_row_activated(GtkTreeView *tree, GtkTreePath *tp)
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
			pl3_push_statusbar_message("Added a song");
			mpd_playlist_queue_add(connection, song_id);
			break;
	}
	/* commit queue, if PL3_CUR_PLAYLIST, then queue is empty, and I don't mind */
	mpd_playlist_queue_commit(connection);
	g_free(song_id);
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
int pl3_find_browser_playlist_key_press(GtkWidget *tree, GdkEventKey *event)
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
	else if(event->keyval == GDK_j)
	{
		pl3_find_browser_search_playlist();
	}
	else
	{
		return pl3_window_key_press_event(tree,event);
	}
	return TRUE;
}

void pl3_find_browser_replace_selected()
{
	mpd_playlist_clear(connection);
	pl3_find_browser_add_selected();
	mpd_player_play(connection);	

}


void pl3_find_browser_add_selected()
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


void pl3_find_browser_button_release_event(GtkWidget *but, GdkEventButton *event)
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

void pl3_find_browser_playlist_changed()
{
	if(pl3_findb_tree != NULL)
	{
		pl3_find_browser_view_browser();
	}
}

void pl3_find_browser_search_playlist()
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
