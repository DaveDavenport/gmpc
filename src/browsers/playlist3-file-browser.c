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

#include "plugin.h"

#include "main.h"
#include "misc.h"
#include "playlist3.h"
#include "playlist3-file-browser.h"
#include "config1.h"
#include "TreeSearchWidget.h"
#include "gmpc-mpddata-model.h"
#include "gmpc-mpddata-treeview.h"

static void pl3_file_browser_edit_columns(void);

static void pl3_file_browser_destroy(void);
static void pl3_file_browser_add(GtkWidget *cat_tree);
static void pl3_file_browser_unselected(GtkWidget *container);
static void pl3_file_browser_selected(GtkWidget *container);
static void pl3_file_browser_cat_sel_changed(GtkWidget *tree,GtkTreeIter *iter);
static void pl3_file_browser_fill_tree(GtkWidget *tree,GtkTreeIter *iter);
static int pl3_file_browser_cat_popup(GtkWidget *menu, int type,GtkWidget *tree, GdkEventButton *event);
static void pl3_file_browser_cat_key_press(GtkWidget *tree, GdkEventKey *event, int selected_type);
static void pl3_file_browser_delete_playlist_from_right(GtkMenuItem *bt);
/* testing */
static void pl3_file_browser_reupdate(void);

static int pl3_file_browser_add_go_menu(GtkWidget *menu);
static void pl3_file_browser_activate(void);
static gboolean pl3_file_browser_button_release_event(GtkWidget *but, GdkEventButton *event);
static gboolean pl3_file_browser_button_press_event(GtkWidget *but, GdkEventButton *event);
static void pl3_file_browser_row_activated(GtkTreeView *tree, GtkTreePath *tp);
static void pl3_file_browser_add_selected(void);
static void pl3_file_browser_replace_selected(void);
static int pl3_file_browser_playlist_key_press(GtkWidget *tree,GdkEventKey *event);
static void pl3_file_browser_show_info(void);
static long unsigned pl3_file_browser_view_folder(GtkTreeIter *iter_cat);
static void pl3_file_browser_update_folder(void);
static void pl3_file_browser_add_folder(void);
static void pl3_file_browser_delete_playlist(GtkMenuItem *bt);
static void pl3_file_browser_connection_changed(MpdObj *mi, int connect, gpointer data);
static void pl3_file_browser_status_changed(MpdObj *mi,ChangedStatusType what, void *data);
static int pl3_file_browser_key_press_event(GtkWidget *mw, GdkEventKey *event, int type);
static void pl3_file_browser_disconnect(void);

/**
 * Plugin structure
 */
gmpcPlBrowserPlugin file_browser_gbp = {
	pl3_file_browser_add,
	pl3_file_browser_selected,
	pl3_file_browser_unselected,
	pl3_file_browser_cat_sel_changed,
	pl3_file_browser_fill_tree,
	pl3_file_browser_cat_popup,
	pl3_file_browser_cat_key_press,
	pl3_file_browser_add_go_menu,
	pl3_file_browser_key_press_event
};

gmpcPlugin file_browser_plug = {
	"File Browser",
	{1,1,1},
	GMPC_PLUGIN_PL_BROWSER,
	0,
	NULL,			                /* path*/
	NULL,			                /* init */
  pl3_file_browser_destroy,                                   /* destroy */
	&file_browser_gbp,		        /* Browser */
	pl3_file_browser_status_changed,        /* status changed */
	pl3_file_browser_connection_changed, 	/* connection changed */
	NULL,		                        /* Preferences */
	NULL,			                /* MetaData */
	NULL,                                   /* get enable */
	NULL                                    /* set enable */
};


extern GladeXML *pl3_xml;

GtkTreeRowReference *pl3_fb_tree_ref = NULL;
/* internal */
GtkWidget *pl3_fb_tree = NULL;
GtkWidget *pl3_fb_vbox = NULL;
GtkWidget *pl3_fb_tree_search = NULL;
GmpcMpdDataModel *pl3_fb_store2 = NULL;

static void pl3_file_browser_search_activate()
{
	GtkTreeModel *model = GTK_TREE_MODEL(pl3_fb_store2);
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(pl3_fb_tree));
	if (gtk_tree_selection_count_selected_rows (selection) == 1)            
	{
		GList *list = gtk_tree_selection_get_selected_rows (selection, &model);
		pl3_file_browser_row_activated(GTK_TREE_VIEW(pl3_fb_tree),(GtkTreePath *)list->data);	
		/* free list */
		g_list_foreach (list, (GFunc) gtk_tree_path_free, NULL);                        	
		g_list_free (list);
	}
}


static void pl3_file_browser_init()
{
	GtkWidget *pl3_fb_sw = NULL;

	pl3_fb_store2 = gmpc_mpddata_model_new();

	/* set up the tree */
	pl3_fb_tree= gmpc_mpddata_treeview_new("file-browser",TRUE, GTK_TREE_MODEL(pl3_fb_store2));

  /* setup signals */
	g_signal_connect(G_OBJECT(pl3_fb_tree), "row-activated",G_CALLBACK(pl3_file_browser_row_activated), NULL);
	g_signal_connect(G_OBJECT(pl3_fb_tree), "button-release-event", G_CALLBACK(pl3_file_browser_button_release_event), NULL);
	g_signal_connect(G_OBJECT(pl3_fb_tree), "button-press-event", G_CALLBACK(pl3_file_browser_button_press_event), NULL);
	g_signal_connect(G_OBJECT(pl3_fb_tree), "key-press-event", G_CALLBACK(pl3_file_browser_playlist_key_press), NULL);

	/* set up the scrolled window */
	pl3_fb_sw = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(pl3_fb_sw), GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(pl3_fb_sw), GTK_SHADOW_ETCHED_IN);
	gtk_container_add(GTK_CONTAINER(pl3_fb_sw), pl3_fb_tree);
	pl3_fb_vbox = gtk_vbox_new(FALSE, 6);


	gtk_box_pack_start(GTK_BOX(pl3_fb_vbox), pl3_fb_sw, TRUE, TRUE,0);
	gtk_widget_show_all(pl3_fb_sw);	
	pl3_fb_tree_search = treesearch_new(GTK_TREE_VIEW(pl3_fb_tree), MPDDATA_MODEL_COL_MARKUP);
	gtk_box_pack_end(GTK_BOX(pl3_fb_vbox), pl3_fb_tree_search, FALSE, TRUE,0);
	g_signal_connect(G_OBJECT(pl3_fb_tree_search),"result-activate", G_CALLBACK(pl3_file_browser_search_activate), NULL);

	/* set initial state */
	debug_printf(DEBUG_INFO,"initialized current playlist treeview\n");
	g_object_ref(G_OBJECT(pl3_fb_vbox));
}

static void pl3_file_browser_add_folder()
{
	GtkTreeSelection *selec = gtk_tree_view_get_selection(playlist3_get_category_tree_view());
	GtkTreeModel *model = GTK_TREE_MODEL(pl3_tree);
	GtkTreeIter iter;

	if(!mpd_check_connected(connection))
	{
		return;
	}
	if(gtk_tree_selection_get_selected(selec,&model, &iter))
	{
		char *path, *icon;
		char *message = NULL;
		gtk_tree_model_get(model, &iter, PL3_CAT_INT_ID, &path,3, &icon, -1);
	
	
		message = g_strdup_printf(_("Added folder '%s' recursively"), path);
		pl3_push_statusbar_message(message);
		q_free(message);
		if(strcmp("media-playlist", icon)) {                                 		
			mpd_playlist_queue_add(connection, path);
		}
		else {
			mpd_playlist_queue_load(connection, path);
		}
		mpd_playlist_queue_commit(connection);
		q_free(path);
		q_free(icon);
	}
}


static void pl3_file_browser_update_folder()
{
	GtkTreeSelection *selec = gtk_tree_view_get_selection((GtkTreeView *)glade_xml_get_widget (pl3_xml, "cat_tree"));
	GtkTreeModel *model = GTK_TREE_MODEL(pl3_tree);
	GtkTreeIter iter;

	if(!mpd_check_connected(connection))
	{
		return;
	}
	if(gtk_tree_selection_get_selected(selec,&model, &iter))
	{
		char *path;
		gtk_tree_model_get(model, &iter, PL3_CAT_INT_ID, &path, -1);
		if(path)
		{
			mpd_database_update_dir(connection, path);
			q_free(path);
		}
	}

}

static void pl3_file_browser_replace_folder()
{
	mpd_playlist_clear(connection);
	pl3_file_browser_add_folder();	
	mpd_player_play(connection);
}


/* add's the toplevel entry for the file browser, it also add's a fantom child */
static void pl3_file_browser_add(GtkWidget *cat_tree)
{
	GtkTreeIter iter,child;
	GtkTreePath *path;
	gtk_tree_store_append(pl3_tree, &iter, NULL);
	gtk_tree_store_set(pl3_tree, &iter, 
			PL3_CAT_TYPE, file_browser_plug.id,
			PL3_CAT_TITLE, _("Browse Filesystem"),
			PL3_CAT_INT_ID, "/",
			PL3_CAT_ICON_ID, "gtk-open",
			PL3_CAT_PROC, FALSE,
			PL3_CAT_ICON_SIZE,GTK_ICON_SIZE_DND,-1);
	/* add fantom child for lazy tree */
	gtk_tree_store_append(pl3_tree, &child, &iter);

	if(pl3_fb_tree_ref)
	{
		gtk_tree_row_reference_free(pl3_fb_tree_ref);
		pl3_fb_tree_ref = NULL;
	}
	path = gtk_tree_model_get_path(GTK_TREE_MODEL(pl3_tree), &iter);
	if(path)
	{
		pl3_fb_tree_ref = gtk_tree_row_reference_new(GTK_TREE_MODEL(pl3_tree),path);
		gtk_tree_path_free(path);
	}
}
static void pl3_file_browser_reupdate_folder(GtkTreeIter *iter)
{
	MpdData *data = NULL;
	gchar *path = NULL;
	gboolean temp = FALSE;
	gtk_tree_model_get(GTK_TREE_MODEL(pl3_tree), iter, PL3_CAT_PROC, &temp,PL3_CAT_INT_ID, &path, -1);
	if(path && temp)
	{
		GtkTreeIter child, child2,child3;
		data = mpd_database_get_directory(connection,path);
		g_free(path);
		if(gtk_tree_model_iter_children(GTK_TREE_MODEL(pl3_tree), &child, iter))
		{
			gchar *test_path = NULL;
			gboolean has_next = FALSE;	
			do {
				gtk_tree_model_get(GTK_TREE_MODEL(pl3_tree), &child, PL3_CAT_PROC, &temp, PL3_CAT_INT_ID, &test_path, -1);

				if(data == NULL)
				{	
					/* if no more data, remove the subdir */
					has_next = gtk_tree_store_remove(GTK_TREE_STORE(pl3_tree), &child);
				}
				else
				{
					int compare =0;
					/* get the next directory */
					while(data->type != MPD_DATA_TYPE_DIRECTORY) data = mpd_data_get_next(data);
					compare = strcmp(data->directory, test_path);
					if(compare < 0)
					{
						gchar *basename = g_path_get_basename (data->directory);
						gtk_tree_store_insert_before(GTK_TREE_STORE(pl3_tree), &child2,iter,&child);
						gtk_tree_store_set (GTK_TREE_STORE(pl3_tree), &child2,
								0, file_browser_plug.id,
								1, basename,
								2, data->directory,
								3, "gtk-open",
								4, FALSE,
								PL3_CAT_ICON_SIZE,1,
								-1);

						gtk_tree_store_append(pl3_tree, &child3, &child2);
						q_free(basename);


						/* if the new dir is smaller the temp, we add it. */
						data = mpd_data_get_next(data);
						has_next = TRUE;//gtk_tree_model_iter_next(pl3_tree, &child);					
					}
					if(compare > 0)
					{	
						/* if it's bigger, we delete the row */
						has_next = gtk_tree_store_remove(pl3_tree, &child);

					}else{
					   /* if equal we process children if available */
						if(temp)
						{
							pl3_file_browser_reupdate_folder(&child);
						}
						/* move to next entry in both */
						has_next = gtk_tree_model_iter_next(GTK_TREE_MODEL(pl3_tree), &child);					
						data = mpd_data_get_next(data);
					}
				}
				g_free(test_path);
			}while(has_next);
			if(data)
			{
				do{
					if(data->type == MPD_DATA_TYPE_DIRECTORY)
					{
						gchar *basename =
							g_path_get_basename (data->directory);
						gtk_tree_store_append(pl3_tree, &child2, iter);
						gtk_tree_store_set (pl3_tree, &child2,
								0, file_browser_plug.id,
								1, basename,
								2, data->directory,
								3, "gtk-open",
								4, FALSE,
								PL3_CAT_ICON_SIZE,1,
								-1);
						q_free(basename);
					}
				}while((data = mpd_data_get_next(data)));
			}
		}
	}
}
static void pl3_file_browser_reupdate()
{

	if(pl3_fb_tree_ref/* && pl3_cat_get_selected_browser() == file_browser_plug.id*/){
		GtkTreeIter iter;

		GtkTreeModel *model = gtk_tree_row_reference_get_model(pl3_fb_tree_ref);
		GtkTreePath *path = gtk_tree_row_reference_get_path(pl3_fb_tree_ref);

		if(gtk_tree_model_get_iter(model, &iter, path))
		{
			pl3_file_browser_reupdate_folder(&iter);
			if(pl3_cat_get_selected_browser() == file_browser_plug.id)
			{
				GtkTreeSelection *sel= gtk_tree_view_get_selection(playlist3_get_category_tree_view());
				if(gtk_tree_selection_get_selected(sel, &model, &iter))
				{
					pl3_file_browser_view_folder(&iter);
				}
			}
		}
		gtk_tree_path_free(path);
	}
}

static long unsigned pl3_file_browser_view_folder(GtkTreeIter *iter_cat)
{
	MpdData* data =NULL;
	char *path = NULL, *icon = NULL;
	long  unsigned time=0;

	/* check the connection state and when its valid proceed */
	if (!mpd_check_connected(connection))
	{
		return 0;
	}
	gtk_tree_model_get(GTK_TREE_MODEL(pl3_tree), iter_cat, 2 , &path,3,&icon, -1);
	if(strcmp("media-playlist",icon))
	{
		data = mpd_database_get_directory(connection, path);
	}
	else{
		debug_printf(DEBUG_INFO,"View Playlist\n");
		data = mpd_database_get_playlist_content(connection, path);
	}
	/* Check, and set the up arrow in the model */
	if(!strcmp(path, "/"))
		gmpc_mpddata_model_set_has_up(pl3_fb_store2, FALSE);	
	else
		gmpc_mpddata_model_set_has_up(pl3_fb_store2, TRUE);	
	time = gmpc_mpddata_model_set_mpd_data(pl3_fb_store2, data);
	q_free(path);
	q_free(icon);
	return time;
}


static void pl3_file_browser_fill_tree(GtkWidget *tree,GtkTreeIter *iter)
{
	char *path;
	MpdData *data = NULL;
	GtkTreeIter child,child2;
	gtk_tree_model_get(GTK_TREE_MODEL(pl3_tree),iter, 2, &path, -1);
	gtk_tree_store_set(pl3_tree, iter, 4, TRUE, -1);

	data = mpd_database_get_directory(connection, path);
	while (data != NULL)
	{
		if (data->type == MPD_DATA_TYPE_DIRECTORY)
		{
			gchar *basename =
				g_path_get_basename (data->directory);
			gtk_tree_store_append (pl3_tree, &child, iter);
			gtk_tree_store_set (pl3_tree, &child,
					0, file_browser_plug.id,
					1, basename,
					2, data->directory,
					3, "gtk-open",
					4, FALSE,
					PL3_CAT_ICON_SIZE,1,
					-1);
			gtk_tree_store_append(pl3_tree, &child2, &child);

			q_free (basename);
		}
		/*	else if(support_playlist && data->type == MPD_DATA_TYPE_PLAYLIST)
			{
			gchar *basename = g_path_get_basename(data->playlist);
			gtk_tree_store_append (pl3_tree, &child, iter);
			gtk_tree_store_set (pl3_tree, &child,
			0, file_browser_plug.id,
			1, basename,
			2, data->playlist,
			3, "media-playlist",
			4, TRUE,
			PL3_CAT_ICON_SIZE,1,
			-1);
			q_free(basename);
			}
			*/
		data = mpd_data_get_next(data);
	}
	if(gtk_tree_model_iter_children(GTK_TREE_MODEL(pl3_tree), &child, iter))
	{
		gtk_tree_store_remove(pl3_tree, &child);
	}
	q_free(path);
}



static int pl3_file_browser_cat_popup(GtkWidget *menu, int type,GtkWidget *tree, GdkEventButton *event)
{
	if(type == file_browser_plug.id)
	{
		/* here we have:  Add. Replace, (update?)*/
		GtkWidget *item;
		/* add the add widget */
		item = gtk_image_menu_item_new_from_stock(GTK_STOCK_ADD,NULL);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
		g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(pl3_file_browser_add_folder), NULL);

		/* add the replace widget */
		item = gtk_image_menu_item_new_with_label(_("Replace"));
		gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
				gtk_image_new_from_stock(GTK_STOCK_REDO, GTK_ICON_SIZE_MENU));
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
		g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(pl3_file_browser_replace_folder), NULL);

		{
			GtkTreeView *tree = playlist3_get_category_tree_view();
			GtkTreeModel *model = (GtkTreeModel *) playlist3_get_category_tree_store();
			GtkTreeSelection *selection  = gtk_tree_view_get_selection(tree);
			GtkTreeIter iter;
			if(gtk_tree_selection_get_selected(selection, &model, &iter))
			{
				char *icon = NULL;
				gtk_tree_model_get(model, &iter,3, &icon, -1);
				if(!strcmp("media-playlist", icon))
				{
					item = gtk_image_menu_item_new_from_stock(GTK_STOCK_DELETE, NULL);
					gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
					g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(pl3_file_browser_delete_playlist_from_right), NULL);
				}
				else
				{
					/* add the update widget */
					item = gtk_image_menu_item_new_with_label(_("Update"));
					gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
							gtk_image_new_from_stock(GTK_STOCK_REFRESH, GTK_ICON_SIZE_MENU));
					gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
					g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(pl3_file_browser_update_folder), NULL);	
				}
				q_free(icon);
			}
		}
		/* show everything and popup */
		return 1;
	}
	return 0;
}

static void pl3_file_browser_cat_key_press(GtkWidget *tree, GdkEventKey *event, int selected_type)
{
	if(selected_type != file_browser_plug.id) return; 
	if(event->state&GDK_CONTROL_MASK && event->keyval == GDK_Insert) {
		pl3_file_browser_replace_folder();
	} else if(event->keyval == GDK_Insert) {
		pl3_file_browser_add_folder();
	}
}

static int pl3_file_browser_playlist_key_press(GtkWidget *tree, GdkEventKey *event)
{
	if(event->state&GDK_CONTROL_MASK && event->keyval == GDK_Insert)
	{
		pl3_file_browser_replace_selected();
	} else if (event->state&GDK_CONTROL_MASK && event->keyval == GDK_f) {
		treesearch_start(TREESEARCH(pl3_fb_tree_search));
	} else if(event->keyval == GDK_Insert) {
		pl3_file_browser_add_selected();
	} else if(event->keyval == GDK_i && event->state&GDK_MOD1_MASK) {
		pl3_file_browser_show_info();
	} else if((event->state&(GDK_CONTROL_MASK|GDK_MOD1_MASK)) == 0 && ((event->keyval >= GDK_space && event->keyval <= GDK_z))) {
		char data[2];
		data[0] = (char)gdk_keyval_to_unicode(event->keyval);
		data[1] = '\0';
		treesearch_start(TREESEARCH(pl3_fb_tree_search));
		gtk_entry_set_text(GTK_ENTRY(TREESEARCH(pl3_fb_tree_search)->entry),data); 
		gtk_editable_set_position(GTK_EDITABLE(TREESEARCH(pl3_fb_tree_search)->entry),1);
		return TRUE;
	} else {
		return pl3_window_key_press_event(tree,event);
	}
	return TRUE;
}

static void pl3_file_browser_cat_sel_changed(GtkWidget *tree,GtkTreeIter *iter)
{
	long unsigned time= 0;
	gchar *string;

	time = pl3_file_browser_view_folder(iter);
	string = format_time(time);
	gtk_statusbar_push(GTK_STATUSBAR(glade_xml_get_widget(pl3_xml, "statusbar2")),0, string);
	q_free(string);
}

static void pl3_file_browser_selected(GtkWidget *container)
{
	if(pl3_fb_tree == NULL)
	{
		pl3_file_browser_init();
	}

	gtk_container_add(GTK_CONTAINER(container), pl3_fb_vbox);
	gtk_widget_grab_focus(pl3_fb_tree);
	gtk_widget_show(pl3_fb_vbox);
}
static void pl3_file_browser_unselected(GtkWidget *container)
{
	gtk_container_remove(GTK_CONTAINER(container), pl3_fb_vbox);
}

static void pl3_file_browser_show_info()
{
	GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW(pl3_fb_tree));
	GtkTreeSelection *selection =gtk_tree_view_get_selection (GTK_TREE_VIEW(pl3_fb_tree));
	if(!mpd_server_check_version(connection,0,12,0))
	{
		return;
	}
	if (gtk_tree_selection_count_selected_rows (selection) > 0)
	{
		GList *list = NULL;
		list = gtk_tree_selection_get_selected_rows (selection, &model);

		list = g_list_last (list);
		//		do
		{
			GtkTreeIter iter;
			char *path;
			int type;
			gtk_tree_model_get_iter (model, &iter, (GtkTreePath *) list->data);
			gtk_tree_model_get (GTK_TREE_MODEL(pl3_fb_store2), &iter,MPDDATA_MODEL_ROW_TYPE,&type, MPDDATA_MODEL_COL_PATH, &path, -1);
			if(type == MPD_DATA_TYPE_SONG)
			{
				mpd_Song *song = mpd_database_get_fileinfo(connection, path);
				if(song)
				{
					info2_activate();
					info2_fill_song_view(song->file);	
				}
				//					call_id3_window_song(song); 
			}
			q_free(path);
		}
		//		while ((list = g_list_previous (list)) && mpd_check_connected(connection));

		g_list_foreach (list, (GFunc) gtk_tree_path_free, NULL);
		g_list_free (list);
	}
}


static void pl3_file_browser_row_activated(GtkTreeView *tree, GtkTreePath *tp)
{
	GtkTreeIter iter;
	gchar *song_path;
	gint r_type;
	/*int playlist_length = mpd_playlist_get_playlist_length(connection);*/

	gtk_tree_model_get_iter(gtk_tree_view_get_model(tree), &iter, tp);
	gtk_tree_model_get(gtk_tree_view_get_model(tree), &iter, MPDDATA_MODEL_COL_PATH,&song_path, MPDDATA_MODEL_ROW_TYPE, &r_type, -1);
	if(song_path == NULL && r_type != -1)
	{
		return;
	}
	if(r_type == MPD_DATA_TYPE_PLAYLIST)
	{
		pl3_push_statusbar_message(_("Loaded playlist"));
		mpd_playlist_queue_load(connection, song_path);
		mpd_playlist_queue_commit(connection);
	}
	else if (r_type == MPD_DATA_TYPE_DIRECTORY)
	{
		GtkTreeSelection *selec = gtk_tree_view_get_selection((GtkTreeView *)glade_xml_get_widget (pl3_xml, "cat_tree"));
		GtkTreeModel *model = GTK_TREE_MODEL(pl3_tree);
		GtkTreeIter iter;

		if(gtk_tree_selection_get_selected(selec,&model, &iter))
		{
			GtkTreeIter citer;
			GtkTreePath *path = gtk_tree_model_get_path(model, &iter);
			gtk_tree_view_expand_row(GTK_TREE_VIEW(glade_xml_get_widget (pl3_xml, "cat_tree")), path, FALSE);
			gtk_tree_path_free(path);
			if(gtk_tree_model_iter_children(model, &citer, &iter))
			{
				do{
					char *name = NULL;
					char *type= NULL;
					gtk_tree_model_get(model, &citer, 2, &name, 3, &type,-1);
					if(strcmp(name, song_path) == 0 && strcmp(type, "gtk-open") == 0)
					{
						gtk_tree_selection_select_iter(selec,&citer);
						path = gtk_tree_model_get_path(model, &citer);
						gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(glade_xml_get_widget (pl3_xml, "cat_tree")), path,NULL,TRUE,0.5,0);
						gtk_tree_path_free(path);
					}
					q_free(name);
					q_free(type);
				}while(gtk_tree_model_iter_next(model, &citer));
			}

		}
	}
	else if (r_type == -1)
	{
		GtkTreeSelection *selec = gtk_tree_view_get_selection((GtkTreeView *)glade_xml_get_widget (pl3_xml, "cat_tree"));
		GtkTreeModel *model = GTK_TREE_MODEL(pl3_tree);
		GtkTreeIter iter;

		if(gtk_tree_selection_get_selected(selec,&model, &iter))
		{
			GtkTreeIter piter;
			if(gtk_tree_model_iter_parent(model, &piter, &iter))
			{
				GtkTreePath *path = NULL;
				gtk_tree_selection_select_iter(selec,&piter);
				path = gtk_tree_model_get_path(model, &piter);
				gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(glade_xml_get_widget (pl3_xml, "cat_tree")), path,NULL,TRUE,0.5,0);
				gtk_tree_path_free(path);
			}
		}
	}
	else
	{
		play_path(song_path);	
	}

	q_free(song_path);
}
static void pl3_file_browser_edit_columns(void)
{
	gmpc_mpddata_treeview_edit_columns(GMPC_MPDDATA_TREEVIEW(pl3_fb_tree));
}

static void pl3_file_browser_add_to_playlist(GtkWidget *menu)
{
	GtkTreeModel *model = GTK_TREE_MODEL(pl3_fb_store2);
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(pl3_fb_tree));
	gchar *data = g_object_get_data(G_OBJECT(menu), "playlist");
	GList *iter, *list = gtk_tree_selection_get_selected_rows (selection, &model);
	printf("add to playlist\n");
	if(list)
	{
		iter = g_list_first(list);
		do{
			GtkTreeIter giter;
			if(gtk_tree_model_get_iter(model, &giter, (GtkTreePath *)iter->data))
			{
				gchar *file = NULL;
				int type = 0;
				gtk_tree_model_get(model, &giter, MPDDATA_MODEL_COL_PATH, &file, MPDDATA_MODEL_ROW_TYPE, &type,-1);
				if(type == MPD_DATA_TYPE_SONG)
				{
					mpd_database_playlist_list_add(connection, data,file); 
				}
				else if(type == MPD_DATA_TYPE_DIRECTORY){
					MpdData *data2 =  mpd_database_get_directory_recursive(connection, file);
					for(;data2;data2 = mpd_data_get_next(data2))
					{
						if(data2->type == MPD_DATA_TYPE_SONG)
						{
							mpd_database_playlist_list_add(connection, data,data2->song->file); 
						}
					}
				}
				g_free(file);
			}
		}while((iter = g_list_next(iter)));

		g_list_foreach (list, (GFunc) gtk_tree_path_free, NULL);                        	
		g_list_free (list);
	}
}
static gboolean pl3_file_browser_button_press_event(GtkWidget *but, GdkEventButton *event)
{
	GtkTreePath *path = NULL;
	if(event->button == 3 && gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(but), event->x, event->y,&path,NULL,NULL,NULL))
	{	
		GtkTreeSelection *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(but));
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
static gboolean pl3_file_browser_button_release_event(GtkWidget *but, GdkEventButton *event)
{

	int has_item = 0;
	GtkWidget *item;
	GtkWidget *menu = NULL;
	GtkTreeSelection *sel = NULL;
	if(event->button != 3) return FALSE;
	menu = gtk_menu_new();
	sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(pl3_fb_tree));
	/* don't show it when where listing custom streams... 
	 * show always when version 12..  or when searching in playlist.
	 */
	if(gtk_tree_selection_count_selected_rows(sel) == 1)
	{
		GtkTreeModel *model = GTK_TREE_MODEL(pl3_fb_store2);
		GList *list = gtk_tree_selection_get_selected_rows(sel, &model);
		if(list != NULL)
		{
			GtkTreeIter iter;
			int row_type;
			char *path;
			list = g_list_first(list);
			gtk_tree_model_get_iter(model, &iter, list->data);
			gtk_tree_model_get(model, &iter,MPDDATA_MODEL_COL_PATH,&path,MPDDATA_MODEL_ROW_TYPE, &row_type, -1); 
			if(row_type == MPD_DATA_TYPE_SONG)
			{
				if(mpd_server_check_version(connection,0,12,0))
				{
					item = gtk_image_menu_item_new_from_stock(GTK_STOCK_DIALOG_INFO,NULL);
					gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
					g_signal_connect(G_OBJECT(item), "activate",
							G_CALLBACK(pl3_file_browser_show_info), NULL);
					has_item = 1;
				}
			}
			else if(row_type == MPD_DATA_TYPE_PLAYLIST)
			{
				item = gtk_image_menu_item_new_from_stock(GTK_STOCK_DELETE,NULL);
				gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
				g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(pl3_file_browser_delete_playlist), NULL);
				has_item = 1;
			}
			else if(row_type == MPD_DATA_TYPE_DIRECTORY)
			{
				item = gtk_image_menu_item_new_with_label(_("Update"));
				gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
						gtk_image_new_from_stock(GTK_STOCK_REFRESH, GTK_ICON_SIZE_MENU));
				gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
				g_signal_connect(G_OBJECT(item), "activate",
						G_CALLBACK(pl3_file_browser_update_folder), NULL);
				has_item = 1;
			}
			g_list_foreach (list,(GFunc) gtk_tree_path_free, NULL);
			g_list_free (list);
			q_free(path);
			if(row_type != -1)
			{
				/* replace the replace widget */
				item = gtk_image_menu_item_new_with_label(_("Replace"));
				gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
						gtk_image_new_from_stock(GTK_STOCK_REDO, GTK_ICON_SIZE_MENU));
				gtk_menu_shell_prepend(GTK_MENU_SHELL(menu), item);
				g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(pl3_file_browser_replace_selected), NULL);

				/* add the delete widget */
				item = gtk_image_menu_item_new_from_stock(GTK_STOCK_ADD,NULL);
				gtk_menu_shell_prepend(GTK_MENU_SHELL(menu), item);
				g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(pl3_file_browser_add_selected), NULL);


				playlist_editor_right_mouse(menu,pl3_file_browser_add_to_playlist);
				has_item = 1;
			}
			{
				mpd_Song *song = NULL;
				GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(pl3_fb_tree));
				GtkTreePath *path;
				GtkTreeIter iter;
				GList *list = gtk_tree_selection_get_selected_rows(gtk_tree_view_get_selection(GTK_TREE_VIEW(pl3_fb_tree)),&model);
				path = list->data;
				/* free result */
				g_list_free(list);
				if(path && gtk_tree_model_get_iter(model, &iter, path)) {
					gtk_tree_model_get(model, &iter, MPDDATA_MODEL_COL_MPDSONG, &song, -1);
					if(song)
					{
						submenu_for_song(menu, song);
					}

				}
				if(path)
					gtk_tree_path_free(path);
			}
		}
	}
	else
	{
		/* replace the replace widget */
		item = gtk_image_menu_item_new_with_label(_("Replace"));
		gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
				gtk_image_new_from_stock(GTK_STOCK_REDO, GTK_ICON_SIZE_MENU));
		gtk_menu_shell_prepend(GTK_MENU_SHELL(menu), item);
		g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(pl3_file_browser_replace_selected), NULL);

		/* add the delete widget */
		item = gtk_image_menu_item_new_from_stock(GTK_STOCK_ADD,NULL);
		gtk_menu_shell_prepend(GTK_MENU_SHELL(menu), item);
		g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(pl3_file_browser_add_selected), NULL);
		has_item = 1;
	}
	/*	if(has_item)*/
	{
		item = gtk_image_menu_item_new_with_label(_("Edit Columns"));
		gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
				gtk_image_new_from_stock(GTK_STOCK_EDIT, GTK_ICON_SIZE_MENU));
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
		g_signal_connect(G_OBJECT(item), "activate",
				G_CALLBACK(pl3_file_browser_edit_columns), NULL);

		gtk_widget_show_all(menu);
		gtk_menu_popup(GTK_MENU(menu), NULL, NULL,NULL, NULL, event->button, event->time);
		return TRUE;
	}
	/*	else{
		gtk_widget_destroy(menu);
		}
		*/
	return FALSE;
}
static void pl3_file_browser_replace_selected()
{
	mpd_playlist_clear(connection);
	pl3_file_browser_add_selected();
	mpd_player_play(connection);
}
static void pl3_file_browser_add_selected()
{
	GtkTreeIter iter;
	GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW(pl3_fb_tree));
	GtkTreeModel *model = GTK_TREE_MODEL (pl3_fb_store2);
	GList *rows = gtk_tree_selection_get_selected_rows (selection, &model);
	int songs=0;
	int dirs = 0;
	int pl = 0;
	/*gchar *message;*/
	if(rows != NULL)
	{
		gchar *name;
		gint type;
		GList *node = g_list_first(rows);
		do
		{
			GtkTreePath *path = node->data;
			gtk_tree_model_get_iter (model, &iter, path);
			gtk_tree_model_get (model, &iter, MPDDATA_MODEL_COL_PATH,&name,MPDDATA_MODEL_ROW_TYPE, &type, -1);
			/* does this bitmask thingy works ok? I think it hsould */
			if(type == MPD_DATA_TYPE_SONG || type == MPD_DATA_TYPE_DIRECTORY) 
			{
				/* add them to the add list */
				mpd_playlist_queue_add(connection, name);
				if(type&PL3_ENTRY_DIRECTORY) dirs++;
				if(type&PL3_ENTRY_SONG) songs++;
			}
			else if (type == MPD_DATA_TYPE_PLAYLIST)
			{
				mpd_playlist_queue_load(connection, name);
				pl++;
			}
			q_free(name);
		}while((node = g_list_next(node)) != NULL);
	}
	/* if there are items in the add list add them to the playlist */
	mpd_playlist_queue_commit(connection);
	if((songs+dirs+pl) != 0)
	{
		GString *string= g_string_new(_("Added"));
		if(songs)
			g_string_append_printf(string, " %i %s%c", songs, (songs >1)? _("songs"):_("songs"), (dirs+pl >0)?',':' ');
		if(dirs)
			g_string_append_printf(string, " %i %s%c", dirs, (dirs>1)?_("directories"):_("directory"), (pl>0)?',':' ');
		if(pl)
			g_string_append_printf(string, " %i %s", pl, (pl>1)?_("playlists"):_("playlist"));
		pl3_push_statusbar_message(string->str);
		g_string_free(string, TRUE);
	}

	g_list_foreach (rows, (GFunc) gtk_tree_path_free, NULL);
	g_list_free (rows);
}

static void pl3_file_browser_delete_playlist_from_right(GtkMenuItem *bt)
{
	GtkTreeView *tree = playlist3_get_category_tree_view();
	GtkTreeModel *model = (GtkTreeModel *) playlist3_get_category_tree_store();
	GtkTreeSelection *selection  = gtk_tree_view_get_selection(tree);
	GtkTreeIter iter;
	char *path= NULL;
	/* create a warning message dialog */
	GtkWidget *dialog = gtk_message_dialog_new (GTK_WINDOW
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


	if(gtk_tree_selection_get_selected(selection, &model, &iter))
	{
		char *icon = NULL;
		gtk_tree_model_get(model, &iter,3, &icon,2, &path, -1);
		if(path && strcmp("media-playlist", icon)) {
			if(path)
				q_free(path);
			path = NULL;
		}
		q_free(icon);
	}


	if(path == NULL){
		gtk_widget_destroy(dialog);
		return;
	}	

	switch (gtk_dialog_run (GTK_DIALOG (dialog)))
	{
		case GTK_RESPONSE_OK:
			mpd_database_delete_playlist(connection, path);
			pl3_file_browser_reupdate();
		default:
			break;
	}
	gtk_widget_destroy (GTK_WIDGET (dialog));
	q_free(path);
}

static void pl3_file_browser_delete_playlist(GtkMenuItem *bt)
{
	char *path= NULL;
	GtkTreeSelection *sel = NULL;
	/* create a warning message dialog */
	GtkWidget *dialog = gtk_message_dialog_new (GTK_WINDOW
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

	sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(pl3_fb_tree));
	if(gtk_tree_selection_count_selected_rows(sel) == 1)
	{
		GtkTreeModel *model = GTK_TREE_MODEL(pl3_fb_store2);
		GList *list = gtk_tree_selection_get_selected_rows(sel, &model);
		if(list != NULL)
		{
			GtkTreeIter iter;

			list = g_list_first(list);
			gtk_tree_model_get_iter(model, &iter, list->data);
			gtk_tree_model_get(model, &iter,MPDDATA_MODEL_COL_PATH,&path,-1); 
			g_list_foreach (list,(GFunc) gtk_tree_path_free, NULL);
			g_list_free (list);
		}
	}


	if(path == NULL){
		gtk_widget_destroy(dialog);
		return;
	}	

	switch (gtk_dialog_run (GTK_DIALOG (dialog)))
	{
		case GTK_RESPONSE_OK:
			mpd_database_delete_playlist(connection, path);
			pl3_file_browser_reupdate();
		default:
			break;
	}
	gtk_widget_destroy (GTK_WIDGET (dialog));
	q_free(path);
}

static void pl3_file_browser_disconnect()
{

	if(pl3_fb_tree_ref) {
		GtkTreeIter iter;
		GtkTreePath *path = gtk_tree_row_reference_get_path(pl3_fb_tree_ref);
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
	}
	if(pl3_fb_store2)
	{
		gmpc_mpddata_model_set_has_up(pl3_fb_store2, FALSE);	
		gmpc_mpddata_model_set_mpd_data(pl3_fb_store2, NULL);	
	}
}


static void pl3_file_browser_activate(void)
{
	GtkTreeSelection *selec = gtk_tree_view_get_selection((GtkTreeView *) playlist3_get_category_tree_view());

	GtkTreePath *path = gtk_tree_row_reference_get_path(pl3_fb_tree_ref); 
	if(path)
	{
		gtk_tree_selection_select_path(selec, path);
		gtk_tree_path_free(path);
	}
}


static int pl3_file_browser_add_go_menu(GtkWidget *menu)
{
	GtkWidget *item = NULL;

	item = gtk_image_menu_item_new_with_label(_("File Browser"));
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item), 
			gtk_image_new_from_stock("gtk-open", GTK_ICON_SIZE_MENU));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	g_signal_connect(G_OBJECT(item), "activate", 
			G_CALLBACK(pl3_file_browser_activate), NULL);
	return 1;
}

static void pl3_file_browser_connection_changed(MpdObj *mi, int connect, gpointer data)
{
	if(!connect) {
		pl3_file_browser_disconnect();
	}
}

static void pl3_file_browser_status_changed(MpdObj *mi,ChangedStatusType what, void *data)
{
	if(what&MPD_CST_DATABASE)
	{
		pl3_file_browser_reupdate();
	}
}	
static int pl3_file_browser_key_press_event(GtkWidget *mw, GdkEventKey *event, int type)
{
	if (event->keyval == GDK_F3) {
		pl3_file_browser_activate();
		return TRUE;
	}                                           	
	return FALSE;
}

static void pl3_file_browser_destroy(void)
{
	if(pl3_fb_vbox)
	{
		gtk_widget_destroy(pl3_fb_vbox);
	}
	if(pl3_fb_store2)
	{
		g_object_unref(pl3_fb_store2);
	}
	if(pl3_fb_tree_ref)
	{
		gtk_tree_row_reference_free(pl3_fb_tree_ref);
	}                                   
}

