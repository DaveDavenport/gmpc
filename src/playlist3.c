/*
 *Copyright (C) 2004 Qball Cow <Qball@qballcow.nl>
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
#include "libmpdclient.h"
#include "strfsong.h"
#include "main.h"
#include "misc.h"
#include "playlist3.h"
#include "open-location.h"
#include "vfs_download.h"
#include "osb_browser.h"
#include "config1.h"

extern config_obj *config;
GladeXML *pl3_xml = NULL;
GtkTreeStore *pl3_tree = NULL;
GtkListStore *pl3_store = NULL;
GtkListStore *pl2_store = NULL;
/* size */
GtkAllocation pl3_wsize = { 0,0,0,0};
int pl3_hidden = TRUE;
void pl2_save_playlist ();
/****************************************************************/
/* We want to move this to mpdinteraction 			*/
/****************************************************************/
void pl3_clear_playlist()
{
	mpd_ob_playlist_clear(connection);
}

void pl3_shuffle_playlist()
{
	mpd_ob_playlist_shuffle(connection);
}

/* custom search and match function, this is a workaround for the problems with in gtk+-2.6 */
gboolean pl3_playlist_tree_search_func(GtkTreeModel *model, gint column, const char *key, GtkTreeIter *iter)
{
	char *value= NULL;
	char *lkey, *lvalue;
	int ret = TRUE;
	if(iter == NULL)
	{
		return TRUE;
	}
	gtk_tree_model_get(model, iter, column, &value, -1);
	if(value == NULL || key == NULL)
	{
		return TRUE;
	}
	lkey = g_utf8_casefold(key,-1);
	lvalue = g_utf8_casefold(value,-1);
	
	if(strstr(lvalue,lkey) != NULL)
	{
		ret = FALSE;
	}
	g_free(lkey);
	g_free(lvalue);
	return ret;
}


/********************************************************************
 * Misc functions 
 */


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
/***********************************
 * Custom Streams
 */
void pl3_custom_stream_add()
{
	GtkTreeIter iter,child;
	if(!cfg_get_single_value_as_int_with_default(config, "playlist", "custom_stream_enable", TRUE))
	{
		return;
	}
	gtk_tree_store_append(pl3_tree, &iter, NULL);
	gtk_tree_store_set(pl3_tree, &iter, 
			PL3_CAT_TYPE, PL3_BROWSE_CUSTOM_STREAM,
			PL3_CAT_TITLE, "Custom Streams",
			PL3_CAT_INT_ID, "",
			PL3_CAT_ICON_ID, "media-stream",
			PL3_CAT_PROC, FALSE,          	
			PL3_CAT_ICON_SIZE,GTK_ICON_SIZE_DND,-1);
	gtk_tree_store_append(pl3_tree, &child, &iter);
	gtk_tree_store_set(pl3_tree, &child, 
			PL3_CAT_TYPE, PL3_BROWSE_CUSTOM_STREAM,
			PL3_CAT_TITLE, "Add a Stream",
			PL3_CAT_INT_ID, "add",
			PL3_CAT_ICON_ID, "icecast",
			PL3_CAT_PROC, FALSE,          	
			PL3_CAT_ICON_SIZE,GTK_ICON_SIZE_DND,-1);
}

void pl3_custom_stream_view_browser()
{

	/* make this path configurable, we don't use gnome-vfs for nothing */
	gchar *path = g_strdup_printf("/%s/.gmpc.cst",g_getenv("HOME"));
	if(g_file_test(path, G_FILE_TEST_EXISTS))
	{
		xmlDocPtr xmldoc = xmlParseFile(path);
		xmlNodePtr root = xmlDocGetRootElement(xmldoc);
		xmlNodePtr cur = root->xmlChildrenNode;
		while(cur != NULL)
		{
			if(xmlStrEqual(cur->name, "entry"))
			{
				xmlNodePtr cur1 = cur->xmlChildrenNode;
				GtkTreeIter iter;
				char *name=NULL;
				gtk_list_store_append(pl3_store, &iter);
				gtk_list_store_set (pl3_store, &iter,
						PL3_SONG_POS, PL3_ENTRY_STREAM, 
						PL3_SONG_STOCK_ID, "media-stream", 
						-1);
				while(cur1 != NULL)
				{
					if(xmlStrEqual(cur1->name, "name"))
					{
						gtk_list_store_set(pl3_store, &iter, PL3_SONG_TITLE, xmlNodeGetContent(cur1), -1);
						name = xmlNodeGetContent(cur1);
					}
					else if(xmlStrEqual(cur1->name, "listen_url"))
					{
						gtk_list_store_set(pl3_store, &iter, PL3_SONG_ID, xmlNodeGetContent(cur1), -1);
					}
					cur1 = cur1->next;
				}

			}

			cur = cur->next;
		}
		xmlFreeDoc(xmldoc);
		xmlCleanupParser();
	}
	g_free(path);
}


void pl3_custom_stream_add_url_changed(GtkEntry *entry, GtkWidget *button)
{
	if(strstr(gtk_entry_get_text(entry), "://"))
	{
		gtk_widget_set_sensitive(button, TRUE);
	}	
	else
	{
		gtk_widget_set_sensitive(button, FALSE);
	}


}

void pl3_custom_stream_add_stream(gchar *name, gchar *url)
{
	GladeXML *xml = glade_xml_new(GLADE_PATH"playlist3.glade", "add_stream",NULL);
	GtkWidget *dialog = glade_xml_get_widget(xml, "add_stream");
	gtk_window_set_transient_for(GTK_WINDOW(dialog),GTK_WINDOW(glade_xml_get_widget(pl3_xml, "pl3_win")));
	g_signal_connect(G_OBJECT(glade_xml_get_widget(xml, "entry_url")),"changed", G_CALLBACK(pl3_custom_stream_add_url_changed), 
			glade_xml_get_widget(xml, "button_add"));
	gtk_widget_show_all(dialog);
	if(name != NULL)
	{
		gtk_entry_set_text(GTK_ENTRY(glade_xml_get_widget(xml, "entry_name")),name);
	}
	if(url != NULL)
	{
		gtk_entry_set_text(GTK_ENTRY(glade_xml_get_widget(xml, "entry_url")),url);
	}                                                                                   	
	switch(gtk_dialog_run(GTK_DIALOG(dialog)))
	{
		case GTK_RESPONSE_OK:
			{
				gchar *path = g_strdup_printf("%s/.gmpc.cst",g_getenv("HOME"));
				xmlDocPtr xmldoc;
				xmlNodePtr newn,new2,root;
				if(g_file_test(path, G_FILE_TEST_EXISTS))
				{
					xmldoc = xmlParseFile(path);
					root = xmlDocGetRootElement(xmldoc);
				}
				else
				{
					xmldoc = xmlNewDoc("1.0");
					root = xmlNewDocNode(xmldoc, NULL, "streams",NULL);
					xmlDocSetRootElement(xmldoc, root);

				}
				newn = xmlNewChild(root, NULL, "entry",NULL);
				new2 = xmlNewChild(newn, NULL, "name", 
						gtk_entry_get_text(GTK_ENTRY(glade_xml_get_widget(xml, "entry_name"))));
				new2 = xmlNewChild(newn, NULL, "listen_url", 
						gtk_entry_get_text(GTK_ENTRY(glade_xml_get_widget(xml, "entry_url"))));

				xmlSaveFile(path, xmldoc);	


				g_free(path);
			}
		default:
			break;	
	}
	gtk_widget_destroy(dialog);
	g_object_unref(xml);
}


/**/
void pl3_custom_stream_save_tree()
{
	gchar *path = g_strdup_printf("%s/.gmpc.cst",g_getenv("HOME"));
	xmlDocPtr xmldoc;
	xmlNodePtr newn,new2,root;                        
	GtkTreeIter iter;		

	xmldoc = xmlNewDoc("1.0");
	root = xmlNewDocNode(xmldoc, NULL, "streams",NULL);
	xmlDocSetRootElement(xmldoc, root);
	if(gtk_tree_model_get_iter_first(GTK_TREE_MODEL(pl3_store), &iter))
	{
		do
		{
			gchar *name, *lurl;
			gtk_tree_model_get(GTK_TREE_MODEL(pl3_store), &iter,SONG_ID, &lurl, SONG_TITLE, &name, -1);
			newn = xmlNewChild(root, NULL, "entry",NULL);
			new2 = xmlNewChild(newn, NULL, "name",name); 
			new2 = xmlNewChild(newn, NULL, "listen_url", lurl);
		}while(gtk_tree_model_iter_next(GTK_TREE_MODEL(pl3_store), &iter));

	}
	xmlSaveFile(path, xmldoc);	
}





/* where going todo this the dirty way.
 * Delete the requested streams then read the info from the tree
 */
void pl3_custom_stream_remove()
{
	GtkTreeModel *model = GTK_TREE_MODEL(pl3_store);
	/* grab the selection from the tree */
	GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW(glade_xml_get_widget (pl3_xml, "playlist_tree")));
	/* check if where connected */
	if (check_connection_state ())
		return;
	/* see if there is a row selected */
	if (gtk_tree_selection_count_selected_rows (selection) > 0)
	{
		GList *list = NULL, *llist = NULL;
		/* grab the selected songs */
		list = gtk_tree_selection_get_selected_rows (selection, &model);
		/* grab the last song that is selected */
		llist = g_list_last (list);
		/* remove every selected song one by one */
		do
		{
			GtkTreeIter iter;
			gtk_tree_model_get_iter (model, &iter,(GtkTreePath *) llist->data);
			gtk_list_store_remove (pl3_store, &iter);

		}
		while ((llist = g_list_previous (llist)));

		/* free list */
		g_list_foreach (list, (GFunc) gtk_tree_path_free, NULL);
		g_list_free (list);

		check_for_errors ();
	}
	pl3_custom_stream_save_tree();
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
			PL3_CAT_ICON_SIZE,GTK_ICON_SIZE_DND,-1);
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

		gtk_tree_model_get(model, &iter, PL3_CAT_TITLE, &name, PL3_CAT_INT_ID,&field,-1);

		num_field = atoi(field);



		if(strcmp(name, "Search"))
		{
			MpdData *data = NULL;
			gtk_entry_set_text(GTK_ENTRY(glade_xml_get_widget(pl3_xml, "search_entry")), name);
			gtk_combo_box_set_active(GTK_COMBO_BOX(glade_xml_get_widget(pl3_xml, "cb_field_selector")), num_field);

			/* do the actual search */
			data = mpd_ob_playlist_find(connection, num_field, name, FALSE);

			while (data != NULL)
			{
				gchar buffer[1024];
				if(data->value.song->time != MPD_SONG_NO_TIME)
				{
					time += data->value.song->time;
				}

				strfsong (buffer, 1024, 
						cfg_get_single_value_as_string_with_default(config, "playlist", "browser_markup",DEFAULT_MARKUP_BROWSER),
						data->value.song);
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



void pl3_current_playlist_row_changed(GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter)
{
	gint pos, new_pos;
	gchar *str = NULL;         	
	gint type = pl3_cat_get_selected_browser();
	if(type != PL3_CURRENT_PLAYLIST) return;
	str = gtk_tree_path_to_string(path);

	gtk_tree_model_get(GTK_TREE_MODEL(pl2_store), iter,SONG_POS, &pos, -1);
	new_pos = atoi(str);
	if(new_pos > pos ) new_pos --;
	/* if there wasn't a move action we don't do anything, because this signal is trigged on every row change */
	if(new_pos == pos)
	{
		g_free(str);
		return;
	}


	mpd_ob_playlist_move_pos(connection, pos, new_pos);
	gtk_list_store_set(pl2_store,iter, SONG_POS, new_pos, -1);
	g_free(str);
}


/* add's the toplevel entry for the current playlist view */
void pl3_current_playlist_add()
{
	GtkTreeIter iter;
	gtk_tree_store_append(pl3_tree, &iter, NULL);
	gtk_tree_store_set(pl3_tree, &iter, 
			PL3_CAT_TYPE, PL3_CURRENT_PLAYLIST,
			PL3_CAT_TITLE, "Current Playlist",
			PL3_CAT_INT_ID, "",
			PL3_CAT_ICON_ID, "media-playlist",
			PL3_CAT_PROC, TRUE,
			PL3_CAT_ICON_SIZE,GTK_ICON_SIZE_DND,
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
	/* see if there is a row selected */
	if (gtk_tree_selection_count_selected_rows (selection) > 0)
	{
		GList *list = NULL, *llist = NULL;
		GtkTreeModel *model = GTK_TREE_MODEL(pl2_store);
		/* start a command list */
		/* grab the selected songs */
		list = gtk_tree_selection_get_selected_rows (selection, &model);
		/* grab the last song that is selected */
		llist = g_list_first (list);
		/* remove every selected song one by one */
		do{
			GtkTreeIter iter;
			int value;
			gtk_tree_model_get_iter (model, &iter,(GtkTreePath *) llist->data);
			gtk_tree_model_get (model, &iter, SONG_ID, &value, -1);
			mpd_ob_playlist_queue_delete_id(connection, value);			
		} while ((llist = g_list_next (llist)));

		/* close the list, so it will be executed */
		mpd_ob_playlist_queue_commit(connection);
		/* free list */
		g_list_foreach (list, (GFunc) gtk_tree_path_free, NULL);
		g_list_free (list);
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
				mpd_ob_playlist_clear(connection);
		}
		gtk_widget_destroy (GTK_WIDGET (dialog));
	}
	/* update everything if where still connected */
	gtk_tree_selection_unselect_all(selection);

	mpd_ob_status_queue_update(connection);
}


void pl3_current_playlist_crop_selected_songs()
{
	/* grab the selection from the tree */
	GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW(glade_xml_get_widget (pl3_xml, "playlist_tree")));

	/* see if there is a row selected */
	if (gtk_tree_selection_count_selected_rows (selection) > 0)
	{
		GtkTreeIter iter;
		/* start a command list */
		/* remove every selected song one by one */
		gtk_tree_model_get_iter_first(GTK_TREE_MODEL(pl2_store), &iter);
		do{
			int value=0;
			if(!gtk_tree_selection_iter_is_selected(selection, &iter))
			{
				gtk_tree_model_get (GTK_TREE_MODEL(pl2_store), &iter, SONG_ID, &value, -1);
				printf("test %i\n", value);
				mpd_ob_playlist_queue_delete_id(connection, value);				
			}
		} while (gtk_tree_model_iter_next(GTK_TREE_MODEL(pl2_store),&iter));
		mpd_ob_playlist_queue_commit(connection);
	}
	/* update everything if where still connected */
	gtk_tree_selection_unselect_all(selection);
	
	mpd_ob_status_queue_update(connection);
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
			gtk_tree_model_get (model, &iter, SONG_ID, &value, -1);
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
		mpd_ob_playlist_queue_add(connection, path);
		mpd_ob_playlist_queue_commit(connection);
	}
}

void pl3_browse_file_update_folder()
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
		message = g_strdup_printf("Updating folder '%s' recursively", path);
		pl3_push_statusbar_message(message);
		g_free(message);
		mpd_ob_playlist_update_dir(connection, path);
	}
}

void pl3_browse_file_replace_folder()
{
	pl3_clear_playlist();
	pl3_browse_file_add_folder();	
	mpd_ob_player_play(connection);
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
			PL3_CAT_ICON_SIZE,GTK_ICON_SIZE_DND,-1);
	/* add fantom child for lazy tree */
	gtk_tree_store_append(pl3_tree, &child, &iter);
}

long unsigned pl3_file_browser_view_folder(GtkTreeIter *iter_cat)
{
	MpdData* data =NULL;
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

	data = mpd_ob_playlist_get_directory(connection, path);
	while (data != NULL)
	{
		if (data->type == MPD_DATA_TYPE_DIRECTORY)
		{
			sub_folder++;
		}
		else if (data->type == MPD_DATA_TYPE_SONG)
		{
			gchar buffer[1024];
			strfsong (buffer, 1024, 
					cfg_get_single_value_as_string_with_default(config, "playlist", "browser_markup",DEFAULT_MARKUP_BROWSER),
					data->value.song);
			if(data->value.song->time != MPD_SONG_NO_TIME)
			{
				time += data->value.song->time;			
			}

			gtk_list_store_append (pl3_store, &iter);
			gtk_list_store_set (pl3_store, &iter,
					0, data->value.song->file,
					1, PL3_ENTRY_SONG,
					2, buffer,               
					5, "media-audiofile",
					-1);

		}

		else if (data->type == MPD_DATA_TYPE_PLAYLIST)
		{
			gchar *basename = g_path_get_basename (data->value.playlist);
			gtk_list_store_append (pl3_store, &iter);
			gtk_list_store_set (pl3_store, &iter,
					0, data->value.playlist,
					1, PL3_ENTRY_PLAYLIST,
					2, basename,
					5, "media-playlist", 
					-1);
			g_free (basename);
		}
		data = mpd_ob_data_get_next(data);
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
	char *path;
	MpdData *data = NULL;
	GtkTreeIter child,child2;
	gtk_tree_model_get(GTK_TREE_MODEL(pl3_tree),iter, 2, &path, -1);
	gtk_tree_store_set(pl3_tree, iter, 4, TRUE, -1);

	data = mpd_ob_playlist_get_directory(connection, path);
	while (data != NULL)
	{
		if (data->type == MPD_DATA_TYPE_DIRECTORY)
		{
			gchar *basename =
				g_path_get_basename (data->value.directory);
			gtk_tree_store_append (pl3_tree, &child, iter);
			gtk_tree_store_set (pl3_tree, &child,
					0, PL3_BROWSE_FILE,
					1, basename,
					2, data->value.directory,
					3, "gtk-open",
					4, FALSE,
					PL3_CAT_ICON_SIZE,1,
					-1);
			gtk_tree_store_append(pl3_tree, &child2, &child);

			g_free (basename);
		}
		data = mpd_ob_data_get_next(data);
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
			gtk_tree_model_get (GTK_TREE_MODEL (pl3_store), &iter, 0,&name, 1, &type, -1);	  
			if(type == PL3_ENTRY_SONG)
			{
				/* add them to the add list */
				mpd_ob_playlist_queue_add(connection, name);
			}
			else if(type == PL3_ENTRY_STREAM)
			{
				ol_create_url(glade_xml_get_widget(pl3_xml, "pl3_win"), name);
			}
			else if (type == PL3_ENTRY_PLAYLIST)
			{
				mpd_ob_playlist_queue_load(connection, name);
			}
			songs++;
		}while((node = g_list_next(node)) != NULL);
	}
	/* if there are items in the add list add them to the playlist */
	mpd_ob_playlist_queue_commit(connection);
	if(songs != 0)
	{
		gint type =  pl3_cat_get_selected_browser();
		if(type == PL3_BROWSE_XIPH || type == PL3_BROWSE_CUSTOM_STREAM)
		{
			message = g_strdup_printf("Added %i streams", songs);
		}
		else message = g_strdup_printf("Added %i songs", songs);
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
	mpd_ob_player_play(connection);
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
			PL3_CAT_ICON_SIZE,GTK_ICON_SIZE_DND,-1);
	/* add fantom child for lazy tree */
	gtk_tree_store_append(pl3_tree, &child, &iter);
}


long unsigned pl3_artist_browser_view_folder(GtkTreeIter *iter_cat)
{
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
		MpdData *data = mpd_ob_playlist_find(connection, MPD_TABLE_ARTIST, artist, TRUE);
		/* artist is selected */
		while(data != NULL)
		{
			if (data->value.song->album == NULL
					|| strlen (data->value.song->album) == 0)
			{
				gchar buffer[1024];
				strfsong (buffer, 1024,
						cfg_get_single_value_as_string_with_default(config, "playlist", "browser_markup",DEFAULT_MARKUP_BROWSER),
						data->value.song);
				if(data->value.song->time != MPD_SONG_NO_TIME)
				{
					time += data->value.song->time;
				}
				if(data->value.song->file == NULL)
				{
					g_print("crap\n");
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
	else 
	{
		/* artist and album is selected */
		MpdData *data = mpd_ob_playlist_find(connection,MPD_TABLE_ALBUM, string, TRUE);
		while (data != NULL)
		{
			if (data->value.song->artist!= NULL
					&& !g_utf8_collate (data->value.song->artist, artist))
			{
				gchar buffer[1024];
				strfsong (buffer, 1024,
						cfg_get_single_value_as_string_with_default(config, "playlist", "browser_markup",DEFAULT_MARKUP_BROWSER),
						data->value.song);
				if(data->value.song->time != MPD_SONG_NO_TIME)
				{
					time += data->value.song->time;
				}
				if(data->value.song->file == NULL)
				{
					g_print("crap\n");
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
	GtkTreeIter child,child2;
	gtk_tree_model_get(GTK_TREE_MODEL(pl3_tree),iter, 1, &artist,2,&alb_artist, -1);
	gtk_tree_store_set(pl3_tree, iter, 4, TRUE, -1);

	if (!mpd_ob_check_connected(connection))
	{
		return;
	}
	if(!strlen(alb_artist))
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
	else if(!g_utf8_collate(artist, alb_artist))
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



void pl3_browse_artist_add_folder()
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

void pl3_browse_artist_replace_folder()
{
	pl3_clear_playlist();
	pl3_browse_artist_add_folder();
	mpd_ob_player_play(connection);
}



/**************************************************
 * PLaylist Tree
 */
void pl3_browse_delete_playlist(GtkToggleButton *bt, char *string)
{
	/* create a warning message dialog */
	GtkWidget *dialog =
		gtk_message_dialog_new (GTK_WINDOW
				(glade_xml_get_widget
				 (pl3_xml, "pl3_win")),
				GTK_DIALOG_MODAL,
				GTK_MESSAGE_WARNING,
				GTK_BUTTONS_NONE,
				_("Are you sure you want to clear the selected playlist?"));
	gtk_dialog_add_buttons (GTK_DIALOG (dialog), GTK_STOCK_NO,
			GTK_RESPONSE_CANCEL, GTK_STOCK_YES,
			GTK_RESPONSE_OK, NULL);
	gtk_dialog_set_default_response (GTK_DIALOG (dialog),
			GTK_RESPONSE_CANCEL);

	switch (gtk_dialog_run (GTK_DIALOG (dialog)))
	{
		case GTK_RESPONSE_OK:
			mpd_ob_playlist_delete(connection, string);
			pl3_cat_sel_changed();
			
	}
	gtk_widget_destroy (GTK_WIDGET (dialog));


	
	

}
int pl3_playlist_button_press_event(GtkTreeView *tree, GdkEventButton *event)
{
	int type = pl3_cat_get_selected_browser();
	GtkTreeSelection *sel = gtk_tree_view_get_selection(tree);
	if(event->button != 3 || !gtk_tree_selection_count_selected_rows(sel) || !mpd_ob_check_connected(connection))	
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
	else if (type == PL3_BROWSE_FILE || type == PL3_BROWSE_ARTIST || type == PL3_FIND || type == PL3_BROWSE_XIPH || type == PL3_BROWSE_CUSTOM_STREAM)
	{

		/* del, crop */
		GtkWidget *item;
		GtkWidget *menu = gtk_menu_new();	
		if(gtk_tree_selection_count_selected_rows(sel) == 1)
		{	
			GtkTreeModel *model = GTK_TREE_MODEL(pl3_store);	
			GList *list = gtk_tree_selection_get_selected_rows(sel, &model);
			if(list != NULL)
			{
				GtkTreeIter iter;
				int row_type;
				char *path;
				list = g_list_first(list);
				gtk_tree_model_get_iter(model, &iter, list->data);
				gtk_tree_model_get(model, &iter,0,&path,1, &row_type, -1); 
				if(row_type == PL3_ENTRY_PLAYLIST)
				{
					item = gtk_image_menu_item_new_from_stock(GTK_STOCK_DELETE,NULL);
					gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
					g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(pl3_browse_delete_playlist), path);
				}
				g_list_foreach (list, gtk_tree_path_free, NULL);
				g_list_free (list);
			}
		}
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

		if(type == PL3_BROWSE_CUSTOM_STREAM)
		{
			item = gtk_image_menu_item_new_from_stock(GTK_STOCK_REMOVE,NULL);                          		
			gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
			g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(pl3_custom_stream_remove), NULL);
		}

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
		mpd_ob_player_play_id(connection, song_id);
	}
	else if (type == PL3_BROWSE_FILE || type == PL3_BROWSE_ARTIST || type == PL3_FIND || type == PL3_BROWSE_XIPH || type == PL3_BROWSE_CUSTOM_STREAM)
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
			mpd_ob_playlist_queue_load(connection, song_id);
		}
		else if (type == PL3_BROWSE_CUSTOM_STREAM || type == PL3_BROWSE_XIPH)
		{
			pl3_browse_add_selected();
		}
		else
		{
			pl3_push_statusbar_message("Added a song");
			mpd_ob_playlist_queue_add(connection, song_id);
		}
		mpd_ob_playlist_queue_commit(connection);
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
	pl3_custom_stream_add();

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
		if(mpd_ob_player_get_current_song_pos(connection) > 0 && mpd_ob_playlist_get_playlist_length(connection)  > 0)
		{
			gchar *str = g_strdup_printf("%i", mpd_ob_player_get_current_song_pos(connection));
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

	if(!read)
	{
		if(type == PL3_BROWSE_FILE)
		{
			pl3_file_browser_fill_tree(iter);
		}
		else if (type == PL3_BROWSE_ARTIST)
		{
			pl3_artist_browser_fill_tree(iter);

		}
	}
	/* avuton's Idea */
	if(cfg_get_single_value_as_int_with_default(config, "playlist", "open-to-position",0))
	{
		gtk_tree_view_scroll_to_cell(tree, path,gtk_tree_view_get_column(tree,0),TRUE,0.5,0);
	}
}


void pl3_cat_sel_changed()
{
	GtkTreeModel *model = GTK_TREE_MODEL(pl3_tree);
	GtkTreeIter iter;
	GtkTreeSelection *selec = gtk_tree_view_get_selection((GtkTreeView *)glade_xml_get_widget (pl3_xml, "cat_tree"));
	GtkTreeView *tree = (GtkTreeView *) glade_xml_get_widget (pl3_xml, "playlist_tree");
	gtk_statusbar_pop(GTK_STATUSBAR(glade_xml_get_widget(pl3_xml, "statusbar2")), 0);
	gtk_widget_hide(glade_xml_get_widget(pl3_xml, "search_box"));
	/*	gtk_tree_view_set_reorderable(tree, FALSE);*/
	if(gtk_tree_selection_get_selected(selec,&model, &iter))
	{
		gint type;
		gtk_tree_model_get(model, &iter, 0, &type, -1);
		if(type == PL3_CURRENT_PLAYLIST)
		{
			gchar *string = format_time(info.playlist_playtime);
			gtk_statusbar_push(GTK_STATUSBAR(glade_xml_get_widget(pl3_xml, "statusbar2")),0, string);
			g_free(string);
			gtk_tree_view_set_model(tree, GTK_TREE_MODEL(pl2_store));
			/*			gtk_tree_view_set_reorderable(tree, TRUE);*/
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
			gchar *url =NULL,*name = NULL;
			gtk_tree_view_set_model(tree, GTK_TREE_MODEL(pl3_store));
			gtk_tree_model_get(model, &iter, PL3_CAT_INT_ID, &url, PL3_CAT_TITLE, &name,-1);

			if(url != NULL && strlen(url) > 0) 
			{
				pl3_xiph_view_browser(url,name);
			}
			else
			{
				gtk_list_store_clear(pl3_store);
			}
			gtk_statusbar_push(GTK_STATUSBAR(glade_xml_get_widget(pl3_xml, "statusbar2")),0, "");
		}
		else if(type == PL3_BROWSE_CUSTOM_STREAM)
		{
			char *id;
			GtkTreeIter parent;
			gtk_tree_view_set_model(tree, GTK_TREE_MODEL(pl3_store));
			gtk_tree_model_get(model, &iter,PL3_CAT_INT_ID , &id, -1);
			if(strlen(id) != 0)
			{


				pl3_custom_stream_add_stream(NULL,NULL);
				gtk_tree_model_iter_parent(model, &parent, &iter);
				gtk_tree_selection_select_iter(selec, &parent);   					

			}
			else
			{	
				gtk_list_store_clear(pl3_store);
				pl3_custom_stream_view_browser();
				gtk_statusbar_push(GTK_STATUSBAR(glade_xml_get_widget(pl3_xml, "statusbar2")),0, "");
			}
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


void pl3_playlist_change()
{
	gint type = pl3_cat_get_selected_browser();
	if(type == PL3_CURRENT_PLAYLIST)
	{
		gchar *string = format_time(info.playlist_playtime);
		gtk_statusbar_push(GTK_STATUSBAR(glade_xml_get_widget(pl3_xml, "statusbar2")),0, string);
		g_free(string);
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

		/* add the update widget */
		item = gtk_image_menu_item_new_with_label("Update");
		gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
				gtk_image_new_from_stock(GTK_STOCK_REFRESH, GTK_ICON_SIZE_MENU));
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
		g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(pl3_browse_file_update_folder), NULL);				

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
	else if (type == PL3_BROWSE_XIPH)
	{
		/* here we have:  Add. Replace*/
		GtkWidget *item;
		GtkWidget *menu = gtk_menu_new();	
		GtkTreeSelection *selec = gtk_tree_view_get_selection((GtkTreeView *)glade_xml_get_widget (pl3_xml, "cat_tree"));
		GtkTreeModel *model = GTK_TREE_MODEL(pl3_tree);
		GtkTreeIter iter;
		char *id;

		/* add the add widget */
		item = gtk_image_menu_item_new_from_stock(GTK_STOCK_ADD,NULL);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
		g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(pl3_xiph_add_source), NULL);		



		if(gtk_tree_selection_get_selected(selec,&model, &iter))
		{
			gtk_tree_model_get(model, &iter, PL3_CAT_INT_ID, &id, -1);
			if(strlen(id) > 0)
			{
				item = gtk_image_menu_item_new_from_stock(GTK_STOCK_REMOVE,NULL);
				gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
				g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(pl3_xiph_del_source), NULL);		


				item = gtk_image_menu_item_new_from_stock(GTK_STOCK_REFRESH,NULL);
				gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
				g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(pl3_xiph_refresh), NULL);		




			}
		}
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
		/* disabled because of problems with gtk 2.6 */
		gtk_widget_grab_focus(glade_xml_get_widget(pl3_xml, "playlist_tree"));
		return FALSE;
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

	/* default gmpc/xmms/winamp key's*/
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
	else
	{
		return FALSE;
	}

	/* don't propagate */
	return TRUE;
}



int pl3_cat_key_press_event(GtkWidget *mw, GdkEventKey *event)
{
	/* call default */
	gint type = pl3_cat_get_selected_browser();
	if(event->state == GDK_CONTROL_MASK && event->keyval == GDK_Insert && type == PL3_BROWSE_FILE)
	{
		pl3_browse_file_replace_folder();		
	}
	else if(event->keyval == GDK_Insert && type == PL3_BROWSE_FILE)
	{
		pl3_browse_file_add_folder();		
	}
	else if (event->state == GDK_CONTROL_MASK && event->keyval == GDK_Insert && type == PL3_BROWSE_ARTIST)
	{
		pl3_browse_artist_replace_folder();
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
	else if (event->keyval == GDK_Delete && type == PL3_BROWSE_CUSTOM_STREAM)
	{
		pl3_custom_stream_remove();	
		return TRUE;
	}
	else if (event->keyval == GDK_Insert &&  event->state == GDK_CONTROL_MASK &&
			(type == PL3_BROWSE_FILE || type == PL3_BROWSE_ARTIST || type == PL3_FIND || type == PL3_BROWSE_XIPH))
	{
		pl3_browse_replace_selected();	
		return TRUE;
	}

	else if (event->keyval == GDK_Insert && 
			(type == PL3_BROWSE_FILE || type == PL3_BROWSE_ARTIST || type == PL3_FIND || type == PL3_BROWSE_XIPH))
	{
		pl3_browse_add_selected();	
		return TRUE;
	}
	else if (event->keyval == GDK_i && type == PL3_CURRENT_PLAYLIST)
	{

		pl3_show_song_info ();
		return  TRUE;
	}
	else if (event->keyval == GDK_space && type == PL3_CURRENT_PLAYLIST)
	{
		if(mpd_ob_player_get_current_song_pos(connection) > 0 && mpd_ob_playlist_get_playlist_length(connection)  > 0)
		{
			gchar *str = g_strdup_printf("%i", mpd_ob_player_get_current_song_pos(connection));
			GtkTreePath *path = gtk_tree_path_new_from_string(str);
			gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(
						glade_xml_get_widget(pl3_xml, "playlist_tree")), 
					path,
					NULL,
					TRUE,0.5,0);
			gtk_tree_path_free(path);
			g_free(str);
		}                                                      		
		return TRUE;			

	}
	/* call default */
	return pl3_window_key_press_event(mw,event);
}


int pl3_pop_statusbar_message(gpointer data)
{
	gint id = GPOINTER_TO_INT(data);
	gtk_statusbar_pop(GTK_STATUSBAR(glade_xml_get_widget(pl3_xml, "statusbar1")), id);
	return FALSE;
}


void pl3_push_statusbar_message(char *mesg)
{
	gint id = gtk_statusbar_get_context_id(GTK_STATUSBAR(glade_xml_get_widget(pl3_xml, "statusbar1")), mesg);
	/* message auto_remove after 5 sec */
	g_timeout_add(5000,(GSourceFunc)pl3_pop_statusbar_message, GINT_TO_POINTER(id));
	gtk_statusbar_push(GTK_STATUSBAR(glade_xml_get_widget(pl3_xml, "statusbar1")), id,mesg);
}


int pl3_close()
{
	if(pl3_xml != NULL)
	{
		gtk_window_get_position(GTK_WINDOW(glade_xml_get_widget(pl3_xml, "pl3_win")), &pl3_wsize.x, &pl3_wsize.y);
		gtk_window_get_size(GTK_WINDOW(glade_xml_get_widget(pl3_xml, "pl3_win")), &pl3_wsize.width, &pl3_wsize.height);

		if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(glade_xml_get_widget(xml_main_window, "tb_pl2"))))
		{
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(glade_xml_get_widget(xml_main_window, "tb_pl2")), FALSE);
			return TRUE;
		}
		gtk_widget_hide(glade_xml_get_widget(pl3_xml, "pl3_win"));	
		pl3_hidden = TRUE;
		return TRUE;
	}
	return TRUE;
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
	pl3_hidden = FALSE;
	if(pl3_xml != NULL)
	{
		gtk_window_move(GTK_WINDOW(glade_xml_get_widget(pl3_xml, "pl3_win")), pl3_wsize.x, pl3_wsize.y);
		gtk_window_resize(GTK_WINDOW(glade_xml_get_widget(pl3_xml, "pl3_win")),pl3_wsize.width, pl3_wsize.height);
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
			"stock-id",3,"stock-size",5,NULL);


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


	pl3_store = gtk_list_store_new (PL3_NROWS, 
			GTK_TYPE_STRING,
			GTK_TYPE_INT,	/* pos id */
			GTK_TYPE_STRING,	/* song title */
			GTK_TYPE_INT,	/* color string */
			G_TYPE_BOOLEAN,
			GTK_TYPE_STRING,
			GTK_TYPE_INT,
			GTK_TYPE_FLOAT);	/* stock id */

	renderer = gtk_cell_renderer_pixbuf_new ();
	column = gtk_tree_view_column_new ();
	gtk_tree_view_column_pack_start (column, renderer, FALSE);
	gtk_tree_view_column_set_attributes (column,
			renderer,
			"stock-id", SONG_STOCK_ID,"yalign", STOCK_ALIGN, NULL);

	renderer = gtk_cell_renderer_text_new ();

	/* insert the column in the tree */
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_set_attributes (column,
			renderer,
			"text", SONG_TITLE,
			"weight", WEIGHT_INT,
			"weight-set", WEIGHT_ENABLE, NULL);

	gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);

	gtk_tree_view_set_search_equal_func(GTK_TREE_VIEW(tree),
			(GtkTreeViewSearchEqualFunc)pl3_playlist_tree_search_func, NULL,NULL);
	pl3_reinitialize_tree();

	/* add the file browser */
	gtk_combo_box_set_active(GTK_COMBO_BOX(glade_xml_get_widget(pl3_xml, "cb_field_selector")),0);
	gtk_widget_show(glade_xml_get_widget(pl3_xml, "pl3_win"));

	/* connect signals that are defined in the gui description */
	glade_xml_signal_autoconnect (pl3_xml);

	g_signal_connect(G_OBJECT(pl2_store), "row-changed", G_CALLBACK(pl3_current_playlist_row_changed), NULL);

	/* select the current playlist */
	if(gtk_tree_model_get_iter_first(GTK_TREE_MODEL(pl3_tree), &iter))
	{
		gtk_tree_selection_select_iter(sel, &iter);
	}
}

/* toggles the playlist on or off */
/* this should be placed on the player's toggle button */
gboolean toggle_playlist3(GtkToggleButton *tb)
{
	if(gtk_toggle_button_get_active(tb))
	{
		create_playlist3();
	}
	else
	{
		if(pl3_xml == NULL) return FALSE;
		pl3_close();
	}
	return TRUE;
}


/* this function takes care the right row is highlighted */
void pl3_highlight_state_change(int old_state, int new_state)
{
	GtkTreeIter iter;
	gchar *temp;
	/* unmark the old pos if it exists */
	if (info.old_pos != -1 && mpd_ob_player_get_state(connection) <= MPD_OB_PLAYER_STOP)
	{
		/* create a string so I can get the right iter */
		temp = g_strdup_printf ("%i", info.old_pos);
		if (gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL (pl2_store), &iter, temp))
		{
			gtk_list_store_set (pl2_store, &iter, WEIGHT_INT,PANGO_WEIGHT_NORMAL, -1);
		}
		g_free (temp);
		/* reset old pos */
		info.old_pos = -1;
	}                                                           
	/* if the old state was stopped. (or  unkown) and the new state is play or pause highight the song again */	
	if(old_state <= MPD_OB_PLAYER_STOP && old_state < new_state)
	{
		pl3_highlight_song_change();
	}
}







void pl3_highlight_song_change ()
{
	GtkTreeIter iter;
	gchar *temp;
	/* check if there is a connection */
	if (!mpd_ob_check_connected (connection))
	{
		return;
	}

	/* unmark the old pos if it exists */
	if (info.old_pos != -1)
	{
		/* create a string so I can get the right iter */
		temp = g_strdup_printf ("%i", info.old_pos);
		if (gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL (pl2_store), &iter, temp))
		{
			gint song_id = 0;
			/* check if we have the song we want */
			gtk_tree_model_get (GTK_TREE_MODEL (pl2_store), &iter, SONG_ID,&song_id, -1);
			/* if the old song is the new song (so tags updated) quit */
			if (song_id == mpd_ob_player_get_current_song_id(connection))
			{
				g_print("song change to same song\n");
				g_free (temp);
				return;
			}
			/* unhighlight the song */
			gtk_list_store_set (pl2_store, &iter, WEIGHT_INT,
					PANGO_WEIGHT_NORMAL, -1);
		}
		g_free (temp);
		/* reset old pos */
		info.old_pos = -1;
	}
	/* check if we need to highlight a song */
	if (mpd_ob_player_get_state(connection) > MPD_OB_PLAYER_STOP && mpd_ob_player_get_current_song_pos(connection) >= 0) 
	{
		temp = g_strdup_printf ("%i", mpd_ob_player_get_current_song_pos(connection));
		if (gtk_tree_model_get_iter_from_string (GTK_TREE_MODEL (pl2_store), &iter, temp))
		{
			gint pos;
			gtk_tree_model_get (GTK_TREE_MODEL (pl2_store), &iter, SONG_POS,
					&pos, -1);
			/* check if we have the right song, if not, print an error */
			if (pos != mpd_ob_player_get_current_song_pos(connection))
			{
				g_print ("Errror %i %i\n", pos,mpd_ob_player_get_current_song_pos(connection));
			}
			gtk_list_store_set (pl2_store, &iter, WEIGHT_INT,
					PANGO_WEIGHT_ULTRABOLD, -1);
		}
		g_free (temp);
		/* set highlighted position */
		info.old_pos = mpd_ob_player_get_current_song_pos(connection);
	}
}


/* create a dialog that allows the user to save the current playlist */
void pl2_save_playlist ()
{
	gchar *str;
	GladeXML *xml = NULL;
	int run = TRUE;
	/* check if the connection is up */
	if (check_connection_state ())
		return;

	/* create the interface */
	xml = glade_xml_new (GLADE_PATH "playlist3.glade", "save_pl", NULL);

	/* run the interface */
	do
	{
		switch (gtk_dialog_run (GTK_DIALOG (glade_xml_get_widget (xml, "save_pl"))))
		{
			case GTK_RESPONSE_OK:
				run = FALSE;
				/* if the users agrees do the following: */
				/* get the song-name */
				str = (gchar *)	gtk_entry_get_text (GTK_ENTRY
						(glade_xml_get_widget (xml, "pl-entry")));
				/* check if the user entered a name, we can't do withouth */
				/* TODO: disable ok button when nothing is entered */
				/* also check if there is a connection */
				if (strlen (str) != 0 && !check_connection_state ())
				{
					if(mpd_ob_playlist_save(connection, str) == MPD_O_PLAYLIST_EXIST)
					{
						gchar *errormsg = g_strdup_printf("<i>Playlist <b>\"%s\"</b> allready exists</i>", str);
						gtk_label_set_markup(GTK_LABEL(glade_xml_get_widget(xml, "label_error")), errormsg);
						gtk_widget_show(glade_xml_get_widget(xml, "hbox5"));
						run = TRUE;
						g_free(errormsg);
					}
				}
				break;
			default:
				run = FALSE;
		}
	}while(run);
	/* destroy the window */
	gtk_widget_destroy (glade_xml_get_widget (xml, "save_pl"));

	/* unref the gui description */
	g_object_unref (xml);
}
