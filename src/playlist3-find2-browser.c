/*
 *Copyright (C) 2004-2005 Qball Cow <qball@sarine.nl>
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
#include "playlist3-find-browser.h"
#include "config1.h"
#include "id3info.h"

static void pl3_find2_browser_category_selection_changed(GtkWidget *, GtkTreeIter *);
static void pl3_find2_browser_selected(GtkWidget *);
static void pl3_find2_browser_unselected(GtkWidget *);
static void pl3_find2_browser_add(GtkWidget *cat_tree);
static int pl3_find2_browser_add_go_menu(GtkWidget *);
static void pl3_find2_browser_search(void);
static void pl3_find2_browser_row_activated(GtkTreeView *, GtkTreePath *);
static int pl3_find2_browser_playlist_key_press(GtkWidget *, GdkEventKey *);
static void pl3_find2_browser_add_selected(void);
static void pl3_find2_browser_button_release_event(GtkWidget *but, GdkEventButton *event);
static void pl3_find2_browser_connection_changed(MpdObj *mi, int connect, gpointer data);
static int pl3_find2_browser_key_press_event(GtkWidget *mw, GdkEventKey *event, int type);
extern GladeXML *pl3_xml;

enum{
	PL3_FIND2_PATH,
	PL3_FIND2_TYPE,
	PL3_FIND2_TITLE,
	PL3_FIND2_ICON,
	PL3_FIND2_PID,
	PL3_FIND2_ROWS
};


#define	PL3_FIND2_CB_PLAYLIST 99
/**
 * Plugin structure
 */
gmpcPlBrowserPlugin find2_browser_gbp = {
	pl3_find2_browser_add,
	pl3_find2_browser_selected,
	pl3_find2_browser_unselected,
	pl3_find2_browser_category_selection_changed,
	NULL,
	NULL,
	NULL,
	pl3_find2_browser_add_go_menu,
	pl3_find2_browser_key_press_event
};

gmpcPlugin find2_browser_plug = {
	"Database Search Browser",
	{1,1,1},
	GMPC_PLUGIN_PL_BROWSER,
	0,
	NULL,			                /* path*/
	NULL,			                /* init */
        NULL,                                   /* Destroy */
	&find2_browser_gbp,		        /* Browser */
	NULL,			                /* status changed */
	pl3_find2_browser_connection_changed, 	/* connection changed */
	NULL,		                        /* Preferences */
	NULL,			                /* MetaData */
	NULL,                                   /* get_enabled */
	NULL                                    /* set_enabled */
};


typedef struct {
    GtkWidget *hbox;
    GtkWidget *combo;
    GtkWidget *entry;
}crit_struct;



/* Playlist window row reference */
static GtkTreeRowReference *pl3_find2_ref = NULL;


extern GladeXML *pl3_xml;

/* internal */
GtkWidget 	*pl3_find2_tree 	= NULL;
GtkListStore 	*pl3_find2_store 	= NULL;
GtkWidget 	*pl3_find2_vbox 	= NULL;
GtkWidget	*pl3_find2_findbut     = NULL;
GtkWidget       *pl3_find2_critaddbut   = NULL;
GtkListStore	*pl3_find2_combo_store 	= NULL;
GtkWidget	*pl3_find2_pb = NULL;

GList *criterias = NULL;
GtkWidget *pl3_find2_crit_vbox = NULL;

static int pl3_find2_browser_button_press_event(GtkTreeView *tree, GdkEventButton *event)
{
	GtkTreeSelection *sel = gtk_tree_view_get_selection(tree);
	if(event->button != 3 || gtk_tree_selection_count_selected_rows(sel) < 2|| !mpd_check_connected(connection))	
	{
		return FALSE;
	}
	return TRUE;
}

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
/*	gtk_list_store_append(pl3_find2_combo_store, &iter);
	gtk_list_store_set(pl3_find2_combo_store, &iter, 1, "Playlist", 0,PL3_FIND2_CB_PLAYLIST, -1);

	gtk_combo_box_set_active(GTK_COMBO_BOX(pl3_find2_combo), 0);
*/
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
}
static void pl3_find2_browser_add_crit()
{
    crit_struct *cs = g_malloc0(sizeof(*cs));
    GtkWidget *removebut = NULL;
    GtkCellRenderer *renderer = NULL;

    cs->hbox = gtk_hbox_new(FALSE, 6);
    cs->combo= gtk_combo_box_new();
    renderer = gtk_cell_renderer_text_new();
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(cs->combo), renderer, TRUE);
    gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(cs->combo), renderer, "text", 1, NULL);
    gtk_combo_box_set_model(GTK_COMBO_BOX(cs->combo), GTK_TREE_MODEL(pl3_find2_combo_store));
    gtk_combo_box_set_active(GTK_COMBO_BOX(cs->combo), 0);
    gtk_box_pack_start(GTK_BOX(cs->hbox), cs->combo, FALSE, TRUE, 0);

    cs->entry = gtk_entry_new();
    g_signal_connect(G_OBJECT(cs->entry), "activate",G_CALLBACK(pl3_find2_browser_search), NULL);
    gtk_box_pack_start(GTK_BOX(cs->hbox), cs->entry, TRUE, TRUE, 0);

    removebut = gtk_button_new();
    gtk_button_set_image(GTK_BUTTON(removebut), gtk_image_new_from_stock(GTK_STOCK_REMOVE, GTK_ICON_SIZE_BUTTON));
    gtk_box_pack_end(GTK_BOX(cs->hbox), removebut, FALSE, TRUE, 0);
    g_signal_connect(G_OBJECT(removebut), "clicked", G_CALLBACK(pl3_find2_browser_remove_crit),cs);

    gtk_box_pack_start(GTK_BOX(pl3_find2_crit_vbox), cs->hbox, FALSE, TRUE,0);

    criterias = g_list_append(criterias, cs);

    gtk_widget_show_all(pl3_find2_vbox);
    /** This causes some weird crashes when adding a default crit */
   /* gtk_widget_grab_focus(cs->combo);*/
    gtk_widget_set_sensitive(pl3_find2_findbut, TRUE);

   
    gtk_widget_set_sensitive(pl3_find2_critaddbut, TRUE);
    if(g_list_length(criterias) >= (MPD_TAG_NUM_OF_ITEM_TYPES-1))
    {
        gtk_widget_set_sensitive(pl3_find2_critaddbut, FALSE);
    }
}

static void pl3_find2_browser_init()
{
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column = NULL;
    GtkWidget  *pl3_find2_sw = NULL;
    GtkWidget *hbox = NULL;

    GValue value = {0,};
    pl3_find2_store = gtk_list_store_new (PL3_FIND2_ROWS, 
            G_TYPE_STRING, /* path to file */
            G_TYPE_INT,	/* type, FILE/PLAYLIST/FOLDER  */
            G_TYPE_STRING,	/* title to display */
            G_TYPE_STRING,
            G_TYPE_INT); /* icon type */


    pl3_find2_combo_store = gtk_list_store_new(2,G_TYPE_INT, G_TYPE_STRING);
    /** Fill the view */
    pl3_find2_fill_combo();

    /* Column */
    renderer = gtk_cell_renderer_pixbuf_new ();
    column = gtk_tree_view_column_new ();
    gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
    gtk_tree_view_column_pack_start (column, renderer, FALSE);
    gtk_tree_view_column_set_attributes (column,renderer,"stock-id", PL3_FIND2_ICON,NULL);


    /* set value for ALL */
    g_value_init(&value, G_TYPE_FLOAT);
    g_value_set_float(&value, 0.0);
    g_object_set_property(G_OBJECT(renderer), "yalign", &value); 

    renderer = gtk_cell_renderer_text_new ();
    gtk_tree_view_column_pack_start (column, renderer, TRUE);
    gtk_tree_view_column_set_attributes (column,renderer,"text", PL3_FIND2_TITLE, NULL);


    /* set up the tree */
    pl3_find2_tree= gtk_tree_view_new_with_model(GTK_TREE_MODEL(pl3_find2_store));
    /* insert the column in the tree */
    gtk_tree_view_append_column (GTK_TREE_VIEW (pl3_find2_tree), column);                                         	
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(pl3_find2_tree), FALSE);
    gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(pl3_find2_tree), TRUE);
    gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW(pl3_find2_tree)), GTK_SELECTION_MULTIPLE);

    /* setup signals */
    g_signal_connect(G_OBJECT(pl3_find2_tree), "row-activated",G_CALLBACK(pl3_find2_browser_row_activated), NULL); 
    g_signal_connect(G_OBJECT(pl3_find2_tree), "button-press-event", G_CALLBACK(pl3_find2_browser_button_press_event), NULL);
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
    GtkWidget *event = gtk_event_box_new();
    GtkWidget *vbox = gtk_vbox_new(FALSE,6);
    GtkWidget *label = gtk_label_new("");
    gtk_misc_set_alignment(GTK_MISC(label), 0,0.5);
    /* Add button */
    pl3_find2_critaddbut = gtk_button_new();
    gtk_button_set_image(GTK_BUTTON(pl3_find2_critaddbut), gtk_image_new_from_stock(GTK_STOCK_ADD, GTK_ICON_SIZE_BUTTON));
    g_signal_connect(G_OBJECT(pl3_find2_critaddbut), "clicked", G_CALLBACK(pl3_find2_browser_add_crit), NULL);


    /* Find button */
    pl3_find2_findbut = gtk_button_new_from_stock(GTK_STOCK_FIND);
    gtk_widget_set_sensitive(pl3_find2_findbut, FALSE);
    g_signal_connect(G_OBJECT(pl3_find2_findbut), "clicked", G_CALLBACK(pl3_find2_browser_search),NULL);
    gtk_label_set_markup(GTK_LABEL(label), _("<span size='xx-large' weight='bold'>Database Search</span>"));
    hbox = gtk_hbox_new(FALSE,0);
    gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, TRUE,0);
    gtk_box_pack_start(GTK_BOX(hbox), pl3_find2_findbut, FALSE, TRUE,0);       
    gtk_box_pack_start(GTK_BOX(hbox), pl3_find2_critaddbut, FALSE, TRUE,0);       
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE,0);
    /* add it */

    gtk_container_set_border_width(GTK_CONTAINER(vbox), 12);
    g_signal_connect(G_OBJECT(vbox), "style-set", G_CALLBACK(pl3_find2_browser_bg_style_changed), event);

    pl3_find2_crit_vbox = gtk_vbox_new(FALSE,6);
    gtk_box_pack_start(GTK_BOX(vbox), pl3_find2_crit_vbox, FALSE, TRUE,0);

    /* add it */
    gtk_container_add(GTK_CONTAINER(event), vbox);
    gtk_box_pack_start(GTK_BOX(pl3_find2_vbox), event, FALSE, TRUE,0);

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
}
static void pl3_find2_browser_unselected(GtkWidget *container)
{
    gtk_container_remove(GTK_CONTAINER(container), pl3_find2_vbox);
}




/*****************************************************************
 * Find Browser
 */
/* add's the toplevel entry for the current playlist view */
static void pl3_find2_browser_add(GtkWidget *cat_tree)
{
    GtkTreePath *path = NULL;
    GtkTreeIter iter;
    gtk_tree_store_append(pl3_tree, &iter, NULL);
    gtk_tree_store_set(pl3_tree, &iter, 
            PL3_CAT_TYPE, find2_browser_plug.id,
            PL3_CAT_TITLE, _("Database Search"),
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

static unsigned long pl3_find2_browser_view_browser()
{
    char *markdata = cfg_get_single_value_as_string_with_default(config, "playlist", "browser_markup",DEFAULT_MARKUP_BROWSER);
    int time=0;
    GList *node = NULL;
    int found = 0;
    GtkTreeIter child;

    MpdData *data = NULL;
    gtk_list_store_clear(pl3_find2_store);
    if(criterias == NULL)
    {
        cfg_free_string(markdata); 
        return 0;
    }
    for(node= g_list_first(criterias); node; node = g_list_next(node))
    {
        GtkTreeIter cc_iter;
        int num_field;
        crit_struct *cs = node->data;
        const gchar *name = gtk_entry_get_text(GTK_ENTRY(cs->entry));
        if(gtk_combo_box_get_active_iter(GTK_COMBO_BOX(cs->combo), &cc_iter) && name && name[0] != '\0')
        {
            if(!found)
            {
                mpd_database_search_start(connection, FALSE);
                found = TRUE;
            }
            gtk_tree_model_get(GTK_TREE_MODEL(pl3_find2_combo_store),&cc_iter , 0, &num_field, -1);
            mpd_database_search_add_constraint(connection, num_field, (char *)name);
        }
    }
    if(!found)
    {
        cfg_free_string(markdata); 
        return 0;
    }

    data = mpd_database_search_commit(connection);
    gtk_tree_view_set_model(GTK_TREE_VIEW(pl3_find2_tree), NULL);
    while (data != NULL)
    {
        gchar buffer[1024];
        if(data->type == MPD_DATA_TYPE_SONG)
        { 
            if(data->song->time != MPD_SONG_NO_TIME)
            {
                time += data->song->time;
            }

            mpd_song_markup(buffer, 1024, markdata,
                    data->song);

            /* add as child of the above created parent folder */
            gtk_list_store_append (pl3_find2_store, &child);
            gtk_list_store_set (pl3_find2_store, &child,
                    PL3_FIND2_PATH, data->song->file,
                    PL3_FIND2_TITLE, buffer,
                    PL3_FIND2_TYPE, PL3_ENTRY_SONG, 
                    PL3_FIND2_ICON, "media-audiofile", 
                    -1);
        }
        else if (data->type == MPD_DATA_TYPE_TAG && data->tag_type == MPD_TAG_ITEM_ARTIST)
        {
            gtk_list_store_prepend (pl3_find2_store, &child);
            gtk_list_store_set (pl3_find2_store, &child,
                    PL3_FIND2_PATH, data->tag,
                    PL3_FIND2_TITLE, data->tag,
                    PL3_FIND2_TYPE, PL3_ENTRY_ARTIST, 
                    PL3_FIND2_ICON, "media-artist", 			  
                    -1);
        }
        else if (data->type == MPD_DATA_TYPE_TAG && data->tag_type == MPD_TAG_ITEM_ALBUM)
        {
            char *buffer = NULL;
            if(data->tag)
            {
                buffer = g_strdup_printf("%s - %s", data->tag, data->tag);
            }
            else
            {
                buffer = g_strdup(data->tag);
            }
            gtk_list_store_prepend (pl3_find2_store, &child);
            gtk_list_store_set (pl3_find2_store, &child,
                    PL3_FIND2_PATH, data->tag,
                    PL3_FIND2_TITLE, buffer,
                    PL3_FIND2_TYPE, PL3_ENTRY_ALBUM,
                    PL3_FIND2_ICON, "media-album",
                    -1);
            q_free(buffer);
        }

        data =  mpd_data_get_next(data);
    }
    gtk_tree_view_set_model(GTK_TREE_VIEW(pl3_find2_tree), GTK_TREE_MODEL(pl3_find2_store));
    cfg_free_string(markdata); 
    return time;
}


static void pl3_find2_browser_search()
{
    long unsigned time = 0;
    gchar *string;	
    gtk_list_store_clear(pl3_find2_store);
    time = pl3_find2_browser_view_browser();
    string = format_time(time);
    gtk_statusbar_push(GTK_STATUSBAR(glade_xml_get_widget(pl3_xml, "statusbar2")),0, string);
    q_free(string);
    return;	
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
        /* iterate over every row */
        list = g_list_last (list);
        do
        {
            GtkTreeIter iter;
            mpd_Song *song =NULL;
            char *path;
            GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(pl3_find2_tree));
            gtk_tree_model_get_iter (model, &iter, (GtkTreePath *) list->data);
            gtk_tree_model_get(model,&iter,PL3_FIND2_PATH, &path,-1);
            song = mpd_database_get_fileinfo(connection, path);
            if(song)
                call_id3_window_song(song); 
            q_free(path);
        }
        while ((list = g_list_previous (list)) && mpd_check_connected(connection));
        /* free list */
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
    gtk_tree_model_get(gtk_tree_view_get_model(tree), &iter, PL3_FIND2_PATH,&song_id, PL3_FIND2_TYPE, &r_type, -1);
    {
        int playlist_length = mpd_playlist_get_playlist_length(connection);
        pl3_push_statusbar_message(_("Added a song"));
        if(mpd_server_check_command_allowed(connection, "addid") == MPD_SERVER_COMMAND_ALLOWED){
            int songid = mpd_playlist_add_get_id(connection, song_id);
            if(songid >= 0) {
                mpd_player_play_id(connection, songid);
            }
        } else{
            mpd_playlist_add(connection, song_id);
            if(playlist_length == 0)
            {
                mpd_player_play(connection);
            }
        }
    }

    q_free(song_id);
}

static void pl3_find2_browser_category_selection_changed(GtkWidget *tree, GtkTreeIter *iter)
{
    long unsigned time = 0;
    gchar *string;	
    gtk_list_store_clear(pl3_find2_store);
    time = pl3_find2_browser_view_browser();
    string = format_time(time);
    gtk_statusbar_push(GTK_STATUSBAR(glade_xml_get_widget(pl3_xml, "statusbar2")),0, string);
    q_free(string);
}

static void pl3_find2_browser_replace_selected()
{
    mpd_playlist_clear(connection);
    pl3_find2_browser_add_selected();
    mpd_player_play(connection);	

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
    GtkTreeModel *model = GTK_TREE_MODEL (pl3_find2_store);
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
            gtk_tree_model_get (model, &iter, PL3_FIND2_PATH,&name, PL3_FIND2_TYPE, &type, -1);	  
            /* does this bitmask thingy works ok? I think it hsould */
            if(type&(PL3_ENTRY_SONG))
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


static void pl3_find2_browser_button_release_event(GtkWidget *but, GdkEventButton *event)
{
    if(event->button != 3) return;
    else if(gtk_tree_selection_count_selected_rows(gtk_tree_view_get_selection(GTK_TREE_VIEW(pl3_find2_tree))) > 0)
    {
        GtkWidget *item;
        GtkWidget *menu = gtk_menu_new();
        item = gtk_image_menu_item_new_from_stock(GTK_STOCK_ADD,NULL);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
        g_signal_connect(G_OBJECT(item), "activate",
                G_CALLBACK(pl3_find2_browser_add_selected), NULL);
        /* add the replace widget */
        item = gtk_image_menu_item_new_with_label("Replace");
        gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
                gtk_image_new_from_stock(GTK_STOCK_REDO, GTK_ICON_SIZE_MENU));
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
        g_signal_connect(G_OBJECT(item), "activate",
                G_CALLBACK(pl3_find2_browser_replace_selected), NULL);

        if(mpd_server_check_version(connection,0,12,0))
        {
            item = gtk_image_menu_item_new_from_stock(GTK_STOCK_DIALOG_INFO,NULL);
            gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
            g_signal_connect(G_OBJECT(item), "activate",
                    G_CALLBACK(pl3_find2_browser_show_info), NULL);
        }
        gtk_widget_show_all(menu);
        gtk_menu_popup(GTK_MENU(menu), NULL, NULL,NULL, NULL, event->button, event->time);
    }
    return;
}

static void pl3_find2_browser_disconnect()
{
    if(pl3_find2_store) gtk_list_store_clear(pl3_find2_store);
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

/**
 * Trigger Playlist search
 * This switches to the search window set focus on entry and set searh on playlist.
 * TODO: Move to search plugin?
 */
static void pl3_playlist_search()
{
    if(!mpd_check_connected(connection))
    {
        return;
    }

    if(pl3_xml)
    {
        GtkTreePath *path = gtk_tree_row_reference_get_path(pl3_find2_ref);
        GtkTreeSelection *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(glade_xml_get_widget(pl3_xml, "cat_tree")));
        gtk_window_present(GTK_WINDOW(glade_xml_get_widget(pl3_xml, "pl3_win")));
        gtk_tree_selection_select_path(sel, path);
        gtk_tree_view_set_cursor(GTK_TREE_VIEW(glade_xml_get_widget(pl3_xml, "cat_tree")), path, NULL, FALSE);
        gtk_tree_path_free(path);
    }
}

static int pl3_find2_browser_add_go_menu(GtkWidget *menu)
{
    GtkWidget *item = NULL;

    item = gtk_image_menu_item_new_with_label(_("Database Search"));
    gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item), 
            gtk_image_new_from_stock("gtk-find", GTK_ICON_SIZE_MENU));
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
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

static int pl3_find2_browser_key_press_event(GtkWidget *mw, GdkEventKey *event, int type)
{
    if (event->keyval == GDK_F5)
    {
        pl3_find2_browser_activate();
        return TRUE;
    }                                           	
    else if(event->state&GDK_CONTROL_MASK && event->keyval == GDK_j)
    {
        pl3_playlist_search();
        return TRUE;
    }

    return FALSE;
}
