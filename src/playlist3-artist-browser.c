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
#include "playlist3-artist-browser.h"
#include "config1.h"
#include "TreeSearchWidget.h"

void pl3_artist_browser_add();
void pl3_artist_browser_fill_tree(GtkWidget *tree, GtkTreeIter *iter);
void pl3_artist_browser_category_selection_changed(GtkWidget *tree,GtkTreeIter *iter);
void pl3_artist_browser_selected(GtkWidget *container);
void pl3_artist_browser_unselected(GtkWidget *container);
int pl3_artist_browser_cat_popup(GtkWidget *menu, int type,GtkWidget *tree, GdkEventButton *event);
void pl3_artist_browser_category_key_press(GtkWidget *tree, GdkEventKey *event, int selected_type);

void pl3_artist_browser_dbase_updated();
void pl3_artist_browser_disconnect();
int pl3_artist_browser_add_go_menu(GtkWidget *menu);


void pl3_artist_browser_row_activated(GtkTreeView *tree, GtkTreePath *tp);
void pl3_artist_browser_show_info();
void pl3_artist_browser_button_release_event(GtkWidget *but, GdkEventButton *event);
void pl3_artist_browser_add_selected();
void pl3_artist_browser_replace_selected();
int pl3_artist_browser_playlist_key_press(GtkWidget *tree, GdkEventKey *event);
void pl3_artist_browser_connection_changed(MpdObj *mi, int connect, gpointer data);
int pl3_artist_browser_key_press_event(GtkWidget *mw, GdkEventKey *event, int type);



enum{
	PL3_AB_ARTIST,
	PL3_AB_ALBUM,
	PL3_AB_FILE,
	PL3_AB_TYPE,
	PL3_AB_TITLE,
	PL3_AB_ICON,
	PL3_AB_ROWS
};
extern GladeXML *pl3_xml;

/**
 * Plugin structure
 */
gmpcPlBrowserPlugin artist_browser_gbp = {
	pl3_artist_browser_add,
	pl3_artist_browser_selected,
	pl3_artist_browser_unselected,
	pl3_artist_browser_category_selection_changed,
	pl3_artist_browser_fill_tree,
	pl3_artist_browser_cat_popup,
	pl3_artist_browser_category_key_press,
	pl3_artist_browser_add_go_menu,
	pl3_artist_browser_key_press_event
};

gmpcPlugin artist_browser_plug = {
	"Artist Browser",
	{1,1,1},
	GMPC_PLUGIN_PL_BROWSER,
	0,
	NULL,			/* path*/
	NULL,			/* init */
	&artist_browser_gbp,		/* Browser */
	NULL,			/* status changed */
	pl3_artist_browser_connection_changed, 		/* connection changed */
	NULL,		/* Preferences */
	NULL,			/* MetaData */
	NULL,			/* Get enable */
	NULL			/* Set Enable */
};

/* internal */
GtkWidget *pl3_ab_tree = NULL;
GtkListStore *pl3_ab_store = NULL;
GtkWidget *pl3_ab_vbox = NULL;
GtkWidget *pl3_ab_tree_search = NULL;

GtkWidget *pl3_cat_tree = NULL; /* the left pane tree */


GtkTreeRowReference *pl3_ab_tree_ref = NULL;


void pl3_artist_browser_dbase_updated()
{
	if(pl3_ab_tree_ref){
		GtkTreeIter parent, child;
		GtkTreeModel *model = gtk_tree_row_reference_get_model(pl3_ab_tree_ref);
		GtkTreePath *path = gtk_tree_row_reference_get_path(pl3_ab_tree_ref);
		if(!path) return;
		if(gtk_tree_model_get_iter(model, &parent, path)){
			while(gtk_tree_model_iter_children(model, &child,&parent))
			{
				gtk_tree_store_remove(GTK_TREE_STORE(model), &child);
			}
		}
		/* add fake, and set not-openend bit */
		gtk_tree_store_append(pl3_tree, &child, &parent);
		gtk_tree_store_set(GTK_TREE_STORE(model), &parent, PL3_CAT_PROC,FALSE,-1);

	}
}

int pl3_artist_browser_button_press_event(GtkTreeView *tree, GdkEventButton *event)
{
	GtkTreeSelection *sel = gtk_tree_view_get_selection(tree);
	if(event->button != 3 || gtk_tree_selection_count_selected_rows(sel) < 2|| !mpd_check_connected(connection))
	{
		return FALSE;
	}
	return TRUE;
}
static void pl3_artist_browser_search_activate()
{
	GtkTreeModel *model = GTK_TREE_MODEL(pl3_ab_store);
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(pl3_ab_tree));
	if (gtk_tree_selection_count_selected_rows (selection) == 1)            
	{
		GList *list = gtk_tree_selection_get_selected_rows (selection, &model);
		pl3_artist_browser_row_activated(GTK_TREE_VIEW(pl3_ab_tree),(GtkTreePath *)list->data);	
		/* free list */
		g_list_foreach (list, (GFunc) gtk_tree_path_free, NULL);                        	
		g_list_free (list);
	}
}

void pl3_artist_browser_init()
{
	GtkWidget *pl3_ab_sw = NULL;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column = NULL;
	GValue value;
	pl3_ab_store = gtk_list_store_new (PL3_AB_ROWS, 
			G_TYPE_STRING, /* PL3_AB_ARTIST */
			G_TYPE_STRING, /* PL3_AB_ALBUM  */
			G_TYPE_STRING, /* PL3_AB_FILE */
			G_TYPE_INT,	 /* PL3_AB_TYPE  */
			G_TYPE_STRING, /* PL3_AB_TITLE */
			GDK_TYPE_PIXBUF); /* icon type */



	renderer = gtk_cell_renderer_pixbuf_new ();

	column = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_column_pack_start (column, renderer, FALSE);
	gtk_tree_view_column_set_attributes (column,renderer,"pixbuf", PL3_AB_ICON,NULL);
	memset(&value, 0, sizeof(value));
	/* set value for ALL */
	g_value_init(&value, G_TYPE_FLOAT);
	g_value_set_float(&value, 0.0);
	g_object_set_property(G_OBJECT(renderer), "yalign", &value); 

	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_set_attributes (column,renderer,"text", PL3_AB_TITLE, NULL);


	/* set up the tree */
	pl3_ab_tree= gtk_tree_view_new_with_model(GTK_TREE_MODEL(pl3_ab_store));
	/* insert the column in the tree */
	gtk_tree_view_append_column (GTK_TREE_VIEW (pl3_ab_tree), column);
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(pl3_ab_tree), FALSE);
	gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(pl3_ab_tree), TRUE);
	gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW(pl3_ab_tree)), GTK_SELECTION_MULTIPLE);
	gtk_tree_view_set_enable_search(GTK_TREE_VIEW(pl3_ab_tree), FALSE);

	/* setup signals */
	g_signal_connect(G_OBJECT(pl3_ab_tree), "row-activated",G_CALLBACK(pl3_artist_browser_row_activated), NULL); 
	g_signal_connect(G_OBJECT(pl3_ab_tree), "button-press-event", G_CALLBACK(pl3_artist_browser_button_press_event), NULL);
	g_signal_connect(G_OBJECT(pl3_ab_tree), "button-release-event", G_CALLBACK(pl3_artist_browser_button_release_event), NULL);
	g_signal_connect(G_OBJECT(pl3_ab_tree), "key-press-event", G_CALLBACK(pl3_artist_browser_playlist_key_press), NULL);

	/* set up the scrolled window */
	pl3_ab_sw = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(pl3_ab_sw), GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(pl3_ab_sw), GTK_SHADOW_ETCHED_IN);

	pl3_ab_vbox = gtk_vbox_new(FALSE, 6);

	gtk_container_add(GTK_CONTAINER(pl3_ab_sw), pl3_ab_tree);

	gtk_box_pack_start(GTK_BOX(pl3_ab_vbox), pl3_ab_sw, TRUE, TRUE,0);
	gtk_widget_show_all(pl3_ab_sw);	
	pl3_ab_tree_search = treesearch_new(GTK_TREE_VIEW(pl3_ab_tree), PL3_AB_TITLE);
	gtk_box_pack_end(GTK_BOX(pl3_ab_vbox), pl3_ab_tree_search, FALSE, TRUE,0);
	g_signal_connect(G_OBJECT(pl3_ab_tree_search),"result-activate", G_CALLBACK(pl3_artist_browser_search_activate), NULL);
	/* set initial state */
	g_object_ref(G_OBJECT(pl3_ab_vbox));
}


void pl3_artist_browser_add()
{
	GtkTreePath *path;
	GtkTreeIter iter,child;
	gtk_tree_store_append(pl3_tree, &iter, NULL);
	gtk_tree_store_set(pl3_tree, &iter, 
			PL3_CAT_TYPE, artist_browser_plug.id,
			PL3_CAT_TITLE, _("Browse Artists"),
			PL3_CAT_INT_ID, "",
			PL3_CAT_ICON_ID, "media-artist",
			PL3_CAT_PROC, FALSE,
			PL3_CAT_ICON_SIZE,GTK_ICON_SIZE_DND,-1);
	/* add fantom child for lazy tree */
	gtk_tree_store_append(pl3_tree, &child, &iter);
	if(pl3_ab_tree_ref)
	{
		gtk_tree_row_reference_free(pl3_ab_tree_ref);
		pl3_ab_tree_ref = NULL;
	}
	path = gtk_tree_model_get_path(GTK_TREE_MODEL(pl3_tree), &iter);
	if(path)
	{
		pl3_ab_tree_ref = gtk_tree_row_reference_new(GTK_TREE_MODEL(pl3_tree),path);
		gtk_tree_path_free(path);
	}
}

void pl3_artist_browser_cover_art_fetched(mpd_Song *song, MetaDataResult ret, char *coverpath,gpointer data)
{
	GtkTreeRowReference *ref = data;
	if(song == NULL || ref == NULL) return;
	else
	{
		GtkTreeIter iter;
		GtkTreePath *path = gtk_tree_row_reference_get_path(ref);
		if(path)
		{
			if(gtk_tree_model_get_iter(GTK_TREE_MODEL(pl3_ab_store), &iter, path))
			{
				if(ret == META_DATA_AVAILABLE)
				{
					int size = cfg_get_single_value_as_int_with_default(config, "cover-art", "browser-size",64);
					GdkPixbuf *pb = gdk_pixbuf_new_from_file_at_size(coverpath,size,size,NULL);
					screenshot_add_border(&pb);
					//draw_pixbuf_border(pb);	
					gtk_list_store_set(pl3_ab_store,&iter, PL3_AB_ICON, pb, -1);
					if(pb)g_object_unref(pb);
					gtk_tree_row_reference_free(ref);
				}
				else if(ret == META_DATA_FETCHING)
				{
					GdkPixbuf *pb= gtk_widget_render_icon(GTK_WIDGET(pl3_ab_tree), "media-loading-cover", -1, NULL);
					gtk_list_store_set(pl3_ab_store,&iter, PL3_AB_ICON, pb, -1);
					if(pb)g_object_unref(pb);
				}
				else
				{
					GdkPixbuf *pb = gtk_widget_render_icon(GTK_WIDGET(pl3_ab_tree),"media-no-cover", -1, NULL);
					gtk_list_store_set(pl3_ab_store,&iter, PL3_AB_ICON, pb, -1);
					if(pb)g_object_unref(pb);
					gtk_tree_row_reference_free(ref);
				}
			}
			gtk_tree_path_free(path);
		}
	}

}



long unsigned pl3_artist_browser_view_folder(GtkTreeIter *iter_cat)
{
	char *artist, *string;
	GtkTreeIter iter;
	GtkTreePath *path = NULL;
	int depth = 0;
	long unsigned time =0;

	if(pl3_ab_tree == NULL || pl3_ab_store == NULL) return 0;

	gtk_tree_model_get(GTK_TREE_MODEL(pl3_tree), iter_cat, 2 , &artist, 1,&string, -1);
	if (!mpd_check_connected(connection))
		return 0;


	path = gtk_tree_model_get_path(GTK_TREE_MODEL(pl3_tree), iter_cat);
	if(path == NULL)
	{
		return 0;
	}
	depth = gtk_tree_path_get_depth(path) -1;
	gtk_tree_path_free(path);
	if(artist == NULL || string == NULL)
	{
		if(artist) g_free(artist);
		if(string) g_free(string);
		return 0;
	}
	if(depth == 0)
	{
		/*lowest level, do nothing */
		/* fill artist list */
		MpdData *data = mpd_database_get_artists(connection);
		gtk_tree_view_set_model(GTK_TREE_VIEW(pl3_ab_tree), NULL);
		while(data != NULL)
		{
			GdkPixbuf *pb = gtk_widget_render_icon(pl3_ab_tree, "media-artist", GTK_ICON_SIZE_MENU,NULL);
			gtk_list_store_append (pl3_ab_store,&iter);
			gtk_list_store_set (pl3_ab_store,&iter,
					PL3_AB_ARTIST, data->tag,
					PL3_AB_TYPE, PL3_ENTRY_ARTIST, /* the field */
					PL3_AB_TITLE, data->tag, /* the artist name, if(1 and 2 together its an artist field) */
					PL3_AB_ICON,pb,
					-1);
			if(pb)g_object_unref(pb);
			/* To Damn Slow 
			if(strlen(data->tag))//ret == COVER_ART_NOT_FETCHED)
			{
				GtkTreePath *path = gtk_tree_model_get_path(GTK_TREE_MODEL(pl3_ab_store), &iter);
				GtkTreeRowReference* rowref = gtk_tree_row_reference_new(GTK_TREE_MODEL(pl3_ab_store),path);		
				mpd_Song *song = mpd_newSong();
				song->artist = g_strdup(data->tag);
				song->album = NULL; 
				meta_data_get_path_callback(song, META_ARTIST_ART, pl3_artist_browser_cover_art_fetched, rowref);
				mpd_freeSong(song);	
				gtk_tree_path_free(path);
			}
*/


			data = mpd_data_get_next(data);
		}
		gtk_tree_view_set_model(GTK_TREE_VIEW(pl3_ab_tree), GTK_TREE_MODEL(pl3_ab_store));

		g_free(artist);
		g_free(string);

		return 0;
	}
	if(depth == 1)
	{
		int albums = 0;
		GdkPixbuf *pb = NULL;
		MpdData *data = NULL;
		/**
		 * Set "Up" entry
		 */
		pb = gtk_widget_render_icon(pl3_ab_tree, "gtk-go-up",GTK_ICON_SIZE_MENU,NULL);
		gtk_list_store_append (pl3_ab_store, &iter);
		gtk_list_store_set (pl3_ab_store,&iter,
				PL3_AB_ARTIST, NULL,
				PL3_AB_ALBUM, NULL,
				PL3_AB_TYPE, PL3_ENTRY_DIR_UP,
				PL3_AB_TITLE, "..",
				PL3_AB_ICON,pb,
				-1);
		if(pb) g_object_unref(pb);	

		data = mpd_database_get_albums(connection,artist);
		while(data != NULL){
			GtkTreePath *path = NULL;
			GtkTreeRowReference* rowref = NULL;
			mpd_Song *song = NULL; 

			pb = gtk_widget_render_icon(pl3_ab_tree, "media-no-cover",-1/* GTK_ICON_SIZE_MENU*/,NULL);
			gtk_list_store_append (pl3_ab_store, &iter);
			gtk_list_store_set (pl3_ab_store,&iter,
					PL3_AB_ARTIST, artist,
					PL3_AB_ALBUM, data->tag,
					PL3_AB_TYPE, PL3_ENTRY_ALBUM,
					PL3_AB_TITLE, data->tag,
					PL3_AB_ICON,pb,
					-1);
			if(pb)g_object_unref(pb);

			/**
			 * create song and request metadata 
			 */
			path = gtk_tree_model_get_path(GTK_TREE_MODEL(pl3_ab_store), &iter);
			rowref = gtk_tree_row_reference_new(GTK_TREE_MODEL(pl3_ab_store),path);		
			song = mpd_newSong();                                                                     			
			song->artist = g_strdup(artist);
			song->album = g_strdup(data->tag);
			meta_data_get_path_callback(song, META_ALBUM_ART, pl3_artist_browser_cover_art_fetched, rowref);
			mpd_freeSong(song);	
			gtk_tree_path_free(path);

			
			data = mpd_data_get_next(data);
		}


		data = mpd_database_find(connection, MPD_TABLE_ARTIST, artist, TRUE);
		/* artist is selected */
		while(data != NULL)
		{
			if(data->type == MPD_DATA_TYPE_SONG)
			{
				if (data->song->album == NULL || strlen (data->song->album) == 0)
				{
					gchar buffer[1024];
					GdkPixbuf *pb = gtk_widget_render_icon(pl3_ab_tree, "media-audiofile", GTK_ICON_SIZE_MENU,NULL);
					char *markdata = cfg_get_single_value_as_string_with_default(config, "playlist", "browser_markup",DEFAULT_MARKUP_BROWSER);
					mpd_song_markup(buffer, 1024,markdata,data->song);
					cfg_free_string(markdata);
					if(data->song->time != MPD_SONG_NO_TIME)
					{
						time += data->song->time;
					}
					if(data->song->file == NULL)
					{
						debug_printf(DEBUG_WARNING,"pl3_browser_view_folder: crap mpdSong has no file attribute.\n");
					}
					gtk_list_store_append (pl3_ab_store, &iter);
					gtk_list_store_set (pl3_ab_store, &iter,
							PL3_AB_TITLE,	buffer,
							PL3_AB_ARTIST,  data->song->artist,
							PL3_AB_ALBUM,   data->song->album,
							PL3_AB_FILE,	data->song->file,
							PL3_AB_TYPE,	PL3_ENTRY_SONG,
							PL3_AB_ICON,	pb,
							-1);
					if(pb)g_object_unref(pb);
				}
				else albums++;
			}
			data = mpd_data_get_next(data);
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
		char *markdata = cfg_get_single_value_as_string_with_default(config, "playlist", "browser_markup",DEFAULT_MARKUP_BROWSER);
		GdkPixbuf *pb = NULL;
		MpdData *data = NULL;
		/**
		 * Set "Up" entry
		 */
		pb = gtk_widget_render_icon(pl3_ab_tree, "gtk-go-up",GTK_ICON_SIZE_MENU,NULL);
		gtk_list_store_append (pl3_ab_store, &iter);                                  		
		gtk_list_store_set (pl3_ab_store,&iter,
				PL3_AB_ARTIST, NULL,
				PL3_AB_ALBUM, NULL,
				PL3_AB_TYPE, PL3_ENTRY_DIR_UP,
				PL3_AB_TITLE, "..",
				PL3_AB_ICON,pb,
				-1);
		if(pb) g_object_unref(pb);	
		/* artist and album is selected */
		data = mpd_database_find(connection,MPD_TABLE_ALBUM, string, TRUE);
		while (data != NULL)
		{
			if(data->type == MPD_DATA_TYPE_SONG)
			{
				if (data->song->artist!= NULL && !g_utf8_collate (data->song->artist, artist))
				{
					gchar buffer[1024];
					pb = gtk_widget_render_icon(pl3_ab_tree, "media-audiofile", GTK_ICON_SIZE_MENU,NULL);
					mpd_song_markup(buffer, 1024,markdata,data->song);
					if(data->song->time != MPD_SONG_NO_TIME)
					{
						time += data->song->time;
					}
					gtk_list_store_append (pl3_ab_store, &iter);
					gtk_list_store_set (pl3_ab_store, &iter,
							PL3_AB_TITLE, buffer,
							PL3_AB_ARTIST, data->song->artist,
							PL3_AB_ALBUM, data->song->album,
							PL3_AB_FILE, data->song->file,
							PL3_AB_TYPE, PL3_ENTRY_SONG,
							PL3_AB_ICON,pb,
							-1);
					if(pb)g_object_unref(pb);
				}
			}
			data = mpd_data_get_next(data);
		}
		cfg_free_string(markdata);

	}
	g_free(artist);
	g_free(string);

	return time;
}


void pl3_artist_browser_fill_tree(GtkWidget *tree, GtkTreeIter *iter)
{
	char *artist, *alb_artist;
	int depth =0;
	GtkTreePath *path = NULL;
	GtkTreeIter child,child2;
	if (!mpd_check_connected(connection))
	{
		return;
	}

	path = gtk_tree_model_get_path(GTK_TREE_MODEL(pl3_tree), iter);
	if(path == NULL)
	{
		return;
	}
	depth = gtk_tree_path_get_depth(path) -1;
	gtk_tree_path_free(path);

	gtk_tree_model_get(GTK_TREE_MODEL(pl3_tree),iter, 1, &artist,2,&alb_artist, -1);
	gtk_tree_store_set(pl3_tree, iter, 4, TRUE, -1);


	if(depth == 0)
	{
		/* fill artist list */
		MpdData *data = mpd_database_get_artists(connection);

		while(data != NULL)
		{
			gtk_tree_store_append (pl3_tree, &child, iter);
			gtk_tree_store_set (pl3_tree, &child,
					0, artist_browser_plug.id,
					1, data->tag, /* the field */
					2, data->tag, /* the artist name, if(1 and 2 together its an artist field) */
					3, "media-artist",
					4, FALSE,
					PL3_CAT_ICON_SIZE,1,
					-1);
			gtk_tree_store_append(pl3_tree, &child2, &child);

			data = mpd_data_get_next(data);
		}
		if(gtk_tree_model_iter_children(GTK_TREE_MODEL(pl3_tree), &child, iter))
		{
			gtk_tree_store_remove(pl3_tree, &child); 
		}
	}
	/* if where inside a artist */
	else if(depth == 1)
	{
		MpdData *data = mpd_database_get_albums(connection,artist);
		while(data != NULL){
			gtk_tree_store_append (pl3_tree, &child, iter);
			gtk_tree_store_set (pl3_tree, &child,
					0, artist_browser_plug.id,
					1, data->tag,
					2, artist,
					3, "media-album", 
					4, TRUE, 
					PL3_CAT_ICON_SIZE,1,
					-1);
			data = mpd_data_get_next(data);
		}

		if(gtk_tree_model_iter_children(GTK_TREE_MODEL(pl3_tree), &child, iter))
		{
			gtk_tree_store_remove(pl3_tree, &child); 
		}
	}
	g_free(artist);
	g_free(alb_artist);
}



void pl3_artist_browser_add_folder()
{
	GtkTreeSelection *selec = gtk_tree_view_get_selection((GtkTreeView *)pl3_cat_tree);
	GtkTreeModel *model = GTK_TREE_MODEL(pl3_tree);
	GtkTreeIter iter;

	if(!mpd_check_connected(connection))
	{
		return;
	}
	if(gtk_tree_selection_get_selected(selec,&model, &iter))
	{
		char *artist, *title;
		gtk_tree_model_get(model, &iter, PL3_CAT_INT_ID, &artist, PL3_CAT_TITLE,&title, -1);
		if(!artist || !title)
		{
			if(artist)g_free(artist);
			if(title)g_free(title);
			return;
		}
		if(strlen(artist) && !g_utf8_collate(artist,title))
		{
			/* artist selected */
			gchar *message = g_strdup_printf("Added songs from artist '%s'",artist);
			MpdData * data = mpd_database_find(connection, MPD_TABLE_ARTIST, artist, TRUE);
			while (data != NULL)
			{
				if(data->type == MPD_DATA_TYPE_SONG)
				{
					mpd_playlist_queue_add(connection, data->song->file);
				}
				data = mpd_data_get_next(data);
			}
			pl3_push_statusbar_message(message);
			g_free(message);

		}
		else
		{
			/* album selected */
			/* fetch all songs by this album and check if the artist is right. from mpd and add them to the add-list */
			gchar *message = g_strdup_printf("Added songs from album '%s' ",title);
			MpdData *data = mpd_database_find(connection, MPD_TABLE_ALBUM, title, TRUE);
			while (data != NULL)
			{
				if(data->type == MPD_DATA_TYPE_SONG)
				{
					if (!g_utf8_collate (data->song->artist, artist))
					{
						mpd_playlist_queue_add(connection,data->song->file);
					}
				}
				data = mpd_data_get_next(data);
			}
			pl3_push_statusbar_message(message);
			g_free(message);

		}
		/* if there are items in the add list add them to the playlist */
		mpd_playlist_queue_commit(connection);
		if(artist)g_free(artist);
		if(title)g_free(title);
	}
}

void pl3_artist_browser_replace_folder()
{
	mpd_playlist_clear(connection);
	pl3_artist_browser_add_folder();
	mpd_player_play(connection);
}

void pl3_artist_browser_category_key_press(GtkWidget *tree, GdkEventKey *event, int selected_type)
{
	if(event->state&GDK_CONTROL_MASK && event->keyval == GDK_Insert )
	{
		pl3_artist_browser_replace_folder();
	}

	else if (event->keyval == GDK_Insert)
	{
		pl3_artist_browser_add_folder();
	}
}

void pl3_artist_browser_show_info()
{
	GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW(pl3_ab_tree));
	GtkTreeSelection *selection =gtk_tree_view_get_selection (GTK_TREE_VIEW(pl3_ab_tree));
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
			gtk_tree_model_get (GTK_TREE_MODEL(pl3_ab_store), &iter,
					PL3_AB_FILE, &path,
					PL3_AB_TYPE, &type,
					-1);
			if(type == PL3_ENTRY_SONG)
			{
				data = mpd_database_find_adv(connection,TRUE,MPD_TAG_ITEM_FILENAME,path,-1);
				while(data != NULL)
				{
					if(data->type == MPD_DATA_TYPE_SONG)
					{
						call_id3_window_song(mpd_songDup(data->song));
					}
					data = mpd_data_get_next(data);
				}
			}
			if(path)g_free(path);
		}
		while ((list = g_list_previous (list)) && mpd_check_connected(connection));
		/* free list */
		g_list_foreach (list, (GFunc) gtk_tree_path_free, NULL);
		g_list_free (list);
	}
}

void pl3_artist_browser_row_activated(GtkTreeView *tree, GtkTreePath *tp)
{
	GtkTreeIter iter;
	gchar *artist = NULL;
	gchar *album = NULL;
	gchar *file = NULL;
	gint r_type;
	int playlist_length = mpd_playlist_get_playlist_length(connection);

	gtk_tree_model_get_iter(gtk_tree_view_get_model(tree), &iter, tp);
	gtk_tree_model_get(gtk_tree_view_get_model(tree), &iter, 
			PL3_AB_ARTIST, &artist,
			PL3_AB_ALBUM, &album,
			PL3_AB_FILE, &file,
			PL3_AB_TYPE,&r_type,
			-1);
	if (r_type&(PL3_ENTRY_ARTIST|PL3_ENTRY_ALBUM))
	{
		GtkTreeSelection *selec = gtk_tree_view_get_selection((GtkTreeView *)pl3_cat_tree);
		GtkTreeModel *model = GTK_TREE_MODEL(pl3_tree);
		GtkTreeIter iter;

		if(gtk_tree_selection_get_selected(selec,&model, &iter))
		{
			GtkTreeIter citer;
			GtkTreePath *path = gtk_tree_model_get_path(model, &iter);
			gtk_tree_view_expand_row(GTK_TREE_VIEW(pl3_cat_tree), path, FALSE);
			gtk_tree_path_free(path);
			if(gtk_tree_model_iter_children(model, &citer, &iter))
			{
				do{
					char *name = NULL;
					gtk_tree_model_get(model, &citer, 1, &name, -1);
					if(strcmp(name, (r_type&PL3_ENTRY_ARTIST)?artist:album) == 0)
					{
						gtk_tree_selection_select_iter(selec,&citer);
						path = gtk_tree_model_get_path(model, &citer);
						gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(pl3_cat_tree),
								path,NULL,TRUE,0.5,0);
						gtk_tree_path_free(path);
						g_free(name);
						break;
					}
					g_free(name);
				}while(gtk_tree_model_iter_next(model, &citer));
			}
		}
	}
	else if(r_type&PL3_ENTRY_DIR_UP)
	{
		GtkTreeSelection *selec = gtk_tree_view_get_selection((GtkTreeView *)pl3_cat_tree);
		GtkTreeModel *model = GTK_TREE_MODEL(pl3_tree);
		GtkTreeIter iter,parent;

		if(gtk_tree_selection_get_selected(selec,&model, &iter))
		{
			gtk_tree_model_iter_parent(model, &parent, &iter);
			gtk_tree_selection_select_iter(selec, &parent);
		}
	}
	else
	{
		pl3_push_statusbar_message("Added a song");
		mpd_playlist_queue_add(connection, file);
	}

	mpd_playlist_queue_commit(connection);
	if(file)g_free(file);
	if(artist) g_free(artist);
	if(album) g_free(album);
	if(playlist_length == 0)
	{
		mpd_player_play(connection);
	}
}

void pl3_artist_browser_category_selection_changed(GtkWidget *tree,GtkTreeIter *iter)
{
	long unsigned time= 0;
	gchar *string;
	gtk_list_store_clear(pl3_ab_store);
	time = pl3_artist_browser_view_folder(iter);
	string = format_time(time);
	pl3_push_rsb_message(string);
	g_free(string);
	/* store the tree */
	pl3_cat_tree= GTK_WIDGET(tree);
}

void pl3_artist_browser_selected(GtkWidget *container)
{
	if(pl3_ab_tree == NULL)
	{
		pl3_artist_browser_init();
	}

	gtk_container_add(GTK_CONTAINER(container), pl3_ab_vbox);
	gtk_widget_grab_focus(pl3_ab_tree);
	gtk_widget_show(pl3_ab_vbox);
}
void pl3_artist_browser_unselected(GtkWidget *container)
{
	gtk_container_remove(GTK_CONTAINER(container),pl3_ab_vbox);
}

void pl3_artist_browser_button_release_event(GtkWidget *but, GdkEventButton *event)
{
	if(event->button != 3) return;
	GtkWidget *item;
	GtkWidget *menu = gtk_menu_new();
	GtkTreeSelection *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(pl3_ab_tree));
	/* 
	 * show always when version 12..  or when searching in playlist.
	 */
	if(gtk_tree_selection_count_selected_rows(sel) == 1)
	{
		GtkTreeModel *model = GTK_TREE_MODEL(pl3_ab_store);
		GList *list = gtk_tree_selection_get_selected_rows(sel, &model);
		if(list != NULL)
		{
			GtkTreeIter iter;
			int row_type;
			char *path;
			list = g_list_first(list);
			gtk_tree_model_get_iter(model, &iter, list->data);
			gtk_tree_model_get(model, &iter,PL3_AB_ARTIST,&path,PL3_AB_TYPE, &row_type, -1);
			if(row_type&PL3_ENTRY_SONG)
			{
				if(mpd_server_check_version(connection,0,12,0))
				{
					item = gtk_image_menu_item_new_from_stock(GTK_STOCK_DIALOG_INFO,NULL);
					gtk_menu_shell_prepend(GTK_MENU_SHELL(menu), item);
					g_signal_connect(G_OBJECT(item), "activate",
							G_CALLBACK(pl3_artist_browser_show_info), NULL);
				}
			}
			g_list_foreach (list,(GFunc) gtk_tree_path_free, NULL);
			g_list_free (list);
		}
	}
	/* add the replace widget */
	item = gtk_image_menu_item_new_with_label("Replace");
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
			gtk_image_new_from_stock(GTK_STOCK_REDO, GTK_ICON_SIZE_MENU));
	gtk_menu_shell_prepend(GTK_MENU_SHELL(menu), item);
	g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(pl3_artist_browser_replace_selected), NULL);
	/* add the add widget */
	item = gtk_image_menu_item_new_from_stock(GTK_STOCK_ADD,NULL);
	gtk_menu_shell_prepend(GTK_MENU_SHELL(menu), item);
	g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(pl3_artist_browser_add_selected), NULL);

	gtk_widget_show_all(menu);
	gtk_menu_popup(GTK_MENU(menu), NULL, NULL,NULL, NULL, event->button, event->time);
	return;
}

void pl3_artist_browser_replace_selected()
{
	mpd_playlist_clear(connection);
	pl3_artist_browser_add_selected();
	mpd_player_play(connection);
}
void pl3_artist_browser_add_selected()
{
	GtkTreeIter iter;
	GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW(pl3_ab_tree));
	GtkTreeModel *model = GTK_TREE_MODEL (pl3_ab_store);
	GList *rows = gtk_tree_selection_get_selected_rows (selection, &model);
	int songs=0;
	gchar *message;
	if(rows != NULL)
	{
		gchar *artist,*album,*file;
		gint type;
		GList *node = g_list_first(rows);
		do
		{
			GtkTreePath *path = node->data;
			gtk_tree_model_get_iter (model, &iter, path);
			gtk_tree_model_get (model, &iter, PL3_AB_ARTIST,&artist, 
					PL3_AB_FILE, &file,
					PL3_AB_ALBUM, &album,
					PL3_AB_TYPE, &type, -1);
			/* does this bitmask thingy works ok? I think it hsould */
			if(type&(PL3_ENTRY_SONG))
			{
				/* add them to the add list */
				mpd_playlist_queue_add(connection, file);
				songs++;
			}
			else if (type&PL3_ENTRY_ARTIST)
			{
				MpdData * data = mpd_database_find(connection, MPD_TABLE_ARTIST, artist, TRUE);
				while (data != NULL)
				{
					if(data->type == MPD_DATA_TYPE_SONG)
					{
						mpd_playlist_queue_add(connection, data->song->file);
						songs++;
					}
					data = mpd_data_get_next(data);
				}
			}
			else if (type&PL3_ENTRY_ALBUM)
			{
				MpdData *data = NULL;
				data = mpd_database_find(connection, MPD_TABLE_ALBUM, album, TRUE);
				while (data != NULL)
				{
					if(data->type == MPD_DATA_TYPE_SONG)
					{
						if (!g_utf8_collate (data->song->artist, artist))
						{
							songs++;
							mpd_playlist_queue_add(connection,data->song->file);
						}
					}
					data = mpd_data_get_next(data);
				}
			}
			if(artist)g_free(artist);
			if(album)g_free(album);
			if(file)g_free(file);

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

int pl3_artist_browser_playlist_key_press(GtkWidget *tree, GdkEventKey *event)
{
	if(event->state&GDK_CONTROL_MASK && event->keyval == GDK_Insert)
	{
		pl3_artist_browser_replace_selected();
	}
	else if (event->state&GDK_CONTROL_MASK && event->keyval == GDK_f)
	{
		treesearch_start(TREESEARCH(pl3_ab_tree_search));
	}                                                                	
	else if(event->keyval == GDK_Insert)
	{
		pl3_artist_browser_add_selected();
	}
	else if(event->keyval == GDK_i)
	{
		pl3_artist_browser_show_info();
	}
	else
	{
		return pl3_window_key_press_event(tree,event);
	}
	return TRUE;
}

int pl3_artist_browser_cat_popup(GtkWidget *menu, int type,GtkWidget *tree, GdkEventButton *event)
{
	if(type == artist_browser_plug.id)
	{
		/* here we have:  Add. Replace*/
		GtkWidget *item;
		/* add the add widget */
		item = gtk_image_menu_item_new_from_stock(GTK_STOCK_ADD,NULL);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
		g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(pl3_artist_browser_add_folder), NULL);

		/* add the replace widget */
		item = gtk_image_menu_item_new_with_label("Replace");
		gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
				gtk_image_new_from_stock(GTK_STOCK_REDO, GTK_ICON_SIZE_MENU));
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
		g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(pl3_artist_browser_replace_folder), NULL);

		return 1;
	}
	return 0;
}



void pl3_artist_browser_disconnect()
{
	if(pl3_ab_tree_ref) {
		GtkTreeIter iter;
		GtkTreePath *path = gtk_tree_row_reference_get_path(pl3_ab_tree_ref);
		if(path && gtk_tree_model_get_iter(GTK_TREE_MODEL(pl3_tree), &iter, path))
		{
			GtkTreeIter child;
			int valid = gtk_tree_model_iter_children(GTK_TREE_MODEL(pl3_tree), &child, &iter);
			while(valid){
				valid = gtk_tree_store_remove(pl3_tree,&child);
			}
			/* set unopened */
			gtk_tree_store_set(pl3_tree,&iter,PL3_CAT_PROC,FALSE,-1);
			/* add phantom child */
			gtk_tree_store_append(pl3_tree, &child, &iter);
		}
		if(pl3_ab_store) gtk_list_store_clear(pl3_ab_store);
	}
}



void pl3_artist_browser_activate()
{
	GtkTreeSelection *selec = gtk_tree_view_get_selection((GtkTreeView *)
			glade_xml_get_widget (pl3_xml, "cat_tree"));

	/**
	 * Fix this to be nnot static
	 */	
	GtkTreePath *path = gtk_tree_path_new_from_string("2"); 
	if(path)
	{
		gtk_tree_selection_select_path(selec, path);
	}
}


int pl3_artist_browser_add_go_menu(GtkWidget *menu)
{
	GtkWidget *item = NULL;

	item = gtk_image_menu_item_new_with_label(_("Artist Browser"));
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item), 
			gtk_image_new_from_stock("media-artist", GTK_ICON_SIZE_MENU));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	g_signal_connect(G_OBJECT(item), "activate", 
			G_CALLBACK(pl3_artist_browser_activate), NULL);

	return 1;
}

void pl3_artist_browser_connection_changed(MpdObj *mi, int connect, gpointer data)
{
	if(!connect)
	{
		pl3_artist_browser_disconnect();
	}
}
int pl3_artist_browser_key_press_event(GtkWidget *mw, GdkEventKey *event, int type)
{
	if (event->keyval == GDK_F3)
	{
		pl3_artist_browser_activate();
		return TRUE;
	}                                           	

	return FALSE;
}
