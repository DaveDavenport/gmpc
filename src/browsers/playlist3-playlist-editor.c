/* Gnome Music Player Client (GMPC)
 * Copyright (C) 2004-2010 Qball Cow <qball@sarine.nl>
 * Project homepage: http://gmpc.wikia.com/
 
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
#include <glib/gstdio.h>
#include <gdk/gdkkeysyms.h>
#include <libmpd/libmpd.h>
#include <string.h>
#include "main.h"
#include "playlist3.h"
#include "gmpc-mpddata-model.h"
#include "gmpc-mpddata-treeview.h"
#include "gmpc-mpddata-model-sort.h"
#include "gmpc-extras.h"
#include "browsers/playlist3-playlist-editor.h"
#include "Widgets/mpd-async-request.h"

#define DEFAULT_MARKUP_BROWSER 	"[%name%: &[%artist% - ]%title%]|%name%|[%artist% - ]%title%|%shortfile%|"

static GtkTreeRowReference *playlist_editor_browser_ref = NULL;
static GtkWidget *playlist_editor_browser = NULL;
static GtkWidget *playlist_editor_song_tree = NULL;
static void playlist_editor_status_changed(MpdObj *mi, ChangedStatusType what, void *data);
static int playlist_editor_add_go_menu(GtkWidget *);
GtkWidget *playlist_editor_icon_view = NULL;
GmpcMpdDataModelSort *playlist_editor_list_store = NULL;
void playlist_editor_fill_list_real(void);
static void playlist_editor_save_myself(void);

enum {
	PL_NAME,
    PL_MTIME,
	PL_IMAGE,
	PL_NUM_COLS
};

GtkListStore *playlist_editor_store = NULL;

/**
 * Enable/Disable plugin
 */
static int playlist_editor_get_enabled(void)
{
	return cfg_get_single_value_as_int_with_default(config, "playlist-plugin", "enable", TRUE);
}
static void playlist_editor_set_enabled(int enabled)
{
	cfg_set_single_value_as_int(config, "playlist-plugin", "enable", enabled);
};


/**
 * Browser extention 
 */
gmpcPlBrowserPlugin playlist_editor_gbp = {
  .add = playlist_editor_browser_add,   /* Add */
  .selected = playlist_editor_browser_selected,   /* Selected */
  .unselected = playlist_editor_browser_unselected,   /* Unselected */
  .cat_right_mouse_menu = playlist_editor_browser_cat_menu,   /* */
  .add_go_menu = playlist_editor_add_go_menu
};

gmpcPlugin playlist_editor_plugin = {
  .name 					= "Playlist Editor",
  .version					=  {0,15,0},
  .plugin_type 				= GMPC_PLUGIN_PL_BROWSER|GMPC_INTERNALL,
  .init						= playlist_editor_init,
  .destroy 					= playlist_editor_destroy, 
  .browser					=  &playlist_editor_gbp,
  .mpd_status_changed		= playlist_editor_status_changed,
  .mpd_connection_changed 	= playlist_editor_conn_changed,
  .get_enabled				= playlist_editor_get_enabled,
  .set_enabled				= playlist_editor_set_enabled,
  .save_yourself			= playlist_editor_save_myself
};

/**
 * Init plugin
 */
void playlist_editor_init(void)
{
}
/**
 * Destroy Plugin
 */
static gchar *old_playlist = NULL;
void playlist_editor_destroy(void)
{
  if(playlist_editor_browser)
  {
    gtk_widget_destroy(playlist_editor_browser);
    playlist_editor_browser = NULL;
  }
  if(old_playlist) {
        g_free(old_playlist);
        old_playlist = NULL;
  }
}


/**
 * Connection changed
 */
void playlist_editor_conn_changed(MpdObj *mi, int connect, void *userdata)
{
	playlist_editor_fill_list_real();
}


void playlist_editor_browser_add(GtkWidget *cat_tree)
{
	GtkListStore *store= playlist3_get_category_tree_store();
	GtkTreePath *path = NULL;
	GtkTreeIter iter;
	gint pos = cfg_get_single_value_as_int_with_default(config, "playlist-plugin","position",6);

	/* Check if enabled */
	if(!cfg_get_single_value_as_int_with_default(config, "playlist-plugin", "enable", TRUE)) return;

	playlist3_insert_browser(&iter, pos);
	gtk_list_store_set(store, &iter, 
			PL3_CAT_TYPE, playlist_editor_plugin.id,
			PL3_CAT_TITLE, _("Playlist Editor"),
			PL3_CAT_ICON_ID, "media-playlist",
			-1);
	/**
	 * Clean up old row reference if it exists
	 */
	if (playlist_editor_browser_ref)
	{
		gtk_tree_row_reference_free(playlist_editor_browser_ref);
		playlist_editor_browser_ref = NULL;
	}
	/**
	 * create row reference
	 */
	path = gtk_tree_model_get_path(GTK_TREE_MODEL(playlist3_get_category_tree_store()), &iter);
	if (path)
	{
		playlist_editor_browser_ref = gtk_tree_row_reference_new(GTK_TREE_MODEL(playlist3_get_category_tree_store()), path);
		gtk_tree_path_free(path);
	}


}

static MpdData *__playlist_editor_async_function(MpdObj *mi, gpointer function_data)
{
    MpdData* data = mpd_database_get_playlist_content(mi, (gchar *)function_data); 
    g_free(function_data);
    return data;
}
static void __playlist_editor_async_callback(MpdData *data, gpointer callback_data)
{
    gmpc_mpddata_model_set_mpd_data(GMPC_MPDDATA_MODEL(playlist_editor_list_store), data);		
    gtk_widget_set_sensitive(playlist_editor_browser, TRUE);
}

static void playlist_editor_browser_playlist_editor_selected(GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer userdata)
{
    gchar *pl_path = NULL;
    MpdData *data ;
    gtk_tree_model_get(GTK_TREE_MODEL(playlist_editor_store), iter, PL_NAME, &pl_path, -1);
    if(pl_path){
        gtk_widget_set_sensitive(playlist_editor_browser, FALSE);
        gmpc_mpddata_model_set_mpd_data(GMPC_MPDDATA_MODEL(playlist_editor_list_store), NULL);		
        gmpc_mpddata_model_sort_set_playlist(GMPC_MPDDATA_MODEL_SORT(playlist_editor_list_store), pl_path);
        mpd_async_request(__playlist_editor_async_callback,NULL, __playlist_editor_async_function, g_strdup(pl_path));
        if(old_playlist) g_free(old_playlist);
        old_playlist =pl_path;
    }
}
/* always forces an update */
static void playlist_editor_browser_playlist_editor_changed(GtkWidget *giv, gpointer data)
{
    GtkTreeSelection * sel= gtk_tree_view_get_selection(GTK_TREE_VIEW(playlist_editor_icon_view));
    GtkTreeModel * model= gtk_tree_view_get_model(GTK_TREE_VIEW(playlist_editor_icon_view));
    GtkTreeIter iter;
	gmpc_mpddata_model_set_mpd_data(GMPC_MPDDATA_MODEL(playlist_editor_list_store), NULL);		
	gmpc_mpddata_model_sort_set_playlist(GMPC_MPDDATA_MODEL_SORT(playlist_editor_list_store), NULL);
    /* Need to catch wrong changed signals here */
    if(gtk_tree_selection_get_selected(sel, &model, &iter)){
        playlist_editor_browser_playlist_editor_selected(model, NULL, &iter, data);                
    }

}

static void playlist_editor_browser_playlist_editor_changed_real(GtkWidget *giv, gpointer data)
{
    GtkTreeSelection * sel= gtk_tree_view_get_selection(GTK_TREE_VIEW(playlist_editor_icon_view));
    GtkTreeModel * model= gtk_tree_view_get_model(GTK_TREE_VIEW(playlist_editor_icon_view));
    GtkTreeIter iter;
    /* Need to catch wrong changed signals here */
    if(gtk_tree_selection_get_selected(sel, &model, &iter))
    {
        gchar *pl_path = NULL;
        gtk_tree_model_get(GTK_TREE_MODEL(model), &iter, PL_NAME, &pl_path, -1);
        if(pl_path){
            if(old_playlist && g_utf8_collate(pl_path, old_playlist) == 0) {
                g_free(pl_path);
                return;
            }
            g_free(pl_path);
        }
        playlist_editor_browser_playlist_editor_selected(model, NULL, &iter, data);                
        return;
    }
    /* Clear if nothing found */
	gmpc_mpddata_model_set_mpd_data(GMPC_MPDDATA_MODEL(playlist_editor_list_store), NULL);		
	gmpc_mpddata_model_sort_set_playlist(GMPC_MPDDATA_MODEL_SORT(playlist_editor_list_store), NULL);
}

static gint __sort_func(gpointer aa, gpointer bb, gpointer c)
{
    MpdData_real *a = *(MpdData_real **)aa;
    MpdData_real *b = *(MpdData_real **)bb;
    if(a->type == MPD_DATA_TYPE_PLAYLIST && b->type == MPD_DATA_TYPE_PLAYLIST)
    {
        if(a->playlist->path == NULL && b->playlist->path != NULL)
            return -1;
        else if(b->playlist->path == NULL && a->playlist->path != NULL)
            return 1;
        else if (a->playlist->path  && b->playlist->path)
        {
            int val;
            gchar *sa,*sb;
            sa = g_utf8_strdown(a->playlist->path, -1);
            sb = g_utf8_strdown(b->playlist->path, -1);
            val = g_utf8_collate(sa,sb);
            g_free(sa);
            g_free(sb);
            return val;
        }
    }
    return a->type - b->type;
}
void playlist_editor_fill_list(void)
{
    if(!mpd_server_has_idle(connection))
    {
        playlist_editor_fill_list_real();
    }
}
void playlist_editor_fill_list_real(void)
{
    GtkTreeIter iter;
    if(playlist_editor_browser)
	{
        gboolean valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(playlist_editor_store), &iter);
		MpdData *data = mpd_database_playlist_list(connection);
        data = misc_sort_mpddata(data, (GCompareDataFunc) __sort_func, NULL);
		for(;data;data =mpd_data_get_next(data))
		{
			if(data->type ==  MPD_DATA_TYPE_PLAYLIST)
			{
loop:            
                if(valid) {
                    char *name = NULL;
                    int val = 0;
                    gtk_tree_model_get(GTK_TREE_MODEL(playlist_editor_store), &iter, PL_NAME, &name, -1);
                    val = strcmp(name,data->playlist->path);

                    if(val == 0) { 
                        GtkTreePath *path = NULL;
                        gchar *mtime = NULL;
                        gtk_tree_model_get(GTK_TREE_MODEL(playlist_editor_store), &iter, PL_MTIME, &mtime, -1);
                        gtk_list_store_set(playlist_editor_store, &iter, PL_MTIME, data->playlist->mtime, -1);
                        /* do nothing */
                        path = gtk_tree_model_get_path(GTK_TREE_MODEL(playlist_editor_store), &iter);
                        valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(playlist_editor_store), &iter);
                        if(path) {
                            if(gtk_tree_selection_path_is_selected(gtk_tree_view_get_selection(GTK_TREE_VIEW(playlist_editor_icon_view)), path))
                            {
                                if(mtime == NULL || data->playlist->mtime == NULL || strcmp(mtime, data->playlist->mtime) != 0) {
                                    /* update view */
                                    playlist_editor_browser_playlist_editor_changed(playlist_editor_icon_view, NULL);
                                }
                            }
                            gtk_tree_path_free(path);
                        }
                        g_free(mtime);
                    }
                    else if (val > 0) {
                        GtkTreeIter niter;
                        GdkPixbuf *pb = NULL; 
                        if(g_utf8_collate(data->playlist->path, _("Favorites"))){
                            pb = gtk_icon_theme_load_icon(gtk_icon_theme_get_default(), "media-playlist", 32, 0,NULL);
                        }else{
                            pb = gtk_icon_theme_load_icon(gtk_icon_theme_get_default(), "emblem-favorite",32, 0,NULL);
                        }
                        gtk_list_store_insert_before(playlist_editor_store,&niter, &iter);
                        gtk_list_store_set(playlist_editor_store, &niter,PL_NAME, data->playlist->path, PL_MTIME, data->playlist->mtime, PL_IMAGE, pb, -1);
                        if(pb)
                            g_object_unref(pb);
                    }else{

                        valid = gtk_list_store_remove(playlist_editor_store, &iter);
                        g_free(name);
                        goto loop;
                    }
                    g_free(name);

                }
                else
                {
                    GdkPixbuf *pb = NULL; 

                    if(g_utf8_collate(data->playlist->path, _("Favorites"))){
                        pb = gtk_icon_theme_load_icon(gtk_icon_theme_get_default(), "media-playlist", 32, 0,NULL);
                    }else{
                        pb = gtk_icon_theme_load_icon(gtk_icon_theme_get_default(), "emblem-favorite",32, 0,NULL);
                    }
                    gtk_list_store_append(playlist_editor_store, &iter);
                    gtk_list_store_set(playlist_editor_store, &iter,PL_NAME, data->playlist->path, PL_MTIME, data->playlist->mtime, PL_IMAGE, pb, -1);
                    if(pb)
                        g_object_unref(pb);
                    
                }
                
			}
		}
        while(valid) {
            valid = gtk_list_store_remove(playlist_editor_store, &iter);
        }
        if(playlist_editor_browser_ref) {
            GtkTreeIter piter;
            GtkTreePath *path;
            path = gtk_tree_row_reference_get_path(playlist_editor_browser_ref);
            if(path)
            {
                if(gtk_tree_model_get_iter(GTK_TREE_MODEL(gtk_tree_row_reference_get_model(playlist_editor_browser_ref)), &piter,path))
                {
                    gchar *title = g_strdup_printf("<span color='grey'>(%i)</span>", 
                            gtk_tree_model_iter_n_children(GTK_TREE_MODEL(playlist_editor_store),NULL));
                    gtk_list_store_set(GTK_LIST_STORE(gtk_tree_row_reference_get_model(playlist_editor_browser_ref)), &piter,
                            PL3_CAT_NUM_ITEMS, title, -1);
                    g_free(title);
                }
                gtk_tree_path_free(path);
            }
        }

	}
  /*  if(playlist_editor_icon_view)
        gtk_tree_selection_selected_foreach(gtk_tree_view_get_selection(GTK_TREE_VIEW(playlist_editor_icon_view)),
                playlist_editor_browser_playlist_editor_selected, NULL);
                */
}

/*********
 * Playlist list handling 
 */
static void playlist_editor_list_delete_songs(GtkButton *button, GtkTreeView *tree) 
{
	gchar *pl_path = NULL;
    GtkTreeModel *model_view = gtk_tree_view_get_model(GTK_TREE_VIEW(playlist_editor_icon_view));
	GList *it,*list =gtk_tree_selection_get_selected_rows(gtk_tree_view_get_selection(GTK_TREE_VIEW(playlist_editor_icon_view)),&model_view);    
	for(it = g_list_first(list);it; it = g_list_next(it))
	{
		GtkTreePath *path = it->data;
		GtkTreeIter iter;
		if(gtk_tree_model_get_iter(GTK_TREE_MODEL(playlist_editor_store), &iter, path) && !pl_path)
		{
			gtk_tree_model_get(GTK_TREE_MODEL(playlist_editor_store), &iter, PL_NAME, &pl_path, -1);
		}
	}
	g_list_foreach(list, (GFunc)gtk_tree_path_free, NULL);
	g_list_free(list);

	if(pl_path)
	{
		GtkTreeSelection *sel = gtk_tree_view_get_selection(tree);
		GtkTreeModel *model = gtk_tree_view_get_model(tree);
		GList *data;
		list= gtk_tree_selection_get_selected_rows(sel, &model);
		for(data = g_list_last(list); data; data = g_list_previous(data))
		{
			GtkTreePath *path = data->data;
			GtkTreeIter iter;
			if(gtk_tree_model_get_iter(model, &iter, path))
			{
				int *pos = gtk_tree_path_get_indices(path);
				mpd_database_playlist_list_delete(connection, pl_path,pos[0]);
			}
		}
		g_list_foreach (list,(GFunc) gtk_tree_path_free, NULL);
		g_list_free (list);

		g_free(pl_path);
	}
}


static void playlist_editor_list_add_songs(GtkButton *button, GtkTreeView *tree) 
{
    int songs  = 0;
	GtkTreeSelection *sel= gtk_tree_view_get_selection(tree);
	GtkTreeModel *model = gtk_tree_view_get_model(tree);
	GList *data,*list= gtk_tree_selection_get_selected_rows(sel, &model);
	for(data = g_list_first(list); data; data = g_list_next(data))
	{
		GtkTreePath *path = data->data;
		GtkTreeIter iter;
		if(gtk_tree_model_get_iter(model, &iter, path))
		{
			gchar *song_path;
			gtk_tree_model_get(model, &iter,MPDDATA_MODEL_COL_PATH, &song_path, -1); 
			mpd_playlist_queue_add(connection, song_path);
            songs++;
			g_free(song_path);
		}
	}
	mpd_playlist_queue_commit(connection);
	g_list_foreach (list,(GFunc) gtk_tree_path_free, NULL);
	g_list_free (list);
    if(songs > 0)
    {
        gchar *added = g_strdup_printf("%s %i %s.", _("Added"), songs, ngettext("song", "songs", songs));
        pl3_push_statusbar_message(added);
        g_free(added);

    }
}
static void playlist_editor_list_replace_songs(GtkButton *button, GtkTreeView *tree)
{
    mpd_playlist_clear(connection);
    playlist_editor_list_add_songs(button, tree);
    mpd_player_play(connection);
}
static void playlist_editor_clear_playlist(GtkWidget *item, gpointer data)
{
  gchar *path = g_object_get_data(G_OBJECT(item), "path");
  mpd_database_playlist_clear(connection,path);
  playlist_editor_browser_playlist_editor_changed(playlist_editor_icon_view,NULL);

}
static void playlist_editor_load_playlist(GtkWidget *item, gpointer data)
{
  gchar *path = g_object_get_data(G_OBJECT(item), "path");
  mpd_playlist_queue_load(connection,path);
  mpd_playlist_queue_commit(connection);
}
static void playlist_editor_replace_playlist(GtkWidget *item, gpointer data)
{
  gchar *path = g_object_get_data(G_OBJECT(item), "path");
  mpd_playlist_clear(connection);
  mpd_playlist_queue_load(connection,path);
  mpd_playlist_queue_commit(connection);
  mpd_player_play(connection);
}
static void playlist_editor_delete_playlist(GtkWidget *item, gpointer data)
{
  gchar *path = g_object_get_data(G_OBJECT(item), "path");
  mpd_database_delete_playlist(connection, path);
  playlist_editor_fill_list();
}

static void playlist_editor_new_entry_changed(GtkEntry *entry, GtkWidget *button)
{
	if(strlen(gtk_entry_get_text(entry)) > 0)
	{
		/* todo check existing */
		gtk_widget_set_sensitive(button,TRUE);
	}
	else
	{
		gtk_widget_set_sensitive(button,FALSE);
	}
}

static void playlist_editor_new_entry_activate(GtkEntry *entry, GtkWidget *button)
{
	if(strlen(gtk_entry_get_text(entry)) > 0)
		g_signal_emit_by_name(G_OBJECT(button), "clicked");
}

static void playlist_editor_new_playlist(GtkWidget *item, gpointer data)
{
	int done = 0;
    GtkWidget *pl3_win = playlist3_get_window(); 
    GtkWidget *dialog = gtk_dialog_new_with_buttons(_("New playlist"), NULL,GTK_DIALOG_MODAL|GTK_DIALOG_DESTROY_WITH_PARENT,
				GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				NULL);
	GtkWidget *button = NULL; 
	GtkWidget *hbox = gtk_hbox_new(FALSE,6);
	GtkWidget *label = gtk_label_new(_("Name:"));	
	GtkWidget *entry = gtk_entry_new();
    gtk_entry_set_activates_default(GTK_ENTRY(entry), TRUE);
    gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(pl3_win));
	button = gtk_dialog_add_button(GTK_DIALOG(dialog), GTK_STOCK_NEW, GTK_RESPONSE_ACCEPT);
	g_signal_connect(G_OBJECT(entry), "changed", G_CALLBACK(playlist_editor_new_entry_changed), button);
	g_signal_connect(G_OBJECT(entry), "activate", G_CALLBACK(playlist_editor_new_entry_activate), button);
	gtk_widget_set_sensitive(button, FALSE);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), hbox, TRUE, TRUE,0);
	
	gtk_misc_set_alignment(GTK_MISC(label), 1,0.5);
	gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, FALSE,0);
	gtk_box_pack_start(GTK_BOX(hbox), entry, TRUE, FALSE,0);
	gtk_container_set_border_width(GTK_CONTAINER(hbox), 9);
	gtk_container_set_border_width(GTK_CONTAINER(dialog), 3);
	gtk_widget_show_all(dialog);
	while(!done)
	{
		switch(gtk_dialog_run(GTK_DIALOG(dialog)))
		{
			case GTK_RESPONSE_ACCEPT:
                g_object_set_data_full(G_OBJECT(item), "playlist", g_strdup(gtk_entry_get_text(GTK_ENTRY(entry))), g_free);
                mpd_database_playlist_clear(connection, gtk_entry_get_text(GTK_ENTRY(entry)));
				playlist_editor_fill_list();

			default:
				done = TRUE;
				break;
		}
	}
	gtk_widget_destroy(dialog);
}
static void playlist_editor_rename_playlist(GtkWidget *item, gpointer data)
{
	int done = 0;
	char *name = g_object_get_data(G_OBJECT(item), "path");
	GtkWidget *dialog = gtk_dialog_new_with_buttons(_("Rename Playlist"), NULL,GTK_DIALOG_MODAL|GTK_DIALOG_DESTROY_WITH_PARENT,
				GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				NULL);
	GtkWidget *button = NULL; 
	GtkWidget *hbox = gtk_hbox_new(FALSE,6);
	GtkWidget *label = gtk_label_new(_("Name:"));	
	GtkWidget *entry = gtk_entry_new();
	button = gtk_dialog_add_button(GTK_DIALOG(dialog), _("Rename"), GTK_RESPONSE_ACCEPT);
	g_signal_connect(G_OBJECT(entry), "changed", G_CALLBACK(playlist_editor_new_entry_changed), button);
	gtk_entry_set_text(GTK_ENTRY(entry), name);
	gtk_widget_set_sensitive(button, FALSE);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), hbox, TRUE, TRUE,0);
	
	gtk_misc_set_alignment(GTK_MISC(label), 1,0.5);
	gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, FALSE,0);
	gtk_box_pack_start(GTK_BOX(hbox), entry, TRUE, FALSE,0);
	gtk_container_set_border_width(GTK_CONTAINER(hbox), 9);
	gtk_container_set_border_width(GTK_CONTAINER(dialog), 3);

	gtk_widget_show_all(dialog);
	while(!done)
	{
		switch(gtk_dialog_run(GTK_DIALOG(dialog)))
		{
			case GTK_RESPONSE_ACCEPT:
				mpd_database_playlist_rename(connection, name,gtk_entry_get_text(GTK_ENTRY(entry)));
				playlist_editor_fill_list();
			default:
				done = TRUE;
				break;
		}
	}
	gtk_widget_destroy(dialog);
}

static gboolean playlist_editor_key_released(GtkTreeView *tree, GdkEventButton *button, gpointer data)
{
	if(button->button == 3)
	{
		GtkTreeSelection *sel= gtk_tree_view_get_selection(tree);


			GtkWidget *menu = gtk_menu_new();
      GtkWidget *item = NULL;
      if(gtk_tree_selection_count_selected_rows(sel) > 0)
      {
        item = gtk_image_menu_item_new_from_stock(GTK_STOCK_ADD,NULL);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
        g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(playlist_editor_list_add_songs), tree);

        item = gtk_image_menu_item_new_with_label(_("Replace"));
        gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
                gtk_image_new_from_stock(GTK_STOCK_REDO, GTK_ICON_SIZE_MENU));
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
        g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(playlist_editor_list_replace_songs), tree);
      	if(mpd_server_check_version(connection, 0,13,0))
        {
          item = gtk_image_menu_item_new_from_stock(GTK_STOCK_REMOVE,NULL);
          gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
          g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(playlist_editor_list_delete_songs), tree);
        }
      }

     
      gmpc_mpddata_treeview_right_mouse_intergration(GMPC_MPDDATA_TREEVIEW(tree), GTK_MENU(menu));


      gtk_widget_show_all(menu);
      gtk_menu_popup(GTK_MENU(menu), NULL, NULL,NULL, NULL, 0, button->time);
      return TRUE;
  }
  return FALSE;
}
static gboolean playlist_editor_key_pressed(GtkWidget *giv, GdkEventKey *event, gpointer data)
{
    if(event->keyval == GDK_Delete)
    {
        playlist_editor_list_delete_songs(NULL, GTK_TREE_VIEW(giv)); 
    }
    else if(event->state&GDK_CONTROL_MASK && (event->keyval == GDK_Insert || event->keyval == GDK_KP_Insert)) {
        playlist_editor_list_replace_songs(NULL, GTK_TREE_VIEW(giv)); 
    }
    else if (event->keyval == GDK_Insert || event->keyval == GDK_KP_Insert)
    {
        playlist_editor_list_add_songs(NULL, GTK_TREE_VIEW(giv)); 
    }
  
	return FALSE;                                                                                                     
}
static gboolean playlist_editor_browser_button_release_event(GtkWidget *giv, GdkEventButton *event, gpointer data)
{
  if(event->button == 3)
  {
    GtkTreeModel *model_view = gtk_tree_view_get_model(GTK_TREE_VIEW(playlist_editor_icon_view));
    GtkWidget *menu = gtk_menu_new();
    GtkWidget *item = NULL;
    GList *list = NULL;
    /* New */
    if(mpd_server_check_version(connection, 0,13,0))
    {
      item = gtk_image_menu_item_new_from_stock(GTK_STOCK_NEW,NULL);
      gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
      g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(playlist_editor_new_playlist),NULL);                                   
    }

    list = gtk_tree_selection_get_selected_rows(gtk_tree_view_get_selection(GTK_TREE_VIEW(giv)),&model_view);
    if(list)
    {
      char *path = NULL;
      GtkTreeIter iter;
      if(gtk_tree_model_get_iter(GTK_TREE_MODEL(playlist_editor_store), &iter, (GtkTreePath *)list->data))
      {
        gtk_tree_model_get(GTK_TREE_MODEL(playlist_editor_store), &iter,PL_NAME, &path, -1); 

        /* replace */
        item = gtk_image_menu_item_new_with_label(_("Replace"));
        gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item), gtk_image_new_from_stock(GTK_STOCK_REFRESH, GTK_ICON_SIZE_MENU));
        g_object_set_data_full(G_OBJECT(item), "path", g_strdup(path), g_free);
        gtk_menu_shell_prepend(GTK_MENU_SHELL(menu), item);                            
        g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(playlist_editor_replace_playlist),NULL);                                   
        /* load */
        item = gtk_image_menu_item_new_from_stock(GTK_STOCK_ADD,NULL);
        g_object_set_data_full(G_OBJECT(item), "path", g_strdup(path), g_free);
        gtk_menu_shell_prepend(GTK_MENU_SHELL(menu), item);                            
        g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(playlist_editor_load_playlist),NULL);

        /* delete */
        item = gtk_image_menu_item_new_from_stock(GTK_STOCK_DELETE,NULL);
        g_object_set_data_full(G_OBJECT(item), "path", g_strdup(path), g_free);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);     
        g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(playlist_editor_delete_playlist),NULL);

        if(mpd_server_check_version(connection, 0,13,0))
        {
            /* delete */
            item = gtk_image_menu_item_new_with_label(_("Rename"));
            gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item), gtk_image_new_from_stock(GTK_STOCK_EDIT,GTK_ICON_SIZE_MENU));
            g_object_set_data_full(G_OBJECT(item), "path", g_strdup(path), g_free);
            gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);     
            g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(playlist_editor_rename_playlist),NULL);

            /* clear */
            item = gtk_image_menu_item_new_from_stock(GTK_STOCK_CLEAR,NULL);
            g_object_set_data_full(G_OBJECT(item), "path", g_strdup(path), g_free);
            gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);     
            g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(playlist_editor_clear_playlist),NULL);
        }

        g_free(path);
      }
      g_list_foreach (list, (GFunc)gtk_tree_path_free, NULL);
      g_list_free (list);
    }

    gtk_widget_show_all(menu);
    gtk_menu_popup(GTK_MENU(menu), NULL, NULL,NULL, NULL, 0, event->time);
    return TRUE;
  }
  return FALSE;
}
static void playtime_changed(GmpcMpdDataModel *model, gulong playtime)
{
    if(pl3_cat_get_selected_browser() == playlist_editor_plugin.id)
    {
        playlist3_show_playtime(playtime);
    }
}
/**
 * Selected songs are cut, now it has to be deleted
 */
static void playlist_editor_cut_songs(GtkTreeView *tree)
{
    playlist_editor_list_delete_songs(NULL, tree);
}
static void playlist_editor_paste_after_songs(GtkTreeView *tree, GList *paste_list)
{
	gchar *pl_path = NULL;
    GtkTreeModel *model_view = gtk_tree_view_get_model(GTK_TREE_VIEW(playlist_editor_icon_view));
	GList *it,*list = gtk_tree_selection_get_selected_rows(gtk_tree_view_get_selection(GTK_TREE_VIEW(playlist_editor_icon_view)),&model_view);
	for(it = g_list_first(list);it; it = g_list_next(it))
	{
		GtkTreePath *path = it->data;
		GtkTreeIter iter;
		if(gtk_tree_model_get_iter(GTK_TREE_MODEL(playlist_editor_store), &iter, path) && !pl_path)
		{
			gtk_tree_model_get(GTK_TREE_MODEL(playlist_editor_store), &iter, PL_NAME, &pl_path, -1);
		}
	}
	g_list_foreach(list, (GFunc)gtk_tree_path_free, NULL);
	g_list_free(list);

	if(pl_path)
    {
        /* grab the selection from the tree */
        GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW(tree));
        /* check if where connected */
        /* see if there is a row selected */
        if (gtk_tree_selection_count_selected_rows (selection) > 0)
        {
            GList *llist = NULL;
            GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(tree));
            /* start a command list */
            /* grab the selected songs */
            list = gtk_tree_selection_get_selected_rows (selection, &model);
            /* grab the last song that is selected */
            llist = g_list_last(list);
            /* remove every selected song one by one */
            if(llist){
                int id;
                char *path = NULL;
                int *indices = gtk_tree_path_get_indices((GtkTreePath *)llist->data);
                int length = GMPC_MPDDATA_MODEL(model)->num_rows;
                GList *liter = g_list_first(paste_list);
                id = indices[0];
                while(liter)
                {
                    path = liter->data;
                    mpd_database_playlist_list_add(connection, pl_path, path);
                    mpd_database_playlist_move(connection, pl_path,length, id+1); 
                    length++;
                    liter = g_list_next(liter);
                }
            }
            /* free list */
            g_list_foreach (list, (GFunc) gtk_tree_path_free, NULL);
            g_list_free (list);
        }else{
            GList *liter = g_list_last(paste_list);;
            while(liter)
            {
                char *path = liter->data;
                mpd_database_playlist_list_add(connection, pl_path, path);
                liter = g_list_previous(liter);
            }

        }
        gtk_tree_selection_unselect_all(selection);
        g_free(pl_path);
    }

}
static void playlist_editor_paste_before_songs(GtkTreeView *tree, GList *paste_list)
{
	gchar *pl_path = NULL;
    GtkTreeModel *model_view = gtk_tree_view_get_model(GTK_TREE_VIEW(playlist_editor_icon_view));
	GList *it,*list = gtk_tree_selection_get_selected_rows(gtk_tree_view_get_selection(GTK_TREE_VIEW(playlist_editor_icon_view)),&model_view);
	for(it = g_list_first(list);it; it = g_list_next(it))
	{
		GtkTreePath *path = it->data;
		GtkTreeIter iter;
		if(gtk_tree_model_get_iter(GTK_TREE_MODEL(playlist_editor_store), &iter, path) && !pl_path)
		{
			gtk_tree_model_get(GTK_TREE_MODEL(playlist_editor_store), &iter, PL_NAME, &pl_path, -1);
		}
	}
	g_list_foreach(list, (GFunc)gtk_tree_path_free, NULL);
	g_list_free(list);
    if(pl_path)
    {

        /* grab the selection from the tree */
        GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW(tree));

        /* check if where connected */
        /* see if there is a row selected */
        if (gtk_tree_selection_count_selected_rows (selection) > 0)
        {
            GList *llist = NULL;
            GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(tree));
            /* start a command list */
            /* grab the selected songs */
            list = gtk_tree_selection_get_selected_rows (selection, &model);
            /* grab the last song that is selected */
            llist = g_list_first(list);
            /* remove every selected song one by one */
            if(llist){
                int id;
                int *indices = gtk_tree_path_get_indices((GtkTreePath *)llist->data);
                int length = GMPC_MPDDATA_MODEL(model)->num_rows;
                GList *liter = g_list_first(paste_list);
                id = indices[0];

                while(liter)
                {
                    char *path = liter->data;
                    mpd_database_playlist_list_add(connection, pl_path, path);
                    mpd_database_playlist_move(connection, pl_path,length, id); 
                    /* length one longer */
                    length++;
                    liter = g_list_next(liter);
                }
            }
            /* free list */
            g_list_foreach (list, (GFunc) gtk_tree_path_free, NULL);
            g_list_free (list);
        }else{
            GList *liter = g_list_last(paste_list);
            while(liter)
            {
                char *path = liter->data;
                mpd_database_playlist_list_add(connection, pl_path, path);
                liter = g_list_previous(liter);
            }

        }

        gtk_tree_selection_unselect_all(selection);
        g_free(pl_path);
    }
}
static void playlist_editor_row_activated(GtkTreeView *tree, GtkTreePath *path, GtkTreeViewColumn *column, gpointer user_data)
{
    GtkTreeModel *model = gtk_tree_view_get_model(tree);
    GtkTreeIter iter;
    if(gtk_tree_model_get_iter(model,&iter,path))
    {
        mpd_Song *song;
        gtk_tree_model_get(model, &iter, MPDDATA_MODEL_COL_MPDSONG,&song, -1); 
        if(song->file)
        {
            play_path(song->file);
        }
    }
}

static void playlist_editor_browser_init(void)
{
    GtkCellRenderer *renderer = NULL;
    GtkTreeViewColumn *column = NULL;
    GtkWidget *tree = NULL;
    GtkWidget *sw = NULL;
    /* */
    playlist_editor_browser = gtk_hpaned_new();
	gmpc_paned_size_group_add_paned(GMPC_PANED_SIZE_GROUP(paned_size_group), GTK_PANED(playlist_editor_browser));
    /** browser */
    sw = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw), GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw), GTK_SHADOW_ETCHED_IN);

    gtk_paned_add1(GTK_PANED(playlist_editor_browser), sw);

    /* icon view*/
    playlist_editor_store = gtk_list_store_new(PL_NUM_COLS, G_TYPE_STRING, G_TYPE_STRING,GDK_TYPE_PIXBUF);

    playlist_editor_icon_view = tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(playlist_editor_store));//gtk_icon_view_new_with_model(GTK_TREE_MODEL(playlist_editor_store));
    gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(tree), TRUE);

    column = gtk_tree_view_column_new(); 
	gtk_tree_view_column_set_title(column, _("Playlists"));
    renderer = gtk_cell_renderer_pixbuf_new();
    gtk_tree_view_column_pack_start(column, renderer, FALSE);
    gtk_tree_view_column_add_attribute(column, renderer, "pixbuf", PL_IMAGE);
    renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	gtk_tree_view_column_add_attribute(column, renderer, "text", PL_NAME);
	gtk_tree_view_insert_column(GTK_TREE_VIEW(tree),column, -1);



 /*   gtk_icon_view_set_selection_mode(GTK_ICON_VIEW(tree), GTK_SELECTION_BROWSE);
    gtk_icon_view_set_pixbuf_column(GTK_ICON_VIEW(tree), PL_IMAGE);
    gtk_icon_view_set_text_column(GTK_ICON_VIEW(tree), PL_NAME);*/
    gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW(tree)), GTK_SELECTION_BROWSE);
    gtk_container_add(GTK_CONTAINER(sw), tree);

    g_signal_connect(G_OBJECT(gtk_tree_view_get_selection(GTK_TREE_VIEW(tree))), "changed", G_CALLBACK(playlist_editor_browser_playlist_editor_changed_real), NULL);
//    g_signal_connect(G_OBJECT(tree), "item-activated", G_CALLBACK(playlist_editor_browser_activate_cursor_item), NULL);
    g_signal_connect(G_OBJECT(tree), "button-release-event", G_CALLBACK(playlist_editor_browser_button_release_event), NULL);

    /* file list */

    sw = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw), GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw), GTK_SHADOW_ETCHED_IN);

    //	gtk_box_pack_start(GTK_BOX(playlist_editor_browser), sw, TRUE, TRUE,0);
    gtk_paned_add2(GTK_PANED(playlist_editor_browser), sw);


    playlist_editor_list_store= gmpc_mpddata_model_sort_new();
    gmpc_mpddata_model_disable_image(GMPC_MPDDATA_MODEL(playlist_editor_list_store));
    g_signal_connect(G_OBJECT(playlist_editor_list_store), "playtime_changed", G_CALLBACK(playtime_changed), NULL);


    playlist_editor_song_tree = tree = gmpc_mpddata_treeview_new("playlist-browser",FALSE, GTK_TREE_MODEL(playlist_editor_list_store));
    gmpc_mpddata_treeview_enable_click_fix(GMPC_MPDDATA_TREEVIEW(playlist_editor_song_tree));

    /* Copy paste routines */
    g_signal_connect(G_OBJECT(tree), "cut", G_CALLBACK(playlist_editor_cut_songs), NULL);
    g_signal_connect(G_OBJECT(tree), "paste-after", G_CALLBACK(playlist_editor_paste_after_songs), NULL);
    g_signal_connect(G_OBJECT(tree), "paste-before", G_CALLBACK(playlist_editor_paste_before_songs), NULL);


    g_signal_connect(G_OBJECT(tree), "row-activated", G_CALLBACK(playlist_editor_row_activated), NULL);



    gtk_container_add(GTK_CONTAINER(sw), tree);
    gtk_tree_view_set_reorderable(GTK_TREE_VIEW(tree), TRUE);

    g_signal_connect(G_OBJECT(tree), "button-release-event", G_CALLBACK(playlist_editor_key_released), NULL);
    g_signal_connect(G_OBJECT(tree), "key-press-event", G_CALLBACK(playlist_editor_key_pressed), NULL);
    g_object_ref_sink(playlist_editor_browser);

    gtk_widget_show_all(playlist_editor_browser);
}

void playlist_editor_browser_selected(GtkWidget *container)
{
    if(!playlist_editor_browser) {
        playlist_editor_browser_init();
        playlist_editor_fill_list_real();
    }
    gtk_container_add(GTK_CONTAINER(container), playlist_editor_browser);
    gtk_widget_show_all(playlist_editor_browser);

    playlist3_show_playtime(gmpc_mpddata_model_get_playtime(GMPC_MPDDATA_MODEL(playlist_editor_list_store))); 
}

void playlist_editor_browser_unselected(GtkWidget *container)
{
    gtk_container_remove(GTK_CONTAINER(container), playlist_editor_browser);
}

int playlist_editor_browser_cat_menu(GtkWidget *menu, int type, GtkWidget *tree, GdkEventButton *event)
{
    if(type == playlist_editor_plugin.id)
    {
    }
    return 0;
}

static void playlist_editor_add_to_new(GtkWidget *item, gpointer data)
{
    gpointer cb_data =g_object_get_data(G_OBJECT(item), "cb-data");
    void (*callback)(GtkWidget *item, gpointer data) = data;

    playlist_editor_new_playlist(item, NULL);
    callback(item, cb_data); 


}

void playlist_editor_right_mouse(GtkWidget *menu, void (*add_to_playlist)(GtkWidget *menu, gpointer data), gpointer cb_data)
{
    GtkWidget *sitem;
    GtkWidget *item;
    GtkWidget *smenu;

    if(!mpd_server_check_version(connection, 0,13,0))
    {
        return;
    }



    smenu  = gtk_menu_new();

    sitem = gtk_image_menu_item_new_from_stock(GTK_STOCK_NEW, NULL);
    g_object_set_data(G_OBJECT(sitem), "cb-data", cb_data);
    g_signal_connect(G_OBJECT(sitem), "activate", G_CALLBACK(playlist_editor_add_to_new), add_to_playlist);
    gtk_menu_shell_append(GTK_MENU_SHELL(smenu), sitem);

    {
        MpdData *data = mpd_database_playlist_list(connection);
        data = misc_sort_mpddata(data, (GCompareDataFunc) __sort_func, NULL);
        for(;data;data =mpd_data_get_next(data))
        {
            if(data->type ==  MPD_DATA_TYPE_PLAYLIST)
            {
                
                sitem = gtk_image_menu_item_new_with_label(data->playlist->path);
                if(g_utf8_collate(data->playlist->path, _("Favorites"))== 0)
                {
                    gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(sitem), 
                            gtk_image_new_from_icon_name("emblem-favorite", GTK_ICON_SIZE_MENU));
                }else{
                    gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(sitem), 
                            gtk_image_new_from_icon_name("media-playlist", GTK_ICON_SIZE_MENU));
                }
                g_object_set_data_full(G_OBJECT(sitem),"playlist", g_strdup(data->playlist->path), g_free);
                gtk_menu_shell_append(GTK_MENU_SHELL(smenu), sitem);
                g_signal_connect(G_OBJECT(sitem), "activate", G_CALLBACK(add_to_playlist), cb_data);
                gtk_widget_show(sitem);                             				
            }
        }
    }

    /* Add */
    item = gtk_menu_item_new_with_label(_("Add to playlist"));
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), smenu);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
    gtk_widget_show(item);

    gtk_widget_show(smenu);
}

static void playlist_editor_status_changed(MpdObj *mi, ChangedStatusType what, void *data)
{
    if(what&MPD_CST_STORED_PLAYLIST)
    {
        playlist_editor_fill_list_real();
    }
}
static void playlist_editor_activate(GtkWidget *item, gpointer data)
{
    GtkTreeSelection *selec = gtk_tree_view_get_selection((GtkTreeView *)
            playlist3_get_category_tree_view());	

    if(playlist_editor_browser_ref)
    {
        GtkTreePath *path = gtk_tree_row_reference_get_path(playlist_editor_browser_ref); 
        if(path)
        {
            gtk_tree_selection_select_path(selec, path);
            gtk_tree_path_free(path);
        }
    }
}


static void favorites_add_current_song(void)
{
  mpd_Song *song = mpd_playlist_get_current_song(connection);
  if(song->file) 
  {
      mpd_database_playlist_list_add(connection, _("Favorites"), song->file); 
      playlist3_show_error_message(_("Added playing song to favorites list."), ERROR_INFO);
  }
}
static int playlist_editor_add_go_menu(GtkWidget *menu)
{
    GtkWidget *item = NULL;
    if(!cfg_get_single_value_as_int_with_default(config, "playlist-plugin", "enable", 1)) return 0;
    item = gtk_image_menu_item_new_with_label(_("Playlist Editor"));
    gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item), 
            gtk_image_new_from_icon_name("media-playlist", GTK_ICON_SIZE_MENU));

    gtk_widget_add_accelerator(GTK_WIDGET(item), 
            "activate", 
            gtk_menu_get_accel_group(GTK_MENU(menu)), 
            GDK_F5, 0, 
            GTK_ACCEL_VISIBLE);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
    g_signal_connect(G_OBJECT(item), "activate", 
            G_CALLBACK(playlist_editor_activate), NULL);
    /** */
    item = gtk_image_menu_item_new_with_label(_("Add Current Song to favorites"));
    gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item), gtk_image_new_from_icon_name("emblem-favorite", GTK_ICON_SIZE_MENU));
    gtk_widget_add_accelerator(GTK_WIDGET(item), "activate", gtk_menu_get_accel_group(GTK_MENU(menu)), GDK_Return, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(favorites_add_current_song), NULL);

    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    return 1;
}

static void playlist_editor_save_myself(void)
{
    if(playlist_editor_browser_ref)
    {
        GtkTreePath *path = gtk_tree_row_reference_get_path(playlist_editor_browser_ref);
        if(path)
        {
            gint *indices = gtk_tree_path_get_indices(path);
            cfg_set_single_value_as_int(config, "playlist-plugin","position",indices[0]);
            gtk_tree_path_free(path);
        }
    }
}

