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
#include "main.h"
#include "misc.h"
#include "playlist3.h"
/* every part split out over multiple files */
#include "revision.h"
#include "gmpc-clicklabel.h"
#include <gmpc-liststore-sort.h>
#include "vala/gmpc-progress.h"

/**
 * Default keybinding settings are defined here:
 */
#include "playlist3-keybindings.h"

#define ALBUM_SIZE_SMALL 40
#define ALBUM_SIZE_LARGE 70
/* Drag and drop Target table */
static GtkTargetEntry target_table[] = 
{
    { "x-url/http", 0, 0 },
	{ "_NETSCAPE_URL", 0, 1},
	{ "text/uri-list", 0, 2},
    { "audio/*",0,3},
    { "audio/x-scpls", 0,4},
	{ "internal-drop",0,99}
};


GtkWidget *new_pb = NULL;
GtkWidget *header_labels[5];
void playlist3_new_header(void);
void playlist3_update_header(void);

static gboolean playlist3_error_expose(GtkWidget *wid, GdkEventExpose *event, gpointer data);
static void playlist_connection_changed(MpdObj *mi, int connect, gpointer data);

gboolean pl3_pb_button_press_event (GtkWidget *pb, GdkEventButton *event, gpointer user_data);
gboolean pl3_pb_scroll_event ( GtkWidget *pb, GdkEventScroll *event, gpointer user_data);

void set_browser_format(void);
void set_playlist_format(void);
static void playlist_zoom_level_changed(void);
static void playlist_player_volume_changed(GtkWidget *vol_but);
void pl3_option_menu_activate(void);
static void pl3_plugin_changed_interface(void);

int pl3_zoom = PLAYLIST_NO_ZOOM;
int pl3_old_zoom = PLAYLIST_NO_ZOOM;

/* Glade declarations, otherwise these would be static */
void about_window(void);
int pl3_cat_tree_button_press_event(GtkTreeView *, GdkEventButton *);
int pl3_cat_tree_button_release_event(GtkTreeView *, GdkEventButton *);
int pl3_window_key_press_event(GtkWidget *, GdkEventKey *);
int pl3_window_key_press_event(GtkWidget *, GdkEventKey *);
void playlist_zoom_in(void);
void playlist_zoom_out(void);
int pl3_cat_key_press_event(GtkWidget *, GdkEventKey *);
void cur_song_center_enable_tb(GtkToggleButton *);
void show_cover_case_tb(GtkToggleButton *but);
void save_possize_enable_tb(GtkToggleButton *);
void playlist_menu_repeat_changed(GtkCheckMenuItem *);
void playlist_menu_random_changed(GtkCheckMenuItem *);
void playlist_menu_cover_image_changed(GtkCheckMenuItem *);
void playlist_player_cover_art_pressed(GtkEventBox *, GdkEventButton *);
void hide_on_close_enable_tb(GtkToggleButton *but);
void pl3_window_fullscreen(void);
gboolean pl3_close(void);
static void pl3_update_profiles_menu(GmpcProfiles *prof,const int changed, const int col, const gchar *id);
gboolean playlist3_enter_notify_event(GtkWidget *wid, GdkEventCrossing *event, gpointer data);
gboolean playlist3_leave_notify_event(GtkWidget *wid, GdkEventCrossing *event, gpointer data);

static void pl3_profiles_changed(GmpcProfiles *prof,const int changed, const int col, const gchar *id);
static void playlist3_server_output_changed(GtkWidget *item, gpointer data);
static void playlist3_fill_server_menu(void);
static void playlist3_server_update_db(void);

/* Old category browser style */
static int old_type = -1;

/* interface description */
GladeXML *pl3_xml = NULL;
/* category treeview-store */
GtkTreeModel *pl3_tree = NULL;


/* size */
GtkAllocation pl3_wsize = {0,0,0,0};
int pl3_hidden = TRUE;

static void playlist_status_changed(MpdObj *mi, ChangedStatusType what, void *userdata);

/* Playlist "Plugin" */
static void playlist_pref_construct(GtkWidget *container);
static void playlist_pref_destroy(GtkWidget *container);
GladeXML *playlist_pref_xml = NULL;

static GtkWidget *volume_slider = NULL;

static guint updating_id = 0;

gmpcPrefPlugin playlist_gpp = {
	.construct					= playlist_pref_construct,
	.destroy  					= playlist_pref_destroy
};

gmpcPlugin playlist_plug = {
	.name 						= N_("Interface"),
	.version 					= {1,1,1},
	.plugin_type 				= GMPC_INTERNALL,
	.mpd_status_changed 		= &playlist_status_changed,
    .mpd_connection_changed     = &playlist_connection_changed,
	.pref 						= &playlist_gpp,
};

/* Get the type of the selected row..
 * -1 means no row selected
 */
int  pl3_cat_get_selected_browser()
{
	return old_type;
}


/**************************************************
 * Category Tree
 */
static void pl3_initialize_tree(void)
{
	int i;
	GtkTreePath *path;
	GtkTreeSelection *sel;
	if(pl3_xml == NULL) return;

	path = gtk_tree_path_new_from_string("0");
	sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(glade_xml_get_widget(pl3_xml, "cat_tree")));
	if(old_type >= 0)
	{
		GtkWidget *container = glade_xml_get_widget(pl3_xml, "browser_container");
		plugins[plugin_get_pos(old_type)]->browser->unselected(container);
		old_type = -1;
	}

	gtk_list_store_clear(GTK_LIST_STORE(pl3_tree));

	for(i=0; i< num_plugins;i++)
	{
		if(plugins[i]->plugin_type&GMPC_PLUGIN_PL_BROWSER)
		{
			if(!(plugins[i]->get_enabled && plugins[i]->get_enabled() == FALSE))
			{
				if(plugins[i]->browser && plugins[i]->browser->add)
				{
					plugins[i]->browser->add(glade_xml_get_widget(pl3_xml, "cat_tree"));
				}
			}
		}
	}


	gtk_tree_selection_select_path(sel, path);
	gtk_tree_view_set_cursor(GTK_TREE_VIEW(glade_xml_get_widget(pl3_xml, "cat_tree")),
			path, NULL, FALSE);
	gtk_tree_path_free(path);
}

static void pl3_cat_combo_changed(GtkComboBox *box)
{
	GtkTreeIter iter;
	GtkTreeSelection *selec = gtk_tree_view_get_selection((GtkTreeView *)glade_xml_get_widget (pl3_xml, "cat_tree"));
	GtkTreeModel *model =  gtk_tree_view_get_model((GtkTreeView *)glade_xml_get_widget (pl3_xml, "cat_tree"));
	if(gtk_combo_box_get_active_iter(box, &iter)) 
	{
		GtkTreeIter cat_iter;
		GtkTreePath *path = NULL;

        path = gtk_tree_model_get_path(gtk_combo_box_get_model(box), &iter);
        if( path && gtk_tree_model_get_iter(GTK_TREE_MODEL(pl3_tree), &cat_iter, path))
		{
			GtkTreeIter piter;
			if(gtk_tree_selection_get_selected(selec,&model, &piter))
			{
				GtkTreePath *ppath = gtk_tree_model_get_path(model, &piter);
				if(ppath && gtk_tree_path_is_descendant(ppath, path))
				{
					gtk_tree_path_free(path);
					gtk_tree_path_free(ppath);
					return;
				}
				gtk_tree_path_free(ppath);
			}	
			if(gtk_tree_path_get_depth(path)>0)
			{
				if(!gtk_tree_selection_iter_is_selected(selec, &cat_iter))
				{
					gtk_tree_selection_select_iter(selec, &cat_iter);
				}
			}
		}
		if(path)
			gtk_tree_path_free(path);
	}
}


/**
 * Function to handle a change in category.
 */
static void pl3_cat_sel_changed(GtkTreeSelection *selec, gpointer *userdata)
{
    GtkTreeView *tree = (GtkTreeView *)glade_xml_get_widget (pl3_xml, "cat_tree");
	GtkTreeModel *model = gtk_tree_view_get_model(tree); 
	GtkTreeIter iter;
	GtkWidget *container = glade_xml_get_widget(pl3_xml, "browser_container");
    if(!model)
        return;

	if(gtk_tree_selection_get_selected(selec,&model, &iter))
	{
		gint type;
		gtk_tree_model_get(model, &iter, 0, &type, -1);

		/**
		 * Reposition the breadcrumb
		 */
		{
			int *ind;
			GtkTreePath *path;
			path = gtk_tree_model_get_path(model, &iter);
			ind = gtk_tree_path_get_indices(path);
			gtk_combo_box_set_active(GTK_COMBO_BOX(glade_xml_get_widget(pl3_xml, "cb_cat_selector")),
					ind[0]);

			gtk_tree_path_free(path);

		}
		/**
		 * Start switching side view (if type changed )
		 */
        if(old_type != -1 && plugins[plugin_get_pos(old_type)]->browser->unselected != NULL)
        {
            plugins[plugin_get_pos(old_type)]->browser->unselected(container);
        }
        old_type = -1;
		pl3_push_rsb_message("");
		/** if type changed give a selected signal */
		if((old_type != type) && (plugins[plugin_get_pos(type)]->browser->selected != NULL))
		{ 
			plugins[plugin_get_pos(type)]->browser->selected(container);
		}
		/**
		 * update old value, so get_selected_category is correct before calling selection_changed
		 */
		old_type = type;

	}
	else
	{
		if(old_type != -1 && plugins[plugin_get_pos(old_type)]->browser->unselected)
		{
			plugins[plugin_get_pos(old_type)]->browser->unselected(container);
		}
		old_type = -1;
		gtk_tree_model_get_iter_first(model, &iter);
		gtk_tree_selection_select_iter(selec, &iter);

	}
	pl3_option_menu_activate();
}


/* handle right mouse clicks on the cat tree view */
/* gonna be a big function*/
int pl3_cat_tree_button_press_event(GtkTreeView *tree, GdkEventButton *event)
{
	GtkTreeSelection *sel = gtk_tree_view_get_selection(tree);
	if(event->button != 3 ||
			gtk_tree_selection_count_selected_rows(sel) < 2||
			!mpd_check_connected(connection))
	{
		return FALSE; 
	}
	return TRUE;

}

void pl3_option_menu_activate(void)
{
	GtkWidget *tree = glade_xml_get_widget (pl3_xml, "cat_tree");
	int i;
	gint type  = pl3_cat_get_selected_browser();
	int menu_items = 0;
	GdkEventButton *event = NULL;
	GtkWidget *menu = NULL;

	gtk_menu_item_set_submenu(GTK_MENU_ITEM(glade_xml_get_widget(pl3_xml, "menu_option")), NULL);

	if(!mpd_check_connected(connection) || type == -1) return;


	menu = gtk_menu_new();

	for(i=0; i< num_plugins;i++)
	{
		if(plugins[i]->browser != NULL)
		{
			if(plugins[i]->browser->cat_right_mouse_menu != NULL)
			{
				menu_items += plugins[i]->browser->cat_right_mouse_menu(menu,type,tree,event);
			}
		}
	}
	if(menu_items)
	{
		gtk_widget_show_all(menu);
    	gtk_menu_item_set_submenu(GTK_MENU_ITEM(glade_xml_get_widget(pl3_xml, "menu_option")), menu);
	    gtk_widget_set_sensitive(GTK_WIDGET(glade_xml_get_widget(pl3_xml, "menu_option")),TRUE);
	}
	else{
		gtk_widget_destroy(menu);
    gtk_widget_set_sensitive(GTK_WIDGET(glade_xml_get_widget(pl3_xml, "menu_option")),FALSE);
  }

}
int pl3_cat_tree_button_release_event(GtkTreeView *tree, GdkEventButton *event)
{
	int i;
	gint type  = pl3_cat_get_selected_browser();
	int menu_items = 0;
	GtkWidget *menu = NULL;
	if(type == -1 || !mpd_check_connected(connection) || event->button != 3)
	{
		/* no selections, or no usefull one.. so propagate the signal */
		return FALSE;
	}

	menu = gtk_menu_new();

	for(i=0; i< num_plugins;i++)
	{
		if(plugins[i]->browser != NULL)
		{
			if(plugins[i]->browser->cat_right_mouse_menu != NULL)
			{
				menu_items += plugins[i]->browser->cat_right_mouse_menu(menu,type,GTK_WIDGET(tree),event);
			}
		}
	}

	if(menu_items)
	{
		gtk_widget_show_all(menu);
		gtk_menu_popup(GTK_MENU(menu), NULL, NULL,NULL, NULL, /*event->button*/0, event->time);
	}
	else
	{
		gtk_widget_destroy(menu);
	}
	return TRUE;
}


/**********************************************************
 * MISC
 */
gboolean pl3_window_is_fullscreen(void)
{
	GtkWidget *win = glade_xml_get_widget(pl3_xml, "pl3_win");
	GdkWindowState state  = 0;
	if(win->window)
		state = gdk_window_get_state(win->window);	
	return (state&GDK_WINDOW_STATE_FULLSCREEN)? TRUE:FALSE;
}
void pl3_window_fullscreen(void)
{
	GtkWidget *win = glade_xml_get_widget(pl3_xml, "pl3_win");

    if(pl3_zoom < PLAYLIST_MINI)
    {
        if(pl3_window_is_fullscreen())
        {
            gtk_window_unfullscreen(GTK_WINDOW(win));
        }
        else{
            gtk_window_fullscreen(GTK_WINDOW(win));
        }
    }
}
int pl3_window_key_press_event(GtkWidget *mw, GdkEventKey *event)
{
    int i=0;
    int found = 0;
    conf_mult_obj *list;
    gint type = pl3_cat_get_selected_browser();
    /**
     * Following key's are only valid when connected
     */
    if(!mpd_check_connected(connection))
    {
        return FALSE;
    }

    for(i=0; i< num_plugins;i++) 
    {
        if(plugins[i]->plugin_type&GMPC_PLUGIN_PL_BROWSER) 
        {
            if(plugins[i]->browser && plugins[i]->browser->key_press_event) 
            {
                if((plugins[i]->browser->key_press_event(mw, event, type))) return TRUE;
            }                                                                           	
        }
    }

//    printf("%i-%i\n", event->state, event->keyval); 
    list = cfg_get_key_list(config, KB_GLOBAL);
    /* If no keybindings are found, add the default ones */
    if(list == NULL)
    {
        for(i=0;i<KB_NUM;i++)
        {
            cfg_set_single_value_as_int(config, KB_GLOBAL,Keybindname[i], KeybindingDefault[i][0]);
            cfg_set_single_value_as_int(config, MK_GLOBAL,Keybindname[i], KeybindingDefault[i][1]);
            cfg_set_single_value_as_int(config, AC_GLOBAL,Keybindname[i],KeybindingDefault[i][2]);
        }
        list = cfg_get_key_list(config, KB_GLOBAL);
    }
    /* Walk through the keybinding list */
    if(list)
    {
        int edited=0;
        conf_mult_obj *iter = NULL;
        /* Sort list on name. so chains can be defined */
        do{
            edited = 0;
            iter = list;
            do{
                if(iter->next){
                    if(strcmp(iter->key, iter->next->key) > 0)
                    {
                        char *temp = iter->key;
                        iter->key = iter->next->key;
                        iter->next->key = temp;
                        edited = 1;
                    }
                }
                iter = iter->next;
            } while(iter);
        }while(edited);


        for(iter = list;iter;iter = iter->next) {
            int keycode =  cfg_get_single_value_as_int_with_default(config, KB_GLOBAL,iter->key,-1);
            int keymask =  cfg_get_single_value_as_int_with_default(config, MK_GLOBAL,iter->key,0);

            if(keycode >= 0 && (event->state == keymask) && (keycode == event->keyval))
            {
                int action = cfg_get_single_value_as_int_with_default(config, AC_GLOBAL,iter->key,-1);
                found = 1;
//                printf("Doing action: %i\n",action);
                /* Play control */
                if(action == KB_ACTION_PLAY) play_song(); 
                else if(action == KB_ACTION_NEXT) next_song();
                else if(action == KB_ACTION_PREV) prev_song();
                else if(action == KB_ACTION_STOP) stop_song();
                /* Other actions */
                else if(action == KB_ACTION_CLEAR_PLAYLIST) mpd_playlist_clear(connection);
                else if(action == KB_ACTION_FULL_ADD_PLAYLIST) mpd_playlist_add(connection, "/");
                /* View control */
                else if(action == KB_ACTION_INTERFACE_COLLAPSE) playlist_zoom_out();
                else if(action == KB_ACTION_INTERFACE_EXPAND) playlist_zoom_in();
                else if(action == KB_ACTION_FULLSCREEN) pl3_window_fullscreen();
                /* Program control */
                else if(action == KB_ACTION_QUIT) main_quit(); 
                else if(action == KB_ACTION_CLOSE) pl3_close();
                else if(action == KB_ACTION_REPEAT) mpd_player_set_repeat(connection, !mpd_player_get_repeat(connection)); 
                else if(action == KB_ACTION_RANDOM) mpd_player_set_random(connection, !mpd_player_get_random(connection)); 
                else if(action == KB_ACTION_TOGGLE_MUTE) volume_toggle_mute(); 
                else {
                    debug_printf(DEBUG_ERROR, "Keybinding action (%i) for: %i %i is invalid\n", action,event->state, event->keyval);
                    found = 0;
                }
            }
        }
        cfg_free_multiple(list);
    }
	if(!found){
		return FALSE;
	}

	/* don't propagate */
	return TRUE;
}

/**
 * Let plugins handle the key-press on the Category tree
 * TODO: Make the plugins capable of stopping the single.
 * So catch return value.
 */
int pl3_cat_key_press_event(GtkWidget *mw, GdkEventKey *event)
{
	/* call default */
	int i;
	gint type = pl3_cat_get_selected_browser();

	for(i=0; i< num_plugins;i++)
	{
		if(plugins[i]->browser != NULL)
		{
			if(plugins[i]->browser->cat_key_press != NULL)
			{
				plugins[i]->browser->cat_key_press(mw,event,type);
			}
		}
	}
	/**
	 * Now return to the default handler
	 */
	return pl3_window_key_press_event(mw,event);
}

/**
 * Remove message from the status bar 
 * Used internally by timeout
 */
static int pl3_pop_statusbar_message(gpointer data)
{
	gint id = GPOINTER_TO_INT(data);
	gtk_statusbar_pop(GTK_STATUSBAR(glade_xml_get_widget(pl3_xml, "statusbar1")), id);
	return FALSE;
}
/** 
 * Put message on status bar
 * This will be removed after 5 seconds
 */
void pl3_push_statusbar_message(char *mesg)
{
	gint id = gtk_statusbar_get_context_id(GTK_STATUSBAR(glade_xml_get_widget(pl3_xml, "statusbar1")), mesg);
	/* message auto_remove after 5 sec */
	g_timeout_add_seconds(5,(GSourceFunc)pl3_pop_statusbar_message, GINT_TO_POINTER(id));
	gtk_statusbar_push(GTK_STATUSBAR(glade_xml_get_widget(pl3_xml, "statusbar1")), id,mesg);
}
/**
 * Push message to 2nd status bar
 * Message overwrites the previous message
 */
void pl3_push_rsb_message(gchar *string)
{
	gtk_statusbar_push(GTK_STATUSBAR(glade_xml_get_widget(pl3_xml, "statusbar2")),0, string);
}

/**
 * Close the playlist and save position/size 
 */
gboolean pl3_close()
{
	/* only save when window is PLAYLIST_SMALL or NO ZOOM */
	if(pl3_xml != NULL)
	{
		gtk_window_get_position(GTK_WINDOW(glade_xml_get_widget(pl3_xml, "pl3_win")), &pl3_wsize.x, &pl3_wsize.y);


		cfg_set_single_value_as_int(config, "playlist", "xpos", pl3_wsize.x);
		cfg_set_single_value_as_int(config, "playlist", "ypos", pl3_wsize.y);


		if(pl3_zoom <= PLAYLIST_SMALL)
		{
            gtk_window_get_size(GTK_WINDOW(glade_xml_get_widget(pl3_xml, "pl3_win")), &pl3_wsize.width, &pl3_wsize.height);
			debug_printf(DEBUG_INFO,"pl3_close: save size: %i %i\n", pl3_wsize.width, pl3_wsize.height);
			cfg_set_single_value_as_int(config, "playlist", "width", pl3_wsize.width);
			cfg_set_single_value_as_int(config, "playlist", "height", pl3_wsize.height);
		}
		if(pl3_zoom < PLAYLIST_SMALL)
		{
			cfg_set_single_value_as_int(config, "playlist", "pane-pos", gtk_paned_get_position(
						GTK_PANED(glade_xml_get_widget(pl3_xml, "hpaned1"))));
		}
	}

	if(cfg_get_single_value_as_int_with_default(config, "playlist", "hide-on-close", FALSE))
	{
		if(tray_icon2_get_available())
		{
			pl3_toggle_hidden();
			return TRUE;
		}
		gtk_window_iconify(GTK_WINDOW(glade_xml_get_widget(pl3_xml, "pl3_win")));
		return TRUE;

	}
	/**
	 * Quit the program
	 */
	main_quit();
	return TRUE;
}
/**
 * Hide the playlist.
 * Before hiding save current size and position 
 */
int pl3_hide()
{
	if(!tray_icon2_get_available())
	{
		gtk_window_iconify(GTK_WINDOW(glade_xml_get_widget(pl3_xml, "pl3_win")));
		return 1;
	}
	if(pl3_xml != NULL && !pl3_hidden )
	{
		/** Save position 
		*/
		gtk_window_get_position(GTK_WINDOW(glade_xml_get_widget(pl3_xml, "pl3_win")), &pl3_wsize.x, &pl3_wsize.y);
		cfg_set_single_value_as_int(config, "playlist", "xpos", pl3_wsize.x);
		cfg_set_single_value_as_int(config, "playlist", "ypos", pl3_wsize.y);
		/* save size, only when in SMALL or no zoom mode
		*/	
		if(pl3_zoom <= PLAYLIST_SMALL)
		{
			gtk_window_get_size(GTK_WINDOW(glade_xml_get_widget(pl3_xml, "pl3_win")), &pl3_wsize.width, &pl3_wsize.height);
			debug_printf(DEBUG_INFO,"pl3_hide: save size: %i %i\n", pl3_wsize.width, pl3_wsize.height);
			cfg_set_single_value_as_int(config, "playlist", "width", pl3_wsize.width);
			cfg_set_single_value_as_int(config, "playlist", "height", pl3_wsize.height);
			cfg_set_single_value_as_int(config, "playlist", "pane-pos", gtk_paned_get_position(
						GTK_PANED(glade_xml_get_widget(pl3_xml, "hpaned1"))));
		}
		gtk_widget_hide(glade_xml_get_widget(pl3_xml, "pl3_win"));
		pl3_hidden = TRUE;
	}
	return TRUE;
}



static void pl3_updating_changed(MpdObj *mi, int updating)
{
	char *mesg = _("MPD database is updating");
    printf("update changed callback %i %p\n",updating, pl3_xml );
	if(pl3_xml != NULL)
	{
		gtk_statusbar_pop(GTK_STATUSBAR(glade_xml_get_widget(pl3_xml, "statusbar1")), updating_id);
		gtk_widget_hide(glade_xml_get_widget(pl3_xml, "image_updating"));
		if(updating >0)
		{
			updating_id = gtk_statusbar_get_context_id(GTK_STATUSBAR(glade_xml_get_widget(pl3_xml, "statusbar1")), mesg);
			gtk_statusbar_push(GTK_STATUSBAR(glade_xml_get_widget(pl3_xml, "statusbar1")), updating_id,mesg);
			gtk_widget_show(glade_xml_get_widget(pl3_xml, "image_updating"));

			playlist3_show_error_message(_("<b>MPD is updating its database</b>"), ERROR_INFO);
		}
		else if(updating_id > 0) 
		{
			playlist3_show_error_message(_("<b>MPD finished updating its database</b>"), ERROR_INFO);
            updating_id = 0;
        }
	}
}

/* create the playlist view 
 * This is done only once, for the rest its hidden, but still there
 */
static void pl3_show_and_position_window()
{
	if(!pl3_xml) return;
	if(pl3_wsize.x  >0 || pl3_wsize.y>0) {
		gtk_window_move(GTK_WINDOW(glade_xml_get_widget(pl3_xml, "pl3_win")),
				pl3_wsize.x,
				pl3_wsize.y);
	}
	if(pl3_wsize.height>0 && pl3_wsize.width>0) {
        debug_printf(DEBUG_INFO,"restore size %i %i\n",pl3_wsize.width, pl3_wsize.height);
        gtk_window_resize(GTK_WINDOW(glade_xml_get_widget(pl3_xml, "pl3_win")),
				pl3_wsize.width,
				pl3_wsize.height);
	}
	gtk_widget_show(glade_xml_get_widget(pl3_xml, "pl3_win"));
	gtk_window_present(GTK_WINDOW(glade_xml_get_widget(pl3_xml, "pl3_win")));

}

void pl3_toggle_hidden()
{
	if(pl3_hidden)
	{
		create_playlist3();
	}
	else
	{
		pl3_hide();
	}
}

static void playlist3_source_drag_data_recieved (GtkWidget          *widget,
		GdkDragContext     *context,
		gint                x,
		gint                y,
		GtkSelectionData   *data,
		guint               info,
		guint               time_recieved)
{
	if(info != 99)
	{
        int found =0;
		const gchar *url_data = (gchar *)data->data; 
        /* Hack, move this too libmpd? */
        char ** handlers = mpd_server_get_url_handlers(connection);
        int has_http = FALSE, has_file = FALSE;
        int i =0;
        for(i=0; handlers && handlers[i] ; i++)
        {
            if(strcmp(handlers[i], "http://") == 0){
                has_http = TRUE;
            }else if (strcmp(handlers[i], "file://") == 0) {
                has_file = TRUE;
            }
        }
        if(handlers) g_strfreev(handlers);
        if(url_data)
        {
           
            gchar **url = g_uri_list_extract_uris(url_data);
            for(i=0; url && url[i]; i++)
            {
                if( has_file && strncmp(url[i], "file://", 7) == 0){
                    char *uri = g_uri_unescape_string(url[i], "");
                    mpd_playlist_add(connection, uri);
                    g_free(uri);
                    found = 1;
                }
                else if(has_http && strncmp(url[i], "http://", 7) == 0)
                {
                    url_start_real(url[i]);
                    found = 1;           
                }
            }
            if(url)g_strfreev(url);
        }

        gtk_drag_finish(context, found, FALSE, time_recieved);
    } else {
		MpdData * mdata ;
		gchar **stripped;
		int i;
		guchar * odata = gtk_selection_data_get_text(data);
		stripped = g_strsplit((gchar *)odata, "\n", 0);
		g_free(odata);
		if(context->action == GDK_ACTION_MOVE)
		{
			mpd_playlist_clear(connection);
		}
		mpd_database_search_start(connection, TRUE);
		for(i=0; stripped && stripped[i];i++)	
		{
			gchar ** request = g_strsplit(stripped[i],":", 2);
			mpd_database_search_add_constraint(connection, mpd_misc_get_tag_by_name(request[0]), request[1]);
			g_strfreev(request);
		}
		mdata = mpd_database_search_commit(connection);
		for(; mdata; mdata= mpd_data_get_next(mdata))
		{
			mpd_playlist_queue_add(connection, mdata->song->file);
		}
		mpd_playlist_queue_commit(connection);
		if(context->action == GDK_ACTION_MOVE)
		{
			mpd_player_play(connection);
		}

		g_strfreev(stripped);
		gtk_drag_finish(context, TRUE, FALSE, time_recieved);
	}
}







gboolean pl3_pb_scroll_event ( GtkWidget *pb, GdkEventScroll *event, gpointer user_data)
{
    if(event->direction == GDK_SCROLL_UP)
    {
        seek_ps(5);
    }
    else if (event->direction == GDK_SCROLL_DOWN)
    {
        seek_ns(5);
    }
    return TRUE;
}


gboolean pl3_pb_button_press_event (GtkWidget *pb, GdkEventButton *event, gpointer user_data)
{
    gint width;
    gdouble pos;
    if(event->type == GDK_BUTTON_PRESS)
    {
        if(event->button == 1)
        {
            if(event->window)
            {

                gdk_drawable_get_size(event->window, &width, NULL);
                pos = (gdouble)event->x/(gdouble)width;
                mpd_player_seek(connection,(int) mpd_status_get_total_song_time(connection)*pos);
            }
        }
        else if (event->button == 3)
        {
            gboolean a = !gmpc_progress_get_do_countdown(GMPC_PROGRESS(pb));
            gmpc_progress_set_do_countdown(GMPC_PROGRESS(pb), a);
            cfg_set_single_value_as_int(config, "playlist", "progressbar-countdown", a);
        }
    }
    /* propagate the signal */
    return FALSE;
}

/**
 * When the position of the slider change, update the artist image
 */
    static void
pl3_win_pane_changed(GtkWidget *panel, GParamSpec *arg1, gpointer data)
{
	gint position = 0;
	g_object_get(G_OBJECT(panel), "position", &position, NULL);
	/* force minimum size 16 */
	if(position -16 < 16) position = 32;
	gmpc_metaimage_set_size(GMPC_METAIMAGE(glade_xml_get_widget(pl3_xml, "metaimage_artist_art")), position-16);
	gmpc_metaimage_reload_image(GMPC_METAIMAGE(glade_xml_get_widget(pl3_xml, "metaimage_artist_art")));

}

static void about_dialog_activate(GtkWidget *dialog, const gchar *uri, gpointer data)
{
	open_uri(uri);
}

void create_playlist3 ()
{
    GtkListStore *pl3_crumbs = NULL;
    conf_mult_obj *list = NULL;
    GtkCellRenderer *renderer;
	GtkWidget *tree;
	GtkTreeSelection *sel;
	GtkTreeViewColumn *column = NULL;
	gchar *path = NULL;
	GtkTreeIter iter;

	/* indicate that the playlist is not hidden */
	pl3_hidden = FALSE;

	/**
	 * If the playlist allready exists,
	 * It is probly coming from a hidden state,
	 * so re-position the window 
	 */
	if(pl3_xml != NULL)
	{
		pl3_show_and_position_window();
		return;
	}


	/* initial, setting the url hook */
	gtk_about_dialog_set_url_hook((GtkAboutDialogActivateLinkFunc)about_dialog_activate, NULL, NULL);



	/* load gui desciption */
	path = gmpc_get_full_glade_path("playlist3.glade");
	pl3_xml = glade_xml_new (path, "pl3_win", NULL);
	q_free(path);
	/*
	 * Check if the file is loaded, if not then show an error message and abort the program
	 */
	if(pl3_xml == NULL)
	{
		debug_printf(DEBUG_ERROR, "Failed to open playlist3.glade.\n");
		show_error_message(_("Failed to open the interface description file!\n"
					"Please reinstall gmpc"), TRUE);
		abort();
	}
	/** murrine hack */
	if(cfg_get_single_value_as_int_with_default(config, "Default", "murrine-hack", FALSE))
	{
		GdkScreen *screen;
		GdkColormap *colormap;
		GtkWidget *win = glade_xml_get_widget(pl3_xml, "pl3_win");

		screen   = gtk_window_get_screen (GTK_WINDOW(win));
		colormap = gdk_screen_get_rgba_colormap (screen);

		if (colormap)
			gtk_widget_set_default_colormap (colormap);

	}




	/* create tree store for the "category" view */
	if (pl3_tree == NULL)
	{
		/* song id, song title */
		pl3_tree = (GtkTreeModel *)gmpc_liststore_sort_new (PL3_CAT_NROWS, 
				G_TYPE_INT,	/* row type, see free_type struct */
				G_TYPE_STRING, /* display name */
				G_TYPE_STRING,/* full path and stuff for backend */
				G_TYPE_STRING, /* icon id */
				G_TYPE_BOOLEAN,  /* cat proc */
				G_TYPE_UINT,  /* icon size */
				G_TYPE_STRING, /* browser markup */
				G_TYPE_INT, 		/* ordering */
                G_TYPE_STRING   /* Num items */
				);
	}


	tree = glade_xml_get_widget (pl3_xml, "cat_tree");
	gtk_tree_view_set_model (GTK_TREE_VIEW (tree), GTK_TREE_MODEL (pl3_tree));
	sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));
	gtk_tree_selection_set_mode(GTK_TREE_SELECTION(sel), GTK_SELECTION_BROWSE);
    gtk_tree_view_set_reorderable(GTK_TREE_VIEW(tree), TRUE);

	renderer = gtk_cell_renderer_pixbuf_new ();
	column = gtk_tree_view_column_new ();
	gtk_tree_view_column_pack_start (column, renderer, FALSE);
	gtk_tree_view_column_set_attributes (column, renderer, "icon-name",3,"stock-size",5,NULL);

	renderer = gtk_cell_renderer_text_new ();
	/* insert the column in the tree */
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_set_attributes (column, renderer, "text", 1, NULL);
    g_object_set(renderer, "ellipsize", PANGO_ELLIPSIZE_END, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);
	gtk_tree_view_set_search_column(GTK_TREE_VIEW(tree), 1);

	g_signal_connect(G_OBJECT(sel), "changed", G_CALLBACK(pl3_cat_sel_changed), NULL);

    renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_column_pack_start (column, renderer, FALSE);
	gtk_tree_view_column_set_attributes (column, renderer, "markup", PL3_CAT_NUM_ITEMS, NULL);
    g_object_set(renderer, "xalign", 1.0, NULL);
    /* Make sure the scroll bars get removed when folding in the folders again */
	gtk_tree_view_column_set_sizing(column,GTK_TREE_VIEW_COLUMN_AUTOSIZE);



	/**
	 * Bread Crumb system.
	 */
	pl3_crumbs = (GtkListStore *)(pl3_tree);
	gtk_combo_box_set_model(GTK_COMBO_BOX(glade_xml_get_widget(pl3_xml, "cb_cat_selector")), 
			GTK_TREE_MODEL(pl3_crumbs));
	renderer = gtk_cell_renderer_pixbuf_new ();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(glade_xml_get_widget(pl3_xml, "cb_cat_selector")),renderer,FALSE); 
	gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(glade_xml_get_widget(pl3_xml, "cb_cat_selector")),renderer,
			"icon-name", PL3_CAT_ICON_ID);                                                                                          	

	renderer = gtk_cell_renderer_text_new ();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(glade_xml_get_widget(pl3_xml, "cb_cat_selector")),renderer,TRUE); 
	gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(glade_xml_get_widget(pl3_xml, "cb_cat_selector")),renderer,
			"text", PL3_CAT_TITLE);

    renderer = gtk_cell_renderer_text_new();
    gtk_cell_layout_pack_end(GTK_CELL_LAYOUT(glade_xml_get_widget(pl3_xml, "cb_cat_selector")), renderer, FALSE);
	gtk_cell_layout_add_attribute (GTK_CELL_LAYOUT(glade_xml_get_widget(pl3_xml, "cb_cat_selector")), renderer, "markup", PL3_CAT_NUM_ITEMS);
    g_object_set(renderer, "xalign", 1.0, NULL);


	g_signal_connect(glade_xml_get_widget(pl3_xml, "cb_cat_selector"),
			"changed", G_CALLBACK(pl3_cat_combo_changed), NULL);
	/* initialize the category view */ 
	pl3_initialize_tree();


	gtk_widget_show(glade_xml_get_widget(pl3_xml, "vbox_playlist_player"));




    {
        GtkWidget *pb = (GtkWidget *)gmpc_progress_new();
        gtk_box_pack_start(GTK_BOX(glade_xml_get_widget(pl3_xml, "hbox_progress")), pb, TRUE, TRUE, 0);
        gtk_widget_show(pb);
        g_signal_connect(G_OBJECT(pb), "button-press-event", G_CALLBACK(pl3_pb_button_press_event), NULL);
        g_signal_connect(G_OBJECT(pb), "scroll-event", G_CALLBACK(pl3_pb_scroll_event), NULL);
        new_pb = pb;

        gmpc_progress_set_do_countdown(GMPC_PROGRESS(pb),
                cfg_get_single_value_as_int_with_default(config, "playlist", "progressbar-countdown", FALSE));
    }






	/* Add volume slider. */
	volume_slider = gtk_volume_button_new();
	gtk_button_set_relief(GTK_BUTTON(volume_slider), GTK_RELIEF_NORMAL);
	gtk_box_pack_end(GTK_BOX(glade_xml_get_widget(pl3_xml, "hbox12"/*playlist_player"*/)), volume_slider, FALSE, FALSE, 0);
	gtk_widget_show_all(volume_slider);
	playlist_status_changed(connection, MPD_CST_STATE|MPD_CST_SONGID|MPD_CST_ELAPSED_TIME|MPD_CST_VOLUME|MPD_CST_REPEAT|MPD_CST_RANDOM|MPD_CST_PERMISSION,NULL);
	g_signal_connect(G_OBJECT(volume_slider), "value_changed", G_CALLBACK(playlist_player_volume_changed), NULL);

	/* Restore values from config */
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(glade_xml_get_widget(pl3_xml, "menu_check_cover_image")),
			cfg_get_single_value_as_int_with_default(config, "playlist", "cover-image-enable", 0));

	/* Make sure change is applied */

	playlist3_new_header();
	/* connect signals that are defined in the gui description */
	glade_xml_signal_autoconnect (pl3_xml);



	if(mpd_status_db_is_updating(connection))
	{
		pl3_updating_changed(connection, 1);
	}

	/* select the current playlist */
	if(gtk_tree_model_get_iter_first(GTK_TREE_MODEL(pl3_tree), &iter))
	{
		gtk_tree_selection_select_iter(sel, &iter);
	}








		/**
	 * Insert new custom widget
	 */

	gmpc_metaimage_set_size(GMPC_METAIMAGE(glade_xml_get_widget(pl3_xml, "metaimage_album_art")), ALBUM_SIZE_LARGE);
	gmpc_metaimage_set_image_type(GMPC_METAIMAGE(glade_xml_get_widget(pl3_xml, "metaimage_album_art")), META_ALBUM_ART);
	gmpc_metaimage_set_no_cover_icon(GMPC_METAIMAGE(glade_xml_get_widget(pl3_xml, "metaimage_album_art")),"gmpc"); 
	gmpc_metaimage_set_connection(GMPC_METAIMAGE(glade_xml_get_widget(pl3_xml, "metaimage_album_art")), connection);
//	gtk_widget_set_size_request(glade_xml_get_widget(pl3_xml, "metaimage_album_art"),80,80);
	/** make sure size is updated */
	gmpc_metaimage_set_cover_na(GMPC_METAIMAGE(glade_xml_get_widget(pl3_xml, "metaimage_album_art")));

	gmpc_metaimage_set_image_type(GMPC_METAIMAGE(glade_xml_get_widget(pl3_xml, "metaimage_artist_art")), META_ARTIST_ART);
	gmpc_metaimage_set_hide_on_na(GMPC_METAIMAGE(glade_xml_get_widget(pl3_xml, "metaimage_artist_art")), TRUE);
	gmpc_metaimage_set_connection(GMPC_METAIMAGE(glade_xml_get_widget(pl3_xml, "metaimage_artist_art")), connection);
	if(!cfg_get_single_value_as_int_with_default(config, "playlist", "cover-image-enable", FALSE))
	{
		gmpc_metaimage_set_is_visible(GMPC_METAIMAGE(glade_xml_get_widget(pl3_xml, "metaimage_artist_art")), FALSE);
	}
	gmpc_metaimage_set_squared(GMPC_METAIMAGE(glade_xml_get_widget(pl3_xml, "metaimage_artist_art")), TRUE);
	gmpc_metaimage_set_size(GMPC_METAIMAGE(glade_xml_get_widget(pl3_xml, "metaimage_artist_art")), 200);

	gtk_widget_hide(glade_xml_get_widget(pl3_xml, "metaimage_artist_art"));
    /* restore the window's position and size, if the user wants this.*/
  if(cfg_get_single_value_as_int_with_default(config, "playlist", "savepossize", 0))
	{
		/* Load values from config file */
		pl3_wsize.x =	cfg_get_single_value_as_int_with_default(config, "playlist", "xpos", 0);
		pl3_wsize.y =	cfg_get_single_value_as_int_with_default(config, "playlist", "ypos", 0);
		pl3_wsize.width = cfg_get_single_value_as_int_with_default(config, "playlist", "width", 0);
		pl3_wsize.height = cfg_get_single_value_as_int_with_default(config, "playlist", "height", 0);
		/* restore location + position */
		/*pl3_show_and_position_window();*/

		if(pl3_wsize.x  >0 || pl3_wsize.y>0) {
			gtk_window_move(GTK_WINDOW(glade_xml_get_widget(pl3_xml, "pl3_win")),
					pl3_wsize.x,
					pl3_wsize.y);
		}
		if(pl3_wsize.height>0 && pl3_wsize.width>0) {
            debug_printf(DEBUG_INFO,"restore size %i %i\n",pl3_wsize.width, pl3_wsize.height);
            gtk_window_resize(GTK_WINDOW(glade_xml_get_widget(pl3_xml, "pl3_win")),
					pl3_wsize.width,
					pl3_wsize.height);
		}
		/* restore pane position */
		if(cfg_get_single_value_as_int(config, "playlist", "pane-pos") != CFG_INT_NOT_DEFINED )
		{

			gtk_paned_set_position(GTK_PANED(glade_xml_get_widget(pl3_xml, "hpaned1")),
					cfg_get_single_value_as_int(config, "playlist", "pane-pos"));
		}
		/**
		 * restore zoom level
		 */


		gtk_widget_show(glade_xml_get_widget(pl3_xml, "pl3_win"));

	}
	else
	{
		gtk_widget_show(glade_xml_get_widget(pl3_xml, "pl3_win"));
	}
    pl3_zoom = cfg_get_single_value_as_int_with_default(config, "playlist","zoomlevel",PLAYLIST_NO_ZOOM);
    playlist_zoom_level_changed();

	pl3_update_go_menu();	
	/* make it update itself */
	pl3_update_profiles_menu(gmpc_profiles,PROFILE_ADDED,-1, NULL);
	g_signal_connect(G_OBJECT(gmpc_profiles), "changed", G_CALLBACK(pl3_update_profiles_menu), NULL);
	g_signal_connect(G_OBJECT(gmpc_profiles), "changed", G_CALLBACK(pl3_profiles_changed), NULL);

	/**
	 * Set as drag destination
	 */
	gtk_drag_dest_set(glade_xml_get_widget(pl3_xml, "hbox_playlist_player"),
			GTK_DEST_DEFAULT_ALL,
			target_table, 6,
			GDK_ACTION_COPY|GDK_ACTION_LINK|GDK_ACTION_DEFAULT|GDK_ACTION_MOVE);
	g_signal_connect (G_OBJECT (glade_xml_get_widget(pl3_xml, "hbox_playlist_player")),"drag_data_received",
			GTK_SIGNAL_FUNC (playlist3_source_drag_data_recieved),
			NULL);

	/**
	 * Setup error box
	 */
	{
		GtkWidget *event = glade_xml_get_widget(pl3_xml, "error_event");
		gtk_widget_set_app_paintable(event, TRUE);
		g_signal_connect(G_OBJECT(event), "expose-event", G_CALLBACK(playlist3_error_expose), NULL);
	}

	/* A signal that responses on change of pane position */
	g_signal_connect(G_OBJECT(glade_xml_get_widget(pl3_xml,"hpaned1")),
									"notify::position", G_CALLBACK(pl3_win_pane_changed), NULL);

    /* update it */
    pl3_win_pane_changed(glade_xml_get_widget(pl3_xml,"hpaned1"), NULL, NULL);
	/**
	 *
	 */
	playlist_connection_changed(connection, FALSE,NULL);
    /**
     * Update keybindings 
     */
    list = cfg_get_key_list(config, KB_GLOBAL);
    /* If no keybindings are found, add the default ones */
    if(list == NULL)
    {
        int i;
        for(i=0;i<KB_NUM;i++)
        {
            cfg_set_single_value_as_int(config, KB_GLOBAL,Keybindname[i], KeybindingDefault[i][0]);
            cfg_set_single_value_as_int(config, MK_GLOBAL,Keybindname[i], KeybindingDefault[i][1]);
            cfg_set_single_value_as_int(config, AC_GLOBAL,Keybindname[i],KeybindingDefault[i][2]);
        }
        list = cfg_get_key_list(config, KB_GLOBAL);
    }
    if(list) {
        GtkAccelGroup *ac= gtk_accel_group_new();
        int action_seen = 0;
            //        GtkAccelGroup *ac = gtk_menu_get_accel_group(glade_xml_get_widget(pl3_xml, "menuitem_control_menu"));
        conf_mult_obj *conf_iter = list;
        gtk_window_add_accel_group(GTK_WINDOW(glade_xml_get_widget(pl3_xml, "pl3_win")), ac);
        while(conf_iter){
            int action = cfg_get_single_value_as_int_with_default(config, AC_GLOBAL,conf_iter->key,-1);
            int keycode =  cfg_get_single_value_as_int_with_default(config, KB_GLOBAL,conf_iter->key,-1);
            int keymask =  cfg_get_single_value_as_int_with_default(config, MK_GLOBAL,conf_iter->key,0);
            if(keycode >=0 && action >= 0)
            {
                GtkWidget *item = NULL;
                int state = (((action_seen)&(1<<action)) == 0)?GTK_ACCEL_VISIBLE:0;
                action_seen |= (1<<action);

                if(action == KB_ACTION_PLAY) {
                   item = glade_xml_get_widget(pl3_xml,"menu_play"); 
                } else 
                if(action == KB_ACTION_STOP) {
                   item = glade_xml_get_widget(pl3_xml,"menu_stop"); 
                } else
                if(action == KB_ACTION_NEXT) {
                   item = glade_xml_get_widget(pl3_xml,"menu_next"); 
                } else
                if(action == KB_ACTION_PREV) {
                   item = glade_xml_get_widget(pl3_xml,"menu_prev"); 
                }
                if(action == KB_ACTION_FULLSCREEN) {
                   item = glade_xml_get_widget(pl3_xml,"fullscreen2"); 
                } else 
                if(action == KB_ACTION_INTERFACE_EXPAND) {
                   item = glade_xml_get_widget(pl3_xml,"zoom_in2"); 
                } else
               if(action == KB_ACTION_INTERFACE_COLLAPSE) {
                   item = glade_xml_get_widget(pl3_xml,"zoom_out2"); 
               } else 
               if(action == KB_ACTION_REPEAT) {
                   item = glade_xml_get_widget(pl3_xml,"menu_repeat"); 
               } else
               if(action == KB_ACTION_RANDOM) {
                   item = glade_xml_get_widget(pl3_xml,"menu_random"); 
               
               } else 
               if (action == KB_ACTION_TOGGLE_MUTE) {
                   item = glade_xml_get_widget(pl3_xml,"menu_mute_toggle"); 
               }
               if(item){
                   gtk_widget_add_accelerator(item, "activate", ac, keycode, keymask, state);
               }
            }
            conf_iter = conf_iter->next;
        }
        cfg_free_multiple(list);
    }



}

/**
 * Helper functions
 */
gboolean playlist3_get_active()
{
	return (pl3_xml != NULL);
}
GtkListStore *playlist3_get_category_tree_store()
{
	if(!playlist3_get_active()) return NULL;
	return GTK_LIST_STORE(pl3_tree);
}
GtkTreeView *playlist3_get_category_tree_view()
{
	if(!playlist3_get_active()) return NULL;
	return (GtkTreeView *)glade_xml_get_widget(pl3_xml, "cat_tree");
} 

/****************************************************************************************
 *  PREFERENCES										*
 ****************************************************************************************/
/* prototyping for glade */
void ck_stop_on_exit_toggled_cb(GtkToggleButton *but, gpointer data);
void ck_show_tooltip_enable_tb(GtkToggleButton *but);

void show_cover_case_tb(GtkToggleButton *but)
{
	int bool1  = gtk_toggle_button_get_active(but);
	cfg_set_single_value_as_int(config, "metaimage","addcase", bool1);
}
void ck_stop_on_exit_toggled_cb(GtkToggleButton *but, gpointer data)
{
	int bool1  = gtk_toggle_button_get_active(but);
	cfg_set_single_value_as_int(config, "connection","stop-on-exit", bool1);
}
void hide_on_close_enable_tb(GtkToggleButton *but)
{
	int bool1  = gtk_toggle_button_get_active(but);
	cfg_set_single_value_as_int(config, "playlist","hide-on-close", bool1);
}
void cur_song_center_enable_tb(GtkToggleButton *but)
{
	int bool1  = gtk_toggle_button_get_active(but);
	cfg_set_single_value_as_int(config, "playlist","st_cur_song", bool1);
}
void save_possize_enable_tb(GtkToggleButton *but)
{
	int bool1  = gtk_toggle_button_get_active(but);
	cfg_set_single_value_as_int(config, "playlist","savepossize", bool1);
}
void ck_show_tooltip_enable_tb(GtkToggleButton *but)
{
	int bool1  = gtk_toggle_button_get_active(but);
	cfg_set_single_value_as_int(config, "GmpcTreeView","show-tooltip", bool1);
}


void playlist_pref_destroy(GtkWidget *container)
{
	if(playlist_pref_xml)
	{
		GtkWidget *vbox = glade_xml_get_widget(playlist_pref_xml, "playlist-vbox");
		gtk_container_remove(GTK_CONTAINER(container),vbox);
		g_object_unref(playlist_pref_xml);
		playlist_pref_xml = NULL;
	}
}
void playlist_pref_construct(GtkWidget *container)
{
	gchar *path = gmpc_get_full_glade_path("gmpc.glade");
	playlist_pref_xml = glade_xml_new(path, "playlist-vbox",NULL);

	if(playlist_pref_xml)
	{
		GtkWidget *vbox = glade_xml_get_widget(playlist_pref_xml, "playlist-vbox");
		gtk_toggle_button_set_active(
				GTK_TOGGLE_BUTTON(glade_xml_get_widget(playlist_pref_xml, "ck_ps")),
				cfg_get_single_value_as_int_with_default(config,"playlist", "st_cur_song", 0));
		gtk_toggle_button_set_active(
				GTK_TOGGLE_BUTTON(glade_xml_get_widget(playlist_pref_xml, "ck_possize")),
				cfg_get_single_value_as_int_with_default(config,"playlist", "savepossize", 0));
		gtk_toggle_button_set_active(
				GTK_TOGGLE_BUTTON(glade_xml_get_widget(playlist_pref_xml, "ck_hide_on_close")),      		
				cfg_get_single_value_as_int_with_default(config,"playlist", "hide-on-close", 0));

		gtk_toggle_button_set_active(
				GTK_TOGGLE_BUTTON(glade_xml_get_widget(playlist_pref_xml, "ck_stop_on_exit")),      		
				cfg_get_single_value_as_int_with_default(config,"connection","stop-on-exit", 0));

		gtk_toggle_button_set_active(
				GTK_TOGGLE_BUTTON(glade_xml_get_widget(playlist_pref_xml, "ck_cover_case")),      		
				cfg_get_single_value_as_int_with_default(config,"metaimage", "addcase", FALSE));

		gtk_toggle_button_set_active(
				GTK_TOGGLE_BUTTON(glade_xml_get_widget(playlist_pref_xml, "ck_show_tooltip")),      		
				cfg_get_single_value_as_int_with_default(config,"GmpcTreeView", "show-tooltip", TRUE));


		gtk_container_add(GTK_CONTAINER(container),vbox);
		glade_xml_signal_autoconnect(playlist_pref_xml);
	}
	q_free(path);
}

/* Playlist player */
static void playlist_player_set_song(MpdObj *mi)
{
	char buffer[1024];
	mpd_Song *song = mpd_playlist_get_current_song(mi);
	if(song)
	{
		char *mark =cfg_get_single_value_as_string_with_default(
				config,
				"playlist",
				"player_markup",
				DEFAULT_PLAYLIST_PLAYER_MARKUP);
		/**
		 * Render song markup 
		 */
		mpd_song_markup_escaped(buffer, 1024,mark,song);
		cfg_free_string(mark);
	}
}



/**
 * Menu Callback functions
 */

void playlist_menu_repeat_changed(GtkCheckMenuItem *menu)
{
	int active = gtk_check_menu_item_get_active(menu);
	if(active != mpd_player_get_repeat(connection))
	{
		mpd_player_set_repeat(connection, active);
	}
}
void playlist_menu_random_changed(GtkCheckMenuItem *menu)
{
	int active = gtk_check_menu_item_get_active(menu);
	if(active != mpd_player_get_random(connection))
	{
		mpd_player_set_random(connection, active);
	}
}
/**
 * This is artist image
 * FIXME: Rename
 */
void playlist_menu_cover_image_changed(GtkCheckMenuItem *menu)
{
	int active = gtk_check_menu_item_get_active(menu);
	cfg_set_single_value_as_int(config, "playlist", "cover-image-enable", active);

	gmpc_metaimage_set_is_visible(GMPC_METAIMAGE(glade_xml_get_widget(pl3_xml, "metaimage_artist_art")), active);
}

/***
 * Zooming functions
 */
void playlist_zoom_out()
{
	if((pl3_zoom+1) >= PLAYLIST_ZOOM_LEVELS) return;
	pl3_old_zoom = pl3_zoom;
	pl3_zoom++;
	playlist_zoom_level_changed();
}
void playlist_zoom_in()
{
	if(pl3_zoom <= PLAYLIST_NO_ZOOM) return;
	pl3_old_zoom = pl3_zoom;
	pl3_zoom--;
	playlist_zoom_level_changed();
}

/**
 * FIXME: Needs propper grouping and cleaning up
 */
static void playlist_zoom_level_changed()
{
	if(pl3_old_zoom <= PLAYLIST_SMALL)
	{
		gtk_window_get_size(GTK_WINDOW(glade_xml_get_widget(pl3_xml, "pl3_win")), &pl3_wsize.width, &pl3_wsize.height);
		cfg_set_single_value_as_int(config, "playlist", "width", pl3_wsize.width);
		cfg_set_single_value_as_int(config, "playlist", "height", pl3_wsize.height);
		debug_printf(DEBUG_INFO,"save size: %i %i\n", pl3_wsize.width, pl3_wsize.height);
	} 

    if(pl3_old_zoom == PLAYLIST_MINI && pl3_zoom != PLAYLIST_MINI)
    {
        GtkWidget *box = glade_xml_get_widget(pl3_xml, "pl3_button_control_box");
        GtkWidget *top = glade_xml_get_widget(pl3_xml, "hbox10");
        GtkWidget *vtop = glade_xml_get_widget(pl3_xml, "vbox_playlist_player");
        /* add my own reference */
        g_object_ref(box);
        gtk_container_remove(GTK_CONTAINER(vtop),box);
        gtk_box_pack_end(GTK_BOX(top), box, FALSE, TRUE, 0);
        /* release my reference */
        g_object_unref(box);
        gtk_widget_show(box);
        gmpc_progress_set_hide_text(GMPC_PROGRESS(new_pb), FALSE);

        gmpc_metaimage_set_size(GMPC_METAIMAGE(glade_xml_get_widget(pl3_xml, "metaimage_album_art")), ALBUM_SIZE_LARGE);
        gmpc_metaimage_reload_image(GMPC_METAIMAGE(glade_xml_get_widget(pl3_xml, "metaimage_album_art")));

    }
    if(pl3_old_zoom != PLAYLIST_MINI && pl3_zoom == PLAYLIST_MINI)
    {
        GtkWidget *box = glade_xml_get_widget(pl3_xml, "pl3_button_control_box");
        GtkWidget *top = glade_xml_get_widget(pl3_xml, "hbox10");
        GtkWidget *vtop = glade_xml_get_widget(pl3_xml, "vbox_playlist_player");
        /* add my own reference */
        g_object_ref(box);
        gtk_container_remove(GTK_CONTAINER(top),box);
        gtk_box_pack_end(GTK_BOX(vtop), box, FALSE, TRUE, 3);
        /* release my reference */
        g_object_unref(box);
        gtk_widget_show(box);

        gmpc_progress_set_hide_text(GMPC_PROGRESS(new_pb), TRUE);
        gmpc_metaimage_set_size(GMPC_METAIMAGE(glade_xml_get_widget(pl3_xml, "metaimage_album_art")),ALBUM_SIZE_SMALL);
        gmpc_metaimage_reload_image(GMPC_METAIMAGE(glade_xml_get_widget(pl3_xml, "metaimage_album_art")));

    }

	/* Show full view */
	gtk_widget_show(glade_xml_get_widget(pl3_xml, "hpaned1"));
	gtk_widget_show(glade_xml_get_widget(pl3_xml, "hbox1"));
	gtk_widget_show(glade_xml_get_widget(pl3_xml, "header_box"));
	/** Menu Bar */
	gtk_widget_show(glade_xml_get_widget(pl3_xml, "menubar1"));
	/** BUTTON BOX */
	gtk_widget_show(glade_xml_get_widget(pl3_xml, "pl3_button_control_box"));

	gtk_window_set_resizable(GTK_WINDOW(glade_xml_get_widget(pl3_xml, "pl3_win")), TRUE);
	if(pl3_wsize.width > 0 && pl3_wsize.height >0 && pl3_old_zoom == PLAYLIST_MINI)
	{
		debug_printf(DEBUG_INFO,"restore size %i %i\n",pl3_wsize.width, pl3_wsize.height);
		gtk_window_resize(GTK_WINDOW(glade_xml_get_widget(pl3_xml, "pl3_win")),	pl3_wsize.width, pl3_wsize.height);
	}
	gtk_widget_show(glade_xml_get_widget(pl3_xml, "vbox5"));
	gtk_widget_hide(glade_xml_get_widget(pl3_xml, "bread_crumb"));
	gtk_widget_show(glade_xml_get_widget(pl3_xml, "menu_option"));
	gtk_widget_show(glade_xml_get_widget(pl3_xml, "menu_go"));

	/* Now start hiding */
	switch(pl3_zoom)
	{
		case PLAYLIST_NO_ZOOM:
			break;
		case PLAYLIST_MINI:
			gtk_widget_hide(glade_xml_get_widget(pl3_xml, "hbox1"));
			gtk_widget_hide(glade_xml_get_widget(pl3_xml, "hpaned1"));
			gtk_widget_hide(glade_xml_get_widget(pl3_xml, "menu_option"));
			gtk_widget_hide(glade_xml_get_widget(pl3_xml, "menu_go"));
			if(glade_xml_get_widget(pl3_xml, "pl3_win")->window)
			{
				if(gdk_window_get_state(glade_xml_get_widget(pl3_xml, "pl3_win")->window)&GDK_WINDOW_STATE_MAXIMIZED)
				{
					gtk_window_unmaximize(GTK_WINDOW(glade_xml_get_widget(pl3_xml, "pl3_win")));
				}

				if(gdk_window_get_state(glade_xml_get_widget(pl3_xml, "pl3_win")->window)&GDK_WINDOW_STATE_FULLSCREEN)
				{
					gtk_window_unfullscreen(GTK_WINDOW(glade_xml_get_widget(pl3_xml, "pl3_win")));
				}
			}
			gtk_window_set_resizable(GTK_WINDOW(glade_xml_get_widget(pl3_xml, "pl3_win")), FALSE);

		case PLAYLIST_SMALL:
			gtk_widget_hide(glade_xml_get_widget(pl3_xml, "vbox5"));
			gtk_widget_show(glade_xml_get_widget(pl3_xml, "bread_crumb"));

			gtk_widget_grab_focus(glade_xml_get_widget(pl3_xml, "pl3_win"));
		default:
			break;
	}
	/** Save zoom level
	*/
	cfg_set_single_value_as_int(config, "playlist","zoomlevel",pl3_zoom);
}

/***
 * Handle a connect/Disconnect
 */
static void playlist_connection_changed(MpdObj *mi, int connect, gpointer data)
{
	/* Set menu items */
	if(connect) {
		gtk_widget_set_sensitive(glade_xml_get_widget(pl3_xml, "vbox_playlist_player"), TRUE);
		gtk_widget_set_sensitive(glade_xml_get_widget(pl3_xml, "hpaned1"), TRUE);

		gtk_widget_set_sensitive(glade_xml_get_widget(pl3_xml, "menu_connect"), FALSE);
		gtk_widget_set_sensitive(glade_xml_get_widget(pl3_xml, "menu_disconnect"), TRUE);
		gtk_widget_set_sensitive(glade_xml_get_widget(pl3_xml, "menuitem_sendpassword"), TRUE);
		gtk_widget_set_sensitive(glade_xml_get_widget(pl3_xml, "view1"), TRUE);
		gtk_widget_set_sensitive(glade_xml_get_widget(pl3_xml, "menu_option"), TRUE);


		pl3_push_rsb_message(_("Connected"));	
	} else {
		gtk_widget_set_sensitive(glade_xml_get_widget(pl3_xml, "vbox_playlist_player"), FALSE);
		gtk_widget_set_sensitive(glade_xml_get_widget(pl3_xml, "hpaned1"), FALSE);             		
		gtk_widget_set_sensitive(glade_xml_get_widget(pl3_xml, "menu_connect"), TRUE);
		gtk_widget_set_sensitive(glade_xml_get_widget(pl3_xml, "menu_disconnect"), FALSE);
		gtk_widget_set_sensitive(glade_xml_get_widget(pl3_xml, "menuitem_sendpassword"),FALSE);
		gtk_widget_set_sensitive(glade_xml_get_widget(pl3_xml, "view1"), FALSE);
		gtk_widget_set_sensitive(glade_xml_get_widget(pl3_xml, "menu_option"), FALSE);
		pl3_push_rsb_message(_("Not Connected"));
	}
    /** Set back to the current borwser, and update window title */
	if(connect){
		gchar *string = NULL;
        GtkTreeIter iter;
        GtkTreeSelection *selec = gtk_tree_view_get_selection((GtkTreeView *)
				glade_xml_get_widget (pl3_xml, "cat_tree"));
		GtkTreeModel *model = GTK_TREE_MODEL(pl3_tree);                      		
		if(gtk_tree_model_get_iter_first(model, &iter)){
			gtk_tree_selection_select_iter(selec, &iter);
		}
		string = g_strdup_printf("%s - %s %s",
				_("GMPC"), 
				_("Connected to"),
				mpd_get_hostname(mi));
		gtk_window_set_title(GTK_WINDOW(glade_xml_get_widget(pl3_xml, "pl3_win")), string);
		q_free(string);
	}
	else{
		gchar *string = NULL;
		string = g_strdup_printf("%s - %s",
				_("GMPC"), 
				_("Disconnected"));
		gtk_window_set_title(GTK_WINDOW(glade_xml_get_widget(pl3_xml, "pl3_win")), string);		
		q_free(string);                                                                    	
	}

	/*
	 * make the playlist update itself
	 */	
	playlist_status_changed(connection, 
			MPD_CST_STATE|MPD_CST_SONGID|MPD_CST_ELAPSED_TIME|MPD_CST_VOLUME|MPD_CST_REPEAT|MPD_CST_RANDOM|MPD_CST_PERMISSION,
			NULL);


	/**
	 * Also need updating
	 */
	pl3_option_menu_activate();

	playlist3_fill_server_menu();

	/**
	 * update interface
	 * items that are caused by the plugin.
	 */
	pl3_plugin_changed_interface();

}
/**
 * Update the window to status changes in mpd
 */
void playlist_status_changed(MpdObj *mi, ChangedStatusType what, void *userdata)
{
	char buffer[1024];
	GtkWidget *image = NULL;
	/**
	 * if the window isn't there yet, return
	 */
	if(!pl3_xml)return;

	/**
	 * Player state changed
	 */
	if(what&MPD_CST_STATE)
	{
		int state = mpd_player_get_state(mi);
		switch(state){
			case MPD_PLAYER_PLAY:
                {
                    gchar *markup = cfg_get_single_value_as_string_with_default(config, 
                            "playlist",                             /* Category */
                            "window-markup",                        /* Key */
                            "[%title% - &[%artist%]]|%shortfile%"   /* default value */
                            );
                    /**
                     * Update the image in the menu
                     */
                    image = gtk_image_menu_item_get_image(GTK_IMAGE_MENU_ITEM(glade_xml_get_widget(pl3_xml, "menu_play")));
                    gtk_image_set_from_stock(GTK_IMAGE(image), "gtk-media-pause", GTK_ICON_SIZE_MENU);
                    gtk_image_set_from_stock(GTK_IMAGE(glade_xml_get_widget(pl3_xml, "pp_but_play_img")),
                            "gtk-media-pause",GTK_ICON_SIZE_BUTTON);

                    /**
                     * Update song indicator in window 
                     */	
                    playlist_player_set_song(mi);

                    /**
                     * Update window title
                     */
                    mpd_song_markup(buffer,1024,markup, mpd_playlist_get_current_song(connection));
                    gtk_window_set_title(GTK_WINDOW(glade_xml_get_widget(pl3_xml, "pl3_win")), buffer);		

                    g_free(markup);
                    break;
                }
			case MPD_PLAYER_PAUSE:
				/** Update menu and button images */
				image = gtk_image_menu_item_get_image(GTK_IMAGE_MENU_ITEM(glade_xml_get_widget(pl3_xml, "menu_play")));
				gtk_image_set_from_stock(GTK_IMAGE(image), "gtk-media-play", GTK_ICON_SIZE_MENU);                     				
				gtk_image_set_from_stock(GTK_IMAGE(
							glade_xml_get_widget(pl3_xml, "pp_but_play_img")),
						"gtk-media-play",GTK_ICON_SIZE_BUTTON);

				/**
				 * Set paused in Window string 
				 */
				mpd_song_markup(buffer, 1024,"[%title% - &[%artist%] (paused)]|%shortfile% (paused)", mpd_playlist_get_current_song(connection));
				gtk_window_set_title(GTK_WINDOW(glade_xml_get_widget(pl3_xml, "pl3_win")), buffer);		
				break;
			default:
				image = gtk_image_menu_item_get_image(GTK_IMAGE_MENU_ITEM(glade_xml_get_widget(pl3_xml, "menu_play")));
				gtk_image_set_from_stock(GTK_IMAGE(image), "gtk-media-play", GTK_ICON_SIZE_MENU);
				/* Make sure it's reset correctly */
                gmpc_progress_set_time(GMPC_PROGRESS(new_pb), 0, 0); 

				gtk_image_set_from_stock(GTK_IMAGE(
							glade_xml_get_widget(pl3_xml, "pp_but_play_img")),
						"gtk-media-play",GTK_ICON_SIZE_BUTTON);

				gtk_window_set_title(GTK_WINDOW(glade_xml_get_widget(pl3_xml, "pl3_win")), _("GMPC"));		
		}
		playlist3_update_header();
	}
	/**
	 * Handle song change or Playlist change
	 * Anything that can change metadta
	 */
	if(what&MPD_CST_SONGID || what&MPD_CST_SONGPOS || what&MPD_CST_PLAYLIST)
	{
		playlist3_update_header();
		if(mpd_player_get_state(mi) == MPD_PLAYER_PLAY)
		{
			playlist_player_set_song(mi);
		}
		/* make is update markups and stuff */
		playlist_status_changed(mi, MPD_CST_STATE,NULL);
	}
	/**
	 * set repeat buttons in menu correct
	 */
	if(what&MPD_CST_REPEAT)
	{
		if(mpd_check_connected(connection))
		{
			char *string = g_strdup_printf(_("Repeat: %s"), (mpd_player_get_repeat(connection))? _("On"):_("Off"));
			pl3_push_statusbar_message(string);
			q_free(string);

			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(glade_xml_get_widget(pl3_xml, "menu_repeat")),
					mpd_player_get_repeat(connection));
		}

	}
	if(what&MPD_CST_RANDOM)
	{
		if(mpd_check_connected(connection))
		{
			char *string = g_strdup_printf(_("Random: %s"), (mpd_player_get_random(connection))? _("On"):_("Off"));
			pl3_push_statusbar_message(string);
			q_free(string);

			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(glade_xml_get_widget(pl3_xml, "menu_random")),
					mpd_player_get_random(connection));
		}
	}                                                                                                        	
	if(what&MPD_CST_ELAPSED_TIME)
	{
		if(mpd_check_connected(connection))
        {
            int totalTime = mpd_status_get_total_song_time(connection);
            int elapsedTime = mpd_status_get_elapsed_song_time(connection);			
            gmpc_progress_set_time(GMPC_PROGRESS(new_pb), totalTime, elapsedTime); 
        }
		else
		{

            gmpc_progress_set_time(GMPC_PROGRESS(new_pb), 0, 0); 
        }
	}
	if(what&MPD_CST_VOLUME)
	{
        int volume = gtk_scale_button_get_value(GTK_SCALE_BUTTON(volume_slider))*100;
        /* don't do anything if nothing is changed */
        if(mpd_status_get_volume(connection) != volume)
        {
            gtk_scale_button_set_value(GTK_SCALE_BUTTON(volume_slider),
                    mpd_status_get_volume(connection)/100.0);
        }

	}
	if(what&MPD_CST_PERMISSION)
	{
		/* Check for control */
		if(mpd_server_check_command_allowed(connection,"play") == MPD_SERVER_COMMAND_ALLOWED &&
				mpd_check_connected(connection))
		{
			gtk_widget_set_sensitive(glade_xml_get_widget(pl3_xml, "pl3_button_control_box"),TRUE);
			gtk_widget_set_sensitive(glade_xml_get_widget(pl3_xml, "menuitem_control"),TRUE);
		}
		else
		{
			gtk_widget_set_sensitive(glade_xml_get_widget(pl3_xml, "pl3_button_control_box"),FALSE);
			gtk_widget_set_sensitive(glade_xml_get_widget(pl3_xml, "menuitem_control"),FALSE);
		}
	}

	if(what&MPD_CST_UPDATING)
	{
		pl3_updating_changed(connection, mpd_status_db_is_updating(connection));
	}
    if(what&MPD_CST_SERVER_ERROR)
    {
        gchar *error = mpd_status_get_mpd_error(mi);
        if(error)
        {
            gchar *mes = g_markup_printf_escaped("%s: '%s'", _("MPD Reported the following error"), error);
            playlist3_show_error_message(mes, ERROR_WARNING);
            q_free(mes);
            q_free(error);
        }
    }
    if(what&MPD_CST_OUTPUT)
    {
        playlist3_fill_server_menu();
    }
}

void playlist_player_cover_art_pressed(GtkEventBox *event_widget, GdkEventButton *event)
{
	mpd_Song *song = NULL;
	int state = mpd_player_get_state(connection);
	if(state == MPD_PLAYER_STOP || state == MPD_PLAYER_UNKNOWN) return;
	if(!mpd_check_connected(connection)) return;
	song = mpd_playlist_get_current_song(connection);
	if(song)
	{
		info2_activate();
		info2_fill_song_view(song);	
	}
}

static void playlist_player_volume_changed(GtkWidget *vol_but)
{
	int volume = gtk_scale_button_get_value(GTK_SCALE_BUTTON(vol_but))*100;
	if(mpd_status_get_volume(connection) != volume)
	{
		mpd_status_set_volume(connection, volume);
	}
}





void about_window()
{
	gchar *path = gmpc_get_full_glade_path("gmpc.glade");
	GladeXML *diagxml = glade_xml_new(path, "aboutdialog",NULL);
	GtkWidget *dialog = glade_xml_get_widget(diagxml, "aboutdialog");
	gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(glade_xml_get_widget(pl3_xml, "pl3_win")));

	q_free(path);

	if(strlen(revision))
	{
		path = g_strdup_printf("%s\nRevision: %s", VERSION, revision);
	}
	else
	{
		path = g_strdup_printf("%s\n%s\n",VERSION, GMPC_TAGLINE);
	}
    gtk_about_dialog_set_copyright(GTK_ABOUT_DIALOG(dialog), GMPC_COPYRIGHT);
	gtk_about_dialog_set_website(GTK_ABOUT_DIALOG(dialog), GMPC_WEBSITE);
	gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(dialog),path); 

	q_free(path);
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
	g_object_unref(diagxml);
}


/****************************************************
 * Interface stuff
 */
void pl3_update_go_menu()
{
	int i=0;
	int items = 0;
	GtkWidget *menu = NULL;
    GtkAccelGroup *group = gtk_accel_group_new();
	/***
	 * Remove any old menu
	 */
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(glade_xml_get_widget(pl3_xml, "menu_go")), NULL);
	/**
	 * Create a new menu
	 */
	menu = gtk_menu_new();
    gtk_menu_set_accel_group(GTK_MENU(menu), group);
	g_object_unref(group);
    gtk_window_add_accel_group(GTK_WINDOW(glade_xml_get_widget(pl3_xml, "pl3_win")), group);
	if(mpd_check_connected(connection)) {
		for(i=0; i< num_plugins;i++) {
			if(plugins[i]->plugin_type&GMPC_PLUGIN_PL_BROWSER) {                                                   
				if(plugins[i]->browser && plugins[i]->browser->add_go_menu) {
					items += plugins[i]->browser->add_go_menu(menu);
				}                                                                           	
			}
		}
	}

	/**
	 * Attach menu
	 */
	if(items) {
		gtk_widget_show_all(menu);
		gtk_menu_item_set_submenu(GTK_MENU_ITEM(glade_xml_get_widget(pl3_xml, "menu_go")),
				menu);
		gtk_widget_set_sensitive(glade_xml_get_widget(pl3_xml, "menu_go"), TRUE);
	} else {
		gtk_widget_set_sensitive(glade_xml_get_widget(pl3_xml, "menu_go"), FALSE);
		gtk_widget_destroy(menu);
	}
}

/**
 * This function should be called by a plugin when something in the interface changed.
 */
static void pl3_plugin_changed_interface()
{
	/**
	 * Call this at the end, to update the menu
	 */
	pl3_update_go_menu();
}
static void pl3_profile_selected(GtkRadioMenuItem *radio,gpointer data)
{
	if(gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(radio)))
	{
		gchar *uid = g_object_get_data(G_OBJECT(radio), "uid");
		if(!uid)
		{
			return;
		}
		connection_set_current_profile(uid);
		if(mpd_check_connected(connection))
		{
			mpd_disconnect(connection);			
			connect_to_mpd();
		}
	}
}
static void pl3_profiles_changed(GmpcProfiles *prof,const int changed, const int col, const gchar *id)
{
	if(changed == PROFILE_ADDED)
	{
		gchar *message = g_strdup_printf("%s '%s' %s  ", _("Profile"), gmpc_profiles_get_name(prof,id), _("added"));
		pl3_push_statusbar_message(message);
		g_free(message);
	}
	else if(changed == PROFILE_COL_CHANGED && col == PROFILE_COL_HOSTNAME)
	{
		gchar *message = g_strdup_printf("%s '%s' %s %s", _("Profile"), gmpc_profiles_get_name(prof,id), _("changed hostname to:"), gmpc_profiles_get_hostname(prof,id));
		pl3_push_statusbar_message(message);
		g_free(message);
	}

}
static void pl3_update_profiles_menu(GmpcProfiles *prof,const int changed, const int col, const gchar *id)
{
	int items = 0;
	GtkWidget *menu = NULL;
	gchar *current = gmpc_profiles_get_current(gmpc_profiles);
	GList *iter, *mult;
	/* check if there is anything changed that is important for us. */
	if(changed == PROFILE_COL_CHANGED && col != PROFILE_COL_NAME)
	{
		q_free(current);
		return;
	}
	/***
	 * Remove any old menu
	 */
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(glade_xml_get_widget(pl3_xml, "menu_profiles")),NULL);
	/**
	 * Create a new menu
	 */
	menu = gtk_menu_new();

	mult = gmpc_profiles_get_profiles_ids(gmpc_profiles); 
	if(mult)
	{
		GSList *group = NULL;
		iter = mult;
		do{
			/** Get profile name */
			const gchar *value = gmpc_profiles_get_name(gmpc_profiles, (char *)iter->data); 
			GtkWidget *item	= gtk_radio_menu_item_new_with_label(group,value);
			/* get new group */
			group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(item));	
			/* add to the menu */
			gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

			/* check the current profile */
			if(!strcmp((char *)(iter->data), current))
			{
				gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item), TRUE);
			}

			/**
			 * Attach click handler
			 */
			g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(pl3_profile_selected), NULL);

			/** Attach the uid to the handler */
			value = g_strdup((char *)(iter->data));
			g_object_set_data_full(G_OBJECT(item), "uid", (gpointer)value, g_free);

			items++;
		}while((iter = g_list_next(iter)));
		g_list_foreach(mult, (GFunc)g_free, NULL);
		g_list_free(mult);


	}

	/**
	 * Attach menu
	 */
	if(items) {
		gtk_widget_show_all(menu);
		gtk_menu_item_set_submenu(GTK_MENU_ITEM(glade_xml_get_widget(pl3_xml, "menu_profiles")),
				menu);
		gtk_widget_set_sensitive(glade_xml_get_widget(pl3_xml, "menu_profiles"), TRUE);
	} else {
		gtk_widget_set_sensitive(glade_xml_get_widget(pl3_xml, "menu_profiles"), FALSE);
		gtk_widget_destroy(menu);
	}
	g_free(current);
}

static void playlist3_server_output_changed(GtkWidget *item, gpointer data)
{
	int id = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(item), "id"));
	int state = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(item));
	mpd_server_set_output_device(connection, id, state);

}
static void playlist3_server_update_db(void)
{
	
	mpd_database_update_dir(connection, "/");
}

static void playlist3_fill_server_menu(void)
{
	/** Clear old items */
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(glade_xml_get_widget(pl3_xml, "menuitem_server")),NULL);
	
	/* if connected fill with items */
	if(mpd_check_connected(connection))
	{
		GtkWidget *menu = gtk_menu_new();
  	GtkWidget *menu_item = NULL;
    GtkAccelGroup *group = gtk_accel_group_new();
    int i = 0;
		MpdData *data= NULL;
    gtk_menu_set_accel_group(GTK_MENU(menu), group);
    /* todo, does this needs to be removed, or does that go automatically when the accell group get destroyed?  */
    gtk_window_add_accel_group(GTK_WINDOW(glade_xml_get_widget(pl3_xml, "pl3_win")), group);
    
		/* Update DB */
		menu_item = gtk_image_menu_item_new_with_label(_("Update Database"));
		gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menu_item),
							gtk_image_new_from_stock(GTK_STOCK_REFRESH, GTK_ICON_SIZE_MENU));
		g_signal_connect(G_OBJECT(menu_item), "activate", G_CALLBACK(playlist3_server_update_db), NULL);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

		menu_item = gtk_separator_menu_item_new();
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

		data = mpd_server_get_output_devices(connection);
		for(;data;data = mpd_data_get_next(data))
		{
			menu_item = gtk_check_menu_item_new_with_label(data->output_dev->name);
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item), data->output_dev->enabled?TRUE:FALSE);
			gtk_widget_add_accelerator(menu_item, "activate", group, GDK_1+i, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

			g_signal_connect(G_OBJECT(menu_item), "toggled", G_CALLBACK(playlist3_server_output_changed),NULL);
			g_object_set_data(G_OBJECT(menu_item), "id", GINT_TO_POINTER(data->output_dev->id));
			gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
      i++;
		}
		gtk_menu_item_set_submenu(GTK_MENU_ITEM(glade_xml_get_widget(pl3_xml, "menuitem_server")),menu);
		gtk_widget_show_all(menu);
		/* Server Menu Item */
		gtk_widget_set_sensitive(glade_xml_get_widget(pl3_xml, "menuitem_server"), TRUE);
	}
	else
	{
		/* Server Menu Item */
		gtk_widget_set_sensitive(glade_xml_get_widget(pl3_xml, "menuitem_server"), FALSE);
	}
}
/**
 * new header 
 */

static void playlist3_header_song(void)
{
	mpd_Song *song = mpd_playlist_get_current_song(connection);
	if(song)
	{
		info2_activate();
		info2_fill_song_view(song);	
	}
}
static void playlist3_header_artist(void)
{
	mpd_Song *song = mpd_playlist_get_current_song(connection);
	if(song && song->artist)
	{
		info2_activate();
		info2_fill_artist_view(song->artist);
	}
}
static void playlist3_header_album(void)
{
	mpd_Song *song = mpd_playlist_get_current_song(connection);
	if(song && song->artist && song->album)
	{
		info2_activate();
		info2_fill_album_view(song->artist,song->album);
	}
}


void playlist3_new_header(void)
{
	GtkWidget *hbox10 = glade_xml_get_widget(pl3_xml, "header_box");
	if(hbox10)
	{
		GtkWidget *hbox = gtk_hbox_new(FALSE, 6);
		GtkWidget *vbox = gtk_vbox_new(FALSE, 0);

        gtk_widget_set_size_request(hbox, 250,-1);
		/** Title */
		header_labels[0] = gmpc_clicklabel_new("");
		gmpc_clicklabel_font_size(GMPC_CLICKLABEL(header_labels[0]),4);
		gmpc_clicklabel_set_do_bold(GMPC_CLICKLABEL(header_labels[0]),FALSE);

		header_labels[1] = gtk_label_new(_("By"));
		/** Artist */
		header_labels[2] = gmpc_clicklabel_new("");
		gmpc_clicklabel_set_ellipsize(GMPC_CLICKLABEL(header_labels[2]),PANGO_ELLIPSIZE_NONE);
		gmpc_clicklabel_set_do_bold(GMPC_CLICKLABEL(header_labels[2]),FALSE);
		gmpc_clicklabel_set_do_italic(GMPC_CLICKLABEL(header_labels[2]),TRUE);

		header_labels[3] = gtk_label_new(_("From"));
		/** Albumr */
		header_labels[4] = gmpc_clicklabel_new("");
		gmpc_clicklabel_set_do_bold(GMPC_CLICKLABEL(header_labels[4]),FALSE);
		gmpc_clicklabel_set_do_italic(GMPC_CLICKLABEL(header_labels[4]),TRUE);

		gtk_box_pack_start(GTK_BOX(vbox), header_labels[0], FALSE, TRUE,0);
		gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE,0);

		gtk_box_pack_start(GTK_BOX(hbox), header_labels[1], FALSE, TRUE,0);
		gtk_box_pack_start(GTK_BOX(hbox), header_labels[2], FALSE, TRUE,0);
		gtk_box_pack_start(GTK_BOX(hbox), header_labels[3], FALSE, TRUE,0);
		gtk_box_pack_start(GTK_BOX(hbox), header_labels[4], TRUE,  TRUE,0);

		g_signal_connect(G_OBJECT(header_labels[0]), "button-press-event", G_CALLBACK(playlist3_header_song), NULL);
		g_signal_connect(G_OBJECT(header_labels[2]), "button-press-event", G_CALLBACK(playlist3_header_artist), NULL);
		g_signal_connect(G_OBJECT(header_labels[4]), "button-press-event", G_CALLBACK(playlist3_header_album), NULL);

		gtk_container_add(GTK_CONTAINER(hbox10), vbox);
		gtk_widget_show_all(hbox10);
	}
}

void playlist3_update_header(void)
{
	if(header_labels[0] != NULL)
	{
		char buffer[1024];
		if(mpd_check_connected(connection))
		{
			mpd_Song *song = mpd_playlist_get_current_song(connection);
			/** Set new header */
			if(mpd_player_get_state(connection) != MPD_STATUS_STATE_STOP && song){

				mpd_song_markup(buffer, 1024,"[%title%|%shortfile%][ (%name%)]",song);
				gmpc_clicklabel_set_text(GMPC_CLICKLABEL(header_labels[0]),buffer);
				if(song->artist) {
					gtk_widget_show(header_labels[1]);
					gtk_widget_show(header_labels[2]);
					gmpc_clicklabel_set_text(GMPC_CLICKLABEL(header_labels[2]),song->artist);
				} else {
					gtk_widget_hide(header_labels[1]);
					gtk_widget_hide(header_labels[2]);
				}
				if(song->album) {
					gtk_widget_show(header_labels[3]);
					gtk_widget_show(header_labels[4]);
					gmpc_clicklabel_set_text(GMPC_CLICKLABEL(header_labels[4]),song->album);
				} else {
					gtk_widget_hide(header_labels[3]);
					gtk_widget_hide(header_labels[4]);
				}

			}
			else
			{
				gmpc_clicklabel_set_text(GMPC_CLICKLABEL(header_labels[0]),_("Not Playing"));
				gtk_widget_hide(header_labels[1]);
				gtk_widget_hide(header_labels[2]);
				gtk_widget_hide(header_labels[3]);
				gtk_widget_hide(header_labels[4]);
			}
		}
		else 
		{
			gmpc_clicklabel_set_text(GMPC_CLICKLABEL(header_labels[0]),_("Not Connected"));
			gtk_widget_hide(header_labels[1]);
			gtk_widget_hide(header_labels[2]);
			gtk_widget_hide(header_labels[3]);
			gtk_widget_hide(header_labels[4]);

		}
	}
}



static gboolean playlist3_error_expose(GtkWidget *wid, GdkEventExpose *event, gpointer data)
{
	int width = wid->allocation.width;
	int height = wid->allocation.height;
	cairo_t *cr = gdk_cairo_create(wid->window);

	cairo_set_line_width (cr, 1.0);
	cairo_rectangle(cr, 0,0,width,height);
	cairo_close_path (cr);                                                    	
	gdk_cairo_set_source_color(cr, 	&(wid->style->mid[GTK_STATE_NORMAL]));
	cairo_fill_preserve(cr);                                                 
	gdk_cairo_set_source_color(cr, 	&(wid->style->black));
	cairo_stroke (cr);
	cairo_destroy(cr);
	return FALSE;
}

void playlist3_insert_browser(GtkTreeIter *iter, gint position)
{
	GtkTreeIter it,*sib= NULL;
	gint pos=0;
	GtkTreeModel *model = GTK_TREE_MODEL(pl3_tree); 
	if(gtk_tree_model_get_iter_first(model, &it))
	{

		do{
			gtk_tree_model_get(model, &it, PL3_CAT_ORDER, &pos, -1); 
			if(position <= pos)
				sib = &it;
		}while(sib == NULL && gtk_tree_model_iter_next(model, &it));
	}
	gtk_list_store_insert_before(GTK_LIST_STORE(pl3_tree), iter, sib);
	gtk_list_store_set(GTK_LIST_STORE(pl3_tree), iter, PL3_CAT_ORDER, position, -1);
}


/**
 * Category editing
 */


void set_browser_format(void)
{
    char *string = gmpc_signals_get_browser_markup(gmpc_signals); 
    char *format = edit_song_markup(string);
    cfg_free_string(string);
    if(format != NULL)
    {
        gmpc_signals_browser_markup_changed(gmpc_signals, format); 
    }
    q_free(format);
}

void playlist3_destroy(void)
{
	GtkWidget *win = glade_xml_get_widget(pl3_xml, "pl3_win");
    gtk_widget_destroy(win);
    g_object_unref(pl3_xml);
}

gboolean playlist3_show_playtime(gulong playtime)
{
     if(playtime) {
         gchar *string = format_time(playtime);
         pl3_push_rsb_message(string);
         q_free(string);
     } else {
         pl3_push_rsb_message("");
     }
     return FALSE;
}


/***
 * Help menu
 */

void url_visit_website(void);
void url_getting_help(void);

void url_visit_website(void)
{
	open_uri(GMPC_WEBSITE);
}

void url_getting_help(void)
{
	open_uri(GMPC_BUGTRACKER);
}
