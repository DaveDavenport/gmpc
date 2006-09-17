/*
 *Copyright (C) 2004-2006 Qball Cow <Qball@qballcow.nl>
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
#include "bacon-volume.h"
#include "main.h"
#include "misc.h"
#include "playlist3.h"
/* every part split out over multiple files */
#include "playlist3-find-browser.h"
#include "playlist3-file-browser.h"
#include "playlist3-artist-browser.h"
#include "playlist3-current-playlist-browser.h"
#include "revision.h"
#include "id3info.h"

static GtkTargetEntry target_table[] = {
        { "x-url/http", 0, 0 },
	{ "_NETSCAPE_URL", 0, 1},
	{ "text/uri-list", 0, 2}
};


enum {
	PLAYLIST_NO_ZOOM,
	PLAYLIST_SMALL,
	PLAYLIST_MINI,
	PLAYLIST_ZOOM_LEVELS
}PlaylistZoom;

static void playlist_zoom_level_changed(void);
static void playlist_player_volume_changed(BaconVolumeButton *vol_but);
void pl3_option_menu_activate(void);
static void pl3_plugin_changed_interface(void);

int pl3_zoom = PLAYLIST_NO_ZOOM;

/* Glade declarations, otherwise these would be static */
void about_window(void);
void pl3_cat_row_activated(GtkTreeView *, GtkTreePath *, GtkTreeViewColumn *);
void pl3_cat_row_expanded(GtkTreeView *, GtkTreeIter *, GtkTreePath *);
void pl3_cat_bread_crumb_up(void);
int pl3_cat_tree_button_press_event(GtkTreeView *, GdkEventButton *);
int pl3_cat_tree_button_release_event(GtkTreeView *, GdkEventButton *);
int pl3_window_key_press_event(GtkWidget *, GdkEventKey *);
int pl3_window_key_press_event(GtkWidget *, GdkEventKey *);
void playlist_zoom_in(void);
void playlist_zoom_out(void);
int pl3_cat_key_press_event(GtkWidget *, GdkEventKey *);
void cur_song_center_enable_tb(GtkToggleButton *);
void open_to_position_enable_tb(GtkToggleButton *);
void save_possize_enable_tb(GtkToggleButton *);
void set_browser_format(void);
void set_playlist_format(void);
void playlist_menu_repeat_changed(GtkCheckMenuItem *);
void playlist_menu_random_changed(GtkCheckMenuItem *);
void playlist_menu_cover_image_changed(GtkCheckMenuItem *);
int pl3_progress_seek_start(void);
int pl3_progress_seek_stop(void);
void playlist_player_cover_art_pressed(GtkEventBox *, GdkEventButton *);
gboolean pl3_close(void);

static int old_type = -1;

GladeXML *pl3_xml = NULL;
GtkTreeStore *pl3_tree = NULL;


/* size */
GtkAllocation pl3_wsize = { 0,0,0,0};
int pl3_hidden = TRUE;
static int pl3p_seek = FALSE;

static void playlist_status_changed(MpdObj *mi, ChangedStatusType what, void *userdata);
/* Playlist "Plugin" */
static void playlist_pref_construct(GtkWidget *container);
static void playlist_pref_destroy(GtkWidget *container);
GladeXML *playlist_pref_xml = NULL;

static GtkWidget *volume_slider = NULL;

gmpcPrefPlugin playlist_gpp = {
	playlist_pref_construct,
	playlist_pref_destroy
};

gmpcPlugin playlist_plug = {
	"Playlist",
	{1,1,1},
	GMPC_INTERNALL,
	0,
	NULL, /* initialize */
	NULL,
	NULL,
	&playlist_status_changed,
	NULL,
	&playlist_gpp,
	NULL,
	NULL,
	NULL
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
static void pl3_initialize_tree()
{
	int i;
	if(pl3_xml == NULL) return;

	GtkTreePath *path = gtk_tree_path_new_from_string("0");
	GtkTreeSelection *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(glade_xml_get_widget(pl3_xml, "cat_tree")));
	if(old_type >= 0)
	{
		GtkWidget *container = glade_xml_get_widget(pl3_xml, "browser_container");
		plugins[plugin_get_pos(old_type)]->browser->unselected(container);
		old_type = -1;
	}

	gtk_tree_store_clear(pl3_tree);

	for(i=0; i< num_plugins;i++)
	{
		if(plugins[i]->plugin_type&GMPC_PLUGIN_PL_BROWSER)
		{
			if(plugins[i]->browser && plugins[i]->browser->add)
			{
				plugins[i]->browser->add(glade_xml_get_widget(pl3_xml, "cat_tree"));
			}
		}
	}


	gtk_tree_selection_select_path(sel, path);
	gtk_tree_view_set_cursor(GTK_TREE_VIEW(glade_xml_get_widget(pl3_xml, "cat_tree")),
			path, NULL, FALSE);
	gtk_tree_path_free(path);
}

void pl3_cat_row_activated(GtkTreeView *tree, GtkTreePath *tp, GtkTreeViewColumn *col)
{
	gint type = pl3_cat_get_selected_browser();
	if(!mpd_check_connected(connection) || type == -1)
	{
		return;
	}

	if(gtk_tree_view_row_expanded(tree,tp))
	{
		gtk_tree_view_collapse_row(tree,tp);
	}
	else
	{
		gtk_tree_view_expand_row(tree,tp,FALSE);
	}
}


void pl3_cat_row_expanded(GtkTreeView *tree, GtkTreeIter *iter, GtkTreePath *path)
{
	gint type,read;
	gtk_tree_model_get(GTK_TREE_MODEL(pl3_tree), iter, 0, &type,4,&read, -1);
	/* check if the connection isnt down */
	if(!mpd_check_connected(connection))
	{
		/* if connection down, don't let the treeview open */
		gtk_tree_view_collapse_row(tree,path);
		return;
	}
	if(!read)
	{
		if(plugins[plugin_get_pos(type)]->browser != NULL)
		{
			if(plugins[plugin_get_pos(type)]->browser->cat_row_expanded != NULL)
			{
				plugins[plugin_get_pos(type)]->browser->cat_row_expanded(GTK_WIDGET(tree), iter);
			}
		}
	}
	/* avuton's Idea */
	if(cfg_get_single_value_as_int_with_default(config, "playlist", "open-to-position",0))
	{
		gtk_tree_view_scroll_to_cell(tree, path,gtk_tree_view_get_column(tree,0),TRUE,0.5,0);
	}
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
		gtk_tree_model_get(gtk_combo_box_get_model(box), &iter, 2, &path, -1);
		if(gtk_tree_model_get_iter(GTK_TREE_MODEL(pl3_tree), &cat_iter, path))
		{
			GtkTreeIter piter;
			if(gtk_tree_selection_get_selected(selec,&model, &piter))
			{
				GtkTreePath *ppath = gtk_tree_model_get_path(model, &piter);
				if(ppath && gtk_tree_path_is_descendant(ppath, path))
				{
					gtk_tree_path_free(ppath);
					return;

				}
				gtk_tree_path_free(ppath);
			}	
			if(gtk_tree_path_get_depth(path)>0)
			{
				GtkTreeView *tree = (GtkTreeView *) glade_xml_get_widget (pl3_xml, "cat_tree");
				if(!gtk_tree_selection_iter_is_selected(selec, &cat_iter))
				{
					gtk_tree_view_expand_to_path(tree, path);
					gtk_tree_selection_select_iter(selec, &cat_iter);
					/**
					 * Hopefully a fix
					 */
					if(pl3_zoom == PLAYLIST_NO_ZOOM)
					{
						gtk_tree_view_scroll_to_cell(tree, path, 
								NULL,TRUE, 
								0.5, 0);
					}
				}
			}
		}
	}
}

void pl3_cat_bread_crumb_up()
{
	GtkTreeModel *model = GTK_TREE_MODEL(pl3_tree);
	GtkTreeIter iter,parent;
	GtkTreeSelection *selec = gtk_tree_view_get_selection((GtkTreeView *)glade_xml_get_widget (pl3_xml, "cat_tree"));

	if(gtk_tree_selection_get_selected(selec,&model, &iter))
	{
		if(gtk_tree_model_iter_parent(model, &parent, &iter))
		{
			gtk_tree_selection_select_iter(selec, &parent);
		}
	}
}

/**
 * Function to handle a change in category.
 */
void pl3_cat_sel_changed()
{
	GtkTreeModel *model = GTK_TREE_MODEL(pl3_tree);
	GtkTreeIter iter;
	GtkTreeSelection *selec = gtk_tree_view_get_selection((GtkTreeView *)glade_xml_get_widget (pl3_xml, "cat_tree"));
	GtkTreeView *tree = (GtkTreeView *) glade_xml_get_widget (pl3_xml, "cat_tree");

	GtkWidget *container = glade_xml_get_widget(pl3_xml, "browser_container");

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
		if(old_type != type )
		{
			if(old_type != -1)
			{
				plugins[plugin_get_pos(old_type)]->browser->unselected(container);
			}
			old_type = -1;
		}
		pl3_push_rsb_message("");
		/** if type changed give a selected signal */
		if(old_type != type)plugins[plugin_get_pos(type)]->browser->selected(container);
		/**
		 * update old value, so get_selected_category is correct before calling selection_changed
		 */
		old_type = type;
		/**
		 * now give a selection changed signal 
		 */
		if(plugins[plugin_get_pos(type)]->browser->cat_selection_changed)
		{
			plugins[plugin_get_pos(type)]->browser->cat_selection_changed(GTK_WIDGET(tree),&iter);
		}
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

	gtk_menu_item_remove_submenu(GTK_MENU_ITEM(glade_xml_get_widget(pl3_xml, "menu_option")));
	
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
	}
	else{
		gtk_widget_destroy(menu);
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
		gtk_menu_popup(GTK_MENU(menu), NULL, NULL,NULL, NULL, event->button, event->time);
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
int pl3_window_key_press_event(GtkWidget *mw, GdkEventKey *event)
{
	int i=0;
	gint type = pl3_cat_get_selected_browser();
	/**
	 * Handle Zoom In Key
	 * (Ctrl-+)
	 */
	if((event->keyval == GDK_plus && event->state&GDK_CONTROL_MASK) || event->keyval == GDK_KP_Add)
	{
		playlist_zoom_in();
		return TRUE;
	}
	/**
	 * Handle Zoom Out Key
	 * (Ctrl--)
	 */
	if((event->keyval == GDK_minus && event->state&GDK_CONTROL_MASK) || event->keyval == GDK_KP_Subtract) 	
	{
		playlist_zoom_out();
		return TRUE;
	}                                                          	
	/**
	 * Fullscreen 
	 */
	if(event->keyval == GDK_F12)
	{
		GtkWidget *win = glade_xml_get_widget(pl3_xml, "pl3_win");
		GdkWindowState state = gdk_window_get_state(win->window);	
		if(state&GDK_WINDOW_STATE_FULLSCREEN)
		{
			gtk_window_unfullscreen(GTK_WINDOW(win));
		}
		else{
			gtk_window_fullscreen(GTK_WINDOW(win));
		}
	}
	/**
	 * Close the window on ctrl-w
	 * or Ctrl Q
	 */
	if ((event->keyval == GDK_w || event->keyval == GDK_q)&& event->state == GDK_CONTROL_MASK)
	{
		pl3_close();
		return TRUE;
	}

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

	/* default gmpc/xmms/winamp key's*/
	if (event->state&GDK_CONTROL_MASK && event->keyval == GDK_z )
	{
		prev_song();
	}
	else if (event->state&GDK_CONTROL_MASK && (event->keyval == GDK_x || event->keyval == GDK_c))
	{
		play_song();
	}
	else if (event->state&GDK_CONTROL_MASK && event->keyval == GDK_v)
	{
		stop_song();
	}
	else if (event->state&GDK_CONTROL_MASK && event->keyval == GDK_b)
	{
		next_song();
	}
	else
	{
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
	g_timeout_add(5000,(GSourceFunc)pl3_pop_statusbar_message, GINT_TO_POINTER(id));
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
	if(pl3_xml != NULL && pl3_zoom <= PLAYLIST_SMALL)
	{
		gtk_window_get_position(GTK_WINDOW(glade_xml_get_widget(pl3_xml, "pl3_win")), &pl3_wsize.x, &pl3_wsize.y);
		gtk_window_get_size(GTK_WINDOW(glade_xml_get_widget(pl3_xml, "pl3_win")), &pl3_wsize.width, &pl3_wsize.height);

		cfg_set_single_value_as_int(config, "playlist", "xpos", pl3_wsize.x);
		cfg_set_single_value_as_int(config, "playlist", "ypos", pl3_wsize.y);
		cfg_set_single_value_as_int(config, "playlist", "width", pl3_wsize.width);
		cfg_set_single_value_as_int(config, "playlist", "height", pl3_wsize.height);
		cfg_set_single_value_as_int(config, "playlist", "pane-pos", gtk_paned_get_position(
					GTK_PANED(glade_xml_get_widget(pl3_xml, "hpaned1"))));
	}

	if(cfg_get_single_value_as_int_with_default(config, "playlist", "hide-on-close", FALSE))
	{
		if(tray_availible())
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
	if(!tray_availible())
		return 1;
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



void pl3_updating_changed(MpdObj *mi, int updating)
{
	char *mesg = "MPD database is updating";
	if(pl3_xml != NULL)
	{
		gint id = gtk_statusbar_get_context_id(GTK_STATUSBAR(glade_xml_get_widget(pl3_xml, "statusbar1")), mesg);

		if(updating >0)
		{
			gtk_statusbar_push(GTK_STATUSBAR(glade_xml_get_widget(pl3_xml, "statusbar1")), id,mesg);
			gtk_widget_show(glade_xml_get_widget(pl3_xml, "image_updating"));
		}
		else
		{
			gtk_statusbar_pop(GTK_STATUSBAR(glade_xml_get_widget(pl3_xml, "statusbar1")), id);
			gtk_widget_hide(glade_xml_get_widget(pl3_xml, "image_updating"));
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
		gtk_window_resize(GTK_WINDOW(glade_xml_get_widget(pl3_xml, "pl3_win")),
				pl3_wsize.width,
				pl3_wsize.height);
	}
	gtk_widget_show(glade_xml_get_widget(pl3_xml, "pl3_win"));
	gtk_window_present(GTK_WINDOW(glade_xml_get_widget(pl3_xml, "pl3_win")));

}
void pl3_show_window()
{
	if(!pl3_xml) return;
	if(!pl3_hidden)
	{
		gtk_widget_show(glade_xml_get_widget(pl3_xml, "pl3_win"));
		gtk_window_present(GTK_WINDOW(glade_xml_get_widget(pl3_xml, "pl3_win")));
	}
	else
	{
		create_playlist3();
	}
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

/**
 * Sync the lowest level of the cat_tree with the crumb system
 */
static void pl3_tree_row_inserted(GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, GtkTreeModel *model2)
{
	if(gtk_tree_path_get_depth(path) == 1)
	{
		GtkTreeIter citer;
		char *name, *stock_id;
		gtk_tree_model_get(model, iter, PL3_CAT_TITLE, &name, PL3_CAT_ICON_ID, &stock_id, -1);
		gtk_list_store_append(GTK_LIST_STORE(model2), &citer);
		gtk_list_store_set(GTK_LIST_STORE(model2), &citer, 0,name,1,stock_id,-1);
	}
}

static void pl3_tree_row_changed(GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, GtkTreeModel *model2)
{
	if(gtk_tree_path_get_depth(path) == 1)
	{
		GtkTreeIter citer;
		char *name, *stock_id,*strpath;
		gtk_tree_model_get(model, iter, PL3_CAT_TITLE, &name, PL3_CAT_ICON_ID, &stock_id, -1);
		strpath = gtk_tree_path_to_string(path);
		gtk_tree_model_get_iter_from_string(model2,&citer,strpath);
		gtk_list_store_set(GTK_LIST_STORE(model2), &citer, 0,name,1,stock_id,2,gtk_tree_path_copy(path),-1);
		g_free(strpath);
	}
}
static void pl3_tree_row_deleted(GtkTreeModel *model, GtkTreePath *path, GtkTreeModel *model2)
{
	if(gtk_tree_path_get_depth(path) == 1)
	{
		GtkTreeIter citer;
		GtkTreePath *cpath;
		char *strpath;
		strpath = gtk_tree_path_to_string(path);
		gtk_tree_model_get_iter_from_string(model2,&citer,strpath);
		gtk_tree_model_get(model2, &citer, 2,&cpath, -1);
		if(cpath)
			gtk_tree_path_free(cpath);
		gtk_list_store_remove(GTK_LIST_STORE(model2), &citer);
		g_free(strpath);
	}

}
static void playlist3_source_drag_data_recieved (GtkWidget          *widget,
                            GdkDragContext     *context,
                            gint                x,
                            gint                y,
                            GtkSelectionData   *data,
                            guint               info,
                            guint               time)
{
	gchar **url = g_strsplit(data->data,"\n", -1);
	printf("%i %s\n",info,data->data);
		
	
	gtk_drag_finish(context, TRUE, FALSE, time);
	url_start_real(url[0]);
	g_strfreev(url);
}

void create_playlist3 ()
{
	GtkListStore *pl3_crumbs = NULL;

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
	/* load gui desciption */
	path = gmpc_get_full_glade_path("playlist3.glade");
	pl3_xml = glade_xml_new (path, "pl3_win", NULL);
	g_free(path);
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

	/* create tree store for the "category" view */
	if (pl3_tree == NULL)
	{
		/* song id, song title */
		pl3_tree = gtk_tree_store_new (PL3_CAT_NROWS, 
				G_TYPE_INT,	/* row type, see free_type struct */
				G_TYPE_STRING, /* display name */
				G_TYPE_STRING,/* full path and stuff for backend */
				G_TYPE_STRING, /* icon id */
				G_TYPE_BOOLEAN,  /* cat proc */
				G_TYPE_UINT,  /* icon size */
				G_TYPE_STRING /* browser markup */
				);
	}

	tree = glade_xml_get_widget (pl3_xml, "cat_tree");
	gtk_tree_view_set_model (GTK_TREE_VIEW (tree), GTK_TREE_MODEL (pl3_tree));

	renderer = gtk_cell_renderer_pixbuf_new ();
	column = gtk_tree_view_column_new ();
	gtk_tree_view_column_pack_start (column, renderer, FALSE);
	gtk_tree_view_column_set_attributes (column, renderer, "stock-id",3,"stock-size",5,NULL);

	renderer = gtk_cell_renderer_text_new ();
	/* insert the column in the tree */
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_set_attributes (column, renderer, "text", 1, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);
	gtk_tree_view_set_search_column(GTK_TREE_VIEW(tree), 1);
	sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));
	g_signal_connect(G_OBJECT(sel), "changed", G_CALLBACK(pl3_cat_sel_changed), NULL);

	/* Make sure the scroll bars get removed when folding in the folders again */
	gtk_tree_view_column_set_sizing(column,GTK_TREE_VIEW_COLUMN_AUTOSIZE);



	/**
	 * Bread Crumb system.
	 */
	pl3_crumbs = gtk_list_store_new(3, 
			G_TYPE_STRING, /* text */
			G_TYPE_STRING, /* stock id */
			G_TYPE_POINTER /* Tree Path */
			);

	gtk_combo_box_set_model(GTK_COMBO_BOX(glade_xml_get_widget(pl3_xml, "cb_cat_selector")), 
			GTK_TREE_MODEL(pl3_crumbs));
	renderer = gtk_cell_renderer_pixbuf_new ();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(glade_xml_get_widget(pl3_xml, "cb_cat_selector")),renderer,FALSE); 
	gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(glade_xml_get_widget(pl3_xml, "cb_cat_selector")),renderer,
			"stock-id", 1);                                                                                          	

	renderer = gtk_cell_renderer_text_new ();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(glade_xml_get_widget(pl3_xml, "cb_cat_selector")),renderer,TRUE); 
	gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(glade_xml_get_widget(pl3_xml, "cb_cat_selector")),renderer,
			"text", 0);

	g_signal_connect(glade_xml_get_widget(pl3_xml, "cb_cat_selector"),
			"changed", G_CALLBACK(pl3_cat_combo_changed), NULL);
	g_signal_connect(G_OBJECT(pl3_tree), "row-inserted", G_CALLBACK(pl3_tree_row_inserted),pl3_crumbs);
	g_signal_connect(G_OBJECT(pl3_tree), "row-changed", G_CALLBACK(pl3_tree_row_changed),pl3_crumbs);
	g_signal_connect(G_OBJECT(pl3_tree), "row-deleted", G_CALLBACK(pl3_tree_row_deleted),pl3_crumbs);
	/* initialize the category view */ 
	pl3_initialize_tree();


	gtk_widget_show(glade_xml_get_widget(pl3_xml, "vbox_playlist_player"));

	/* Add volume slider. */
	volume_slider = bacon_volume_button_new(GTK_ICON_SIZE_BUTTON, 0, 100, 1);
	gtk_box_pack_end(GTK_BOX(glade_xml_get_widget(pl3_xml, "hbox12"/*playlist_player"*/)), volume_slider, FALSE, TRUE, 0);
	gtk_widget_show_all(volume_slider);
	playlist_status_changed(connection, MPD_CST_STATE|MPD_CST_SONGID|MPD_CST_ELAPSED_TIME|MPD_CST_VOLUME|MPD_CST_REPEAT|MPD_CST_RANDOM|MPD_CST_PERMISSION,NULL);
	g_signal_connect(G_OBJECT(volume_slider), "value_changed", G_CALLBACK(playlist_player_volume_changed), NULL);

	/* Restore values from config */
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(glade_xml_get_widget(pl3_xml, "menu_check_cover_image")),
			cfg_get_single_value_as_int_with_default(config, "playlist", "cover-image-enable", 0));

	/* Make sure change is applied */


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
		pl3_zoom = cfg_get_single_value_as_int_with_default(config, "playlist","zoomlevel",PLAYLIST_NO_ZOOM);
		playlist_zoom_level_changed();
		gtk_widget_show(glade_xml_get_widget(pl3_xml, "pl3_win"));
	}
	else
	{
		gtk_widget_show(glade_xml_get_widget(pl3_xml, "pl3_win"));
	}
	/**
	 * Insert new custom widget
	 */
	gmpc_metaimage_set_image_type(GMPC_METAIMAGE(glade_xml_get_widget(pl3_xml, "metaimage_album_art")), META_ALBUM_ART);
	gmpc_metaimage_set_connection(GMPC_METAIMAGE(glade_xml_get_widget(pl3_xml, "metaimage_album_art")), connection);
/*	gmpc_metaimage_set_draw_shadow(GMPC_METAIMAGE(glade_xml_get_widget(pl3_xml, "metaimage_album_art")),TRUE);*/
	gtk_widget_set_size_request(glade_xml_get_widget(pl3_xml, "metaimage_album_art"),70,70);

	gmpc_metaimage_set_image_type(GMPC_METAIMAGE(glade_xml_get_widget(pl3_xml, "metaimage_artist_art")), META_ARTIST_ART);
	gmpc_metaimage_set_hide_on_na(GMPC_METAIMAGE(glade_xml_get_widget(pl3_xml, "metaimage_artist_art")), TRUE);
	if(cfg_get_single_value_as_int_with_default(config, "playlist", "cover-image-enable", FALSE))
	{
		gmpc_metaimage_set_connection(GMPC_METAIMAGE(glade_xml_get_widget(pl3_xml, "metaimage_artist_art")), connection);
	}
	gmpc_metaimage_set_size(GMPC_METAIMAGE(glade_xml_get_widget(pl3_xml, "metaimage_artist_art")), 200);
	gtk_widget_hide(glade_xml_get_widget(pl3_xml, "metaimage_artist_art"));
	
	pl3_update_go_menu();	
	pl3_update_profiles_menu();

	/**
	 * Set as drag destination
	 */
	gtk_drag_dest_set(glade_xml_get_widget(pl3_xml, "pl3_win"),
		GTK_DEST_DEFAULT_ALL,
		target_table, 3,
		GDK_ACTION_COPY|GDK_ACTION_LINK|GDK_ACTION_DEFAULT|GDK_ACTION_MOVE);
	g_signal_connect (G_OBJECT (glade_xml_get_widget(pl3_xml, "pl3_win")),"drag_data_received",
			GTK_SIGNAL_FUNC (playlist3_source_drag_data_recieved),
			NULL);


}

/**
 * Helper functions
 */
gboolean playlist3_get_active()
{
	return (pl3_xml != NULL);
}
GtkTreeStore *playlist3_get_category_tree_store()
{
	if(!playlist3_get_active()) return NULL;
	return pl3_tree;
}
GtkTreeView *playlist3_get_category_tree_view()
{
	if(!playlist3_get_active()) return NULL;
	return (GtkTreeView *)glade_xml_get_widget(pl3_xml, "cat_tree");
} 

/****************************************************************************************
 *  PREFERENCES										*
 ****************************************************************************************/

void cur_song_center_enable_tb(GtkToggleButton *but)
{
	int bool1  = gtk_toggle_button_get_active(but);
	cfg_set_single_value_as_int(config, "playlist","st_cur_song", bool1);
}
void open_to_position_enable_tb(GtkToggleButton *but)
{
	int bool1  = gtk_toggle_button_get_active(but);
	cfg_set_single_value_as_int(config, "playlist","open-to-position", bool1);
}
void save_possize_enable_tb(GtkToggleButton *but)
{
	int bool1  = gtk_toggle_button_get_active(but);
	cfg_set_single_value_as_int(config, "playlist","savepossize", bool1);
}
void set_browser_format()
{
	char *string = cfg_get_single_value_as_string_with_default(config, "playlist", "browser_markup",DEFAULT_MARKUP_BROWSER);
	char *format = edit_song_markup(string);
	cfg_free_string(string);
	if(format != NULL)
	{
		cfg_set_single_value_as_string(config, "playlist","browser_markup",format);
	}
	g_free(format);
}

void set_playlist_format()
{
	char *string = cfg_get_single_value_as_string_with_default(config, "playlist", "markup",DEFAULT_PLAYLIST_MARKUP);
	char *format = edit_song_markup(string);
	cfg_free_string(string);
	if(format != NULL)
	{
		cfg_set_single_value_as_string(config, "playlist","markup",format);
		playlist_list_set_markup(PLAYLIST_LIST(playlist),format);
	}
	g_free(format);
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
				GTK_TOGGLE_BUTTON(glade_xml_get_widget(playlist_pref_xml,"ck_of")),
				cfg_get_single_value_as_int_with_default(config,"playlist", "open-to-position", 0));
		gtk_toggle_button_set_active(
				GTK_TOGGLE_BUTTON(glade_xml_get_widget(playlist_pref_xml, "ck_ps")),
				cfg_get_single_value_as_int_with_default(config,"playlist", "st_cur_song", 0));
		gtk_toggle_button_set_active(
				GTK_TOGGLE_BUTTON(glade_xml_get_widget(playlist_pref_xml, "ck_possize")),
				cfg_get_single_value_as_int_with_default(config,"playlist", "savepossize", 0));

		gtk_container_add(GTK_CONTAINER(container),vbox);
		glade_xml_signal_autoconnect(playlist_pref_xml);
	}
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


		/**
		 * Set markup
		 */
		gtk_label_set_markup(GTK_LABEL(glade_xml_get_widget(pl3_xml,"pp_label")),
				buffer);
		gtk_label_set_markup(GTK_LABEL(glade_xml_get_widget(pl3_xml,"pp_label_mini")),
				buffer);
	}
	else
	{
		/**
		 * When not playing set "not playlist
		 */
		gtk_label_set_markup(GTK_LABEL(glade_xml_get_widget(pl3_xml,"pp_label")),
				_("<span size=\"large\" weight=\"bold\">Not Playing</span>"));

		gtk_label_set_markup(GTK_LABEL(glade_xml_get_widget(pl3_xml,"pp_label_mini")),
				_("<span size=\"large\" weight=\"bold\">Not Playing</span>"));		
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
	if(active)
	{
		gmpc_metaimage_set_connection(GMPC_METAIMAGE(glade_xml_get_widget(pl3_xml, "metaimage_artist_art")), connection);
		gmpc_metaimage_update_cover(GMPC_METAIMAGE(glade_xml_get_widget(pl3_xml, "metaimage_artist_art")), connection,MPD_CST_SONGID,gmpcconn);
	}
	else{
		gmpc_metaimage_set_connection(GMPC_METAIMAGE(glade_xml_get_widget(pl3_xml, "metaimage_artist_art")), NULL);
		gmpc_metaimage_set_cover_na(GMPC_METAIMAGE(glade_xml_get_widget(pl3_xml, "metaimage_artist_art")));
	}
}

/***
 * Zooming functions
 */
void playlist_zoom_out()
{
	if((pl3_zoom+1) >= PLAYLIST_ZOOM_LEVELS) return;
	pl3_zoom++;
	playlist_zoom_level_changed();
}
void playlist_zoom_in()
{
	if(pl3_zoom <= PLAYLIST_NO_ZOOM) return;
	pl3_zoom--;
	playlist_zoom_level_changed();
}

/**
 * FIXME: Needs propper grouping and cleaning up
 */
static void playlist_zoom_level_changed()
{
	/* Show full view */
	gtk_widget_show(glade_xml_get_widget(pl3_xml, "hpaned1"));
	gtk_widget_show(glade_xml_get_widget(pl3_xml, "hbox1"));
	gtk_widget_hide(glade_xml_get_widget(pl3_xml, "pp_label_mini"));
	gtk_widget_show(glade_xml_get_widget(pl3_xml, "pp_label"));
	gtk_widget_show(glade_xml_get_widget(pl3_xml, "hseparator1"));
	/** Menu Bar */
	gtk_widget_show(glade_xml_get_widget(pl3_xml, "menubar1"));
	/** BUTTON BOX */
	gtk_widget_show(glade_xml_get_widget(pl3_xml, "pp_progres_label"));
	gtk_widget_show(glade_xml_get_widget(pl3_xml, "pl3_button_control_box"));



	gtk_window_set_resizable(GTK_WINDOW(glade_xml_get_widget(pl3_xml, "pl3_win")), TRUE);
	if(pl3_wsize.width > 0 && pl3_wsize.height >0)
	{
		gtk_window_resize(GTK_WINDOW(glade_xml_get_widget(pl3_xml, "pl3_win")),	pl3_wsize.width, pl3_wsize.height);
	}
	gtk_widget_show(glade_xml_get_widget(pl3_xml, "vbox5"));
	gtk_widget_hide(glade_xml_get_widget(pl3_xml, "bread_crumb"));


	/* Now start hiding */
	switch(pl3_zoom)
	{
		case PLAYLIST_NO_ZOOM:
			break;
		case PLAYLIST_MINI:
			gtk_window_set_resizable(GTK_WINDOW(glade_xml_get_widget(pl3_xml, "pl3_win")), FALSE);
			gtk_widget_hide(glade_xml_get_widget(pl3_xml, "hbox1"));
			gtk_widget_show(glade_xml_get_widget(pl3_xml, "pp_label_mini"));
			gtk_widget_hide(glade_xml_get_widget(pl3_xml, "pp_label"));     			
			gtk_widget_hide(glade_xml_get_widget(pl3_xml, "hseparator1"));
			gtk_widget_hide(glade_xml_get_widget(pl3_xml, "hpaned1"));
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
void playlist_connection_changed(MpdObj *mi, int connect)
{
	/* Set menu items */
	gtk_widget_set_sensitive(glade_xml_get_widget(pl3_xml, "menu_connect"), !connect);
	gtk_widget_set_sensitive(glade_xml_get_widget(pl3_xml, "menu_disconnect"), connect);
	gtk_widget_set_sensitive(glade_xml_get_widget(pl3_xml, "menuitem_sendpassword"), connect);


	/** Set back to the current borwser, and update window title */
	if(connect){
		gchar *string = NULL;
		GtkTreeSelection *selec = gtk_tree_view_get_selection((GtkTreeView *)
				glade_xml_get_widget (pl3_xml, "cat_tree"));
		GtkTreeModel *model = GTK_TREE_MODEL(pl3_tree);                      		
		GtkTreeIter iter;
		if(gtk_tree_model_get_iter_first(model, &iter)){
			gtk_tree_selection_select_iter(selec, &iter);
		}
		string = g_strdup_printf("%s - %s %s",
				_("GMPC"), 
				_("Connected to"),
				mpd_get_hostname(mi));
		gtk_window_set_title(GTK_WINDOW(glade_xml_get_widget(pl3_xml, "pl3_win")), string);
		g_free(string);
	}
	else{
		gchar *string = NULL;
		string = g_strdup_printf("%s - %s",
				_("GMPC"), 
				_("Disconnected"));
		gtk_window_set_title(GTK_WINDOW(glade_xml_get_widget(pl3_xml, "pl3_win")), string);		
		g_free(string);                                                                    	
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
				mpd_song_markup(buffer, 1024,"[%title% - &[%artist%]]|%shortfile%", mpd_playlist_get_current_song(connection));
				gtk_window_set_title(GTK_WINDOW(glade_xml_get_widget(pl3_xml, "pl3_win")), buffer);		

				break;
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
				gtk_label_set_markup(GTK_LABEL
						(glade_xml_get_widget(pl3_xml,"pp_label")),
						"<span size=\"large\" weight=\"bold\">Paused</span>");

				gtk_label_set_markup(GTK_LABEL
						(glade_xml_get_widget(pl3_xml,"pp_label_mini")),
						"<span size=\"large\" weight=\"bold\">Paused</span>");				
				mpd_song_markup(buffer, 1024,"[%title% - &[%artist%] (paused)]|%shortfile% (paused)", mpd_playlist_get_current_song(connection));
				gtk_window_set_title(GTK_WINDOW(glade_xml_get_widget(pl3_xml, "pl3_win")), buffer);		
				break;
			default:
				image = gtk_image_menu_item_get_image(GTK_IMAGE_MENU_ITEM(glade_xml_get_widget(pl3_xml, "menu_play")));
				gtk_image_set_from_stock(GTK_IMAGE(image), "gtk-media-play", GTK_ICON_SIZE_MENU);


				gtk_image_set_from_stock(GTK_IMAGE(
							glade_xml_get_widget(pl3_xml, "pp_but_play_img")),
						"gtk-media-play",GTK_ICON_SIZE_BUTTON);
				gtk_label_set_markup(GTK_LABEL
						(glade_xml_get_widget(pl3_xml,"pp_label")),
						"<span size=\"large\" weight=\"bold\">Not Playing</span>");

				gtk_label_set_markup(GTK_LABEL
						(glade_xml_get_widget(pl3_xml,"pp_label_mini")),
						"<span size=\"large\" weight=\"bold\">Not Playing</span>");				

				gtk_window_set_title(GTK_WINDOW(glade_xml_get_widget(pl3_xml, "pl3_win")), _("GMPC"));		
		}
	}
	/**
	 * Handle song change or Playlist change
	 * Anything that can change metadta
	 */
	if(what&MPD_CST_SONGID || what&MPD_CST_SONGPOS || what&MPD_CST_PLAYLIST)
	{
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
			char *string = g_strdup_printf("Repeat: %s", (mpd_player_get_repeat(connection))? _("On"):_("Off"));
			pl3_push_statusbar_message(string);
			g_free(string);

			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(glade_xml_get_widget(pl3_xml, "menu_repeat")),
					mpd_player_get_repeat(connection));
		}

	}
	if(what&MPD_CST_RANDOM)
	{
		if(mpd_check_connected(connection))
		{
			char *string = g_strdup_printf("Random: %s", (mpd_player_get_random(connection))? _("On"):_("Off"));
			pl3_push_statusbar_message(string);
			g_free(string);

			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(glade_xml_get_widget(pl3_xml, "menu_random")),
					mpd_player_get_random(connection));
		}
	}                                                                                                        	
	if(what&MPD_CST_ELAPSED_TIME)
	{
		char *string = NULL;
		if(mpd_check_connected(connection))
		{
			int totalTime = mpd_status_get_total_song_time(connection);
			int elapsedTime = mpd_status_get_elapsed_song_time(connection);			
			if(!pl3p_seek)
			{
				gtk_range_set_value(GTK_RANGE(glade_xml_get_widget(pl3_xml, "pp_progres")),
						(elapsedTime/(float)totalTime)*100.0);
			}
			if(totalTime == 0)
			{
				string = g_strdup("");

			}
			else if(elapsedTime/60 >99 || totalTime/60 > 99)
			{
				string = g_strdup_printf("%02i:%02i - %02i:%02i",
						(elapsedTime/3600),
						(elapsedTime/60)%60,
						(totalTime/3600),
						(totalTime/60)%60
						);
			}
			else{
				string = g_strdup_printf("%02i:%02i - %02i:%02i",
						(elapsedTime/60),
						elapsedTime%60,
						(totalTime/60),
						(totalTime%60)
						);
			}
		}
		else
		{
			string = g_strdup(_("Not Connected"));
			gtk_range_set_value(GTK_RANGE(glade_xml_get_widget(pl3_xml, "pp_progres")),0);
		}

		gtk_label_set_text(GTK_LABEL(glade_xml_get_widget(pl3_xml, "pp_progres_label")),
				string);
		g_free(string);
	}
	if(what&MPD_CST_VOLUME)
	{
		bacon_volume_button_set_value(BACON_VOLUME_BUTTON(volume_slider),mpd_status_get_volume(connection));

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
}


/* start seeking in the song..  only allow this when you're playing or paused */
/* block it other wise. */
/* everything is blocked until the seek is done. */
/* show time to seek to in entry box */
int pl3_progress_seek_start()
{
	if(mpd_player_get_state(connection) != MPD_PLAYER_PLAY && 
			mpd_player_get_state(connection) != MPD_PLAYER_PAUSE)
	{
		return TRUE;
	}
	pl3p_seek = TRUE;
	return FALSE;
}


/* apply seek changes */
int pl3_progress_seek_stop()
{
	pl3p_seek = FALSE;
	if(!mpd_check_connected(connection))
	{
		return TRUE;
	}
	else if(mpd_player_get_state(connection) == MPD_PLAYER_PLAY || mpd_player_get_state(connection) == MPD_PLAYER_PAUSE)
	{
		GtkRange *scale = (GtkRange *)glade_xml_get_widget(pl3_xml, "pp_progres");
		gdouble value = gtk_range_get_value(scale);
		if(value >=0)
		{
			int change = (int)(mpd_status_get_total_song_time(connection)*(double)(value/100));
			mpd_player_seek(connection, change);
		}
	}
	return FALSE;
}

void playlist_player_cover_art_pressed(GtkEventBox *event_widget, GdkEventButton *event)
{
	int state = mpd_player_get_state(connection);
	if(state == MPD_PLAYER_STOP || state == MPD_PLAYER_UNKNOWN) return;
	if(!mpd_check_connected(connection)) return;
	call_id3_window(mpd_player_get_current_song_id(connection));
}

static void playlist_player_volume_changed(BaconVolumeButton *vol_but)
{
	int volume = bacon_volume_button_get_value(vol_but);
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
	g_free(path);

	if(strlen(revision))
	{
		path = g_strdup_printf("%s\nRevision: %s", VERSION, revision);
	}
	else
	{
		path = g_strdup_printf("%s\n", VERSION);
	}

	gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(dialog),path); 
	g_free(path);
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
	/***
	 * Remove any old menu
	 */
	gtk_menu_item_remove_submenu(GTK_MENU_ITEM(glade_xml_get_widget(pl3_xml, "menu_go")));
	/**
	 * Create a new menu
	 */
	menu = gtk_menu_new();

	if(mpd_check_connected(connection)) 
	{
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
			printf("no uid found\n");
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

void pl3_update_profiles_menu(void)
{
	int items = 0;
	GtkWidget *menu = NULL;
	gchar *current = connection_get_current_profile();
	conf_mult_obj *iter, *mult;
	/***
	 * Remove any old menu
	 */
	gtk_menu_item_remove_submenu(GTK_MENU_ITEM(glade_xml_get_widget(pl3_xml, "menu_profiles")));
	/**
	 * Create a new menu
	 */
	menu = gtk_menu_new();

	mult = cfg_get_class_list(profiles);
	if(mult)
	{
		GSList *group = NULL;
		iter = mult;
		do{
			/** Get profile name */
			gchar *value = cfg_get_single_value_as_string_with_default(profiles, iter->key, "name", "Name");
			GtkWidget *item	= gtk_radio_menu_item_new_with_label(group,value);
			g_free(value);	
			/* get new group */
			group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(item));	
			/* add to the menu */
			gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

			/* check the current profile */
			if(!strcmp(iter->key, current))
			{
				gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item), TRUE);
			}

			/**
			 * Attach click handler
			 */
			g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(pl3_profile_selected), NULL);

			/** Attach the uid to the handler */
			value = g_strdup(iter->key);
			g_object_set_data_full(G_OBJECT(item), "uid", value, g_free);

			iter = iter->next;
			items++;
		}while(iter);
		cfg_free_multiple(mult);
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
}
