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
#include <gdk/gdkkeysyms.h>
#include <stdlib.h>
#include <string.h>
#include <config.h>

#include "main.h"
#include "misc.h"
#include "playlist3.h"
#include "playlist3-current-playlist-browser.h"
#include "playlist3-find2-browser.h"
#include "gmpc-mpddata-model.h"
#include "gmpc-mpddata-model-playlist.h"
#include "gmpc-mpddata-treeview.h"
#include "eggcolumnchooserdialog.h"
#include "advanced-search.h"
#ifdef USE_SYSTEM_LIBSEXY
#include <libsexy/sexy-icon-entry.h>
#endif
#include "playlist3-messages.h"

#include "playlist3-playlist-editor.h"



enum {
    PLAY_QUEUE_DUMMY_PROPERTY,
    PLAY_QUEUE_UID
};
/**
 * Private data structure
 */
typedef struct _PlayQueuePluginPrivate {
    gulong status_changed_handler;
    gulong connection_changed_handler;

    /* Quick Search */
    gboolean search_keep_open;
    GtkWidget *filter_entry;
    GtkTreeModel *mod_fill;
    gboolean quick_search;
    guint quick_search_timeout;
    /* Other widgets */
    GtkWidget *pl3_cp_tree;
    GtkWidget *pl3_cp_vbox;
    /* reference to the row */
    GtkTreeRowReference *pl3_curb_tree_ref;
    /* Gchar *uid */
    gchar *uid;
} _PlayQueuePluginPrivate;

/**
 * Propperty setter/getter
 */
static void play_queue_get_property (GObject * object, guint property_id, GValue * value, GParamSpec * pspec) {
	PlayQueuePlugin * self;
	self = (PlayQueuePlugin *)(object);
	switch (property_id) {
		case PLAY_QUEUE_UID:
		g_value_set_string (value, self->priv->uid);
		break;
		default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
		break;
	}

}
static void play_queue_set_property (GObject * object, guint property_id, const GValue * value, GParamSpec * pspec)
{
	PlayQueuePlugin * self;
	self = (PlayQueuePlugin *)(object);
	switch (property_id) {
		case PLAY_QUEUE_UID:
            if(self->priv->uid) g_free(self->priv->uid);
            self->priv->uid = g_value_dup_string(value);
		break;
		default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
		break;
	}
}


static void pl3_current_playlist_browser_paste_after_songs(GtkTreeView *tree, GList *paste_list, PlayQueuePlugin *self);
static void pl3_current_playlist_browser_paste_before_songs(GtkTreeView *tree, GList *paste_list, PlayQueuePlugin *self);
static void pl3_current_playlist_browser_delete_selected_songs (PlayQueuePlugin *self);


static void pl3_current_playlist_browser_add(GmpcPluginBrowserIface *obj, GtkWidget *cat_tree);

static void pl3_current_playlist_browser_selected(GmpcPluginBrowserIface *obj, GtkContainer *container);
static void pl3_current_playlist_browser_unselected(GmpcPluginBrowserIface *obj, GtkContainer *container);

static void pl3_current_playlist_browser_activate(PlayQueuePlugin *self);


/* just for here */
static void pl3_current_playlist_browser_row_activated(GtkTreeView *tree, GtkTreePath *path, GtkTreeViewColumn *col,PlayQueuePlugin *self);
static int  pl3_current_playlist_browser_button_release_event(GtkTreeView *tree, GdkEventButton *event, PlayQueuePlugin *self);
static int  pl3_current_playlist_browser_key_release_event(GtkTreeView *tree, GdkEventKey *event,PlayQueuePlugin *self);
static void pl3_current_playlist_browser_show_info(PlayQueuePlugin *self);
static void pl3_current_playlist_save_playlist(void);
static void pl3_current_playlist_browser_shuffle_playlist(void);
static void pl3_current_playlist_browser_clear_playlist(void);

static void pl3_current_playlist_browser_init(PlayQueuePlugin *self);


static void pl3_cp_current_song_changed(GmpcMpdDataModelPlaylist *model2,GtkTreePath *path, GtkTreeIter *iter,PlayQueuePlugin *self)
{
    GtkTreeModel *model;
    if(self->priv->pl3_cp_tree == NULL) return;
    model = gtk_tree_view_get_model(GTK_TREE_VIEW(self->priv->pl3_cp_tree));
    if(GMPC_IS_MPDDATA_MODEL_PLAYLIST(model))
    {
        if(cfg_get_single_value_as_int_with_default(config, "playlist", "st_cur_song", 0))
        {
            gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(self->priv->pl3_cp_tree),
                    path,
                    NULL,
                    TRUE,0.5,0);
        }
    }
}
static void __real_pl3_total_playtime_changed(GmpcMpdDataModelPlaylist *model, unsigned long loaded_songs, unsigned long total_playtime, gpointer user_data)
{
     if(mpd_playlist_get_playlist_length(connection)&&loaded_songs > 0)

     {
         unsigned long total_songs = GMPC_MPDDATA_MODEL(model)->num_rows;
         guint playtime = total_playtime*((gdouble)(total_songs/(gdouble)loaded_songs));
         gchar *string = format_time(playtime);
         GString *tstring = g_string_new("");

         g_string_append_printf(tstring, "%lu %s",
                 total_songs,
                 ngettext("item", "items", total_songs )
                 );
         if(string)
         {
             g_string_append_printf(tstring, ", %s ", string);
             if(loaded_songs != total_songs) {
                tstring = g_string_append(tstring, _("(Estimation)"));
            }
         }
         pl3_push_rsb_message(tstring->str);

         q_free(string);

         g_string_free(tstring, TRUE);

     } else {

         pl3_push_rsb_message("");

     }
}

static void pl3_total_playtime_changed(GmpcMpdDataModelPlaylist *model, unsigned long loaded_songs, unsigned long total_playtime, PlayQueuePlugin *self)
{
    if(pl3_cat_get_selected_browser() == GMPC_PLUGIN_BASE(self)->id)
    {
        __real_pl3_total_playtime_changed(model, loaded_songs, total_playtime, self);
    }
}
static void pl3_current_playlist_browser_crop_current_song(PlayQueuePlugin *self, const gchar *param)
{
    mpd_Song *song = mpd_playlist_get_current_song(connection);
    if(song)
    {
        int length = mpd_playlist_get_playlist_length(connection);
        /* Move to first place, this avoid possible "issues" */
        mpd_playlist_move_id(connection,song->id, 0);
        for(;length > 1; length--){
            mpd_playlist_queue_delete_pos(connection, length-1);
            if((length&16383) == 16383) {
                mpd_playlist_queue_commit(connection);
            }
        }
        mpd_playlist_queue_commit(connection);
    }
}
static void pl3_cp_ec_playlist(PlayQueuePlugin *self, const gchar *param)
{
    pl3_find2_select_plugin_id(GMPC_PLUGIN_BASE(self)->id);
    pl3_find2_do_search_any(param);
}

static void pl3_cp_init(PlayQueuePlugin *self)
{
    g_signal_connect(G_OBJECT(playlist), "current_song_changed", G_CALLBACK(pl3_cp_current_song_changed), self);
    g_signal_connect(G_OBJECT(playlist), "total_playtime_changed", G_CALLBACK(pl3_total_playtime_changed), self);


    gmpc_easy_command_add_entry(gmpc_easy_command,
                    _("switch play queue"),"",
                    _("Switch to play queue"),
                    (GmpcEasyCommandCallback *)pl3_current_playlist_browser_activate, self);
    gmpc_easy_command_add_entry(gmpc_easy_command,
                    _("Clear play queue"),"",
                    _("Clear play queue"),
                    (GmpcEasyCommandCallback *)pl3_current_playlist_browser_clear_playlist, self);

    gmpc_easy_command_add_entry(gmpc_easy_command,
                    _("Crop current song"),"",
                    _("Crop the playlist so it only contains the current song"),
                    (GmpcEasyCommandCallback *)pl3_current_playlist_browser_crop_current_song, self);

    gmpc_easy_command_add_entry(gmpc_easy_command,
            _("search playlist"), ".*",
            _("Search playlist <query>"),
                (GmpcEasyCommandCallback *)pl3_cp_ec_playlist, self);
}
void pl3_current_playlist_destroy(PlayQueuePlugin *self);

/* internal */


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

void pl3_current_playlist_destroy(PlayQueuePlugin *self)
{
    /* destroy the entry */
    if(self->priv->pl3_curb_tree_ref)
    {
        GtkTreeIter piter;
        GtkTreePath *path;
        path = gtk_tree_row_reference_get_path(self->priv->pl3_curb_tree_ref);
        if(path)
        {
            if(gtk_tree_model_get_iter(GTK_TREE_MODEL(gtk_tree_row_reference_get_model(self->priv->pl3_curb_tree_ref)), &piter,path))
            {
                gtk_list_store_remove(GTK_LIST_STORE(gtk_tree_row_reference_get_model(self->priv->pl3_curb_tree_ref)), &piter);
            }
            gtk_tree_path_free(path);
        }
        gtk_tree_row_reference_free(self->priv->pl3_curb_tree_ref);
        self->priv->pl3_curb_tree_ref = NULL;
    }
    /* destroy the browser */
    if(self->priv->pl3_cp_vbox)
    {
        /* remove the signal handler so when the widget is destroyed, the numbering of the labels are not changed again */
        g_signal_handlers_disconnect_by_func(G_OBJECT(self->priv->pl3_cp_tree), G_CALLBACK(pl3_current_playlist_column_changed), NULL);
        g_object_unref(self->priv->pl3_cp_vbox);
        self->priv->pl3_cp_vbox = NULL;
    }
    self->priv->pl3_cp_tree =  NULL;
}




static gboolean mod_fill_do_entry_changed(PlayQueuePlugin *self)
{
    const gchar *text2 = gtk_entry_get_text(GTK_ENTRY(self->priv->filter_entry));
    
    if(strlen(text2) > 0)
    {
    
        MpdData *data = NULL;
        data = advanced_search(text2, TRUE);
        gmpc_mpddata_model_set_mpd_data(GMPC_MPDDATA_MODEL(self->priv->mod_fill), data);
        self->priv->quick_search = TRUE;
        gtk_tree_view_set_model(GTK_TREE_VIEW(self->priv->pl3_cp_tree), self->priv->mod_fill);
        gtk_widget_show(self->priv->filter_entry);
    }
    else
    {
        gtk_tree_view_set_model(GTK_TREE_VIEW(self->priv->pl3_cp_tree), playlist);

        gmpc_mpddata_model_set_mpd_data(GMPC_MPDDATA_MODEL(self->priv->mod_fill), NULL);
        if(!self->priv->search_keep_open)
        {
            self->priv->quick_search = 0;
            gtk_widget_hide(self->priv->filter_entry);
            gtk_widget_grab_focus(self->priv->pl3_cp_tree);
        }
    }
    self->priv->quick_search_timeout = 0;
    return FALSE;
}

static gboolean mod_fill_entry_key_press_event(GtkWidget *entry, GdkEventKey *event, PlayQueuePlugin *self)
{
    const gchar *text = gtk_entry_get_text(GTK_ENTRY(entry));

    if(strlen(text) == 0)
    {
        if(event->keyval == GDK_BackSpace || event->keyval == GDK_Escape)
        {
            self->priv->search_keep_open = FALSE;
            gtk_tree_view_set_model(GTK_TREE_VIEW(self->priv->pl3_cp_tree), playlist);

            gmpc_mpddata_model_set_mpd_data(GMPC_MPDDATA_MODEL(self->priv->mod_fill), NULL);
            self->priv->quick_search = 0;
            gtk_widget_hide(entry);
            gtk_widget_grab_focus(self->priv->pl3_cp_tree);
            return TRUE;
        }
    }else if(event->keyval == GDK_Escape)
    {
        self->priv->search_keep_open = FALSE;
        gtk_entry_set_text(GTK_ENTRY(entry), "");
    }
    else if (event->keyval == GDK_Up || event->keyval == GDK_Down)
    {
        gtk_widget_grab_focus(self->priv->pl3_cp_tree);
        return TRUE;
    }
    return FALSE;
}

static void  mod_fill_entry_changed(GtkWidget *entry, PlayQueuePlugin *self)
{
    if(self->priv->quick_search_timeout != 0){
        g_source_remove(self->priv->quick_search_timeout);
        self->priv->quick_search_timeout = 0;
    }

    if(cfg_get_single_value_as_int_with_default(config, "general", "search-as-you-type", 0) == 1)
    {
        self->priv->quick_search_timeout = g_timeout_add(250, (GSourceFunc)mod_fill_do_entry_changed, self);
    }
    gtk_widget_show(entry);
}
static void mod_fill_entry_activate(GtkWidget *entry, PlayQueuePlugin *self)
{
    if(self->priv->quick_search_timeout != 0)
        g_source_remove(self->priv->quick_search_timeout);
	mod_fill_do_entry_changed(self);
    gtk_widget_grab_focus(self->priv->pl3_cp_tree);
}
#ifndef USE_SYSTEM_LIBSEXY
#if GTK_CHECK_VERSION(2,16,0)
static void mod_fill_clear_search_entry(GtkEntry *entry, GtkEntryIconPosition icon_pos, GdkEvent *event, gpointer user_data)
{
    if(icon_pos == GTK_ENTRY_ICON_SECONDARY){
        gtk_entry_set_text(GTK_ENTRY(entry), "");
    }
}
#endif
#endif
static void pl3_current_playlist_browser_init(PlayQueuePlugin *self)
{
	GtkWidget *entry = NULL,*tree = NULL,*sw = NULL, *pl3_cp_sw;
    /* Mark the plugin internal */
    GMPC_PLUGIN_BASE(self)->plugin_type = GMPC_INTERNALL|GMPC_PLUGIN_PL_BROWSER;

	self->priv->pl3_cp_vbox = gtk_vbox_new(FALSE,6);
    tree = gmpc_mpddata_treeview_new(self->priv->uid, FALSE, GTK_TREE_MODEL(playlist));

    g_signal_connect(G_OBJECT(tree), "paste_before", G_CALLBACK(pl3_current_playlist_browser_paste_before_songs), self);
    g_signal_connect(G_OBJECT(tree), "paste_after", G_CALLBACK(pl3_current_playlist_browser_paste_after_songs), self);
    g_signal_connect_swapped(G_OBJECT(tree), "cut", G_CALLBACK(pl3_current_playlist_browser_delete_selected_songs), self);

    /* filter */
    self->priv->mod_fill = (GtkTreeModel *)gmpc_mpddata_model_new();
#ifdef USE_SYSTEM_LIBSEXY
    entry = sexy_icon_entry_new(); 
    sexy_icon_entry_add_clear_button(SEXY_ICON_ENTRY(entry));
#else
    entry = gtk_entry_new();
#if GTK_CHECK_VERSION(2,16,0)
    gtk_entry_set_icon_from_stock(GTK_ENTRY(entry), GTK_ENTRY_ICON_SECONDARY, GTK_STOCK_CLEAR);
    g_signal_connect(GTK_ENTRY(entry), "icon-press", G_CALLBACK(mod_fill_clear_search_entry), NULL);

    gtk_entry_set_icon_from_stock(GTK_ENTRY(entry), GTK_ENTRY_ICON_PRIMARY, GTK_STOCK_FIND);
    gtk_entry_set_icon_activatable(GTK_ENTRY(entry), GTK_ENTRY_ICON_PRIMARY, FALSE);
#endif
#endif    
    gtk_box_pack_end(GTK_BOX(self->priv->pl3_cp_vbox), entry, FALSE, TRUE,0);
    self->priv->filter_entry= entry;
    g_signal_connect(G_OBJECT(entry), "changed", G_CALLBACK(mod_fill_entry_changed), self);
    g_signal_connect(G_OBJECT(entry), "key-press-event", G_CALLBACK(mod_fill_entry_key_press_event), self);
    g_signal_connect(G_OBJECT(entry), "activate", G_CALLBACK(mod_fill_entry_activate), self);

    gtk_tree_view_set_reorderable(GTK_TREE_VIEW(tree), TRUE);
	sw = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw), GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw), GTK_SHADOW_ETCHED_IN);
    gtk_container_add(GTK_CONTAINER(sw), tree);
	gtk_box_pack_start(GTK_BOX(self->priv->pl3_cp_vbox), GTK_WIDGET(sw), TRUE, TRUE,0);
    gtk_widget_show_all(sw);
	/* set up the tree */
	gtk_tree_view_set_enable_search(GTK_TREE_VIEW(tree), FALSE);

    gmpc_mpddata_treeview_enable_click_fix(GMPC_MPDDATA_TREEVIEW(tree));
    /* setup signals */
	g_signal_connect(G_OBJECT(tree), "row-activated",G_CALLBACK(pl3_current_playlist_browser_row_activated),self); 
	g_signal_connect(G_OBJECT(tree), "button-release-event", G_CALLBACK(pl3_current_playlist_browser_button_release_event), self);
	g_signal_connect(G_OBJECT(tree), "key-press-event", G_CALLBACK(pl3_current_playlist_browser_key_release_event), self);

	/* set up the scrolled window */
    pl3_cp_sw = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(pl3_cp_sw), GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(pl3_cp_sw), GTK_SHADOW_ETCHED_IN);


	/* set initial state */
    self->priv->pl3_cp_tree = tree;
	g_object_ref_sink(G_OBJECT(self->priv->pl3_cp_vbox));
}

static void pl3_current_playlist_browser_select_current_song(PlayQueuePlugin *self)
{
	if(self->priv->pl3_cp_tree == NULL|| !GTK_WIDGET_REALIZED(self->priv->pl3_cp_tree)) return;
	/* scroll to the playing song */
	if(mpd_player_get_current_song_pos(connection) >= 0 && mpd_playlist_get_playlist_length(connection)  > 0)
	{
		GtkTreePath *path = gtk_tree_path_new_from_indices(mpd_player_get_current_song_pos(connection),-1);
		if(path != NULL && GMPC_IS_MPDDATA_MODEL_PLAYLIST(gtk_tree_view_get_model(GTK_TREE_VIEW(self->priv->pl3_cp_tree))))
		{
        	gtk_tree_view_set_cursor(GTK_TREE_VIEW(self->priv->pl3_cp_tree), path, NULL, FALSE);
		}
		gtk_tree_path_free(path);
	}      
}

static void pl3_current_playlist_browser_scroll_to_current_song(PlayQueuePlugin *self)
{
	if(self->priv->pl3_cp_tree == NULL || !GTK_WIDGET_REALIZED(self->priv->pl3_cp_tree)) return;
	/* scroll to the playing song */
	if(mpd_player_get_current_song_pos(connection) >= 0 && mpd_playlist_get_playlist_length(connection)  > 0)
	{
		GtkTreePath *path = gtk_tree_path_new_from_indices(mpd_player_get_current_song_pos(connection),-1);
		if(path != NULL)
		{
			gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(self->priv->pl3_cp_tree),
					path,
					NULL,
					TRUE,0.5,0);
			gtk_tree_path_free(path);
		}
	
	}      
}

/* add's the toplevel entry for the current playlist view */
static void pl3_current_playlist_browser_add(GmpcPluginBrowserIface *obj, GtkWidget *cat_tree)
{
    PlayQueuePlugin *self = (PlayQueuePlugin*)obj;
	GtkTreeIter iter;
	GtkTreePath *path;
	gint pos = cfg_get_single_value_as_int_with_default(config, "current-playlist","position",0);
	playlist3_insert_browser(&iter, pos);
	gtk_list_store_set(GTK_LIST_STORE(pl3_tree), &iter, 
			PL3_CAT_TYPE, GMPC_PLUGIN_BASE(self)->id /*current_playlist_plug.id*/,/*PL3_CURRENT_PLAYLIST,*/
			PL3_CAT_TITLE, _(gmpc_plugin_base_get_name(GMPC_PLUGIN_BASE(self))),
			PL3_CAT_ICON_ID, "playlist-browser",
			-1);
	if(self->priv->pl3_curb_tree_ref)
	{
		gtk_tree_row_reference_free(self->priv->pl3_curb_tree_ref);
		self->priv->pl3_curb_tree_ref = NULL;
	}
	path = gtk_tree_model_get_path(GTK_TREE_MODEL(pl3_tree), &iter);
	if(path)
	{
		self->priv->pl3_curb_tree_ref = gtk_tree_row_reference_new(GTK_TREE_MODEL(pl3_tree),path);
		gtk_tree_path_free(path);
	}
}


/* delete all selected songs,
 * if no songs select ask the user if he want's to clear the list 
 */
static void pl3_current_playlist_browser_delete_selected_songs (PlayQueuePlugin *self)
{
	/* grab the selection from the tree */
	GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW(self->priv->pl3_cp_tree));
	/* check if where connected */
	/* see if there is a row selected */
	if (gtk_tree_selection_count_selected_rows (selection) > 0)
	{
		GList *list = NULL, *llist = NULL;
		GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(self->priv->pl3_cp_tree));
		/* start a command list */
		/* grab the selected songs */
		list = gtk_tree_selection_get_selected_rows (selection, &model);
		/* grab the last song that is selected */
		llist = g_list_last (list);
		/* remove every selected song one by one */
		do{
			GtkTreeIter iter;
			int value;
			gtk_tree_model_get_iter (model, &iter,(GtkTreePath *) llist->data);
			gtk_tree_model_get (model, &iter, MPDDATA_MODEL_COL_SONG_ID, &value, -1);
			mpd_playlist_queue_delete_id(connection, value);			
		} while ((llist = g_list_previous (llist)));

		/* close the list, so it will be executed */
		mpd_playlist_queue_commit(connection);
		/* unselect all if multiple songs were selected */
		if(g_list_length(list) > 1)
			gtk_tree_selection_unselect_all(selection);
		/* free list */
		g_list_foreach (list, (GFunc) gtk_tree_path_free, NULL);
		g_list_free (list);
	}
	else
	{
		/* create a warning message dialog */
		GtkWidget *dialog =
			gtk_message_dialog_new (GTK_WINDOW
					(playlist3_get_window()),
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
            default:
                break;
		}
		gtk_widget_destroy (GTK_WIDGET (dialog));
	}
	/* update everything if where still connected */

	mpd_status_queue_update(connection);
}

static void pl3_current_playlist_browser_crop_selected_songs(PlayQueuePlugin *self)
{
	GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(self->priv->pl3_cp_tree));
	/* grab the selection from the tree */
	GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW(self->priv->pl3_cp_tree));

	/* see if there is a row selected */	
	if (gtk_tree_selection_count_selected_rows (selection) > 0)
	{
		GtkTreeIter iter;
		/* we want to delete from back to front, so we have to transverse this list */
		if(gtk_tree_model_get_iter_first(GTK_TREE_MODEL(model), &iter))
		{

			do{
				if(!gtk_tree_selection_iter_is_selected(selection, &iter))
				{
					int id=0;
					/* song pos starts at 1, not a 0, compensate for that */
					gtk_tree_model_get (GTK_TREE_MODEL(model), &iter, MPDDATA_MODEL_COL_SONG_ID, &id, -1);
					mpd_playlist_queue_delete_id(connection, id);
				}
			}while(gtk_tree_model_iter_next(GTK_TREE_MODEL(model), &iter));
		}
		mpd_playlist_queue_commit(connection);
		/* update everything if where still connected */
		gtk_tree_selection_unselect_all(selection);

		mpd_status_queue_update(connection);
	}
}

static void pl3_current_playlist_editor_add_to_playlist(GtkWidget *menu, gpointer cb_data)
{
    PlayQueuePlugin *self = (PlayQueuePlugin *)cb_data;
    GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(self->priv->pl3_cp_tree));
    GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(self->priv->pl3_cp_tree));
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

	playlist_editor_fill_list();
}

static void pl3_current_playlist_browser_paste_after_songs(GtkTreeView *tree, GList *paste_list, PlayQueuePlugin *self)
{
    /* grab the selection from the tree */
    GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW(self->priv->pl3_cp_tree));

    int seen= 0;
    /* check if where connected */
    /* see if there is a row selected */
    if (gtk_tree_selection_count_selected_rows (selection) > 0)
    {
        GList *list = NULL, *llist = NULL;
        GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(self->priv->pl3_cp_tree));
        /* start a command list */
        /* grab the selected songs */
        list = gtk_tree_selection_get_selected_rows (selection, &model);
        /* grab the last song that is selected */
        llist = g_list_last(list);
        /* remove every selected song one by one */
        if(llist){
            GtkTreeIter iter;
            gtk_tree_model_get_iter (model, &iter,(GtkTreePath *) llist->data);
            /* Trick that avoids roundtrip to mpd */
            {
                int id;
                char *path = NULL;
                int length = mpd_playlist_get_playlist_length(connection);
                GList *liter = g_list_first(paste_list);
                gtk_tree_model_get (model, &iter, MPDDATA_MODEL_COL_SONG_POS, &id, -1);			
                while(liter)
                {
                    int song_id;
                    path = liter->data;
                    song_id = mpd_playlist_add_get_id(connection, path);
                    if(song_id == -1 && !seen)
                    {
                        playlist3_show_error_message(_("Your mpd has a broken 'addid', pasting will fail."), ERROR_WARNING);      
                        seen = 1;
                    }
                    mpd_playlist_move_pos(connection, length, id);
                    length++;
                    liter = g_list_next(liter);
                }
            }
        }
        /* free list */
        g_list_foreach (list, (GFunc) gtk_tree_path_free, NULL);
        g_list_free (list);
    }else{
        GList *liter = g_list_first(paste_list);;
        while(liter)
        {
            char *path = liter->data;
            int song_id = mpd_playlist_add_get_id(connection, path);
            if(song_id == -1 && !seen)
            {
                playlist3_show_error_message(_("Your mpd has a broken 'addid', pasting will fail."), ERROR_WARNING);      
                seen = 1;
            }
            liter = g_list_next(liter);
        }

    }
    gtk_tree_selection_unselect_all(selection);
}
static void pl3_current_playlist_browser_paste_before_songs(GtkTreeView *tree, GList *paste_list, PlayQueuePlugin *self)
{
    /* grab the selection from the tree */
    GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW(self->priv->pl3_cp_tree));

    int seen= 0;
    /* check if where connected */
    /* see if there is a row selected */
    if (gtk_tree_selection_count_selected_rows (selection) > 0)
    {
        GList *list = NULL, *llist = NULL;
        GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(self->priv->pl3_cp_tree));
        /* start a command list */
        /* grab the selected songs */
        list = gtk_tree_selection_get_selected_rows (selection, &model);
        /* grab the last song that is selected */
        llist = g_list_first(list);
        /* remove every selected song one by one */
        if(llist){
            GtkTreeIter iter;
            gtk_tree_model_get_iter (model, &iter,(GtkTreePath *) llist->data);
            /* Trick that avoids roundtrip to mpd */
            {
                int id;
                int length = mpd_playlist_get_playlist_length(connection);
                GList *liter = g_list_first(paste_list);
                gtk_tree_model_get (model, &iter, MPDDATA_MODEL_COL_SONG_POS, &id, -1);			
                while(liter)
                {
                    char *path = liter->data;
                    int song_id = mpd_playlist_add_get_id(connection, path);
                    if(song_id == -1 && !seen)
                    {
                        playlist3_show_error_message(_("Your mpd has a broken 'addid', pasting will fail."), ERROR_WARNING);      
                        seen = 1;
                    }
                    mpd_playlist_move_pos(connection, length, id-1);
                    /* The song is now one lower */
                    /* length one longer */
                    length++;
                    liter = g_list_next(liter);
                }
            }
        }
        /* free list */
        g_list_foreach (list, (GFunc) gtk_tree_path_free, NULL);
        g_list_free (list);
    }else{
            GList *liter = g_list_first(paste_list);
            while(liter)
            {
                char *path = liter->data;
                int song_id = mpd_playlist_add_get_id(connection, path);
                if(song_id == -1 && !seen)
                {
                    playlist3_show_error_message(_("Your mpd has a broken 'addid', pasting will fail."), ERROR_WARNING);      
                    seen = 1;
                }
                liter = g_list_next(liter);
            }

        }

    gtk_tree_selection_unselect_all(selection);
}


static int pl3_current_playlist_browser_button_release_event(GtkTreeView *tree, GdkEventButton *event, PlayQueuePlugin *self)
{
	if(event->button == 3)
	{
		/* del, crop */
		GtkWidget *item;
		GtkWidget *menu = gtk_menu_new();	

		int rows = gtk_tree_selection_count_selected_rows(gtk_tree_view_get_selection(tree));
		/* add the delete widget */
        item = gtk_image_menu_item_new_from_stock(GTK_STOCK_REMOVE,NULL);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
        g_signal_connect_swapped(G_OBJECT(item), "activate", G_CALLBACK(pl3_current_playlist_browser_delete_selected_songs),self);

        if(rows)
        {
            /* add the delete widget */
            item = gtk_image_menu_item_new_with_label(_("Crop"));
            gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
                    gtk_image_new_from_stock(GTK_STOCK_CUT, GTK_ICON_SIZE_MENU));
            g_signal_connect_swapped(G_OBJECT(item), "activate", G_CALLBACK(pl3_current_playlist_browser_crop_selected_songs), self);		
            gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
        }
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

        if(rows == 1) {
            mpd_Song *song;
			GtkTreePath *path;
			GtkTreeModel *model = gtk_tree_view_get_model(tree);
            GtkTreeIter iter;
			GList *list = gtk_tree_selection_get_selected_rows(gtk_tree_view_get_selection(GTK_TREE_VIEW(tree)), &model);
			path = list->data;
            if(path && gtk_tree_model_get_iter(model, &iter, path)) {
				gtk_tree_model_get(model, &iter, MPDDATA_MODEL_COL_MPDSONG, &song, -1);
				if(song) 
                {
                    item = gtk_image_menu_item_new_from_stock(GTK_STOCK_DIALOG_INFO,NULL);
                    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
                    g_signal_connect_swapped(G_OBJECT(item), "activate", G_CALLBACK(pl3_current_playlist_browser_show_info), self);		

                    /* Add song sebmenu */
                    submenu_for_song(menu, song);
                }
            }
            g_list_foreach(list,(GFunc)gtk_tree_path_free, NULL);
            g_list_free(list);
        }

        
		playlist_editor_right_mouse(menu,pl3_current_playlist_editor_add_to_playlist,self);
        gmpc_mpddata_treeview_right_mouse_intergration(GMPC_MPDDATA_TREEVIEW(tree), GTK_MENU(menu));
		gtk_widget_show_all(menu);
		gtk_menu_popup(GTK_MENU(menu), NULL, NULL,NULL, NULL,0, event->time);	
		return TRUE;
	}
    return FALSE;
}

static void pl3_current_playlist_browser_row_activated(GtkTreeView *tree, GtkTreePath *path, GtkTreeViewColumn *col,PlayQueuePlugin *self)
{
    GtkTreeIter iter;
    gint song_id;
    GtkTreeModel *model = gtk_tree_view_get_model(tree);
    gtk_tree_model_get_iter(model, &iter, path);
    gtk_tree_model_get(model, &iter, MPDDATA_MODEL_COL_SONG_ID,&song_id, -1);
    mpd_player_play_id(connection, song_id);

    if(!self->priv->search_keep_open && model == self->priv->mod_fill)
    {
        gtk_tree_view_set_model(GTK_TREE_VIEW(self->priv->pl3_cp_tree), playlist);

        gmpc_mpddata_model_set_mpd_data(GMPC_MPDDATA_MODEL(self->priv->mod_fill), NULL);
        self->priv->quick_search = 0;
        gtk_widget_hide(self->priv->filter_entry);

        pl3_current_playlist_browser_select_current_song(self);
    }

}

static void pl3_current_playlist_browser_show_info(PlayQueuePlugin *self)
{

    GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW(self->priv->pl3_cp_tree));
    GtkTreeSelection *selection =gtk_tree_view_get_selection (GTK_TREE_VIEW(self->priv->pl3_cp_tree));
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

static void pl3_current_playlist_browser_selected(GmpcPluginBrowserIface *obj, GtkContainer *container)
{
    PlayQueuePlugin *self = (PlayQueuePlugin *)obj;
    unsigned long a = 0,b = 0;
    if(self->priv->pl3_cp_vbox == NULL)
    {
        pl3_current_playlist_browser_init((PlayQueuePlugin *)obj);
    }
    gtk_container_add(GTK_CONTAINER(container), self->priv->pl3_cp_vbox);
    gtk_widget_show(self->priv->pl3_cp_vbox);

    gmpc_mpddata_model_playlist_get_total_playtime(GMPC_MPDDATA_MODEL_PLAYLIST(playlist), &a, &b);
    __real_pl3_total_playtime_changed(GMPC_MPDDATA_MODEL_PLAYLIST(playlist),a,b,NULL);

    gtk_widget_grab_focus(self->priv->pl3_cp_tree);
}
static void pl3_current_playlist_browser_unselected(GmpcPluginBrowserIface *obj, GtkContainer *container)
{
    PlayQueuePlugin *self = (PlayQueuePlugin *)obj;
    gtk_container_remove(GTK_CONTAINER(container), self->priv->pl3_cp_vbox);
}


static int pl3_current_playlist_browser_option_menu(GmpcPluginBrowserIface *obj, GtkMenu *menu)
{
    /* here we have:  Save, Clear*/
    GtkWidget *item;
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
static int pl3_current_playlist_tool_menu_integration(GmpcPluginToolMenuIface *obj, GtkMenu *menu)
{
    GtkWidget *item;
    item = gtk_image_menu_item_new_with_label(_("Add URL"));
    gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
		    gtk_image_new_from_icon_name("add-url", GTK_ICON_SIZE_MENU));
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
    g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(url_start), NULL);

    return 1;
}

static int  pl3_current_playlist_browser_key_release_event(GtkTreeView *tree, GdkEventKey *event,PlayQueuePlugin *self)
{
    if(event->keyval == GDK_Delete)
    {
        pl3_current_playlist_browser_delete_selected_songs (self);
        return TRUE;                                          		
    }
    else if(event->keyval == GDK_i && event->state&GDK_MOD1_MASK)
    {
        pl3_current_playlist_browser_show_info(self);
        return TRUE;
    }
    else if (event->keyval == GDK_space)
    {
        pl3_current_playlist_browser_scroll_to_current_song(self);
        pl3_current_playlist_browser_select_current_song(self);
        return TRUE;			
    }
    else if (event->keyval == GDK_f && event->state&GDK_CONTROL_MASK)
    {
        mod_fill_entry_changed(self->priv->filter_entry,self);
        gtk_widget_grab_focus(self->priv->filter_entry);        
        self->priv->search_keep_open = TRUE;
        return TRUE;
    }
    else if((event->state&(GDK_CONTROL_MASK|GDK_MOD1_MASK)) == 0 )/*&& 
            ((event->keyval >= GDK_space && event->keyval <= GDK_z)))*/
    {
        char data[10];
        guint32 uc = gdk_keyval_to_unicode(event->keyval);
        if(uc)
        {
            memset(data,'\0',10);
            g_unichar_to_utf8 (uc, data);
            gtk_widget_grab_focus(self->priv->filter_entry);      
            gtk_entry_set_text(GTK_ENTRY(self->priv->filter_entry),data);
            gtk_editable_set_position(GTK_EDITABLE(self->priv->filter_entry),1);

            return TRUE;
        }
    }
    return pl3_window_key_press_event(GTK_WIDGET(tree),event);
}

/* create a dialog that allows the user to save the current playlist */
static void pl3_current_playlist_save_playlist (void)
{
    gchar *str;
    GtkBuilder *xml = NULL;
    int run = TRUE;
    /* check if the connection is up */
    if (!mpd_check_connected(connection))
    {
        return;
    }
    /* create the interface */
    str = gmpc_get_full_glade_path("playlist-save-dialog.ui");
    xml = gtk_builder_new();
    gtk_builder_add_from_file(xml, str, NULL);
    q_free(str);

    /* run the interface */
    do
    {
        switch (gtk_dialog_run (GTK_DIALOG ((GtkWidget *) gtk_builder_get_object (xml, "save_pl"))))
        {
            case GTK_RESPONSE_OK:
                run = FALSE;
                /* if the users agrees do the following: */
                /* get the song-name */
                str = (gchar *)	gtk_entry_get_text (GTK_ENTRY
                        ((GtkWidget *) gtk_builder_get_object (xml, "pl-entry")));
                /* check if the user entered a name, we can't do withouth */
                /* TODO: disable ok button when nothing is entered */
                /* also check if there is a connection */
                if (strlen (str) != 0 && mpd_check_connected(connection))
                {
                    int retv = mpd_database_save_playlist(connection, str);
                    if(retv == MPD_DATABASE_PLAYLIST_EXIST )
                    {
                        gchar *errormsg = g_markup_printf_escaped(_("<i>Playlist <b>\"%s\"</b> already exists\nOverwrite?</i>"), str);
                        gtk_label_set_markup(GTK_LABEL((GtkWidget *) gtk_builder_get_object(xml, "label_error")), errormsg);
                        gtk_widget_show((GtkWidget *)gtk_builder_get_object(xml, "hbox5"));
                        /* ask to replace */
                        gtk_widget_set_sensitive(GTK_WIDGET((GtkWidget *) gtk_builder_get_object(xml, "pl-entry")), FALSE);
                        switch (gtk_dialog_run (GTK_DIALOG ((GtkWidget *) gtk_builder_get_object (xml, "save_pl"))))
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
                        gtk_widget_set_sensitive(GTK_WIDGET((GtkWidget *) gtk_builder_get_object(xml, "pl-entry")), TRUE);
                        gtk_widget_hide((GtkWidget *) gtk_builder_get_object(xml, "hbox5"));

                        q_free(errormsg);
                    }
                    else if (retv != MPD_OK)
                    {
                        playlist3_show_error_message(_("Failed to save the playlist file."), ERROR_WARNING);
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
    gtk_widget_destroy ((GtkWidget *) gtk_builder_get_object (xml, "save_pl"));

    /* unref the gui description */
    g_object_unref (xml);
}

static void pl3_current_playlist_browser_clear_playlist(void)
{
    mpd_playlist_clear(connection);
}

static void pl3_current_playlist_browser_shuffle_playlist(void)
{
    mpd_playlist_shuffle(connection);
}

static void pl3_current_playlist_status_changed(GmpcConnection *conn, MpdObj *mi, ChangedStatusType what, PlayQueuePlugin *self)
{
    if(self->priv->pl3_cp_vbox == NULL)
        return;
    if(what&MPD_CST_PLAYLIST)
    {

        if(self->priv->quick_search)
            mod_fill_do_entry_changed(self);
        if(self->priv->pl3_curb_tree_ref) {
            GtkTreeIter iter;
            GtkTreePath *path;
			path = gtk_tree_row_reference_get_path(self->priv->pl3_curb_tree_ref);
			if(path)
			{
				if(gtk_tree_model_get_iter(GTK_TREE_MODEL(gtk_tree_row_reference_get_model(self->priv->pl3_curb_tree_ref)), &iter,path))
				{
                    gchar *title = g_strdup_printf("<span color='grey'>(%i)</span>", mpd_playlist_get_playlist_length(connection));
                    gtk_list_store_set(GTK_LIST_STORE(gtk_tree_row_reference_get_model(self->priv->pl3_curb_tree_ref)), &iter,
                        PL3_CAT_NUM_ITEMS, title, -1);
                    g_free(title);
                }
				gtk_tree_path_free(path);
			}

        }
    }
}

static void pl3_current_playlist_browser_activate(PlayQueuePlugin *self)
{
    GtkTreeSelection *selec = gtk_tree_view_get_selection((GtkTreeView *)
            glade_xml_get_widget (pl3_xml, "cat_tree"));


    GtkTreePath *path = gtk_tree_row_reference_get_path(self->priv->pl3_curb_tree_ref); 
    if(path)
    {
        gtk_tree_selection_select_path(selec, path);
        gtk_tree_path_free(path);
    }
    gtk_widget_grab_focus(self->priv->pl3_cp_tree);
}

static void pl3_current_playlist_do_playlist_search(GmpcPluginBase *self)
{
    pl3_find2_select_plugin_id(self->id);
}
static int pl3_current_playlist_browser_add_go_menu(GmpcPluginBrowserIface *obj, GtkMenu *menu)
{
    GtkWidget *item = NULL;

    item = gtk_image_menu_item_new_with_label(_("Play Queue"));
    gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item), 
            gtk_image_new_from_icon_name("playlist-browser", GTK_ICON_SIZE_MENU));
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
    gtk_widget_add_accelerator(GTK_WIDGET(item), "activate", gtk_menu_get_accel_group(GTK_MENU(menu)), GDK_F1, 0, GTK_ACCEL_VISIBLE);
    g_signal_connect_swapped(G_OBJECT(item), "activate", 
            G_CALLBACK(pl3_current_playlist_browser_activate), obj);

    item = gtk_image_menu_item_new_with_label(_("Search Playlist"));
    gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item), 
            gtk_image_new_from_icon_name("gtk-find", GTK_ICON_SIZE_MENU));
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
    gtk_widget_add_accelerator(GTK_WIDGET(item), "activate", gtk_menu_get_accel_group(GTK_MENU(menu)), GDK_j, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    g_signal_connect_swapped(G_OBJECT(item), "activate", 
            G_CALLBACK(pl3_current_playlist_do_playlist_search), obj);
    return 2;
}

static void pl3_current_playlist_connection_changed(GmpcConnection *conn, MpdObj *mi, int connect,PlayQueuePlugin *self)
{
    if(!connect && self->priv->filter_entry)
    {
        gtk_entry_set_text(GTK_ENTRY(self->priv->filter_entry), "");
    }
}
/* function that saves the settings */
static void pl3_current_playlist_save_myself(GmpcPluginBase *obj)
{
    PlayQueuePlugin *self = (PlayQueuePlugin *)obj;
    if(self->priv->pl3_curb_tree_ref)
    {
        GtkTreePath *path = gtk_tree_row_reference_get_path(self->priv->pl3_curb_tree_ref);
        if(path)
        {
            gint *indices = gtk_tree_path_get_indices(path);
            cfg_set_single_value_as_int(config, "current-playlist","position",indices[0]);
            gtk_tree_path_free(path);
        }
    }
}

/**
 * GmpcPlugin
 */
GType play_queue_plugin_get_type(void);

static int *play_queue_plugin_get_version(GmpcPluginBase *plug, int *length)
{
	static int version[3] = {0,0,1};
	if(length) *length = 3;
	return (int *)version;
}

static const char *play_queue_plugin_get_name(GmpcPluginBase *plug)
{
	return N_("Play Queue");
}
static void play_queue_plugin_finalize(GObject *obj) {
    PlayQueuePlugin *self = (PlayQueuePlugin *)obj;
	PlayQueuePluginClass * klass = (g_type_class_peek (play_queue_plugin_get_type()));
	gpointer parent_class = g_type_class_peek_parent (klass);
	pl3_current_playlist_destroy((PlayQueuePlugin *)obj);

	if(self->priv){
        if(self->priv->uid) {
            g_free(self->priv->uid);
            self->priv->uid = NULL;
        }
        if(self->priv->mod_fill){
            g_object_unref(self->priv->mod_fill);
            self->priv->mod_fill = NULL;
        }
        if(g_signal_handler_is_connected(G_OBJECT(gmpcconn), self->priv->status_changed_handler))
            g_signal_handler_disconnect(G_OBJECT(gmpcconn), self->priv->status_changed_handler);

        if(g_signal_handler_is_connected(G_OBJECT(gmpcconn), self->priv->connection_changed_handler))
            g_signal_handler_disconnect(G_OBJECT(gmpcconn), self->priv->connection_changed_handler);
		g_free(self->priv);
		self->priv = NULL;
	}
	if(parent_class)
		G_OBJECT_CLASS(parent_class)->finalize(obj);
}

static GObject *play_queue_plugin_constructor(GType type, guint n_construct_properties, GObjectConstructParam * construct_properties) {
	PlayQueuePluginClass * klass;
	PlayQueuePlugin *self;
	GObjectClass * parent_class;
	klass = (g_type_class_peek (play_queue_plugin_get_type()));
	parent_class = G_OBJECT_CLASS (g_type_class_peek_parent (klass));
	self = (PlayQueuePlugin *) parent_class->constructor (type, n_construct_properties, construct_properties);

	/* setup private structure */
	self->priv = g_malloc0(sizeof(PlayQueuePluginPrivate));
    self->priv->status_changed_handler = g_signal_connect_object(G_OBJECT(gmpcconn), "status-changed", G_CALLBACK(pl3_current_playlist_status_changed), self, 0);
    self->priv->connection_changed_handler = g_signal_connect_object(G_OBJECT(gmpcconn), "connection-changed", G_CALLBACK(pl3_current_playlist_connection_changed), self, 0);
    /* quick search */
    self->priv->search_keep_open = FALSE;
    self->priv->filter_entry = NULL;
    self->priv->mod_fill = NULL;
    self->priv->quick_search = 0;
    self->priv->quick_search_timeout =0;
    /* other widgets */
    self->priv->pl3_cp_tree = NULL;
    self->priv->pl3_cp_vbox = NULL;
    self->priv->pl3_curb_tree_ref = NULL;

	/* Make it an internal plugin */
	GMPC_PLUGIN_BASE(self)->plugin_type = GMPC_PLUGIN_PL_BROWSER|GMPC_INTERNALL;
	pl3_cp_init(self);

    return G_OBJECT(self);
}
/**
 * Base GmpcPluginBase class
 */
static void play_queue_plugin_class_init (PlayQueuePluginClass *klass)
{
	/* Connect destroy and construct */ 
	G_OBJECT_CLASS(klass)->finalize =		play_queue_plugin_finalize;
	G_OBJECT_CLASS(klass)->constructor =	play_queue_plugin_constructor;
	G_OBJECT_CLASS (klass)->get_property = play_queue_get_property;
	G_OBJECT_CLASS (klass)->set_property = play_queue_set_property;
	/* Connect plugin functions */
	GMPC_PLUGIN_BASE_CLASS(klass)->get_version = play_queue_plugin_get_version;
	GMPC_PLUGIN_BASE_CLASS(klass)->get_name =	 play_queue_plugin_get_name;

	GMPC_PLUGIN_BASE_CLASS(klass)->save_yourself =	 pl3_current_playlist_save_myself;


	g_object_class_install_property (G_OBJECT_CLASS (klass), PLAY_QUEUE_UID,
            g_param_spec_string ("uid", "uid", "stores an unique id", "play-queue", G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB | G_PARAM_READABLE | G_PARAM_WRITABLE));

}
/**
 * Browser interface
 */
static void play_queue_browser_iface_init(GmpcPluginBrowserIfaceIface * iface) {
	iface->browser_add = pl3_current_playlist_browser_add;
    iface->browser_selected = pl3_current_playlist_browser_selected;
    iface->browser_unselected = pl3_current_playlist_browser_unselected;
    iface->browser_option_menu = pl3_current_playlist_browser_option_menu; 
    iface->browser_add_go_menu = pl3_current_playlist_browser_add_go_menu; 
}
/**
 * Tool menu interface
 */
static void play_queue_plugin_tool_menu_iface_init (GmpcPluginToolMenuIfaceIface * iface) {
	iface->tool_menu_integration = pl3_current_playlist_tool_menu_integration;
}
/* Integrate Search */
static gboolean play_queue_plugin_is_field_supported(GmpcPluginIntegrateSearchIface *obj, mpd_TagItems tag)
{
    if(tag == MPD_TAG_NUM_OF_ITEM_TYPES) return TRUE; 
    return mpd_server_tag_supported(connection, tag);
}

static MpdData * play_queue_plugin_is_search(GmpcPluginIntegrateSearchIface *obj, mpd_TagItems num_field , const gchar *search_string)
{
    MpdData *data_t = NULL;
    if(num_field == MPD_TAG_NUM_OF_ITEM_TYPES){
        data_t = advanced_search(search_string, TRUE);
    }else{
        gchar ** splitted = tokenize_string(search_string);
        int i =0;
        gboolean found = FALSE;
        for(i=0;splitted && splitted[i];i++) {                                                        
            if(!found) {
                mpd_playlist_search_start(connection, FALSE);
                found = TRUE;
            }
            mpd_playlist_search_add_constraint(connection, num_field, splitted[i]);
        }
        if(splitted)
            g_strfreev(splitted);
        if(found) {
            data_t = mpd_playlist_search_commit(connection);
        }
    }
    return data_t;
}

static void play_queue_plugin_is_iface_init (GmpcPluginIntegrateSearchIfaceIface * iface) {
    iface->field_supported = play_queue_plugin_is_field_supported;
    iface->search = play_queue_plugin_is_search;
}

GType play_queue_plugin_get_type(void) {
	static GType play_queue_plugin_type_id = 0;
	if(play_queue_plugin_type_id == 0) {
		static const GTypeInfo info = {
			.class_size = sizeof(PlayQueuePluginClass),
			.class_init = (GClassInitFunc)play_queue_plugin_class_init,
			.instance_size = sizeof(PlayQueuePlugin),
			.n_preallocs = 0
		};
        static const GInterfaceInfo iface_info = {
            (GInterfaceInitFunc) play_queue_browser_iface_init,
            (GInterfaceFinalizeFunc) NULL, NULL};
        static const GInterfaceInfo iface_tm_info = {
            (GInterfaceInitFunc) play_queue_plugin_tool_menu_iface_init,
            (GInterfaceFinalizeFunc) NULL, NULL};

        static const GInterfaceInfo iface_is_info = {
            (GInterfaceInitFunc) play_queue_plugin_is_iface_init,
            (GInterfaceFinalizeFunc) NULL, NULL};
        play_queue_plugin_type_id = g_type_register_static(GMPC_PLUGIN_TYPE_BASE, "PlayQueuePlugin", &info, 0);

		g_type_add_interface_static (play_queue_plugin_type_id, GMPC_PLUGIN_TYPE_BROWSER_IFACE, &iface_info);
		g_type_add_interface_static (play_queue_plugin_type_id, GMPC_PLUGIN_TYPE_TOOL_MENU_IFACE, &iface_tm_info);
		g_type_add_interface_static (play_queue_plugin_type_id, GMPC_PLUGIN_TYPE_INTEGRATE_SEARCH_IFACE, &iface_is_info);
	}
	return play_queue_plugin_type_id;
}
PlayQueuePlugin * play_queue_plugin_new(const gchar *uid) {
	PlayQueuePlugin *plug  =g_object_new(play_queue_plugin_get_type(), "uid", uid, NULL);

    return plug;
}



