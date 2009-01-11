/* Gnome Music Player Client (GMPC)
 * Copyright (C) 2004-2009 Qball Cow <qball@sarine.nl>
 * Project homepage: http://gmpcwiki.sarine.nl/
 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <stdlib.h>
#include <string.h>
#include "main.h"
#include "misc.h"
#include "playlist3.h"
#include "playlist3-file-browser.h"
#include "TreeSearchWidget.h"
#include "gmpc-mpddata-model.h"
#include "gmpc-mpddata-treeview.h"
#include "playlist3-playlist-editor.h"

static void pl3_file_browser_edit_columns(void);

static void pl3_file_browser_destroy(void);
static void pl3_file_browser_add(GtkWidget *cat_tree);
static void pl3_file_browser_unselected(GtkWidget *container);
static void pl3_file_browser_selected(GtkWidget *container);
static void pl3_file_browser_fill_tree(GtkWidget *tree,GtkTreeIter *iter, GtkTreePath *tpath, gpointer user_data);
static void pl3_file_browser_collapse_row(GtkTreeView *tree, GtkTreeIter *iter, GtkTreePath *path, gpointer user_data);
static int pl3_file_browser_cat_popup(GtkWidget *tree, GdkEventButton *event, gpointer user_data);
static gboolean pl3_file_browser_cat_key_press(GtkWidget *tree, GdkEventKey *event,gpointer data); 
static void pl3_file_browser_delete_playlist_from_right(GtkMenuItem *bt);
/* testing */
static void pl3_file_browser_reupdate(void);
static void pl3_file_browser_save_myself(void);
static int pl3_file_browser_add_go_menu(GtkWidget *menu);
static void pl3_file_browser_activate(void);
static gboolean pl3_file_browser_button_release_event(GtkWidget *but, GdkEventButton *event);
static void pl3_file_browser_row_activated(GtkTreeView *tree, GtkTreePath *tp);
static void pl3_file_browser_add_selected(void);
static void pl3_file_browser_replace_selected(void);
static int pl3_file_browser_playlist_key_press(GtkWidget *tree,GdkEventKey *event);
static void pl3_file_browser_show_info(void);
static void pl3_file_browser_view_folder(GtkTreeSelection *selection, gpointer user_data);
static void pl3_file_browser_update_folder(void);
static void pl3_file_browser_add_folder(void);
static void pl3_file_browser_delete_playlist(GtkMenuItem *bt);
static void pl3_file_browser_connection_changed(MpdObj *mi, int connect, gpointer data);
static void pl3_file_browser_status_changed(MpdObj *mi,ChangedStatusType what, void *data);
static void pl3_file_browser_disconnect(void);

GtkTreeRowReference *pl3_fb_tree_ref = NULL;

enum {
    PL3_FB_ICON = 0,
    PL3_FB_NAME = 1,
    PL3_FB_PATH = 2,
    PL3_FB_OPEN = 3
};
/**
 * Get/Set enabled
 */
static int pl3_file_browser_get_enabled(void)
{
	return cfg_get_single_value_as_int_with_default(config, "file-browser","enable", TRUE);
}
static void pl3_file_browser_set_enabled(int enabled)
{
	cfg_set_single_value_as_int(config, "file-browser","enable", enabled);
	if(enabled ) {
		GtkTreeView *tree = playlist3_get_category_tree_view();
		pl3_file_browser_add((GtkWidget *)tree);
	} else if (!enabled ) {
		pl3_file_browser_destroy();
	}
}

/**
 * Plugin structure
 */
gmpcPlBrowserPlugin file_browser_gbp = {
	.add            = pl3_file_browser_add,
	.selected       = pl3_file_browser_selected,
	.unselected     = pl3_file_browser_unselected,
	.add_go_menu    = pl3_file_browser_add_go_menu,
};

gmpcPlugin file_browser_plug = {
	.name = 						N_("File Browser"),
	.version = 						{1,1,1},
	.plugin_type = 					GMPC_PLUGIN_PL_BROWSER|GMPC_INTERNALL,
  	.destroy = 						pl3_file_browser_destroy,
	.browser = 						&file_browser_gbp,
	.mpd_status_changed = 			pl3_file_browser_status_changed,
	.mpd_connection_changed = 		pl3_file_browser_connection_changed, 
	.get_enabled = 					pl3_file_browser_get_enabled,
	.set_enabled = 					pl3_file_browser_set_enabled,
	.save_yourself = 				pl3_file_browser_save_myself
};


/* internal */
GtkWidget *pl3_fb_tree = NULL;
GtkWidget *pl3_fb_vbox = NULL;
GtkWidget *pl3_fb_tree_search = NULL;
GmpcMpdDataModel *pl3_fb_store2 = NULL;
static GtkTreeStore *pl3_fb_dir_store = NULL;
static GtkWidget *pl3_fb_dir_tree = NULL;

static void pl3_file_browser_search_activate(void)
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
static void pl3_file_browser_dir_row_activated(GtkTreeView *tree, GtkTreePath *tp, GtkTreeViewColumn *col,gpointer user_data)
{
	if(!mpd_check_connected(connection))
		return;
	if(gtk_tree_view_row_expanded(tree,tp))
		gtk_tree_view_collapse_row(tree,tp);
	else
		gtk_tree_view_expand_row(tree,tp,FALSE);
}
static void playtime_changed(GmpcMpdDataModel *model, gulong playtime)
{
    if(pl3_cat_get_selected_browser() == file_browser_plug.id)
    {
        playlist3_show_playtime(playtime);
    }
}

static void pl3_file_browser_init(void)
{
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;
	GtkWidget *pl3_fb_sw = NULL;
    GtkWidget *vbox,*sw,*tree;
    int pos;

	pl3_fb_store2 = gmpc_mpddata_model_new();
    gmpc_mpddata_model_disable_image(GMPC_MPDDATA_MODEL(pl3_fb_store2));
    g_signal_connect(G_OBJECT(pl3_fb_store2), "playtime_changed", G_CALLBACK(playtime_changed), NULL);


    pos = cfg_get_single_value_as_int_with_default(config, "file-browser", "pane-pos", 150);
    pl3_fb_vbox = gtk_hpaned_new();
    gtk_paned_set_position(GTK_PANED(pl3_fb_vbox), pos);
	vbox = gtk_vbox_new(FALSE, 6);


    /* icon id, name, full path */
    pl3_fb_dir_store = gtk_tree_store_new(4, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,G_TYPE_BOOLEAN);

    sw = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw), GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw), GTK_SHADOW_ETCHED_IN);
    pl3_fb_dir_tree = tree = gtk_tree_view_new_with_model (GTK_TREE_MODEL(pl3_fb_dir_store));
    gtk_tree_view_set_enable_tree_lines(GTK_TREE_VIEW(tree), TRUE);
    gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(tree), TRUE);
	column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(column, _("Directories"));
	renderer = gtk_cell_renderer_pixbuf_new();
	gtk_tree_view_column_pack_start(column, renderer, FALSE);
    gtk_tree_view_set_fixed_height_mode(GTK_TREE_VIEW(tree), TRUE);
    gtk_tree_view_column_add_attribute(column, renderer, "icon-name",PL3_FB_ICON);
    gtk_tree_view_column_set_sizing(column , GTK_TREE_VIEW_COLUMN_FIXED);
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	gtk_tree_view_column_add_attribute(column, renderer, "text", PL3_FB_NAME);
	gtk_tree_view_insert_column(GTK_TREE_VIEW(tree),column, -1);
	/* set the search column */
	gtk_tree_view_set_search_column(GTK_TREE_VIEW(tree), PL3_FB_NAME);

    g_signal_connect(G_OBJECT(tree), "row-expanded", G_CALLBACK(pl3_file_browser_fill_tree), NULL);
    g_signal_connect(G_OBJECT(tree), "row-collapsed", G_CALLBACK(pl3_file_browser_collapse_row), NULL);
    g_signal_connect(G_OBJECT(gtk_tree_view_get_selection(GTK_TREE_VIEW(tree))), "changed", G_CALLBACK(pl3_file_browser_view_folder), NULL);
    g_signal_connect(G_OBJECT(tree), "button-release-event", G_CALLBACK(pl3_file_browser_cat_popup), NULL);
    g_signal_connect(G_OBJECT(tree), "key-press-event", G_CALLBACK(pl3_file_browser_cat_key_press), NULL);
    g_signal_connect(G_OBJECT(tree), "row-activated", G_CALLBACK(pl3_file_browser_dir_row_activated), NULL);

    gtk_container_add(GTK_CONTAINER(sw), tree);
    gtk_widget_show_all(sw);
    gtk_paned_add1(GTK_PANED(pl3_fb_vbox), sw);
    
	/* set up the tree */
	pl3_fb_tree= gmpc_mpddata_treeview_new("file-browser",TRUE, GTK_TREE_MODEL(pl3_fb_store2));
    gmpc_mpddata_treeview_enable_click_fix(GMPC_MPDDATA_TREEVIEW(pl3_fb_tree));
  /* setup signals */
	g_signal_connect(G_OBJECT(pl3_fb_tree), "row-activated",G_CALLBACK(pl3_file_browser_row_activated), NULL);
	g_signal_connect(G_OBJECT(pl3_fb_tree), "button-release-event", G_CALLBACK(pl3_file_browser_button_release_event), NULL);
	g_signal_connect(G_OBJECT(pl3_fb_tree), "key-press-event", G_CALLBACK(pl3_file_browser_playlist_key_press), NULL);

	/* set up the scrolled window */
	pl3_fb_sw = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(pl3_fb_sw), GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(pl3_fb_sw), GTK_SHADOW_ETCHED_IN);
	gtk_container_add(GTK_CONTAINER(pl3_fb_sw), pl3_fb_tree);



	gtk_box_pack_start(GTK_BOX(vbox), pl3_fb_sw, TRUE, TRUE,0);
	gtk_widget_show_all(pl3_fb_sw);	
	pl3_fb_tree_search = treesearch_new(GTK_TREE_VIEW(pl3_fb_tree), MPDDATA_MODEL_COL_MARKUP);
	gtk_box_pack_end(GTK_BOX(vbox), pl3_fb_tree_search, FALSE, TRUE,0);
	g_signal_connect(G_OBJECT(pl3_fb_tree_search),"result-activate", G_CALLBACK(pl3_file_browser_search_activate), NULL);

    gtk_paned_add2(GTK_PANED(pl3_fb_vbox), vbox);
	/* set initial state */
	debug_printf(DEBUG_INFO,"initialized current playlist treeview\n");
    gtk_widget_show(vbox);
    gtk_widget_show(pl3_fb_vbox);
	g_object_ref(G_OBJECT(pl3_fb_vbox));
}

static void pl3_file_browser_add_folder(void)
{
	GtkTreeSelection *selec = gtk_tree_view_get_selection(GTK_TREE_VIEW(pl3_fb_dir_tree));
	GtkTreeModel *model = GTK_TREE_MODEL(pl3_fb_dir_store);
	GtkTreeIter iter;

	if(!mpd_check_connected(connection))
	{
		return;
	}
	if(gtk_tree_selection_get_selected(selec,&model, &iter))
	{
		char *path, *icon;
		char *message = NULL;
		gtk_tree_model_get(model, &iter, PL3_FB_PATH, &path,PL3_FB_ICON, &icon, -1);
	
	
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


static void pl3_file_browser_update_folder(void)
{
	GtkTreeSelection *selec = gtk_tree_view_get_selection((GtkTreeView *)pl3_fb_dir_tree);
	GtkTreeModel *model = GTK_TREE_MODEL(pl3_fb_dir_store);
	GtkTreeIter iter;

	if(!mpd_check_connected(connection))
	{
		return;
	}
	if(gtk_tree_selection_get_selected(selec,&model, &iter))
	{
		char *path;
		gtk_tree_model_get(model, &iter, PL3_FB_PATH, &path, -1);
		if(path)
		{
			mpd_database_update_dir(connection, path);
			q_free(path);
		}
	}

}

static void pl3_file_browser_update_folder_left_pane(void)
{
	GtkTreeModel *model = GTK_TREE_MODEL(pl3_fb_store2);
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(pl3_fb_tree));
	GtkTreeIter iter;

	if(!mpd_check_connected(connection))
		return;

	if (gtk_tree_selection_count_selected_rows (selection) == 1)            
	{
		GList *list = gtk_tree_selection_get_selected_rows (selection, &model);
		/* free list */
        if(gtk_tree_model_get_iter(model, &iter,list->data))
        {
            char *path;
            gtk_tree_model_get(model, &iter, MPDDATA_MODEL_COL_PATH, &path, -1);
            if(path)
            {
                mpd_database_update_dir(connection, path);
                q_free(path);
            }
        }
        g_list_foreach (list, (GFunc) gtk_tree_path_free, NULL);                        	
		g_list_free (list);
	}
}



static void pl3_file_browser_replace_folder(void)
{
	mpd_playlist_clear(connection);
    if(mpd_check_connected(connection))
    {
        pl3_file_browser_add_folder();	
        mpd_player_play(connection);
    }
}


/* add's the toplevel entry for the file browser, it also add's a fantom child */
static void pl3_file_browser_add(GtkWidget *cat_tree)
{
	GtkTreeIter iter,child;
	GtkTreePath *path;
	gint pos = cfg_get_single_value_as_int_with_default(config, "file-browser","position",2);
	playlist3_insert_browser(&iter, pos);
	gtk_list_store_set(GTK_LIST_STORE(pl3_tree), &iter, 
			PL3_CAT_TYPE, file_browser_plug.id,
			PL3_CAT_TITLE, _("File Browser"),
			PL3_CAT_INT_ID, "/",
			PL3_CAT_ICON_ID, "gtk-open",
			PL3_CAT_PROC, FALSE,
			PL3_CAT_ICON_SIZE,GTK_ICON_SIZE_DND,-1);
	/* add fantom child for lazy tree */

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


	if(pl3_fb_tree == NULL)
	{
		pl3_file_browser_init();
	}


    gtk_tree_store_append(pl3_fb_dir_store, &iter, NULL);
    gtk_tree_store_set(pl3_fb_dir_store, &iter, 
			PL3_FB_ICON, "gtk-open",
            PL3_FB_NAME, "/",
            PL3_FB_PATH,"/",
            PL3_FB_OPEN,FALSE,
			-1);
    gtk_tree_store_append(pl3_fb_dir_store, &child, &iter);


}
         
static void pl3_file_browser_reupdate_folder(GtkTreeIter *iter)
{
	MpdData *data = NULL;
	gchar *path = NULL;
	gboolean temp = FALSE;
    GtkTreeModel *model = GTK_TREE_MODEL(pl3_fb_dir_store);
	gtk_tree_model_get(GTK_TREE_MODEL(pl3_fb_dir_store), iter, 3, &temp,2, &path, -1);
	if(path && temp)
	{
		GtkTreeIter child, child2,child3;
		data = mpd_database_get_directory(connection,path);
		g_free(path);
		if(gtk_tree_model_iter_children(model, &child, iter))
		{
			gchar *test_path = NULL;
			gboolean has_next = FALSE;	
			do {
				gtk_tree_model_get(model, &child, 3, &temp, 2, &test_path, -1);

				if(data == NULL)
				{	
					/* if no more data, remove the subdir */
					has_next = gtk_tree_store_remove(pl3_fb_dir_store, &child);
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
						gtk_tree_store_insert_before(pl3_fb_dir_store, &child2,iter,&child);
						gtk_tree_store_set (pl3_fb_dir_store, &child2,
                                PL3_FB_ICON, "gtk-open",
								PL3_FB_NAME, basename,
								PL3_FB_PATH, data->directory,
								PL3_FB_OPEN, FALSE,
								-1);

						gtk_tree_store_append(pl3_fb_dir_store, &child3, &child2);
						q_free(basename);

						/* if the new dir is smaller the temp, we add it. */
						data = mpd_data_get_next(data);
						has_next = TRUE;
					}
                    else if(compare > 0)
					{	
						/* if it's bigger, we delete the row */
						has_next = gtk_tree_store_remove(pl3_fb_dir_store, &child);

					}else{
					   /* if equal we process children if available */
						if(temp)
						{
							pl3_file_browser_reupdate_folder(&child);
						}
						/* move to next entry in both */
						has_next = gtk_tree_model_iter_next(model, &child);					
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
						gtk_tree_store_append(pl3_fb_dir_store, &child2, iter);
						gtk_tree_store_set (pl3_fb_dir_store, &child2,
                                PL3_FB_ICON, "gtk-open",
                                PL3_FB_NAME, basename,
								PL3_FB_PATH, data->directory,
								PL3_FB_OPEN, FALSE,
								-1);
						gtk_tree_store_append(pl3_fb_dir_store, &child3, &child2);
						q_free(basename);
					}
				}while((data = mpd_data_get_next(data)));
			}
		}
	}
}
static void pl3_file_browser_reupdate(void)
{

	if(pl3_fb_vbox)
    {
		GtkTreeIter iter;

		GtkTreeModel *model = GTK_TREE_MODEL(pl3_fb_dir_store); 

		if(gtk_tree_model_get_iter_first(model, &iter))
		{
			pl3_file_browser_reupdate_folder(&iter);
            pl3_file_browser_view_folder(gtk_tree_view_get_selection(GTK_TREE_VIEW(pl3_fb_dir_tree)), NULL);
        }
	}
}

static void pl3_file_browser_view_folder(GtkTreeSelection *selection, gpointer user_data)
{
	MpdData* data =NULL;
	char *path = NULL, *icon = NULL;
    GtkTreeModel *model = GTK_TREE_MODEL(pl3_fb_dir_store);
    GtkTreeIter iter_cat;
    /* Clear the view */
    gmpc_mpddata_model_set_mpd_data(pl3_fb_store2, NULL);

    if(!gtk_tree_selection_get_selected(selection, &model, &iter_cat))
        return;

	/* check the connection state and when its valid proceed */
	if (!mpd_check_connected(connection))
	{
		return;
	}
	gtk_tree_model_get(GTK_TREE_MODEL(pl3_fb_dir_store), &iter_cat, PL3_FB_PATH , &path,PL3_FB_ICON,&icon, -1);
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
	gmpc_mpddata_model_set_mpd_data(pl3_fb_store2, data);
	q_free(path);
	q_free(icon);
	return ;
}

static void pl3_file_browser_collapse_row(GtkTreeView *tree, GtkTreeIter *iter, GtkTreePath *path, gpointer user_data)
{
    GtkTreeIter child;
    int valid = gtk_tree_model_iter_children(GTK_TREE_MODEL(pl3_fb_dir_store), &child, iter);
    if(!cfg_get_single_value_as_int_with_default(config, "file-browser", "extra-lazy",TRUE))
        return;

    while(valid){
        valid = gtk_tree_store_remove(pl3_fb_dir_store,&child);
    }
    /* set unopened */
    gtk_tree_store_set(pl3_fb_dir_store,iter,PL3_FB_OPEN,FALSE,-1);
    /* add phantom child */
    gtk_tree_store_append(pl3_fb_dir_store, &child, iter);
}

static void pl3_file_browser_fill_tree(GtkWidget *tree,GtkTreeIter *iter, GtkTreePath *tpath, gpointer user_data)
{
    char *path;
    MpdData *data = NULL;
    GtkTreeIter child,child2;
    gboolean open;
    gtk_tree_model_get(GTK_TREE_MODEL(pl3_fb_dir_store),iter, PL3_FB_PATH, &path, PL3_FB_OPEN, &open,-1);
    gtk_tree_store_set(pl3_fb_dir_store, iter, PL3_FB_OPEN, TRUE, -1);
    if(open == FALSE)
    {
        data = mpd_database_get_directory(connection, path);
        while (data != NULL)
        {
            if (data->type == MPD_DATA_TYPE_DIRECTORY)
            {
                gchar *basename =
                    g_path_get_basename (data->directory);
                gtk_tree_store_append (pl3_fb_dir_store, &child, iter);
                gtk_tree_store_set (pl3_fb_dir_store, &child,
                        PL3_FB_ICON, "gtk-open",
                        PL3_FB_NAME, basename,
                        PL3_FB_PATH, data->directory,
                        PL3_FB_OPEN, FALSE,
                        -1);
                gtk_tree_store_append(pl3_fb_dir_store, &child2, &child);

                q_free (basename);
            }
            data = mpd_data_get_next(data);
        }
        if(gtk_tree_model_iter_children(GTK_TREE_MODEL(pl3_fb_dir_store), &child, iter))
        {
            gtk_tree_store_remove(pl3_fb_dir_store, &child);
        }
    }
    q_free(path);
}



static int pl3_file_browser_cat_popup(GtkWidget *wid, GdkEventButton *event, gpointer user_data)
{
    GtkWidget *menu;
    if(event->button == 3){

        /* here we have:  Add. Replace, (update?)*/
        GtkWidget *item;
        menu = gtk_menu_new();
        /* add the add widget */
        item = gtk_image_menu_item_new_from_stock(GTK_STOCK_ADD,NULL);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
        g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(pl3_file_browser_add_folder), NULL);

        /* add the replace widget */
        item = gtk_image_menu_item_new_with_label(_("Replace"));
        gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
                gtk_image_new_from_stock(GTK_STOCK_REDO, GTK_ICON_SIZE_MENU));
        gtk_menu_item_set_accel_path(GTK_MENU_ITEM(item), NULL);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
        g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(pl3_file_browser_replace_folder), NULL);

        {
            GtkTreeView *tree = GTK_TREE_VIEW(pl3_fb_dir_tree); 
            GtkTreeModel *model = (GtkTreeModel *)pl3_fb_dir_store; 
            GtkTreeSelection *selection  = gtk_tree_view_get_selection(tree);
            GtkTreeIter iter;
            if(gtk_tree_selection_get_selected(selection, &model, &iter))
            {
                char *icon = NULL;
                gtk_tree_model_get(model, &iter,PL3_FB_ICON, &icon, -1);
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
        gtk_widget_show_all(menu);
        gtk_menu_popup(GTK_MENU(menu), NULL, NULL,NULL, NULL, 0, event->time);
        /* show everything and popup */
        return TRUE;
    }
    return FALSE;
}

static gboolean pl3_file_browser_cat_key_press(GtkWidget *tree, GdkEventKey *event,gpointer data)
{
    if(event->state&GDK_CONTROL_MASK && event->keyval == GDK_Insert) {
        pl3_file_browser_replace_folder();
    } else if(event->keyval == GDK_Insert) {
        pl3_file_browser_add_folder();
    }
    return FALSE;
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
    }
    return FALSE;
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

    playlist3_show_playtime(gmpc_mpddata_model_get_playtime(GMPC_MPDDATA_MODEL(pl3_fb_store2))); 
}
static void pl3_file_browser_unselected(GtkWidget *container)
{
    gtk_container_remove(GTK_CONTAINER(container), pl3_fb_vbox);
}

static void pl3_file_browser_show_info(void)
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
                    info2_fill_song_view(song);	
                    mpd_freeSong(song);
                }
            }
            q_free(path);
        }
        g_list_foreach (list, (GFunc) gtk_tree_path_free, NULL);
        g_list_free (list);
    }
}


static void pl3_file_browser_row_activated(GtkTreeView *tree, GtkTreePath *tp)
{
    GtkTreeIter iter;
    gchar *song_path;
    gint r_type;

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
        GtkTreeSelection *selec = gtk_tree_view_get_selection(GTK_TREE_VIEW(pl3_fb_dir_tree));
        GtkTreeModel *model = GTK_TREE_MODEL(pl3_fb_dir_store);

        if(gtk_tree_selection_get_selected(selec,&model, &iter))
        {
            GtkTreeIter citer;
            GtkTreePath *path = gtk_tree_model_get_path(model, &iter);
            gtk_tree_view_expand_row(GTK_TREE_VIEW(pl3_fb_dir_tree), path, FALSE);
            gtk_tree_path_free(path);
            if(gtk_tree_model_iter_children(model, &citer, &iter))
            {
                do{
                    char *name = NULL;
                    char *type= NULL;
                    gtk_tree_model_get(model, &citer, PL3_FB_PATH, &name, PL3_FB_ICON, &type,-1);
                    if(strcmp(name, song_path) == 0 && strcmp(type, "gtk-open") == 0)
                    {
                        gtk_tree_selection_select_iter(selec,&citer);
                        path = gtk_tree_model_get_path(model, &citer);
                        gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(pl3_fb_dir_tree), path,NULL,TRUE,0.5,0);
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
        GtkTreeSelection *selec = gtk_tree_view_get_selection((GtkTreeView *)pl3_fb_dir_tree);
        GtkTreeModel *model = GTK_TREE_MODEL(pl3_fb_dir_store);

        if(gtk_tree_selection_get_selected(selec,&model, &iter))
        {
            GtkTreeIter piter;
            if(gtk_tree_model_iter_parent(model, &piter, &iter))
            {
                GtkTreePath *path = NULL;
                gtk_tree_selection_select_iter(selec,&piter);
                path = gtk_tree_model_get_path(model, &piter);
                gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(pl3_fb_dir_tree), path,NULL,TRUE,0.5,0);
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
        mpd_Song *song = NULL;
        GtkTreeModel *model = GTK_TREE_MODEL(pl3_fb_store2);
        GList *list = gtk_tree_selection_get_selected_rows(sel, &model);
        if(list != NULL)
        {
            GtkTreeIter iter;
            int row_type;
            char *path;
            GtkTreePath *tree_path;
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
                        G_CALLBACK(pl3_file_browser_update_folder_left_pane), NULL);
                has_item = 1;
            }
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

///            model = gtk_tree_view_get_model(GTK_TREE_VIEW(pl3_fb_tree));
  //          list = gtk_tree_selection_get_selected_rows(sel,&model);
    //        if(list)
                tree_path = list->data;
                if(tree_path && gtk_tree_model_get_iter(model, &iter, tree_path)) {
                    gtk_tree_model_get(model, &iter, MPDDATA_MODEL_COL_MPDSONG, &song, -1);
                    if(song)
                    {
                        submenu_for_song(menu, song);
                    }

                }
            //    g_list_foreach (list, (GFunc)gtk_tree_path_free, NULL);
              //  g_list_free(list);
            g_list_foreach (list,(GFunc) gtk_tree_path_free, NULL);
            g_list_free (list);
            q_free(path);
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
    gmpc_mpddata_treeview_right_mouse_intergration(GMPC_MPDDATA_TREEVIEW(pl3_fb_tree), GTK_MENU(menu));
    /*	if(has_item)*/
    {
        item = gtk_image_menu_item_new_with_label(_("Edit Columns"));
        gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
                gtk_image_new_from_stock(GTK_STOCK_EDIT, GTK_ICON_SIZE_MENU));
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
        g_signal_connect(G_OBJECT(item), "activate",
                G_CALLBACK(pl3_file_browser_edit_columns), NULL);

        gtk_widget_show_all(menu);
        gtk_menu_popup(GTK_MENU(menu), NULL, NULL,NULL, NULL, 0, event->time);
        return TRUE;
    }
    /*	else{
        gtk_widget_destroy(menu);
        }
        */
    return FALSE;
}
static void pl3_file_browser_replace_selected(void)
{
    mpd_playlist_clear(connection);
    if(mpd_check_connected(connection))
    {
        pl3_file_browser_add_selected();
        mpd_player_play(connection);
    }
}
static void pl3_file_browser_add_selected(void)
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
                if(type==MPD_DATA_TYPE_DIRECTORY) dirs++;
                if(type==MPD_DATA_TYPE_SONG) songs++;
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
            g_string_append_printf(string, " %i %s%c", songs, ngettext("song", "songs", songs), (dirs+pl >0)?',':' ');
        if(dirs)
            g_string_append_printf(string, " %i %s%c", dirs, ngettext("directory", "directories", dirs), (pl>0)?',':' ');
        if(pl)
            g_string_append_printf(string, " %i %s", pl, ngettext("playlist", "playlists", pl));
        pl3_push_statusbar_message(string->str);
        g_string_free(string, TRUE);
    }

    g_list_foreach (rows, (GFunc) gtk_tree_path_free, NULL);
    g_list_free (rows);
}

static void pl3_file_browser_delete_playlist_from_right(GtkMenuItem *bt)
{
    GtkTreeView *tree = GTK_TREE_VIEW(pl3_fb_dir_tree); 
    GtkTreeModel *model = (GtkTreeModel *) pl3_fb_dir_store; 
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

static void pl3_file_browser_disconnect(void)
{

    if(pl3_fb_tree_ref) {
        GtkTreeIter iter;
        GtkTreeIter child;
        if(gtk_tree_model_get_iter_first(GTK_TREE_MODEL(pl3_fb_dir_store), &iter))
        {
            int valid = gtk_tree_model_iter_children(GTK_TREE_MODEL(pl3_fb_dir_store), &child, &iter);

            while(valid){
                valid = gtk_tree_store_remove(pl3_fb_dir_store,&child);
            }
            /* set unopened */
            gtk_tree_store_set(pl3_fb_dir_store,&iter,PL3_FB_OPEN,FALSE,-1);
            /* add phantom child */
            gtk_tree_store_append(pl3_fb_dir_store, &child, &iter);
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
    if(!pl3_file_browser_get_enabled())
        return 0;

    item = gtk_image_menu_item_new_with_label(_("File Browser"));
    gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item), 
            gtk_image_new_from_stock("gtk-open", GTK_ICON_SIZE_MENU));
    gtk_widget_add_accelerator(GTK_WIDGET(item), 
            "activate",
            gtk_menu_get_accel_group(GTK_MENU(menu)), 
            GDK_F2, 0,
            GTK_ACCEL_VISIBLE);

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
static void pl3_file_browser_save_myself(void)
{
    if(pl3_fb_tree_ref)
    {
        GtkTreePath *path = gtk_tree_row_reference_get_path(pl3_fb_tree_ref);
        if(path)
        {
            gint *indices = gtk_tree_path_get_indices(path);
            debug_printf(DEBUG_INFO,"Saving myself to position: %i\n", indices[0]);
            cfg_set_single_value_as_int(config, "file-browser","position",indices[0]);
            gtk_tree_path_free(path);
        }
    }
    if(pl3_fb_vbox)
    {
        int pos = gtk_paned_get_position(GTK_PANED(pl3_fb_vbox));
        cfg_set_single_value_as_int(config, "file-browser", "pane-pos", pos);
    }
}
