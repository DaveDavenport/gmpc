#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <string.h>
#include <glade/glade.h>
#include <time.h>
#include "libmpdclient.h"
#include "strfsong.h"
#include "main.h"
#include "misc.h"
#include "playlist2.h"
#include "song-browser.h"
#include "open-location.h"

GladeXML *pl3_xml = NULL;
GtkTreeStore *pl3_tree = NULL;
GtkListStore *pl3_store = NULL;
extern GtkTreeModel *pl2_store;

enum{
	PL3_CURRENT_PLAYLIST,
	PL3_BOOKMARKS,
	PL3_BROWSE_FILE,
	PL3_BROWSE_ARTIST,
	PL3_BROWSE_ALBUM
	/* more space for options, like shoutcast */
} tree_type;


enum pl3_cat_store
{
	PL3_CAT_TYPE,
	PL3_CAT_TITLE,
	PL3_CAT_INT_ID,
	PL3_CAT_ICON_ID,
	PL3_CAT_PROC, /* for the lazy tree, if the dir is allready processed */
	PL3_CAT_NROWS
} pl3_cat_store;

#define PL3_ENTRY_PLAYLIST 1
#define PL3_ENTRY_SONG 0

/* for the tree in the right pane. */
enum pl3_store_types
{
	PL3_SONG_ID,
	PL3_SONG_POS,
	PL3_SONG_TITLE,
	PL3_WEIGHT_INT,
	PL3_WEIGHT_ENABLE,
	PL3_SONG_STOCK_ID,
	PL3_NROWS
} pl3_store_type;

/* Get the type of the selected row.. 
 * -1 means no row selected 
 */
int  cat_get_selected_browser()
{
	GtkTreeSelection *selec = gtk_tree_view_get_selection((GtkTreeView *)glade_xml_get_widget (pl3_xml, "cat_tree"));
	GtkTreeModel *model = GTK_TREE_MODEL(pl3_tree);
	GtkTreeIter iter;

	if(gtk_tree_selection_get_selected(selec,&model, &iter))
	{
		gint type;
		gtk_tree_model_get(model, &iter, 0, &type, -1);
		if(type >= PL3_CAT_NROWS || type < 0)
		{
			return -1;
		}
		return type;
	}
	return -1;
}

void cat_row_activated(GtkTreeView *tree, GtkTreePath *tp, GtkTreeViewColumn *col)
{
	gint type = cat_get_selected_browser();
	if(check_connection_state())
	{
		return;
	}
	/* nothing valid, so return */
	if(type == -1)
	{
		return;
	}
	
	else if(type == PL3_CURRENT_PLAYLIST)
	{
		/* scroll to the playing song */
		if(info.status != NULL)
		{
			if(info.status->song != -1)
			{
			gchar *str = g_strdup_printf("%i", info.status->song);
			GtkTreePath *path = gtk_tree_path_new_from_string(str);
			gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(
				glade_xml_get_widget(pl3_xml, "playlist_tree")), 
				path,
				NULL,
				TRUE,0.5,0);
			gtk_tree_path_free(path);
			g_free(str);
			}                                                      		
		}
	}
}

void playlist_row_activated(GtkTreeView *tree, GtkTreePath *tp, GtkTreeViewColumn *col)
{
	gint type = cat_get_selected_browser();
	if(type == -1 || check_connection_state())
	{
		return;
	}
	else if (type == PL3_CURRENT_PLAYLIST)
	{
		GtkTreeIter iter;
		gint song_id;
		gtk_tree_model_get_iter(gtk_tree_view_get_model(tree), &iter, tp);
		gtk_tree_model_get(gtk_tree_view_get_model(tree), &iter, PL3_SONG_ID,&song_id, -1);
		/* send mpd the play command */
		mpd_sendPlayIdCommand (info.connection, song_id);
		mpd_finishCommand (info.connection);
		/* check for errors */                      		
		check_for_errors ();                        		
	}
	else if (type == PL3_BROWSE_FILE)
	{
		GtkTreeIter iter;
		gchar *song_id;
	        gint r_type;
		gtk_tree_model_get_iter(gtk_tree_view_get_model(tree), &iter, tp);
		gtk_tree_model_get(gtk_tree_view_get_model(tree), &iter, PL3_SONG_ID,&song_id, PL3_SONG_POS, &r_type, -1);
		if(song_id == NULL) return;
		if(r_type == PL3_ENTRY_PLAYLIST)
		{	
			mpd_sendLoadCommand(info.connection, song_id);
			mpd_finishCommand(info.connection);
			if(check_for_errors()) return;
		}
		else
		{
			mpd_sendAddCommand(info.connection, song_id);
			mpd_finishCommand(info.connection);
			if(check_for_errors()) return;
		}



	}
}


/* add's the toplevel entry for the current playlist view */
void add_current_playlist()
{
	GtkTreeIter iter;
	gtk_tree_store_append(pl3_tree, &iter, NULL);
	gtk_tree_store_set(pl3_tree, &iter, 
			PL3_CAT_TYPE, PL3_CURRENT_PLAYLIST,
			PL3_CAT_TITLE, "Current Playlist",
			PL3_CAT_INT_ID, "",
			PL3_CAT_ICON_ID, "media-stream",
			PL3_CAT_PROC, TRUE,-1);
}



/* add's the toplevel entry for the file browser, it also add's a fantom child */
void add_file_browser()
{
	GtkTreeIter iter,child;
	gtk_tree_store_append(pl3_tree, &iter, NULL);
	gtk_tree_store_set(pl3_tree, &iter, 
			PL3_CAT_TYPE, PL3_BROWSE_FILE,
			PL3_CAT_TITLE, "Browse Filesystem",
			PL3_CAT_INT_ID, "/",
			PL3_CAT_ICON_ID, "gtk-directory",
			PL3_CAT_PROC, FALSE,-1);
	/* add fantom child for lazy tree */
	gtk_tree_store_append(pl3_tree, &child, &iter);
}


void add_artist_browser()
{
	GtkTreeIter iter,child;
	gtk_tree_store_append(pl3_tree, &iter, NULL);
	gtk_tree_store_set(pl3_tree, &iter, 
			PL3_CAT_TYPE, PL3_BROWSE_ARTIST,
			PL3_CAT_TITLE, "Browse Artists",        	
			PL3_CAT_INT_ID, "",
			PL3_CAT_ICON_ID, "media-artist",
			PL3_CAT_PROC, FALSE,-1);
	/* add fantom child for lazy tree */
	gtk_tree_store_append(pl3_tree, &child, &iter);
}

void view_file_browser_folder(GtkTreeIter *iter_cat)
{
	mpd_InfoEntity *ent = NULL;
	char *path;
	int sub_folder = 0;
	GtkTreeIter iter;
	gtk_tree_model_get(GTK_TREE_MODEL(pl3_tree), iter_cat, 2 , &path, -1);
	/* check the connection state and when its valid proceed */
	if (check_connection_state ())
	{
		return;
	}

	mpd_sendLsInfoCommand (info.connection, path);
	ent = mpd_getNextInfoEntity (info.connection);
	while (ent != NULL)
	{
		if (ent->type == MPD_INFO_ENTITY_TYPE_DIRECTORY)
		{
			sub_folder++;
		}
		else if (ent->type == MPD_INFO_ENTITY_TYPE_SONG)
		{
			gchar buffer[1024];
			strfsong (buffer, 1024, preferences.markup_song_browser,ent->info.song);
			gtk_list_store_append (pl3_store, &iter);
			gtk_list_store_set (pl3_store, &iter,
					0, ent->info.song->file,
					1, PL3_ENTRY_SONG,
					2, buffer,               
					5, "media-audiofile",
					-1);

		}

		else if (ent->type == MPD_INFO_ENTITY_TYPE_PLAYLISTFILE)
		{
			gchar *basename = g_path_get_basename (ent->info.playlistFile->path);
			gtk_list_store_append (pl3_store, &iter);
			gtk_list_store_set (pl3_store, &iter,
					0, ent->info.playlistFile->path,
					1, PL3_ENTRY_PLAYLIST,
					2, basename,
					5, "gtk-index", 
					-1);
			g_free (basename);
		}

		mpd_freeInfoEntity (ent);
		ent = mpd_getNextInfoEntity (info.connection);
	}
	/* remove the fantom child if there are no subfolders anyway. */
	if(!sub_folder)
	{
		if(gtk_tree_model_iter_children(GTK_TREE_MODEL(pl3_tree), &iter, iter_cat))
		{
			gtk_tree_store_remove(pl3_tree, &iter);      		
		}
	}

}

void view_artist_browser_folder(GtkTreeIter *iter_cat)
{
	mpd_InfoEntity *ent = NULL;
	char *artist, *string;
	GtkTreeIter iter;
	gtk_tree_model_get(GTK_TREE_MODEL(pl3_tree), iter_cat, 2 , &artist, 1,&string, -1);
	if (check_connection_state ())
		return;

	if(artist == NULL || string == NULL)
	{
		return;
	}
	if(strlen(artist) == 0)
	{
		/*lowest level, do nothing */
		return;
	}
	if(!strcmp(artist,string))
	{
		int albums = 0;
		/* artist is selected */
		mpd_sendFindCommand (info.connection, MPD_TABLE_ARTIST, artist);
		while ((ent = mpd_getNextInfoEntity (info.connection)) != NULL)
		{
			if (ent->info.song->album == NULL
					|| strlen (ent->info.song->album) == 0)
			{
				gchar buffer[1024];
				strfsong (buffer, 1024, preferences.markup_song_browser,
						ent->info.song);
				if(ent->info.song->file == NULL)
				{
					g_print("crap\n");
				}
				gtk_list_store_append (pl3_store, &iter);
				gtk_list_store_set (pl3_store, &iter,
						2, buffer,
						0, ent->info.song->file,
						5,"media-audiofile",
						-1);
			}
			else albums++;
			mpd_freeInfoEntity (ent);
		}

		if(!albums)
		{
			if(gtk_tree_model_iter_children(GTK_TREE_MODEL(pl3_tree), &iter, iter_cat))
			{
				gtk_tree_store_remove(pl3_tree, &iter);      		
			}
		}                                                                                  		
	}
	else 
	{
		/* artist and album is selected */
		mpd_sendFindCommand (info.connection, MPD_TABLE_ALBUM, string);
		while ((ent = mpd_getNextInfoEntity (info.connection)) != NULL)
		{
			if (ent->info.song->artist!= NULL
					&& !g_utf8_collate (ent->info.song->artist, artist))
			{
				gchar buffer[1024];
				strfsong (buffer, 1024, preferences.markup_song_browser,
						ent->info.song);
				if(ent->info.song->file == NULL)
				{
					g_print("crap\n");
				}
				gtk_list_store_append (pl3_store, &iter);
				gtk_list_store_set (pl3_store, &iter,
						2, buffer,
						0, ent->info.song->file,
						5,"media-audiofile",
						-1);

			}
			mpd_freeInfoEntity (ent);                                       	
		}

	}
}


void artist_browser_fill_tree(GtkTreeIter *iter)
{
	char *artist, *alb_artist;
	GtkTreeIter child,child2;
	gtk_tree_model_get(GTK_TREE_MODEL(pl3_tree),iter, 1, &artist,2,&alb_artist, -1);
	gtk_tree_store_set(pl3_tree, iter, 4, TRUE, -1);

	if (check_connection_state ())
	{
		return;
	}
	if(!strlen(alb_artist))
	{
		/* fill artist list */
		char *string;
		g_print("filling with artists\n");
		mpd_sendListCommand (info.connection, MPD_TABLE_ARTIST, NULL);
		while ((string = mpd_getNextArtist (info.connection)) != NULL)
		{
			gtk_tree_store_append (pl3_tree, &child, iter);
			gtk_tree_store_set (pl3_tree, &child,
					0, PL3_BROWSE_ARTIST,
					1, string, /* the field */
					2, string, /* the artist name, if(1 and 2 together its an artist field) */
					3, "media-artist",
					4, FALSE,
					-1);
			gtk_tree_store_append(pl3_tree, &child2, &child);

		}
		mpd_finishCommand(info.connection);
		if(gtk_tree_model_iter_children(GTK_TREE_MODEL(pl3_tree), &child, iter))
		{
			gtk_tree_store_remove(pl3_tree, &child); 
		}
	}
	/* if where inside a artist */
	else if(!strcmp(artist, alb_artist))
	{
		char *string;
		mpd_sendListCommand (info.connection, MPD_TABLE_ALBUM, artist);
		while ((string = mpd_getNextAlbum (info.connection)) != NULL)
		{
			gtk_tree_store_append (pl3_tree, &child, iter);
			gtk_tree_store_set (pl3_tree, &child,
					0, PL3_BROWSE_ARTIST,
					1, string,
					2, artist,
					3, "media-album", 
					4, TRUE, -1);
			g_free (string);
		}
		mpd_finishCommand (info.connection);
		if(gtk_tree_model_iter_children(GTK_TREE_MODEL(pl3_tree), &child, iter))
		{
			gtk_tree_store_remove(pl3_tree, &child); 
		}
	}
}


void file_browser_fill_tree(GtkTreeIter *iter)
{
	mpd_InfoEntity *ent = NULL;
	char *path;
	GtkTreeIter child,child2;
	gtk_tree_model_get(GTK_TREE_MODEL(pl3_tree),iter, 2, &path, -1);
	gtk_tree_store_set(pl3_tree, iter, 4, TRUE, -1);
	if (check_connection_state ())
	{
		return;
	}



	mpd_sendLsInfoCommand (info.connection, path);

	ent = mpd_getNextInfoEntity (info.connection);
	while (ent != NULL)
	{
		if (ent->type == MPD_INFO_ENTITY_TYPE_DIRECTORY)
		{
			gchar *basename =
				g_path_get_basename (ent->info.directory->path);
			gtk_tree_store_append (pl3_tree, &child, iter);
			gtk_tree_store_set (pl3_tree, &child,
					0, PL3_BROWSE_FILE,
					1, basename,
					2, ent->info.directory->path,
					3, "gtk-open",
					4, FALSE,
					-1);
			gtk_tree_store_append(pl3_tree, &child2, &child);

			g_free (basename);
		}

		mpd_freeInfoEntity (ent);
		ent = mpd_getNextInfoEntity (info.connection);
	}
	if(gtk_tree_model_iter_children(GTK_TREE_MODEL(pl3_tree), &child, iter))
	{
		gtk_tree_store_remove(pl3_tree, &child); 
	}
}


void cat_row_expanded(GtkTreeView *tree, GtkTreeIter *iter, GtkTreePath *path)
{
	gint type,read;
	gtk_tree_model_get(GTK_TREE_MODEL(pl3_tree), iter, 0, &type,4,&read, -1);
	g_print("%i %i\n", type, read);
	if(read) return;
	if(type == PL3_BROWSE_FILE)
	{
		file_browser_fill_tree(iter);
	}
	else if (type == PL3_BROWSE_ARTIST)
	{
		artist_browser_fill_tree(iter);

	}

}


void cat_sel_changed()
{
	GtkTreeModel *model = GTK_TREE_MODEL(pl3_tree);
	GtkTreeIter iter;
	GtkTreeSelection *selec = gtk_tree_view_get_selection((GtkTreeView *)
			glade_xml_get_widget (pl3_xml, "cat_tree"));
	GtkTreeView *tree = (GtkTreeView *) glade_xml_get_widget (pl3_xml, "playlist_tree");
	g_print("Changed\n");

	if(gtk_tree_selection_get_selected(selec,&model, &iter))
	{
		gint type;
		gtk_tree_model_get(model, &iter, 0, &type, -1);
		g_print("type:%i\n", type);
		if(type == PL3_CURRENT_PLAYLIST)
		{
			g_print("adding list\n");
			gtk_tree_view_set_model(tree, GTK_TREE_MODEL(pl2_store));
		}
		else if (type == PL3_BROWSE_FILE)
		{
			g_print("show songs\n");
			gtk_list_store_clear(pl3_store);	
			view_file_browser_folder(&iter);
			gtk_tree_view_set_model(tree, GTK_TREE_MODEL(pl3_store));

		}
		else if (type == PL3_BROWSE_ARTIST)
		{
			g_print("show songs for artist browser\n");
			gtk_list_store_clear(pl3_store);	
			view_artist_browser_folder(&iter);
			gtk_tree_view_set_model(tree, GTK_TREE_MODEL(pl3_store));
		}
		/* when it's not a know type remove the model */
		else
		{
			gtk_tree_view_set_model(tree, NULL);	
		}	

	}
	/* when not connected remove the model */
	else
	{
		gtk_tree_view_set_model(tree, NULL);	
	}
}


void pl3_close()
{
	if(pl3_xml != NULL)
	{

		gtk_widget_hide(glade_xml_get_widget(pl3_xml, "pl3_win"));	
		return;
	}
}

/* create the playlist view 
 * This is done only once, for the rest its hidden, but still there
 */

void create_playlist3 ()
{
	GtkCellRenderer *renderer;
	GtkWidget *tree;
	GtkTreeSelection *sel;
	GtkTreeViewColumn *column = NULL;

	if(pl3_xml != NULL)
	{

		gtk_window_present(GTK_WINDOW(glade_xml_get_widget(pl3_xml, "pl3_win")));
		return;
	}

	/* load gui desciption */
	pl3_xml = glade_xml_new (GLADE_PATH "playlist3.glade", "pl3_win", NULL);
	if(pl3_xml == NULL)
	{
		g_print("Failed to open playlist3.glade.\n");
		return;
	}
	if (pl3_tree == NULL)
	{
		/* song id, song title */
		pl3_tree = gtk_tree_store_new (5, 
				GTK_TYPE_INT,	/* row type, see free_type struct */
				GTK_TYPE_STRING, /* display name */
				GTK_TYPE_STRING,/* full path and stuff for backend */
				GTK_TYPE_STRING,
				GTK_TYPE_BOOL);	/* stock_id*/
	}

	tree = glade_xml_get_widget (pl3_xml, "cat_tree");

	gtk_tree_view_set_model (GTK_TREE_VIEW (tree), GTK_TREE_MODEL (pl3_tree));



	/* draw the column with the songs */
	renderer = gtk_cell_renderer_pixbuf_new ();
	column = gtk_tree_view_column_new ();
	gtk_tree_view_column_pack_start (column, renderer, FALSE);
	gtk_tree_view_column_set_attributes (column,
			renderer,
			"stock-id",3, NULL);


	renderer = gtk_cell_renderer_text_new ();

	/* insert the column in the tree */
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_set_attributes (column,
			renderer,
			"text", 1,
			NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);
	gtk_tree_view_set_search_column(GTK_TREE_VIEW(tree), 1);
	sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));
	g_signal_connect(G_OBJECT(sel), "changed", G_CALLBACK(cat_sel_changed), NULL);


	/* right column */
	tree = glade_xml_get_widget (pl3_xml, "playlist_tree");
	gtk_tree_view_set_search_column(GTK_TREE_VIEW(tree), 2);

	pl3_store = gtk_list_store_new (6, 
			GTK_TYPE_STRING,	/* song path */
			GTK_TYPE_INT,	/* pos id */
			GTK_TYPE_STRING,	/* song title */
			GTK_TYPE_INT,	/* color string */
			G_TYPE_BOOLEAN,
			GTK_TYPE_STRING);	/* stock id */

	renderer = gtk_cell_renderer_pixbuf_new ();
	column = gtk_tree_view_column_new ();
	gtk_tree_view_column_pack_start (column, renderer, FALSE);
	gtk_tree_view_column_set_attributes (column,
			renderer,
			"stock-id", SONG_STOCK_ID, NULL);

	renderer = gtk_cell_renderer_text_new ();

	/* insert the column in the tree */
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_set_attributes (column,
			renderer,
			"text", SONG_TITLE,
			"weight", WEIGHT_INT,
			"weight-set", WEIGHT_ENABLE, NULL);

	gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);

	/* add the current playlist */
	add_current_playlist();
	add_file_browser();
	add_artist_browser();

	/* add the file browser */

	gtk_widget_show_all(glade_xml_get_widget(pl3_xml, "pl3_win"));

	/* connect signals that are defined in the gui description */
	glade_xml_signal_autoconnect (pl3_xml);
}





