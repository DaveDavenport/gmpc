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


/****************************************************************/
/* We want to move this to mpdinteraction 			*/
/****************************************************************/
void pl3_clear_playlist()
{
	if(check_connection_state()) return;
	mpd_sendClearCommand(info.connection);
	mpd_finishCommand(info.connection);
}








/********************************************************************
 * Misc functions 
 */
gchar * format_time(unsigned long seconds)
{
	int days = seconds/86400;
	int houres = (seconds % 86400)/3600;
	int minutes = (seconds % 3600)/60;
	char *ret;
	GString *str = g_string_new("Total time: ");
	if(days != 0)
	{
		g_string_append_printf(str, "%i days ", days);
	}	
	if(houres != 0)
	{
		g_string_append_printf(str, "%i hours ", houres);
	}
	if(minutes != 0)
	{
		g_string_append_printf(str, "%i minutes ", minutes);
	}                                                         	
	if(seconds == 0)
	{
		g_string_append(str, "0");
	}
	ret = str->str;
	g_string_free(str, FALSE);
	return ret;
}


/* Get the type of the selected row.. 
 * -1 means no row selected 
 */
int  pl3_cat_get_selected_browser()
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


/*****************************************************************
 * CURRENT BROWSER
 */


/* add's the toplevel entry for the current playlist view */
void pl3_current_playlist_add()
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










/********************************************************
 * FILE BROWSER 				  	*
 */
void pl3_browse_file_add_folder()
{
	GtkTreeSelection *selec = gtk_tree_view_get_selection((GtkTreeView *)glade_xml_get_widget (pl3_xml, "cat_tree"));
	GtkTreeModel *model = GTK_TREE_MODEL(pl3_tree);
	GtkTreeIter iter;

	if(check_connection_state())
	{
		return;
	}
	if(gtk_tree_selection_get_selected(selec,&model, &iter))
	{
		char *path;
		gtk_tree_model_get(model, &iter, PL3_CAT_INT_ID, &path, -1);
		mpd_sendAddCommand(info.connection, path);
		mpd_finishCommand(info.connection);
		check_for_errors();
	}
}


void pl3_browse_file_replace_folder()
{
	pl3_clear_playlist();
	pl3_browse_file_add_folder();	
}


/* add's the toplevel entry for the file browser, it also add's a fantom child */
void pl3_file_browser_add()
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

long unsigned pl3_file_browser_view_folder(GtkTreeIter *iter_cat)
{
	mpd_InfoEntity *ent = NULL;
	char *path;
	int sub_folder = 0;
	GtkTreeIter iter;
	long  unsigned time=0;
	gtk_tree_model_get(GTK_TREE_MODEL(pl3_tree), iter_cat, 2 , &path, -1);
	/* check the connection state and when its valid proceed */
	if (check_connection_state ())
	{
		return 0;
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
			if(ent->info.song->time != MPD_SONG_NO_TIME)
			{
				time += ent->info.song->time;			
			}

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
	return time;
}


void pl3_file_browser_fill_tree(GtkTreeIter *iter)
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


/***********************************************************************
 *  ARTIST BROWSER
 */



void pl3_artist_browser_add()
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


long unsigned pl3_artist_browser_view_folder(GtkTreeIter *iter_cat)
{
	mpd_InfoEntity *ent = NULL;
	char *artist, *string;
	GtkTreeIter iter;
	long unsigned time =0;
	gtk_tree_model_get(GTK_TREE_MODEL(pl3_tree), iter_cat, 2 , &artist, 1,&string, -1);
	if (check_connection_state ())
		return 0;

	if(artist == NULL || string == NULL)
	{
		return 0;
	}
	if(strlen(artist) == 0)
	{
		/*lowest level, do nothing */
		return 0;
	}
	if(!g_utf8_collate(artist,string))
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
				if(ent->info.song->time != MPD_SONG_NO_TIME)
				{
					time += ent->info.song->time;
				}
				if(ent->info.song->file == NULL)
				{
					g_print("crap\n");
				}
				gtk_list_store_append (pl3_store, &iter);
				gtk_list_store_set (pl3_store, &iter,
						2, buffer,
						0, ent->info.song->file,
						1, PL3_ENTRY_SONG,
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
				if(ent->info.song->time != MPD_SONG_NO_TIME)
				{
					time += ent->info.song->time;
				}
				if(ent->info.song->file == NULL)
				{
					g_print("crap\n");
				}
				gtk_list_store_append (pl3_store, &iter);
				gtk_list_store_set (pl3_store, &iter,
						2, buffer,
						0, ent->info.song->file,
						1, PL3_ENTRY_SONG,
						5,"media-audiofile",
						-1);

			}
			mpd_freeInfoEntity (ent);                                       	
		}

	}
	return time;
}


void pl3_artist_browser_fill_tree(GtkTreeIter *iter)
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
	else if(!g_utf8_collate(artist, alb_artist))
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



void pl3_browse_artist_add_folder()
{
	GtkTreeSelection *selec = gtk_tree_view_get_selection((GtkTreeView *)glade_xml_get_widget (pl3_xml, "cat_tree"));
	GtkTreeModel *model = GTK_TREE_MODEL(pl3_tree);
	GtkTreeIter iter;

	if(check_connection_state())
	{
		return;
	}
	if(gtk_tree_selection_get_selected(selec,&model, &iter))
	{
		char *artist, *title;
		mpd_InfoEntity *ent = NULL;
		gtk_tree_model_get(model, &iter, PL3_CAT_INT_ID, &artist, PL3_CAT_TITLE,&title, -1);
		GList *add_list = NULL;
		if(!artist || !title)
		{
			return;
		}
		if(strlen(artist) == 0)
		{
			/*toplevel node */
			/* do nothing here, what is there todo? */
		}
		else if(!g_utf8_collate(artist,title))
		{
			/* artist selected */
			mpd_sendFindCommand (info.connection, MPD_TABLE_ARTIST, artist);
			while ((ent = mpd_getNextInfoEntity (info.connection)) != NULL)
			{                                                                         			
				add_list = g_list_append (add_list, g_strdup (ent->info.song->file));
				mpd_freeInfoEntity (ent);
			}
			mpd_finishCommand (info.connection);

		}
		else
		{
			/* album selected */
			/* fetch all songs by this album and check if the artist is right. from mpd and add them to the add-list */
			mpd_sendFindCommand (info.connection, MPD_TABLE_ALBUM, title);
			while ((ent = mpd_getNextInfoEntity (info.connection)) != NULL)
			{
				if (!g_utf8_collate (ent->info.song->artist, artist))
				{
					add_list = g_list_append (add_list, g_strdup (ent->info.song->file));
				}
				mpd_freeInfoEntity (ent);
			}
			mpd_finishCommand (info.connection);

		}

		/* if there are items in the add list add them to the playlist */
		if (check_connection_state ())
			return;
		if (add_list != NULL)
		{
			GList *song;
			mpd_sendCommandListBegin (info.connection);
			song = g_list_first (add_list);
			do
			{
				mpd_sendAddCommand (info.connection, song->data);
			}
			while ((song = g_list_next (song)) != NULL);
			mpd_sendCommandListEnd (info.connection);
			mpd_finishCommand (info.connection);
			check_for_errors ();
			g_list_foreach (add_list, (GFunc) g_free, NULL);
			g_list_free (add_list);
		}
	}

}

void pl3_browse_artist_replace_folder()
{
	pl3_clear_playlist();
	pl3_browse_artist_add_folder();
}



/**************************************************
 * PLaylist Tree
 */



void pl3_playlist_row_activated(GtkTreeView *tree, GtkTreePath *tp, GtkTreeViewColumn *col)
{
	gint type = pl3_cat_get_selected_browser();
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
	else if (type == PL3_BROWSE_FILE || type == PL3_BROWSE_ARTIST)
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







/**************************************************
 * Category Tree 
 */


void pl3_cat_row_activated(GtkTreeView *tree, GtkTreePath *tp, GtkTreeViewColumn *col)
{
	gint type = pl3_cat_get_selected_browser();
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


void pl3_cat_row_expanded(GtkTreeView *tree, GtkTreeIter *iter, GtkTreePath *path)
{
	gint type,read;
	gtk_tree_model_get(GTK_TREE_MODEL(pl3_tree), iter, 0, &type,4,&read, -1);
	if(read) return;
	if(type == PL3_BROWSE_FILE)
	{
		pl3_file_browser_fill_tree(iter);
	}
	else if (type == PL3_BROWSE_ARTIST)
	{
		pl3_artist_browser_fill_tree(iter);

	}

}


void pl3_cat_sel_changed()
{
	GtkTreeModel *model = GTK_TREE_MODEL(pl3_tree);
	GtkTreeIter iter;
	GtkTreeSelection *selec = gtk_tree_view_get_selection((GtkTreeView *)
			glade_xml_get_widget (pl3_xml, "cat_tree"));
	GtkTreeView *tree = (GtkTreeView *) glade_xml_get_widget (pl3_xml, "playlist_tree");
	gtk_statusbar_pop(GTK_STATUSBAR(glade_xml_get_widget(pl3_xml, "statusbar2")), 0);

	if(gtk_tree_selection_get_selected(selec,&model, &iter))
	{
		gint type;
		gtk_tree_model_get(model, &iter, 0, &type, -1);
		if(type == PL3_CURRENT_PLAYLIST)
		{
			if(info.stats != NULL)
			{	
				gchar *string = format_time(info.playlist_playtime);
				gtk_statusbar_push(GTK_STATUSBAR(glade_xml_get_widget(pl3_xml, "statusbar2")),0, string);
				g_free(string);
			}
			gtk_tree_view_set_model(tree, GTK_TREE_MODEL(pl2_store));
		}
		else if (type == PL3_BROWSE_FILE)
		{
			long unsigned time= 0;
			gchar *string;
			gtk_list_store_clear(pl3_store);	
			time = pl3_file_browser_view_folder(&iter);
			gtk_tree_view_set_model(tree, GTK_TREE_MODEL(pl3_store));
			string = format_time(time);
			gtk_statusbar_push(GTK_STATUSBAR(glade_xml_get_widget(pl3_xml, "statusbar2")),0, string);
			g_free(string);
		}
		else if (type == PL3_BROWSE_ARTIST)
		{
			long unsigned time= 0;
			gchar *string;        			
			gtk_list_store_clear(pl3_store);	
			time = pl3_artist_browser_view_folder(&iter);
			gtk_tree_view_set_model(tree, GTK_TREE_MODEL(pl3_store));
			string = format_time(time);
			gtk_statusbar_push(GTK_STATUSBAR(glade_xml_get_widget(pl3_xml, "statusbar2")),0, string);
			g_free(string);
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
	/* gtk seems to want this when model changes  */
	/* so we do it */
	gtk_tree_view_set_search_column(GTK_TREE_VIEW(tree), 2);
}

void pl3_update()
{
	if(pl3_xml == NULL|| info.status == NULL)
	{
		return;
	}
	if(info.playlist_id != info.status->playlist)
	{
		gint type = pl3_cat_get_selected_browser();
		if(type == PL3_CURRENT_PLAYLIST)
		{
			gchar *string = format_time(info.playlist_playtime);
			gtk_statusbar_push(GTK_STATUSBAR(glade_xml_get_widget(pl3_xml, "statusbar2")),0, string);
			g_free(string);


		}
	}
}


/* handle right mouse clicks on the cat tree view */
/* gonna be a big function*/
int pl3_cat_tree_button_press_event(GtkTreeView *tree, GdkEventButton *event)
{
	gint type  = pl3_cat_get_selected_browser();
	if(type == -1)
	{
		/* no selections, or no usefull one.. so propagate the signal */
		return FALSE;
	}

	if(event->button != 3)
	{
		/* if its not the right mouse button propagate the signal */
		return FALSE;
	}
	/* if it's the current playlist */
	if(type == PL3_CURRENT_PLAYLIST)
	{
		/* here we have:  Save, Clear*/
		GtkWidget *item;
		GtkWidget *menu = gtk_menu_new();	
		/* add the save widget */
		item = gtk_image_menu_item_new_from_stock(GTK_STOCK_SAVE,NULL);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
		/* TODO: Write own fun ction */
		g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(pl2_save_playlist), NULL);
		/* add the clear widget */
		item = gtk_image_menu_item_new_from_stock(GTK_STOCK_CLEAR,NULL);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
		g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(pl3_clear_playlist), NULL);

		/* show everything and popup */
		gtk_widget_show_all(menu);
		gtk_menu_popup(GTK_MENU(menu), NULL, NULL,NULL, NULL, event->button, event->time);
	}
	else if (type == PL3_BROWSE_FILE)
	{
		/* here we have:  Add. Replace, (update?)*/
		GtkWidget *item;
		GtkWidget *menu = gtk_menu_new();	
		/* add the add widget */
		item = gtk_image_menu_item_new_from_stock(GTK_STOCK_ADD,NULL);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
		g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(pl3_browse_file_add_folder), NULL);		

		/* add the replace widget */
		item = gtk_image_menu_item_new_with_label("Replace");
		gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
				gtk_image_new_from_stock(GTK_STOCK_REDO, GTK_ICON_SIZE_MENU));
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
		g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(pl3_browse_file_replace_folder), NULL);				

		/* show everything and popup */
		gtk_widget_show_all(menu);                                                        		
		gtk_menu_popup(GTK_MENU(menu), NULL, NULL,NULL, NULL, event->button, event->time);

	}
	else if (type == PL3_BROWSE_ARTIST)
	{
		/* here we have:  Add. Replace*/
		GtkWidget *item;
		GtkWidget *menu = gtk_menu_new();	
		/* add the add widget */
		item = gtk_image_menu_item_new_from_stock(GTK_STOCK_ADD,NULL);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
		g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(pl3_browse_artist_add_folder), NULL);		

		/* add the replace widget */
		item = gtk_image_menu_item_new_with_label("Replace");
		gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
				gtk_image_new_from_stock(GTK_STOCK_REDO, GTK_ICON_SIZE_MENU));
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
		g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(pl3_browse_artist_replace_folder), NULL);				

		/* show everything and popup */
		gtk_widget_show_all(menu);                                                        		
		gtk_menu_popup(GTK_MENU(menu), NULL, NULL,NULL, NULL, event->button, event->time);
	}

	return TRUE;
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
	g_signal_connect(G_OBJECT(sel), "changed", G_CALLBACK(pl3_cat_sel_changed), NULL);


	/* right column */
	tree = glade_xml_get_widget (pl3_xml, "playlist_tree");
	gtk_tree_view_set_search_column(GTK_TREE_VIEW(tree), 2);

	gtk_tree_selection_set_mode (GTK_TREE_SELECTION(gtk_tree_view_get_selection
				(GTK_TREE_VIEW (tree))),
			GTK_SELECTION_MULTIPLE);


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
	pl3_current_playlist_add();
	pl3_file_browser_add();
	pl3_artist_browser_add();

	/* add the file browser */

	gtk_widget_show_all(glade_xml_get_widget(pl3_xml, "pl3_win"));

	/* connect signals that are defined in the gui description */
	glade_xml_signal_autoconnect (pl3_xml);
}





