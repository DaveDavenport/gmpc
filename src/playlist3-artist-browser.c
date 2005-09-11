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


extern config_obj *config;
extern GladeXML *pl3_xml;
extern GtkListStore *pl2_store;


/****************************************************************************************
 *  ARTIST BROWSER									*
 *  When mpd 0.12 is stable this function is deprecated, but needed for pre-0.12 mpd's  *
 ****************************************************************************************/

void pl3_artist_browser_add()
{
   GtkTreeIter iter,child;
   gtk_tree_store_append(pl3_tree, &iter, NULL);
   gtk_tree_store_set(pl3_tree, &iter, 
	 PL3_CAT_TYPE, PL3_BROWSE_ARTIST,
	 PL3_CAT_TITLE, "Browse Artists",        	
	 PL3_CAT_INT_ID, "",
	 PL3_CAT_ICON_ID, "media-artist",
	 PL3_CAT_PROC, FALSE,
	 PL3_CAT_ICON_SIZE,GTK_ICON_SIZE_DND,-1);
   /* add fantom child for lazy tree */
   gtk_tree_store_append(pl3_tree, &child, &iter);
}


long unsigned pl3_artist_browser_view_folder(GtkTreeIter *iter_cat)
{
   char *artist, *string;
   GtkTreeIter iter;
   GtkTreePath *path = NULL;
   int depth = 0;
   long unsigned time =0;
   gtk_tree_model_get(GTK_TREE_MODEL(pl3_tree), iter_cat, 2 , &artist, 1,&string, -1);
   if (check_connection_state ())
      return 0;


   path = gtk_tree_model_get_path(GTK_TREE_MODEL(pl3_tree), iter_cat);
   if(path == NULL)
   {
      printf("Failed to get path\n");
      return 0;
   }
   depth = gtk_tree_path_get_depth(path) -1;                      	
   gtk_tree_path_free(path);
   if(artist == NULL || string == NULL)
   {
      return 0;
   }
   if(depth == 0)
   {
      /*lowest level, do nothing */
      /* fill artist list */
      MpdData *data = mpd_ob_playlist_get_artists(connection);

      while(data != NULL)
      {	
	 gtk_list_store_append (pl3_store,&iter);
	 gtk_list_store_set (pl3_store,&iter,
	       0, data->value.artist,
	       1, PL3_ENTRY_ARTIST, /* the field */
	       2, data->value.artist, /* the artist name, if(1 and 2 together its an artist field) */
	       5, "media-artist",
	       -1);

	 data = mpd_ob_data_get_next(data);
      }
      return 0;
   }
   if(depth == 1)
   {
      int albums = 0;
      MpdData *data = mpd_ob_playlist_get_albums(connection,artist);
      while(data != NULL){
	 gtk_list_store_append (pl3_store, &iter);
	 gtk_list_store_set (pl3_store,&iter,
	       0, artist,
	       1, PL3_ENTRY_ALBUM,
	       2, data->value.album,
	       5, "media-album", 
	       -1);
	 data = mpd_ob_data_get_next(data);
      }


      data = mpd_ob_playlist_find(connection, MPD_TABLE_ARTIST, artist, TRUE);
      /* artist is selected */
      while(data != NULL)
      {
	 if (data->value.song->album == NULL
	       || strlen (data->value.song->album) == 0)
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
	 }
	 else albums++;
	 data = mpd_ob_data_get_next(data);
      }

      if(!albums)
      {
	 if(gtk_tree_model_iter_children(GTK_TREE_MODEL(pl3_tree), &iter, iter_cat))
	 {
	    gtk_tree_store_remove(pl3_tree, &iter);      		
	 }
      }                                                                                  		
   }
   else if(depth ==2)
   {
      /* artist and album is selected */
      MpdData *data = mpd_ob_playlist_find(connection,MPD_TABLE_ALBUM, string, TRUE);
      while (data != NULL)
      {
	 if (data->value.song->artist!= NULL
	       && !g_utf8_collate (data->value.song->artist, artist))
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

	 }
	 data = mpd_ob_data_get_next(data);
      }

   }
   return time;
}


void pl3_artist_browser_fill_tree(GtkTreeIter *iter)
{
   char *artist, *alb_artist;
   int depth =0;
   GtkTreePath *path = NULL;
   GtkTreeIter child,child2;
   gtk_tree_model_get(GTK_TREE_MODEL(pl3_tree),iter, 1, &artist,2,&alb_artist, -1);
   gtk_tree_store_set(pl3_tree, iter, 4, TRUE, -1);

   path = gtk_tree_model_get_path(GTK_TREE_MODEL(pl3_tree), iter);
   if(path == NULL)
   {
      printf("Failed to get path\n");
      return;
   }
   depth = gtk_tree_path_get_depth(path) -1;                      	
   gtk_tree_path_free(path);


   if (!mpd_ob_check_connected(connection))
   {
      return;
   }
   if(depth == 0)
   {
      /* fill artist list */
      MpdData *data = mpd_ob_playlist_get_artists(connection);

      while(data != NULL)
      {	
	 gtk_tree_store_append (pl3_tree, &child, iter);
	 gtk_tree_store_set (pl3_tree, &child,
	       0, PL3_BROWSE_ARTIST,
	       1, data->value.artist, /* the field */
	       2, data->value.artist, /* the artist name, if(1 and 2 together its an artist field) */
	       3, "media-artist",
	       4, FALSE,
	       PL3_CAT_ICON_SIZE,1,
	       -1);
	 gtk_tree_store_append(pl3_tree, &child2, &child);

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
      MpdData *data = mpd_ob_playlist_get_albums(connection,artist);
      while(data != NULL){
	 gtk_tree_store_append (pl3_tree, &child, iter);
	 gtk_tree_store_set (pl3_tree, &child,
	       0, PL3_BROWSE_ARTIST,
	       1, data->value.album,
	       2, artist,
	       3, "media-album", 
	       4, TRUE, 
	       PL3_CAT_ICON_SIZE,1,
	       -1);
	 data = mpd_ob_data_get_next(data);
      }

      if(gtk_tree_model_iter_children(GTK_TREE_MODEL(pl3_tree), &child, iter))
      {
	 gtk_tree_store_remove(pl3_tree, &child); 
      }
   }
}



void pl3_artist_browser_add_folder()
{
   GtkTreeSelection *selec = gtk_tree_view_get_selection((GtkTreeView *)glade_xml_get_widget (pl3_xml, "cat_tree"));
   GtkTreeModel *model = GTK_TREE_MODEL(pl3_tree);
   GtkTreeIter iter;

   if(!mpd_ob_check_connected(connection))
   {
      return;
   }
   if(gtk_tree_selection_get_selected(selec,&model, &iter))
   {
      char *artist, *title;
      gtk_tree_model_get(model, &iter, PL3_CAT_INT_ID, &artist, PL3_CAT_TITLE,&title, -1);
      if(!artist || !title)
      {
	 return;
      }
      if(strlen(artist) && !g_utf8_collate(artist,title))
      {
	 /* artist selected */
	 gchar *message = g_strdup_printf("Added songs from artist '%s'",artist);
	 MpdData * data = mpd_ob_playlist_find(connection, MPD_TABLE_ARTIST, artist, TRUE);
	 while (data != NULL)
	 {                                                                         			
	    mpd_ob_playlist_queue_add(connection, data->value.song->file);
	    data = mpd_ob_data_get_next(data);
	 }
	 pl3_push_statusbar_message(message);
	 g_free(message);

      }
      else
      {
	 /* album selected */
	 /* fetch all songs by this album and check if the artist is right. from mpd and add them to the add-list */
	 gchar *message = g_strdup_printf("Added songs from album '%s' ",title);
	 MpdData *data = mpd_ob_playlist_find(connection, MPD_TABLE_ALBUM, title, TRUE);
	 while (data != NULL)
	 {
	    if (!g_utf8_collate (data->value.song->artist, artist))
	    {
	       mpd_ob_playlist_queue_add(connection,data->value.song->file);
	    }
	    data = mpd_ob_data_get_next(data);
	 }
	 pl3_push_statusbar_message(message);
	 g_free(message);

      }

      /* if there are items in the add list add them to the playlist */
      mpd_ob_playlist_queue_commit(connection);
   }

}

void pl3_artist_browser_replace_folder()
{
   pl3_clear_playlist();
   pl3_artist_browser_add_folder();
   mpd_ob_player_play(connection);
}

void pl3_artist_browser_category_key_press(GdkEventKey *event)
{
   if(event->state == GDK_CONTROL_MASK && event->keyval == GDK_Insert )
   {
      pl3_artist_browser_replace_folder();
   }
   else if (event->keyval == GDK_Insert)
   {
      pl3_artist_browser_add_folder();
   }
}

