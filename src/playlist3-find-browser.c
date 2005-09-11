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

/*****************************************************************
 * Find Browser
 */
/* add's the toplevel entry for the current playlist view */
void pl_find_browser_add()
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

unsigned long pl_find_browser_view_browser()
{
   GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW(glade_xml_get_widget (pl3_xml, "cat_tree")));
   GtkTreeModel *model = GTK_TREE_MODEL(pl3_tree);
   GtkTreeIter iter;
   int time=0;
   gtk_list_store_clear(pl3_store);
   if(gtk_tree_selection_get_selected(selection,&model, &iter))
   {
      gchar *name=NULL;
      gint num_field=0;
      GtkTreeIter child;

      MpdData *data = NULL;
      name = (gchar *)gtk_entry_get_text(GTK_ENTRY(glade_xml_get_widget(pl3_xml, "search_entry")));
      num_field = gtk_combo_box_get_active(GTK_COMBO_BOX(glade_xml_get_widget(pl3_xml, "cb_field_selector")));
      if(name == NULL || !strlen(name)) return 0 ;
      /* do the actual search */
      if(num_field < 4)
      {
	 data = mpd_ob_playlist_find(connection, num_field, name, FALSE);
      }
      else if(num_field == 4){
	 data = mpd_ob_playlist_token_find(connection, name);
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
		  gtk_list_store_append(pl3_store, &piter);
		  gtk_list_store_set(pl3_store, &piter,
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
	 char *markdata = cfg_get_single_value_as_string_with_default(config, "playlist", "browser_markup",DEFAULT_MARKUP_BROWSER);
	 if(data->value.song->time != MPD_SONG_NO_TIME)
	 {
	    time += data->value.song->time;
	 }

	 strfsong (buffer, 1024, markdata,
	       data->value.song);
	 cfg_free_string(markdata);
	 /* add as child of the above created parent folder */
	 gtk_list_store_append (pl3_store, &child);
	 gtk_list_store_set (pl3_store, &child,
	       PL3_SONG_ID, data->value.song->file,
	       PL3_SONG_TITLE, buffer,
	       PL3_SONG_POS, PL3_ENTRY_SONG, 
	       PL3_SONG_STOCK_ID, "media-audiofile", 
	       -1);

	 data =  mpd_ob_data_get_next(data);
      }
   }
   return time;
}


void pl_find_browser_entry_change(GtkEntry *entry)
{
   gtk_widget_set_sensitive(glade_xml_get_widget(pl3_xml, "find_button"), (strlen(gtk_entry_get_text(entry)) > 0)?1:0);

}

void pl_find_browser_search()
{

   pl_find_browser_view_browser();
   return;	
}
