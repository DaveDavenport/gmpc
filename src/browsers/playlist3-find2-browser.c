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
#include "main.h"
#include "misc.h"
#include "playlist3.h"
#include "playlist3-find2-browser.h"
#include "gmpc-mpddata-model.h"
#include "gmpc-mpddata-treeview.h"
#include "advanced-search.h"
#include "playlist3-playlist-editor.h"
#ifdef USE_SYSTEM_LIBSEXY
#include <libsexy/sexy-icon-entry.h>
#endif

static void pl3_find2_browser_destroy(void);
static void pl3_find2_browser_selected(GtkWidget *);
static void pl3_find2_browser_unselected(GtkWidget *);
static void pl3_find2_browser_add(GtkWidget *cat_tree);
static int pl3_find2_browser_add_go_menu(GtkWidget *);
static void pl3_find2_browser_search(void);
static void pl3_find2_browser_row_activated(GtkTreeView *, GtkTreePath *);
static int pl3_find2_browser_playlist_key_press(GtkWidget *, GdkEventKey *);
static void pl3_find2_browser_add_selected(void);
static gboolean pl3_find2_browser_button_release_event(GtkWidget *but, GdkEventButton *event);
static void pl3_find2_browser_connection_changed(MpdObj *mi, int connect, gpointer data);
static gboolean pl3_find2_entry_key_press_event(GtkWidget *entry, GdkEventKey *event, gpointer data);
static void pl3_find2_browser_status_changed(MpdObj *mi,ChangedStatusType what, void *data);

static void pl3_find2_entry_changed(GtkWidget *entry, gpointer data);
static void pl3_find2_save_myself(void);



/* Playlist window row reference */
static GtkTreeRowReference *pl3_find2_ref = NULL;

/* internal */
static GtkWidget            *pl3_find2_curpl        = NULL;
static GtkListStore         *pl3_find2_curpl_model  = NULL;
static GtkWidget 			*pl3_find2_tree 		= NULL;
static GmpcMpdDataModel 	*pl3_find2_store2 		= NULL;
static GtkWidget 			*pl3_find2_vbox 		= NULL;
static GtkListStore		    *pl3_find2_combo_store 	= NULL;
static GtkListStore 		*pl3_find2_autocomplete = NULL;

static GtkWidget            *search_combo           = NULL;
static GtkWidget            *search_entry           = NULL;

#define QUERY_ENTRY MPD_TAG_NUM_OF_ITEM_TYPES
static void pl3_find2_fill_combo(gmpcPluginParent *plug)
{
    GtkTreeIter iter;
    int i=0, max = MPD_TAG_NUM_OF_ITEM_TYPES;
    gtk_list_store_clear(pl3_find2_combo_store);

    for(i=0;i< max;i++)
    {
        if(plug == NULL || gmpc_plugin_browser_integrate_search_field_supported(plug,i))
        {
            gtk_list_store_append(pl3_find2_combo_store, &iter);
            gtk_list_store_set(pl3_find2_combo_store, &iter, 1, mpdTagItemKeys[i], 0,i, -1);	
        }
    }
    if(plug == NULL || gmpc_plugin_browser_integrate_search_field_supported(plug,QUERY_ENTRY))
    {
        gtk_list_store_append(pl3_find2_combo_store, &iter);
        gtk_list_store_set(pl3_find2_combo_store, &iter, 1, _("Query"), 0,QUERY_ENTRY, -1);	
    }
}

static void pl3_find2_combo_box_field_changed(GtkComboBox *cb, gpointer data)
{
	GtkTreeIter iter;
	if(gtk_combo_box_get_active_iter(cb, &iter))
	{
		gint selected_type;
		gtk_tree_model_get(GTK_TREE_MODEL(pl3_find2_combo_store), &iter, 0, &selected_type, -1);
        if(gtk_combo_box_get_active_iter(GTK_COMBO_BOX(data), &iter))
        {
            gchar *type;
            gchar *cfield=NULL;
            gtk_tree_model_get(gtk_combo_box_get_model(GTK_COMBO_BOX(data)), &iter, 0, &type, -1);
            if(type)
            {
                cfield = g_strdup_printf("selected_type_%s", type);
                g_free(type);
                cfg_set_single_value_as_int(config, "find2-browser", cfield, selected_type);
                g_free(cfield);
            }
        }
    }
}

static void playtime_changed(GmpcMpdDataModel *model, gulong playtime)
{
    if(pl3_cat_get_selected_browser() == find2_browser_plug.id)
    {
        playlist3_show_playtime(playtime);
    }
}

static void pl3_find2_browser_type_plugin_changed(GtkComboBox *box, gpointer user_data)
{
	GtkTreeIter iter;
	GtkTreeModel *model = gtk_combo_box_get_model(box);
	gmpcPluginParent *plug = NULL;
    gchar *cfield =NULL;
    gchar *type = NULL;
    gint selected_type;

    /* this should always be active */
    if(gtk_combo_box_get_active_iter(box, &iter))
	{
		gtk_tree_model_get(GTK_TREE_MODEL(model), &iter, 0, &type, 2, &plug,-1);
		pl3_find2_fill_combo(plug);
	}

    cfield = g_strdup_printf("selected_type_%s", type);
    selected_type = cfg_get_single_value_as_int_with_default(config, "find2-browser",cfield, 0);
    g_free(cfield);
    g_free(type);
    /* Loop through the field until the match is found */
    for(gtk_tree_model_get_iter_first(GTK_TREE_MODEL(pl3_find2_combo_store), &iter);
            gtk_list_store_iter_is_valid(pl3_find2_combo_store, &iter);
            gtk_tree_model_iter_next(GTK_TREE_MODEL(pl3_find2_combo_store), &iter))
    {
        gint sel_type;
        gtk_tree_model_get(GTK_TREE_MODEL(pl3_find2_combo_store), &iter, 0, &sel_type, -1);
        if(sel_type == selected_type)
        {
            gtk_combo_box_set_active_iter(GTK_COMBO_BOX(search_combo), &iter);
            return;
        }
    }
    /* default, if nothing is found. */
    gtk_combo_box_set_active(GTK_COMBO_BOX(search_combo), 0);
}

#ifndef USE_SYSTEM_LIBSEXY
#if GTK_CHECK_VERSION(2,16,0)
static void pl3_find2_browser_clear_search_entry(GtkEntry *entry, GtkEntryIconPosition icon_pos, GdkEvent *event, gpointer user_data)
{
    if(icon_pos == GTK_ENTRY_ICON_SECONDARY){
        gtk_entry_set_text(GTK_ENTRY(entry), "");
    }
}
#endif
#endif
/**
 * Construct the browser 
 */
static void pl3_find2_browser_init(void)
{
	GtkWidget *button;
    GtkWidget  *pl3_find2_sw = NULL;
    GtkWidget *hbox = NULL;
    GtkCellRenderer *renderer;
    GtkEntryCompletion *entrcomp;
    int i;
	/* autocomplete later on */
	pl3_find2_autocomplete = gtk_list_store_new(1, G_TYPE_STRING);

	pl3_find2_store2 = gmpc_mpddata_model_new();
    gmpc_mpddata_model_disable_image(GMPC_MPDDATA_MODEL(pl3_find2_store2));
    g_signal_connect(G_OBJECT(pl3_find2_store2), "playtime_changed", G_CALLBACK(playtime_changed), NULL);


    pl3_find2_combo_store = gtk_list_store_new(2,G_TYPE_INT, G_TYPE_STRING);
    /** Fill the view */
    pl3_find2_fill_combo(NULL);

    /* Column */
     /* set up the tree */
    pl3_find2_tree= gmpc_mpddata_treeview_new("find2-browser",TRUE,GTK_TREE_MODEL(pl3_find2_store2));
    gmpc_mpddata_treeview_enable_click_fix(GMPC_MPDDATA_TREEVIEW(pl3_find2_tree));
    /* Disable interactive search, somewhat*/
    gtk_tree_view_set_enable_search(GTK_TREE_VIEW(pl3_find2_tree), FALSE);
    /* setup signals */
    g_signal_connect(G_OBJECT(pl3_find2_tree), "row-activated",G_CALLBACK(pl3_find2_browser_row_activated), NULL); 
    g_signal_connect(G_OBJECT(pl3_find2_tree), "button-release-event", G_CALLBACK(pl3_find2_browser_button_release_event), NULL);
    g_signal_connect(G_OBJECT(pl3_find2_tree), "key-press-event", G_CALLBACK(pl3_find2_browser_playlist_key_press), NULL);

    /* set up the scrolled window */
    pl3_find2_sw = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(pl3_find2_sw), GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(pl3_find2_sw), GTK_SHADOW_ETCHED_IN);
    gtk_container_add(GTK_CONTAINER(pl3_find2_sw), pl3_find2_tree);

    /* set initial state */

    pl3_find2_vbox = gtk_vbox_new(FALSE, 6);
    gtk_box_pack_start(GTK_BOX(pl3_find2_vbox), pl3_find2_sw, TRUE, TRUE,0);

    /** Add a default item */
    hbox = gtk_hbox_new(FALSE, 6);

    /* search in playlist */
    pl3_find2_curpl_model = gtk_list_store_new(3, G_TYPE_STRING, G_TYPE_INT, G_TYPE_POINTER);
    pl3_find2_curpl = gtk_combo_box_new_with_model(GTK_TREE_MODEL(pl3_find2_curpl_model));
    renderer = gtk_cell_renderer_text_new();
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(pl3_find2_curpl), renderer, TRUE);
    gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(pl3_find2_curpl), renderer, "text", 0, NULL);

    for(i=0;i< num_plugins;i++){
        if(gmpc_plugin_get_enabled(plugins[i])){
            if(gmpc_plugin_is_browser(plugins[i])){
                if(gmpc_plugin_browser_has_integrate_search(plugins[i])){
                    GtkTreeIter titer;
                    gtk_list_store_append(GTK_LIST_STORE(pl3_find2_curpl_model),&titer);
                    gtk_list_store_set(GTK_LIST_STORE(pl3_find2_curpl_model), &titer,
                                0, gmpc_plugin_get_name(plugins[i]),
                                1,0,
                                2, plugins[i],
                                -1);
                }
            }
        }
    }

    gtk_combo_box_set_active(GTK_COMBO_BOX(pl3_find2_curpl), 0);
	g_signal_connect(G_OBJECT(pl3_find2_curpl), "changed", G_CALLBACK(pl3_find2_browser_type_plugin_changed), NULL);
	
    gtk_box_pack_start(GTK_BOX(hbox), pl3_find2_curpl, FALSE, TRUE,0);

	/* What tag field */
    search_combo= gtk_combo_box_new();

    renderer = gtk_cell_renderer_text_new();
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(search_combo), renderer, TRUE);
    gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(search_combo), renderer, "text", 1, NULL);
    gtk_combo_box_set_model(GTK_COMBO_BOX(search_combo), GTK_TREE_MODEL(pl3_find2_combo_store));

    /* Update the tag combo to the right field */
    pl3_find2_browser_type_plugin_changed(GTK_COMBO_BOX(pl3_find2_curpl),NULL);

    gtk_box_pack_start(GTK_BOX(hbox), search_combo, FALSE, TRUE, 0);
    g_signal_connect(G_OBJECT(search_combo), "changed", G_CALLBACK(pl3_find2_combo_box_field_changed), pl3_find2_curpl);


#ifdef USE_SYSTEM_LIBSEXY
    search_entry = sexy_icon_entry_new();
    sexy_icon_entry_add_clear_button(SEXY_ICON_ENTRY(search_entry));
#else
    search_entry = gtk_entry_new();
#if GTK_CHECK_VERSION(2,16,0)
    gtk_entry_set_icon_from_stock(GTK_ENTRY(search_entry), GTK_ENTRY_ICON_SECONDARY, GTK_STOCK_CLEAR);
    g_signal_connect(GTK_ENTRY(search_entry), "icon-press", G_CALLBACK(pl3_find2_browser_clear_search_entry), NULL);
#endif
#endif
	entrcomp = gtk_entry_completion_new();
	gtk_entry_completion_set_text_column(entrcomp, 0);
	gtk_entry_completion_set_inline_completion(entrcomp, TRUE);
	gtk_entry_completion_set_model(GTK_ENTRY_COMPLETION(entrcomp), GTK_TREE_MODEL(pl3_find2_autocomplete));
	gtk_entry_completion_set_popup_completion(GTK_ENTRY_COMPLETION(entrcomp), TRUE);
	gtk_entry_set_completion(GTK_ENTRY(search_entry), entrcomp);


    g_signal_connect(G_OBJECT(search_entry), "activate",G_CALLBACK(pl3_find2_browser_search), NULL);
    g_signal_connect(G_OBJECT(search_entry), "key-press-event",G_CALLBACK(pl3_find2_entry_key_press_event), NULL);
    g_signal_connect(G_OBJECT(search_entry), "changed",G_CALLBACK(pl3_find2_entry_changed), NULL);
    gtk_box_pack_start(GTK_BOX(hbox), search_entry, TRUE, TRUE, 0);


    /* find button */
    button = gtk_button_new_from_stock(GTK_STOCK_FIND);
    g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(pl3_find2_browser_search),NULL);
    gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, TRUE,0);       



    gtk_box_pack_start(GTK_BOX(pl3_find2_vbox), hbox, FALSE,TRUE,0);

    gtk_widget_show_all(pl3_find2_vbox);
    g_object_ref_sink(G_OBJECT(pl3_find2_vbox));
}

static void pl3_find2_browser_selected(GtkWidget *container)
{
    if(pl3_find2_tree == NULL)
    {
        pl3_find2_browser_init();
    }
    gtk_container_add(GTK_CONTAINER(container),pl3_find2_vbox);
    gtk_widget_grab_focus(pl3_find2_tree);
    gtk_widget_show(pl3_find2_vbox);

    playlist3_show_playtime(gmpc_mpddata_model_get_playtime(GMPC_MPDDATA_MODEL(pl3_find2_store2))); 
}
static void pl3_find2_browser_unselected(GtkWidget *container)
{
    gtk_container_remove(GTK_CONTAINER(container), pl3_find2_vbox);
}




/*****************************************************************
 * Find Browser
 */
static void pl3_find2_browser_add(GtkWidget *cat_tree)
{
    GtkTreePath *path = NULL;
    GtkTreeIter iter;
	gint pos = cfg_get_single_value_as_int_with_default(config, "find2-browser","position",4);
	playlist3_insert_browser(&iter, pos);
    gtk_list_store_set(GTK_LIST_STORE(pl3_tree), &iter, 
            PL3_CAT_TYPE, find2_browser_plug.id,
            PL3_CAT_TITLE, _("Search"),
            PL3_CAT_ICON_ID, "gtk-find",
            -1);

    if (pl3_find2_ref) {
        gtk_tree_row_reference_free(pl3_find2_ref);
        pl3_find2_ref = NULL;
    }

    path = gtk_tree_model_get_path(GTK_TREE_MODEL(playlist3_get_category_tree_store()), &iter);
    if (path) {
        pl3_find2_ref = gtk_tree_row_reference_new(GTK_TREE_MODEL(playlist3_get_category_tree_store()), path);
        gtk_tree_path_free(path);
    }
}

/**
 * Search 
 */
static void pl3_find2_browser_search(void)
{
    GtkTreeIter iter,cc_iter;
    if(pl3_find2_vbox == NULL)
        return;

    gtk_tree_view_set_model(GTK_TREE_VIEW(pl3_find2_tree), NULL);
    if(gtk_combo_box_get_active_iter(GTK_COMBO_BOX(pl3_find2_curpl), &iter)){
        int type;
        int num_field;
        gmpcPluginParent *plug = NULL;
        gtk_tree_model_get(GTK_TREE_MODEL(pl3_find2_curpl_model), &iter, 1, &type, 2, &plug,-1);

        if(gtk_combo_box_get_active_iter(GTK_COMBO_BOX(search_combo), &cc_iter))
        {
            GError *error = NULL;
            MpdData *data_t = NULL;
            const gchar *real_name = gtk_entry_get_text(GTK_ENTRY(search_entry));
            gchar *name = g_strstrip(g_strdup(real_name));
            gtk_tree_model_get(GTK_TREE_MODEL(pl3_find2_combo_store),&cc_iter , 0, &num_field, -1);
            if(plug)
                data_t = gmpc_plugin_browser_integrate_search(plug,num_field, name,&error); 
            if(error){
                gchar *mes = g_markup_printf_escaped("<b>%s</b> %s: %s",gmpc_plugin_get_name(plug),_("Search failed"), error->message);
                playlist3_show_error_message(mes, ERROR_WARNING);
                g_free(mes);
                g_error_free(error);
                error = NULL;
            }
            gmpc_mpddata_model_set_mpd_data(pl3_find2_store2, data_t);
            /* update autocompletion */
            if(data_t) {
                int found2 = 0;
                for(gtk_tree_model_get_iter_first(GTK_TREE_MODEL(pl3_find2_autocomplete), &iter);
                        gtk_list_store_iter_is_valid(pl3_find2_autocomplete, &iter) && !found2;
                        gtk_tree_model_iter_next(GTK_TREE_MODEL(pl3_find2_autocomplete), &iter))
                {
                    gchar *entry;
                    gtk_tree_model_get(GTK_TREE_MODEL(pl3_find2_autocomplete), &iter, 0,&entry,-1);
                    if(strcmp(entry, name) == 0)
                    {
                        found2 = TRUE;
                    }
                    g_free(entry);
                }
                if(!found2) {
                    gtk_list_store_insert_with_values(pl3_find2_autocomplete, &iter,-1, 0,name,-1);
                }					
            }
            g_free(name);
        }
    }
    gtk_tree_view_set_model(GTK_TREE_VIEW(pl3_find2_tree), GTK_TREE_MODEL(pl3_find2_store2));
    /* Update the row counter */
    if(pl3_find2_ref) {
        GtkTreePath *path;
        path = gtk_tree_row_reference_get_path(pl3_find2_ref);
        if(path)
        {
            if(gtk_tree_model_get_iter(GTK_TREE_MODEL(gtk_tree_row_reference_get_model(pl3_find2_ref)), &iter,path))
            {
                gchar *title = g_strdup_printf("<span color='grey'>(%i)</span>", 
                        gtk_tree_model_iter_n_children(GTK_TREE_MODEL(pl3_find2_store2),NULL));
                gtk_list_store_set(GTK_LIST_STORE(gtk_tree_row_reference_get_model(pl3_find2_ref)), &iter,
                        PL3_CAT_NUM_ITEMS, title, -1);
                g_free(title);
            }
            gtk_tree_path_free(path);
        }
    }
}


static void pl3_find2_browser_show_info(void)
{
    GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW(pl3_find2_tree));
    GtkTreeSelection *selection =gtk_tree_view_get_selection (GTK_TREE_VIEW(pl3_find2_tree));
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
            mpd_Song *song =NULL;
            gtk_tree_model_get_iter (model, &iter, (GtkTreePath *) list->data);
            gtk_tree_model_get(model,&iter,MPDDATA_MODEL_COL_MPDSONG, &song,-1);
			if(song) {
				info2_activate();
				info2_fill_song_view(song);	
			}
        }

        g_list_foreach (list, (GFunc) gtk_tree_path_free, NULL);
        g_list_free (list);
    }
}

static void pl3_find2_browser_row_activated(GtkTreeView *tree, GtkTreePath *tp)
{
    GtkTreeIter iter;
    gchar *song_id;
    gint r_type;
    gtk_tree_model_get_iter(gtk_tree_view_get_model(tree), &iter, tp);
    gtk_tree_model_get(gtk_tree_view_get_model(tree), &iter,MPDDATA_MODEL_COL_PATH,&song_id,MPDDATA_MODEL_ROW_TYPE, &r_type, -1);
    {
			play_path(song_id);
		}

    q_free(song_id);
}

static void pl3_find2_browser_replace_selected(void)
{
    mpd_playlist_clear(connection);
    if(mpd_check_connected(connection))
    {
        pl3_find2_browser_add_selected();
        mpd_player_play(connection);	
    }
}

static int pl3_find2_browser_playlist_key_press(GtkWidget *tree, GdkEventKey *event)
{
    if(event->state == GDK_CONTROL_MASK && (event->keyval == GDK_Insert || event->keyval == GDK_KP_Insert))
    {
        pl3_find2_browser_replace_selected();		
    }
    else if(event->keyval == GDK_Insert || event->keyval == GDK_KP_Insert)
    {
        pl3_find2_browser_add_selected();		
    }
    else if(event->keyval == GDK_i && event->state&GDK_MOD1_MASK)
    {
        pl3_find2_browser_show_info();
    }
    /* Make ctrl-f focus the search entry */
    else if (event->keyval == GDK_f && event->state&GDK_CONTROL_MASK)
    {
        gtk_widget_grab_focus(GTK_WIDGET(search_entry));
        return FALSE;
    }
    else
    {
        return pl3_window_key_press_event(tree,event);
    }
    return TRUE;
}



static void pl3_find2_browser_add_selected(void)
{
    GtkTreeIter iter;
    GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW(pl3_find2_tree));
    GtkTreeModel *model = GTK_TREE_MODEL (pl3_find2_store2);
    GList *rows = gtk_tree_selection_get_selected_rows (selection, &model);
    int songs=0;
    if(rows != NULL)
    {
        gchar *name;
        gint type;
        GList *node = g_list_first(rows);
        do
        {
            GtkTreePath *path = node->data;
            gtk_tree_model_get_iter (model, &iter, path);
            gtk_tree_model_get (model, &iter,MPDDATA_MODEL_COL_PATH,&name,MPDDATA_MODEL_ROW_TYPE, &type, -1);	  
            /* does this bitmask thingy works ok? I think it hsould */
            if(type == MPD_DATA_TYPE_SONG)
            {
                if((songs&16383)==16383){
                    mpd_playlist_queue_commit(connection);
                }
                /* add them to the add list */
                mpd_playlist_queue_add(connection, name);
                songs++;
            }
            q_free(name);
        }while((node = g_list_next(node)) != NULL);
    }
    /* if there are items in the add list add them to the playlist */
    mpd_playlist_queue_commit(connection);
    if(songs != 0)
    {
        gchar * message = g_strdup_printf("Added %i song%s", songs, (songs != 1)? "s":"");
        pl3_push_statusbar_message(message);
        q_free(message);
    }

    g_list_foreach (rows, (GFunc) gtk_tree_path_free, NULL);
    g_list_free (rows);
}

static void pl3_find2_browser_add_all(void)
{
    GtkTreeIter iter;
    GtkTreeModel *model = GTK_TREE_MODEL (pl3_find2_store2);
    int songs=0;
    if(gtk_tree_model_get_iter_first(model, &iter))
    {
        gchar *name;
        gint type;
        do
        {
            gtk_tree_model_get (model, &iter,MPDDATA_MODEL_COL_PATH,&name,MPDDATA_MODEL_ROW_TYPE, &type, -1);	  
            /* does this bitmask thingy works ok? I think it hsould */
            if(type == MPD_DATA_TYPE_SONG)
            {
                if((songs&16383)==16383){
                    mpd_playlist_queue_commit(connection);
                }
                /* add them to the add list */
                mpd_playlist_queue_add(connection, name);
                songs++;
            }
            q_free(name);
        }while(gtk_tree_model_iter_next(model, &iter));
    }
    /* if there are items in the add list add them to the playlist */
    mpd_playlist_queue_commit(connection);
    if(songs != 0)
    {
        gchar * message = g_strdup_printf("Added %i song%s", songs, (songs != 1)? "s":"");
        pl3_push_statusbar_message(message);
        q_free(message);
    }
}
static void pl3_find2_playlist_editor_add_to_playlist(GtkWidget *menu, gpointer cb_data)
{
    GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(pl3_find2_tree));
    GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(pl3_find2_tree));
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

/**
 * Play the selected row, only usefull if one row is selected
 */
static void pl3_find2_activate_row(GtkWidget *item,GtkTreeView *tree)
{
	GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(pl3_find2_tree));
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(pl3_find2_tree));
	GList *iter, *list = gtk_tree_selection_get_selected_rows (selection, &model);
	if(list)
	{
		iter = g_list_first(list);
		do{
			GtkTreeIter giter;
			if(gtk_tree_model_get_iter(model, &giter, (GtkTreePath *)iter->data))
			{
				gchar *song_id;
				gtk_tree_model_get(gtk_tree_view_get_model(tree), &giter,MPDDATA_MODEL_COL_PATH,&song_id, -1);
				play_path(song_id);
				q_free(song_id);
			}
		}while((iter = g_list_next(iter)));

		g_list_foreach (list, (GFunc) gtk_tree_path_free, NULL);                        	
		g_list_free (list);
	}
}

static gboolean pl3_find2_browser_button_release_event(GtkWidget *but, GdkEventButton *event)
{
    if(event->button != 3) return FALSE;
    else if(gtk_tree_selection_count_selected_rows(gtk_tree_view_get_selection(GTK_TREE_VIEW(pl3_find2_tree))) > 0)
    {
      GtkWidget *item;
      GtkWidget *menu = gtk_menu_new();
/*
			if(gtk_tree_selection_count_selected_rows(gtk_tree_view_get_selection(GTK_TREE_VIEW(pl3_find2_tree))) == 1)	
			{
				item = gtk_image_menu_item_new_from_stock(GTK_STOCK_MEDIA_PLAY,NULL);
				gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
				g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(pl3_find2_activate_row), pl3_find2_tree);
			}

*/
      item = gtk_image_menu_item_new_from_stock(GTK_STOCK_ADD,NULL);
      gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
      g_signal_connect(G_OBJECT(item), "activate",
          G_CALLBACK(pl3_find2_browser_add_selected), NULL);
      gtk_widget_show(item);

      item = gtk_image_menu_item_new_with_label(_("Add all"));
      gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item), gtk_image_new_from_stock(GTK_STOCK_ADD, GTK_ICON_SIZE_MENU));
      gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
      g_signal_connect(G_OBJECT(item), "activate",
          G_CALLBACK(pl3_find2_browser_add_all), NULL);
      gtk_widget_show(item);

           /* add the replace widget */
      item = gtk_image_menu_item_new_with_label(_("Replace"));
      gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
          gtk_image_new_from_stock(GTK_STOCK_REDO, GTK_ICON_SIZE_MENU));
      gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
      g_signal_connect(G_OBJECT(item), "activate",
          G_CALLBACK(pl3_find2_browser_replace_selected), NULL);
      gtk_widget_show(item);

      if(mpd_server_check_version(connection,0,12,0))
      {
        item = gtk_image_menu_item_new_from_stock(GTK_STOCK_DIALOG_INFO,NULL);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
        g_signal_connect(G_OBJECT(item), "activate",
            G_CALLBACK(pl3_find2_browser_show_info), NULL);
        gtk_widget_show(item);

      }
      /* add sub menu */
      if(gtk_tree_selection_count_selected_rows(gtk_tree_view_get_selection(GTK_TREE_VIEW(pl3_find2_tree))) == 1)
      {
          mpd_Song *song = NULL;
          GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(pl3_find2_tree));
          GtkTreePath *path;
          GtkTreeIter iter;
          GList *list = gtk_tree_selection_get_selected_rows(gtk_tree_view_get_selection(GTK_TREE_VIEW(pl3_find2_tree)),&model);
          path = list->data;
          /* free result */
          g_list_free(list);
          if(path && gtk_tree_model_get_iter(model, &iter, path)) {
              gtk_tree_model_get(model, &iter, MPDDATA_MODEL_COL_MPDSONG, &song, -1);
              if(song)
                  submenu_for_song(menu, song);
          }
          if(path)
              gtk_tree_path_free(path);
      }


      playlist_editor_right_mouse(menu,pl3_find2_playlist_editor_add_to_playlist, NULL);
      gmpc_mpddata_treeview_right_mouse_intergration(GMPC_MPDDATA_TREEVIEW(pl3_find2_tree), GTK_MENU(menu));

      gtk_widget_show_all(menu);
      gtk_menu_popup(GTK_MENU(menu), NULL, NULL,NULL, NULL, 0, event->time);
      return TRUE;
    }
    return FALSE;
}

static void pl3_find2_browser_activate(void)
{
    GtkTreeSelection *selec = gtk_tree_view_get_selection((GtkTreeView *)
            glade_xml_get_widget (pl3_xml, "cat_tree"));

    GtkTreePath *path = gtk_tree_row_reference_get_path(pl3_find2_ref);
    if(path)
    {
        gtk_tree_selection_select_path(selec, path);
        gtk_tree_path_free(path);
    }
    gtk_widget_grab_focus(GTK_WIDGET(search_entry));
}

static int pl3_find2_browser_add_go_menu(GtkWidget *menu)
{
    GtkWidget *item = NULL;

    item = gtk_image_menu_item_new_with_label(_("Search"));
    gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item), 
            gtk_image_new_from_icon_name("gtk-find", GTK_ICON_SIZE_MENU));
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	gtk_widget_add_accelerator(GTK_WIDGET(item), 
            "activate",
            gtk_menu_get_accel_group(GTK_MENU(menu)),
            GDK_F3, 0,
            GTK_ACCEL_VISIBLE);
    g_signal_connect(G_OBJECT(item), "activate", 
            G_CALLBACK(pl3_find2_browser_activate), NULL);

    return 1;
}

static void pl3_find2_browser_connection_changed(MpdObj *mi, int connect, gpointer data)
{
    /* Clear the list */
    if(connect)
    {
        if(pl3_find2_curpl)
            pl3_find2_browser_type_plugin_changed(GTK_COMBO_BOX(pl3_find2_curpl), NULL);
        pl3_find2_browser_search();
    }
}

void pl3_find2_select_plugin_id(int id)
{
    GtkTreeIter iter;
    GtkTreeModel *model;
    pl3_find2_browser_activate();
    model = gtk_combo_box_get_model(GTK_COMBO_BOX(pl3_find2_curpl));        
    if(gtk_tree_model_get_iter_first(model, &iter)){
        int found = 1;
        do{
            gmpcPluginParent *plug;
            gtk_tree_model_get(model, &iter,2, &plug, -1); 
            if(plug != NULL  && gmpc_plugin_get_id(plug) == id){ 
                gtk_combo_box_set_active_iter(GTK_COMBO_BOX(pl3_find2_curpl), &iter);
                found = 0;
            }
        }while(gtk_tree_model_iter_next(model, &iter) && found);
    }
    gtk_widget_grab_focus(search_entry);
}

static guint entry_timeout = 0;

static gboolean do_entry_changed_search(GtkEntry *entry)
{
    pl3_find2_browser_search();
    entry_timeout = 0;
    return FALSE;
}

static void pl3_find2_entry_changed(GtkWidget *entry, gpointer data)
{
    if(entry_timeout > 0) {
        g_source_remove(entry_timeout);
        entry_timeout = 0;
    }
    if(cfg_get_single_value_as_int_with_default(config, "general", "search-as-you-type", 0) == 1)
    {
        entry_timeout = g_timeout_add(250, (GSourceFunc)do_entry_changed_search, entry);
    }
}
static gboolean pl3_find2_entry_key_press_event(GtkWidget *entry, GdkEventKey *event, gpointer data)
{
	if(event->keyval == GDK_Escape)
		gtk_entry_set_text(GTK_ENTRY(entry), "");

    return FALSE;
}

static void pl3_find2_browser_destroy(void)
{
  if(pl3_find2_ref)
  {
      GtkTreeIter iter;
      GtkTreePath *path;
      path = gtk_tree_row_reference_get_path(pl3_find2_ref);
      if(path)
      {
          if(gtk_tree_model_get_iter(GTK_TREE_MODEL(gtk_tree_row_reference_get_model(pl3_find2_ref)), &iter,path))
          {
              gtk_list_store_remove(GTK_LIST_STORE(gtk_tree_row_reference_get_model(pl3_find2_ref)), &iter);
          }
          gtk_tree_path_free(path);
      }
      gtk_tree_row_reference_free(pl3_find2_ref);
      pl3_find2_ref = NULL;
  }
  if(pl3_find2_vbox)
  {
    gtk_widget_destroy(pl3_find2_vbox);
    pl3_find2_vbox = NULL;
  }
  if(pl3_find2_store2)
  {
    g_object_unref(pl3_find2_store2);
    pl3_find2_store2 = NULL;
  }
}

static void pl3_find2_browser_status_changed(MpdObj *mi,ChangedStatusType what, void *data)
{
	if(what&MPD_CST_DATABASE)
	{
		pl3_find2_browser_search(); 
	}
    if(what&MPD_CST_PERMISSION)
    {
        if(pl3_find2_curpl)
            pl3_find2_browser_type_plugin_changed(GTK_COMBO_BOX(pl3_find2_curpl),NULL);
    }
}	
static void pl3_find2_save_myself(void)
{
	if(pl3_find2_ref)
	{
		GtkTreePath *path = gtk_tree_row_reference_get_path(pl3_find2_ref);
		if(path)
		{
			gint *indices = gtk_tree_path_get_indices(path);
			cfg_set_single_value_as_int(config, "find2-browser","position",indices[0]);
			gtk_tree_path_free(path);
		}
	}
}
/* Easy command integration */

void pl3_find2_do_search_any(const char *param)
{
    GtkTreeIter iter;
    GtkTreeModel *model;
    model  = gtk_combo_box_get_model(GTK_COMBO_BOX(search_combo));
    if(gtk_tree_model_get_iter_first(model, &iter)){
        int found = 1;
        do{
            int field;
            gtk_tree_model_get(model, &iter,0, &field, -1); 
            if(field == QUERY_ENTRY){
                gtk_combo_box_set_active_iter(GTK_COMBO_BOX(search_combo), &iter); 
                found = 0;
            }
        }while(gtk_tree_model_iter_next(model, &iter) && found);
    }
    gtk_entry_set_text(GTK_ENTRY(search_entry), param);
    gtk_widget_activate(search_entry);
}
static void pl3_find2_plugin_init(void)
{
    gmpc_easy_command_add_entry(gmpc_easy_command,
                _("switch search"),"",
                _("Switch to the search browser"),
                (GmpcEasyCommandCallback *)pl3_find2_browser_activate, NULL); 
}
/**
 * Plugin structure
 */
gmpcPlBrowserPlugin find2_browser_gbp = {
	.add = pl3_find2_browser_add,
	.selected = pl3_find2_browser_selected,
	.unselected = pl3_find2_browser_unselected,
	.add_go_menu = pl3_find2_browser_add_go_menu,
};

gmpcPlugin find2_browser_plug = {
	.name = 						N_("Search Browser"),
	.version = 						{1,1,1},
	.plugin_type = 					GMPC_PLUGIN_PL_BROWSER|GMPC_INTERNALL,
    .init    =                      pl3_find2_plugin_init,
    .destroy = 						pl3_find2_browser_destroy,
	.browser = 						&find2_browser_gbp,
	.mpd_status_changed = 			pl3_find2_browser_status_changed,
	.mpd_connection_changed = 		pl3_find2_browser_connection_changed,
	.save_yourself = 				pl3_find2_save_myself,
};
