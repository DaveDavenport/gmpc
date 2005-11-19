/*
 *Copyright (C) 2004-2005 Qball Cow <Qball@qballcow.nl>
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
#include "playlist3-tag-browser.h"

int old_type = -1;

extern config_obj *config;
GladeXML *pl3_xml = NULL;
GtkTreeStore *pl3_tree = NULL;
GtkListStore *pl3_store = NULL;
GtkListStore *pl2_store= NULL;

/* size */
GtkAllocation pl3_wsize = { 0,0,0,0};
int pl3_hidden = TRUE;

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
	NULL,
	NULL,
	NULL,	
	&playlist_gpp
};

/* Get the type of the selected row.. 
 * -1 means no row selected 
 */
int  pl3_cat_get_selected_browser()
{
	GtkTreeSelection *selec = gtk_tree_view_get_selection((GtkTreeView *)glade_xml_get_widget (pl3_xml, "cat_tree"));
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
void pl3_reinitialize_tree()
{
	int i;
	if(pl3_xml == NULL) return;
	GtkTreePath *path = gtk_tree_path_new_from_string("0");
	GtkTreeSelection *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(glade_xml_get_widget(pl3_xml, "cat_tree")));
	printf("%i %i\n", old_type, old_type&PLUGIN_ID_MARK);
	if((old_type&PLUGIN_ID_MARK) != 0)
	{
		if(old_type >= 0)
		{
			GtkWidget *container = glade_xml_get_widget(pl3_xml, "browser_container");
			plugins[old_type^PLUGIN_ID_MARK]->browser->unselected(container);
			old_type = -1;
		}
	}

	gtk_tree_store_clear(pl3_tree);
	/* add the current playlist */
	pl3_current_playlist_browser_add();
	pl3_file_browser_add();       	
	pl3_artist_browser_add();
	pl3_find_browser_add();
	pl3_custom_tag_browser_add();

	for(i=0; i< num_plugins;i++)
	{
		if(plugins[i]->plugin_type == GMPC_PLUGIN_PL_BROWSER)
		{
			if(plugins[i]->browser && plugins[i]->browser->add)
			{
				printf("adding plugin id: %i %i\n", i,plugins[i]->id^PLUGIN_ID_MARK);
				plugins[i]->browser->add(glade_xml_get_widget(pl3_xml, "cat_tree"));
			}
		}
	}


	gtk_widget_grab_focus(glade_xml_get_widget(pl3_xml, "cat_tree"));

	gtk_tree_selection_select_path(sel, path);               		
	gtk_tree_view_set_cursor(GTK_TREE_VIEW(glade_xml_get_widget(pl3_xml, "cat_tree")), path, NULL, FALSE);
	gtk_tree_path_free(path);
}

void pl3_disconnect()
{
	if(pl3_xml != NULL)
	{
		pl3_reinitialize_tree();
	}
}

void pl3_cat_row_activated(GtkTreeView *tree, GtkTreePath *tp, GtkTreeViewColumn *col)
{
	gint type = pl3_cat_get_selected_browser();
	if(check_connection_state())
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
	if(check_connection_state())
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
		else if (type == PL3_BROWSE_CUSTOM_TAG)
		{
			pl3_custom_tag_browser_fill_tree(iter);
		}
		else if(type|PLUGIN_ID_MARK)
		{
			if(plugins[type^PLUGIN_ID_MARK]->browser != NULL)
			{
				if(plugins[type^PLUGIN_ID_MARK]->browser->fill_tree != NULL)
				{
					plugins[type^PLUGIN_ID_MARK]->browser->fill_tree(tree, iter);
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
			else if (old_type == PL3_BROWSE_CUSTOM_TAG)
			{
				pl3_tag_browser_unselected(container);
			}
			else if (old_type|PLUGIN_ID_MARK && old_type > 0)
			{
				printf("old_type: %i\n", old_type);
				plugins[old_type^PLUGIN_ID_MARK]->browser->unselected(container);
				old_type = -1;
			}
		}

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
		else if (type == PL3_BROWSE_CUSTOM_TAG)
		{
			if(old_type != type) pl3_tag_browser_selected(container);
			pl3_custom_tag_browser_category_selection_changed(tree,&iter);
		}
		else if (type == PL3_FIND)
		{
			if(old_type != type){
				pl3_find_browser_selected(container);
				pl3_find_browser_category_selection_changed(tree,&iter);
			}
		}
		else if(type|PLUGIN_ID_MARK)
		{
			printf("plugins: %s\n", plugins[type^PLUGIN_ID_MARK]->name);
			if(old_type != type)plugins[type^PLUGIN_ID_MARK]->browser->selected(container);
			plugins[type^PLUGIN_ID_MARK]->browser->cat_selection_changed(GTK_WIDGET(tree),&iter);
		}
		old_type = type;
	}
}


/* handle right mouse clicks on the cat tree view */
/* gonna be a big function*/
int pl3_cat_tree_button_press_event(GtkTreeView *tree, GdkEventButton *event)
{
	GtkTreeSelection *sel = gtk_tree_view_get_selection(tree);
	if(event->button != 3 || gtk_tree_selection_count_selected_rows(sel) < 2|| !mpd_check_connected(connection))	
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
	printf("type == %i\n", type);
	if(type == -1 || check_connection_state())
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
	menu_items 	+= pl3_current_playlist_browser_cat_menu_popup(menu, type,tree,event);
	menu_items	+= pl3_file_browser_cat_popup(menu,type,tree,event);
	menu_items	+= pl3_artist_browser_cat_popup(menu, type, tree, event);
	menu_items	+= pl3_custom_tag_browser_right_mouse_menu(menu,type,GTK_WIDGET(tree),event);



	for(i=0; i< num_plugins;i++)
	{
		if(plugins[i]->browser != NULL)
		{
			printf("bpl: %i\n", i);
			if(plugins[i]->browser->cat_right_mouse_menu != NULL)
			{
				menu_items += plugins[i]->browser->cat_right_mouse_menu(menu,type,GTK_WIDGET(tree), event);
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

	if(check_connection_state())
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
			GtkTreeSelection *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(glade_xml_get_widget(pl3_xml, "cat_tree")));
			gtk_widget_grab_focus(glade_xml_get_widget(pl3_xml, "cat_tree"));
			gtk_tree_selection_select_path(sel, path);               	                                             		
			gtk_tree_view_set_cursor(GTK_TREE_VIEW(glade_xml_get_widget(pl3_xml, "cat_tree")), path, NULL, FALSE);
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
	else if (type == PL3_BROWSE_CUSTOM_TAG)
	{

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
	if(!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(glade_xml_get_widget(xml_main_window, "tb_pl2"))))
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

void create_playlist3 ()
{
	GtkCellRenderer *renderer;
	GtkWidget *tree;
	GtkTreeSelection *sel;
	GtkTreeViewColumn *column = NULL;
	gchar *path = NULL;
	GtkTreeIter iter;
	pl3_hidden = FALSE;
	if(pl3_xml != NULL)
	{

		if(pl3_wsize.x  >0 || pl3_wsize.y>0) gtk_window_move(GTK_WINDOW(glade_xml_get_widget(pl3_xml, "pl3_win")), pl3_wsize.x, pl3_wsize.y);
		if(pl3_wsize.height>0 || pl3_wsize.width>0) gtk_window_resize(GTK_WINDOW(glade_xml_get_widget(pl3_xml, "pl3_win")),pl3_wsize.width, pl3_wsize.height);
		gtk_widget_show(glade_xml_get_widget(pl3_xml, "pl3_win"));
		gtk_window_present(GTK_WINDOW(glade_xml_get_widget(pl3_xml, "pl3_win")));
		return;
	}
	if(!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(glade_xml_get_widget(xml_main_window, "tb_pl2"))))
	{
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(glade_xml_get_widget(xml_main_window, "tb_pl2")),TRUE);
		return;
	}

	/* load gui desciption */
	path = gmpc_get_full_glade_path("playlist3.glade");
	pl3_xml = glade_xml_new (path, "pl3_win", NULL);
	g_free(path);
	if(pl3_xml == NULL)
	{
		debug_printf(DEBUG_ERROR, "create_playlist3: Failed to open playlist3.glade.\n");
		return;
	}
	if(cfg_get_single_value_as_int_with_default(config, "playlist", "savepossize", 0))
	{
		pl3_wsize.x = 		cfg_get_single_value_as_int_with_default(config, "playlist", "xpos", 0);
		pl3_wsize.y = 		cfg_get_single_value_as_int_with_default(config, "playlist", "ypos", 0);
		pl3_wsize.width = 	cfg_get_single_value_as_int_with_default(config, "playlist", "width", 0);
		pl3_wsize.height = 	cfg_get_single_value_as_int_with_default(config, "playlist", "height", 0);

		if(pl3_wsize.x  >0 || pl3_wsize.y>0) {
			gtk_window_move(GTK_WINDOW(glade_xml_get_widget(pl3_xml, "pl3_win")), pl3_wsize.x, pl3_wsize.y);
		}
		if(pl3_wsize.height>0 || pl3_wsize.width>0) {
			gtk_window_resize(GTK_WINDOW(glade_xml_get_widget(pl3_xml, "pl3_win")),pl3_wsize.width, pl3_wsize.height);
		}
	}
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

	pl3_reinitialize_tree();

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

	/* unmark the old pos if it exists */
	if (info.old_pos != -1)
	{
		/* create a string so I can get the right iter */
		temp = g_strdup_printf ("%i", info.old_pos);
		if (gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL (pl2_store), &iter, temp))
		{
			gint song_id = 0, song_type = 0;
			/* check if we have the song we want */
			gtk_tree_model_get (GTK_TREE_MODEL (pl2_store), &iter, SONG_ID,&song_id,SONG_TYPE,&song_type, -1);
			/* if the old song is the new song (so tags updated) quit */
			if (song_id == mpd_player_get_current_song_id(connection))
			{
				g_free (temp);
				return;
			}
			/* unhighlight the song */
			gtk_list_store_set (pl2_store, &iter, WEIGHT_INT,PANGO_WEIGHT_NORMAL, 
					SONG_STOCK_ID, (!song_type)?"media-audiofile":"media-stream",    			
					-1);
		}
		g_free (temp);
		/* reset old pos */
		info.old_pos = -1;
	}
	/* check if we need to highlight a song */
	if (mpd_player_get_state(connection) > MPD_PLAYER_STOP && mpd_player_get_current_song_pos(connection) >= 0) 
	{
		temp = g_strdup_printf ("%i", mpd_player_get_current_song_pos(connection));
		if (gtk_tree_model_get_iter_from_string (GTK_TREE_MODEL (pl2_store), &iter, temp))
		{
			gint pos;
			gtk_tree_model_get (GTK_TREE_MODEL (pl2_store), &iter, SONG_POS,
					&pos, -1);
			/* check if we have the right song, if not, print an error */
			if (pos != mpd_player_get_current_song_pos(connection))
			{
				debug_printf(DEBUG_ERROR,"pl3_highlight_song_change: Error %i %i should be the same\n", pos,mpd_player_get_current_song_pos(connection));
			}
			gtk_list_store_set (pl2_store, &iter, WEIGHT_INT,PANGO_WEIGHT_ULTRABOLD,SONG_STOCK_ID,"gtk-media-play", -1);

			if(cfg_get_single_value_as_int_with_default(config, "playlist", "st_cur_song", 0) && 
					pl3_xml != NULL && PL3_CURRENT_PLAYLIST == pl3_cat_get_selected_browser())
			{
				pl3_current_playlist_browser_scroll_to_current_song();
			}
		}
		g_free (temp);
		/* set highlighted position */
		info.old_pos = mpd_player_get_current_song_pos(connection);
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
/****************************************************************************************
 *  PREFERENCES 									*
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
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(glade_xml_get_widget(playlist_pref_xml, "ck_of")), 
				cfg_get_single_value_as_int_with_default(config,"playlist", "open-to-position", 0));                          	
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(glade_xml_get_widget(playlist_pref_xml, "ck_ps")), 
				cfg_get_single_value_as_int_with_default(config,"playlist", "st_cur_song", 0));	
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(glade_xml_get_widget(playlist_pref_xml, "ck_possize")), 
				cfg_get_single_value_as_int_with_default(config,"playlist", "savepossize", 0));	

		gtk_container_add(GTK_CONTAINER(container),vbox);
		glade_xml_signal_autoconnect(playlist_pref_xml);
	}
}

void pl3_database_changed()
{
		pl3_file_browser_reupdate();
}
