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

#include "plugin.h"
#include "main.h"
#include "misc.h"
#include "config1.h"

#include "playlist3.h"
/* every part split out over multiple files */
#include "playlist3-find-browser.h"
#include "playlist3-file-browser.h"
#include "playlist3-artist-browser.h"
#include "playlist3-current-playlist-browser.h"

void pl3_show_and_position_window();
static void playlist_player_update_image(MpdObj *mi);

int old_type = -1;

GladeXML *pl3_xml = NULL;
GtkTreeStore *pl3_tree = NULL;
/* The Clipboard */

GQueue *pl3_queue = NULL;
/* size */
GtkAllocation pl3_wsize = { 0,0,0,0};
int pl3_hidden = TRUE;
static int pl3p_seek = FALSE;

void playlist_status_changed(MpdObj *mi, ChangedStatusType what, void *userdata);
/* Playlist "Plugin" */
void playlist_pref_construct(GtkWidget *container);
void playlist_pref_destroy(GtkWidget *container);
GladeXML *playlist_pref_xml = NULL;
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
	&playlist_gpp
};

/* Get the type of the selected row..
 * -1 means no row selected
 */
int  pl3_cat_get_selected_browser()
{
	GtkTreeSelection *selec = gtk_tree_view_get_selection((GtkTreeView *)
			glade_xml_get_widget (pl3_xml, "cat_tree"));
	GtkTreeModel *model = GTK_TREE_MODEL(pl3_tree);
	GtkTreeIter iter;

	if(gtk_tree_selection_get_selected(selec,&model, &iter))
	{
		gint type;
		gtk_tree_model_get(model, &iter, 0, &type, -1);
		if(type < 0)
		{
			return -1;
		}
		return type;
	}
	return -1;
}


/**************************************************
 * Category Tree
 */
void pl3_initialize_tree()
{
	int i;
	if(pl3_xml == NULL) return;

	GtkTreePath *path = gtk_tree_path_new_from_string("0");
	GtkTreeSelection *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(glade_xml_get_widget(pl3_xml, "cat_tree")));
	if((old_type&PLUGIN_ID_MARK || old_type&PLUGIN_ID_INTERNALL) != 0)
	{
		if(old_type >= 0)
		{
			GtkWidget *container = glade_xml_get_widget(pl3_xml, "browser_container");
			plugins[plugin_get_pos(old_type)]->browser->unselected(container);
			old_type = -1;
		}
	}

	gtk_tree_store_clear(pl3_tree);
	/* add the current playlist */
	pl3_current_playlist_browser_add();
	pl3_file_browser_add();
	pl3_artist_browser_add();
	pl3_find_browser_add();

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


	gtk_widget_grab_focus(glade_xml_get_widget(pl3_xml, "cat_tree"));

	gtk_tree_selection_select_path(sel, path);
	gtk_tree_view_set_cursor(GTK_TREE_VIEW(glade_xml_get_widget(pl3_xml, "cat_tree")),
			path, NULL, FALSE);
	gtk_tree_path_free(path);
}

void pl3_disconnect()
{
	if(pl3_xml != NULL)
	{
		pl3_file_browser_disconnect();
		pl3_artist_browser_disconnect();
		pl3_find_browser_disconnect();
	}
}

void pl3_cat_row_activated(GtkTreeView *tree, GtkTreePath *tp, GtkTreeViewColumn *col)
{
	gint type = pl3_cat_get_selected_browser();
	if(!mpd_check_connected(connection))
	{
		return;
	}
	/* nothing valid, so return */
	if(type == -1)
	{
		return;
	}

	else if(type == PL3_CURRENT_PLAYLIST)
	{

		pl3_current_playlist_browser_scroll_to_current_song();
	}
	else
	{
		if(gtk_tree_view_row_expanded(tree,tp))
		{
			gtk_tree_view_collapse_row(tree,tp);
		}
		else
		{
			gtk_tree_view_expand_row(tree,tp,FALSE);
		}
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
		if(type == PL3_BROWSE_FILE)
		{
			pl3_file_browser_fill_tree(iter);
		}
		else if (type == PL3_BROWSE_ARTIST)
		{
			pl3_artist_browser_fill_tree(iter);
		}
		else if(type&PLUGIN_ID_MARK || type&PLUGIN_ID_INTERNALL)
		{
			if(plugins[plugin_get_pos(type)]->browser != NULL)
			{
				if(plugins[plugin_get_pos(type)]->browser->fill_tree != NULL)
				{
					plugins[plugin_get_pos(type)]->browser->fill_tree(tree, iter);
				}
			}
		}
	}
	/* avuton's Idea */
	if(cfg_get_single_value_as_int_with_default(config, "playlist", "open-to-position",0))
	{
		gtk_tree_view_scroll_to_cell(tree, path,gtk_tree_view_get_column(tree,0),TRUE,0.5,0);
	}
}


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
		if(old_type != type )
		{
			if(old_type == PL3_CURRENT_PLAYLIST)
			{
				pl3_current_playlist_browser_unselected();
			}
			else if (old_type == PL3_BROWSE_FILE)
			{
				pl3_file_browser_unselected();
			}
			else if (old_type == PL3_BROWSE_ARTIST)
			{
				pl3_artist_browser_unselected(container);
			}
			else if (old_type == PL3_FIND)
			{
				pl3_find_browser_unselected(container);
			}
			else if ((old_type&PLUGIN_ID_MARK || old_type&PLUGIN_ID_INTERNALL) && old_type > 0)
			{
				plugins[plugin_get_pos(old_type)]->browser->unselected(container);
				old_type = -1;
			}
		}
		pl3_push_rsb_message("");
		if(type == PL3_CURRENT_PLAYLIST)
		{
			if(old_type != type)
			{
				pl3_current_playlist_browser_selected();
			}
		}
		else if (type == PL3_BROWSE_FILE)
		{
			if(old_type != type) pl3_file_browser_selected();
			pl3_file_browser_cat_sel_changed(GTK_TREE_VIEW(tree),&iter);
		}
		else if (type == PL3_BROWSE_ARTIST)
		{
			if(old_type != type) pl3_artist_browser_selected(container);
			pl3_artist_browser_category_selection_changed(tree,&iter);
		}
		else if (type == PL3_FIND)
		{
			if(old_type != type){
				pl3_find_browser_selected(container);
				pl3_find_browser_category_selection_changed(tree,&iter);
			}
		}
		else if(type&PLUGIN_ID_MARK || type&PLUGIN_ID_INTERNALL)
		{
			if(old_type != type)plugins[plugin_get_pos(type)]->browser->selected(container);
			plugins[plugin_get_pos(type)]->browser->cat_selection_changed(GTK_WIDGET(tree),&iter);
		}
		old_type = type;
	}
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
int pl3_cat_tree_button_release_event(GtkTreeView *tree, GdkEventButton *event)
{
	int i;
	gint type  = pl3_cat_get_selected_browser();
	int menu_items = 0;
	GtkWidget *menu = NULL;
	if(type == -1 || !mpd_check_connected(connection))
	{
		/* no selections, or no usefull one.. so propagate the signal */
		return FALSE;
	}
	menu = gtk_menu_new();
	if(event->button != 3)
	{
		/* if its not the right mouse button propagate the signal */
		return FALSE;
	}
	/* if it's the current playlist */
	menu_items	+= pl3_current_playlist_browser_cat_menu_popup(menu, type,tree,event);
	menu_items	+= pl3_file_browser_cat_popup(menu,type,tree,event);
	menu_items	+= pl3_artist_browser_cat_popup(menu, type, tree, event);

	for(i=0; i< num_plugins;i++)
	{
		if(plugins[i]->browser != NULL)
		{
			if(plugins[i]->browser->cat_right_mouse_menu != NULL)
			{
				menu_items += plugins[i]->browser->cat_right_mouse_menu(
						menu,
						type,
						GTK_WIDGET(tree),
						event);
			}
		}
	}

	/* plugins */
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
	if(event->keyval == GDK_f && event->state != GDK_CONTROL_MASK)
	{
		/* disabled because of problems with gtk 2.6 */
		return FALSE;
	}
	else if (event->keyval == GDK_w && event->state == GDK_CONTROL_MASK)
	{
		pl3_close();
	}
	else if (event->keyval == GDK_Escape)
	{
		pl3_close();
	}

	if(!mpd_check_connected(connection))
	{
		return FALSE;
	}
	/* on F1 move to current playlist */
	if (event->keyval == GDK_F1)
	{
		GtkTreeIter iter;
		GtkTreeSelection *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(glade_xml_get_widget(pl3_xml, "cat_tree")));

		/* select the current playlist */
		if(gtk_tree_model_get_iter_first(GTK_TREE_MODEL(pl3_tree), &iter))
		{
			gtk_tree_selection_select_iter(sel, &iter);
		}
	}
	else if (event->keyval == GDK_F2)
	{
		GtkTreePath *path = gtk_tree_path_new_from_string("1");
		GtkTreeSelection *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(glade_xml_get_widget(pl3_xml, "cat_tree")));
		gtk_widget_grab_focus(glade_xml_get_widget(pl3_xml, "cat_tree"));

		gtk_tree_selection_select_path(sel, path);
		gtk_tree_view_set_cursor(GTK_TREE_VIEW(glade_xml_get_widget(pl3_xml, "cat_tree")), path, NULL, FALSE);
		gtk_tree_path_free(path);
	}
	else if (event->keyval == GDK_F3)
	{
		GtkTreePath *path = gtk_tree_path_new_from_string("2");
		GtkTreeSelection *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(glade_xml_get_widget(pl3_xml, "cat_tree")));
		gtk_widget_grab_focus(glade_xml_get_widget(pl3_xml, "cat_tree"));

		gtk_tree_selection_select_path(sel, path);
		gtk_tree_view_set_cursor(GTK_TREE_VIEW(glade_xml_get_widget(pl3_xml, "cat_tree")), path, NULL, FALSE);
		gtk_tree_path_free(path);
	}
	else if (event->keyval == GDK_F4 || event->keyval == GDK_j)
	{
		if(event->keyval == GDK_j)
		{
			pl3_playlist_search();
		}
		else
		{
			GtkTreePath *path = gtk_tree_path_new_from_string("3");
			GtkTreeSelection *sel = gtk_tree_view_get_selection(
					GTK_TREE_VIEW(glade_xml_get_widget(pl3_xml, "cat_tree")));
			gtk_widget_grab_focus(glade_xml_get_widget(pl3_xml, "cat_tree"));
			gtk_tree_selection_select_path(sel, path);
			gtk_tree_view_set_cursor(
					GTK_TREE_VIEW(glade_xml_get_widget(pl3_xml, "cat_tree")),
					path,
					NULL,
					FALSE);
			gtk_tree_path_free(path);
		}
	}

	/* default gmpc/xmms/winamp key's*/
	else if (event->keyval == GDK_z )
	{
		prev_song();
	}
	else if (event->keyval == GDK_x || event->keyval == GDK_c)
	{
		play_song();
	}
	else if (event->keyval == GDK_v)
	{
		stop_song();
	}
	else if (event->keyval == GDK_b)
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



int pl3_cat_key_press_event(GtkWidget *mw, GdkEventKey *event)
{
	/* call default */
	int i;
	gint type = pl3_cat_get_selected_browser();
	if(type == PL3_BROWSE_FILE)
	{
		pl3_file_browser_cat_key_press(event);
	}
	else if (type == PL3_BROWSE_ARTIST)
	{
		pl3_artist_browser_category_key_press(event);
	}

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
	return pl3_window_key_press_event(mw,event);
}

void pl3_playlist_search()
{
	if(!mpd_check_connected(connection))
	{
		return;
	}
	if(!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(
					glade_xml_get_widget(xml_main_window, "tb_pl2"))))
	{
		create_playlist3();
	}
	else
	{
		gtk_window_present(GTK_WINDOW(glade_xml_get_widget(pl3_xml, "pl3_win")));
	}
	if(pl3_xml != NULL)
	{
		GtkTreePath *path = gtk_tree_path_new_from_string("3");
		GtkTreeSelection *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(glade_xml_get_widget(pl3_xml, "cat_tree")));
		gtk_widget_grab_focus(glade_xml_get_widget(pl3_xml, "cat_tree"));
		gtk_tree_selection_select_path(sel, path);
		gtk_tree_view_set_cursor(GTK_TREE_VIEW(glade_xml_get_widget(pl3_xml, "cat_tree")), path, NULL, FALSE);
		gtk_tree_path_free(path);
		pl3_find_browser_search_playlist();
	}
}

int pl3_pop_statusbar_message(gpointer data)
{
	gint id = GPOINTER_TO_INT(data);
	gtk_statusbar_pop(GTK_STATUSBAR(glade_xml_get_widget(pl3_xml, "statusbar1")), id);
	return FALSE;
}


void pl3_push_statusbar_message(char *mesg)
{
	gint id = gtk_statusbar_get_context_id(GTK_STATUSBAR(glade_xml_get_widget(pl3_xml, "statusbar1")), mesg);
	/* message auto_remove after 5 sec */
	g_timeout_add(5000,(GSourceFunc)pl3_pop_statusbar_message, GINT_TO_POINTER(id));
	gtk_statusbar_push(GTK_STATUSBAR(glade_xml_get_widget(pl3_xml, "statusbar1")), id,mesg);
}

void pl3_push_rsb_message(gchar *string)
{
	gtk_statusbar_push(GTK_STATUSBAR(glade_xml_get_widget(pl3_xml, "statusbar2")),0, string);
}

int pl3_close()
{
	if(pl3_xml != NULL && !pl3_hidden)
	{
		gtk_window_get_position(GTK_WINDOW(glade_xml_get_widget(pl3_xml, "pl3_win")), &pl3_wsize.x, &pl3_wsize.y);
		gtk_window_get_size(GTK_WINDOW(glade_xml_get_widget(pl3_xml, "pl3_win")), &pl3_wsize.width, &pl3_wsize.height);

		cfg_set_single_value_as_int(config, "playlist", "xpos", pl3_wsize.x);
		cfg_set_single_value_as_int(config, "playlist", "ypos", pl3_wsize.y);
		cfg_set_single_value_as_int(config, "playlist", "width", pl3_wsize.width);
		cfg_set_single_value_as_int(config, "playlist", "height", pl3_wsize.height);
		cfg_set_single_value_as_int(config, "playlist", "pane-pos", gtk_paned_get_position(
					GTK_PANED(glade_xml_get_widget(pl3_xml, "hpaned1"))));
		if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(glade_xml_get_widget(xml_main_window, "tb_pl2"))))
		{
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(glade_xml_get_widget(xml_main_window, "tb_pl2")), FALSE);
			return TRUE;
		}
		gtk_widget_hide(glade_xml_get_widget(pl3_xml, "pl3_win"));
		pl3_hidden = TRUE;
		return TRUE;
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
void pl3_show_and_position_window()
{
	if(!pl3_xml) return;
	if(pl3_wsize.x  >0 || pl3_wsize.y>0) {
		gtk_window_move(GTK_WINDOW(glade_xml_get_widget(pl3_xml, "pl3_win")),
				pl3_wsize.x,
				pl3_wsize.y);
	}
	if(pl3_wsize.height>0 || pl3_wsize.width>0) {
		gtk_window_resize(GTK_WINDOW(glade_xml_get_widget(pl3_xml, "pl3_win")),
				pl3_wsize.width,
				pl3_wsize.height);
	}
	gtk_widget_show(glade_xml_get_widget(pl3_xml, "pl3_win"));
	gtk_window_present(GTK_WINDOW(glade_xml_get_widget(pl3_xml, "pl3_win")));
	
}

void create_playlist3 ()
{
	GtkCellRenderer *renderer;
	GtkWidget *tree;
	GtkTreeSelection *sel;
	GtkTreeViewColumn *column = NULL;
	gchar *path = NULL;
	GtkTreeIter iter;
	/* indicate that the playlist is not hidden */
	pl3_hidden = FALSE;
		/* set the toggle button in the main windows, if it isn't allready in
	 * the propper state
	 *
	 * Setting the toggle button will "re-call" this function again, so
	 * step out of this function, and when it gets called again it skips
	 * this part.
	 */
	if(!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(glade_xml_get_widget(xml_main_window, "tb_pl2"))))
	{
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(glade_xml_get_widget(xml_main_window, "tb_pl2")),TRUE);
		return;
	}

	if(pl3_xml != NULL)
	{
		pl3_show_and_position_window();
		return;
	}
	if(!pl3_queue)
	{
		pl3_queue= g_queue_new();
	}
	/* load gui desciption */
	path = gmpc_get_full_glade_path("playlist3.glade");
	pl3_xml = glade_xml_new (path, "pl3_win", NULL);
	g_free(path);
	if(pl3_xml == NULL)
	{
		debug_printf(DEBUG_ERROR, "Failed to open playlist3.glade.\n");
		return;
	}
	/* restore the window's position and size */
	if(cfg_get_single_value_as_int_with_default(config, "playlist", "savepossize", 0))
	{
		pl3_wsize.x =	cfg_get_single_value_as_int_with_default(config, "playlist", "xpos", 0);
		pl3_wsize.y =	cfg_get_single_value_as_int_with_default(config, "playlist", "ypos", 0);
		pl3_wsize.width = cfg_get_single_value_as_int_with_default(config, "playlist", "width", 0);
		pl3_wsize.height = cfg_get_single_value_as_int_with_default(config, "playlist", "height", 0);

		if(pl3_wsize.x  >0 || pl3_wsize.y>0) {
			gtk_window_move(GTK_WINDOW(glade_xml_get_widget(pl3_xml, "pl3_win")),
					pl3_wsize.x, pl3_wsize.y);
		}
		if(pl3_wsize.height>0 || pl3_wsize.width>0) {
			gtk_window_resize(GTK_WINDOW(glade_xml_get_widget(pl3_xml, "pl3_win")),
					pl3_wsize.width, pl3_wsize.height);
		}

		if(cfg_get_single_value_as_int(config, "playlist", "pane-pos") != CFG_INT_NOT_DEFINED )
		{

			gtk_paned_set_position(GTK_PANED(glade_xml_get_widget(pl3_xml, "hpaned1")),
					cfg_get_single_value_as_int(config, "playlist", "pane-pos"));
		}
	}
	/* create tree store for the "category" view */
	if (pl3_tree == NULL)
	{
		/* song id, song title */
		pl3_tree = gtk_tree_store_new (PL3_CAT_NROWS, 
				GTK_TYPE_INT,	/* row type, see free_type struct */
				GTK_TYPE_STRING, /* display name */
				GTK_TYPE_STRING,/* full path and stuff for backend */
				GTK_TYPE_STRING, /* icon id */
				GTK_TYPE_BOOL,  /* cat proc */
				GTK_TYPE_UINT,  /* icon size */
				GTK_TYPE_STRING /* browser markup */
				);
	}

	tree = glade_xml_get_widget (pl3_xml, "cat_tree");
	gtk_tree_view_set_model (GTK_TREE_VIEW (tree), GTK_TREE_MODEL (pl3_tree));

	/* draw the column with the songs */
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

	pl3_initialize_tree();


	if(cfg_get_single_value_as_int_with_default(config, "playlist","player", FALSE))
	{
		gtk_widget_show(glade_xml_get_widget(pl3_xml, "vbox_playlist_player"));
	}
	playlist_status_changed(connection, MPD_CST_STATE|MPD_CST_SONGID|MPD_CST_ELAPSED_TIME,NULL);


	gtk_widget_show(glade_xml_get_widget(pl3_xml, "pl3_win"));

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
}

/* toggles the playlist on or off */
/* this should be placed on the player's toggle button */
gboolean toggle_playlist3(GtkToggleButton *tb)
{
	if(gtk_toggle_button_get_active(tb))
	{
		create_playlist3();
	}
	else
	{
		if(pl3_xml == NULL) return FALSE;
		pl3_close();
	}
	return TRUE;
}

void pl3_highlight_song_change ()
{
	GtkTreeIter iter;
	gchar *temp;
	/* check if there is a connection */
	if (!mpd_check_connected (connection))
	{
		return;
	}

	/* check if we need to highlight a song */
	if (mpd_player_get_state(connection) > MPD_PLAYER_STOP && mpd_player_get_current_song_pos(connection) >= 0)
	{
		temp = g_strdup_printf ("%i", mpd_player_get_current_song_pos(connection));
		if (gtk_tree_model_get_iter_from_string (GTK_TREE_MODEL (playlist), &iter, temp))
		{
			gint pos;
			gtk_tree_model_get (GTK_TREE_MODEL (playlist), &iter, PLAYLIST_LIST_COL_SONG_POS,
					&pos, -1);
			/* check if we have the right song, if not, print an error */
			if (pos != mpd_player_get_current_song_pos(connection))
			{
				debug_printf(DEBUG_ERROR,"Error %i "\
						" %i should be the same\n",
						pos,
						mpd_player_get_current_song_pos(connection));
			}

			if(cfg_get_single_value_as_int_with_default(config, "playlist", "st_cur_song", 0) &&
					pl3_xml != NULL && PL3_CURRENT_PLAYLIST == pl3_cat_get_selected_browser())
			{
				pl3_current_playlist_browser_scroll_to_current_song();
			}
		}
		g_free (temp);
		/* set highlighted position */
	}
}

void pl3_playlist_changed()
{
	int type = 0;
	if(pl3_xml == NULL) return;
	type  = pl3_cat_get_selected_browser();
	if(type ==  PL3_CURRENT_PLAYLIST)
	{
		pl3_current_playlist_browser_playlist_changed();
	}
	if(type == PL3_FIND)
	{
		pl3_find_browser_playlist_changed();
	}
}

/* helper functions */
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
void playlist3_player_enable_tb(GtkToggleButton *but)
{
	int bool1  = gtk_toggle_button_get_active(but);
	cfg_set_single_value_as_int(config, "playlist","player", bool1);
	if(pl3_xml)
	{
		if(bool1)
		{
			gtk_widget_show(
					glade_xml_get_widget(
						pl3_xml,
						"vbox_playlist_player"));

		}
		else
		{
			gtk_widget_hide(
					glade_xml_get_widget(
						pl3_xml,
						"vbox_playlist_player"));

		}
	}
}


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

void set_player_format()
{
	char *string = cfg_get_single_value_as_string_with_default(config, "player", "display_markup",	DEFAULT_PLAYER_MARKUP);
	char *format = edit_song_markup(string);
	cfg_free_string(string);

	if(format != NULL)
	{
		cfg_set_single_value_as_string(config, "player","display_markup",format);
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
		gtk_toggle_button_set_active(
				GTK_TOGGLE_BUTTON(glade_xml_get_widget(playlist_pref_xml, "ck_playlist_player")),
				cfg_get_single_value_as_int_with_default(config,"playlist", "player", 0));


		gtk_container_add(GTK_CONTAINER(container),vbox);
		glade_xml_signal_autoconnect(playlist_pref_xml);
	}
}

void pl3_database_changed()
{
	pl3_file_browser_reupdate();
}
static void pl3_player_repeat()
{
	mpd_player_set_repeat(connection, !mpd_player_get_repeat(connection));
}
static void pl3_player_random()
{
	mpd_player_set_random(connection, !mpd_player_get_random(connection));
}

int playlist_player_button_press_event (GtkWidget *event_box, GdkEventButton *event)
{
	if(!mpd_check_connected(connection)) return FALSE;
	if(event->button == 1)
	{
		mpd_Song *song = mpd_playlist_get_current_song(connection);
		if(song)
		{
			call_id3_window_song(mpd_songDup(song));
		}
	}
	if(event->button == 3)
	{
		GtkWidget *item = NULL;
		GtkWidget *menu = gtk_menu_new();

		item = gtk_check_menu_item_new_with_label("Repeat");
		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item), mpd_player_get_repeat(connection));
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
		g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(pl3_player_repeat), NULL);	

		item = gtk_check_menu_item_new_with_label("Random");
		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item), mpd_player_get_random(connection));
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
		g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(pl3_player_random), NULL);	
		item = gtk_separator_menu_item_new();
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);



		item = gtk_check_menu_item_new_with_label("Hide Player");
		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item), player_get_hidden());
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
		if(player_get_hidden()) {
			g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(player_show), NULL);
		}
		else {
			g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(player_hide), NULL);
		}
		
		item = gtk_image_menu_item_new_from_stock(GTK_STOCK_PREFERENCES,NULL);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
		g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(create_preferences_window), NULL);

		item = gtk_separator_menu_item_new();
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

		item = gtk_image_menu_item_new_from_stock(GTK_STOCK_QUIT,NULL);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);      
		g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(main_quit), NULL);		

		gtk_widget_show_all(menu);
		gtk_menu_popup(GTK_MENU(menu), NULL,NULL,NULL,NULL,event->button,event->time);
	}
	return FALSE;
}

/* Playlist player */
void playlist_player_set_song(MpdObj *mi)
{
	char buffer[1024];
	mpd_Song *song = mpd_playlist_get_current_song(mi);
	if(song)
	{
		int id;
		GString *string = g_string_new("");
		char *mark =cfg_get_single_value_as_string_with_default(
				config,
				"playlist",
				"player_markup",
				DEFAULT_PLAYLIST_PLAYER_MARKUP);
		mpd_song_markup(buffer, 1024,mark,song);
		cfg_free_string(mark);

		g_string_append(string,buffer);
		for(id=0;id < string->len; id++)
		{
			if(string->str[id] == '&')
			{
				g_string_insert(string, id+1, "amp;");
				id++;
			}
		}

		gtk_label_set_markup(GTK_LABEL
				(glade_xml_get_widget(pl3_xml,"pp_label")),
				string->str);
		g_string_free(string, TRUE);
	}
	else
	{
		gtk_label_set_markup(GTK_LABEL
				(glade_xml_get_widget(pl3_xml,"pp_label")),
				"<span size=\"large\" weight=\"bold\">Not Playing</span>");
	}
}
static void playlist_player_update_image_callback(mpd_Song *song)
{
	mpd_Song *current = mpd_playlist_get_current_song(connection);
	if(song != NULL && current  != NULL) {
		if(song->file && current->file)
		{
			if(!strcmp(song->file, current->file))
			{
				playlist_player_update_image(connection);
			}
		}
	}
}

static void playlist_player_update_image(MpdObj *mi)
{
	gchar *path= NULL;
	int ret = 0;
	mpd_Song *song = mpd_playlist_get_current_song(connection);
	ret = cover_art_fetch_image_path(song, &path);
	if(ret == COVER_ART_OK_LOCAL) {
		GdkPixbuf *pb = NULL;
		pb = gdk_pixbuf_new_from_file_at_size(path,60,60,NULL);
		gtk_image_set_from_pixbuf(GTK_IMAGE(glade_xml_get_widget(pl3_xml, "pp_cover_image")),pb);
		gtk_widget_show(glade_xml_get_widget(pl3_xml, "pp_cover_image"));
		g_object_unref(pb);
	}
	else{
		gtk_widget_hide(glade_xml_get_widget(pl3_xml, "pp_cover_image"));
	}
	if(path) g_free(path);
	if(ret == COVER_ART_NOT_FETCHED)
	{
		cover_art_fetch_image(song,                                                                             	
				(CoverArtCallback)playlist_player_update_image_callback,NULL);
	}

}
static void playlist_player_clear_image()
{
	gtk_widget_hide(glade_xml_get_widget(pl3_xml, "pp_cover_image"));
}
void playlist_status_changed(MpdObj *mi, ChangedStatusType what, void *userdata)
{
	if(!pl3_xml)return;
	if(what&MPD_CST_STATE)
	{
		int state = mpd_player_get_state(mi);
		switch(state){
			case MPD_PLAYER_PLAY:

				gtk_image_set_from_stock(GTK_IMAGE(
							glade_xml_get_widget(pl3_xml, "pp_but_play_img")),
						"gtk-media-pause",GTK_ICON_SIZE_BUTTON);
				playlist_player_set_song(mi);
				playlist_player_update_image(mi);
				break;
			case MPD_PLAYER_PAUSE:
				gtk_image_set_from_stock(GTK_IMAGE(
							glade_xml_get_widget(pl3_xml, "pp_but_play_img")),
						"gtk-media-play",GTK_ICON_SIZE_BUTTON);
				gtk_label_set_markup(GTK_LABEL
						(glade_xml_get_widget(pl3_xml,"pp_label")),
						"<span size=\"large\" weight=\"bold\">Paused</span>");

				break;
			default:
				playlist_player_clear_image();
				gtk_image_set_from_stock(GTK_IMAGE(
							glade_xml_get_widget(pl3_xml, "pp_but_play_img")),
						"gtk-media-play",GTK_ICON_SIZE_BUTTON);
				gtk_label_set_markup(GTK_LABEL
						(glade_xml_get_widget(pl3_xml,"pp_label")),
						"<span size=\"large\" weight=\"bold\">Not Playing</span>");
		}

	}
	if(what&MPD_CST_SONGID || what&MPD_CST_SONGPOS || what&MPD_CST_PLAYLIST)
	{
		if(mpd_player_get_state(mi) == MPD_PLAYER_PLAY)
		{
			playlist_player_set_song(mi);
		}
		if(mpd_player_get_state(mi) != MPD_PLAYER_STOP &&
				mpd_player_get_state(mi) != MPD_PLAYER_UNKNOWN)
		{
			playlist_player_update_image(mi);
		}
	}
	if(what&MPD_CST_ELAPSED_TIME)
	{
		char *string = NULL;
		int totalTime = mpd_status_get_total_song_time(connection);
		int elapsedTime = mpd_status_get_elapsed_song_time(connection);
		if(mpd_check_connected(connection))
		{
			if(!pl3p_seek)
			{
				gtk_range_set_value(GTK_RANGE(glade_xml_get_widget(pl3_xml, "pp_progres")),
						(elapsedTime/(float)totalTime)*100.0);
			}
			if(elapsedTime/60 >99 || totalTime/60 > 99)
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
	msg_pop_popup();
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
