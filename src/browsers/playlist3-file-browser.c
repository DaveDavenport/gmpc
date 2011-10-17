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
#include "main.h"
#include "misc.h"
#include "playlist3.h"
#include "playlist3-file-browser.h"
#include "playlist3-find2-browser.h"
#include "advanced-search.h"
#include "gmpc-mpddata-model.h"
#include "gmpc-mpddata-treeview.h"
#include "playlist3-playlist-editor.h"
#include "gmpc-extras.h"

#define LOG_DOMAIN "FileBrowser"

static void pl3_file_browser_plugin_init(void);

static gboolean pl3_file_browser_is_field_supported(int tag);
static MpdData *pl3_file_browser_is_search(const int num_field, const gchar * search_string, GError ** error);

static int pl3_file_browser_option_menu(GmpcMpdDataTreeview *tree, GtkMenu *menu);

static void pl3_file_browser_destroy(void);
static void pl3_file_browser_add(GtkWidget * cat_tree);
static void pl3_file_browser_unselected(GtkWidget * container);
static void pl3_file_browser_selected(GtkWidget * container);
static void pl3_file_browser_fill_tree(GtkWidget * tree, GtkTreeIter * iter, GtkTreePath * tpath, gpointer user_data);
static void pl3_file_browser_collapse_row(GtkTreeView * tree, GtkTreeIter * iter, GtkTreePath * path,
										  gpointer user_data);
static int pl3_file_browser_cat_popup(GtkWidget * tree, GdkEventButton * event, gpointer user_data);
static gboolean pl3_file_browser_cat_key_press(GtkWidget * tree, GdkEventKey * event, gpointer data);
static void pl3_file_browser_delete_playlist_from_right(GtkMenuItem * bt);
/* testing */
static void pl3_file_browser_reupdate(void);
static void pl3_file_browser_save_myself(void);
static int pl3_file_browser_add_go_menu(GtkWidget * menu);
static void pl3_file_browser_activate(void);
static gboolean pl3_file_browser_button_release_event(GtkWidget * but, GdkEventButton * event);
static void pl3_file_browser_row_activated(GtkTreeView * tree, GtkTreePath * tp);
static void pl3_file_browser_add_selected(void);
static void pl3_file_browser_replace_selected(void);
static int pl3_file_browser_playlist_key_press(GtkWidget * tree, GdkEventKey * event);
static void pl3_file_browser_show_info(void);
static void pl3_file_browser_view_folder(GtkTreeSelection * selection, gpointer user_data);
static void pl3_file_browser_update_folder(void);
static void pl3_file_browser_add_folder(void);
static void pl3_file_browser_delete_playlist(GtkMenuItem * bt);
static void pl3_file_browser_connection_changed(MpdObj * mi, int connect, gpointer data);
static void pl3_file_browser_status_changed(MpdObj * mi, ChangedStatusType what, void *data);
static void pl3_file_browser_disconnect(void);

GtkTreeRowReference *pl3_fb_tree_ref = NULL;

enum
{
	PL3_FB_ICON = 0,
	PL3_FB_NAME = 1,
	PL3_FB_PATH = 2,
	PL3_FB_OPEN = 3
};
/**
 * Get/Set enabled
 */
static int pl3_file_browser_get_enabled(void)
{
	return cfg_get_single_value_as_int_with_default(config, "file-browser", "enable", TRUE);
}

static void pl3_file_browser_set_enabled(int enabled)
{
	cfg_set_single_value_as_int(config, "file-browser", "enable", enabled);
	if (enabled)
	{
		GtkTreeView *tree = playlist3_get_category_tree_view();
		pl3_file_browser_add((GtkWidget *) tree);
	} else if (!enabled)
	{
		pl3_file_browser_destroy();
	}
}

/**
 * Plugin structure
 */
gmpcPlBrowserPlugin file_browser_gbp = {
	.add = pl3_file_browser_add,
	.selected = pl3_file_browser_selected,
	.unselected = pl3_file_browser_unselected,
	.add_go_menu = pl3_file_browser_add_go_menu,
	.integrate_search_field_supported = pl3_file_browser_is_field_supported,
	.integrate_search = pl3_file_browser_is_search,
    .song_list_option_menu = pl3_file_browser_option_menu
};

gmpcPlugin file_browser_plug = {
	.name = N_("Database"),
	.version = {1, 1, 1}
	,
	.plugin_type = GMPC_PLUGIN_PL_BROWSER | GMPC_INTERNALL,
	.init = pl3_file_browser_plugin_init,
	.destroy = pl3_file_browser_destroy,
	.browser = &file_browser_gbp,
	.mpd_status_changed = pl3_file_browser_status_changed,
	.mpd_connection_changed = pl3_file_browser_connection_changed,
	.get_enabled = pl3_file_browser_get_enabled,
	.set_enabled = pl3_file_browser_set_enabled,
	.save_yourself = pl3_file_browser_save_myself
};

/* internal */
GtkWidget *pl3_fb_tree = NULL;
GtkWidget *pl3_fb_vbox = NULL;
GtkWidget *pl3_fb_tree_search = NULL;
GmpcMpdDataModel *pl3_fb_store2 = NULL;
static GtkTreeStore *pl3_fb_dir_store = NULL;
static GtkWidget *pl3_fb_dir_tree = NULL;
static GtkWidget *pl3_fb_warning_box = NULL;

static void pl3_file_browser_dir_row_activated(GtkTreeView * tree, GtkTreePath * tp, GtkTreeViewColumn * col,
											   gpointer user_data)
{
	if (!mpd_check_connected(connection))
		return;
	if (gtk_tree_view_row_expanded(tree, tp))
		gtk_tree_view_collapse_row(tree, tp);
	else
		gtk_tree_view_expand_row(tree, tp, FALSE);
}

static void playtime_changed(GmpcMpdDataModel * model, gulong playtime)
{
}
static void pl3_file_support_help_button_clicked(GObject *a)
{
	open_help("ghelp:gmpc?ProblemSolving");
}
static void pl3_file_browser_init(void)
{
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	GtkWidget *pl3_fb_sw = NULL;
	GtkWidget *vbox, *sw, *tree,*label;
    GtkWidget *misc, *button;

	pl3_fb_store2 = gmpc_mpddata_model_new();
	gmpc_mpddata_model_disable_image(GMPC_MPDDATA_MODEL(pl3_fb_store2));
	g_signal_connect(G_OBJECT(pl3_fb_store2), "playtime_changed", G_CALLBACK(playtime_changed), NULL);

	pl3_fb_vbox = gtk_hpaned_new();
	gmpc_paned_size_group_add_paned(GMPC_PANED_SIZE_GROUP(paned_size_group), GTK_PANED(pl3_fb_vbox));
	vbox = gtk_vbox_new(FALSE, 6);

	/* icon id, name, full path */
	pl3_fb_dir_store = gtk_tree_store_new(4, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_BOOLEAN);

	sw = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw), GTK_SHADOW_ETCHED_IN);
	pl3_fb_dir_tree = tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(pl3_fb_dir_store));
	gtk_tree_view_set_enable_tree_lines(GTK_TREE_VIEW(tree), TRUE);
	gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(tree), TRUE);
	column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(column, _("Directories"));
	renderer = gtk_cell_renderer_pixbuf_new();
	gtk_tree_view_column_pack_start(column, renderer, FALSE);
	gtk_tree_view_column_add_attribute(column, renderer, "icon-name", PL3_FB_ICON);
	gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_FIXED);
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	gtk_tree_view_column_add_attribute(column, renderer, "text", PL3_FB_NAME);
	gtk_tree_view_insert_column(GTK_TREE_VIEW(tree), column, -1);

	gtk_tree_view_set_fixed_height_mode(GTK_TREE_VIEW(tree), TRUE);
	gtk_tree_view_set_search_column(GTK_TREE_VIEW(pl3_fb_dir_tree), PL3_FB_NAME);
	/* set the search column */
	gtk_tree_view_set_search_column(GTK_TREE_VIEW(tree), PL3_FB_NAME);

	g_signal_connect(G_OBJECT(tree), "row-expanded", G_CALLBACK(pl3_file_browser_fill_tree), NULL);
	g_signal_connect(G_OBJECT(tree), "row-collapsed", G_CALLBACK(pl3_file_browser_collapse_row), NULL);
	g_signal_connect(G_OBJECT(gtk_tree_view_get_selection(GTK_TREE_VIEW(tree))), "changed",
					 G_CALLBACK(pl3_file_browser_view_folder), NULL);
	g_signal_connect(G_OBJECT(tree), "button-release-event", G_CALLBACK(pl3_file_browser_cat_popup), NULL);
	g_signal_connect(G_OBJECT(tree), "key-press-event", G_CALLBACK(pl3_file_browser_cat_key_press), NULL);
	g_signal_connect(G_OBJECT(tree), "row-activated", G_CALLBACK(pl3_file_browser_dir_row_activated), NULL);

	gtk_container_add(GTK_CONTAINER(sw), tree);
	gtk_widget_show_all(sw);
	gtk_paned_add1(GTK_PANED(pl3_fb_vbox), sw);

	/* set up the tree */
	pl3_fb_tree = gmpc_mpddata_treeview_new("file-browser", TRUE, GTK_TREE_MODEL(pl3_fb_store2));
	gmpc_mpddata_treeview_enable_click_fix(GMPC_MPDDATA_TREEVIEW(pl3_fb_tree));
	gtk_tree_view_set_search_column(GTK_TREE_VIEW(pl3_fb_tree), MPDDATA_MODEL_COL_SONG_TITLE);
	/* setup signals */
	g_signal_connect(G_OBJECT(pl3_fb_tree), "row-activated", G_CALLBACK(pl3_file_browser_row_activated), NULL);
	g_signal_connect(G_OBJECT(pl3_fb_tree), "button-release-event", G_CALLBACK(pl3_file_browser_button_release_event),
					 NULL);
	g_signal_connect(G_OBJECT(pl3_fb_tree), "key-press-event", G_CALLBACK(pl3_file_browser_playlist_key_press), NULL);

	/* set up the scrolled window */
	pl3_fb_sw = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(pl3_fb_sw), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(pl3_fb_sw), GTK_SHADOW_ETCHED_IN);
	gtk_container_add(GTK_CONTAINER(pl3_fb_sw), pl3_fb_tree);

	gtk_box_pack_start(GTK_BOX(vbox), pl3_fb_sw, TRUE, TRUE, 0);
	gtk_widget_show_all(pl3_fb_sw);

    /******************************************/
	/* Warning box for when there is no music */
    /******************************************/
	pl3_fb_warning_box = gtk_vbox_new(FALSE, 6);
    /* label */
    label = gtk_label_new("");
	gtk_label_set_markup(GTK_LABEL(label),
						 _("It seems you have no music in your database.\n"
						   "To add music, copy the music to your <i>music_directory</i> as specified in your mpd config file.\n"
						   "Then update the database. (Server->Update Database)"));
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.0);
    gtk_widget_show(label);
    gtk_box_pack_start(GTK_BOX(pl3_fb_warning_box), label, FALSE, FALSE, 0);

    /* help button */
    misc    = gtk_alignment_new(0, 0.5, 0, 0);
    button  = gtk_button_new_from_stock(GTK_STOCK_HELP);
    g_signal_connect(G_OBJECT(button), "clicked",
            G_CALLBACK(pl3_file_support_help_button_clicked), NULL);
    gtk_container_add(GTK_CONTAINER(misc), button);
    gtk_widget_show(button);
    gtk_widget_show_all(misc);
    gtk_box_pack_start(GTK_BOX(pl3_fb_warning_box), misc, FALSE, FALSE, 0);
    gtk_widget_set_no_show_all(pl3_fb_warning_box, TRUE);


	gtk_box_pack_end(GTK_BOX(vbox), pl3_fb_warning_box, FALSE, TRUE, 0);
    gtk_paned_add2(GTK_PANED(pl3_fb_vbox), vbox);
    /* set initial state */
    gtk_widget_show(vbox);
	gtk_widget_show(pl3_fb_vbox);
	g_object_ref_sink(G_OBJECT(pl3_fb_vbox));
}

static void pl3_file_browser_add_folder(void)
{
	GtkTreeSelection *selec = gtk_tree_view_get_selection(GTK_TREE_VIEW(pl3_fb_dir_tree));
	GtkTreeModel *model = GTK_TREE_MODEL(pl3_fb_dir_store);
	GtkTreeIter iter;

	if (!mpd_check_connected(connection))
	{
		return;
	}
	if (gtk_tree_selection_get_selected(selec, &model, &iter))
	{
		char *path, *icon;
		char *message = NULL;
		gtk_tree_model_get(model, &iter, PL3_FB_PATH, &path, PL3_FB_ICON, &icon, -1);

		if (strcmp("media-playlist", icon))
		{
			mpd_playlist_queue_add(connection, path);
		} else
		{
			mpd_playlist_queue_load(connection, path);
		}
		mpd_playlist_queue_commit(connection);
		q_free(path);
		q_free(icon);
	}
}

static void pl3_file_browser_update_folder(void)
{
	GtkTreeSelection *selec = gtk_tree_view_get_selection((GtkTreeView *) pl3_fb_dir_tree);
	GtkTreeModel *model = GTK_TREE_MODEL(pl3_fb_dir_store);
	GtkTreeIter iter;

	if (!mpd_check_connected(connection))
	{
		return;
	}
	if (gtk_tree_selection_get_selected(selec, &model, &iter))
	{
		char *path;
		gtk_tree_model_get(model, &iter, PL3_FB_PATH, &path, -1);
		if (path)
		{
			mpd_database_update_dir(connection, path);
			q_free(path);
		}
	}

}

static void pl3_file_browser_update_folder_left_pane(void)
{
	GtkTreeModel *model = GTK_TREE_MODEL(pl3_fb_store2);
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(pl3_fb_tree));
	GtkTreeIter iter;

	if (!mpd_check_connected(connection))
		return;

	if (gtk_tree_selection_count_selected_rows(selection) == 1)
	{
		GList *list = gtk_tree_selection_get_selected_rows(selection, &model);
		/* free list */
		if (gtk_tree_model_get_iter(model, &iter, list->data))
		{
			char *path;
			gtk_tree_model_get(model, &iter, MPDDATA_MODEL_COL_PATH, &path, -1);
			if (path)
			{
				mpd_database_update_dir(connection, path);
				q_free(path);
			}
		}
		g_list_foreach(list, (GFunc) gtk_tree_path_free, NULL);
		g_list_free(list);
	}
}

static void pl3_file_browser_replace_folder(void)
{
	mpd_playlist_clear(connection);
	if (mpd_check_connected(connection))
	{
		pl3_file_browser_add_folder();
		mpd_player_play(connection);
	}
}

/* add's the toplevel entry for the file browser, it also add's a fantom child */
static void pl3_file_browser_add(GtkWidget * cat_tree)
{
	GtkTreeIter iter;
	GtkTreePath *path;
	gint pos;
	INIT_TIC_TAC()
	pos = cfg_get_single_value_as_int_with_default(config, "file-browser", "position", 2);

	TEC("get pos ")
	playlist3_insert_browser(&iter, PL3_CAT_BROWSER_LIBRARY+pos%PL3_CAT_BROWSER_LIBRARY);
	TEC("insert browser")
	gtk_list_store_set(GTK_LIST_STORE(pl3_tree), &iter,
					   PL3_CAT_TYPE, file_browser_plug.id,
					   PL3_CAT_TITLE, _("Database"), PL3_CAT_ICON_ID, "gmpc-database", -1);
	TEC("set list store")

	if (pl3_fb_tree_ref)
	{
		gtk_tree_row_reference_free(pl3_fb_tree_ref);
		pl3_fb_tree_ref = NULL;
	}
	TEC("Free references")
	
	path = gtk_tree_model_get_path(GTK_TREE_MODEL(pl3_tree), &iter);
	if (path)
	{
		pl3_fb_tree_ref = gtk_tree_row_reference_new(GTK_TREE_MODEL(pl3_tree), path);
		gtk_tree_path_free(path);
	}
	TEC("New references")


}

static int directory_sort_func(gpointer ppaa, gpointer ppbb, gpointer data)
{
	MpdData_real *a = *(MpdData_real **) ppaa;
	MpdData_real *b = *(MpdData_real **) ppbb;
	int val = 0;
	if (!(a && b))
		return val;
	if (a->type != b->type)
		return a->type - b->type;
	if ((a->type == MPD_DATA_TYPE_DIRECTORY) && (b->type == MPD_DATA_TYPE_DIRECTORY))
	{
		if (a->directory && b->directory)
		{
			gchar *sa, *sb;
			sa = g_utf8_strdown(a->directory, -1);
			sb = g_utf8_strdown(b->directory, -1);
			val = g_utf8_collate(sa, sb);
			g_free(sa);
			g_free(sb);
		}
	}
	return val;
}

/* Reverse sort function, needed for prepending instead of appending */
static int directory_sort_func_inv(gpointer a, gpointer b, gpointer d)
{
	return -directory_sort_func(a, b, d);
}

static void pl3_file_browser_reupdate_folder(GtkTreeIter * iter)
{
	MpdData *data = NULL;
	gchar *path = NULL;
	gboolean temp = FALSE;
	GtkTreeModel *model = GTK_TREE_MODEL(pl3_fb_dir_store);
	gtk_tree_model_get(GTK_TREE_MODEL(pl3_fb_dir_store), iter, 3, &temp, 2, &path, -1);
	if (path && temp)
	{
		GtkTreeIter child, child2, child3;
		data = mpd_database_get_directory(connection, path);
		data = misc_sort_mpddata(data, (GCompareDataFunc) directory_sort_func, NULL);
		g_free(path);
		if (gtk_tree_model_iter_children(model, &child, iter))
		{
			gchar *test_path = NULL;
			gboolean has_next = FALSE;
			do
			{
				gtk_tree_model_get(model, &child, 3, &temp, 2, &test_path, -1);

				if (data == NULL)
				{
					/* if no more data, remove the subdir */
					has_next = gtk_tree_store_remove(pl3_fb_dir_store, &child);
				} else
				{
					int compare = 0;
					/* get the next directory */
					while (data && data->type != MPD_DATA_TYPE_DIRECTORY)
						data = mpd_data_get_next(data);
					if (data)
					{
						compare = strcmp(data->directory, test_path);
						if (compare < 0)
						{
							gchar *basename = g_path_get_basename(data->directory);
							gtk_tree_store_insert_before(pl3_fb_dir_store, &child2, iter, &child);
							gtk_tree_store_set(pl3_fb_dir_store, &child2,
											   PL3_FB_ICON, "gtk-open",
											   PL3_FB_NAME, basename,
											   PL3_FB_PATH, data->directory, PL3_FB_OPEN, FALSE, -1);

							gtk_tree_store_append(pl3_fb_dir_store, &child3, &child2);
							q_free(basename);

							/* if the new dir is smaller the temp, we add it. */
							data = mpd_data_get_next(data);
							has_next = TRUE;
						} else if (compare > 0)
						{
							/* if it's bigger, we delete the row */
							has_next = gtk_tree_store_remove(pl3_fb_dir_store, &child);

						} else
						{
							/* if equal we process children if available */
							if (temp)
							{
								pl3_file_browser_reupdate_folder(&child);
							}
							/* move to next entry in both */
							has_next = gtk_tree_model_iter_next(model, &child);
							data = mpd_data_get_next(data);
						}
					}
				}
				g_free(test_path);
			} while (has_next);
			if (data)
			{
				do
				{
					if (data->type == MPD_DATA_TYPE_DIRECTORY)
					{
						gchar *basename = g_path_get_basename(data->directory);
						gtk_tree_store_append(pl3_fb_dir_store, &child2, iter);
						gtk_tree_store_set(pl3_fb_dir_store, &child2,
										   PL3_FB_ICON, "gtk-open",
										   PL3_FB_NAME, basename, PL3_FB_PATH, data->directory, PL3_FB_OPEN, FALSE, -1);
						gtk_tree_store_append(pl3_fb_dir_store, &child3, &child2);
						q_free(basename);
					}
				} while ((data = mpd_data_get_next(data)));
			}
		}
	}
}

static void pl3_file_browser_reupdate(void)
{

	if (pl3_fb_vbox)
	{
		GtkTreeIter iter;

		GtkTreeModel *model = GTK_TREE_MODEL(pl3_fb_dir_store);

		if (mpd_stats_get_total_songs(connection) == 0 && !mpd_status_db_is_updating(connection))
		{
			gtk_widget_show(pl3_fb_warning_box);
		} else
		{
			gtk_widget_hide(pl3_fb_warning_box);
		}

		if (gtk_tree_model_get_iter_first(model, &iter))
		{
			GtkTreeIter child;
			if (!gtk_tree_model_iter_children(model, &child, &iter))
			{
				gtk_tree_store_set(GTK_TREE_STORE(model), &iter, PL3_FB_OPEN, FALSE, -1);
				gtk_tree_store_append(pl3_fb_dir_store, &child, &iter);
			}
			pl3_file_browser_reupdate_folder(&iter);
			pl3_file_browser_view_folder(gtk_tree_view_get_selection(GTK_TREE_VIEW(pl3_fb_dir_tree)), NULL);
		}
	}
}

static void pl3_file_browser_view_folder(GtkTreeSelection * selection, gpointer user_data)
{
	MpdData *data = NULL;
	char *path = NULL, *icon = NULL;
	GtkTreeModel *model = GTK_TREE_MODEL(pl3_fb_dir_store);
	GtkTreeIter iter_cat;
	/* Clear the view */
	gmpc_mpddata_model_set_mpd_data(pl3_fb_store2, NULL);

	if (!gtk_tree_selection_get_selected(selection, &model, &iter_cat))
		return;

	/* check the connection state and when its valid proceed */
	if (!mpd_check_connected(connection))
	{
		return;
	}
	gtk_tree_model_get(GTK_TREE_MODEL(pl3_fb_dir_store), &iter_cat, PL3_FB_PATH, &path, PL3_FB_ICON, &icon, -1);
	if (strcmp("media-playlist", icon))
	{
		data = mpd_database_get_directory(connection, path);
	} else
	{
		data = mpd_database_get_playlist_content(connection, path);
	}
	/* Check, and set the up arrow in the model */
	if (!strcmp(path, "/"))
		gmpc_mpddata_model_set_has_up(pl3_fb_store2, FALSE);
	else
		gmpc_mpddata_model_set_has_up(pl3_fb_store2, TRUE);
	gmpc_mpddata_model_set_mpd_data(pl3_fb_store2, data);
	q_free(path);
	q_free(icon);
	return;
}

static void pl3_file_browser_collapse_row(GtkTreeView * tree, GtkTreeIter * iter, GtkTreePath * path,
										  gpointer user_data)
{
	GtkTreeIter child;
	int valid = gtk_tree_model_iter_children(GTK_TREE_MODEL(pl3_fb_dir_store), &child, iter);
	if (!cfg_get_single_value_as_int_with_default(config, "file-browser", "extra-lazy", TRUE))
		return;

	while (valid)
	{
		valid = gtk_tree_store_remove(pl3_fb_dir_store, &child);
	}
	/* set unopened */
	gtk_tree_store_set(pl3_fb_dir_store, iter, PL3_FB_OPEN, FALSE, -1);
	/* add phantom child */
	gtk_tree_store_append(pl3_fb_dir_store, &child, iter);
}

static void pl3_file_browser_fill_tree(GtkWidget * tree, GtkTreeIter * iter, GtkTreePath * tpath, gpointer user_data)
{
	char *path;
	MpdData *data = NULL;
	GtkTreeIter child, child2, dummy;
	gboolean open;
	gtk_tree_model_get(GTK_TREE_MODEL(pl3_fb_dir_store), iter, PL3_FB_PATH, &path, PL3_FB_OPEN, &open, -1);
	gtk_tree_store_set(pl3_fb_dir_store, iter, PL3_FB_OPEN, TRUE, -1);
	if (open == FALSE)
	{
		GTimer *tim = g_timer_new();
		data = mpd_database_get_directory(connection, path);
		/* Do a reverse sort, because adding it to the gtk view by prepending is faster
		 * then appending */
		data = misc_sort_mpddata(data, (GCompareDataFunc) directory_sort_func_inv, NULL);
		g_log(LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Elapsed time sorting before adding: %f\n", g_timer_elapsed(tim, NULL));

		if (gtk_tree_model_iter_children(GTK_TREE_MODEL(pl3_fb_dir_store), &dummy, iter))
		{
			while (data != NULL)
			{
				if (data->type == MPD_DATA_TYPE_DIRECTORY)
				{
					gchar *basename = g_path_get_basename(data->directory);
					gtk_tree_store_prepend(pl3_fb_dir_store, &child, iter);
					gtk_tree_store_set(pl3_fb_dir_store, &child,
									   PL3_FB_ICON, "gtk-open",
									   PL3_FB_NAME, basename, PL3_FB_PATH, data->directory, PL3_FB_OPEN, FALSE, -1);
					gtk_tree_store_append(pl3_fb_dir_store, &child2, &child);

					q_free(basename);
				}
				data = mpd_data_get_next(data);
			}

			gtk_tree_store_remove(pl3_fb_dir_store, &dummy);
		}

		g_log(LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Elapsed time sorting after adding: %f\n", g_timer_elapsed(tim, NULL));
		g_timer_destroy(tim);

	}

	q_free(path);
}

static int pl3_file_browser_cat_popup(GtkWidget * wid, GdkEventButton * event, gpointer user_data)
{
	GtkWidget *menu;
	if (event->button == 3)
	{

		/* here we have:  Add. Replace, (update?) */
		GtkWidget *item;
		menu = gtk_menu_new();
		/* add the add widget */
		item = gtk_image_menu_item_new_from_stock(GTK_STOCK_ADD, NULL);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
		g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(pl3_file_browser_add_folder), NULL);

		/* add the replace widget */
		item = gtk_image_menu_item_new_with_label(_("Replace"));
		gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
									  gtk_image_new_from_stock(GTK_STOCK_REDO, GTK_ICON_SIZE_MENU));
		gtk_menu_item_set_accel_path(GTK_MENU_ITEM(item), NULL);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
		g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(pl3_file_browser_replace_folder), NULL);

		{
			GtkTreeView *tree = GTK_TREE_VIEW(pl3_fb_dir_tree);
			GtkTreeModel *model = (GtkTreeModel *) pl3_fb_dir_store;
			GtkTreeSelection *selection = gtk_tree_view_get_selection(tree);
			GtkTreeIter iter;
			if (gtk_tree_selection_get_selected(selection, &model, &iter))
			{
				char *icon = NULL;
				gtk_tree_model_get(model, &iter, PL3_FB_ICON, &icon, -1);
				if (!strcmp("media-playlist", icon))
				{
					item = gtk_image_menu_item_new_from_stock(GTK_STOCK_DELETE, NULL);
					gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
					g_signal_connect(G_OBJECT(item), "activate",
									 G_CALLBACK(pl3_file_browser_delete_playlist_from_right), NULL);
				} else
				{
					/* add the update widget */
					item = gtk_image_menu_item_new_with_label(_("Update"));
					gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
												  gtk_image_new_from_stock(GTK_STOCK_REFRESH, GTK_ICON_SIZE_MENU));
					gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
					g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(pl3_file_browser_update_folder), NULL);
				}
				q_free(icon);
			}
		}
		gtk_widget_show_all(menu);
		gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, 0, event->time);
		/* show everything and popup */
		return TRUE;
	}
	return FALSE;
}

static gboolean pl3_file_browser_cat_key_press(GtkWidget * tree, GdkEventKey * event, gpointer data)
{
	if (event->state & GDK_CONTROL_MASK && (event->keyval == GDK_Insert || event->keyval == GDK_KP_Insert))
	{
		pl3_file_browser_replace_folder();
	} else if (event->keyval == GDK_Insert || event->keyval == GDK_KP_Insert)
	{
		pl3_file_browser_add_folder();
	}
	return FALSE;
}

static int pl3_file_browser_playlist_key_press(GtkWidget * tree, GdkEventKey * event)
{
	if (event->state & GDK_CONTROL_MASK && (event->keyval == GDK_Insert || event->keyval == GDK_KP_Insert))
	{
		pl3_file_browser_replace_selected();
	} else if (event->keyval == GDK_Insert || event->keyval == GDK_KP_Insert)
	{
		pl3_file_browser_add_selected();
	} else if (event->keyval == GDK_i && event->state & GDK_MOD1_MASK)
	{
		pl3_file_browser_show_info();
	}
	return FALSE;
}

static void pl3_file_browser_selected(GtkWidget * container)
{
	if (pl3_fb_tree == NULL)
	{
		GtkTreeIter iter,child;
		pl3_file_browser_init();
		gtk_tree_store_append(pl3_fb_dir_store, &iter, NULL);
		gtk_tree_store_set(pl3_fb_dir_store, &iter,
				PL3_FB_ICON, "gtk-open", PL3_FB_NAME, "/", PL3_FB_PATH, "/", PL3_FB_OPEN, FALSE, -1);
		gtk_tree_store_append(pl3_fb_dir_store, &child, &iter);
	}

	gtk_container_add(GTK_CONTAINER(container), pl3_fb_vbox);
	gtk_widget_grab_focus(pl3_fb_tree);
	gtk_widget_show(pl3_fb_vbox);

}

static void pl3_file_browser_unselected(GtkWidget * container)
{
	gtk_container_remove(GTK_CONTAINER(container), pl3_fb_vbox);
}

static void pl3_file_browser_show_info(void)
{
	GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(pl3_fb_tree));
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(pl3_fb_tree));
	if (!mpd_server_check_version(connection, 0, 12, 0))
	{
		return;
	}
	if (gtk_tree_selection_count_selected_rows(selection) > 0)
	{
		GList *list = NULL;
		list = gtk_tree_selection_get_selected_rows(selection, &model);

		list = g_list_last(list);
		{
			mpd_Song *song = NULL;
			GtkTreeIter iter;
			int type;
			gtk_tree_model_get_iter(model, &iter, (GtkTreePath *) list->data);
			gtk_tree_model_get(GTK_TREE_MODEL(pl3_fb_store2), &iter,
							   MPDDATA_MODEL_ROW_TYPE, &type, MPDDATA_MODEL_COL_MPDSONG, &song, -1);
			if (type == MPD_DATA_TYPE_SONG)
			{
				if (song)
				{
					info2_activate();
					info2_fill_song_view(song);
				}
			}
		}
		g_list_foreach(list, (GFunc) gtk_tree_path_free, NULL);
		g_list_free(list);
	}
}

static void pl3_file_browser_row_activated(GtkTreeView * tree, GtkTreePath * tp)
{
	GtkTreeIter iter;
	gchar *song_path;
	gint r_type;

	gtk_tree_model_get_iter(gtk_tree_view_get_model(tree), &iter, tp);
	gtk_tree_model_get(gtk_tree_view_get_model(tree), &iter, MPDDATA_MODEL_COL_PATH, &song_path, MPDDATA_MODEL_ROW_TYPE,
					   &r_type, -1);
	if (song_path == NULL && r_type != -1)
	{
		return;
	}
	if (r_type == MPD_DATA_TYPE_PLAYLIST)
	{
		mpd_playlist_queue_load(connection, song_path);
		mpd_playlist_queue_commit(connection);
	} else if (r_type == MPD_DATA_TYPE_DIRECTORY)
	{
		GtkTreeSelection *selec = gtk_tree_view_get_selection(GTK_TREE_VIEW(pl3_fb_dir_tree));
		GtkTreeModel *model = GTK_TREE_MODEL(pl3_fb_dir_store);

		if (gtk_tree_selection_get_selected(selec, &model, &iter))
		{
			GtkTreeIter citer;
			GtkTreePath *path = gtk_tree_model_get_path(model, &iter);
			gtk_tree_view_expand_row(GTK_TREE_VIEW(pl3_fb_dir_tree), path, FALSE);
			gtk_tree_path_free(path);
			if (gtk_tree_model_iter_children(model, &citer, &iter))
			{
				do
				{
					char *name = NULL;
					char *type = NULL;
					gtk_tree_model_get(model, &citer, PL3_FB_PATH, &name, PL3_FB_ICON, &type, -1);
					if (strcmp(name, song_path) == 0 && strcmp(type, "gtk-open") == 0)
					{
						gtk_tree_selection_select_iter(selec, &citer);
						path = gtk_tree_model_get_path(model, &citer);
						gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(pl3_fb_dir_tree), path, NULL, TRUE, 0.5, 0);
						gtk_tree_path_free(path);
					}
					q_free(name);
					q_free(type);
				} while (gtk_tree_model_iter_next(model, &citer));
			}

		}
	} else if (r_type == -1)
	{
		GtkTreeSelection *selec = gtk_tree_view_get_selection((GtkTreeView *) pl3_fb_dir_tree);
		GtkTreeModel *model = GTK_TREE_MODEL(pl3_fb_dir_store);

		if (gtk_tree_selection_get_selected(selec, &model, &iter))
		{
			GtkTreeIter piter;
			if (gtk_tree_model_iter_parent(model, &piter, &iter))
			{
				GtkTreePath *path = NULL;
				gtk_tree_selection_select_iter(selec, &piter);
				path = gtk_tree_model_get_path(model, &piter);
				gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(pl3_fb_dir_tree), path, NULL, TRUE, 0.5, 0);
				gtk_tree_path_free(path);
			}
		}
	} else
	{
		play_path(song_path);
	}

	q_free(song_path);
}

static void pl3_file_browser_add_to_playlist(GtkWidget * menu, gpointer cb_data)
{
	GtkTreeModel *model = GTK_TREE_MODEL(pl3_fb_store2);
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(pl3_fb_tree));
	gchar *data = g_object_get_data(G_OBJECT(menu), "playlist");
	GList *iter, *list = gtk_tree_selection_get_selected_rows(selection, &model);
	if (list)
	{
		iter = g_list_first(list);
		do
		{
			GtkTreeIter giter;
			if (gtk_tree_model_get_iter(model, &giter, (GtkTreePath *) iter->data))
			{
				gchar *file = NULL;
				int type = 0;
				gtk_tree_model_get(model, &giter, MPDDATA_MODEL_COL_PATH, &file, MPDDATA_MODEL_ROW_TYPE, &type, -1);
				if (type == MPD_DATA_TYPE_SONG)
				{
					mpd_database_playlist_list_add(connection, data, file);
				} else if (type == MPD_DATA_TYPE_DIRECTORY)
				{
					MpdData *data2 = mpd_database_get_directory_recursive(connection, file);
					for (; data2; data2 = mpd_data_get_next(data2))
					{
						if (data2->type == MPD_DATA_TYPE_SONG)
						{
							mpd_database_playlist_list_add(connection, data, data2->song->file);
						}
					}
				}
				g_free(file);
			}
		} while ((iter = g_list_next(iter)));

		g_list_foreach(list, (GFunc) gtk_tree_path_free, NULL);
		g_list_free(list);
	}
}

static gboolean pl3_file_browser_button_release_event(GtkWidget * but, GdkEventButton * event)
{

	int has_item = 0;
	GtkWidget *item;
	GtkWidget *menu = NULL;
	GtkTreeSelection *sel = NULL;
	if (event->button != 3)
		return FALSE;
	menu = gtk_menu_new();
	sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(pl3_fb_tree));
	/* don't show it when where listing custom streams...
	 * show always when version 12..  or when searching in playlist.
	 */
	if (gtk_tree_selection_count_selected_rows(sel) == 1)
	{
		mpd_Song *song = NULL;
		GtkTreeModel *model = GTK_TREE_MODEL(pl3_fb_store2);
		GList *list = gtk_tree_selection_get_selected_rows(sel, &model);
		if (list != NULL)
		{
			GtkTreeIter iter;
			int row_type;
			char *path;
			GtkTreePath *tree_path;
			list = g_list_first(list);
			gtk_tree_model_get_iter(model, &iter, list->data);
			gtk_tree_model_get(model, &iter, MPDDATA_MODEL_COL_PATH, &path, MPDDATA_MODEL_ROW_TYPE, &row_type, -1);
			if (row_type == MPD_DATA_TYPE_SONG)
			{
				if (mpd_server_check_version(connection, 0, 12, 0))
				{
					item = gtk_image_menu_item_new_from_stock(GTK_STOCK_DIALOG_INFO, NULL);
					gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
					g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(pl3_file_browser_show_info), NULL);
					has_item = 1;
				}
			} else if (row_type == MPD_DATA_TYPE_PLAYLIST)
			{
				item = gtk_image_menu_item_new_from_stock(GTK_STOCK_DELETE, NULL);
				gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
				g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(pl3_file_browser_delete_playlist), NULL);
				has_item = 1;
			} else if (row_type == MPD_DATA_TYPE_DIRECTORY)
			{
				item = gtk_image_menu_item_new_with_label(_("Update"));
				gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
											  gtk_image_new_from_stock(GTK_STOCK_REFRESH, GTK_ICON_SIZE_MENU));
				gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
				g_signal_connect(G_OBJECT(item), "activate",
								 G_CALLBACK(pl3_file_browser_update_folder_left_pane), NULL);
				has_item = 1;
			}
			if (row_type != -1)
			{
				/* replace the replace widget */
				item = gtk_image_menu_item_new_with_label(_("Replace"));
				gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
											  gtk_image_new_from_stock(GTK_STOCK_REDO, GTK_ICON_SIZE_MENU));
				gtk_menu_shell_prepend(GTK_MENU_SHELL(menu), item);
				g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(pl3_file_browser_replace_selected), NULL);

				/* add the delete widget */
				item = gtk_image_menu_item_new_from_stock(GTK_STOCK_ADD, NULL);
				gtk_menu_shell_prepend(GTK_MENU_SHELL(menu), item);
				g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(pl3_file_browser_add_selected), NULL);

				playlist_editor_right_mouse(menu, pl3_file_browser_add_to_playlist, NULL);
				has_item = 1;
			}

			tree_path = list->data;
			if (tree_path && gtk_tree_model_get_iter(model, &iter, tree_path))
			{
				gtk_tree_model_get(model, &iter, MPDDATA_MODEL_COL_MPDSONG, &song, -1);
				if (song)
				{
					submenu_for_song(menu, song);
				}
			}
			g_list_foreach(list, (GFunc) gtk_tree_path_free, NULL);
			g_list_free(list);
			q_free(path);
		}
	} else
	{
		/* replace the replace widget */
		item = gtk_image_menu_item_new_with_label(_("Replace"));
		gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
									  gtk_image_new_from_stock(GTK_STOCK_REDO, GTK_ICON_SIZE_MENU));
		gtk_menu_shell_prepend(GTK_MENU_SHELL(menu), item);
		g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(pl3_file_browser_replace_selected), NULL);

		/* add the delete widget */
		item = gtk_image_menu_item_new_from_stock(GTK_STOCK_ADD, NULL);
		gtk_menu_shell_prepend(GTK_MENU_SHELL(menu), item);
		g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(pl3_file_browser_add_selected), NULL);
		has_item = 1;
	}
	gmpc_mpddata_treeview_right_mouse_intergration(GMPC_MPDDATA_TREEVIEW(pl3_fb_tree), GTK_MENU(menu));
	/*  if(has_item) */
	{
		/*
		   item = gtk_image_menu_item_new_with_label(_("Edit Columns"));
		   gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
		   gtk_image_new_from_stock(GTK_STOCK_EDIT, GTK_ICON_SIZE_MENU));
		   gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
		   g_signal_connect(G_OBJECT(item), "activate",
		   G_CALLBACK(pl3_file_browser_edit_columns), NULL);
		 */
		gtk_widget_show_all(menu);
		gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, 0, event->time);
		return TRUE;
	}
	/*  else{
	   gtk_widget_destroy(menu);
	   }
	 */
	return FALSE;
}

static void pl3_file_browser_replace_selected(void)
{
	mpd_playlist_clear(connection);
	if (mpd_check_connected(connection))
	{
		pl3_file_browser_add_selected();
		mpd_player_play(connection);
	}
}

static void pl3_file_browser_add_selected(void)
{
	GtkTreeIter iter;
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(pl3_fb_tree));
	GtkTreeModel *model = GTK_TREE_MODEL(pl3_fb_store2);
	GList *rows = gtk_tree_selection_get_selected_rows(selection, &model);
	int songs = 0;
	int dirs = 0;
	int pl = 0;
	/*gchar *message; */
	if (rows != NULL)
	{
		gchar *name;
		gint type;
		GList *node = g_list_first(rows);
		do
		{
			GtkTreePath *path = node->data;
			gtk_tree_model_get_iter(model, &iter, path);
			gtk_tree_model_get(model, &iter, MPDDATA_MODEL_COL_PATH, &name, MPDDATA_MODEL_ROW_TYPE, &type, -1);
			/* does this bitmask thingy works ok? I think it hsould */
			if (type == MPD_DATA_TYPE_SONG || type == MPD_DATA_TYPE_DIRECTORY)
			{
				/* add them to the add list */
				mpd_playlist_queue_add(connection, name);
				if (type == MPD_DATA_TYPE_DIRECTORY)
					dirs++;
				if (type == MPD_DATA_TYPE_SONG)
					songs++;
			} else if (type == MPD_DATA_TYPE_PLAYLIST)
			{
				mpd_playlist_queue_load(connection, name);
				pl++;
			}
			q_free(name);
		} while ((node = g_list_next(node)) != NULL);
	}
	/* if there are items in the add list add them to the playlist */
	mpd_playlist_queue_commit(connection);
	g_list_foreach(rows, (GFunc) gtk_tree_path_free, NULL);
	g_list_free(rows);
}

static void pl3_file_browser_delete_playlist_from_right(GtkMenuItem * bt)
{
	GtkTreeView *tree = GTK_TREE_VIEW(pl3_fb_dir_tree);
	GtkTreeModel *model = (GtkTreeModel *) pl3_fb_dir_store;
	GtkTreeSelection *selection = gtk_tree_view_get_selection(tree);
	GtkTreeIter iter;
	char *path = NULL;
	/* create a warning message dialog */
	GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(playlist3_get_window()),
											   GTK_DIALOG_MODAL,
											   GTK_MESSAGE_WARNING,
											   GTK_BUTTONS_NONE,
											   _("Are you sure you want to clear the selected playlist?"));

	gtk_dialog_add_buttons(GTK_DIALOG(dialog), GTK_STOCK_NO, GTK_RESPONSE_CANCEL, GTK_STOCK_YES, GTK_RESPONSE_OK, NULL);
	gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_CANCEL);

	if (gtk_tree_selection_get_selected(selection, &model, &iter))
	{
		char *icon = NULL;
		gtk_tree_model_get(model, &iter, 3, &icon, 2, &path, -1);
		if (path && strcmp("media-playlist", icon))
		{
			if (path)
				q_free(path);
			path = NULL;
		}
		q_free(icon);
	}

	if (path == NULL)
	{
		gtk_widget_destroy(dialog);
		return;
	}

	switch (gtk_dialog_run(GTK_DIALOG(dialog)))
	{
	case GTK_RESPONSE_OK:
		mpd_database_delete_playlist(connection, path);
		pl3_file_browser_reupdate();
	default:
		break;
	}
	gtk_widget_destroy(GTK_WIDGET(dialog));
	q_free(path);
}

static void pl3_file_browser_delete_playlist(GtkMenuItem * bt)
{
	char *path = NULL;
	GtkTreeSelection *sel = NULL;
	/* create a warning message dialog */
	GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(playlist3_get_window()),
											   GTK_DIALOG_MODAL,
											   GTK_MESSAGE_WARNING,
											   GTK_BUTTONS_NONE,
											   _("Are you sure you want to clear the selected playlist?"));

	gtk_dialog_add_buttons(GTK_DIALOG(dialog), GTK_STOCK_NO, GTK_RESPONSE_CANCEL, GTK_STOCK_YES, GTK_RESPONSE_OK, NULL);
	gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_CANCEL);

	sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(pl3_fb_tree));
	if (gtk_tree_selection_count_selected_rows(sel) == 1)
	{
		GtkTreeModel *model = GTK_TREE_MODEL(pl3_fb_store2);
		GList *list = gtk_tree_selection_get_selected_rows(sel, &model);
		if (list != NULL)
		{
			GtkTreeIter iter;

			list = g_list_first(list);
			gtk_tree_model_get_iter(model, &iter, list->data);
			gtk_tree_model_get(model, &iter, MPDDATA_MODEL_COL_PATH, &path, -1);
			g_list_foreach(list, (GFunc) gtk_tree_path_free, NULL);
			g_list_free(list);
		}
	}

	if (path == NULL)
	{
		gtk_widget_destroy(dialog);
		return;
	}

	switch (gtk_dialog_run(GTK_DIALOG(dialog)))
	{
	case GTK_RESPONSE_OK:
		mpd_database_delete_playlist(connection, path);
		pl3_file_browser_reupdate();
	default:
		break;
	}
	gtk_widget_destroy(GTK_WIDGET(dialog));
	q_free(path);
}

static void pl3_file_browser_disconnect(void)
{

	if (pl3_fb_tree_ref)
	{
		GtkTreeIter iter;
		GtkTreeIter child;
		if (pl3_fb_dir_store != NULL && gtk_tree_model_get_iter_first(GTK_TREE_MODEL(pl3_fb_dir_store), &iter))
		{
			int valid = gtk_tree_model_iter_children(GTK_TREE_MODEL(pl3_fb_dir_store), &child, &iter);

			while (valid)
			{
				valid = gtk_tree_store_remove(pl3_fb_dir_store, &child);
			}
			/* set unopened */
			gtk_tree_store_set(pl3_fb_dir_store, &iter, PL3_FB_OPEN, FALSE, -1);
			/* add phantom child */
			gtk_tree_store_append(pl3_fb_dir_store, &child, &iter);
		}
	}
	if (pl3_fb_store2)
	{
		gmpc_mpddata_model_set_has_up(pl3_fb_store2, FALSE);
		gmpc_mpddata_model_set_mpd_data(pl3_fb_store2, NULL);
	}
}

static void pl3_file_browser_activate(void)
{
	GtkTreeSelection *selec = gtk_tree_view_get_selection((GtkTreeView *) playlist3_get_category_tree_view());

	GtkTreePath *path = gtk_tree_row_reference_get_path(pl3_fb_tree_ref);
	if (path)
	{
		gtk_tree_selection_select_path(selec, path);
		gtk_tree_path_free(path);
	}
}

static int pl3_file_browser_add_go_menu(GtkWidget * menu)
{
	GtkWidget *item = NULL;
	if (!pl3_file_browser_get_enabled())
		return 0;

	item = gtk_image_menu_item_new_with_label(_("Database"));
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
								  gtk_image_new_from_icon_name("gmpc-database", GTK_ICON_SIZE_MENU));
	gtk_widget_add_accelerator(GTK_WIDGET(item),
							   "activate", gtk_menu_get_accel_group(GTK_MENU(menu)), GDK_F2, 0, GTK_ACCEL_VISIBLE);

	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(pl3_file_browser_activate), NULL);
	return 1;
}

static void pl3_file_browser_connection_changed(MpdObj * mi, int connect, gpointer data)
{
	if (connect)
	{
		if(pl3_fb_tree != NULL)
		{
			GtkTreePath *path = gtk_tree_path_new_from_string("0");
			if (mpd_stats_get_total_songs(connection) == 0)
			{
				gtk_widget_show(pl3_fb_warning_box);
			} else
			{
				gtk_widget_hide(pl3_fb_warning_box);
			}

			gtk_tree_view_expand_to_path(GTK_TREE_VIEW(pl3_fb_dir_tree), path);
			gtk_tree_path_free(path);
		}
	} else
		pl3_file_browser_disconnect();
}

static void pl3_file_browser_status_changed(MpdObj * mi, ChangedStatusType what, void *data)
{
	if (what & MPD_CST_UPDATING)
	{
		if (pl3_fb_vbox != NULL && mpd_status_db_is_updating(connection))
		{
			gtk_widget_hide(pl3_fb_warning_box);
		}
	}
	if (what & MPD_CST_DATABASE)
	{
		pl3_file_browser_reupdate();
	}
    /* We might not be able to read the data.
     * because off insufficient permission.
     * So if permission failed, lets reload things (it is fairly cheap.
     */
    else if (what & MPD_CST_PERMISSION)
    {
		pl3_file_browser_reupdate();
    }
}

static void pl3_file_browser_destroy(void)
{
	if (pl3_fb_tree_ref)
	{
		GtkTreeIter iter;
		GtkTreePath *path;
		path = gtk_tree_row_reference_get_path(pl3_fb_tree_ref);
		if (path)
		{
			if (gtk_tree_model_get_iter(GTK_TREE_MODEL(gtk_tree_row_reference_get_model(pl3_fb_tree_ref)), &iter, path))
			{
				gtk_list_store_remove(GTK_LIST_STORE(gtk_tree_row_reference_get_model(pl3_fb_tree_ref)), &iter);
			}
			gtk_tree_path_free(path);
		}
		gtk_tree_row_reference_free(pl3_fb_tree_ref);
		pl3_fb_tree_ref = NULL;
	}
	if (pl3_fb_vbox)
	{
		gtk_widget_destroy(pl3_fb_vbox);
        pl3_fb_vbox = NULL;
	}
	if (pl3_fb_store2)
	{
		g_object_unref(pl3_fb_store2);
        pl3_fb_store2 = NULL;
	}
    if(pl3_fb_tree) {
        pl3_fb_tree = NULL;
    }
}

static void pl3_file_browser_save_myself(void)
{
	if (pl3_fb_tree_ref)
	{
		GtkTreePath *path = gtk_tree_row_reference_get_path(pl3_fb_tree_ref);
		if (path)
		{
			gint *indices = gtk_tree_path_get_indices(path);
			g_log(LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Saving myself to position: %i\n", indices[0]);
			cfg_set_single_value_as_int(config, "file-browser", "position", indices[0]);
			gtk_tree_path_free(path);
		}
	}
}

static void pl3_file_browser_open_path_real(gchar ** dirs, GtkTreeIter * parent)
{
	GtkTreeIter iter;
	if ((dirs[0]) == NULL)
	{
		/* found dir */
		GtkTreePath *path = NULL;
		gtk_tree_selection_select_iter(gtk_tree_view_get_selection(GTK_TREE_VIEW(pl3_fb_dir_tree)), parent);

		path = gtk_tree_model_get_path(GTK_TREE_MODEL(pl3_fb_dir_store), parent);
		gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(pl3_fb_dir_tree), path, NULL, TRUE, 0.5, 0);
		gtk_tree_path_free(path);
		return;
	}
	if (gtk_tree_model_iter_children(GTK_TREE_MODEL(pl3_fb_dir_store), &iter, parent))
	{
		do
		{
			gchar *name = NULL;
			gtk_tree_model_get(GTK_TREE_MODEL(pl3_fb_dir_store), &iter, PL3_FB_NAME, &name, -1);
			if (name && g_utf8_collate(name, dirs[0]) == 0)
			{
				GtkTreePath *tpath = gtk_tree_model_get_path(GTK_TREE_MODEL(pl3_fb_dir_store), &iter);
				gtk_tree_view_expand_row(GTK_TREE_VIEW(pl3_fb_dir_tree), tpath, FALSE);
				gtk_tree_path_free(tpath);
				pl3_file_browser_open_path_real(&dirs[1], &iter);
				g_free(name);
				return;
			}
			if (name)
				g_free(name);
		} while (gtk_tree_model_iter_next(GTK_TREE_MODEL(pl3_fb_dir_store), &iter));
	}

}

void pl3_file_browser_open_path(const gchar * path)
{
	pl3_file_browser_activate();
	if (pl3_fb_dir_store)
	{
		gchar **dirs = g_strsplit(path, G_DIR_SEPARATOR_S, -1);
		if (dirs)
		{
			GtkTreeIter iter;
			if (gtk_tree_model_iter_children(GTK_TREE_MODEL(pl3_fb_dir_store), &iter, NULL))
			{
				GtkTreePath *tpath = gtk_tree_model_get_path(GTK_TREE_MODEL(pl3_fb_dir_store), &iter);
				gtk_tree_view_expand_row(GTK_TREE_VIEW(pl3_fb_dir_tree), tpath, FALSE);
				gtk_tree_path_free(tpath);
				pl3_file_browser_open_path_real(dirs, &iter);
			}
			g_strfreev(dirs);
		}
	}
}

static gboolean pl3_file_browser_is_field_supported(int tag)
{
	if (tag == MPD_TAG_NUM_OF_ITEM_TYPES)
		return TRUE;
	return mpd_server_tag_supported(connection, tag);
}

static MpdData *pl3_file_browser_is_search(const int num_field, const gchar * search_string, GError ** error)
{
	MpdData *data_t = NULL;
	if (num_field == MPD_TAG_NUM_OF_ITEM_TYPES)
	{
		data_t = advanced_search(search_string, FALSE);
	} else
	{
		gchar **splitted = tokenize_string(search_string);
		int i = 0;
		gboolean found = FALSE;
		for (i = 0; splitted && splitted[i]; i++)
		{
			if (!found)
			{
				mpd_database_search_start(connection, FALSE);
				found = TRUE;
			}
			mpd_database_search_add_constraint(connection, num_field, splitted[i]);
		}
		if (splitted)
			g_strfreev(splitted);
		if (found)
		{
			data_t = mpd_database_search_commit(connection);
		}
	}
	return data_t;
}

void pl3_find2_ec_database(gpointer user_data, const char *param)
{
	pl3_find2_select_plugin_id(file_browser_plug.id);
	pl3_find2_do_search_any(param);
}

static void pl3_file_browser_plugin_init(void)
{
	gmpc_easy_command_add_entry(gmpc_easy_command,
								_("search database"), ".*",
								_("Search database <query>"), (GmpcEasyCommandCallback *) pl3_find2_ec_database, NULL);
}

/***  Integrates the file browser in the right mouse menu ****/

/* Handle the click on the menu item */
static void pl3_file_browser_option_menu_activate (GtkMenuItem *item, gpointer data)
{
    /* Get previously stored path from item */
    const gchar *path = g_object_get_data(G_OBJECT(item), "path");
    /* if there is one, act */
    if(path != NULL)
    {
        /* This function calls the file browser and opens the path */
        pl3_file_browser_open_path(path);
    }
}

/* add item to menu */
static int pl3_file_browser_option_menu(GmpcMpdDataTreeview *tree, GtkMenu *menu)
{
    int retv = 0;
    GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));
    /* Only works on 1 song */
    if(gtk_tree_selection_count_selected_rows(selection) == 1)
    {
        GList *list;
        GtkTreeModel *model;
        mpd_Song *song = NULL;
        /* Get a list of selected rows (1) */
        list = gtk_tree_selection_get_selected_rows(selection, &model);
        if(list) {
            GtkTreeIter iter;
            GtkTreePath *path  = (GtkTreePath *)list->data;
            /* Convert the path into an actual iter we can use to get values from the model */
            if(gtk_tree_model_get_iter(model, &iter,path))
            {
                /* Get a pointer to the mpd_Song in the model. */
                gtk_tree_model_get(model, &iter, MPDDATA_MODEL_COL_MPDSONG, &song, -1);
                /* Only show if song exists and has a path */
                if(song && song->file)
                {
                    gchar *scheme = g_uri_parse_scheme(song->file);
                    /* If path has a scheme it is not in our db */
                    if(!scheme)
                    {
                        GtkWidget *item = gtk_image_menu_item_new_with_label(_("Lookup directory in database"));
                        /* Add folder icon */
                        gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
                                gtk_image_new_from_stock("gtk-open", GTK_ICON_SIZE_MENU));

                        /* Attach a copy of the path to open, so we don't have to look it up again */
                        g_object_set_data_full(G_OBJECT(item), "path", g_path_get_dirname(song->file), (GDestroyNotify)g_free);
                        gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

                        /* Connect signal */
                        g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(pl3_file_browser_option_menu_activate), NULL);
                        retv++;
                    }
                    else g_free(scheme);

                }
            }
        }
        /* Free the list of rows */
        g_list_foreach (list, (GFunc) gtk_tree_path_free, NULL);
        g_list_free (list);
    }

    return retv;
}
