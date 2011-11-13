/* Gnome Music Player Client (GMPC)
 * Copyright (C) 2004-2011 Qball Cow <qball@gmpclient.org>
 * Project homepage: http://gmpclient.org/

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
#include "plugin.h"
#include "misc.h"
#include "playlist3.h"
#include "preferences.h"
#include "revision.h"
#include "gmpc-metaimage.h"
#include "gmpc-extras.h"
#include "GUI/cmd.h"
#include "GUI/status_icon.h"
#include "GUI/title_header.h"
#include "GUI/control_window.h"

// Collapsed mode.
#define SIDEBAR_SMALL 32
// Default size.
#define SIDEBAR_LARGE -1 

#ifndef GDK_KEY_0
#define GDK_KEY_0 GDK_0
#endif

#ifndef GDK_KEY_1
#define GDK_KEY_1 GDK_1
#endif

#ifndef GDK_KEY_2
#define GDK_KEY_2 GDK_2
#endif


#ifndef GDK_KEY_3
#define GDK_KEY_3 GDK_3
#endif

#ifndef GDK_KEY_4
#define GDK_KEY_4 GDK_4
#endif


#ifndef GDK_KEY_5
#define GDK_KEY_5 GDK_5
#endif


#ifndef GDK_KEY_6
#define GDK_KEY_6 GDK_6
#endif


#ifndef GDK_KEY_7
#define GDK_KEY_7 GDK_7
#endif


#ifndef GDK_KEY_8
#define GDK_KEY_8 GDK_8
#endif


#ifndef GDK_KEY_9
#define GDK_KEY_9 GDK_9
#endif



#define ALBUM_SIZE_SMALL 42
#define ALBUM_SIZE_LARGE 42

#define LOG_DOMAIN "Playlist"

/* Drag and drop Target table */
static GtkTargetEntry target_table[] =
{
    {(gchar *) "x-url/http", 0, 0},
    {(gchar *) "_NETSCAPE_URL", 0, 1},
    {(gchar *) "text/uri-list", 0, 2},
    {(gchar *) "audio/*", 0, 3},
    {(gchar *) "audio/x-scpls", 0, 4},
    {(gchar *) "internal-drop", 0, 99}
};

GtkWidget *metaimage_album_art = NULL;
GtkWidget *metaimage_artist_art = NULL;
GmpcFavoritesButton *favorites_button = NULL;

GtkCellRenderer *sidebar_text = NULL;
/**
 * Widgets used in the header.
 * and the new progresbar
 */
static GtkWidget *new_pb = NULL;

/**
 * Indicates the zoom level and the previous zoom level.
 */
int pl3_zoom = PLAYLIST_NO_ZOOM;
int pl3_old_zoom = PLAYLIST_NO_ZOOM;

static void playlist_zoom_level_changed(void);
void playlist_zoom_in(void);
void playlist_zoom_out(void);


void pl3_pb_seek_event(GtkWidget * pb, guint seek_time, gpointer user_data);

void set_browser_format(void);
void set_playlist_format(void);

gboolean playlist_player_volume_changed(GtkWidget * vol_but, int new_vol);

/* Glade declarations, otherwise these would be static */
void about_window(void);
int pl3_cat_tree_button_press_event(GtkTreeView *, GdkEventButton *);
int pl3_cat_tree_button_release_event(GtkTreeView *, GdkEventButton *);

void cur_song_center_enable_tb(GtkToggleButton *);
void show_cover_case_tb(GtkToggleButton * but);
void save_possize_enable_tb(GtkToggleButton *);
void playlist_menu_repeat_changed(GtkToggleAction *);
void playlist_menu_single_mode_changed(GtkToggleAction * );
void playlist_menu_consume_changed(GtkToggleAction * );

void playlist_menu_random_changed(GtkToggleAction *);
void playlist_menu_artist_image_changed(GtkToggleAction *);
void hide_on_close_enable_tb(GtkToggleButton * but);
gboolean pl3_close(void);
static void pl3_update_profiles_menu(GmpcProfiles * prof, const int changed, const int col, const gchar * id);
gboolean playlist3_enter_notify_event(GtkWidget * wid, GdkEventCrossing * event, gpointer data);
gboolean playlist3_leave_notify_event(GtkWidget * wid, GdkEventCrossing * event, gpointer data);
gboolean pl3_window_focus_out_event(GtkWidget *window, GdkEventFocus *event, gpointer data);

static void pl3_profiles_changed(GmpcProfiles * prof, const int changed, const int col, const gchar * id);
static void playlist3_server_output_changed(GtkWidget * item, gpointer data);
static void playlist3_fill_server_menu(void);
void playlist3_server_update_db(void);

void easy_command_help_window(void);

void pl3_style_set_event(GtkWidget *widget, GtkStyle *previous_style, gpointer user_data);

void pl3_sidebar_plugins_init(void);

/* Old category browser style */
static int old_type = -1;

/* interface description */
GtkBuilder *pl3_xml = NULL;
/* category treeview-store */
GtkTreeModel *pl3_tree = NULL;

/* size */
static GtkAllocation pl3_wsize = { 0, 0, 0, 0 };

static int pl3_hidden = TRUE;

static void playlist_status_changed(MpdObj * mi, ChangedStatusType what, void *userdata);

/* Playlist "Plugin" */
static void playlist_pref_construct(GtkWidget * container);
static void playlist_pref_destroy(GtkWidget * container);

static GtkBuilder *playlist_pref_xml = NULL;

void ck_search_as_you_type(GtkToggleButton * but);


/* Get the type of the selected row..
 * -1 means no row selected
 */
int pl3_cat_get_selected_browser(void)
{
    return old_type;
}




/**
 * Extras for better integration
 */

void init_extra_playlist_state(void);
void enable_extra_playlist(GtkToggleAction *action);

/**************************************************
 * Category Tree
 */
static void pl3_initialize_tree(void)
{
    int i;
    GtkTreePath *path;
    GtkTreeSelection *sel;
	GtkWidget *cat_tree = GTK_WIDGET(gtk_builder_get_object(pl3_xml, "cat_tree"));

	INIT_TIC_TAC()

    path = gtk_tree_path_new_from_string("0");
    sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(gtk_builder_get_object(pl3_xml, "cat_tree")));
	TEC("Get selection");
    for (i = 0; i < num_plugins; i++)
    {
        if (gmpc_plugin_is_browser(plugins[i]))
        {
            if (gmpc_plugin_get_enabled(plugins[i]))
            {
                gmpc_plugin_browser_add(plugins[i], cat_tree);
            }
        }
		TEC("setup %s", gmpc_plugin_get_name(plugins[i]))
	}

    gtk_tree_view_set_cursor(GTK_TREE_VIEW(gtk_builder_get_object(pl3_xml, "cat_tree")), path, NULL, FALSE);
	TEC("set cursor");
    gtk_tree_path_free(path);
	TEC("finish set");
}


/**
 * Function to handle a change in category.
 */
static void pl3_cat_sel_changed(GtkTreeSelection * selec, gpointer * userdata)
{
    GtkTreeView *tree = (GtkTreeView *) gtk_builder_get_object(pl3_xml, "cat_tree");
    GtkTreeModel *model = gtk_tree_view_get_model(tree);
    GtkTreeIter iter;
    GtkWidget *container = GTK_WIDGET(gtk_builder_get_object(pl3_xml, "browser_container"));
    if (!model)
        return;
    if (gtk_tree_selection_get_selected(selec, &model, &iter))
    {
        gint type;

        gtk_tree_model_get(model, &iter, PL3_CAT_TYPE, &type, -1);


        /**
         * Start switching side view (if type changed )
         */
        if (old_type != -1)
        {
            gmpc_plugin_browser_unselected(plugins[plugin_get_pos(old_type)], container);
        }
        old_type = -1;
		if(type > -1)
		{
			/** if type changed give a selected signal */
			if ((old_type != type))
			{
				gmpc_plugin_browser_selected(plugins[plugin_get_pos(type)], container);

			}
		}
        /**
         * update old value, so get_selected_category is correct before calling selection_changed
         */
        old_type = type;

    } else
    {
        if (old_type != -1)
        {
            gmpc_plugin_browser_unselected(plugins[plugin_get_pos(old_type)], container);
        }
        old_type = -1;
        gtk_tree_model_get_iter_first(model, &iter);
        gtk_tree_selection_select_iter(selec, &iter);
    }
    pl3_option_menu_activate();
}


/* handle right mouse clicks on the cat tree view */
/* gonna be a big function*/
int pl3_cat_tree_button_press_event(GtkTreeView * tree, GdkEventButton * event)
{
    GtkTreeSelection *sel = gtk_tree_view_get_selection(tree);
    if (event->button != 3 || gtk_tree_selection_count_selected_rows(sel) < 2 || !mpd_check_connected(connection))
    {
        return FALSE;
    }
    return TRUE;

}


void pl3_option_menu_activate(void)
{

    GtkWidget *tree = GTK_WIDGET(gtk_builder_get_object(pl3_xml, "cat_tree"));
    int i;
    gint type = pl3_cat_get_selected_browser();
    int menu_items = 0;
    GdkEventButton *event = NULL;
    GtkWidget *menu = NULL;
    GtkUIManager *ui = GTK_UI_MANAGER(gtk_builder_get_object(pl3_xml, "uimanager1"));
    GtkMenuItem *m_item = GTK_MENU_ITEM(gtk_ui_manager_get_widget(ui, "/menubartest/menu_option"));

    //gtk_menu_item_set_submenu(m_item, NULL);

    if (!mpd_check_connected(connection) || type == -1)
        return;

    menu = gtk_menu_new();

    for (i = 0; i < num_plugins; i++)
    {
        if (gmpc_plugin_is_browser(plugins[i]))
        {
            menu_items += gmpc_plugin_browser_cat_right_mouse_menu(plugins[i], menu, type, tree, event);
        }
    }
    if (menu_items)
    {
        gtk_widget_show_all(menu);
        gtk_menu_item_set_submenu(m_item, menu);
        gtk_action_set_sensitive(GTK_ACTION(gtk_builder_get_object(pl3_xml, "menu_option")), TRUE);
    } else
    {
        g_object_ref_sink(menu);
        g_object_unref(menu);
        gtk_action_set_sensitive(GTK_ACTION(gtk_builder_get_object(pl3_xml, "menu_option")), FALSE);
    }

}


int pl3_cat_tree_button_release_event(GtkTreeView * tree, GdkEventButton * event)
{
    int i;
    gint type = pl3_cat_get_selected_browser();
    int menu_items = 0;
    GtkWidget *menu = NULL;

    if (type == -1 || !mpd_check_connected(connection) || event->button != 3)
    {
        /* no selections, or no usefull one.. so propagate the signal */
        return FALSE;
    }

    menu = gtk_menu_new();

    for (i = 0; i < num_plugins; i++)
    {
        if (gmpc_plugin_is_browser(plugins[i]))
        {
            menu_items += gmpc_plugin_browser_cat_right_mouse_menu(plugins[i], menu, type, GTK_WIDGET(tree), event);
        }
    }

    if (menu_items)
    {
        gtk_widget_show_all(menu);
        gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL,
            /*event->button */ 0, event->time);
    } else
    {
        g_object_ref_sink(menu);
        g_object_unref(menu);
    }
    return TRUE;
}

void pl3_sidebar_plugins_init(void)
{
    int i;
    for (i = 0; i < num_plugins; i++)
    {
		// This is implicitely done inside sidebar_init
//        if (gmpc_plugin_is_sidebar(plugins[i]))
        {
			gmpc_plugin_sidebar_init(plugins[i]);
        }
    }
}

/**********************************************************
 * MISC
 */
static GtkWidget *control_window = NULL;
static gboolean pl3_win_state_event(GtkWidget * window, GdkEventWindowState * event, gpointer data)
{
	GtkWidget *vbox1 =  GTK_WIDGET(gtk_builder_get_object(pl3_xml, "vbox1"));
    GtkWidget *p = GTK_WIDGET(gtk_builder_get_object(pl3_xml, "alignment1"));
    GtkWidget *h = GTK_WIDGET(gtk_builder_get_object(pl3_xml, "hbox1"));
    GtkWidget *b = (GTK_WIDGET(gtk_builder_get_object(pl3_xml, "menubartest")));
    if (((event->new_window_state) & GDK_WINDOW_STATE_FULLSCREEN))
    {
		if(control_window == NULL) {
			control_window = create_control_window(window);
			gtk_box_pack_start(GTK_BOX(vbox1), control_window, FALSE, FALSE, 0);
		}
        gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(pl3_xml, "sidebar")));
        gtk_widget_hide(p);
        gtk_widget_hide(h);
        gtk_widget_hide(b);
    } else if ((event->changed_mask) & GDK_WINDOW_STATE_FULLSCREEN)
    {
		control_window_destroy(control_window);
		control_window = NULL;
   		playlist_zoom_level_changed();
        gtk_widget_show(p);
        gtk_widget_show(h);
        gtk_widget_show(b);
    }
    return FALSE;
}

gboolean alt_button_pressed = FALSE;
/**
 * This avoids the 'keybinding help' to become sticky when moving the window, or chainging
 * focus to other window.
 */
gboolean pl3_window_focus_out_event(GtkWidget *window, GdkEventFocus *event, gpointer data)
{
	if(alt_button_pressed)
	{
		GtkWidget *tree = GTK_WIDGET(gtk_builder_get_object(pl3_xml, "cat_tree"));
		alt_button_pressed = FALSE;
		gtk_widget_queue_draw(GTK_WIDGET(tree));
	}
	GmpcToolsBindingOverlayNotify *p = gtk_builder_get_object(pl3_xml, "binding_overlay_notify");
	gmpc_tools_binding_overlay_notify_key_released(p,GDK_MOD1_MASK|GDK_CONTROL_MASK);
	return FALSE;
}


gboolean pl3_window_is_fullscreen(void)
{
    GtkWidget *win = playlist3_get_window();
    GdkWindowState state = 0;
    if (win->window)
        state = gdk_window_get_state(win->window);
    return (state & GDK_WINDOW_STATE_FULLSCREEN) ? TRUE : FALSE;
}


void pl3_window_fullscreen(void)
{
    GtkWidget *win = playlist3_get_window();

    if (pl3_zoom < PLAYLIST_MINI)
    {
        if (pl3_window_is_fullscreen())
        {
            gtk_window_unfullscreen(GTK_WINDOW(win));
        } else
        {
            gtk_window_fullscreen(GTK_WINDOW(win));
        }
    }
}


int pl3_window_key_release_event(GtkWidget * mw, GdkEventKey * event)
{
	if(event->keyval == GDK_KEY_Alt_L || event->keyval == GDK_KEY_Alt_R || event->keyval == GDK_KEY_Meta_L|| event->keyval == GDK_KEY_Meta_R) {
		GtkWidget *tree = GTK_WIDGET(gtk_builder_get_object(pl3_xml, "cat_tree"));
		alt_button_pressed = FALSE;
		gtk_widget_queue_draw(GTK_WIDGET(tree));
		GmpcToolsBindingOverlayNotify *p = gtk_builder_get_object(pl3_xml, "binding_overlay_notify");
		gmpc_tools_binding_overlay_notify_key_released(p,GDK_MOD1_MASK);
	}
	if(event->keyval == GDK_KEY_Control_L || event->keyval == GDK_KEY_Control_R) {
		GmpcToolsBindingOverlayNotify *p = gtk_builder_get_object(pl3_xml, "binding_overlay_notify");
		gmpc_tools_binding_overlay_notify_key_released(p,GDK_CONTROL_MASK);
	}
	return FALSE;
}
int pl3_window_key_press_event(GtkWidget * mw, GdkEventKey * event)
{
    int i = 0;
    gint type = pl3_cat_get_selected_browser();
	if(event->keyval == GDK_KEY_Alt_L || event->keyval == GDK_KEY_Alt_R) {
		GtkWidget *tree = GTK_WIDGET(gtk_builder_get_object(pl3_xml, "cat_tree"));
		alt_button_pressed = TRUE;
		gtk_widget_queue_draw(GTK_WIDGET(tree));
		GmpcToolsBindingOverlayNotify *p = gtk_builder_get_object(pl3_xml, "binding_overlay_notify");
		gmpc_tools_binding_overlay_notify_key_pressed(p,GDK_MOD1_MASK);
	}
	if(event->keyval == GDK_KEY_Control_L || event->keyval == GDK_KEY_Control_R) 
	{
		GmpcToolsBindingOverlayNotify *p = gtk_builder_get_object(pl3_xml, "binding_overlay_notify");
		gmpc_tools_binding_overlay_notify_key_pressed(p,GDK_CONTROL_MASK);
	}
    /**
     * Following key's are only valid when connected
     */
    if (!mpd_check_connected(connection))
    {
        return FALSE;
    }
    for (i = 0; i < num_plugins; i++)
    {
        if (gmpc_plugin_is_browser(plugins[i]))
        {
            gmpc_plugin_browser_key_press_event(plugins[i], mw, event, type);
        }
    }
	if((event->state&GDK_MOD1_MASK) > 0)
	{
		guint kev = event->keyval;
		if(kev >= GDK_KEY_0 && kev <= GDK_KEY_9)
		{
			int index = 0;
			GtkTreeIter iter;
			kev-=GDK_KEY_0;
			if (gtk_tree_model_get_iter_first(pl3_tree, &iter))
			{
				if(kev == 0) kev+=10;
				do{
					gint type =0 ;
					gtk_tree_model_get(pl3_tree, &iter, PL3_CAT_TYPE, &type, -1);
					if(type >= 0) index++;
					if(index == kev && type >= 0) 
					{
						GtkWidget *cat_tree = GTK_WIDGET(gtk_builder_get_object(pl3_xml, "cat_tree"));
						GtkTreeSelection *select = gtk_tree_view_get_selection(GTK_TREE_VIEW(cat_tree));

						// if this is allready selected, do +10. this allows us to go up to 20 browsers.
						if(type == old_type  && gtk_tree_selection_iter_is_selected(select, &iter)) {
							kev+=10;	
						}else{
							gtk_tree_selection_select_iter(select, &iter);
							return;
						}
					}
				}while(gtk_tree_model_iter_next(pl3_tree, &iter));
			}
		}
	}

    /* don't propagate */
    return FALSE;
}

/**
 * Close the playlist and save position/size
 */
gboolean pl3_close(void)
{
    /* only save when window is PLAYLIST_SMALL or NO ZOOM */
    if (pl3_xml != NULL)
    {
        GtkWidget *window = playlist3_get_window();
        int maximized = FALSE;
        if (window->window)
        {
            GdkWindowState state = gdk_window_get_state(window->window);
            maximized = ((state & GDK_WINDOW_STATE_MAXIMIZED) > 0);
        }
        cfg_set_single_value_as_int(config, "playlist", "maximized", maximized);

        gtk_window_get_position(GTK_WINDOW(window), &pl3_wsize.x, &pl3_wsize.y);

        cfg_set_single_value_as_int(config, "playlist", "xpos", pl3_wsize.x);
        cfg_set_single_value_as_int(config, "playlist", "ypos", pl3_wsize.y);

        if (pl3_zoom <= PLAYLIST_SMALL)
        {
            gtk_window_get_size(GTK_WINDOW(window), &pl3_wsize.width, &pl3_wsize.height);
            g_log(LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "pl3_close: save size: %i %i\n", pl3_wsize.width, pl3_wsize.height);
            cfg_set_single_value_as_int(config, "playlist", "width", pl3_wsize.width);
            cfg_set_single_value_as_int(config, "playlist", "height", pl3_wsize.height);
        }
    }

    if (cfg_get_single_value_as_int_with_default(config, "playlist", "hide-on-close", FALSE))
    {
        if (tray_icon2_get_available())
        {
            pl3_toggle_hidden();
            return TRUE;
        }
        gtk_window_iconify(GTK_WINDOW(playlist3_get_window()));
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
int pl3_hide(void)
{
    GtkWidget *pl3_win = playlist3_get_window();
    if (!tray_icon2_get_available())
    {
        gtk_window_iconify(GTK_WINDOW(pl3_win));
        return 1;
    }
    if (pl3_xml != NULL && !pl3_hidden)
    {
        GtkWidget *window = playlist3_get_window();
        int maximized = FALSE;
        if (window->window)
        {
            GdkWindowState state = gdk_window_get_state(window->window);
            maximized = ((state & GDK_WINDOW_STATE_MAXIMIZED) > 0);
        }
        cfg_set_single_value_as_int(config, "playlist", "maximized", maximized);
        /** Save position
         */
        gtk_window_get_position(GTK_WINDOW(pl3_win), &pl3_wsize.x, &pl3_wsize.y);
        cfg_set_single_value_as_int(config, "playlist", "xpos", pl3_wsize.x);
        cfg_set_single_value_as_int(config, "playlist", "ypos", pl3_wsize.y);
        /* save size, only when in SMALL or no zoom mode
         */
        if (pl3_zoom <= PLAYLIST_SMALL)
        {
            gtk_window_get_size(GTK_WINDOW(pl3_win), &pl3_wsize.width, &pl3_wsize.height);
            g_log(LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "pl3_hide: save size: %i %i\n", pl3_wsize.width, pl3_wsize.height);
            cfg_set_single_value_as_int(config, "playlist", "width", pl3_wsize.width);
            cfg_set_single_value_as_int(config, "playlist", "height", pl3_wsize.height);
		}
        gtk_widget_hide(pl3_win);
        pl3_hidden = TRUE;
    }
    #ifdef HAVE_APP_INDICATOR
        tray_icon2_update_menu();
    #endif
    return TRUE;
}


/* create the playlist view
 * This is done only once, for the rest its hidden, but still there
 */
static void pl3_show_and_position_window(void)
{
    GtkWidget *pl3_win;
    if (!pl3_xml)
        return;
    pl3_win = playlist3_get_window();
    if (pl3_wsize.x > 0 || pl3_wsize.y > 0)
    {
        gtk_window_move(GTK_WINDOW(pl3_win), pl3_wsize.x, pl3_wsize.y);
    }
    if (pl3_wsize.height > 0 && pl3_wsize.width > 0)
    {
        g_log(LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "restore size %i %i\n", pl3_wsize.width, pl3_wsize.height);
        gtk_window_resize(GTK_WINDOW(pl3_win), pl3_wsize.width, pl3_wsize.height);
    }
    gtk_widget_show(pl3_win);
    gtk_window_present(GTK_WINDOW(pl3_win));

}


gboolean playlist3_window_is_hidden(void)
{
    return pl3_hidden;
}


void pl3_toggle_hidden(void)
{
    if (pl3_hidden)
    {
        create_playlist3();
    } else
    {
        pl3_hide();
    }
}


static void playlist3_source_drag_data_recieved(GtkWidget * widget,
GdkDragContext * context,
gint x,
gint y, GtkSelectionData * data, guint info, guint time_recieved)
{
    if (info != 99)
    {
        int found = 0;
        const gchar *url_data = (gchar *) data->data;
        int i;
        if (url_data)
        {

            gchar **url = g_uri_list_extract_uris(url_data);
            for (i = 0; url && url[i]; i++)
            {
                gchar *scheme = g_uri_parse_scheme(url[i]);
                /* Don't add lines withouth an actual scheme. */
                if (scheme)
                {
            		gchar *fu = g_uri_unescape_string(url[i], NULL);
                    url_start_real(fu);
                    g_free(fu);
                    g_free(scheme);
                }
            }
            if (url)
                g_strfreev(url);
        }

        gtk_drag_finish(context, found, FALSE, time_recieved);
    } else
    {
        MpdData *mdata;
        gchar **stripped;
        int i;
        guchar *odata = gtk_selection_data_get_text(data);
        stripped = g_strsplit((gchar *) odata, "\n", 0);
        g_free(odata);
        if (context->action == GDK_ACTION_MOVE)
        {
            mpd_playlist_clear(connection);
        }
        mpd_database_search_start(connection, TRUE);
        for (i = 0; stripped && stripped[i]; i++)
        {
            gchar **request = g_strsplit(stripped[i], ":", 2);
            mpd_database_search_add_constraint(connection, mpd_misc_get_tag_by_name(request[0]), request[1]);
            g_strfreev(request);
        }
        mdata = mpd_database_search_commit(connection);
        for (; mdata; mdata = mpd_data_get_next(mdata))
        {
            mpd_playlist_queue_add(connection, mdata->song->file);
        }
        mpd_playlist_queue_commit(connection);
        if (context->action == GDK_ACTION_MOVE)
        {
            mpd_player_play(connection);
        }

        g_strfreev(stripped);
        gtk_drag_finish(context, TRUE, FALSE, time_recieved);
    }
}


/**
 * Progresbar
 */
void pl3_pb_seek_event(GtkWidget * pb, guint seek_time, gpointer user_data)
{
    mpd_player_seek(connection, (int)seek_time);
}



static void about_dialog_activate(GtkWidget * dialog, const gchar * uri, gpointer data)
{
    open_uri(uri);
}


/***
 * Handle a connect/Disconnect
 */
static void playlist_connection_changed(MpdObj * mi, int connect, gpointer data)
{
    GtkWidget *pl3_win = playlist3_get_window();
    /* Set menu items */
    if (connect)
    {
        char **handlers;
		gboolean found = FALSE;
        gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(pl3_xml, "vbox_playlist_player")), TRUE);
        gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(pl3_xml, "hpaned1-hbox")), TRUE);

        gtk_action_set_sensitive(GTK_ACTION(gtk_builder_get_object(pl3_xml, "MPDConnect")), FALSE);
        gtk_action_set_sensitive(GTK_ACTION(gtk_builder_get_object(pl3_xml, "MPDDisconnect")), TRUE);
        gtk_action_set_sensitive(GTK_ACTION(gtk_builder_get_object(pl3_xml, "MPDPassword")), TRUE);
        gtk_action_set_sensitive(GTK_ACTION(gtk_builder_get_object(pl3_xml, "menu_view")), TRUE);
        gtk_action_set_sensitive(GTK_ACTION(gtk_builder_get_object(pl3_xml, "menu_option")), TRUE);

		/* Check if MPD supports 'file://' (so local files). */
		/* TODO: make this a separate function */
		handlers = mpd_server_get_url_handlers(connection);
		if (handlers)
		{
			int i=0;
			for(; !found && handlers != NULL && handlers[i] != NULL; i++) {
				if(g_utf8_collate(handlers[i], "file://") == 0) {
					found = TRUE;
				}
			}
			g_strfreev(handlers);
		}
        gtk_action_set_sensitive(GTK_ACTION(gtk_builder_get_object(pl3_xml, "open_local_file")), found);
    } else
    {
        gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(pl3_xml, "vbox_playlist_player")), FALSE);
        gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(pl3_xml, "hpaned1-hbox")), FALSE);

        gtk_action_set_sensitive(GTK_ACTION(gtk_builder_get_object(pl3_xml, "MPDConnect")), TRUE);
        gtk_action_set_sensitive(GTK_ACTION(gtk_builder_get_object(pl3_xml, "MPDDisconnect")), FALSE);
        gtk_action_set_sensitive(GTK_ACTION(gtk_builder_get_object(pl3_xml, "MPDPassword")), FALSE);
        gtk_action_set_sensitive(GTK_ACTION(gtk_builder_get_object(pl3_xml, "menu_view")), FALSE);
        gtk_action_set_sensitive(GTK_ACTION(gtk_builder_get_object(pl3_xml, "menu_option")), FALSE);
        gtk_action_set_sensitive(GTK_ACTION(gtk_builder_get_object(pl3_xml, "open_local_file")), FALSE);
    }
    /** Set back to the current borwser, and update window title */
    if (connect)
    {
        gchar *string = NULL;
        GtkTreeIter iter;
        GtkTreeSelection *selec = gtk_tree_view_get_selection((GtkTreeView *) gtk_builder_get_object(pl3_xml,
            "cat_tree"));
        GtkTreeModel *model = GTK_TREE_MODEL(pl3_tree);
        if (gtk_tree_model_get_iter_first(model, &iter))
        {
            gtk_tree_selection_select_iter(selec, &iter);
        }
        if (gmpc_profiles_get_number_of_profiles(gmpc_profiles) > 1)
        {
            gchar *id = gmpc_profiles_get_current(gmpc_profiles);
            if (id)
            {
                string =
                    g_strdup_printf("[%s] %s - %s %s", gmpc_profiles_get_name(gmpc_profiles, id), _("GMPC"),
                    _("Connected to"), mpd_get_hostname(mi));
                g_free(id);
            }
        }
        if (!string)
            string = g_strdup_printf("%s - %s %s", _("GMPC"), _("Connected to"), mpd_get_hostname(mi));
        gtk_window_set_title(GTK_WINDOW(pl3_win), string);
        g_free(string);
    } else
    {
        gchar *string = NULL;

        if (gmpc_profiles_get_number_of_profiles(gmpc_profiles) > 1)
        {
            gchar *id = gmpc_profiles_get_current(gmpc_profiles);
            if (id)
            {
                string =
                    g_strdup_printf("[%s] %s - %s", gmpc_profiles_get_name(gmpc_profiles, id), _("GMPC"),
                    _("Disconnected"));
                g_free(id);
            }
        }
        if (!string)
            string = g_strdup_printf("%s - %s", _("GMPC"), _("Disconnected"));
        gtk_window_set_title(GTK_WINDOW(pl3_win), string);
        g_free(string);
    }

    /*
     * make the playlist update itself
     */
    playlist_status_changed(connection,
        MPD_CST_STATE | MPD_CST_SONGID | MPD_CST_NEXTSONG |
        MPD_CST_ELAPSED_TIME | MPD_CST_VOLUME |
        MPD_CST_REPEAT | MPD_CST_RANDOM | MPD_CST_PERMISSION
        | MPD_CST_SINGLE_MODE | MPD_CST_CONSUME_MODE | MPD_CST_UPDATING, NULL);

    /**
     * Also need updating
     */
    pl3_option_menu_activate();
    pl3_tool_menu_update();

    playlist3_fill_server_menu();

    /**
     * update interface
     * items that are caused by the plugin.
     */
    pl3_update_go_menu();

}

void pl3_style_set_event( GtkWidget *widget,
                          GtkStyle  *previous_style,
                          gpointer   user_data)
{
    if (cfg_get_single_value_as_int_with_default(config, "Default", "use-dark-style-header", TRUE))
    {
        gtk_rc_parse_string("widget \"*header*\" style \"dark\"");
    }
}
gboolean pl3_cat_select_function(GtkTreeSelection *select, GtkTreeModel *model, GtkTreePath *path, gboolean cur_select, gpointer data)
{
	GtkTreeIter iter;
	if(gtk_tree_model_get_iter(model, &iter, path))
	{
        gint type =0 ;
        gtk_tree_model_get(model, &iter, PL3_CAT_TYPE, &type, -1);
		if(type >= 0) return TRUE;
	}
	return FALSE;
}
void pl3_sidebar_text_get_key_number( GtkTreeViewColumn *column, 
		GtkCellRenderer *renderer,
		GtkTreeModel *model,
		GtkTreeIter *d_iter,
		gpointer data)
{
	int number = 0;
	GtkTreeIter iter;
	GtkTreePath *pa1 = gtk_tree_model_get_path(model, d_iter);
	if(!alt_button_pressed) {
		g_object_set(G_OBJECT(renderer), "show-number", FALSE, NULL);
		return;
	}
	if (gtk_tree_model_get_iter_first(pl3_tree, &iter))
	{
		do{
			gint type =0 ;
			gtk_tree_model_get(pl3_tree, &iter, PL3_CAT_TYPE, &type, -1);
			if(type >= 0) number++;
			if( type >= 0) 
			{
				GtkTreePath *pa2 = gtk_tree_model_get_path(model, &iter);
				if(gtk_tree_path_compare(pa2, pa1) == 0){
					g_object_set(G_OBJECT(renderer), "number", number%10, NULL);
					g_object_set(G_OBJECT(renderer), "show-number", TRUE, NULL);
					gtk_tree_path_free(pa2);
					gtk_tree_path_free(pa1);
					return;
				}
				gtk_tree_path_free(pa2);
			}
		}while(gtk_tree_model_iter_next(pl3_tree, &iter));
	}
	gtk_tree_path_free(pa1);
	g_object_set(G_OBJECT(renderer), "number", number, NULL);
	g_object_set(G_OBJECT(renderer), "show-number", FALSE, NULL);
}

void create_playlist3(void)
{
    GtkWidget *pb,*ali;
    GtkCellRenderer *renderer;
    GtkWidget *tree;
    GtkTreeSelection *sel;
    GtkTreeViewColumn *column = NULL;
    gchar *path = NULL;
    GtkTreeIter iter;
    GError *error = NULL;
	INIT_TIC_TAC();
    /* indicate that the playlist is not hidden */
    pl3_hidden = FALSE;

    /**
     * If the playlist already exists,
     * It is probably coming from a hidden state,
     * so re-position the window
     */
    if (pl3_xml != NULL)
    {
        pl3_show_and_position_window();
        return;
    }

    /** Ambiance / Radiance theme "dark" header */
    if (cfg_get_single_value_as_int_with_default(config, "Default", "use-dark-style-header", TRUE))
    {
        gtk_rc_parse_string("widget \"*header*\" style \"dark\"");
    }

    /** use background color for the sidebar treeview cells */
    gtk_rc_parse_string (
        "style \"sidebar-treeview\"\n"
        "{\n"
        "   GtkTreeView::odd-row-color = @bg_color\n"
        "   GtkTreeView::even-row-color = @bg_color\n"
        "}\n"
        "widget \"*.sidebar.*\" style \"sidebar-treeview\"");

	/** menubar */
	gtk_rc_parse_string (
        "style \"menubar-style\"\n"
        "{\n"
        "   GtkMenuBar::shadow-type = none\n"
        "}\n"
        "widget \"*.menubar\" style \"menubar-style\"\n");

    /* initial, setting the url hook */
    gtk_about_dialog_set_url_hook((GtkAboutDialogActivateLinkFunc) about_dialog_activate, NULL, NULL);
    TEC("Setup dialog url hook")
    /* load gui desciption */
    path = gmpc_get_full_glade_path("playlist3.ui");
    TEC("get path")
    pl3_xml = gtk_builder_new();
    TEC("create builder")
    if(gtk_builder_add_from_file(pl3_xml, path,&error) == 0)
    {
        /*
         * Check if the file is loaded, if not then show an error message and abort the program
        if (pl3_xml == NULL)
        {
        */
        g_log(LOG_DOMAIN, G_LOG_LEVEL_WARNING, "Failed to open playlist3.glade: %s\n", error->message);
        abort();
    }
    g_free(path);
    TEC("Load builder file")

    /* create tree store for the "category" view */
    if (pl3_tree == NULL)
    {
        GType types[] =
        {
            G_TYPE_INT,          /* row type, see free_type struct */
            G_TYPE_STRING,       /* display name */
            G_TYPE_STRING,       /* full path and stuff for backend */
            G_TYPE_STRING,       /* icon id */
            G_TYPE_INT,          /* ordering */
			G_TYPE_INT
        };
        /* song id, song title */
        pl3_tree = (GtkTreeModel *) gmpc_tools_liststore_sort_new();
        gtk_list_store_set_column_types(GTK_LIST_STORE(pl3_tree), PL3_CAT_NROWS, types);
    }
	TEC("Setup pl3_tree")
	{
		GtkTreeIter iter;

		playlist3_insert_browser(&iter, PL3_CAT_BROWSER_LIBRARY);
		gtk_list_store_set(GTK_LIST_STORE(pl3_tree), &iter,
				PL3_CAT_TYPE,-1, PL3_CAT_TITLE, _("Library"),PL3_CAT_BOLD, PANGO_WEIGHT_ULTRABOLD,-1);

		playlist3_insert_browser(&iter, PL3_CAT_BROWSER_ONLINE_MEDIA);
		gtk_list_store_set(GTK_LIST_STORE(pl3_tree), &iter,
				PL3_CAT_TYPE,-1, PL3_CAT_TITLE, _("Online Media"),PL3_CAT_BOLD, PANGO_WEIGHT_ULTRABOLD,-1);

		playlist3_insert_browser(&iter, PL3_CAT_BROWSER_MISC);
		gtk_list_store_set(GTK_LIST_STORE(pl3_tree), &iter,
				PL3_CAT_TYPE,-1, PL3_CAT_TITLE, _("Misc."),PL3_CAT_BOLD, PANGO_WEIGHT_ULTRABOLD,-1);
	}


    tree = GTK_WIDGET(gtk_builder_get_object(pl3_xml, "cat_tree"));
	gtk_tree_selection_set_select_function(gtk_tree_view_get_selection(GTK_TREE_VIEW(tree)),
			pl3_cat_select_function, pl3_tree, NULL);

    gtk_tree_view_set_model(GTK_TREE_VIEW(tree), GTK_TREE_MODEL(pl3_tree));
    sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));
    gtk_tree_selection_set_mode(GTK_TREE_SELECTION(sel), GTK_SELECTION_BROWSE);
    gtk_tree_view_set_reorderable(GTK_TREE_VIEW(tree), TRUE);
	// Enable tooltip on the treeview.
	gtk_tree_view_set_tooltip_column(GTK_TREE_VIEW(tree), PL3_CAT_TITLE);

    sidebar_text = renderer = my_cell_renderer_new();//gtk_cell_renderer_pixbuf_new();
    g_object_set(G_OBJECT(renderer), "xalign", 0.5,NULL);
    column = gtk_tree_view_column_new();


    gtk_tree_view_column_pack_start(column, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func(column, sidebar_text, pl3_sidebar_text_get_key_number, NULL, NULL);
    g_object_set(G_OBJECT(renderer), "stock-size", GTK_ICON_SIZE_MENU, NULL);
    {
        int w, h;
        if (gtk_icon_size_lookup(GTK_ICON_SIZE_MENU, &w, &h))
        {
            g_object_set(G_OBJECT(renderer), "height", h+10, NULL);
            g_object_set(G_OBJECT(renderer), "image-width", w,NULL);
        }
    }
    gtk_tree_view_column_set_attributes(column, renderer,
			"icon-name", PL3_CAT_ICON_ID,
			"text", PL3_CAT_TITLE, 
			"weight", PL3_CAT_BOLD, NULL);
    g_object_set(renderer, "weight-set", TRUE, NULL);

    gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
    gtk_tree_view_set_search_column(GTK_TREE_VIEW(tree), PL3_CAT_TITLE);
    g_signal_connect_after(G_OBJECT(sel), "changed", G_CALLBACK(pl3_cat_sel_changed), NULL);

	TEC("setup cat_tree");

	/* initialize the category view */
	pl3_initialize_tree();

	TEC("Init category tree")
    gtk_widget_show(GTK_WIDGET(gtk_builder_get_object(pl3_xml, "vbox_playlist_player")));

    /**
     * The new progress bar
     */
    pb = (GtkWidget *) gtk_builder_get_object(pl3_xml, "hbox_progress");
    gtk_widget_show(pb);
    g_signal_connect(G_OBJECT(pb), "seek-event", G_CALLBACK(pl3_pb_seek_event), NULL);

    new_pb = pb;

	TEC("Init progress bar")
    /* Make sure change is applied */

    playlist3_new_header();

    pl3_sidebar_plugins_init();

	TEC("Init header")
    if (!cfg_get_single_value_as_int_with_default(config, "Interface", "hide-favorites-icon", FALSE))
    {
        favorites_button = gmpc_favorites_button_new();
        ali = gtk_alignment_new(0.0, 0.5, 0.0, 0.0);
        gtk_container_add(GTK_CONTAINER(ali), GTK_WIDGET(favorites_button));
        gtk_box_pack_start(GTK_BOX(gtk_builder_get_object(pl3_xml, "hbox10")), GTK_WIDGET(ali), FALSE, FALSE, 0);
		gtk_widget_show_all(GTK_WIDGET(ali));
		TEC("Init fav icon")
	}
    playlist_status_changed(connection,
        MPD_CST_STATE | MPD_CST_SONGID | MPD_CST_NEXTSONG |
        MPD_CST_ELAPSED_TIME | MPD_CST_VOLUME |
        MPD_CST_REPEAT | MPD_CST_RANDOM | MPD_CST_PERMISSION
        | MPD_CST_SINGLE_MODE | MPD_CST_CONSUME_MODE, NULL);
    g_signal_connect(G_OBJECT(gtk_builder_get_object(pl3_xml, "volume_button")),
        "value_changed", G_CALLBACK(playlist_player_volume_changed), NULL);

	TEC("Signal setup")

	/* Restore values from config */
    gtk_toggle_action_set_active(GTK_TOGGLE_ACTION
        (gtk_builder_get_object
        (pl3_xml, "ViewShowArtistImage")),
        cfg_get_single_value_as_int_with_default
        (config, "playlist", "cover-image-enable", 0));

    /* connect signals that are defined in the gui description */
    gtk_builder_connect_signals(pl3_xml,NULL);

	TEC("connect signals")

    /* select the current playlist */
    if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(pl3_tree), &iter))
    {
        gtk_tree_selection_select_iter(sel, &iter);
    }

	TEC("Select view")
    /*
     * Insert new custom widget
     */
    metaimage_album_art = gmpc_metaimage_new(META_ALBUM_ART);

    /* Hide when requested. */
    if (cfg_get_single_value_as_int_with_default(config, "Interface", "hide-album-art", FALSE))
    {
        gmpc_metaimage_set_is_visible(GMPC_METAIMAGE(metaimage_album_art), FALSE);
    }
    gtk_box_pack_start(GTK_BOX
        (gtk_builder_get_object(pl3_xml, "hbox_playlist_player")), metaimage_album_art, FALSE, TRUE, 0);

    gtk_box_reorder_child(GTK_BOX
        (gtk_builder_get_object(pl3_xml, "hbox_playlist_player")), metaimage_album_art, 0);
    gmpc_metaimage_set_size(GMPC_METAIMAGE(metaimage_album_art), ALBUM_SIZE_LARGE);
    gmpc_metaimage_set_no_cover_icon(GMPC_METAIMAGE(metaimage_album_art), (char *)"gmpc");
    gmpc_metaimage_set_connection(GMPC_METAIMAGE(metaimage_album_art), connection);
    /** make sure size is updated */
    gmpc_metaimage_set_cover_na(GMPC_METAIMAGE(metaimage_album_art));

    metaimage_artist_art = gmpc_metaimage_new(META_ARTIST_ART);
    gtk_container_add(GTK_CONTAINER(gtk_builder_get_object(pl3_xml, "sidebar_artist_image_alignment")), metaimage_artist_art);

    gmpc_metaimage_set_no_cover_icon(GMPC_METAIMAGE(metaimage_artist_art), (char *)"no-artist");
    gmpc_metaimage_set_loading_cover_icon(GMPC_METAIMAGE(metaimage_artist_art), (char *)"fetching-artist");
    gmpc_metaimage_set_connection(GMPC_METAIMAGE(metaimage_artist_art), connection);
    if (!cfg_get_single_value_as_int_with_default(config, "playlist", "cover-image-enable", FALSE))
    {
        gmpc_metaimage_set_is_visible(GMPC_METAIMAGE(metaimage_artist_art), FALSE);
    }
    gmpc_metaimage_set_squared(GMPC_METAIMAGE(metaimage_artist_art), FALSE);
    gmpc_metaimage_set_size(GMPC_METAIMAGE(metaimage_artist_art), 145);

	TEC("Setup metaimages")
    /* restore the window's position and size, if the user wants this. */
    if (cfg_get_single_value_as_int_with_default(config, "playlist", "savepossize", 1))
    {
        int maximized = cfg_get_single_value_as_int_with_default(config, "playlist", "maximized", 0);
        /* Load values from config file */
        pl3_wsize.x = cfg_get_single_value_as_int_with_default(config, "playlist", "xpos", 0);
        pl3_wsize.y = cfg_get_single_value_as_int_with_default(config, "playlist", "ypos", 0);
        pl3_wsize.width = cfg_get_single_value_as_int_with_default(config, "playlist", "width", 0);
        pl3_wsize.height = cfg_get_single_value_as_int_with_default(config, "playlist", "height", 0);
		TEC("get settings")
        /* restore location + position */
        /*pl3_show_and_position_window(); */

        if (pl3_wsize.x > 0 || pl3_wsize.y > 0)
        {
            gtk_window_move(GTK_WINDOW(playlist3_get_window()), pl3_wsize.x, pl3_wsize.y);
        }
		TEC("move window settings")
        if (pl3_wsize.height > 0 && pl3_wsize.width > 0)
        {
            g_log(LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "restore size %i %i\n", pl3_wsize.width, pl3_wsize.height);
            gtk_window_resize(GTK_WINDOW(playlist3_get_window()), pl3_wsize.width, pl3_wsize.height);
        }
		TEC("resize window settings")
        /* restore pane position */
		gtk_widget_set_size_request(GTK_WIDGET(gtk_builder_get_object(pl3_xml,"sidebar")), 
				SIDEBAR_LARGE,-1); 
		TEC("set pane window settings")
        if (maximized)
            gtk_window_maximize(GTK_WINDOW(playlist3_get_window()));
		TEC("maximize pane window settings")
        /**
         * restore zoom level
         */
    }
	gtk_widget_show(playlist3_get_window());
	TEC("Show window")
	pl3_zoom = cfg_get_single_value_as_int_with_default(config, "playlist", "zoomlevel", PLAYLIST_NO_ZOOM);
    playlist_zoom_level_changed();
	TEC("zoom level")

    #ifdef HAVE_APP_INDICATOR
        tray_icon2_update_menu();
    #endif

	TEC("Restore state")
    pl3_update_go_menu();
	TEC("Go menu")
    /* make it update itself */
    pl3_update_profiles_menu(gmpc_profiles, PROFILE_ADDED, -1, NULL);
    g_signal_connect(G_OBJECT(gmpc_profiles), "changed", G_CALLBACK(pl3_update_profiles_menu), NULL);
    g_signal_connect(G_OBJECT(gmpc_profiles), "changed", G_CALLBACK(pl3_profiles_changed), NULL);

	TEC("Update profiles")
    /**
     * Set as drag destination
     */
    gtk_drag_dest_set(GTK_WIDGET(gtk_builder_get_object(pl3_xml, "hbox_playlist_player")),
        GTK_DEST_DEFAULT_ALL,
        target_table, 6, GDK_ACTION_COPY | GDK_ACTION_LINK | GDK_ACTION_DEFAULT | GDK_ACTION_MOVE);
    g_signal_connect(G_OBJECT
        (gtk_builder_get_object(pl3_xml, "hbox_playlist_player")),
        "drag_data_received", GTK_SIGNAL_FUNC(playlist3_source_drag_data_recieved), NULL);

	TEC("setup drag")
    /**
     *
     */
    playlist_connection_changed(connection, FALSE, NULL);

    g_signal_connect(G_OBJECT(playlist3_get_window()), "window-state-event", G_CALLBACK(pl3_win_state_event), NULL);

	TEC("signal connn changed")

    g_signal_connect(G_OBJECT(playlist3_get_window()), "style-set", G_CALLBACK(pl3_style_set_event), NULL);

    /**
     * Add status icons
     */
    main_window_init_default_status_icons();
    main_window_update_status_icons();

	TEC("Update status icon")
    /* Update extra */
    init_extra_playlist_state();
	TEC("Setup extra playlist")
}


/**
 * Helper functions
 */
GtkListStore *playlist3_get_category_tree_store(void)
{
    if (pl3_xml == NULL)
        return NULL;
    return GTK_LIST_STORE(pl3_tree);
}


GtkTreeView *playlist3_get_category_tree_view(void)
{
    if (pl3_xml == NULL)
        return NULL;
    return (GtkTreeView *) gtk_builder_get_object(pl3_xml, "cat_tree");
}

GtkWidget* playlist3_get_widget_by_id(const char *id) {
    return (GtkWidget *) gtk_builder_get_object(pl3_xml, id);
}


/****************************************************************************************
 *  PREFERENCES										*
 ****************************************************************************************/
/* prototyping for glade */
void ck_stop_on_exit_toggled_cb(GtkToggleButton * but, gpointer data);
void ck_show_tooltip_enable_tb(GtkToggleButton * but);

G_MODULE_EXPORT void show_cover_case_tb(GtkToggleButton * but)
{
    int bool1 = gtk_toggle_button_get_active(but);
    cfg_set_single_value_as_int(config, "metaimage", "addcase", bool1);
    gmpc_meta_watcher_force_reload(gmw);
}


G_MODULE_EXPORT void ck_stop_on_exit_toggled_cb(GtkToggleButton * but, gpointer data)
{
    int bool1 = gtk_toggle_button_get_active(but);
    cfg_set_single_value_as_int(config, "connection", "stop-on-exit", bool1);
}


G_MODULE_EXPORT void hide_on_close_enable_tb(GtkToggleButton * but)
{
    int bool1 = gtk_toggle_button_get_active(but);
    cfg_set_single_value_as_int(config, "playlist", "hide-on-close", bool1);
}


G_MODULE_EXPORT void cur_song_center_enable_tb(GtkToggleButton * but)
{
    int bool1 = gtk_toggle_button_get_active(but);
    cfg_set_single_value_as_int(config, "playlist", "st_cur_song", bool1);
}


G_MODULE_EXPORT void save_possize_enable_tb(GtkToggleButton * but)
{
    int bool1 = gtk_toggle_button_get_active(but);
    cfg_set_single_value_as_int(config, "playlist", "savepossize", bool1);
}


G_MODULE_EXPORT void ck_show_tooltip_enable_tb(GtkToggleButton * but)
{
    int bool1 = gtk_toggle_button_get_active(but);
    cfg_set_single_value_as_int(config, "GmpcTreeView", "show-tooltip", bool1);
}


G_MODULE_EXPORT void ck_search_as_you_type(GtkToggleButton * but)
{
    int bool1 = gtk_toggle_button_get_active(but);
    cfg_set_single_value_as_int(config, "general", "search-as-you-type", bool1);
}


static void playlist_pref_destroy(GtkWidget * container)
{
    if (playlist_pref_xml)
    {
        GtkWidget *vbox = (GtkWidget *) gtk_builder_get_object(playlist_pref_xml,
            "playlist-vbox");
        gtk_container_remove(GTK_CONTAINER(container), vbox);
        g_object_unref(playlist_pref_xml);
        playlist_pref_xml = NULL;
    }
}


void playlist_pref_construct(GtkWidget * container)
{
	GError *error = NULL;
    gchar *path = gmpc_get_full_glade_path("preferences-playlist.ui");
    playlist_pref_xml = gtk_builder_new();

    gtk_builder_add_from_file(playlist_pref_xml, path, &error);
	if(error) {
		g_warning("Failed to open builder file: %s", error->message);
		g_error_free(error);
		return;
	}
    if (playlist_pref_xml)
    {
        GtkWidget *vbox = (GtkWidget *) gtk_builder_get_object(playlist_pref_xml,
            "playlist-vbox");
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON
            (gtk_builder_get_object
            (playlist_pref_xml, "ck_ps")),
            cfg_get_single_value_as_int_with_default(config, "playlist", "st_cur_song", 0));
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON
            (gtk_builder_get_object
            (playlist_pref_xml, "ck_possize")),
            cfg_get_single_value_as_int_with_default(config, "playlist", "savepossize", 1));
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON
            (gtk_builder_get_object
            (playlist_pref_xml, "ck_hide_on_close")),
            cfg_get_single_value_as_int_with_default(config, "playlist", "hide-on-close", 0));

        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON
            (gtk_builder_get_object
            (playlist_pref_xml, "ck_stop_on_exit")),
            cfg_get_single_value_as_int_with_default(config, "connection", "stop-on-exit", 0));

        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON
            (gtk_builder_get_object
            (playlist_pref_xml, "ck_cover_case")),
            cfg_get_single_value_as_int_with_default(config, "metaimage", "addcase", TRUE));

        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON
            (gtk_builder_get_object
            (playlist_pref_xml, "ck_show_tooltip")),
            cfg_get_single_value_as_int_with_default
            (config, "GmpcTreeView", "show-tooltip", TRUE));

        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON
            (gtk_builder_get_object
            (playlist_pref_xml, "ck_search_as_you_type")),
            cfg_get_single_value_as_int_with_default(config, "general", "search-as-you-type",
            0));
        gtk_container_add(GTK_CONTAINER(container), vbox);
		gtk_widget_show_all(vbox);
        gtk_builder_connect_signals(playlist_pref_xml, NULL);
    }
    g_free(path);
}


/**
 * Menu Callback functions
 */

void playlist_menu_repeat_changed(GtkToggleAction * action)
{
    int active = gtk_toggle_action_get_active(action);
    if (active != mpd_player_get_repeat(connection))
    {
        mpd_player_set_repeat(connection, active);
    }
}


void playlist_menu_random_changed(GtkToggleAction *action)
{
    int active = gtk_toggle_action_get_active(action);
    if (active != mpd_player_get_random(connection))
    {
        mpd_player_set_random(connection, active);
    }
}


void playlist_menu_single_mode_changed(GtkToggleAction * action)
{
    int active = gtk_toggle_action_get_active(action);
    if (active != mpd_player_get_single(connection))
    {
        mpd_player_set_single(connection, active);
    }
}


void playlist_menu_consume_changed(GtkToggleAction * action)
{
    int active = gtk_toggle_action_get_active(action);
    if (active != mpd_player_get_consume(connection))
    {
        mpd_player_set_consume(connection, active);
    }
}


/**
 * This is artist image
 * FIXME: Rename
 */
void playlist_menu_artist_image_changed(GtkToggleAction *ta)
{
    int active = gtk_toggle_action_get_active(ta);
    cfg_set_single_value_as_int(config, "playlist", "cover-image-enable", active);
	if(pl3_zoom < PLAYLIST_SMALL)
	{
		gmpc_metaimage_set_is_visible(GMPC_METAIMAGE(metaimage_artist_art), active);
		if (active)
			gtk_widget_show(metaimage_artist_art);
	}
}


/***
 * Zooming functions
 */
void playlist_zoom_out(void)
{
    /* Do not change zoom level when fullscreen */
    if(pl3_window_is_fullscreen())
		return;
    if ((pl3_zoom + 1) >= PLAYLIST_ZOOM_LEVELS)
        return;
    pl3_old_zoom = pl3_zoom;
    pl3_zoom++;
    playlist_zoom_level_changed();
}


void playlist_zoom_in(void)
{
    /* Do not change zoom level when fullscreen */
    if(pl3_window_is_fullscreen())
		return;
    if (pl3_zoom <= PLAYLIST_NO_ZOOM)
        return;
    pl3_old_zoom = pl3_zoom;
    pl3_zoom--;
    playlist_zoom_level_changed();
}


/**
 * FIXME: Needs propper grouping and cleaning up
 */
static void playlist_zoom_level_changed(void)
{
    GtkWidget *pl3_win = playlist3_get_window();
	printf("playlist3 zoom level changed\n");

    if (pl3_old_zoom <= PLAYLIST_SMALL)
    {
        gtk_window_get_size(GTK_WINDOW(pl3_win), &pl3_wsize.width, &pl3_wsize.height);
        cfg_set_single_value_as_int(config, "playlist", "width", pl3_wsize.width);
        cfg_set_single_value_as_int(config, "playlist", "height", pl3_wsize.height);
        g_log(LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "save size: %i %i\n", pl3_wsize.width, pl3_wsize.height);
    }

    if (pl3_old_zoom == PLAYLIST_MINI && pl3_zoom != PLAYLIST_MINI)
    {
        GtkWidget *box =GTK_WIDGET(gtk_builder_get_object(pl3_xml, "pl3_button_control_box"));
        GtkWidget *top = GTK_WIDGET(gtk_builder_get_object(pl3_xml, "hbox10"));
        GtkWidget *vtop = GTK_WIDGET(gtk_builder_get_object(pl3_xml, "vbox_playlist_player"));
        /* add my own reference */
        g_object_ref(box);
        gtk_container_remove(GTK_CONTAINER(vtop), box);
        gtk_box_pack_end(GTK_BOX(top), box, FALSE, TRUE, 0);
        gtk_box_reorder_child(GTK_BOX(top), box, 0);
        /* release my reference */
        g_object_unref(box);
        gtk_widget_show(box);
        gmpc_progress_set_hide_text(GMPC_PROGRESS(new_pb), FALSE);

        /* Album image only if enabled. */
        if(metaimage_album_art != NULL)
        {
            gmpc_metaimage_set_size(GMPC_METAIMAGE(metaimage_album_art), ALBUM_SIZE_LARGE);
            gmpc_metaimage_reload_image(GMPC_METAIMAGE(metaimage_album_art));
        }

    }
    if (pl3_old_zoom != PLAYLIST_MINI && pl3_zoom == PLAYLIST_MINI)
    {
        GtkWidget *box =GTK_WIDGET(gtk_builder_get_object(pl3_xml, "pl3_button_control_box"));
        GtkWidget *top = GTK_WIDGET(gtk_builder_get_object(pl3_xml, "hbox10"));
        GtkWidget *vtop = GTK_WIDGET(gtk_builder_get_object(pl3_xml, "vbox_playlist_player"));
        /* add my own reference */
        g_object_ref(box);
        gtk_container_remove(GTK_CONTAINER(top), box);
        gtk_box_pack_end(GTK_BOX(vtop), box, FALSE, TRUE, 3);
        /* release my reference */
        g_object_unref(box);
        gtk_widget_show(box);

        gmpc_progress_set_hide_text(GMPC_PROGRESS(new_pb), TRUE);

        /* Album image only if enabled. */
        if(metaimage_album_art != NULL)
        {
            gmpc_metaimage_set_size(GMPC_METAIMAGE(metaimage_album_art), ALBUM_SIZE_SMALL);
            gmpc_metaimage_reload_image(GMPC_METAIMAGE(metaimage_album_art));
        }

    }

    /* Show full view */
    gtk_widget_show(GTK_WIDGET(gtk_builder_get_object(pl3_xml, "hpaned1-hbox")));
    gtk_widget_show(GTK_WIDGET(gtk_builder_get_object(pl3_xml, "hbox1")));
    gtk_widget_show(GTK_WIDGET(gtk_builder_get_object(pl3_xml, "hbox10")));
    /** Menu Bar */
    /** BUTTON BOX */
    gtk_widget_show(GTK_WIDGET(gtk_builder_get_object(pl3_xml, "pl3_button_control_box")));

    gtk_window_set_resizable(GTK_WINDOW(pl3_win), TRUE);
    if (pl3_wsize.width > 0 && pl3_wsize.height > 0 && pl3_old_zoom == PLAYLIST_MINI)
    {
        g_log(LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "restore size %i %i\n", pl3_wsize.width, pl3_wsize.height);
        gtk_window_resize(GTK_WINDOW(pl3_win), pl3_wsize.width, pl3_wsize.height);
    }
    gtk_widget_show(GTK_WIDGET(gtk_builder_get_object(pl3_xml, "sidebar")));

    if (cfg_get_single_value_as_int_with_default(config, "playlist", "cover-image-enable", FALSE))
    {
        gmpc_metaimage_set_is_visible(GMPC_METAIMAGE(metaimage_artist_art), TRUE);
		gtk_widget_show(metaimage_artist_art);
	}
	gtk_action_set_visible(GTK_ACTION(gtk_builder_get_object(pl3_xml, "menu_go")),TRUE);
    gtk_action_set_visible(GTK_ACTION(gtk_builder_get_object(pl3_xml, "menu_option")),TRUE);

	gboolean st_shown;
	g_object_get(G_OBJECT(sidebar_text), "show_text", &st_shown, NULL);
	if(!st_shown)
	{
		/* restore pane position */
		g_object_set(sidebar_text, "show_text", TRUE, NULL);
		gtk_widget_set_size_request(GTK_WIDGET(gtk_builder_get_object(pl3_xml,"sidebar")), 
				SIDEBAR_LARGE,-1); 
		gmpc_sidebar_plugins_update_state(GMPC_PLUGIN_SIDEBAR_STATE_FULL);
	}

    /* Now start hiding */
    switch (pl3_zoom)
    {
        case PLAYLIST_NO_ZOOM:
            break;
        case PLAYLIST_MINI:
            gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(pl3_xml, "hpaned1-hbox")));
            gtk_action_set_visible(GTK_ACTION(gtk_builder_get_object(pl3_xml, "menu_option")),FALSE);
            gtk_action_set_visible(GTK_ACTION(gtk_builder_get_object(pl3_xml, "menu_go")),FALSE);
            if (pl3_win->window)
            {
                if (gdk_window_get_state(pl3_win->window) & GDK_WINDOW_STATE_MAXIMIZED)
                {
                    gtk_window_unmaximize(GTK_WINDOW(pl3_win));
                }

                if (gdk_window_get_state(pl3_win->window) & GDK_WINDOW_STATE_FULLSCREEN)
                {
                    gtk_window_unfullscreen(GTK_WINDOW(pl3_win));
                }
            }
            gtk_window_set_resizable(GTK_WINDOW(pl3_win), FALSE);

            gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(pl3_xml, "sidebar")));
            break;
		case PLAYLIST_SMALL:
			gmpc_metaimage_set_is_visible(GMPC_METAIMAGE(metaimage_artist_art), FALSE);
			if(st_shown) {
				g_object_set(sidebar_text, "show_text", FALSE, NULL);
				gtk_widget_set_size_request(GTK_WIDGET(gtk_builder_get_object(pl3_xml,"sidebar")),
						SIDEBAR_SMALL,-1); 

				gtk_widget_queue_draw(GTK_WIDGET(gtk_builder_get_object(pl3_xml,"sidebar")));
				gmpc_sidebar_plugins_update_state(GMPC_PLUGIN_SIDEBAR_STATE_COLLAPSED);
			}
			gtk_widget_grab_focus(pl3_win);
        default:
            break;
    }
    /** Save zoom level
     */
    cfg_set_single_value_as_int(config, "playlist", "zoomlevel", pl3_zoom);
}


/**
 * Update the window to status changes in mpd
 */
static void playlist_status_changed(MpdObj * mi, ChangedStatusType what, void *userdata)
{
    char buffer[1024];
    GtkWidget *pl3_win = playlist3_get_window();
    /**
     * if the window isn't there yet, return
     */
    if (!pl3_xml)
        return;
	control_window_status_update(mi, what, control_window);
    /**
     * Player state changed
     */
    if (what & MPD_CST_STATE)
    {
        mpd_Song *song = mpd_playlist_get_current_song(connection);
        int state = mpd_player_get_state(mi);
        switch (state)
        {
            case MPD_PLAYER_PLAY:
            {
                gchar *markup = cfg_get_single_value_as_string_with_default(config,
                    "playlist",  /* Category */
                    "window-markup",/* Key */
                                 /* default value */
                    "[%title% - &[%artist%]]|%name%|%shortfile%"
                    );
                /**
                 * Update the image in the menu
                 */
                gtk_action_set_stock_id(GTK_ACTION(gtk_builder_get_object(pl3_xml, "MPDPlayPause")), "gtk-media-pause");
                gtk_image_set_from_stock(GTK_IMAGE
                    (gtk_builder_get_object
                    (pl3_xml, "play_button_image")), "gtk-media-pause",
					GTK_ICON_SIZE_MENU);

                /**
                 * Update window title
                 */
                mpd_song_markup(buffer, 1024, markup, mpd_playlist_get_current_song(connection));

                if (gmpc_profiles_get_number_of_profiles(gmpc_profiles) > 1)
                {
                    gchar *id = gmpc_profiles_get_current(gmpc_profiles);
                    if (id)
                    {
                        gchar *string = g_strdup_printf("[%s] %s", gmpc_profiles_get_name(gmpc_profiles,
                            id), buffer);
                        gtk_window_set_title(GTK_WINDOW(pl3_win), string);
                        g_free(id);
                        g_free(string);
                    }
                } else
                gtk_window_set_title(GTK_WINDOW(pl3_win), buffer);

                g_free(markup);
                break;
            }
            case MPD_PLAYER_PAUSE:
            {
                gchar *markup = cfg_get_single_value_as_string_with_default(config,
                    "playlist",  /* Category */
                    "window-markup",/* Key */
                                 /* default value */
                    "[%title% - &[%artist%]]|%name%|%shortfile%"
                    );
                /** Update menu and button images */

                gtk_action_set_stock_id(GTK_ACTION(gtk_builder_get_object(pl3_xml, "MPDPlayPause")), "gtk-media-play");
                gtk_image_set_from_stock(GTK_IMAGE
                    (gtk_builder_get_object
                    (pl3_xml, "play_button_image")), "gtk-media-play",
					GTK_ICON_SIZE_MENU);
                /**
                 * Set paused in Window string
                 */
                mpd_song_markup(buffer, 1024 - strlen(_("paused") - 4),
                    markup, mpd_playlist_get_current_song(connection));
                /* Append translated paused */
                strcat(buffer, " (");
                strcat(buffer, _("paused"));
                strcat(buffer, ")");

                if (gmpc_profiles_get_number_of_profiles(gmpc_profiles) > 1)
                {
                    gchar *id = gmpc_profiles_get_current(gmpc_profiles);
                    if (id)
                    {
                        gchar *string = g_strdup_printf("[%s] %s", gmpc_profiles_get_name(gmpc_profiles,
                            id), buffer);
                        gtk_window_set_title(GTK_WINDOW(pl3_win), string);
                        g_free(id);
                        g_free(string);
                    }
                } else
                gtk_window_set_title(GTK_WINDOW(pl3_win), buffer);
                g_free(markup);
                break;
            }
            default:
                gtk_action_set_stock_id(GTK_ACTION(gtk_builder_get_object(pl3_xml, "MPDPlayPause")), "gtk-media-play");
                /* Make sure it's reset correctly */
                gmpc_progress_set_time(GMPC_PROGRESS(new_pb), 0, 0);

                gtk_image_set_from_stock(GTK_IMAGE
                    (gtk_builder_get_object
                    (pl3_xml, "play_button_image")), "gtk-media-play",
					GTK_ICON_SIZE_MENU);
                if (gmpc_profiles_get_number_of_profiles(gmpc_profiles) > 1)
                {
                    gchar *id = gmpc_profiles_get_current(gmpc_profiles);
                    if (id)
                    {
                        gchar *string = g_strdup_printf("[%s] %s", gmpc_profiles_get_name(gmpc_profiles,
                            id), _("GMPC"));
                        gtk_window_set_title(GTK_WINDOW(pl3_win), string);
                        g_free(id);
                        g_free(string);
                    }
                } else
                gtk_window_set_title(GTK_WINDOW(pl3_win), _("GMPC"));
        }
        playlist3_update_header();

        if(favorites_button != NULL)
        {
            if (state == MPD_PLAYER_PLAY || state == MPD_PLAYER_PAUSE)
            {
                gmpc_favorites_button_set_song(favorites_button, song);
            } else
            {
                gmpc_favorites_button_set_song(favorites_button, NULL);
            }
        }
    }
    /**
     * Handle song change or Playlist change
     * Anything that can change metadta
     */
    if (what & MPD_CST_SONGID || what & MPD_CST_SONGPOS || what & MPD_CST_PLAYLIST)
    {
        playlist3_update_header();
        /* make is update markups and stuff */
        playlist_status_changed(mi, MPD_CST_STATE, NULL);
    }
    /**
     * set repeat buttons in menu correct
     */
    if (what & MPD_CST_REPEAT)
    {
        if (mpd_check_connected(connection))
        {
            gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(gtk_builder_get_object(pl3_xml, "MPDRepeat")),
                mpd_player_get_repeat(connection));
        }

    }
    if (what & MPD_CST_RANDOM)
    {
        if (mpd_check_connected(connection))
        {
            gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(gtk_builder_get_object(pl3_xml, "MPDRandom")),
                mpd_player_get_random(connection));
        }
    }
    if (what & (MPD_CST_RANDOM | MPD_CST_REPEAT | MPD_CST_SINGLE_MODE | MPD_CST_CONSUME_MODE))
    {
        main_window_update_status_icons();
    }
    if (what & MPD_CST_SINGLE_MODE)
    {
        if (mpd_check_connected(connection))
        {
            gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(gtk_builder_get_object(pl3_xml, "MPDSingleMode")),
                mpd_player_get_single(connection));
        }
    }

    if (what & MPD_CST_CONSUME_MODE)
    {
        if (mpd_check_connected(connection))
        {
            gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(gtk_builder_get_object(pl3_xml, "MPDConsumeMode")),
                mpd_player_get_consume(connection));
        }
    }
    if (what & MPD_CST_ELAPSED_TIME)
    {
        if (mpd_check_connected(connection))
        {
            int totalTime = mpd_status_get_total_song_time(connection);
            int elapsedTime = mpd_status_get_elapsed_song_time(connection);
            gmpc_progress_set_time(GMPC_PROGRESS(new_pb), totalTime, elapsedTime);
        } else
        {

            gmpc_progress_set_time(GMPC_PROGRESS(new_pb), 0, 0);
        }
    }
    if (what & MPD_CST_PERMISSION)
    {
        gtk_action_set_sensitive(GTK_ACTION(gtk_builder_get_object(pl3_xml, "MPDSingleMode")),
            (
            mpd_check_connected(connection) &&
            mpd_server_check_command_allowed(connection, "single") == MPD_SERVER_COMMAND_ALLOWED
            )
            );
        gtk_action_set_sensitive(GTK_ACTION(gtk_builder_get_object(pl3_xml, "MPDConsumeMode")),
            (
            mpd_check_connected(connection) &&
            mpd_server_check_command_allowed(connection, "consume") == MPD_SERVER_COMMAND_ALLOWED
            )
            );
        gtk_action_set_sensitive(GTK_ACTION(gtk_builder_get_object(pl3_xml, "MPDPlayPause")),
            (
            mpd_check_connected(connection) &&
            mpd_server_check_command_allowed(connection, "play") == MPD_SERVER_COMMAND_ALLOWED
            )
            );
        gtk_action_set_sensitive(GTK_ACTION(gtk_builder_get_object(pl3_xml, "MPDNext")),
            (
            mpd_check_connected(connection) &&
            mpd_server_check_command_allowed(connection, "next") == MPD_SERVER_COMMAND_ALLOWED
            )
            );
        gtk_action_set_sensitive(GTK_ACTION(gtk_builder_get_object(pl3_xml, "MPDPrevious")),
            (
            mpd_check_connected(connection) &&
            mpd_server_check_command_allowed(connection, "previous") == MPD_SERVER_COMMAND_ALLOWED
            )
            );
        gtk_action_set_sensitive(GTK_ACTION(gtk_builder_get_object(pl3_xml, "MPDStop")),
            (
            mpd_check_connected(connection) &&
            mpd_server_check_command_allowed(connection, "stop") == MPD_SERVER_COMMAND_ALLOWED
            )
            );
        gtk_action_set_sensitive(GTK_ACTION(gtk_builder_get_object(pl3_xml, "MPDRepeat")),
            (
            mpd_check_connected(connection) &&
            mpd_server_check_command_allowed(connection, "repeat") == MPD_SERVER_COMMAND_ALLOWED
            )
            );
        gtk_action_set_sensitive(GTK_ACTION(gtk_builder_get_object(pl3_xml, "MPDRandom")),
            (
            mpd_check_connected(connection)&&
            mpd_server_check_command_allowed(connection, "random") == MPD_SERVER_COMMAND_ALLOWED
            )
            );
        /* Also update volume stuff */
        what = what|MPD_CST_VOLUME;
    }

    if (what & MPD_CST_VOLUME)
    {
        GtkWidget *volume_button = GTK_WIDGET(gtk_builder_get_object(pl3_xml, "volume_button"));
                                 //gtk_scale_button_get_value(GTK_SCALE_BUTTON(volume_button)) * 100;
        int volume = gmpc_widgets_volume_get_volume_level(GMPC_WIDGETS_VOLUME(volume_button));
        int new_volume = mpd_status_get_volume(connection);
        if (new_volume >= 0 &&
            mpd_server_check_command_allowed(connection, "setvol") == MPD_SERVER_COMMAND_ALLOWED
            )
        {
            gtk_action_set_sensitive(GTK_ACTION(gtk_builder_get_object(pl3_xml, "MPDMuted")),
                TRUE
                );
            gtk_widget_set_sensitive(volume_button, TRUE);
            /* don't do anything if nothing is changed */
            if (new_volume != volume)
            {
                gmpc_widgets_volume_set_volume_level(GMPC_WIDGETS_VOLUME(volume_button), new_volume );
            }
        } else
        {
            gtk_action_set_sensitive(GTK_ACTION(gtk_builder_get_object(pl3_xml, "MPDMuted")),
                FALSE
                );
            gtk_widget_set_sensitive(volume_button, FALSE);
        }
    }
    if (what & MPD_CST_SERVER_ERROR)
    {
        gchar *error = mpd_status_get_mpd_error(mi);
        if (error)
        {
            gchar *mes = g_markup_printf_escaped("%s: '%s'",
                _("MPD Reported the following error"),
                error);
            playlist3_show_error_message(mes, ERROR_WARNING);
            g_free(mes);
            g_free(error);
        }
    }
    if (what & MPD_CST_OUTPUT)
    {
        playlist3_fill_server_menu();
    }
    if (what & MPD_CST_NEXTSONG)
    {

        GtkWidget *next_button = GTK_WIDGET(gtk_builder_get_object(pl3_xml, "next_button"));
        if (next_button)
        {
            int i = mpd_player_get_next_song_id(mi);
            if (i >= 0)
            {
                mpd_Song *song = mpd_playlist_get_song(mi, i);
                if (song)
                {
                    mpd_song_markup(buffer, 1024, "[%title% - &[%artist%]]|%shortfile%", song);
                    gtk_widget_set_tooltip_text(next_button, buffer);
                    mpd_freeSong(song);
                } else
                gtk_widget_set_tooltip_text(next_button, "");
            } else
            gtk_widget_set_tooltip_text(next_button, "");
        }
    }
}

gboolean playlist_player_volume_changed(GtkWidget * vol_but, int new_vol)
{
    int volume = new_vol;        //gtk_scale_button_get_value(GTK_SCALE_BUTTON(vol_but)) * 100;
    int new_volume = mpd_status_get_volume(connection);
    if (new_volume >= 0 && new_volume != volume)
    {
        mpd_status_set_volume(connection, volume);
        return FALSE;
    }
    return FALSE;
}


void about_window(void)
{
    gchar *path = gmpc_get_full_glade_path("aboutdialog.ui");
    GtkBuilder *xml = gtk_builder_new();
    GtkWidget *dialog = NULL;
    gtk_builder_add_from_file(xml, path, NULL);
    dialog = (GtkWidget *) gtk_builder_get_object(xml, "aboutdialog");

    gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(playlist3_get_window()));
    gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER_ON_PARENT);
    g_free(path);

    if (strlen(revision))
    {
        path = g_strdup_printf("%s\nRevision: %s", VERSION, revision);
    } else
    {
        path = g_strdup_printf("%s\n%s\n", VERSION, GMPC_TAGLINE);
    }
    gtk_about_dialog_set_copyright(GTK_ABOUT_DIALOG(dialog), GMPC_COPYRIGHT);
    gtk_about_dialog_set_program_name(GTK_ABOUT_DIALOG(dialog), _("Gnome Music Player Client"));
    gtk_about_dialog_set_website(GTK_ABOUT_DIALOG(dialog), GMPC_WEBSITE);
    gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(dialog), path);

    g_free(path);
    gtk_widget_show(dialog);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
    g_object_unref(xml);
}


/****************************************************
 * Interface stuff
 */
void pl3_update_go_menu(void)
{
    int i = 0;
    int items = 0;
    GtkWidget *menu = NULL;
    GtkAccelGroup *group = playlist3_get_accel_group();
    GtkUIManager *ui = GTK_UI_MANAGER(gtk_builder_get_object(pl3_xml, "uimanager1"));
    GtkMenuItem *m_item = GTK_MENU_ITEM(gtk_ui_manager_get_widget(ui, "/menubartest/menu_go"));
    /***
     * Remove any old menu
     */
    gtk_menu_item_set_submenu(m_item, NULL);
    /**
     * Create a new menu
     */
    menu = gtk_menu_new();
    gtk_menu_set_accel_group(GTK_MENU(menu), group);
    if (mpd_check_connected(connection))
    {
        for (i = 0; i < num_plugins; i++)
        {
            if (gmpc_plugin_is_browser(plugins[i]))
            {
                items += gmpc_plugin_browser_add_go_menu(plugins[i], menu);
            }
        }
    }

    /**
     * Attach menu
     */
    if (items)
    {
        gtk_widget_show_all(menu);
        gtk_menu_item_set_submenu(m_item, menu);
        gtk_action_set_sensitive(GTK_ACTION(gtk_builder_get_object(pl3_xml, "menu_go")), TRUE);
    } else
    {
        gtk_action_set_sensitive(GTK_ACTION(gtk_builder_get_object(pl3_xml, "menu_go")), FALSE);
        g_object_ref_sink(menu);
        g_object_unref(menu);
    }
}


static void pl3_profile_selected(GtkRadioMenuItem * radio, gpointer data)
{
    if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(radio)))
    {
        gchar *uid = g_object_get_data(G_OBJECT(radio), "uid");
        if (!uid)
        {
            return;
        }
        connection_set_current_profile(uid);
        if (mpd_check_connected(connection))
        {
            mpd_disconnect(connection);
            connect_to_mpd();
        }
    }
}


static void pl3_profiles_changed(GmpcProfiles * prof, const int changed, const int col, const gchar * id)
{
    if (!mpd_check_connected(connection))
    {
        playlist_connection_changed(connection, 0, NULL);
    }
}


static void pl3_update_profiles_menu(GmpcProfiles * prof, const int changed, const int col, const gchar * id)
{
    int items = 0;
    GtkWidget *menu = NULL;
    gchar *current = gmpc_profiles_get_current(gmpc_profiles);
    GList *iter, *mult;
    GtkUIManager *ui = GTK_UI_MANAGER(gtk_builder_get_object(pl3_xml, "uimanager1"));
    GtkMenuItem *m_item = GTK_MENU_ITEM(gtk_ui_manager_get_widget(ui, "/menubartest/menu_music/menu_profiles"));
    /* check if there is anything changed that is important for us. */

    if (changed == PROFILE_COL_CHANGED && col != PROFILE_COL_NAME)
    {
        g_free(current);
        return;
    }
    /***
     * Remove any old menu
     */
    gtk_menu_item_set_submenu(m_item, NULL);
    /**
     * Create a new menu
     */
    menu = gtk_menu_new();

    mult = gmpc_profiles_get_profiles_ids(gmpc_profiles);
    if (mult)
    {
        GSList *group = NULL;
        iter = mult;
        do
        {
            /** Get profile name */
            const gchar *value = gmpc_profiles_get_name(gmpc_profiles, (char *)iter->data);
            GtkWidget *item = gtk_radio_menu_item_new_with_label(group, value);
            /* get new group */
            group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(item));
            /* add to the menu */
            gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

            /* check the current profile */
            if (!strcmp((char *)(iter->data), current))
            {
                gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item), TRUE);
            }

            /**
             * Attach click handler
             */
            g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(pl3_profile_selected), NULL);

            /** Attach the uid to the handler */
            value = g_strdup((char *)(iter->data));
            g_object_set_data_full(G_OBJECT(item), "uid", (gpointer) value, g_free);

            items++;
        } while ((iter = g_list_next(iter)));
        g_list_foreach(mult, (GFunc) g_free, NULL);
        g_list_free(mult);

    }

    /**
     * Attach menu
     */
    if (items)
    {
        gtk_widget_show_all(menu);
        gtk_menu_item_set_submenu(m_item , menu);
        gtk_action_set_sensitive(GTK_ACTION(gtk_builder_get_object(pl3_xml, "menu_profiles")), TRUE);
    } else
    {
        gtk_action_set_sensitive(GTK_ACTION(gtk_builder_get_object(pl3_xml, "menu_profiles")), FALSE);
        g_object_ref_sink(menu);
        g_object_unref(menu);
    }
    g_free(current);
}


static void playlist3_server_output_changed(GtkWidget * item, gpointer data)
{
    int id = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(item), "id"));
    int state = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(item));
    mpd_server_set_output_device(connection, id, state);

}


void playlist3_server_update_db(void)
{
    mpd_database_update_dir(connection, "/");
}


static GList *server_menu_items = NULL;
static void playlist3_fill_server_menu(void)
{
    GtkUIManager *ui = GTK_UI_MANAGER(gtk_builder_get_object(pl3_xml, "uimanager1"));
    GtkMenuItem *m_item = GTK_MENU_ITEM(gtk_ui_manager_get_widget(ui, "/menubartest/menu_server"));
    /** Clear old items */
    if(server_menu_items != NULL)
    {
        g_list_foreach(server_menu_items, (GFunc)gtk_widget_destroy, NULL);
        g_list_free(server_menu_items);
        server_menu_items = NULL;
    }

    /* if connected fill with items */
    if (mpd_check_connected(connection))
    {
        GtkWidget *menu = gtk_menu_item_get_submenu(GTK_MENU_ITEM(m_item));
        GtkWidget *menu_item = NULL;
        int i = 0;
        MpdData *data = NULL;

        data = mpd_server_get_output_devices(connection);
        if(data)
        {
            menu_item = gtk_separator_menu_item_new();
            server_menu_items = g_list_append(server_menu_items, menu_item);
            gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
        }
        for (; data; data = mpd_data_get_next(data))
        {
            menu_item = gtk_check_menu_item_new_with_label(data->output_dev->name);
            server_menu_items = g_list_append(server_menu_items, menu_item);
            gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item), data->output_dev->enabled ? TRUE : FALSE);
            gtk_widget_add_accelerator(menu_item, "activate",
                gtk_ui_manager_get_accel_group(GTK_UI_MANAGER(ui)),
                GDK_1 + i, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

            g_signal_connect(G_OBJECT(menu_item), "toggled", G_CALLBACK(playlist3_server_output_changed), NULL);
            g_object_set_data(G_OBJECT(menu_item), "id", GINT_TO_POINTER(data->output_dev->id));
            gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
            i++;
        }
        gtk_widget_show_all(menu);
        /* Server Menu Item */
        gtk_action_set_sensitive(GTK_ACTION(gtk_builder_get_object(pl3_xml, "menu_server")), TRUE);
    } else
    {
        /* Server Menu Item */
        gtk_action_set_sensitive(GTK_ACTION(gtk_builder_get_object(pl3_xml, "menu_server")), FALSE);
    }
}


/**
 * new header
 */
/* glue code */

extern GmpcBrowsersMetadata *browsers_metadata;

void info2_activate(void)
{
    GtkTreeView *tree = (GtkTreeView *) gtk_builder_get_object(pl3_xml, "cat_tree");
    gmpc_browsers_metadata_select_browser(browsers_metadata, tree);
}


void info2_fill_song_view(mpd_Song * song)
{
    info2_activate();
    gmpc_browsers_metadata_set_song(browsers_metadata, song);
}


void info2_fill_artist_view(const gchar * artist)
{
    info2_activate();
    gmpc_browsers_metadata_set_artist(browsers_metadata, artist);
}


void info2_fill_album_view(const gchar * artist, const gchar * album)
{
    info2_activate();
    gmpc_browsers_metadata_set_album(browsers_metadata, artist, album);
}


void playlist3_insert_browser(GtkTreeIter * iter, gint position)
{
    GtkTreeIter it, *sib = NULL;
    gint pos = 0;
    GtkTreeModel *model = GTK_TREE_MODEL(pl3_tree);
    if (gtk_tree_model_get_iter_first(model, &it))
    {
        do
        {
            gtk_tree_model_get(model, &it, PL3_CAT_ORDER, &pos, -1);
            if (position <= pos)
                sib = &it;
        } while (sib == NULL && gtk_tree_model_iter_next(model, &it));
    }
    gtk_list_store_insert_before(GTK_LIST_STORE(pl3_tree), iter, sib);
    gtk_list_store_set(GTK_LIST_STORE(pl3_tree), iter, PL3_CAT_ORDER, position, PL3_CAT_BOLD, PANGO_WEIGHT_NORMAL, -1);
}


/**
 * Category editing
 */

void playlist3_destroy(void)
{
    GtkWidget *win = playlist3_get_window();
    if(server_menu_items) g_list_free(server_menu_items);
    gtk_widget_destroy(win);
    g_object_unref(pl3_xml);
}

GtkWidget *playlist3_get_window(void)
{
    return GTK_WIDGET(gtk_builder_get_object(pl3_xml, "pl3_win"));
}


/***
 * Help menu
 */
/* Make glade happy */
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


gmpcPrefPlugin playlist_gpp =
{
    .construct = playlist_pref_construct,
    .destroy = playlist_pref_destroy
};

gmpcPlugin playlist_plug =
{
    .name = N_("Interface"),
    .version = {1, 1, 1},
    .plugin_type = GMPC_INTERNALL,
    .mpd_status_changed = &playlist_status_changed,
    .mpd_connection_changed = &playlist_connection_changed,
    .pref = &playlist_gpp,
};


/**
 * Tool menu
 */

void pl3_tool_menu_update(void)
{
    int i;
    int menu_items = 0;
    GtkWidget *menu = NULL;
    GtkAccelGroup *group = gtk_accel_group_new();
    GtkUIManager *ui = GTK_UI_MANAGER(gtk_builder_get_object(pl3_xml, "uimanager1"));
    GtkMenuItem *m_item = GTK_MENU_ITEM(gtk_ui_manager_get_widget(ui, "/menubartest/menu_tool"));
    gtk_menu_item_set_submenu(m_item, NULL);
    gtk_action_set_sensitive(GTK_ACTION(gtk_builder_get_object(pl3_xml, "menu_tool")), FALSE);
    if (!mpd_check_connected(connection))
        return;

    menu = gtk_menu_new();
    gtk_menu_set_accel_group(GTK_MENU(menu), group);
    g_object_unref(group);
    gtk_window_add_accel_group(GTK_WINDOW(playlist3_get_window()), group);
    for (i = 0; i < num_plugins; i++)
    {
        menu_items += gmpc_plugin_tool_menu_integration(plugins[i], GTK_MENU(menu));
    }
    if (menu_items)
    {
        gtk_widget_show_all(menu);
        gtk_menu_item_set_submenu(m_item, menu);
        gtk_action_set_sensitive(GTK_ACTION(gtk_builder_get_object(pl3_xml, "menu_tool")), TRUE);
    } else
    {
        g_object_ref_sink(menu);
        g_object_unref(menu);
    }
}


void easy_command_help_window(void)
{
    if (gmpc_easy_command)
        gmpc_easy_command_help_window(gmpc_easy_command, NULL);
}



/**
 * Extra wrappings for menu
 */
extern gmpcPlugin extraplaylist_plugin;
void enable_extra_playlist(GtkToggleAction *action)
{
    gboolean state = gtk_toggle_action_get_active(action);
    if(extraplaylist_plugin.set_enabled)
    {
        if(extraplaylist_plugin.get_enabled() != state)
        {
            extraplaylist_plugin.set_enabled(state);
        }
    }
    preferences_window_update();
}


void init_extra_playlist_state(void)
{
    GtkToggleAction *action = GTK_TOGGLE_ACTION(gtk_builder_get_object(pl3_xml, "ViewExtraPlaylist"));
    if(extraplaylist_plugin.get_enabled)
    {
        gtk_toggle_action_set_active(action, extraplaylist_plugin.get_enabled());
    }
}

GtkAccelGroup *playlist3_get_accel_group(void)
{
	static GtkAccelGroup *group = NULL;
	if(group == NULL) {
		group = gtk_accel_group_new();
		gtk_window_add_accel_group(GTK_WINDOW(playlist3_get_window()), group);
	}
	return group;
}

void open_local_file(void)
{
	char **handlers;
	gboolean found = FALSE;
	if(!mpd_check_connected(connection))
		return;

	/* Check if MPD supports 'file://' (so local files). */
	handlers = mpd_server_get_url_handlers(connection);
	if (handlers)
	{
		int i=0;
		for(; !found && handlers != NULL && handlers[i] != NULL; i++) {
			if(g_utf8_collate(handlers[i], "file://") == 0) {
				found = TRUE;
			}
		}
		g_strfreev(handlers);
	}
	/* error message or file open */
	if(!found) {
		/* Message not found */
		GtkWidget *gmd = gtk_message_dialog_new(
					(GtkWindow*)playlist3_get_window(),
					GTK_DIALOG_MODAL,
					GTK_MESSAGE_ERROR,
					GTK_BUTTONS_CLOSE,
					_(
						"To playback local files, you need to be connected "
						"using unix socket.\n"
						"See the MPD website for more information."
					)
				);
		gtk_dialog_run(GTK_DIALOG(gmd));
		gtk_widget_destroy(GTK_WIDGET(gmd));
	}else {
		GtkWidget *fmd = gtk_file_chooser_dialog_new(
						"Select a local file",
						(GtkWindow*)playlist3_get_window(),
						GTK_FILE_CHOOSER_ACTION_OPEN,
						GTK_STOCK_CLOSE,
						GTK_RESPONSE_CLOSE,
						GTK_STOCK_OPEN,
						GTK_RESPONSE_OK,
						NULL);
		gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(fmd), TRUE);
		switch(gtk_dialog_run(GTK_DIALOG(fmd)))
		{
			case GTK_RESPONSE_OK:
			{
				GSList *iter;
				GSList *uris = gtk_file_chooser_get_uris(GTK_FILE_CHOOSER(fmd));
				for(iter = uris;
					iter != NULL;
					iter = g_slist_next(iter))
				{
					char *uri = g_uri_unescape_string(iter->data,NULL);
					url_start_real(uri);
					g_free(uri);
				}
                g_slist_foreach (uris, (GFunc) g_free, NULL);
                g_slist_free (uris);
				break;
			}
			default:
				break;
		}
		gtk_widget_destroy(GTK_WIDGET(fmd));
	}
}

void show_user_manual(void);
void show_user_manual(void)
{
	open_help("ghelp:gmpc");
}

GmpcPluginSidebarState playlist3_get_sidebar_state(void)
{
	gboolean st_shown;
	g_object_get(G_OBJECT(sidebar_text), "show_text", &st_shown, NULL);
	return  st_shown? GMPC_PLUGIN_SIDEBAR_STATE_FULL:GMPC_PLUGIN_SIDEBAR_STATE_COLLAPSED;
}
/* vim: set noexpandtab ts=4 sw=4 sts=4 tw=80: */
