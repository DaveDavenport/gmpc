/*
 *Copyright (C) 2004-2007 Qball Cow <qball@sarine.nl>
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
#include <config.h>
#include <regex.h>

#include "plugin.h"

#include "main.h"
#include "misc.h"
#include "playlist3.h"
#include "playlist3-current-playlist-browser.h"
#include "config1.h"
#include "TreeSearchWidget.h"
#include "gmpc-mpddata-model.h"
#include "gmpc-mpddata-model-playlist.h"
#include "gmpc-mpddata-treeview.h"
#include "eggcolumnchooserdialog.h"
#include "sexy-icon-entry.h"

static int pl3_current_playlist_browser_button_press_event(GtkTreeView *tree, GdkEventButton *event);
static void pl3_current_playlist_browser_scroll_to_current_song(void);
static void pl3_current_playlist_browser_add(GtkWidget *cat_tree);

static void pl3_current_playlist_browser_selected(GtkWidget *container);
static void pl3_current_playlist_browser_unselected(GtkWidget *container);

static int pl3_current_playlist_browser_cat_menu_popup(GtkWidget *menu, int type, GtkWidget *tree, GdkEventButton *event);


static void pl3_current_playlist_status_changed(MpdObj *mi, ChangedStatusType what, void *userdata);
static int pl3_current_playlist_browser_add_go_menu(GtkWidget *menu);
GtkTreeModel *playlist_queue = NULL;



/* just for here */
static void pl3_current_playlist_browser_row_activated(GtkTreeView *tree, GtkTreePath *path, GtkTreeViewColumn *col);
static int  pl3_current_playlist_browser_button_release_event(GtkTreeView *tree, GdkEventButton *event);
static int  pl3_current_playlist_browser_button_press_event(GtkTreeView *tree, GdkEventButton *event);
static int  pl3_current_playlist_browser_key_release_event(GtkTreeView *tree, GdkEventKey *event,GtkWidget *entry);
static void pl3_current_playlist_browser_show_info(void);
static void pl3_current_playlist_save_playlist(void);
static void pl3_current_playlist_browser_shuffle_playlist(void);
static void pl3_current_playlist_browser_clear_playlist(void);
static int pl3_current_playlist_key_press_event(GtkWidget *mw, GdkEventKey *event, int type);
static void pl3_current_playlist_connection_changed(MpdObj *mi, int connect, gpointer data);
static void pl3_current_playlist_save_myself(void);
static void pl3_current_playlist_browser_init(void);
GtkTreeModel *playlist = NULL;
GtkWidget *pl3_cp_tree = NULL;

static void pl3_cp_current_song_changed(GmpcMpdDataModelPlaylist *model,GtkTreePath *path, GtkTreeIter *iter,gpointer data)
{
        if(cfg_get_single_value_as_int_with_default(config, "playlist", "st_cur_song", 0))
        {
            gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(pl3_cp_tree),
                    path,
                    NULL,
                    TRUE,0.5,0);
        }
}


static void pl3_cp_init()
{
    playlist = (GtkTreeModel *)gmpc_mpddata_model_playlist_new(gmpcconn,connection);
    pl3_current_playlist_browser_init();
    g_signal_connect(G_OBJECT(playlist), "current_song_changed", G_CALLBACK(pl3_cp_current_song_changed), NULL);
}

gmpcPlBrowserPlugin current_playlist_gbp = {
	pl3_current_playlist_browser_add,
	pl3_current_playlist_browser_selected,
	pl3_current_playlist_browser_unselected,
	NULL,
	NULL,
	pl3_current_playlist_browser_cat_menu_popup,
	NULL,
	pl3_current_playlist_browser_add_go_menu,
	pl3_current_playlist_key_press_event
};


gmpcPlugin current_playlist_plug = {
	.name = 							"Current Playlist Browser",
	.version = 							{1,1,1},
	.plugin_type =						GMPC_PLUGIN_PL_BROWSER,
    .init   =                           pl3_cp_init,
	.browser = 							&current_playlist_gbp,
	.mpd_status_changed = 				pl3_current_playlist_status_changed,
	.mpd_connection_changed = 			pl3_current_playlist_connection_changed,
	.destroy = 							pl3_current_playlist_destroy,
	.save_yourself = 					pl3_current_playlist_save_myself
};


/* external objects */
extern GladeXML *pl3_xml;

/* internal */

static GtkWidget *pl3_cp_sw = NULL;
static GtkWidget *pl3_cp_vbox = NULL;
static TreeSearch *tree_search = NULL;
static GtkWidget *pl3_queue_sw = NULL;
static GtkTreeRowReference *pl3_curb_tree_ref = NULL;

static void pl3_current_playlist_search_activate(TreeSearch *search,GtkTreeView *tree)
{
    printf("tree %p %p\n",search, tree);
	GtkTreeModel *model = gtk_tree_view_get_model(tree); 
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));
	if (gtk_tree_selection_count_selected_rows (selection) == 1)            
	{
		GList *list = gtk_tree_selection_get_selected_rows (selection, &model);
		pl3_current_playlist_browser_row_activated(GTK_TREE_VIEW(tree),(GtkTreePath *)list->data, NULL);	
		/* free list */
		g_list_foreach (list, (GFunc) gtk_tree_path_free, NULL);                        	
		g_list_free (list);
	}
}



static void pl3_current_playlist_column_changed(GtkTreeView *tree)
{
	int position = 0;
	GList *iter,*cols = gtk_tree_view_get_columns(tree);
	for(iter = cols; iter; iter = g_list_next(iter))
	{
		gpointer data = g_object_get_data(G_OBJECT(iter->data), "colid");
		int colid = GPOINTER_TO_INT(data);
		char *string = g_strdup_printf("%i", position);
		cfg_set_single_value_as_int(config, "current-playlist-column-pos", string, colid);
		position++;
		q_free(string);
	}
	g_list_free(cols);
}

void pl3_current_playlist_destroy()
{
	if(pl3_cp_tree)
	{
		GList *iter,*cols = gtk_tree_view_get_columns(GTK_TREE_VIEW(pl3_cp_tree));
		for(iter = cols; iter; iter = g_list_next(iter))
		{
			gpointer data = g_object_get_data(G_OBJECT(iter->data), "colid");
			int colid = GPOINTER_TO_INT(data);
			gchar *string = g_strdup_printf("%i", colid);
			int width = gtk_tree_view_column_get_width(GTK_TREE_VIEW_COLUMN(iter->data));
			cfg_set_single_value_as_int(config, "current-playlist-column-width", string,width);
			q_free(string);
		}
		g_list_free(cols);

		/* destroy the entry */ 
		if(pl3_curb_tree_ref)
		{
			GtkTreeIter iter;
			GtkTreePath *path;
			path = gtk_tree_row_reference_get_path(pl3_curb_tree_ref);
			if(path)
			{
				if(gtk_tree_model_get_iter(GTK_TREE_MODEL(gtk_tree_row_reference_get_model(pl3_curb_tree_ref)), &iter,path))
				{
					gtk_tree_store_remove(GTK_TREE_STORE(gtk_tree_row_reference_get_model(pl3_curb_tree_ref)), &iter);
				}
				gtk_tree_path_free(path);
			}
			gtk_tree_row_reference_free(pl3_curb_tree_ref);
			pl3_curb_tree_ref = NULL;
		}
		/* destroy the browser */
		if(pl3_cp_vbox)
		{
			/* remove the signal handler so when the widget is destroyed, the numbering of the labels are not changed again */
			g_signal_handlers_disconnect_by_func(G_OBJECT(pl3_cp_tree), G_CALLBACK(pl3_current_playlist_column_changed), NULL);
			g_object_unref(pl3_cp_vbox);
			pl3_cp_vbox = NULL;
		}
		pl3_cp_tree =  NULL;
	}
}
static void pl3_current_browser_queue_delete(GtkTreeView *tree, GtkTreePath *path)
{
	GtkTreeIter iter;
	GtkTreeModel *model = gtk_tree_view_get_model(tree);
	if(gtk_tree_model_get_iter(model, &iter, path))
	{
		guint id;
		gtk_tree_model_get(model, &iter, MPDDATA_MODEL_COL_SONG_POS, &id, -1);
		mpd_playlist_mpd_queue_remove(connection, id);

	}
}
static int  pl3_current_playlist_browser_queue_key_release_event(GtkTreeView *tree, GdkEventKey *event)
{
	if(event->keyval == GDK_Delete)
	{
		GtkTreeSelection *selection = gtk_tree_view_get_selection(tree);
		GtkTreeModel *model = gtk_tree_view_get_model(tree);
		GList *list, *iter;
		list =  gtk_tree_selection_get_selected_rows (selection, &model);
		if(list)
		{
			for(iter = g_list_last(list);iter;iter = g_list_previous(iter))
			{
				GtkTreeIter liter;
				if(gtk_tree_model_get_iter(model, &liter, iter->data))
				{
					guint id;
					gtk_tree_model_get(model, &liter, MPDDATA_MODEL_COL_SONG_POS, &id, -1);
					mpd_playlist_queue_mpd_queue_remove(connection, id);
				}
			}
			mpd_playlist_queue_commit(connection);
			g_list_foreach (list, (GFunc) gtk_tree_path_free, NULL);
			g_list_free (list);
		}


		return TRUE;                                          		
	}
	return FALSE;
}
static GtkTreeModel *mod_fill = NULL;
static guint timeout=0;

static gboolean mod_fill_do_entry_changed(GtkWidget *entry, GtkWidget *tree)
{
    const gchar *text = gtk_entry_get_text(GTK_ENTRY(entry));
    if(strlen(text) > 0)
    {
        MpdData *data;
        int i =0;
        gchar **splitted = g_strsplit(text, " ", 0);
        gtk_tree_view_set_model(GTK_TREE_VIEW(pl3_cp_tree), NULL);
        
        mpd_playlist_search_start(connection,FALSE);
        for(i=0;splitted[i];i++)
        {
            mpd_playlist_search_add_constraint(connection, MPD_TAG_ITEM_ANY, splitted[i]);
        }
        data = mpd_playlist_search_commit(connection);
        gmpc_mpddata_model_set_mpd_data(GMPC_MPDDATA_MODEL(mod_fill), data);

        g_strfreev(splitted);
        gtk_tree_view_set_model(GTK_TREE_VIEW(pl3_cp_tree), mod_fill);
    }
    else
    {
        gtk_tree_view_set_model(GTK_TREE_VIEW(pl3_cp_tree), playlist);
        gmpc_mpddata_model_set_mpd_data(GMPC_MPDDATA_MODEL(mod_fill), NULL);
        gtk_widget_hide(entry);
    }
    timeout = 0;
    return FALSE;
}

static void  mod_fill_entry_changed(GtkWidget *entry, GtkWidget *tree)
{
    if(timeout != 0)
        g_source_remove(timeout);
    timeout = g_timeout_add(1000, (GSourceFunc)mod_fill_do_entry_changed, entry);
    gtk_widget_show(entry);
}


static void pl3_current_playlist_browser_init(void)
{
	GtkWidget *queue_tree;
  
	pl3_cp_vbox = gtk_vbox_new(FALSE,6);
    GtkWidget *tree = gmpc_mpddata_treeview_new("current-pl", FALSE, GTK_TREE_MODEL(playlist));
    /* filter */
    mod_fill = (GtkTreeModel *)gmpc_mpddata_model_new();
    GtkWidget *entry = sexy_icon_entry_new(); 
    sexy_icon_entry_add_clear_button(SEXY_ICON_ENTRY(entry));
    gtk_box_pack_start(GTK_BOX(pl3_cp_vbox), entry, FALSE, TRUE,0);
    //gtk_widget_show(entry);
    g_signal_connect(G_OBJECT(entry), "changed", G_CALLBACK(mod_fill_entry_changed), tree);

    gtk_tree_view_set_reorderable(GTK_TREE_VIEW(tree), TRUE);
	GtkWidget *sw = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw), GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw), GTK_SHADOW_ETCHED_IN);
    gtk_container_add(GTK_CONTAINER(sw), tree);
	gtk_box_pack_start(GTK_BOX(pl3_cp_vbox), GTK_WIDGET(sw), TRUE, TRUE,0);
    gtk_widget_show_all(sw);
	/* set up the tree */
	gtk_tree_view_set_enable_search(GTK_TREE_VIEW(tree), FALSE);

	/* setup signals */
	g_signal_connect(G_OBJECT(tree), "row-activated",G_CALLBACK(pl3_current_playlist_browser_row_activated), NULL); 
	g_signal_connect(G_OBJECT(tree), "button-press-event", G_CALLBACK(pl3_current_playlist_browser_button_press_event), NULL);
	g_signal_connect(G_OBJECT(tree), "button-release-event", G_CALLBACK(pl3_current_playlist_browser_button_release_event), NULL);
	g_signal_connect(G_OBJECT(tree), "key-press-event", G_CALLBACK(pl3_current_playlist_browser_key_release_event), entry);

	/* set up the scrolled window */
	pl3_cp_sw = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(pl3_cp_sw), GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(pl3_cp_sw), GTK_SHADOW_ETCHED_IN);


	tree_search = (TreeSearch *)treesearch_new(GTK_TREE_VIEW(tree), MPDDATA_MODEL_COL_MARKUP);
	g_signal_connect(G_OBJECT(tree_search), "result-activate", G_CALLBACK(pl3_current_playlist_search_activate),tree);


	gtk_box_pack_end(GTK_BOX(pl3_cp_vbox), GTK_WIDGET(tree_search), FALSE, TRUE, 0);	


	playlist_queue = (GtkTreeModel *)gmpc_mpddata_model_new();	
	pl3_queue_sw = gtk_scrolled_window_new(NULL,NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(pl3_queue_sw), GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(pl3_queue_sw), GTK_SHADOW_ETCHED_IN);
	queue_tree = gmpc_mpddata_treeview_new("queue", FALSE, GTK_TREE_MODEL(playlist_queue));
	gtk_container_add(GTK_CONTAINER(pl3_queue_sw), queue_tree);
	gtk_widget_set_size_request(GTK_WIDGET(pl3_queue_sw), -1, 150);
	gtk_box_pack_start(GTK_BOX(pl3_cp_vbox), GTK_WIDGET(pl3_queue_sw), FALSE, TRUE,0);
	g_signal_connect(G_OBJECT(queue_tree), "row-activated", G_CALLBACK(pl3_current_browser_queue_delete), NULL);
	g_signal_connect(G_OBJECT(queue_tree), "key-press-event", G_CALLBACK(pl3_current_playlist_browser_queue_key_release_event), NULL);
	/* set initial state */
    pl3_cp_tree = tree;
	g_object_ref(G_OBJECT(pl3_cp_vbox));
}

static void pl3_current_playlist_browser_select_current_song()
{
	if(pl3_cp_tree == NULL|| !GTK_WIDGET_REALIZED(pl3_cp_tree)) return;
	/* scroll to the playing song */
	if(mpd_player_get_current_song_pos(connection) >= 0 && mpd_playlist_get_playlist_length(connection)  > 0)
	{
		GtkTreePath *path = gtk_tree_path_new_from_indices(mpd_player_get_current_song_pos(connection),-1);
		if(path != NULL)
		{
        	gtk_tree_view_set_cursor(GTK_TREE_VIEW(pl3_cp_tree), path, NULL, FALSE);
		}
		gtk_tree_path_free(path);
	}      
}

static void pl3_current_playlist_browser_scroll_to_current_song()
{
	if(pl3_cp_tree == NULL || !GTK_WIDGET_REALIZED(pl3_cp_tree)) return;
	/* scroll to the playing song */
	if(mpd_player_get_current_song_pos(connection) >= 0 && mpd_playlist_get_playlist_length(connection)  > 0)
	{
		GtkTreePath *path = gtk_tree_path_new_from_indices(mpd_player_get_current_song_pos(connection),-1);
		if(path != NULL)
		{
			gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(pl3_cp_tree),
					path,
					NULL,
					TRUE,0.5,0);
			gtk_tree_path_free(path);
		}
	
	}      
}

/* add's the toplevel entry for the current playlist view */
static void pl3_current_playlist_browser_add(GtkWidget *cat_tree)
{
	GtkTreeIter iter;
	GtkTreePath *path;
	gint pos = cfg_get_single_value_as_int_with_default(config, "current-playlist","position",0);
	playlist3_insert_browser(&iter, pos);
	//gtk_tree_store_append(pl3_tree, &iter, NULL);
	gtk_tree_store_set(pl3_tree, &iter, 
			PL3_CAT_TYPE, current_playlist_plug.id,/*PL3_CURRENT_PLAYLIST,*/
			PL3_CAT_TITLE, _("Current Playlist"),
			PL3_CAT_INT_ID, "",
			PL3_CAT_ICON_ID, "playlist-browser",
			PL3_CAT_PROC, TRUE,
			PL3_CAT_ICON_SIZE,GTK_ICON_SIZE_DND,
			-1);
	if(pl3_curb_tree_ref)
	{
		gtk_tree_row_reference_free(pl3_curb_tree_ref);
		pl3_curb_tree_ref = NULL;
	}
	path = gtk_tree_model_get_path(GTK_TREE_MODEL(pl3_tree), &iter);
	if(path)
	{
		pl3_curb_tree_ref = gtk_tree_row_reference_new(GTK_TREE_MODEL(pl3_tree),path);
		gtk_tree_path_free(path);
	}
}


static void pl3_current_playlist_browser_queue_add()
{
	/* grab the selection from the tree */
	GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW(pl3_cp_tree));
	/* check if where connected */
	/* see if there is a row selected */
	if (gtk_tree_selection_count_selected_rows (selection) > 0)
	{
		GList *list = NULL, *llist = NULL;
		GtkTreeModel *model =  gtk_tree_view_get_model (GTK_TREE_VIEW(pl3_cp_tree));
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
			gtk_tree_model_get (model, &iter, MPDDATA_MODEL_COL_SONG_ID, &value, -1);
			mpd_playlist_queue_mpd_queue_add(connection, value);			
		} while ((llist = g_list_next (llist)));
		mpd_playlist_queue_commit(connection);
		/* free list */
		g_list_foreach (list, (GFunc) gtk_tree_path_free, NULL);
		g_list_free (list);
	}
	mpd_status_queue_update(connection);
}

/* delete all selected songs,
 * if no songs select ask the user if he want's to clear the list 
 */
static void pl3_current_playlist_browser_delete_selected_songs ()
{
	/* grab the selection from the tree */
	GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW(pl3_cp_tree));
	/* check if where connected */
	/* see if there is a row selected */
	if (gtk_tree_selection_count_selected_rows (selection) > 0)
	{
		GList *list = NULL, *llist = NULL;
        GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(pl3_cp_tree));
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
			gtk_tree_model_get (model, &iter, MPDDATA_MODEL_COL_SONG_ID, &value, -1);
			mpd_playlist_queue_delete_id(connection, value);			
		} while ((llist = g_list_next (llist)));

		/* close the list, so it will be executed */
		mpd_playlist_queue_commit(connection);
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
					_("Are you sure you want to clear the playlist?"));
		gtk_dialog_add_buttons (GTK_DIALOG (dialog), GTK_STOCK_CANCEL,
				GTK_RESPONSE_CANCEL, GTK_STOCK_OK,
				GTK_RESPONSE_OK, NULL);
		gtk_dialog_set_default_response (GTK_DIALOG (dialog),
				GTK_RESPONSE_CANCEL);

		switch (gtk_dialog_run (GTK_DIALOG (dialog)))
		{
			case GTK_RESPONSE_OK:
				/* check if where still connected */
				mpd_playlist_clear(connection);
		}
		gtk_widget_destroy (GTK_WIDGET (dialog));
	}
	/* update everything if where still connected */
	gtk_tree_selection_unselect_all(selection);

	mpd_status_queue_update(connection);
}

static void pl3_current_playlist_browser_crop_selected_songs()
{
    GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(pl3_cp_tree));
	/* grab the selection from the tree */
	GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW(pl3_cp_tree));

	/* see if there is a row selected */	
	if (gtk_tree_selection_count_selected_rows (selection) > 0)
	{
		GtkTreeIter iter;


		/* start a command list */
		/* remove every selected song one by one */
		if(gtk_tree_model_get_iter_first(GTK_TREE_MODEL(model), &iter))
		{
			do{
				int value=0;
				if(!gtk_tree_selection_iter_is_selected(selection, &iter))
				{
					gtk_tree_model_get (GTK_TREE_MODEL(model), &iter, MPDDATA_MODEL_COL_SONG_ID, &value, -1);
					mpd_playlist_queue_delete_id(connection, value);				
				}
			} while (gtk_tree_model_iter_next(GTK_TREE_MODEL(model),&iter));
			mpd_playlist_queue_commit(connection);
		}

	}
	/* update everything if where still connected */
	gtk_tree_selection_unselect_all(selection);

	mpd_status_queue_update(connection);
}

static void pl3_current_playlist_editor_add_to_playlist(GtkWidget *menu)
{
    GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(pl3_cp_tree));
    GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(pl3_cp_tree));
	gchar *data = g_object_get_data(G_OBJECT(menu), "playlist");
	GList *iter, *list = gtk_tree_selection_get_selected_rows (selection, &model);
	if(list)
	{
		iter = g_list_first(list);
		do{
			GtkTreeIter giter;
			if(gtk_tree_model_get_iter(model, &giter, (GtkTreePath *)iter->data))
			{
				gchar *file = NULL;
				gtk_tree_model_get(model, &giter, MPDDATA_MODEL_COL_PATH, &file, -1);
				mpd_database_playlist_list_add(connection, data,file); 
				g_free(file);
			}
		}while((iter = g_list_next(iter)));

		g_list_foreach (list, (GFunc) gtk_tree_path_free, NULL);                        	
		g_list_free (list);
	}
}
static int pl3_current_playlist_browser_button_press_event(GtkTreeView *tree, GdkEventButton *event)
{
	GtkTreePath *path = NULL;
	if(event->button == 3 &&gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(tree), event->x, event->y,&path,NULL,NULL,NULL))
	{	
		GtkTreeSelection *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));
		if(gtk_tree_selection_path_is_selected(sel, path))
		{
			gtk_tree_path_free(path);
			return TRUE;
		}
	}
	if(path) {
		gtk_tree_path_free(path);
	}
	return FALSE;                                                                                                     
}
static void pl3_current_playlist_browser_edit_columns(void)
{
  gmpc_mpddata_treeview_edit_columns(GMPC_MPDDATA_TREEVIEW(pl3_cp_tree));
}

static int pl3_current_playlist_browser_button_release_event(GtkTreeView *tree, GdkEventButton *event)
{
	if(event->button == 3)
	{
		/* del, crop */
		GtkWidget *item;
		GtkWidget *menu = gtk_menu_new();	
    
        /* */
        if(mpd_server_check_command_allowed(connection, "queueinfo") == MPD_SERVER_COMMAND_ALLOWED)
		{
			item = gtk_image_menu_item_new_with_label(_("Add to Queue"));
			gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
					gtk_image_new_from_stock(GTK_STOCK_ADD, GTK_ICON_SIZE_MENU));
			g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(pl3_current_playlist_browser_queue_add), NULL);		
			gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
		}
        if(gtk_tree_selection_count_selected_rows(gtk_tree_view_get_selection(tree)) == 1)	
        {
            mpd_Song *song;
            GtkTreeIter iter;
            GtkTreePath *path;
            GtkTreeModel *model = gtk_tree_view_get_model(tree);
            GList *list = gtk_tree_selection_get_selected_rows(gtk_tree_view_get_selection(GTK_TREE_VIEW(tree)), &model);
            path = list->data;
            /* free result */
            g_list_free(list);
            if(path && gtk_tree_model_get_iter(model, &iter, path)) {
                gtk_tree_model_get(model, &iter, MPDDATA_MODEL_COL_MPDSONG, &song, -1);
                if(song)
                {
                    submenu_for_song(menu, song);
                }
                if(path)                                                                
                    gtk_tree_path_free(path);                                               
            }
        }

        /* add the delete widget */
        item = gtk_image_menu_item_new_from_stock(GTK_STOCK_REMOVE,NULL);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
        g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(pl3_current_playlist_browser_delete_selected_songs), NULL);

        /* add the delete widget */
        item = gtk_image_menu_item_new_with_label(_("Crop"));
        gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
                gtk_image_new_from_stock(GTK_STOCK_CUT, GTK_ICON_SIZE_MENU));
        g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(pl3_current_playlist_browser_crop_selected_songs), NULL);		
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

        gtk_menu_shell_append(GTK_MENU_SHELL(menu),gtk_separator_menu_item_new());
        /* add the clear widget */
        item = gtk_image_menu_item_new_from_stock(GTK_STOCK_CLEAR,NULL);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
        g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(pl3_current_playlist_browser_clear_playlist), NULL);		


        /* add the shuffle widget */
        item = gtk_image_menu_item_new_with_label(_("Shuffle"));
        gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
                gtk_image_new_from_stock(GTK_STOCK_REFRESH, GTK_ICON_SIZE_MENU));
        g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(pl3_current_playlist_browser_shuffle_playlist), NULL);		
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

        gtk_menu_shell_append(GTK_MENU_SHELL(menu),gtk_separator_menu_item_new());

        item = gtk_image_menu_item_new_from_stock(GTK_STOCK_DIALOG_INFO,NULL);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
        g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(pl3_current_playlist_browser_show_info), NULL);		

        /* add the shuffle widget */
 

        item = gtk_image_menu_item_new_with_label(_("Edit Columns"));
        gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
                gtk_image_new_from_stock(GTK_STOCK_EDIT, GTK_ICON_SIZE_MENU));
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
        g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(pl3_current_playlist_browser_edit_columns), NULL);





        playlist_editor_right_mouse(menu,pl3_current_playlist_editor_add_to_playlist);

        gtk_widget_show_all(menu);
        gtk_menu_popup(GTK_MENU(menu), NULL, NULL,NULL, NULL,0, event->time);	
        return TRUE;
    }
    return FALSE;
}

static void pl3_current_playlist_browser_row_activated(GtkTreeView *tree, GtkTreePath *path, GtkTreeViewColumn *col)
{
    GtkTreeIter iter;
    gint song_id;
    gtk_tree_model_get_iter(gtk_tree_view_get_model(tree), &iter, path);
    gtk_tree_model_get(gtk_tree_view_get_model(tree), &iter, MPDDATA_MODEL_COL_SONG_ID,&song_id, -1);
    mpd_player_play_id(connection, song_id);
}

static void pl3_current_playlist_browser_show_info()
{

    GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW(pl3_cp_tree));
    GtkTreeSelection *selection =gtk_tree_view_get_selection (GTK_TREE_VIEW(pl3_cp_tree));
    if (gtk_tree_selection_count_selected_rows (selection) > 0)
    {
        GList *list = NULL;
        list = gtk_tree_selection_get_selected_rows (selection, &model);

        list = g_list_last (list);

        {
            GtkTreeIter iter;
            mpd_Song *song = NULL;
            gtk_tree_model_get_iter (model, &iter, (GtkTreePath *) list->data);
            gtk_tree_model_get (model, &iter, MPDDATA_MODEL_COL_MPDSONG, &song, -1);

            info2_activate();
            info2_fill_song_view(song);	
        }


        g_list_foreach (list, (GFunc) gtk_tree_path_free, NULL);
        g_list_free (list);
    }
}

static void pl3_current_playlist_browser_selected(GtkWidget *container)
{
    if(pl3_cp_vbox == NULL)
    {
        pl3_current_playlist_browser_init();
    }
    gtk_container_add(GTK_CONTAINER(container), pl3_cp_vbox);
    gtk_widget_show(pl3_cp_vbox);

    gtk_widget_grab_focus(pl3_cp_tree);
}
static void pl3_current_playlist_browser_unselected(GtkWidget *container)
{
    gtk_container_remove(GTK_CONTAINER(container), pl3_cp_vbox);
}


static int pl3_current_playlist_browser_cat_menu_popup(GtkWidget *menu, int type, GtkWidget *tree, GdkEventButton *event)
{
    /* here we have:  Save, Clear*/
    GtkWidget *item;
    if(type != current_playlist_plug.id) return 0;
    /* add the save widget */
    item = gtk_image_menu_item_new_from_stock(GTK_STOCK_SAVE,NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
    g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(pl3_current_playlist_save_playlist), NULL);

    /* add the clear widget */
    item = gtk_image_menu_item_new_from_stock(GTK_STOCK_CLEAR,NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
    g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(pl3_current_playlist_browser_clear_playlist), NULL);


    return 1;
}

static int  pl3_current_playlist_browser_key_release_event(GtkTreeView *tree, GdkEventKey *event,GtkWidget *entry)
{
    if(event->keyval == GDK_Delete)
    {
        pl3_current_playlist_browser_delete_selected_songs ();
        return TRUE;                                          		
    }
    else if(event->keyval == GDK_i && event->state&GDK_MOD1_MASK)
    {
        pl3_current_playlist_browser_show_info();
        return TRUE;
    }
    else if (event->keyval == GDK_space)
    {
        pl3_current_playlist_browser_scroll_to_current_song();
        pl3_current_playlist_browser_select_current_song();
        return TRUE;			
    }
    else if (event->keyval == GDK_f && event->state&GDK_CONTROL_MASK)
    {
//        treesearch_start(tree_search);
        mod_fill_entry_changed(entry, NULL);
        gtk_widget_grab_focus(entry);        
        return TRUE;
    }
    else if((event->state&(GDK_CONTROL_MASK|GDK_MOD1_MASK)) == 0 && 
            ((event->keyval >= GDK_space && event->keyval <= GDK_z)))
    {
        char data[2];
        data[0] = (char)gdk_keyval_to_unicode(event->keyval);
        data[1] = '\0';
//        treesearch_start(TREESEARCH(tree_search));
         gtk_widget_grab_focus(entry);      
        gtk_entry_set_text(GTK_ENTRY(entry),data);
        gtk_editable_set_position(GTK_EDITABLE(entry),1);

        return TRUE;
    }
    return pl3_window_key_press_event(GTK_WIDGET(tree),event);
}

/* create a dialog that allows the user to save the current playlist */
static void pl3_current_playlist_save_playlist ()
{
    gchar *str;
    GladeXML *xml = NULL;
    int run = TRUE;
    /* check if the connection is up */
    if (!mpd_check_connected(connection))
    {
        return;
    }
    /* create the interface */
    str = gmpc_get_full_glade_path("playlist3.glade");
    xml = glade_xml_new (str, "save_pl", NULL);
    q_free(str);

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
                if (strlen (str) != 0 && mpd_check_connected(connection))
                {
                    if(mpd_database_save_playlist(connection, str) == MPD_DATABASE_PLAYLIST_EXIST )
                    {
                        gchar *errormsg = g_strdup_printf(_("<i>Playlist <b>\"%s\"</b> already exists\nOverwrite?</i>"), str);
                        gtk_label_set_markup(GTK_LABEL(glade_xml_get_widget(xml, "label_error")), errormsg);
                        gtk_widget_show(glade_xml_get_widget(xml, "hbox5"));
                        /* ask to replace */
                        gtk_widget_set_sensitive(GTK_WIDGET(glade_xml_get_widget(xml, "pl-entry")), FALSE);
                        switch (gtk_dialog_run (GTK_DIALOG (glade_xml_get_widget (xml, "save_pl"))))
                        {
                            case GTK_RESPONSE_OK:
                                run = FALSE;
                                mpd_database_delete_playlist(connection, str);
                                mpd_database_save_playlist(connection,str);
                                GmpcStatusChangedCallback(connection, MPD_CST_DATABASE, NULL);
                                break;
                            default:
                                run = TRUE;
                        }
                        /* return to stare */
                        gtk_widget_set_sensitive(GTK_WIDGET(glade_xml_get_widget(xml, "pl-entry")), TRUE);
                        gtk_widget_hide(glade_xml_get_widget(xml, "hbox5"));

                        q_free(errormsg);
                    }
                    else 
                    {
                        GmpcStatusChangedCallback(connection, MPD_CST_DATABASE, NULL);
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

static void pl3_current_playlist_browser_clear_playlist()
{
    mpd_playlist_clear(connection);
}

static void pl3_current_playlist_browser_shuffle_playlist()
{
    mpd_playlist_shuffle(connection);
}

static void pl3_current_playlist_status_changed(MpdObj *mi, ChangedStatusType what, void *userdata)
{
    if(pl3_cp_vbox == NULL)
        return;

    if(what&MPD_CST_QUEUE)
    {
        if(playlist_queue)
        {
            if(mpd_server_check_command_allowed(connection, "queueinfo") == MPD_SERVER_COMMAND_ALLOWED)
            {
                MpdData *data = mpd_playlist_get_mpd_queue(connection);
                gmpc_mpddata_model_set_mpd_data(GMPC_MPDDATA_MODEL(playlist_queue),data);
                if(data)
                    gtk_widget_show_all(pl3_queue_sw);
                else
                    gtk_widget_hide(pl3_queue_sw);
            }
        }
    }
}

static void pl3_current_playlist_browser_activate()
{
    GtkTreeSelection *selec = gtk_tree_view_get_selection((GtkTreeView *)
            glade_xml_get_widget (pl3_xml, "cat_tree"));


    GtkTreePath *path = gtk_tree_row_reference_get_path(pl3_curb_tree_ref); 
    if(path)
    {
        gtk_tree_selection_select_path(selec, path);
        gtk_tree_path_free(path);
    }
}


static int pl3_current_playlist_browser_add_go_menu(GtkWidget *menu)
{
    GtkWidget *item = NULL;

    item = gtk_image_menu_item_new_with_label(_("Current Playlist"));
    gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item), 
            gtk_image_new_from_icon_name("playlist-browser", GTK_ICON_SIZE_MENU));
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
    gtk_widget_add_accelerator(GTK_WIDGET(item), "activate", gtk_menu_get_accel_group(GTK_MENU(menu)), GDK_F1, 0, GTK_ACCEL_VISIBLE);
    g_signal_connect(G_OBJECT(item), "activate", 
            G_CALLBACK(pl3_current_playlist_browser_activate), NULL);
    return 1;
}

/**
 * 
 */
static int pl3_current_playlist_key_press_event(GtkWidget *mw, GdkEventKey *event, int type)
{

    return FALSE;
}

static void pl3_current_playlist_connection_changed(MpdObj *mi, int connect,gpointer data)
{
}
/* function that saves the settings */
static void pl3_current_playlist_save_myself(void)
{
    if(pl3_curb_tree_ref)
    {
        GtkTreePath *path = gtk_tree_row_reference_get_path(pl3_curb_tree_ref);
        if(path)
        {
            gint *indices = gtk_tree_path_get_indices(path);
            debug_printf(DEBUG_INFO,"Saving myself to position: %i\n", indices[0]);
            cfg_set_single_value_as_int(config, "current-playlist","position",indices[0]);
            gtk_tree_path_free(path);
        }
    }
}

