/*
 *Copyright (C) 2004-2008 Qball Cow <qball@sarine.nl>
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
#include "playlist3-find2-browser.h"
#include "config1.h"

#include "gmpc-mpddata-model.h"
#include "gmpc-mpddata-treeview.h"

#include "playlist3-playlist-editor.h"

static void pl3_find2_browser_edit_columns(void);
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
static int pl3_find2_browser_key_press_event(GtkWidget *mw, GdkEventKey *event, int type);
static void pl3_find2_browser_status_changed(MpdObj *mi,ChangedStatusType what, void *data);
extern GladeXML *pl3_xml;
static void pl3_find2_combo_box_changed(GtkComboBox *cb, gpointer data);

static void pl3_find2_save_myself(void);

int pl3_find2_last_entry = MPD_TAG_ITEM_ANY;



typedef struct {
    GtkWidget *hbox;
    GtkWidget *combo;
    GtkWidget  *lcombo;
    GtkWidget *entry;
    GtkWidget *rmbut;
    int tag_type; 
}crit_struct;



/* Playlist window row reference */
static GtkTreeRowReference *pl3_find2_ref = NULL;


extern GladeXML *pl3_xml;

/* internal */
static GtkWidget            *pl3_find2_curpl        = NULL;
static GtkWidget 			*pl3_find2_tree 		= NULL;
static GmpcMpdDataModel 	*pl3_find2_store2 		= NULL;
static GtkWidget 			*pl3_find2_vbox 		= NULL;
static GtkWidget			*pl3_find2_findbut		= NULL;
static GtkWidget       	    *pl3_find2_critaddbut   = NULL;
static GtkListStore		    *pl3_find2_combo_store 	= NULL;
static GtkListStore 		*pl3_find2_autocomplete = NULL;
static GList 				*criterias 				= NULL;
static GtkWidget 			*pl3_find2_crit_vbox 	= NULL;

static void pl3_find2_fill_combo()
{
	GtkTreeIter iter;
	int i=0, max = 3;
	gtk_list_store_clear(pl3_find2_combo_store);

	if(mpd_server_check_version(connection,0,12,0))
	{
		max = MPD_TAG_NUM_OF_ITEM_TYPES;
	}
	for(i=0;i< max;i++)
	{
		gtk_list_store_append(pl3_find2_combo_store, &iter);
		gtk_list_store_set(pl3_find2_combo_store, &iter, 1, mpdTagItemKeys[i], 0,i, -1);	
	}
}

static void pl3_find2_browser_bg_style_changed(GtkWidget *vbox, GtkStyle *style,  GtkWidget *vp)
{
	gtk_widget_modify_bg(vp,GTK_STATE_NORMAL, &(GTK_WIDGET(vbox)->style->light[GTK_STATE_SELECTED]));
}

static void pl3_find2_browser_remove_crit(GtkWidget *button,crit_struct *cs)
{
    criterias = g_list_remove(criterias, cs);
    gtk_widget_destroy(cs->hbox);
    g_free(cs);
    if(!criterias)
    {
        gtk_widget_set_sensitive(pl3_find2_findbut, FALSE);
    }
    pl3_find2_browser_search();
    gtk_widget_set_sensitive(pl3_find2_critaddbut, TRUE);

    if(g_list_length(criterias) == 1) {
        crit_struct *cs1 = criterias->data;
        gtk_widget_hide(cs1->rmbut); 
    }

}
static void pl3_find2_combo_box_changed(GtkComboBox *cb, gpointer data)
{
  pl3_find2_last_entry= gtk_combo_box_get_active(cb);
}

/**
 * Add a criteria 
 */
static void pl3_find2_browser_add_crit()
{
    crit_struct *cs = g_malloc0(sizeof(*cs));
    GtkWidget *removebut = NULL;
    GtkCellRenderer *renderer = NULL;
	GtkEntryCompletion *ent_comp = NULL;

    cs->hbox = gtk_hbox_new(FALSE, 6);

    cs->lcombo = gtk_combo_box_new_text();
    gtk_combo_box_append_text(GTK_COMBO_BOX(cs->lcombo), _("And"));
    gtk_combo_box_append_text(GTK_COMBO_BOX(cs->lcombo), _("Or"));
    gtk_combo_box_set_active(GTK_COMBO_BOX(cs->lcombo), 0);
    gtk_box_pack_start(GTK_BOX(cs->hbox), cs->lcombo, FALSE, TRUE, 0);

    cs->combo= gtk_combo_box_new();

    renderer = gtk_cell_renderer_text_new();
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(cs->combo), renderer, TRUE);
    gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(cs->combo), renderer, "text", 1, NULL);
    gtk_combo_box_set_model(GTK_COMBO_BOX(cs->combo), GTK_TREE_MODEL(pl3_find2_combo_store));
    gtk_combo_box_set_active(GTK_COMBO_BOX(cs->combo), pl3_find2_last_entry);
    gtk_box_pack_start(GTK_BOX(cs->hbox), cs->combo, FALSE, TRUE, 0);
    g_signal_connect(G_OBJECT(cs->combo), "changed", G_CALLBACK(pl3_find2_combo_box_changed), NULL);


    cs->entry = gtk_entry_new();
	ent_comp = gtk_entry_completion_new();
	gtk_entry_completion_set_text_column(ent_comp, 0);
	gtk_entry_completion_set_inline_completion(ent_comp, TRUE);
	gtk_entry_completion_set_model(GTK_ENTRY_COMPLETION(ent_comp), GTK_TREE_MODEL(pl3_find2_autocomplete));
	gtk_entry_completion_set_popup_completion(GTK_ENTRY_COMPLETION(ent_comp), TRUE);
	gtk_entry_set_completion(GTK_ENTRY(cs->entry), ent_comp);


    g_signal_connect(G_OBJECT(cs->entry), "activate",G_CALLBACK(pl3_find2_browser_search), NULL);
    gtk_box_pack_start(GTK_BOX(cs->hbox), cs->entry, TRUE, TRUE, 0);

    cs->rmbut = removebut = gtk_button_new();
    gtk_button_set_image(GTK_BUTTON(removebut), gtk_image_new_from_stock(GTK_STOCK_REMOVE, GTK_ICON_SIZE_BUTTON));
    gtk_box_pack_end(GTK_BOX(cs->hbox), removebut, FALSE, TRUE, 0);
    g_signal_connect(G_OBJECT(removebut), "clicked", G_CALLBACK(pl3_find2_browser_remove_crit),cs);

    gtk_box_pack_start(GTK_BOX(pl3_find2_crit_vbox), cs->hbox, FALSE, TRUE,0);
    gtk_widget_show_all(pl3_find2_vbox);
    if(!criterias){
        gtk_widget_hide(cs->rmbut);
    }
    else if(g_list_length(criterias) == 1) {
        crit_struct *cs1 = criterias->data;
        gtk_widget_show(cs1->rmbut); 
    }

    criterias = g_list_append(criterias, cs);


    /** This causes some weird crashes when adding a default crit */
   /* gtk_widget_grab_focus(cs->combo);*/
    gtk_widget_set_sensitive(pl3_find2_findbut, TRUE);

   
    gtk_widget_set_sensitive(pl3_find2_critaddbut, TRUE);
    if(g_list_length(criterias) >= (MPD_TAG_NUM_OF_ITEM_TYPES-1))
    {
        gtk_widget_set_sensitive(pl3_find2_critaddbut, FALSE);
    }
}

static void playtime_changed(GmpcMpdDataModel *model, gulong playtime)
{
    if(pl3_cat_get_selected_browser() == find2_browser_plug.id)
    {
        playlist3_show_playtime(playtime);
    }
}

/**
 * Construct the browser 
 */
static void pl3_find2_browser_init()
{
	GtkWidget *frame, *event, *vbox, *label;
    GtkWidget  *pl3_find2_sw = NULL;
    GtkWidget *hbox = NULL;
	/* autocomplete later on */
	pl3_find2_autocomplete = gtk_list_store_new(1, G_TYPE_STRING);

	pl3_find2_store2 = gmpc_mpddata_model_new();
    gmpc_mpddata_model_disable_image(GMPC_MPDDATA_MODEL(pl3_find2_store2));
    g_signal_connect(G_OBJECT(pl3_find2_store2), "playtime_changed", G_CALLBACK(playtime_changed), NULL);


    pl3_find2_combo_store = gtk_list_store_new(2,G_TYPE_INT, G_TYPE_STRING);
    /** Fill the view */
    pl3_find2_fill_combo();

    /* Column */
     /* set up the tree */
    pl3_find2_tree= gmpc_mpddata_treeview_new("find2-browser",TRUE,GTK_TREE_MODEL(pl3_find2_store2));
    gmpc_mpddata_treeview_enable_click_fix(GMPC_MPDDATA_TREEVIEW(pl3_find2_tree));

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
    gtk_box_pack_end(GTK_BOX(pl3_find2_vbox), pl3_find2_sw, TRUE, TRUE,0);

    /* pom */
    frame = gtk_frame_new(NULL);
    event = gtk_event_box_new();
    vbox = gtk_vbox_new(FALSE,6);
    label = gtk_label_new("");
    gtk_misc_set_alignment(GTK_MISC(label), 0,0.5);

    /* Header box */
    hbox = gtk_hbox_new(FALSE,0);

    /* browser label */
    gtk_label_set_markup(GTK_LABEL(label), _("<span size='xx-large' weight='bold'>Search</span>"));
    gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, TRUE,0);

    /* search in playlist */
    pl3_find2_curpl = gtk_check_button_new_with_mnemonic(_("Search in current _play queue"));
    g_signal_connect(G_OBJECT(pl3_find2_curpl), "toggled", G_CALLBACK(pl3_find2_browser_search),NULL);
    gtk_box_pack_start(GTK_BOX(hbox), pl3_find2_curpl, FALSE, TRUE,6);

    /* find button */
    pl3_find2_findbut = gtk_button_new_from_stock(GTK_STOCK_FIND);
    gtk_widget_set_sensitive(pl3_find2_findbut, FALSE);
    g_signal_connect(G_OBJECT(pl3_find2_findbut), "clicked", G_CALLBACK(pl3_find2_browser_search),NULL);
    gtk_box_pack_start(GTK_BOX(hbox), pl3_find2_findbut, FALSE, TRUE,0);       

    /* Add criteria button */
    pl3_find2_critaddbut = gtk_button_new();
    gtk_button_set_image(GTK_BUTTON(pl3_find2_critaddbut), gtk_image_new_from_stock(GTK_STOCK_ADD, GTK_ICON_SIZE_BUTTON));
    g_signal_connect(G_OBJECT(pl3_find2_critaddbut), "clicked", G_CALLBACK(pl3_find2_browser_add_crit), NULL);
    gtk_box_pack_start(GTK_BOX(hbox), pl3_find2_critaddbut, FALSE, TRUE,0);

    /* Add it to the vbox */
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE,0);

    gtk_container_set_border_width(GTK_CONTAINER(vbox), 12);
    g_signal_connect(G_OBJECT(vbox), "style-set", G_CALLBACK(pl3_find2_browser_bg_style_changed), event);

    pl3_find2_crit_vbox = gtk_vbox_new(FALSE,6);
    gtk_box_pack_start(GTK_BOX(vbox), pl3_find2_crit_vbox, FALSE, TRUE,0);

    /* add it */
    gtk_container_add(GTK_CONTAINER(event), vbox);
	gtk_container_add(GTK_CONTAINER(frame), event);
    gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_IN);
    gtk_box_pack_start(GTK_BOX(pl3_find2_vbox), frame, FALSE, TRUE,0);

    gtk_widget_show_all(pl3_find2_vbox);
    g_object_ref(G_OBJECT(pl3_find2_vbox));

    /** Add a default item */
    pl3_find2_browser_add_crit();
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
            PL3_CAT_INT_ID, "",
            PL3_CAT_ICON_ID, "gtk-find",
            PL3_CAT_PROC, TRUE,
            PL3_CAT_ICON_SIZE,GTK_ICON_SIZE_DND,-1);

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










static gint __position_sort(gpointer aa, gpointer bb, gpointer data)
{
    MpdData_real *a = *(MpdData_real **)aa;
    MpdData_real *b = *(MpdData_real **)bb;
    return a->song->pos - b->song->pos;         
}
static unsigned long pl3_find2_browser_view_playlist()
{
	if(mpd_server_check_command_allowed(connection, "playlistsearch")== MPD_SERVER_COMMAND_ALLOWED && 
			mpd_server_check_command_allowed(connection, "playlistfind")== MPD_SERVER_COMMAND_ALLOWED)
    {
        GList *node = NULL;
        int found = 0;
        MpdData *data = NULL, *data_t= NULL;
        gtk_tree_view_set_model(GTK_TREE_VIEW(pl3_find2_tree), NULL);
        if(criterias == NULL)
            return 0;
        /** check rules, see if there is a usefull one, if so compile a regex */
        node= g_list_first(criterias);
        for(; node; node = g_list_next(node))
        {
            GtkTreeIter cc_iter;
            crit_struct *cs = node->data;
            const gchar *name = gtk_entry_get_text(GTK_ENTRY(cs->entry));
            cs->tag_type = -1;
            if(gtk_combo_box_get_active_iter(GTK_COMBO_BOX(cs->combo), &cc_iter) && name && name[0] != '\0')
            {
                int num_field;
                gchar **splitted = NULL;
                int i =0;
                splitted = tokenize_string(name);// g_strsplit(name, " ",0);
                for(i=0;splitted && splitted[i];i++)
                {                                                          
                    if(!found)
                    {
                        mpd_playlist_search_start(connection, FALSE);
                        found = TRUE;
                    }
                    gtk_tree_model_get(GTK_TREE_MODEL(pl3_find2_combo_store),&cc_iter , 0, &num_field, -1);

                    mpd_playlist_search_add_constraint(connection, num_field, splitted[i]);
                }

                //mpd_playlist_search_add_constraint(connection, num_field, name);
                if(splitted)
                    g_strfreev(splitted);
                {
                    GtkTreeIter iter;
                    gboolean found2 = FALSE;
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

            }
            if(node->next)
            {
                crit_struct *cs2 = node->next->data;
                if(gtk_combo_box_get_active(GTK_COMBO_BOX(cs2->lcombo))==1)
                {
                    data = mpd_database_search_commit(connection);
                    data_t = mpd_data_concatenate(data_t, data);
                    data = NULL;
                    found = FALSE;
                }

            }
        }
        if(found)
            data = mpd_playlist_search_commit(connection);
        data_t = mpd_data_concatenate(data_t, data);
        data_t = misc_mpddata_remove_duplicate_songs(data_t);
        data_t = misc_sort_mpddata(data_t,(GCompareDataFunc)__position_sort,NULL); 
        gmpc_mpddata_model_set_mpd_data(pl3_find2_store2, data_t);
        
        gtk_tree_view_set_model(GTK_TREE_VIEW(pl3_find2_tree), GTK_TREE_MODEL(pl3_find2_store2));
        if(pl3_find2_ref) {
            GtkTreeIter iter;
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
    return 0;
}


static unsigned long pl3_find2_browser_view_database()
{
    GList *node = NULL;
    int found = 0;

    MpdData *data_t = NULL, *data =NULL;

    gtk_tree_view_set_model(GTK_TREE_VIEW(pl3_find2_tree), NULL);
    if(criterias == NULL)
    {
        return 0;
    }   
    node= g_list_first(criterias);
    for(; node; node = g_list_next(node))
    {
        GtkTreeIter cc_iter;
        int num_field;
        crit_struct *cs = node->data;
        const gchar *name = gtk_entry_get_text(GTK_ENTRY(cs->entry));
        if(gtk_combo_box_get_active_iter(GTK_COMBO_BOX(cs->combo), &cc_iter) && name && name[0] != '\0')
        {
            gchar **splitted = NULL;
            int i =0;

            gtk_tree_model_get(GTK_TREE_MODEL(pl3_find2_combo_store),&cc_iter , 0, &num_field, -1);

            splitted = tokenize_string(name);//g_strsplit(name, " ",0);
            for(i=0;splitted && splitted[i];i++)
            {
                if(!found)
                {
                    mpd_database_search_start(connection, FALSE);
                    found = TRUE;
                }
                mpd_database_search_add_constraint(connection, num_field, splitted[i]);
            }
            if(splitted)
                g_strfreev(splitted);
            /* hack to correctly update the autocompletion. damn I must write something that does this more efficient */
            {
                GtkTreeIter iter;
                gboolean found2 = FALSE;
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
        }
        if(node->next)
        {
            crit_struct *cs2 = node->next->data;
            if(gtk_combo_box_get_active(GTK_COMBO_BOX(cs2->lcombo))==1)
            {
                data = mpd_database_search_commit(connection);
                data_t = mpd_data_concatenate(data_t, data);
                data = NULL;
                found = FALSE;
            }
        }
    }
    if(found)
        data = mpd_database_search_commit(connection);
    data_t = mpd_data_concatenate(data_t, data);
    data_t = misc_mpddata_remove_duplicate_songs(data_t);
    gmpc_mpddata_model_set_mpd_data(pl3_find2_store2, data_t);
	gtk_tree_view_set_model(GTK_TREE_VIEW(pl3_find2_tree), GTK_TREE_MODEL(pl3_find2_store2));
    if(pl3_find2_ref) {
        GtkTreeIter iter;
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
    return 0;
}

/**
 * Search 
 */
static void pl3_find2_browser_search()
{
    if(pl3_find2_vbox == NULL)
        return;
    if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(pl3_find2_curpl)))
    {
        pl3_find2_browser_view_playlist();
    }else{
        pl3_find2_browser_view_database();
    }
}


static void pl3_find2_browser_show_info()
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
            char *path;
            gtk_tree_model_get_iter (model, &iter, (GtkTreePath *) list->data);
            gtk_tree_model_get(model,&iter,MPDDATA_MODEL_COL_PATH, &path,-1);
            song = mpd_database_get_fileinfo(connection, path);
			if(song) {
				info2_activate();
				info2_fill_song_view(song);	
				mpd_freeSong(song);
			}
            q_free(path);
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

static void pl3_find2_browser_replace_selected()
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
    if(event->state == GDK_CONTROL_MASK && event->keyval == GDK_Insert)
    {
        pl3_find2_browser_replace_selected();		
    }
    else if(event->keyval == GDK_Insert)
    {
        pl3_find2_browser_add_selected();		
    }
    else if(event->keyval == GDK_i && event->state&GDK_MOD1_MASK)
    {
        pl3_find2_browser_show_info();
    }
    else
    {
        return pl3_window_key_press_event(tree,event);
    }
    return TRUE;
}



static void pl3_find2_browser_add_selected()
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
                /* add them to the add list */
                mpd_playlist_queue_add(connection, name);
            }
            songs++;
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
static void pl3_find2_browser_edit_columns(void)
{
  gmpc_mpddata_treeview_edit_columns(GMPC_MPDDATA_TREEVIEW(pl3_find2_tree));

}
static void pl3_find2_playlist_editor_add_to_playlist(GtkWidget *menu)
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

			if(gtk_tree_selection_count_selected_rows(gtk_tree_view_get_selection(GTK_TREE_VIEW(pl3_find2_tree))) == 1)	
			{
				item = gtk_image_menu_item_new_from_stock(GTK_STOCK_MEDIA_PLAY,NULL);
				gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
				g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(pl3_find2_activate_row), pl3_find2_tree);
			}


      item = gtk_image_menu_item_new_from_stock(GTK_STOCK_ADD,NULL);
      gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
      g_signal_connect(G_OBJECT(item), "activate",
          G_CALLBACK(pl3_find2_browser_add_selected), NULL);
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
      item = gtk_image_menu_item_new_with_label(_("Edit Columns"));
      gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
          gtk_image_new_from_stock(GTK_STOCK_EDIT, GTK_ICON_SIZE_MENU));
      gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
      g_signal_connect(G_OBJECT(item), "activate",
          G_CALLBACK(pl3_find2_browser_edit_columns), NULL);
      gtk_widget_show(item);
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


      playlist_editor_right_mouse(menu,pl3_find2_playlist_editor_add_to_playlist);
      gmpc_mpddata_treeview_right_mouse_intergration(GMPC_MPDDATA_TREEVIEW(pl3_find2_tree), GTK_MENU(menu));

      gtk_widget_show_all(menu);
      gtk_menu_popup(GTK_MENU(menu), NULL, NULL,NULL, NULL, 0, event->time);
      return TRUE;
    }
    return FALSE;
}

static void pl3_find2_browser_disconnect()
{
}


static void pl3_find2_browser_activate()
{
    GtkTreeSelection *selec = gtk_tree_view_get_selection((GtkTreeView *)
            glade_xml_get_widget (pl3_xml, "cat_tree"));

    GtkTreePath *path = gtk_tree_row_reference_get_path(pl3_find2_ref);
    if(path)
    {
        gtk_tree_selection_select_path(selec, path);
        gtk_tree_path_free(path);
    }
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
    if(!connect)
    {
        pl3_find2_browser_disconnect();
    }
}

/**
 * Handle program-global key events
 */
static int pl3_find2_browser_key_press_event(GtkWidget *mw, GdkEventKey *event, int type)
{
    if(event->state&GDK_CONTROL_MASK && event->keyval == GDK_j)
    {
		crit_struct *cs;
		pl3_find2_browser_activate();
        
        while(criterias && g_list_length(criterias) > 1)
        {
            cs = criterias->data;
            criterias = g_list_remove(criterias, cs);
            gtk_widget_destroy(cs->hbox);
            q_free(cs);
        }
        if(!criterias)
        {
            pl3_find2_browser_add_crit();
        } 
        cs = criterias->data;
        gtk_combo_box_set_active(GTK_COMBO_BOX(cs->combo), MPD_TAG_ITEM_ANY); 
        gtk_widget_grab_focus(cs->entry);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pl3_find2_curpl), TRUE);
        return TRUE;
    }

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
}	
static void pl3_find2_save_myself(void)
{
	if(pl3_find2_ref)
	{
		GtkTreePath *path = gtk_tree_row_reference_get_path(pl3_find2_ref);
		if(path)
		{
			gint *indices = gtk_tree_path_get_indices(path);
			debug_printf(DEBUG_INFO,"Saving myself to position: %i\n", indices[0]);
			cfg_set_single_value_as_int(config, "find2-browser","position",indices[0]);
			gtk_tree_path_free(path);
		}
	}
}
/**
 * Plugin structure
 */
gmpcPlBrowserPlugin find2_browser_gbp = {
	.add = pl3_find2_browser_add,
	.selected = pl3_find2_browser_selected,
	.unselected = pl3_find2_browser_unselected,
	.add_go_menu = pl3_find2_browser_add_go_menu,
	.key_press_event = pl3_find2_browser_key_press_event
};

gmpcPlugin find2_browser_plug = {
	.name = 						"Search Browser",
	.version = 						{1,1,1},
	.plugin_type = 					GMPC_PLUGIN_PL_BROWSER,
    .destroy = 						pl3_find2_browser_destroy,
	.browser = 						&find2_browser_gbp,
	.mpd_status_changed = 			pl3_find2_browser_status_changed,
	.mpd_connection_changed = 		pl3_find2_browser_connection_changed,
	.save_yourself = 				pl3_find2_save_myself,
};
