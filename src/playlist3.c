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


void add_current_playlist()
{
	GtkTreeIter iter;
	gtk_tree_store_append(pl3_tree, &iter, NULL);
	gtk_tree_store_set(pl3_tree, &iter, 
			0, PL3_CURRENT_PLAYLIST,
			1, "Current Playlist",
			2, "",
			3, "media-stream",
			4, TRUE,-1);
}

void add_file_browser()
{
	GtkTreeIter iter,child;
	gtk_tree_store_append(pl3_tree, &iter, NULL);
	gtk_tree_store_set(pl3_tree, &iter, 
			0, PL3_BROWSE_FILE,
			1, "Browse Filesystem",
			2, "/",
			3, "gtk-directory",
			4, FALSE,-1);
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
	if (check_connection_state ())
 		return;
 
//	gtk_list_store_clear(pl3_store); 
 
	g_print("path songs: %s\n", path);	
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
			strfsong (buffer, 1024, preferences.markup_song_browser,
					ent->info.song);
			gtk_list_store_append (pl3_store, &iter);
			gtk_list_store_set (pl3_store, &iter,
					0, ent->info.song->file,
					2, buffer,               
					5, "media-audiofile", -1);

		}

		else if (ent->type == MPD_INFO_ENTITY_TYPE_PLAYLISTFILE)
		{
			gchar *basename =
				g_path_get_basename (ent->info.playlistFile->path);
			gtk_list_store_append (pl3_store, &iter);
			gtk_list_store_set (pl3_store, &iter,
					0, ent->info.playlistFile->path,
					2, basename,
					5, "gtk-index", -1);
			g_free (basename);
		}

		mpd_freeInfoEntity (ent);
		ent = mpd_getNextInfoEntity (info.connection);
	}
	if(!sub_folder)
	{
		if(gtk_tree_model_iter_children(GTK_TREE_MODEL(pl3_tree), &iter, iter_cat))
		{
			gtk_tree_store_remove(pl3_tree, &iter);      		
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
	g_print("path: %s\n", path);
	/* if there is no child, this should _never happend_ (well it cna't) but to be sure */
	if (check_connection_state ())
		return;



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

}


void cat_sel_changed(GtkTreeSelection *sel)
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
			gtk_tree_view_set_model(tree, pl2_store);
		}
		else if (type == PL3_BROWSE_FILE)
		{
			g_print("show songs\n");
			gtk_list_store_clear(pl3_store);	
			view_file_browser_folder(&iter);
			gtk_tree_view_set_model(tree, pl3_store);

		}
		else
		{
			gtk_tree_view_set_model(tree, NULL);	
		}	

	}
	else
	{
		gtk_tree_view_set_model(tree, NULL);	
	}
}

/* create the playlist view 
 * This is done only once, for the rest its hidden, but still there
 */

void create_playlist3 ()
{
	GtkCellRenderer *renderer;
	GtkWidget *tree;
	GtkTargetEntry target;
	GtkTreeViewColumn *column = NULL;

	/* load gui desciption */
	pl3_xml = glade_xml_new (GLADE_PATH "playlist3.glade", "pl3_win", NULL);
	if(pl3_xml == NULL)
	{
		g_print("Failed to open playlist3.glade.\n");
		return;
	}
	/* obsolete, but cant hurt either */
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
	gtk_tree_view_set_search_column(GTK_TREE_VIEW(tree), 1);


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

	GtkTreeSelection *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));
	g_signal_connect(G_OBJECT(sel), "changed", G_CALLBACK(cat_sel_changed), NULL);


	/* right column */
	tree = glade_xml_get_widget (pl3_xml, "playlist_tree");


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

	/* add the file browser */

	gtk_widget_show_all(glade_xml_get_widget(pl3_xml, "pl3_win"));

	/* connect signals that are defined in the gui description */
	glade_xml_signal_autoconnect (pl3_xml);
}






























