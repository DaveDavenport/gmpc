#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <stdlib.h>
#include <string.h>
#include <glade/glade.h>
#include <time.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include "libmpdclient.h"
#include "strfsong.h"
#include "main.h"
#include "misc.h"
#include "playlist2.h"
#include "playlist3.h"
#include "song-browser.h"
#include "open-location.h"

GladeXML *pl3_xml = NULL;
GtkTreeStore *pl3_tree = NULL;
GtkListStore *pl3_store = NULL;
extern GtkTreeModel *pl2_store;

/****************************************************************/
/* We want to move this to mpdinteraction 			*/
/****************************************************************/
void pl3_clear_playlist()
{
	if(check_connection_state()) return;
	mpd_sendClearCommand(info.connection);
	mpd_finishCommand(info.connection);
}

void pl3_shuffle_playlist()
{
	if(check_connection_state()) return;
	mpd_sendShuffleCommand(info.connection);
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
		if(type >= PL3_NTYPES || type < 0)
		{
			return -1;
		}
		return type;
	}
	return -1;
}

/************************************
 * XIPH BROWSER
 */

void pl3_xiph_add()
{
	GtkTreeIter iter;
	gtk_tree_store_append(pl3_tree, &iter, NULL);
	gtk_tree_store_set(pl3_tree, &iter, 
			PL3_CAT_TYPE, PL3_BROWSE_XIPH,
			PL3_CAT_TITLE, "Icecast",
			PL3_CAT_INT_ID, "",
			PL3_CAT_ICON_ID, "icecast",
			PL3_CAT_PROC, FALSE,          	
			PL3_CAT_ICON_SIZE,3,-1);
}

void pl3_xiph_fill_view(char *buffer)
{
	xmlDocPtr xmldoc = xmlParseMemory(buffer, strlen(buffer));
	xmlNodePtr root = xmlDocGetRootElement(xmldoc);
	xmlNodePtr cur = root->xmlChildrenNode;
	while(cur != NULL)
	{
		if(xmlStrEqual(cur->name, "entry"))
		{
			xmlNodePtr cur1 = cur->xmlChildrenNode;
			GtkTreeIter iter;
			char *name=NULL, *bitrate=NULL, *genre=NULL;
			gtk_list_store_append(pl3_store, &iter);
			gtk_list_store_set (pl3_store, &iter,
					PL3_SONG_POS, PL3_ENTRY_SONG, 
					PL3_SONG_STOCK_ID, "media-stream", 
					-1);

			while(cur1 != NULL)
			{
				if(xmlStrEqual(cur1->name, "server_name"))
				{
			//		gtk_list_store_set(pl3_store, &iter, PL3_SONG_TITLE, xmlNodeGetContent(cur1), -1);
					name = xmlNodeGetContent(cur1);
				}
				else if(xmlStrEqual(cur1->name, "genre"))
				{
					genre = xmlNodeGetContent(cur1);
				}
				else if (xmlStrEqual(cur1->name,"bitrate"))
				{
					bitrate = xmlNodeGetContent(cur1);
				}
				else if(xmlStrEqual(cur1->name, "listen_url"))
				{
					gtk_list_store_set(pl3_store, &iter, PL3_SONG_ID, xmlNodeGetContent(cur1), -1);

				}

				cur1 = cur1->next;
			}
			name = g_strdup_printf("Station: %s\nGenre: %s\nBitrate: %s", name,genre, bitrate);
			gtk_list_store_set(pl3_store, &iter, PL3_SONG_TITLE, name, -1);
			g_free(name);

		}

		cur = cur->next;
	}
	xmlFreeDoc(xmldoc);
	xmlCleanupParser();
}

void pl3_xiph_view_browser()
{
	gtk_list_store_clear(pl3_store);
	start_transfer("http://dir.xiph.org/yp.xml",(void *)pl3_xiph_fill_view, NULL, glade_xml_get_widget(pl3_xml, "pl3_win"));
}

/*****************************************************************
 * Find Browser
 */
/* add's the toplevel entry for the current playlist view */
void pl3_find_add()
{
	GtkTreeIter iter;
	gtk_tree_store_append(pl3_tree, &iter, NULL);
	gtk_tree_store_set(pl3_tree, &iter, 
			PL3_CAT_TYPE, PL3_FIND,
			PL3_CAT_TITLE, "Search",
			PL3_CAT_INT_ID, "",
			PL3_CAT_ICON_ID, "gtk-find",
			PL3_CAT_PROC, TRUE,
			PL3_CAT_ICON_SIZE,3,-1);
}

unsigned long pl3_find_view_browser()
{
	GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW(glade_xml_get_widget (pl3_xml, "cat_tree")));
	GtkTreeModel *model = GTK_TREE_MODEL(pl3_tree);
	GtkTreeIter iter;
	int time=0;

	if(gtk_tree_selection_get_selected(selection,&model, &iter))
	{
		gchar *name, *field;
		gint num_field=0;
		GtkTreeIter child;
		mpd_InfoEntity *ent;

		gtk_tree_model_get(model, &iter, PL3_CAT_TITLE, &name, PL3_CAT_INT_ID,&field,-1);

		num_field = atoi(field);



		if(strcmp(name, "Search"))
		{
			gtk_entry_set_text(GTK_ENTRY(glade_xml_get_widget(pl3_xml, "search_entry")), name);
			gtk_combo_box_set_active(GTK_COMBO_BOX(glade_xml_get_widget(pl3_xml, "cb_field_selector")), num_field);

			/* do the actual search */
			mpd_sendSearchCommand (info.connection, num_field, name);

			while ((ent = mpd_getNextInfoEntity (info.connection)) != NULL)
			{
				gchar buffer[1024];
				if(ent->info.song->time != MPD_SONG_NO_TIME)
				{
					time += ent->info.song->time;
				}
				strfsong (buffer, 1024, preferences.markup_song_browser,
						ent->info.song);
				/* add as child of the above created parent folder */
				gtk_list_store_append (pl3_store, &child);
				gtk_list_store_set (pl3_store, &child,
						PL3_SONG_ID, ent->info.song->file,
						PL3_SONG_TITLE, buffer,
						PL3_SONG_POS, PL3_ENTRY_SONG, 
						PL3_SONG_STOCK_ID, "media-audiofile", 
						-1);
				/* free information struct */
				mpd_freeInfoEntity (ent);
			}
			/* check search */
			mpd_finishCommand (info.connection);
			check_for_errors ();
		}
	}
	return time;
}

void pl3_find_search()
{
	GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW(glade_xml_get_widget (pl3_xml, "cat_tree")));
	GtkTreeModel *model = GTK_TREE_MODEL(pl3_tree);
	GtkTreeIter iter,child,tst;
	GtkTreePath *path;
	const char *name;
	gchar *field;
	if(!gtk_tree_selection_get_selected(selection, &model, &iter)) return;
	name = gtk_entry_get_text(GTK_ENTRY(glade_xml_get_widget(pl3_xml, "search_entry")));
	field = g_strdup_printf("%i", gtk_combo_box_get_active(GTK_COMBO_BOX(glade_xml_get_widget(pl3_xml, "cb_field_selector"))));

	if(gtk_tree_model_iter_parent(model, &tst, &iter))
	{
		gtk_tree_store_append(pl3_tree, &child,&tst);
	}
	else
	{
		gtk_tree_store_append(pl3_tree, &child,&iter);
	}

	gtk_tree_store_set(pl3_tree, &child,
			PL3_CAT_TYPE, PL3_FIND,
			PL3_CAT_TITLE, name,
			PL3_CAT_INT_ID, field,
			PL3_CAT_ICON_ID, "gtk-find",
			PL3_CAT_PROC, TRUE,
			PL3_CAT_ICON_SIZE,1,
			-1);
	path = gtk_tree_model_get_path(model,&iter);
	gtk_tree_view_expand_to_path(GTK_TREE_VIEW(glade_xml_get_widget(pl3_xml, "cat_tree")), path);
	gtk_tree_path_free(path);

	gtk_tree_selection_select_iter(selection, &child);


	g_free(field);
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
			PL3_CAT_PROC, TRUE,
			PL3_CAT_ICON_SIZE,3,
			-1);
}

/* delete all selected songs,
 * if no songs select ask the user if he want's to clear the list 
 */
void pl3_current_playlist_delete_selected_songs ()
{
	/* grab the selection from the tree */
	GtkTreeSelection *selection =
		gtk_tree_view_get_selection (GTK_TREE_VIEW
				(glade_xml_get_widget (pl3_xml, "playlist_tree")));
	/* check if where connected */
	if (check_connection_state ())
		return;
	/* see if there is a row selected */
	if (gtk_tree_selection_count_selected_rows (selection) > 0)
	{
		GList *list = NULL, *llist = NULL;
		/* start a command list */
		mpd_sendCommandListBegin (info.connection);
		/* grab the selected songs */
		list = gtk_tree_selection_get_selected_rows (selection, &pl2_store);
		/* grab the last song that is selected */
		llist = g_list_last (list);
		/* remove every selected song one by one */
		do
		{
			GtkTreeIter iter;
			int value;
			gtk_tree_model_get_iter (pl2_store, &iter,
					(GtkTreePath *) llist->data);
			gtk_tree_model_get (pl2_store, &iter, SONG_ID, &value, -1);
			mpd_sendDeleteIdCommand (info.connection, value);
		}
		while ((llist = g_list_previous (llist)));

		/* close the list, so it will be executed */
		mpd_sendCommandListEnd (info.connection);
		mpd_finishCommand (info.connection);
		/* free list */
		g_list_foreach (list, (GFunc) gtk_tree_path_free, NULL);
		g_list_free (list);

		check_for_errors ();
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
					_
					("Are you sure you want to clear the playlist?"));
		gtk_dialog_add_buttons (GTK_DIALOG (dialog), GTK_STOCK_CANCEL,
				GTK_RESPONSE_CANCEL, GTK_STOCK_OK,
				GTK_RESPONSE_OK, NULL);
		gtk_dialog_set_default_response (GTK_DIALOG (dialog),
				GTK_RESPONSE_CANCEL);

		switch (gtk_dialog_run (GTK_DIALOG (dialog)))
		{
			case GTK_RESPONSE_OK:
				/* check if where still connected */
				/* TODO: Replace by default clear function  */
				if (!check_connection_state ())
				{
					/* clear the playlist */
					mpd_sendClearCommand (info.connection);
					mpd_finishCommand (info.connection);
					check_for_errors ();
				}
		}
		gtk_widget_destroy (GTK_WIDGET (dialog));
	}
	/* update everything if where still connected */
	gtk_tree_selection_unselect_all(selection);
	if (!check_connection_state ())
		main_trigger_update ();	
}




void pl3_current_playlist_crop_selected_songs()
{
	/* grab the selection from the tree */
	GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW(glade_xml_get_widget (pl3_xml, "playlist_tree")));

	/* check if where connected */
	if (check_connection_state ())
		return;
	/* see if there is a row selected */
	if (gtk_tree_selection_count_selected_rows (selection) > 0)
	{
		GtkTreeIter iter;
		/* start a command list */
		mpd_sendCommandListBegin (info.connection);
		/* remove every selected song one by one */
		gtk_tree_model_get_iter_first(GTK_TREE_MODEL(pl2_store), &iter);

		do
		{
			int value;
			if(!gtk_tree_selection_iter_is_selected(selection, &iter))
			{
				gtk_tree_model_get (GTK_TREE_MODEL(pl2_store), &iter, SONG_ID, &value, -1);
				mpd_sendDeleteIdCommand (info.connection, value);
			}
		}
		while (gtk_tree_model_iter_next(GTK_TREE_MODEL(pl2_store),&iter));

		/* close the list, so it will be executed */
		mpd_sendCommandListEnd (info.connection);
		mpd_finishCommand (info.connection);

		check_for_errors ();
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
					_
					("Are you sure you want to clear the playlist?"));
		gtk_dialog_add_buttons (GTK_DIALOG (dialog), GTK_STOCK_CANCEL,
				GTK_RESPONSE_CANCEL, GTK_STOCK_OK,
				GTK_RESPONSE_OK, NULL);
		gtk_dialog_set_default_response (GTK_DIALOG (dialog),
				GTK_RESPONSE_CANCEL);

		switch (gtk_dialog_run (GTK_DIALOG (dialog)))
		{
			case GTK_RESPONSE_OK:
				/* check if where still connected */
				/* TODO: Replace by default clear function  */
				if (!check_connection_state ())
				{
					pl3_clear_playlist();
					/* clear the playlist */
					/*					mpd_sendClearCommand (info.connection);
										mpd_finishCommand (info.connection);
										check_for_errors ();                              	
										*/				}
		}                                                         
		gtk_widget_destroy (GTK_WIDGET (dialog));
	}
	/* update everything if where still connected */
	gtk_tree_selection_unselect_all(selection);
	if (!check_connection_state ())
		main_trigger_update ();	
}


void pl3_show_song_info ()
{
	int i = 0;
	GtkTreeModel *model = GTK_TREE_MODEL (pl2_store);
	/* get the tree selection object */
	GtkTreeSelection *selection =
		gtk_tree_view_get_selection (GTK_TREE_VIEW
				(glade_xml_get_widget (pl3_xml, "playlist_tree")));
	/* check if there are selected rows */
	if ((i = gtk_tree_selection_count_selected_rows (selection)) > 0)
	{
		GList *list = NULL;
		list = gtk_tree_selection_get_selected_rows (selection, &model);
		/* iterate over every row */
		list = g_list_last (list);
		do
		{
			GtkTreeIter iter;
			int value;
			gtk_tree_model_get_iter (model, &iter, (GtkTreePath *) list->data);
			gtk_tree_model_get (model, &iter, SONG_POS, &value, -1);
			/* show the info for this song  */
			call_id3_window (value);
			/* go to previous song if still connected */
		}
		while ((list = g_list_previous (list)) && !check_connection_state ());
		/* free list */
		g_list_foreach (list, (GFunc) gtk_tree_path_free, NULL);
		g_list_free (list);
	}
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
		char *message = NULL;
		gtk_tree_model_get(model, &iter, PL3_CAT_INT_ID, &path, -1);
		message = g_strdup_printf("Added folder '%s' recursively", path);
		pl3_push_statusbar_message(message);
		g_free(message);
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
			PL3_CAT_ICON_ID, "gtk-open",
			PL3_CAT_PROC, FALSE,
			PL3_CAT_ICON_SIZE,3,-1);
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
					5, "media-playlist", 
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
					PL3_CAT_ICON_SIZE,1,
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
/*******************************************************
 * Combined functions 
 */



void pl3_browse_add_selected()
{
	GtkTreeIter iter;
	GtkTreeView *tree = GTK_TREE_VIEW(glade_xml_get_widget (pl3_xml, "playlist_tree"));
	GtkTreeSelection *selection = gtk_tree_view_get_selection (tree);
	GtkTreeModel *model = GTK_TREE_MODEL (pl3_store);
	GList *rows = gtk_tree_selection_get_selected_rows (selection, &model);
	GList *add_list = NULL;
	int songs=0;
	gchar *message;


	if(rows != NULL)
	{
		gchar *name;
		gint type;
		GList *node = g_list_first(rows);
		do
		{
			GtkTreePath *path = node->data;
			gtk_tree_model_get_iter (GTK_TREE_MODEL (pl3_store), &iter, path);
			gtk_tree_model_get (GTK_TREE_MODEL (pl3_store), &iter, PL3_SONG_ID, &name, 1, &type, -1);	  
			if(type == PL3_ENTRY_SONG)
			{
				/* add them to the add list */
				add_list = g_list_append (add_list, g_strdup (name));

			}
			else if (type == PL3_ENTRY_PLAYLIST)
			{
				message = g_strdup_printf("Added playlist '%s'", name);
				pl3_push_statusbar_message(message);
				g_free(message);



				/* append a playlist directly, we cant add it to the todo list */
				mpd_sendLoadCommand (info.connection, name);
				mpd_finishCommand (info.connection);
				/* check for errors, and if there is an error step out this while loop */
				if (check_for_errors ())
				{
					node = NULL;
				}
			}
		}while((node = g_list_next(node)) != NULL);
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
			songs++;
		}
		while ((song = g_list_next (song)) != NULL);
		mpd_sendCommandListEnd (info.connection);
		mpd_finishCommand (info.connection);
		check_for_errors ();
		g_list_foreach (add_list, (GFunc) g_free, NULL);
		g_list_free (add_list);
	}
	if(songs != 0)
	{
		message = g_strdup_printf("Added %i songs", songs);
		pl3_push_statusbar_message(message);
		g_free(message);                                       	
	}

	g_list_foreach (rows, (GFunc) gtk_tree_path_free, NULL);
	g_list_free (rows);
}

void pl3_browse_replace_selected()
{
	pl3_clear_playlist();
	pl3_browse_add_selected();
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
			PL3_CAT_PROC, FALSE,
			PL3_CAT_ICON_SIZE,3,-1);
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
					PL3_CAT_ICON_SIZE,1,
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
					4, TRUE, 
					PL3_CAT_ICON_SIZE,1,
					-1);
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
			gchar *message = g_strdup_printf("Added songs from artist '%s'",artist);
			mpd_sendFindCommand (info.connection, MPD_TABLE_ARTIST, artist);
			while ((ent = mpd_getNextInfoEntity (info.connection)) != NULL)
			{                                                                         			
				add_list = g_list_append (add_list, g_strdup (ent->info.song->file));
				mpd_freeInfoEntity (ent);
			}
			mpd_finishCommand (info.connection);
			pl3_push_statusbar_message(message);
			g_free(message);

		}
		else
		{
			/* album selected */
			/* fetch all songs by this album and check if the artist is right. from mpd and add them to the add-list */
			gchar *message = g_strdup_printf("Added songs from album '%s' ",title);
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
			pl3_push_statusbar_message(message);
			g_free(message);

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

int pl3_playlist_button_press_event(GtkTreeView *tree, GdkEventButton *event)
{
	int type = pl3_cat_get_selected_browser();
	GtkTreeSelection *sel = gtk_tree_view_get_selection(tree);
	if(event->button != 3 || !gtk_tree_selection_count_selected_rows(sel) || check_connection_state())	
	{
		return FALSE;
	}
	if(type == PL3_CURRENT_PLAYLIST)
	{
		/* del, crop */
		GtkWidget *item;
		GtkWidget *menu = gtk_menu_new();	
		/* add the delete widget */
		item = gtk_image_menu_item_new_from_stock(GTK_STOCK_REMOVE,NULL);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
		g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(pl3_current_playlist_delete_selected_songs), NULL);


		/* add the delete widget */
		item = gtk_image_menu_item_new_with_label("Crop");
		gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
				gtk_image_new_from_stock(GTK_STOCK_CUT, GTK_ICON_SIZE_MENU));
		g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(pl3_current_playlist_crop_selected_songs), NULL);		
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

		gtk_menu_shell_append(GTK_MENU_SHELL(menu),gtk_separator_menu_item_new());
		/* add the clear widget */
		item = gtk_image_menu_item_new_from_stock(GTK_STOCK_CLEAR,NULL);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
		g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(pl3_clear_playlist), NULL);		


		/* add the shuffle widget */
		item = gtk_image_menu_item_new_with_label("Shuffle");
		gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
				gtk_image_new_from_stock(GTK_STOCK_REFRESH, GTK_ICON_SIZE_MENU));
		g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(pl3_shuffle_playlist), NULL);		
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

		gtk_menu_shell_append(GTK_MENU_SHELL(menu),gtk_separator_menu_item_new());

		item = gtk_image_menu_item_new_from_stock(GTK_STOCK_DIALOG_INFO,NULL);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
		g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(pl3_show_song_info), NULL);		




		gtk_widget_show_all(menu);
		gtk_menu_popup(GTK_MENU(menu), NULL, NULL,NULL, NULL, event->button, event->time);	
	}
	else if (type == PL3_BROWSE_FILE || type == PL3_BROWSE_ARTIST || type == PL3_FIND || type == PL3_BROWSE_XIPH)
	{

		/* del, crop */
		GtkWidget *item;
		GtkWidget *menu = gtk_menu_new();	
		/* add the delete widget */
		item = gtk_image_menu_item_new_from_stock(GTK_STOCK_ADD,NULL);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
		g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(pl3_browse_add_selected), NULL);

		/* add the replace widget */
		item = gtk_image_menu_item_new_with_label("Replace");
		gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
				gtk_image_new_from_stock(GTK_STOCK_REDO, GTK_ICON_SIZE_MENU));
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
		g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(pl3_browse_replace_selected), NULL);

		gtk_widget_show_all(menu);
		gtk_menu_popup(GTK_MENU(menu), NULL, NULL,NULL, NULL, event->button, event->time);	
	}



	return TRUE;
}


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
	else if (type == PL3_BROWSE_FILE || type == PL3_BROWSE_ARTIST || type == PL3_FIND || type == PL3_BROWSE_XIPH)
	{
		GtkTreeIter iter;
		gchar *song_id;
		gint r_type;
		gtk_tree_model_get_iter(gtk_tree_view_get_model(tree), &iter, tp);
		gtk_tree_model_get(gtk_tree_view_get_model(tree), &iter, PL3_SONG_ID,&song_id, PL3_SONG_POS, &r_type, -1);
		if(song_id == NULL) return;
		if(r_type == PL3_ENTRY_PLAYLIST)
		{	
			pl3_push_statusbar_message("Loaded playlist");
			mpd_sendLoadCommand(info.connection, song_id);
			mpd_finishCommand(info.connection);

			if(check_for_errors()) return;
		}
		else
		{
			pl3_push_statusbar_message("Added a song");
			mpd_sendAddCommand(info.connection, song_id);
			mpd_finishCommand(info.connection);
			if(check_for_errors()) return;
		}
	}
}







/**************************************************
 * Category Tree 
 */
void pl3_reinitialize_tree()
{
	GtkTreePath *path = gtk_tree_path_new_from_string("0");
	GtkTreeSelection *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(glade_xml_get_widget(pl3_xml, "cat_tree")));
	gtk_tree_store_clear(pl3_tree);
	/* add the current playlist */
	pl3_current_playlist_add();
	pl3_file_browser_add();       	
	pl3_artist_browser_add();
	pl3_find_add();
	pl3_xiph_add();


	gtk_widget_grab_focus(glade_xml_get_widget(pl3_xml, "cat_tree"));

	gtk_tree_selection_select_path(sel, path);               		
	gtk_tree_view_set_cursor(GTK_TREE_VIEW(glade_xml_get_widget(pl3_xml, "cat_tree")), path, NULL, FALSE);
	gtk_tree_path_free(path);
}



void pl3_disconnect()
{
	if(pl3_xml != NULL)
	{
		pl3_reinitialize_tree();
	}
}

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
			if(info.status->song != -1 && info.status->playlistLength != 0)
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
	else
	{
		if(gtk_tree_view_row_expanded(tree,tp))
		{
			gtk_tree_view_collapse_row(tree,tp);
		}
		else
		{
			gtk_tree_view_expand_row(tree,tp,FALSE);
		}
	}
}


void pl3_cat_row_expanded(GtkTreeView *tree, GtkTreeIter *iter, GtkTreePath *path)
{
	gint type,read;
	gtk_tree_model_get(GTK_TREE_MODEL(pl3_tree), iter, 0, &type,4,&read, -1);
	/* check if the connection isnt down */
	if(check_connection_state())
	{
		/* if connection down, don't let the treeview open */
		gtk_tree_view_collapse_row(tree,path);
		return;

	}

	if(read) return;
	if(type == PL3_BROWSE_FILE)
	{
		pl3_file_browser_fill_tree(iter);
	}
	else if (type == PL3_BROWSE_ARTIST)
	{
		pl3_artist_browser_fill_tree(iter);

	}
	/* avuton's Idea */
	/* TODO: Make this option */
	//	gtk_tree_view_scroll_to_cell(tree, path,gtk_tree_view_get_column(tree,0),TRUE,0.5,0);
}


void pl3_cat_sel_changed()
{
	GtkTreeModel *model = GTK_TREE_MODEL(pl3_tree);
	GtkTreeIter iter;
	GtkTreeSelection *selec = gtk_tree_view_get_selection((GtkTreeView *)
			glade_xml_get_widget (pl3_xml, "cat_tree"));
	GtkTreeView *tree = (GtkTreeView *) glade_xml_get_widget (pl3_xml, "playlist_tree");
	gtk_statusbar_pop(GTK_STATUSBAR(glade_xml_get_widget(pl3_xml, "statusbar2")), 0);
	gtk_widget_hide(glade_xml_get_widget(pl3_xml, "search_box"));
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
		else if (type == PL3_FIND)
		{
			long unsigned time = 0;
			gchar *string;	
			gtk_list_store_clear(pl3_store);
			gtk_tree_view_set_model(tree, GTK_TREE_MODEL(pl3_store));
			gtk_widget_show_all(glade_xml_get_widget(pl3_xml, "search_box"));
			time = pl3_find_view_browser();
			string = format_time(time);
			gtk_statusbar_push(GTK_STATUSBAR(glade_xml_get_widget(pl3_xml, "statusbar2")),0, string);
			g_free(string);

		}
		else if(type == PL3_BROWSE_XIPH)
		{
			gtk_tree_view_set_model(tree, GTK_TREE_MODEL(pl3_store));


			pl3_xiph_view_browser();
			gtk_statusbar_push(GTK_STATUSBAR(glade_xml_get_widget(pl3_xml, "statusbar2")),0, "");
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
	if(type == -1 || check_connection_state())
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


/**********************************************************
 * MISC
 */











int pl3_window_key_press_event(GtkWidget *mw, GdkEventKey *event)
{


	if(event->keyval == GDK_f && event->state != GDK_CONTROL_MASK)
	{
		int retval;
		gtk_widget_grab_focus(glade_xml_get_widget(pl3_xml, "playlist_tree"));
		g_signal_emit_by_name(G_OBJECT(glade_xml_get_widget (pl3_xml, "playlist_tree")), "start-interactive-search",&retval);
	}
	else if (event->keyval == GDK_w && event->state == GDK_CONTROL_MASK)
	{
		pl3_close();
	}
	else if (event->keyval == GDK_Escape)
	{
		pl3_close();
	}

	if(check_connection_state()) return FALSE;
	/* on F1 move to current playlist */
	if (event->keyval == GDK_F1)
	{
		GtkTreeIter iter;
		GtkTreeSelection *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(glade_xml_get_widget(pl3_xml, "cat_tree")));
		gtk_widget_grab_focus(glade_xml_get_widget(pl3_xml, "playlist_tree"));

		/* select the current playlist */
		if(gtk_tree_model_get_iter_first(GTK_TREE_MODEL(pl3_tree), &iter))
		{
			gtk_tree_selection_select_iter(sel, &iter);               		
		}
	}
	else if (event->keyval == GDK_F2)
	{
		GtkTreePath *path = gtk_tree_path_new_from_string("1");
		GtkTreeSelection *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(glade_xml_get_widget(pl3_xml, "cat_tree")));
		gtk_widget_grab_focus(glade_xml_get_widget(pl3_xml, "cat_tree"));

		gtk_tree_selection_select_path(sel, path);               		
		gtk_tree_view_set_cursor(GTK_TREE_VIEW(glade_xml_get_widget(pl3_xml, "cat_tree")), path, NULL, FALSE);
		gtk_tree_path_free(path);
	}
	else if (event->keyval == GDK_F3)
	{
		GtkTreePath *path = gtk_tree_path_new_from_string("2");
		GtkTreeSelection *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(glade_xml_get_widget(pl3_xml, "cat_tree")));
		gtk_widget_grab_focus(glade_xml_get_widget(pl3_xml, "cat_tree"));

		gtk_tree_selection_select_path(sel, path);               		
		gtk_tree_view_set_cursor(GTK_TREE_VIEW(glade_xml_get_widget(pl3_xml, "cat_tree")), path, NULL, FALSE);
		gtk_tree_path_free(path);
	}
	else if (event->keyval == GDK_F4)
	{
		GtkTreePath *path = gtk_tree_path_new_from_string("3");
		GtkTreeSelection *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(glade_xml_get_widget(pl3_xml, "cat_tree")));
		gtk_widget_grab_focus(glade_xml_get_widget(pl3_xml, "cat_tree"));
		gtk_tree_selection_select_path(sel, path);               	
		gtk_tree_view_set_cursor(GTK_TREE_VIEW(glade_xml_get_widget(pl3_xml, "cat_tree")), path, NULL, FALSE);
		gtk_tree_path_free(path);
	}







	/* default gmpc/xmms/gmpc key's*/
	else if (event->keyval == GDK_z && event->state == 0)
	{
		prev_song();	
	}
	else if ((event->keyval == GDK_x || event->keyval == GDK_c )&& event->state == 0)
	{
		play_song();	
	}
	else if (event->keyval == GDK_v && event->state == 0)
	{
		stop_song();
	}
	else if (event->keyval == GDK_b && event->state == 0)
	{
		next_song();
	}

	/* propagate */
	return FALSE;
}



int pl3_cat_key_press_event(GtkWidget *mw, GdkEventKey *event)
{
	/* call default */
	gint type = pl3_cat_get_selected_browser();
	if(event->keyval == GDK_Insert && type == PL3_BROWSE_FILE)
	{
		pl3_browse_file_add_folder();		
	}
	else if (event->keyval == GDK_Insert && type == PL3_BROWSE_ARTIST)
	{
		pl3_browse_artist_add_folder();
	}

	return pl3_window_key_press_event(mw,event);
}

int pl3_playlist_key_press_event(GtkWidget *mw, GdkEventKey *event)
{
	gint type = pl3_cat_get_selected_browser();
	if(event->keyval == GDK_Delete && type == PL3_CURRENT_PLAYLIST)
	{
		pl3_current_playlist_delete_selected_songs ();
		return TRUE;
	}
	else if (event->keyval == GDK_Insert && 
			(type == PL3_BROWSE_FILE || type == PL3_BROWSE_ARTIST || type == PL3_FIND || type == PL3_BROWSE_XIPH))
	{
		pl3_browse_add_selected();	
		return TRUE;
	}
	/* call default */
	return pl3_window_key_press_event(mw,event);
}


int pl3_pop_statusbar_message()
{

	gtk_statusbar_pop(GTK_STATUSBAR(glade_xml_get_widget(pl3_xml, "statusbar1")), 0);
	return FALSE;
}


void pl3_push_statusbar_message(char *mesg)
{
	/* message auto_remove after 5 sec */
	g_timeout_add(5000,(GSourceFunc)pl3_pop_statusbar_message, NULL);
	gtk_statusbar_push(GTK_STATUSBAR(glade_xml_get_widget(pl3_xml, "statusbar1")), 0,mesg);
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
	GtkTreeIter iter;
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
		pl3_tree = gtk_tree_store_new (6, 
				GTK_TYPE_INT,	/* row type, see free_type struct */
				GTK_TYPE_STRING, /* display name */
				GTK_TYPE_STRING,/* full path and stuff for backend */
				GTK_TYPE_STRING,
				GTK_TYPE_BOOL,
				GTK_TYPE_UINT);	/* stock_id*/
	}

	tree = glade_xml_get_widget (pl3_xml, "cat_tree");

	gtk_tree_view_set_model (GTK_TREE_VIEW (tree), GTK_TREE_MODEL (pl3_tree));



	/* draw the column with the songs */
	renderer = gtk_cell_renderer_pixbuf_new ();
	column = gtk_tree_view_column_new ();
	gtk_tree_view_column_pack_start (column, renderer, FALSE);
	gtk_tree_view_column_set_attributes (column,
			renderer,
			"stock-id",3,"stock-size",5, NULL);


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

	pl3_reinitialize_tree();


	/* add the file browser */
	gtk_combo_box_set_active(GTK_COMBO_BOX(glade_xml_get_widget(pl3_xml, "cb_field_selector")),0);
	gtk_widget_show(glade_xml_get_widget(pl3_xml, "pl3_win"));

	/* connect signals that are defined in the gui description */
	glade_xml_signal_autoconnect (pl3_xml);

	/* select the current playlist */
	if(gtk_tree_model_get_iter_first(GTK_TREE_MODEL(pl3_tree), &iter))
	{
		gtk_tree_selection_select_iter(sel, &iter);
	}
}





