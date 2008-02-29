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
#include <regex.h>
#include "plugin.h"

#include "main.h"
#include "misc.h"
#include "playlist3.h"
#include "playlist3-find3-browser.h"
#include "config1.h"

#include "eggcolumnchooserdialog.h"


#include "gmpc-mpddata-model.h"
#include "gmpc-mpddata-treeview.h"

static void pl3_find3_browser_edit_columns(void);
static void pl3_find3_browser_destroy(void);
static void pl3_find3_browser_delete_selected_songs (void);
static void pl3_find3_browser_selected(GtkWidget *);
static void pl3_find3_browser_unselected(GtkWidget *);
static void pl3_find3_browser_add(GtkWidget *cat_tree);
static int pl3_find3_browser_add_go_menu(GtkWidget *);
static void pl3_find3_browser_search(void);
static void pl3_find3_browser_row_activated(GtkTreeView *, GtkTreePath *);
static int pl3_find3_browser_playlist_key_press(GtkWidget *, GdkEventKey *);
static gboolean pl3_find3_browser_button_press_event(GtkWidget *but, GdkEventButton *event,gpointer data);
static gboolean pl3_find3_browser_button_release_event(GtkWidget *but, GdkEventButton *event);
static void pl3_find3_browser_connection_changed(MpdObj *mi, int connect, gpointer data);
static int pl3_find3_browser_key_press_event(GtkWidget *mw, GdkEventKey *event, int type);
static void pl3_find3_browser_status_changed(MpdObj *mi,ChangedStatusType what, void *data);
extern GladeXML *pl3_xml;

static void pl3_find3_combo_box_changed(GtkComboBox *cb, gpointer data);

static void pl3_find3_save_myself(void);

int pl3_find3_last_entry = MPD_TAG_ITEM_ANY;
extern GtkWidget *pl3_cp_tree;
/**
 * Plugin structure
 */
gmpcPlBrowserPlugin find3_browser_gbp = {
    .add                = pl3_find3_browser_add,
    .selected           = pl3_find3_browser_selected,
    .unselected         = pl3_find3_browser_unselected,
    .add_go_menu        = pl3_find3_browser_add_go_menu,
    .key_press_event    = pl3_find3_browser_key_press_event
};

gmpcPlugin find3_browser_plug = {
   	.name = 						 "Playlist Browser",
	.version = 					    {1,1,1},
	.plugin_type = 				    GMPC_PLUGIN_PL_BROWSER,
    .destroy = 						pl3_find3_browser_destroy,  
    .browser = 						&find3_browser_gbp,	
    .mpd_status_changed = 			pl3_find3_browser_status_changed,
    .mpd_connection_changed = 		pl3_find3_browser_connection_changed,
	.save_yourself = 				pl3_find3_save_myself,
};


typedef struct {
    GtkWidget *hbox;
    GtkWidget *combo;
    GtkWidget *lcombo;
    GtkWidget *entry;
    regex_t preq;
    int tag_type; 
}crit3_struct;



/* Playlist window row reference */
static GtkTreeRowReference *pl3_find3_ref = NULL;
static GtkListStore 		*pl3_find3_autocomplete = NULL;

extern GladeXML *pl3_xml;

/* internal */
static GtkWidget 	*pl3_find3_tree 	= NULL;
static GmpcMpdDataModel *pl3_find3_store2 = NULL;

static GtkWidget 	*pl3_find3_vbox 	= NULL;
static GtkWidget	*pl3_find3_findbut     = NULL;
static GtkWidget       *pl3_find3_critaddbut   = NULL;
static GtkListStore	*pl3_find3_combo_store 	= NULL;
static GtkWidget	*pl3_find3_pb = NULL;

static GList *criterias3 = NULL;
static GtkWidget *pl3_find3_crit_vbox = NULL;

static void pl3_find3_fill_combo()
{
    GtkTreeIter iter;
    int i=0, max = 3;
    gtk_list_store_clear(pl3_find3_combo_store);

    max = MPD_TAG_NUM_OF_ITEM_TYPES;
    for(i=0;i< max;i++)
    {
        gtk_list_store_append(pl3_find3_combo_store, &iter);
        gtk_list_store_set(pl3_find3_combo_store, &iter, 1, mpdTagItemKeys[i], 0,i, -1);	
    }
}

static void pl3_find3_browser_bg_style_changed(GtkWidget *vbox, GtkStyle *style,  GtkWidget *vp)
{
    gtk_widget_modify_bg(vp,GTK_STATE_NORMAL, &(GTK_WIDGET(vbox)->style->light[GTK_STATE_SELECTED]));
}

static void pl3_find3_browser_remove_crit(GtkWidget *button,crit3_struct *cs)
{
    criterias3 = g_list_remove(criterias3, cs);
    gtk_widget_destroy(cs->hbox);
    q_free(cs);
    if(!criterias3)
    {
        gtk_widget_set_sensitive(pl3_find3_findbut, FALSE);
    }
    gtk_widget_set_sensitive(pl3_find3_critaddbut, TRUE);
}

static void pl3_find3_combo_box_changed(GtkComboBox *cb, gpointer data)
{
  pl3_find3_last_entry= gtk_combo_box_get_active(cb);
}

static void pl3_find3_browser_add_crit()
{
    crit3_struct *cs = g_malloc0(sizeof(*cs));
    GtkWidget *removebut = NULL;
    GtkCellRenderer *renderer = NULL;
	GtkEntryCompletion *ent_comp;
    cs->tag_type = -1;
    cs->hbox = gtk_hbox_new(FALSE, 6);
    cs->lcombo = gtk_combo_box_new_text();
    gtk_combo_box_append_text(GTK_COMBO_BOX(cs->lcombo), _("And"));
    gtk_combo_box_append_text(GTK_COMBO_BOX(cs->lcombo), _("Or"));
    gtk_combo_box_set_active(GTK_COMBO_BOX(cs->lcombo), 0);
	if(mpd_server_check_command_allowed(connection, "playlistsearch")== MPD_SERVER_COMMAND_ALLOWED && 
			mpd_server_check_command_allowed(connection, "playlistfind")== MPD_SERVER_COMMAND_ALLOWED)
    {
        gtk_box_pack_start(GTK_BOX(cs->hbox), cs->lcombo, FALSE, TRUE, 0);
    }

    cs->combo= gtk_combo_box_new();
    renderer = gtk_cell_renderer_text_new();
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(cs->combo), renderer, TRUE);
    gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(cs->combo), renderer, "text", 1, NULL);
    gtk_combo_box_set_model(GTK_COMBO_BOX(cs->combo), GTK_TREE_MODEL(pl3_find3_combo_store));
    gtk_combo_box_set_active(GTK_COMBO_BOX(cs->combo), pl3_find3_last_entry);
    gtk_box_pack_start(GTK_BOX(cs->hbox), cs->combo, FALSE, TRUE, 0);
    g_signal_connect(G_OBJECT(cs->combo), "changed", G_CALLBACK(pl3_find3_combo_box_changed), NULL);

    cs->entry = gtk_entry_new();
	ent_comp = gtk_entry_completion_new();
	gtk_entry_completion_set_text_column(ent_comp, 0);
	gtk_entry_completion_set_inline_completion(ent_comp, TRUE);
	gtk_entry_completion_set_model(GTK_ENTRY_COMPLETION(ent_comp), GTK_TREE_MODEL(pl3_find3_autocomplete));
	gtk_entry_completion_set_popup_completion(GTK_ENTRY_COMPLETION(ent_comp), TRUE);
	gtk_entry_set_completion(GTK_ENTRY(cs->entry), ent_comp);







    g_signal_connect(G_OBJECT(cs->entry), "activate",G_CALLBACK(pl3_find3_browser_search), NULL);
    gtk_box_pack_start(GTK_BOX(cs->hbox), cs->entry, TRUE, TRUE, 0);

    removebut = gtk_button_new();
    gtk_button_set_image(GTK_BUTTON(removebut), gtk_image_new_from_stock(GTK_STOCK_REMOVE, GTK_ICON_SIZE_BUTTON));
    gtk_box_pack_end(GTK_BOX(cs->hbox), removebut, FALSE, TRUE, 0);
    g_signal_connect(G_OBJECT(removebut), "clicked", G_CALLBACK(pl3_find3_browser_remove_crit),cs);

    gtk_box_pack_start(GTK_BOX(pl3_find3_crit_vbox), cs->hbox, FALSE, TRUE,0);

    criterias3 = g_list_append(criterias3, cs);

    gtk_widget_show_all(pl3_find3_crit_vbox);
    /** This causes some weird crashes when adding a default crit */
    /* gtk_widget_grab_focus(cs->combo);*/
    gtk_widget_set_sensitive(pl3_find3_findbut, TRUE);


    gtk_widget_set_sensitive(pl3_find3_critaddbut, TRUE);
    if(g_list_length(criterias3) >= (MPD_TAG_NUM_OF_ITEM_TYPES-1))
    {
        gtk_widget_set_sensitive(pl3_find3_critaddbut, FALSE);
    }
}

static void pl3_find3_browser_init()
{
    GtkWidget  *pl3_find3_sw = NULL;
    GtkWidget *hbox = NULL;

	pl3_find3_autocomplete = gtk_list_store_new(1, G_TYPE_STRING);

	pl3_find3_store2 = gmpc_mpddata_model_new();
    gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(pl3_find3_store2), GTK_TREE_SORTABLE_UNSORTED_SORT_COLUMN_ID, GTK_SORT_ASCENDING);

    pl3_find3_combo_store = gtk_list_store_new(2,G_TYPE_INT, G_TYPE_STRING);
    /** Fill the view */
    pl3_find3_fill_combo();

    /* set up the tree */
    pl3_find3_tree= gmpc_mpddata_treeview_new("find3-browser",FALSE,GTK_TREE_MODEL(pl3_find3_store2));

    /* setup signals */
    g_signal_connect(G_OBJECT(pl3_find3_tree), "row-activated",G_CALLBACK(pl3_find3_browser_row_activated), NULL); 
    g_signal_connect(G_OBJECT(pl3_find3_tree), "button-press-event", G_CALLBACK(pl3_find3_browser_button_press_event), NULL);
    g_signal_connect(G_OBJECT(pl3_find3_tree), "button-release-event", G_CALLBACK(pl3_find3_browser_button_release_event), NULL);
    g_signal_connect(G_OBJECT(pl3_find3_tree), "key-press-event", G_CALLBACK(pl3_find3_browser_playlist_key_press), NULL);

    /* set up the scrolled window */
    pl3_find3_sw = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(pl3_find3_sw), GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(pl3_find3_sw), GTK_SHADOW_ETCHED_IN);
    gtk_container_add(GTK_CONTAINER(pl3_find3_sw), pl3_find3_tree);

    /* set initial state */

    pl3_find3_vbox = gtk_vbox_new(FALSE, 6);

    /* pom */
	GtkWidget *frame = gtk_frame_new(NULL);
    GtkWidget *event = gtk_event_box_new();
    GtkWidget *vbox = gtk_vbox_new(FALSE,6);
    GtkWidget *label = gtk_label_new("");
    gtk_misc_set_alignment(GTK_MISC(label), 0,0.5);
    /* Add button */
    pl3_find3_critaddbut = gtk_button_new();
    gtk_button_set_image(GTK_BUTTON(pl3_find3_critaddbut), gtk_image_new_from_stock(GTK_STOCK_ADD, GTK_ICON_SIZE_BUTTON));
    g_signal_connect(G_OBJECT(pl3_find3_critaddbut), "clicked", G_CALLBACK(pl3_find3_browser_add_crit), NULL);


    /* Find button */
    pl3_find3_findbut = gtk_button_new_from_stock(GTK_STOCK_FIND);
    gtk_widget_set_sensitive(pl3_find3_findbut, FALSE);
    g_signal_connect(G_OBJECT(pl3_find3_findbut), "clicked", G_CALLBACK(pl3_find3_browser_search),NULL);
    gtk_label_set_markup(GTK_LABEL(label), _("<span size='xx-large' weight='bold'>Playlist Search</span>"));
    hbox = gtk_hbox_new(FALSE,0);
    gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, TRUE,0);
    gtk_box_pack_start(GTK_BOX(hbox), pl3_find3_findbut, FALSE, TRUE,0);       
    gtk_box_pack_start(GTK_BOX(hbox), pl3_find3_critaddbut, FALSE, TRUE,0);       
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE,0);
    /* add it */

    gtk_container_set_border_width(GTK_CONTAINER(vbox), 12);
    g_signal_connect(G_OBJECT(vbox), "style-set", G_CALLBACK(pl3_find3_browser_bg_style_changed), event);

    /* vbox for criterias3 */
    pl3_find3_crit_vbox = gtk_vbox_new(FALSE,6);
    gtk_box_pack_start(GTK_BOX(vbox), pl3_find3_crit_vbox, FALSE, TRUE,0);

    /* add it */
    gtk_container_add(GTK_CONTAINER(event), vbox);
	gtk_container_add(GTK_CONTAINER(frame), event);
    gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_IN);
    gtk_box_pack_start(GTK_BOX(pl3_find3_vbox), frame, FALSE, TRUE,0);

    /** Add tree */
    gtk_box_pack_start(GTK_BOX(pl3_find3_vbox), pl3_find3_sw, TRUE, TRUE,0);
    gtk_widget_show_all(pl3_find3_vbox);


    /* progressbar */
    pl3_find3_pb = gtk_progress_bar_new();
    gtk_box_pack_end(GTK_BOX(pl3_find3_vbox), pl3_find3_pb, FALSE, TRUE,0);
    gtk_widget_hide(pl3_find3_pb);


    g_object_ref(G_OBJECT(pl3_find3_vbox));
    /** Add a default item */
    pl3_find3_browser_add_crit();
}

static void pl3_find3_browser_selected(GtkWidget *container)
{
    if(pl3_find3_tree == NULL)
    {
        pl3_find3_browser_init();
    }

    gtk_container_add(GTK_CONTAINER(container),pl3_find3_vbox);
    gtk_widget_grab_focus(pl3_find3_tree);
    gtk_widget_show(pl3_find3_vbox);
}
static void pl3_find3_browser_unselected(GtkWidget *container)
{
	if(pl3_find3_vbox)
	{
	    gtk_container_remove(GTK_CONTAINER(container), pl3_find3_vbox);
	}
}

/*****************************************************************
 * Find Browser
 */
/* add's the toplevel entry for the current playlist view */
static void pl3_find3_browser_add(GtkWidget *cat_tree)
{
    GtkTreePath *path = NULL;
    GtkTreeIter iter;
	gint pos = cfg_get_single_value_as_int_with_default(config, "find3-browser","position",1);
	playlist3_insert_browser(&iter, pos);
    gtk_list_store_set(GTK_LIST_STORE(pl3_tree), &iter, 
            PL3_CAT_TYPE, find3_browser_plug.id,
            PL3_CAT_TITLE, _("Playlist Search"),
            PL3_CAT_INT_ID, "",
            PL3_CAT_ICON_ID, "playlist-search-browser",
            PL3_CAT_PROC, TRUE,
            PL3_CAT_ICON_SIZE,GTK_ICON_SIZE_DND,-1);

    if (pl3_find3_ref) {
        gtk_tree_row_reference_free(pl3_find3_ref);
        pl3_find3_ref = NULL;
    }

    path = gtk_tree_model_get_path(GTK_TREE_MODEL(playlist3_get_category_tree_store()), &iter);
    if (path) {
        pl3_find3_ref = gtk_tree_row_reference_new(GTK_TREE_MODEL(playlist3_get_category_tree_store()), path);
        gtk_tree_path_free(path);
    }
}

static unsigned long pl3_find3_browser_view_browser_old_style()
{
	MpdData *data = NULL;
    int time=0;
    GList *node2,*node = NULL;
    int found = 0;

    if(criterias3 == NULL)
        return 0;
    /** check rules, see if there is a usefull one, if so compile a regex */
    gtk_tree_view_set_model(GTK_TREE_VIEW(pl3_find3_tree), NULL);
    node= g_list_first(criterias3);
    while(node)
    {
        found = FALSE;
        for(; node; node = g_list_next(node))
        {
            GtkTreeIter cc_iter;
            crit3_struct *cs = node->data;
            const gchar *name = gtk_entry_get_text(GTK_ENTRY(cs->entry));
            cs->tag_type = -1;
            if(gtk_combo_box_get_active_iter(GTK_COMBO_BOX(cs->combo), &cc_iter) && name && name[0] != '\0')
            {
                if(!regcomp(&(cs->preq),name, REG_EXTENDED|REG_ICASE))
                {
                    int num_field;
                    found = TRUE;
                    gtk_tree_model_get(GTK_TREE_MODEL(pl3_find3_combo_store),&cc_iter , 0, &num_field, -1);
                    cs->tag_type = num_field;
                } else {
                    regfree(&(cs->preq));
                    cs->tag_type = -1;
                }
            }
            if(node->next)
            {
                crit3_struct *cs2 = node->next->data;
                if(gtk_combo_box_get_active(GTK_COMBO_BOX(cs2->lcombo)) == 1)
                {
                    node = node->next;
                    break;
                }
            }
        }

        /** Fill now */
        if(found){
            GtkTreeIter iter;
            GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(pl3_cp_tree));
            if(gtk_tree_model_get_iter_first(GTK_TREE_MODEL(model), &iter))
            {
                GList *node3 = NULL;
                int songs = 0, total_songs = mpd_playlist_get_playlist_length(connection);
                int step = total_songs/50;
                if( TRUE )
                {
                    gtk_widget_show(pl3_find3_pb);
                    gtk_widget_set_sensitive(glade_xml_get_widget(pl3_xml, "pl3_win"),FALSE);
                }
                else{
                    total_songs = 0;
                }
                do {
                    int loop = TRUE;
                    mpd_Song *song = NULL;
                    gtk_tree_model_get(GTK_TREE_MODEL(gtk_tree_view_get_model(GTK_TREE_VIEW(pl3_cp_tree))), &iter, MPDDATA_MODEL_COL_MPDSONG, &song, -1);


                    songs++;
                    if(step && total_songs && (songs % step) == 0)
                    {
                        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(pl3_find3_pb), songs/(gdouble)total_songs);
                        while(gtk_events_pending()) gtk_main_iteration();
                    }

                    if((node3 = g_list_first(criterias3)))
                    {
                        loop = FALSE;
                        for(;!loop && node3; node3 = g_list_next(node3))
                        {
                            loop = TRUE;

                            crit3_struct *cs = node3->data;
                            if(cs && cs->tag_type >= 0)
                            {
                                if(song->artist && (cs->tag_type == MPD_TAG_ITEM_ARTIST || cs->tag_type == MPD_TAG_ITEM_ANY)){
                                    if(regexec(&(cs->preq),song->artist,0,NULL,0) == 0)loop = FALSE; 
                                } 
                                if(song->album && (cs->tag_type == MPD_TAG_ITEM_ALBUM || cs->tag_type == MPD_TAG_ITEM_ANY)){
                                    if(regexec(&(cs->preq),song->album,0,NULL,0) == 0)loop = FALSE; 
                                }
                                if(song->title && (cs->tag_type == MPD_TAG_ITEM_TITLE || cs->tag_type == MPD_TAG_ITEM_ANY)){
                                    if(regexec(&(cs->preq),song->title,0,NULL,0) == 0)loop = FALSE; 
                                }
                                if(song->track && (cs->tag_type == MPD_TAG_ITEM_TRACK || cs->tag_type == MPD_TAG_ITEM_ANY)){
                                    if(regexec(&(cs->preq),song->track,0,NULL,0) == 0)loop = FALSE; 
                                }
                                if(song->name && (cs->tag_type == MPD_TAG_ITEM_NAME || cs->tag_type == MPD_TAG_ITEM_ANY)){
                                    if(regexec(&(cs->preq),song->name,0,NULL,0) == 0)loop = FALSE; 
                                }
                                if(song->genre&& (cs->tag_type == MPD_TAG_ITEM_GENRE|| cs->tag_type == MPD_TAG_ITEM_ANY)){
                                    if(regexec(&(cs->preq),song->genre,0,NULL,0) == 0)loop = FALSE; 
                                }
                                if(song->date&& (cs->tag_type == MPD_TAG_ITEM_DATE|| cs->tag_type == MPD_TAG_ITEM_ANY)){
                                    if(regexec(&(cs->preq),song->date,0,NULL,0) == 0)loop = FALSE; 
                                }
                                if(song->composer&& (cs->tag_type == MPD_TAG_ITEM_COMPOSER|| cs->tag_type == MPD_TAG_ITEM_ANY)){
                                    if(regexec(&(cs->preq),song->composer,0,NULL,0) == 0)loop = FALSE; 
                                }
                                if(song->performer&& (cs->tag_type == MPD_TAG_ITEM_PERFORMER||cs->tag_type == MPD_TAG_ITEM_ANY)){
                                    if(regexec(&(cs->preq),song->performer,0,NULL,0) == 0)loop = FALSE; 
                                }
                                if(song->comment&& (cs->tag_type == MPD_TAG_ITEM_COMMENT||cs->tag_type == MPD_TAG_ITEM_ANY)){
                                    if(regexec(&(cs->preq),song->comment,0,NULL,0) == 0)loop = FALSE; 
                                }                                                                        
                                if(song->disc&& (cs->tag_type == MPD_TAG_ITEM_DISC||cs->tag_type == MPD_TAG_ITEM_ANY)){
                                    if(regexec(&(cs->preq),song->disc,0,NULL,0) == 0)loop = FALSE; 
                                }
                                if(song->file&& (cs->tag_type == MPD_TAG_ITEM_FILENAME||cs->tag_type == MPD_TAG_ITEM_ANY)){
                                    if(regexec(&(cs->preq),song->file,0,NULL,0) == 0)loop = FALSE; 
                                }
                            }
                            else
                                loop = FALSE;
                        }
                    }
                    if(!loop)
                    {
                        mpd_Song *song = NULL;
                        gtk_tree_model_get(GTK_TREE_MODEL(gtk_tree_view_get_model(GTK_TREE_VIEW(pl3_cp_tree))), &iter, MPDDATA_MODEL_COL_MPDSONG, &song, -1);
                        data = mpd_new_data_struct_append(data);
                        data->type = MPD_DATA_TYPE_SONG;
                        data->song = mpd_songDup(song);  
                    }
                } while(gtk_tree_model_iter_next(gtk_tree_view_get_model(GTK_TREE_VIEW(pl3_cp_tree)), &iter));

                /* remove the progress bar */
                if(total_songs) {
                    gtk_widget_hide(pl3_find3_pb);   		   
                    gtk_widget_set_sensitive(glade_xml_get_widget(pl3_xml, "pl3_win"),TRUE);
                }
            }
        }
        /* cleanup the regex compiled */
        for(node2= g_list_first(criterias3); node2 ; node2 = g_list_next(node2)) {
            crit3_struct *cs = node2->data;
            if(cs->tag_type >= 0)
                regfree(&(cs->preq));
            cs->tag_type = -1;
        }
    }
    data = misc_mpddata_remove_duplicate_songs(data);
    gmpc_mpddata_model_set_mpd_data(pl3_find3_store2, data);
    /* set model again */
    gtk_tree_view_set_model(GTK_TREE_VIEW(pl3_find3_tree), GTK_TREE_MODEL(pl3_find3_store2));
    return time;
}

static gint __position_sort(gpointer aa, gpointer bb, gpointer data)
{
    MpdData_real *a = *(MpdData_real **)aa;
    MpdData_real *b = *(MpdData_real **)bb;
    return a->song->pos - b->song->pos;         
}




static unsigned long pl3_find3_browser_view_browser()
{
	if(mpd_server_check_command_allowed(connection, "playlistsearch")== MPD_SERVER_COMMAND_ALLOWED && 
			mpd_server_check_command_allowed(connection, "playlistfind")== MPD_SERVER_COMMAND_ALLOWED)
    {
        int time=0;
        GList *node = NULL;
        int found = 0;
        MpdData *data = NULL, *data_t= NULL;
        gtk_tree_view_set_model(GTK_TREE_VIEW(pl3_find3_tree), NULL);
        if(criterias3 == NULL)
            return 0;
        /** check rules, see if there is a usefull one, if so compile a regex */
        node= g_list_first(criterias3);
        for(; node; node = g_list_next(node))
        {
            GtkTreeIter cc_iter;
            crit3_struct *cs = node->data;
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
                    gtk_tree_model_get(GTK_TREE_MODEL(pl3_find3_combo_store),&cc_iter , 0, &num_field, -1);

                    mpd_playlist_search_add_constraint(connection, num_field, splitted[i]);
                }

                //mpd_playlist_search_add_constraint(connection, num_field, name);
                if(splitted)
                    g_strfreev(splitted);
                {
                    GtkTreeIter iter;
                    gboolean found2 = FALSE;
                    for(gtk_tree_model_get_iter_first(GTK_TREE_MODEL(pl3_find3_autocomplete), &iter);
                            gtk_list_store_iter_is_valid(pl3_find3_autocomplete, &iter) && !found2;
                            gtk_tree_model_iter_next(GTK_TREE_MODEL(pl3_find3_autocomplete), &iter))
                    {
                        gchar *entry;
                        gtk_tree_model_get(GTK_TREE_MODEL(pl3_find3_autocomplete), &iter, 0,&entry,-1);
                        if(strcmp(entry, name) == 0)
                        {
                            found2 = TRUE;
                        }
                        g_free(entry);
                    }
                    if(!found2) {
                        gtk_list_store_insert_with_values(pl3_find3_autocomplete, &iter,-1, 0,name,-1);
                    }					
                }

            }
            if(node->next)
            {
                crit3_struct *cs2 = node->next->data;
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
        gmpc_mpddata_model_set_mpd_data(pl3_find3_store2, data_t);

        gtk_tree_view_set_model(GTK_TREE_VIEW(pl3_find3_tree), GTK_TREE_MODEL(pl3_find3_store2));
        return time;
    }
	else
		return pl3_find3_browser_view_browser_old_style();
}


static void pl3_find3_browser_search()
{
	long unsigned time = 0;
	/* if it's not yet created, do nothing */
	if(pl3_find3_vbox == NULL)
		return;

	gchar *string;	
/*	gtk_list_store_clear(pl3_find3_store);*/
	time = pl3_find3_browser_view_browser();
	string = format_time(time);
	gtk_statusbar_push(GTK_STATUSBAR(glade_xml_get_widget(pl3_xml, "statusbar2")),0, string);
	q_free(string);
}


static void pl3_find3_browser_show_info()
{
	GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW(pl3_find3_tree));
	GtkTreeSelection *selection =gtk_tree_view_get_selection (GTK_TREE_VIEW(pl3_find3_tree));
	if(!mpd_server_check_version(connection,0,12,0))
	{
		return;
	}
	if (gtk_tree_selection_count_selected_rows (selection) > 0)
	{
		GList *list = NULL;
		list = gtk_tree_selection_get_selected_rows (selection, &model);

		list = g_list_last (list);
	//	do
		{
			GtkTreeIter iter;
			mpd_Song *song = NULL;
			GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(pl3_find3_tree));
			gtk_tree_model_get_iter (model, &iter, (GtkTreePath *) list->data);
			gtk_tree_model_get(model,&iter,MPDDATA_MODEL_COL_MPDSONG, &song,-1);
			info2_activate();
			info2_fill_song_view(song);	

		}
	//	while ((list = g_list_previous (list)) && mpd_check_connected(connection));

		g_list_foreach (list, (GFunc) gtk_tree_path_free, NULL);
		g_list_free (list);
	}
}

static void pl3_find3_browser_row_activated(GtkTreeView *tree, GtkTreePath *tp)
{
	GtkTreeIter iter;
	int id=-1;
	gtk_tree_model_get_iter(gtk_tree_view_get_model(tree), &iter, tp);
	gtk_tree_model_get(gtk_tree_view_get_model(tree), &iter,MPDDATA_MODEL_COL_SONG_ID,&id, -1);
	mpd_player_play_id(connection, id);
}

static int pl3_find3_browser_playlist_key_press(GtkWidget *tree, GdkEventKey *event)
{
	if(event->keyval == GDK_i && event->state&GDK_MOD1_MASK)
	{
		pl3_find3_browser_show_info();
	}
	else
	{
		return pl3_window_key_press_event(tree,event);
	}
	return TRUE;
}
static void pl3_find3_browser_edit_columns(void)
{
  gmpc_mpddata_treeview_edit_columns(GMPC_MPDDATA_TREEVIEW(pl3_find3_tree));
}
static gboolean pl3_find3_browser_button_press_event(GtkWidget *but, GdkEventButton *event, gpointer data)
{
	GtkTreePath *path = NULL;
	if(event->button == 3 &&gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(but), event->x, event->y,&path,NULL,NULL,NULL))
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
static void pl3_find3_playlist_editor_add_to_playlist(GtkWidget *menu)
{
    GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(pl3_find3_tree));
    GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(pl3_find3_tree));
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



static gboolean pl3_find3_browser_button_release_event(GtkWidget *but, GdkEventButton *event)
{
	if(event->button != 3) return FALSE;
	else if(gtk_tree_selection_count_selected_rows(gtk_tree_view_get_selection(GTK_TREE_VIEW(pl3_find3_tree))) > 0)
	{
		GtkWidget *item;
		GtkWidget *menu = gtk_menu_new();

        if(gtk_tree_selection_count_selected_rows(gtk_tree_view_get_selection(GTK_TREE_VIEW(pl3_find3_tree))) == 1)	
        {
            mpd_Song *song;
            GtkTreeIter iter;
            GtkTreePath *path;
            GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(pl3_find3_tree));
            GList *list = gtk_tree_selection_get_selected_rows(gtk_tree_view_get_selection(GTK_TREE_VIEW(pl3_find3_tree)), &model);
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
		g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(pl3_find3_browser_delete_selected_songs), NULL);
	
		gtk_menu_shell_append(GTK_MENU_SHELL(menu),gtk_separator_menu_item_new());

		if(mpd_server_check_version(connection,0,12,0))
		{
			item = gtk_image_menu_item_new_from_stock(GTK_STOCK_DIALOG_INFO,NULL);
			gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
			g_signal_connect(G_OBJECT(item), "activate",
					G_CALLBACK(pl3_find3_browser_show_info), NULL);
		}
    item = gtk_image_menu_item_new_with_label(_("Edit Columns"));
    gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
        gtk_image_new_from_stock(GTK_STOCK_EDIT, GTK_ICON_SIZE_MENU));
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
    g_signal_connect(G_OBJECT(item), "activate",
        G_CALLBACK(pl3_find3_browser_edit_columns), NULL);


    playlist_editor_right_mouse(menu,pl3_find3_playlist_editor_add_to_playlist);
    gtk_widget_show_all(menu);
    gtk_menu_popup(GTK_MENU(menu), NULL, NULL,NULL, NULL, 0, event->time);
    return TRUE;
  }
  return FALSE ;
}

static void pl3_find3_browser_disconnect()
{
/*   while(criterias3)pl3_find3_browser_remove_crit(NULL, criterias3);*/
  /*	if(pl3_find3_store) gtk_list_store_clear(pl3_find3_store);*/
}


static void pl3_find3_browser_activate()
{
  GtkTreeSelection *selec = gtk_tree_view_get_selection((GtkTreeView *)glade_xml_get_widget (pl3_xml, "cat_tree"));

  GtkTreePath *path = gtk_tree_row_reference_get_path(pl3_find3_ref);
  if(path)
  {
    gtk_tree_selection_select_path(selec, path);
    gtk_tree_path_free(path);
  }
}

/**
 * Trigger Playlist search
 * This switches to the search window set focus on entry and set searh on playlist.
 * TODO: Move to search plugin?
 */
/*
static void pl3_playlist_search()
{
  if(!mpd_check_connected(connection))
  {
    return;
  }

  if(pl3_xml)
  {
    GtkTreePath *path = gtk_tree_row_reference_get_path(pl3_find3_ref);
    GtkTreeSelection *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(glade_xml_get_widget(pl3_xml, "cat_tree")));
    gtk_window_present(GTK_WINDOW(glade_xml_get_widget(pl3_xml, "pl3_win")));
    gtk_tree_selection_select_path(sel, path);
    gtk_tree_view_set_cursor(GTK_TREE_VIEW(glade_xml_get_widget(pl3_xml, "cat_tree")), path, NULL, FALSE);
    gtk_tree_path_free(path);
  }
}
*/
static int pl3_find3_browser_add_go_menu(GtkWidget *menu)
{
  GtkWidget *item = NULL;

  item = gtk_image_menu_item_new_with_label(_("Playlist Search"));
  gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item), 
      gtk_image_new_from_icon_name("playlist-search-browser", GTK_ICON_SIZE_MENU));
  gtk_widget_add_accelerator(GTK_WIDGET(item), "activate", gtk_menu_get_accel_group(GTK_MENU(menu)), GDK_F2, 0, GTK_ACCEL_VISIBLE);
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
  g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(pl3_find3_browser_activate), NULL);

  return 1;
}

static void pl3_find3_browser_connection_changed(MpdObj *mi, int connect, gpointer data)
{
  if(!connect)
  {
    pl3_find3_browser_disconnect();
  }
}

static int pl3_find3_browser_key_press_event(GtkWidget *mw, GdkEventKey *event, int type)
{
/*
  if (event->keyval == GDK_F2)
  {
    pl3_find3_browser_activate();
    return TRUE;
  }                                           	
  else*/ if(event->state&GDK_CONTROL_MASK && event->keyval == GDK_j)
  {
      pl3_find3_browser_activate();
      //pl3_playlist_search();
    crit3_struct *cs;
    while(criterias3 && g_list_length(criterias3) > 1)
    {
      cs = criterias3->data;
      criterias3 = g_list_remove(criterias3, cs);
      gtk_widget_destroy(cs->hbox);
      q_free(cs);
    }
    if(!criterias3)
    {
      pl3_find3_browser_add_crit();
    } 
    cs = criterias3->data;
    gtk_combo_box_set_active(GTK_COMBO_BOX(cs->combo), MPD_TAG_ITEM_ANY); 
    gtk_widget_grab_focus(cs->entry);
    return TRUE;
  }
  return FALSE;
}


static void pl3_find3_browser_delete_selected_songs (void)
{
  /* grab the selection from the tree */
  GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW(pl3_find3_tree));
  /* check if where connected */
  /* see if there is a row selected */
  if (gtk_tree_selection_count_selected_rows (selection) > 0)
  {
    GList *list = NULL, *llist = NULL;
    GtkTreeModel *model = GTK_TREE_MODEL(pl3_find3_store2);
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
      gtk_tree_model_get (model, &iter,MPDDATA_MODEL_COL_SONG_ID, &value, -1);
      mpd_playlist_queue_delete_id(connection, value);			
    } while ((llist = g_list_next (llist)));

    /* close the list, so it will be executed */
    mpd_playlist_queue_commit(connection);
    pl3_find3_browser_view_browser();
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

static void pl3_find3_browser_destroy(void)
{
    if(pl3_find3_ref)
    {
        GtkTreeModel *model = gtk_tree_row_reference_get_model(pl3_find3_ref);
        GtkTreePath *path = gtk_tree_row_reference_get_path(pl3_find3_ref);
        if(path)
        {
            GtkTreeIter iter;
            if(gtk_tree_model_get_iter(model, &iter, path))
            {
                gtk_list_store_remove(GTK_LIST_STORE(model), &iter);
            }
            gtk_tree_path_free(path);
        }
        gtk_tree_row_reference_free(pl3_find3_ref);
        pl3_find3_ref = NULL;
    }

    if(pl3_find3_vbox)
    {
        gtk_widget_destroy(pl3_find3_vbox);
        pl3_find3_vbox = NULL;
    }
    if(pl3_find3_store2)
    {
        g_object_unref(pl3_find3_store2);
        pl3_find3_store2 = NULL;
    }

}
static void pl3_find3_browser_status_changed(MpdObj *mi,ChangedStatusType what, void *data)
{
  if(what&MPD_CST_PLAYLIST)
  {
    pl3_find3_browser_search(); 
  }
}	
static void pl3_find3_save_myself(void)
{
	if(pl3_find3_ref)
	{
		GtkTreePath *path = gtk_tree_row_reference_get_path(pl3_find3_ref);
		if(path)
		{
			gint *indices = gtk_tree_path_get_indices(path);
			debug_printf(DEBUG_INFO,"Saving myself to position: %i\n", indices[0]);
			cfg_set_single_value_as_int(config, "find3-browser","position",indices[0]);
			gtk_tree_path_free(path);
		}
	}
}

