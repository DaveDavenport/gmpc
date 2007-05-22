#include <gtk/gtk.h>
#include <glib/gstdio.h>
#include <gdk/gdkkeysyms.h>


#include <gmpc/plugin.h>
#include <libmpd/libmpd.h>
#include <gmpc/gmpc-mpddata-model.h>
#include <gmpc/gmpc-mpddata-treeview.h>
#include "browsers/playlist3-playlist-editor.h"
#include "main.h"

#define DEFAULT_MARKUP_BROWSER 	"[%name%: &[%artist% - ]%title%]|%name%|[%artist% - ]%title%|%shortfile%|"

static GtkTreeRowReference *playlist_editor_browser_ref = NULL;
static GtkWidget *playlist_editor_browser = NULL;


GtkWidget *playlist_editor_icon_view = NULL;
GmpcMpdDataModel *playlist_editor_list_store = NULL;

enum {
	PL_NAME,
	PL_IMAGE,
	PL_NUM_COLS
};

GtkListStore *playlist_editor_store = NULL;
/**
 * Functions from gmpc 
 */
int connection_get_port();
char *connection_get_hostname();

/**
 * Enable/Disable plugin
 */
static int playlist_editor_get_enabled(void)
{
	return cfg_get_single_value_as_int_with_default(config, "playlist-plugin", "enable", TRUE);
}
static void playlist_editor_set_enabled(int enabled)
{
	cfg_set_single_value_as_int(config, "playlist-plugin", "enable", enabled);
};


/**
 * Browser extention 
 */
gmpcPlBrowserPlugin playlist_editor_gbp = {
  playlist_editor_browser_add,   /* Add */
  playlist_editor_browser_selected,   /* Selected */
  playlist_editor_browser_unselected,   /* Unselected */
  playlist_editor_browser_changed,   /* Changed */
  NULL,
  playlist_editor_browser_cat_menu,   /* */
  NULL,   /* cat key press */
  playlist_editor_go_menu, 
  playlist_editor_key_press 
};

gmpcPlugin playlist_editor_plugin = {
  "Favorites Plugin",
  {0,15,0},
  GMPC_PLUGIN_PL_BROWSER, /* type    */
  0,                      /* id      */
  NULL,                   /* Path    */
  /* functions */
  playlist_editor_init,         /* Init    */
  playlist_editor_destroy,      /* Destroy */
  &playlist_editor_gbp,         /* Browser plugin */
  NULL,                   /* Status changed */
  playlist_editor_conn_changed, /* Connection changed */
  NULL,                   /* Preferences */
  NULL,                   /* MetaData */
  playlist_editor_get_enabled,  /* get Enabled */
  playlist_editor_set_enabled   /* set Enabled */
};

/**
 * Init plugin
 */
void playlist_editor_init(void)
{
}
/**
 * Destroy Plugin
 */
void playlist_editor_destroy(void)
{
  if(playlist_editor_browser)
  {
    gtk_widget_destroy(playlist_editor_browser);
    playlist_editor_browser = NULL;
  }
}


/**
 * Connection changed
 */
void playlist_editor_conn_changed(MpdObj *mi, int connect, void *userdata)
{
}


void playlist_editor_browser_add(GtkWidget *cat_tree)
{
  GtkTreeStore *pl3_tree = playlist3_get_category_tree_store();
	GtkTreePath *path = NULL;
	GtkTreeIter iter;
  /* Check if enabled */
  if(!cfg_get_single_value_as_int_with_default(config, "playlist-plugin", "enable", TRUE)) return;

	gtk_tree_store_append(pl3_tree, &iter, NULL);
	gtk_tree_store_set(pl3_tree, &iter, 
			PL3_CAT_TYPE, playlist_editor_plugin.id,
			PL3_CAT_TITLE, "Playlist Editor",
			PL3_CAT_INT_ID, "",
			PL3_CAT_ICON_ID, "gtk-yes",
			PL3_CAT_PROC, TRUE,
			PL3_CAT_ICON_SIZE,GTK_ICON_SIZE_DND,-1);
	/**
	 * Clean up old row reference if it exists
	 */
	if (playlist_editor_browser_ref)
	{
		gtk_tree_row_reference_free(playlist_editor_browser_ref);
		playlist_editor_browser_ref = NULL;
	}
	/**
	 * create row reference
	 */
	path = gtk_tree_model_get_path(GTK_TREE_MODEL(playlist3_get_category_tree_store()), &iter);
	if (path)
	{
		playlist_editor_browser_ref = gtk_tree_row_reference_new(GTK_TREE_MODEL(playlist3_get_category_tree_store()), path);
		gtk_tree_path_free(path);
	}

	
}

static void playlist_editor_browser_playlist_editor_selected(GtkIconView *giv, GtkTreePath *path, gpointer data)
{
	gchar *pl_path = NULL;
	GtkTreeIter iter;

	if(gtk_tree_model_get_iter(GTK_TREE_MODEL(playlist_editor_store), &iter, path))
	{
		MpdData *data ;
		gtk_tree_model_get(GTK_TREE_MODEL(playlist_editor_store), &iter, PL_NAME, &pl_path, -1);
		data = mpd_database_get_playlist_content(connection, pl_path); 
		gmpc_mpddata_model_set_mpd_data(playlist_editor_list_store, data);		
		g_free(pl_path);
	}
}
static void playlist_editor_browser_playlist_editor_changed(GtkIconView *giv, gpointer data)
{

	gmpc_mpddata_model_set_mpd_data(playlist_editor_list_store, NULL);		
	/* iter all the selected items (aka 1) */
	gtk_icon_view_selected_foreach(giv, playlist_editor_browser_playlist_editor_selected, NULL);
}

static void playlist_editor_fill_list(void)
{
	if(playlist_editor_browser)
	{
		MpdData *data = mpd_database_get_directory(connection, "/");
		gtk_list_store_clear(playlist_editor_store);
		for(;data;data =mpd_data_get_next(data))
		{
			if(data->type ==  MPD_DATA_TYPE_PLAYLIST)
			{
				GtkTreeIter iter;
				GdkPixbuf *pb = NULL; 
				pb = gtk_icon_theme_load_icon(gtk_icon_theme_get_default(), "media-playlist", 64, 0,NULL);
				gtk_list_store_append(playlist_editor_store, &iter);
				gtk_list_store_set(playlist_editor_store, &iter,PL_NAME, data->playlist, PL_IMAGE, pb, -1);
				if(pb)
					g_object_unref(pb);

			}
		}
	}
}
static void playlist_editor_browser_activate_cursor_item(GtkIconView *giv, gpointer data)
{
	printf("activated\n");
	gchar *pl_path = NULL;
	GtkTreeIter iter;
	GList *it,*list = gtk_icon_view_get_selected_items(giv);
	for(it = g_list_first(list);it; it = g_list_next(it))
	{
		GtkTreePath *path = it->data;
		if(gtk_tree_model_get_iter(GTK_TREE_MODEL(playlist_editor_store), &iter, path))
		{
			gtk_tree_model_get(GTK_TREE_MODEL(playlist_editor_store), &iter, PL_NAME, &pl_path, -1);
			mpd_playlist_queue_load(connection, pl_path);
			g_free(pl_path);
		}
	}
	mpd_playlist_queue_commit(connection);
	g_list_foreach(list, (GFunc)gtk_tree_path_free, NULL);
	g_list_free(list);
}
/*********
 * Playlist list handling 
 */
static void playlist_editor_list_delete_songs(GtkButton *button, GtkTreeView *tree) 
{
	gchar *pl_path = NULL;
	GList *it,*list = gtk_icon_view_get_selected_items(GTK_ICON_VIEW(playlist_editor_icon_view));
	for(it = g_list_first(list);it; it = g_list_next(it))
	{
		GtkTreePath *path = it->data;
		GtkTreeIter iter;
		if(gtk_tree_model_get_iter(GTK_TREE_MODEL(playlist_editor_store), &iter, path) && !pl_path)
		{
			gtk_tree_model_get(GTK_TREE_MODEL(playlist_editor_store), &iter, PL_NAME, &pl_path, -1);
		}
	}
	g_list_foreach(list, (GFunc)gtk_tree_path_free, NULL);
	g_list_free(list);

	if(pl_path)
	{
		MpdData *data2 = NULL;
		GtkTreeSelection *select = gtk_tree_view_get_selection(tree);
		GtkTreeModel *model = gtk_tree_view_get_model(tree);
		GList *data,*list= gtk_tree_selection_get_selected_rows(select, &model);
		for(data = g_list_last(list); data; data = g_list_previous(data))
		{
			GtkTreePath *path = data->data;
			GtkTreeIter iter;
			if(gtk_tree_model_get_iter(model, &iter, path))
			{
				int *pos = gtk_tree_path_get_indices(path);
				mpd_database_playlist_list_delete(connection, pl_path,pos[0]);
				printf("pc: %s:%i\n", pl_path, pos[0]);

			}
		}
		mpd_playlist_queue_commit(connection);
		g_list_foreach (list,(GFunc) gtk_tree_path_free, NULL);
		g_list_free (list);

		
		data2 = mpd_database_get_playlist_content(connection, pl_path); 
		gmpc_mpddata_model_set_mpd_data(playlist_editor_list_store, data2);		

		g_free(pl_path);
	}
}


static void playlist_editor_list_add_songs(GtkButton *button, GtkTreeView *tree) 
{
	GtkTreeSelection *select = gtk_tree_view_get_selection(tree);
	GtkTreeModel *model = gtk_tree_view_get_model(tree);
	GList *data,*list= gtk_tree_selection_get_selected_rows(select, &model);
	for(data = g_list_first(list); data; data = g_list_next(data))
	{
		GtkTreePath *path = data->data;
		GtkTreeIter iter;
		if(gtk_tree_model_get_iter(model, &iter, path))
		{
			gchar *song_path;
			gtk_tree_model_get(model, &iter,MPDDATA_MODEL_COL_PATH, &song_path, -1); 
			mpd_playlist_queue_add(connection, song_path);

			g_free(song_path);
		}
	}
	mpd_playlist_queue_commit(connection);
	g_list_foreach (list,(GFunc) gtk_tree_path_free, NULL);
	g_list_free (list);
}

static gboolean playlist_editor_key_pressed(GtkTreeView *tree, GdkEventButton *button, gpointer data)
{
	if(button->button == 3)
	{
		GtkTreeSelection *select = gtk_tree_view_get_selection(tree);
		if(gtk_tree_selection_count_selected_rows(select) > 0)
		{
			GtkWidget *menu = gtk_menu_new();
			GtkWidget *item = gtk_image_menu_item_new_from_stock(GTK_STOCK_ADD,NULL);
			gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
			g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(playlist_editor_list_add_songs), tree);

			item = gtk_image_menu_item_new_from_stock(GTK_STOCK_DELETE,NULL);
			gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
			g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(playlist_editor_list_delete_songs), tree);


			gtk_widget_show_all(menu);
			gtk_menu_popup(GTK_MENU(menu), NULL, NULL,NULL, NULL, button->button, button->time);
			return TRUE;
		}
		
	}
	return FALSE;
}

static void playlist_editor_browser_init()
{
	GtkWidget *tree = NULL;
	GtkWidget *sw = NULL;
	/* */
	playlist_editor_browser = gtk_vbox_new(FALSE,0);
	/** browser */
	sw = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw), GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw), GTK_SHADOW_ETCHED_IN);
	gtk_widget_set_size_request(sw, -1, 100);	
	gtk_box_pack_start(GTK_BOX(playlist_editor_browser), sw, FALSE, TRUE,0);

	/* icon view*/
	playlist_editor_store = gtk_list_store_new(PL_NUM_COLS, G_TYPE_STRING, GDK_TYPE_PIXBUF);

	playlist_editor_icon_view = tree = gtk_icon_view_new_with_model(GTK_TREE_MODEL(playlist_editor_store));
	gtk_icon_view_set_selection_mode(GTK_ICON_VIEW(tree), GTK_SELECTION_BROWSE);
	gtk_icon_view_set_pixbuf_column(GTK_ICON_VIEW(tree), PL_IMAGE);
	gtk_icon_view_set_text_column(GTK_ICON_VIEW(tree), PL_NAME);
	gtk_container_add(GTK_CONTAINER(sw), tree);

	g_signal_connect(G_OBJECT(tree), "selection-changed", G_CALLBACK(playlist_editor_browser_playlist_editor_changed), NULL);
	g_signal_connect(G_OBJECT(tree), "item-activated", G_CALLBACK(playlist_editor_browser_activate_cursor_item), NULL);
	/* file list */

	sw = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw), GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw), GTK_SHADOW_ETCHED_IN);
	
	gtk_box_pack_start(GTK_BOX(playlist_editor_browser), sw, TRUE, TRUE,0);

	playlist_editor_list_store= gmpc_mpddata_model_new();

	tree = gmpc_mpddata_treeview_new("playlist-browser",FALSE, GTK_TREE_MODEL(playlist_editor_list_store));
	gtk_container_add(GTK_CONTAINER(sw), tree);


	g_signal_connect(G_OBJECT(tree), "button-press-event", G_CALLBACK(playlist_editor_key_pressed), NULL);
	g_object_ref(playlist_editor_browser);

	playlist_editor_browser_fill_list();
	gtk_widget_show_all(playlist_editor_browser);
}

void playlist_editor_browser_selected(GtkWidget *container)
{
  if(!playlist_editor_browser) {
    playlist_editor_browser_init();
  }
  gtk_container_add(GTK_CONTAINER(container), playlist_editor_browser);
  gtk_widget_show_all(playlist_editor_browser);
}

void playlist_editor_browser_unselected(GtkWidget *container)
{
  gtk_container_remove(GTK_CONTAINER(container), playlist_editor_browser);
}

void playlist_editor_browser_changed(GtkWidget *tree, GtkTreeIter *iter)
{
	playlist_editor_fill_list();
}


void playlist_editor_browser_fill_list(void)
{



}

int playlist_editor_browser_cat_menu(GtkWidget *menu, int type, GtkWidget *tree, GdkEventButton *event)
{
  if(type == playlist_editor_plugin.id)
  {
  }
  return 0;
}

/**
 * MW Intergration
 */
int playlist_editor_go_menu(GtkWidget *menu)
{
  return 0;
}

int playlist_editor_key_press(GtkWidget *mw, GdkEventKey *event, int type)
{
  return FALSE;
}


void playlist_editor_right_mouse(GtkWidget *menu, void (*add_to_playlist)(GtkWidget *menu))
{
	GtkWidget *sitem;
	GtkWidget *item;
	GtkWidget *smenu;
	smenu  = gtk_menu_new();
	{
		MpdData *data = mpd_database_get_directory(connection, "/");
		for(;data;data =mpd_data_get_next(data))
		{
			if(data->type ==  MPD_DATA_TYPE_PLAYLIST)
			{
				sitem = gtk_image_menu_item_new_with_label(data->playlist);
				gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(sitem), 
						gtk_image_new_from_icon_name("media-playlist", GTK_ICON_SIZE_MENU));
				g_object_set_data_full(G_OBJECT(sitem),"playlist", g_strdup(data->playlist), g_free);
				gtk_menu_shell_append(GTK_MENU_SHELL(smenu), sitem);
				g_signal_connect(G_OBJECT(sitem), "activate", G_CALLBACK(add_to_playlist), NULL);
				gtk_widget_show(sitem);                             				
			}
		}
	}

	/* Add */
	item = gtk_menu_item_new_with_label(_("Add to playlist"));
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), smenu);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	gtk_widget_show(item);
	
	gtk_widget_show(smenu);
}

