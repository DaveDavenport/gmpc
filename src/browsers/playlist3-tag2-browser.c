/**
 * TODO:
 * $ If disconnected, don't destroy all tag browsers, just clear the first one, if connected refill the first one
 * $ Before adding albums use some sort of sorting. (see old tag browser?)
 */

/**
 * Released under the GPL.
 *
 */
#include <stdio.h>
#include <string.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <libmpd/debug_printf.h>
#include "plugin.h"
#include "main.h"
#include "misc.h"
#include "gmpc-mpddata-treeview.h"
#include "gmpc-mpddata-model.h"
#include <libmpd/libmpd-internal.h>

/**
 * dirty hack to workaround single parameter for now 
 */
static int counter;

static void tag2_init(void);
static void tag2_destroy(void);
static void tag2_set_enabled(int enabled);
static int tag2_get_enabled(void);

static void tag2_save_myself(void);
static void tag2_browser_add(GtkWidget *cat_tree);

/* connection changed signal handling */
static void tag2_connection_changed(MpdObj *mi, int connect, gpointer data);
/* intergration in gmpc */
static void tag2_browser_selected(GtkWidget *container);
static void tag2_browser_unselected(GtkWidget *container);
static void tag2_browser_selection_changed(GtkWidget *tree, GtkTreeIter *iter);
static int tag2_browser_add_go_menu(GtkWidget *menu);

/**
 *  preferences 
 */
static void tag2_pref_construct(GtkWidget *container);
static void tag2_pref_destroy(GtkWidget *container);
static GtkWidget *pref_vbox = NULL;
static GtkWidget *pref_entry = NULL;

/** 
 * Preferences structure 
 */
gmpcPrefPlugin tag2_prefs = {
	.construct = tag2_pref_construct,
	.destroy = tag2_pref_destroy
};


/**
 * Browser plugin 
 * Consists of add, selected, unselected.
 */
gmpcPlBrowserPlugin tag2_browser_plugin ={
	.add = tag2_browser_add,
	.selected = tag2_browser_selected,
	.cat_selection_changed = tag2_browser_selection_changed,
	.unselected = tag2_browser_unselected,
	.add_go_menu = tag2_browser_add_go_menu
};

gmpcPlugin tag2_plug = {
	.name = "Tag based browser",
	.version = {0,15,0},
	.plugin_type = GMPC_PLUGIN_PL_BROWSER,
	.init = tag2_init,
	.destroy = tag2_destroy,
	.save_yourself = tag2_save_myself,
	.get_enabled = tag2_get_enabled,
	.set_enabled = tag2_set_enabled,
	.browser = &tag2_browser_plugin,
	.pref = &tag2_prefs,
	.mpd_connection_changed = tag2_connection_changed,
};
/** Little hack to work around gmpc's limitations */
//static GHashTable *tag2_ht = NULL;
static GList *tag2_ht = NULL;
/** This stucture contains all the needed data for a browser
 */
typedef struct _tag_browser {
	gchar 			*name;
	/* The key used as ID in gmpc, and as config id and name */
	gchar 			*key;
	/* The paned window that is packed into gmpc */
	GtkWidget 		*tag2_vbox;
	/* The hbox that is used to pack the tag browsers in to the browser window */
	GtkWidget 		*tag_hbox;
	/* The GmpcMpdDataTreeView that shows the songs */
	GtkTreeView 	*tag_songlist;
	/* List with tag_element's (the tag browsers that are in tag_hbox) */
	GList 			*tag_lists;
	/* GtkTreeRowReference */
	GtkTreeRowReference *ref_iter;
} tag_browser;

/* The current visible browser, this is needed to workaround gmpc's limitation */
static GtkWidget *tag2_current = NULL;
static void tag2_destroy_browser(tag_browser *browser, gpointer user_data);
static void tag2_connection_changed_foreach(tag_browser *browser, gpointer data);
static void tag2_init_browser(tag_browser *browser);

/**
 * Adds a browser to gmpc's category browser
 */
static void tag2_browser_add_browser(GtkWidget *cat_tree, char *key)
{
	GtkTreePath *path;
	GtkTreeIter iter;
	tag_browser *tb;
	gchar *group = g_strdup_printf("tag2-plugin:%s",key);
	gchar *name = cfg_get_single_value_as_string_with_default(config, group, "name", "default");
	GtkTreeStore *pl3_tree = (GtkTreeStore  *)gtk_tree_view_get_model(GTK_TREE_VIEW(cat_tree));
	gint pos = cfg_get_single_value_as_int_with_default(config, group,"position",50+g_list_length(tag2_ht));
	g_free(group);
//	gtk_tree_store_append(pl3_tree, &iter, NULL);
	playlist3_insert_browser(&iter, pos);
	gtk_tree_store_set(pl3_tree, &iter, 
			PL3_CAT_TYPE, tag2_plug.id,
			PL3_CAT_TITLE, name,
			PL3_CAT_INT_ID,key,
			PL3_CAT_ICON_ID, "tag-browser",
			PL3_CAT_PROC, TRUE,
			PL3_CAT_ICON_SIZE,GTK_ICON_SIZE_DND,-1);
	path = gtk_tree_model_get_path(GTK_TREE_MODEL(pl3_tree), &iter);
		
	tb = g_malloc0(sizeof(*tb));
	tb->name = name;
	tb->key = g_strdup(key);
	/* get a reference to the key */
	tb->ref_iter = gtk_tree_row_reference_new(GTK_TREE_MODEL(pl3_tree),path);
	//g_hash_table_insert(tag2_ht, tb->key, tb);
	tag2_ht = g_list_append(tag2_ht,tb);
	tag2_init_browser(tb);
	gtk_tree_path_free(path);
	pl3_update_go_menu();	

}
static void tag2_browser_add(GtkWidget *cat_tree)
{
	conf_mult_obj *cmo,*iter;
	if(!tag2_get_enabled())
		return;
	/**
	 * Init hash table if it does not extists
	 */
	if(!tag2_ht)
	{
		/* init a hash-table based on strings */
		//tag2_ht = g_hash_table_new(g_str_hash, g_str_equal);
	}

	cmo = cfg_get_key_list(config, "tag2-browsers");
	for(iter=cmo;iter;iter = iter->next)
	{
		tag2_browser_add_browser(cat_tree, iter->key);
	}
	if(cmo)
		cfg_free_multiple(cmo);

}


typedef struct _tag_element{
	GtkWidget 		*tree;
	GtkTreeModel 	*model;
	GtkWidget		*sw;
	int				type;
	int 			index;
	tag_browser		*browser;
}tag_element;


/**
 * Get/Set enabled
 */
int tag2_get_enabled()
{
	return cfg_get_single_value_as_int_with_default(config, "tag2-plugin", "enable", TRUE);
}
static void tag2_set_enabled(int enabled)
{
	cfg_set_single_value_as_int(config, "tag2-plugin", "enable", enabled);
	if(enabled && !tag2_ht)
	{
		GtkTreeView *tree = playlist3_get_category_tree_view();
		tag2_browser_add((GtkWidget *)tree);
	} else if (!enabled  && tag2_ht ) {
		tag2_destroy();
	}
}
/* 
 * Destroy the browser
 */
static void tag2_destroy()
{
	/* clear all the browsers */	
//	g_hash_table_foreach(tag2_ht,(GHFunc)tag2_destroy_browser, NULL);
	g_list_foreach(tag2_ht, (GFunc)tag2_destroy_browser, NULL);
	/* free the hash table */
	//g_hash_table_destroy(tag2_ht);
	g_list_free(tag2_ht);
	tag2_ht = NULL;
}

static void tag2_destroy_tag(tag_element *te)
{
	gtk_widget_destroy(te->sw);
	g_free(te);

}





/**
 * Handles right mouse press event
 * this function fixes the behauviour of gtk to something more logic, 
 * the actually handling of the press happens in the release event
 */
static gboolean tag2_song_list_button_press_event(GtkWidget *but, GdkEventButton *event)
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

/**
 * Add songs from the selected browser entry of te
 */
static void tag2_browser_add_selected(GtkWidget *item, tag_element *te)
{
	GtkTreeIter iter;
	GtkTreeSelection *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(te->tree));	
	if(gtk_tree_selection_get_selected(sel, &(te->model), &iter))
	{
		int i;
		MpdData *data;
		/* now get the song content, this needs all the fields of the previous 
		*/
		mpd_database_search_start(connection, TRUE);
		/* add constraints based on previous browsers */
		for(i=0;i<te->index;i++)
		{
			tag_element *te3  = g_list_nth_data(te->browser->tag_lists, i);			
			sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(te3->tree));	
			if(gtk_tree_selection_get_selected(sel, &(te3->model), &iter))
			{	
				gchar *value;
				gtk_tree_model_get(te3->model, &iter, MPDDATA_MODEL_COL_SONG_TITLE, &value, -1);
				mpd_database_search_add_constraint(connection, te3->type, value);
				g_free(value);
			}
		}
		data = mpd_database_search_commit(connection);  
		/* if there is a result queue them and add them */
		if(data)
		{
            data = misc_sort_mpddata_by_album_disc_track(data);
            for(;data;data = mpd_data_get_next(data))
			{
				mpd_playlist_queue_add(connection,data->song->file);
			}
			mpd_playlist_queue_commit(connection);
		}
	}
}

/**
 * Replace playlist with content of selected browser
 */
static void tag2_browser_replace_selected(GtkWidget *item, tag_element *te)
{
	mpd_playlist_clear(connection);
	tag2_browser_add_selected(item, te);
	mpd_player_play(connection);
}

/**
 * Handles right mouse release on song list
 */
static gboolean tag2_browser_button_release_event(GtkTreeView *tree, GdkEventButton *event, tag_element *te)
{
	/* only on right mouse click */
	if(event->button == 3)
	{
		GtkWidget *menu, *item;
		GtkTreeSelection *selection = gtk_tree_view_get_selection(tree);
		int count = gtk_tree_selection_count_selected_rows(selection);
		/* if nothing is selected return */
		if(count == 0)
			return FALSE;

		/* create menu to popup */
		menu = gtk_menu_new();
		/* add the add widget */
		item = gtk_image_menu_item_new_from_stock(GTK_STOCK_ADD,NULL);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
		g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(tag2_browser_add_selected), te);
		/* add the replace widget */
		item = gtk_image_menu_item_new_with_label("Replace");
		gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
				gtk_image_new_from_stock(GTK_STOCK_REDO, GTK_ICON_SIZE_MENU));
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
		g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(tag2_browser_replace_selected), te);

		/* popup */
		gtk_widget_show_all(GTK_WIDGET(menu));
		gtk_menu_popup(GTK_MENU(menu), NULL, NULL,NULL, NULL, 0, event->time);
		return TRUE;

	}
	return FALSE;
}

static void tag2_changed(GtkTreeSelection *sel, tag_element *te)
{
	GtkTreeIter iter;
	tag_browser *browser = te->browser;
	int i;
	/* Clear songs list */
	GtkTreeModel *model = gtk_tree_view_get_model(browser->tag_songlist);
	gmpc_mpddata_model_set_mpd_data(GMPC_MPDDATA_MODEL(model), NULL);
	/* clear the depending browsers  (All the browsers on the right)*/
	for(i=g_list_length(browser->tag_lists)-1; i >= te->index;i--)
	{
		tag_element *te2 = g_list_nth_data(browser->tag_lists, i);
		if(te2)
        {
			gmpc_mpddata_model_set_mpd_data(GMPC_MPDDATA_MODEL(te2->model), NULL);
            gmpc_mpddata_model_set_request_artist(GMPC_MPDDATA_MODEL(te2->model), NULL);
        }
	}
	/* check if the user selected a row, if not do nothing */
	sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(te->tree));	
	if(gtk_tree_selection_get_selected(sel, &(te->model), &iter) && mpd_check_connected(connection))
	{
		tag_element *te2 ;
		MpdData *data;
		/* only do the following query if it isn't the last one */
		if(te->index < g_list_length(browser->tag_lists))
		{	
			te2 = g_list_nth_data(browser->tag_lists,  te->index);
			/* Search for the fields of the next tag, this needs the value/type of all the previous,
			 * Parsed from left to right 
			 */
			mpd_database_search_field_start(connection, te2->type);
			/* fil in the next */
			for(i=0;i<(te2->index-1);i++)
			{
				tag_element *te3  = g_list_nth_data(browser->tag_lists, i);			
				sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(te3->tree));	
				if(gtk_tree_selection_get_selected(sel, &(te3->model), &iter))
				{	
					gchar *value;
					gtk_tree_model_get(te3->model, &iter, MPDDATA_MODEL_COL_SONG_TITLE, &value, -1);
					mpd_database_search_add_constraint(connection, te3->type, value);
                    if(te3->index == (te2->index-1))
                    {           
                        if(te3->type == MPD_TAG_ITEM_ARTIST)
                        {
                            gmpc_mpddata_model_set_request_artist(GMPC_MPDDATA_MODEL(te2->model), value);

                        }
                    }

                    g_free(value);
				}
            }
            data = mpd_database_search_commit(connection);
            
			gmpc_mpddata_model_set_mpd_data(GMPC_MPDDATA_MODEL(te2->model), data);
		}
		/* now get the song content, this needs all the fields of the previous 
		 */
		mpd_database_search_start(connection, TRUE);
		/* fil in the next */
		for(i=0;i<te->index;i++)
		{
			tag_element *te3  = g_list_nth_data(browser->tag_lists, i);			
			sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(te3->tree));	
			if(gtk_tree_selection_get_selected(sel, &(te3->model), &iter))
			{	
				gchar *value;
				gtk_tree_model_get(te3->model, &iter, MPDDATA_MODEL_COL_SONG_TITLE, &value, -1);
				mpd_database_search_add_constraint(connection, te3->type, value);
				g_free(value);
			}
		}
		data = mpd_database_search_commit(connection);                                          		
		gmpc_mpddata_model_set_mpd_data(GMPC_MPDDATA_MODEL(gtk_tree_view_get_model(browser->tag_songlist)), data);                                      	
	}
}
static void tag2_destroy_browser(tag_browser *browser, gpointer user_data) 
{
	GtkTreeIter iter;
	GtkTreeModel *model;
	GtkTreePath *path;
	gchar *d;
	if(!browser)
	{
		return;
	}

	d = g_strdup_printf("tag2-plugin:%s", browser->key);
	cfg_set_single_value_as_int(config, d, "pane-pos", gtk_paned_get_position(GTK_PANED(browser->tag2_vbox))); 
	g_free(d);

	/* remove it from the left hand view */
	model = gtk_tree_row_reference_get_model(browser->ref_iter);
	path = gtk_tree_row_reference_get_path(browser->ref_iter);
	if(path)
	{
		if(gtk_tree_model_get_iter(model,&iter, path))
		{
			gtk_tree_store_remove(GTK_TREE_STORE(model), &iter);
		}
		gtk_tree_path_free(path);
	}	
	gtk_tree_row_reference_free(browser->ref_iter);
	
	/* free browser */
	g_free(browser->name);
	g_free(browser->key);
	/* free the tag browsers */
	g_list_foreach(browser->tag_lists, (GFunc)tag2_destroy_tag, NULL);
	g_list_free(browser->tag_lists);
	/* destroy the container */
	if(browser->tag2_vbox) {
		//gtk_widget_destroy(browser->tag2_vbox);
		g_object_unref(browser->tag2_vbox);
	}
	/* clear structure */
	g_free(browser);
}

static void tag2_songlist_add_tag(tag_browser *browser,const gchar *name, int type)
{
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	tag_element *te = g_malloc0(sizeof(*te));

	browser->tag_lists = g_list_append(browser->tag_lists, te);
	/* Tag Element */
	te->type 	= type;
	te->index 	= g_list_length(browser->tag_lists);
	te->model 	= (GtkTreeModel *) gmpc_mpddata_model_new();
	te->sw 		= gtk_scrolled_window_new(NULL,NULL);
	te->tree 	= gtk_tree_view_new_with_model(GTK_TREE_MODEL(te->model));	
	te->browser = browser;

	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(te->sw), GTK_SHADOW_ETCHED_IN);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(te->sw), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	
	gtk_container_add(GTK_CONTAINER(te->sw), te->tree);

	/* Add the column, and set it up */
	column = gtk_tree_view_column_new();
	
	gtk_tree_view_column_set_title(column, name);

	renderer = gtk_cell_renderer_pixbuf_new();

	gtk_tree_view_column_pack_start(column, renderer, FALSE);
    if(te->type == MPD_TAG_ITEM_ARTIST ||te->type == MPD_TAG_ITEM_ALBUM)
    {
        gtk_tree_view_set_fixed_height_mode(GTK_TREE_VIEW(te->tree), TRUE);

	    gtk_tree_view_column_add_attribute(column, renderer, "pixbuf", MPDDATA_MODEL_META_DATA);
        gtk_cell_renderer_set_fixed_size(renderer, 64,64);
        gtk_tree_view_column_set_sizing(column , GTK_TREE_VIEW_COLUMN_FIXED);
    }
    else
    {
	    gtk_tree_view_column_add_attribute(column, renderer, "icon-name", MPDDATA_MODEL_COL_ICON_ID);
        gtk_tree_view_column_set_sizing(column , GTK_TREE_VIEW_COLUMN_AUTOSIZE);
    }
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	gtk_tree_view_column_add_attribute(column, renderer, "text", MPDDATA_MODEL_COL_SONG_TITLE);
	gtk_tree_view_insert_column(GTK_TREE_VIEW(te->tree),column, -1);//, name,renderer, "text", MPDDATA_MODEL_COL_SONG_TITLE, NULL);
	/* set the search column */
	gtk_tree_view_set_search_column(GTK_TREE_VIEW(te->tree), MPDDATA_MODEL_COL_SONG_TITLE);

	gtk_box_pack_start(GTK_BOX(browser->tag_hbox), te->sw, TRUE, TRUE, 0);
	g_signal_connect(G_OBJECT(te->tree), "button-press-event", G_CALLBACK(tag2_song_list_button_press_event), te);
	g_signal_connect(G_OBJECT(te->tree), "button-release-event", G_CALLBACK(tag2_browser_button_release_event), te);


	
	/* Signal */
	g_signal_connect(G_OBJECT(gtk_tree_view_get_selection(GTK_TREE_VIEW(te->tree))), "changed", G_CALLBACK(tag2_changed), te);

}

static void tag2_create_tags(tag_browser *browser)
{
	gchar *str = cfg_get_single_value_as_string_with_default(config, "tag2-browsers",browser->key, "genre|artist|album");
	if(str)
	{
		char **strv = g_strsplit(str, "|",0);
		int i;
		for(i=0;strv && strv[i];i++)
		{
			int tag = mpd_misc_get_tag_by_name(strv[i]);
			if(tag>=0)
			{
				/* This needs to be done based on config */
				tag2_songlist_add_tag(browser,strv[i],tag);
			}
		}
		g_strfreev(strv);
		g_free(str);
	}
	gtk_widget_show_all(browser->tag2_vbox);
}
/**
 * Add all selected songs 
 */

static void tag2_songlist_add_selected_songs(GtkWidget *bug, tag_browser *browser)
{
	GtkTreeSelection *sel = gtk_tree_view_get_selection(browser->tag_songlist);
	GtkTreeModel *model = gtk_tree_view_get_model(browser->tag_songlist);
	/* get a list of selected paths */
	GList *list = gtk_tree_selection_get_selected_rows(sel, &model);
	if(list)
	{
		GtkTreeIter piter;
		GList *iter = g_list_first(list);
		/* iterate over all the selected rows */
		for(;iter;iter = g_list_next(iter))
		{
			/* get iter from path */
			if(gtk_tree_model_get_iter(model, &piter, iter->data))
			{
				gchar *path;
				gtk_tree_model_get(model, &piter, MPDDATA_MODEL_COL_PATH, &path, -1);
				mpd_playlist_queue_add(connection, path);
				g_free(path);
			}			
		}
		mpd_playlist_queue_commit(connection);
		/* cleanup */
		g_list_foreach(list, (GFunc)gtk_tree_path_free, NULL);
		g_list_free(list);

	}
}
/**
 * Replace with selected songs
 */
static void tag2_songlist_replace_selected_songs(GtkWidget *bug, tag_browser *browser)
{
	GtkTreeSelection *sel = gtk_tree_view_get_selection(browser->tag_songlist);
	/* if no song is selected don't do anything */
	if(gtk_tree_selection_count_selected_rows(sel) > 0)
	{
		/* clear the current playlist */
		mpd_playlist_clear(connection);
		/* Add selected songs */
		tag2_songlist_add_selected_songs(bug, browser);
		/* start playing, as is standard in gmpc */
		mpd_player_play(connection);
	}
}

/**
 * Calls the column editing function of the GmpcTreeview
 */
static void tag2_song_list_edit_columns(GtkMenuItem *item, tag_browser *browser)
{
	gmpc_mpddata_treeview_edit_columns(GMPC_MPDDATA_TREEVIEW(browser->tag_songlist));
}


/**
 * Handles right mouse release on song list
 */
static gboolean tag2_song_list_button_release_event(GtkTreeView *tree, 
		GdkEventButton *event, 
		tag_browser *browser)
{
	/* only on right mouse click */
	if(event->button == 3)
	{
		GtkWidget *menu, *item;
		GtkTreeSelection *selection = gtk_tree_view_get_selection(tree);
		int count = gtk_tree_selection_count_selected_rows(selection);
		
		/* create menu to popup */
		menu = gtk_menu_new();
		/* only show when soething is selected */
		if(count >0)
		{
			/* add the add widget */
			item = gtk_image_menu_item_new_from_stock(GTK_STOCK_ADD,NULL);
			gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
			g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(tag2_songlist_add_selected_songs), browser);
			/* add the replace widget */
			item = gtk_image_menu_item_new_with_label("Replace");
			gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
					gtk_image_new_from_stock(GTK_STOCK_REDO, GTK_ICON_SIZE_MENU));
			gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
			g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(tag2_songlist_replace_selected_songs), browser);

			/* Separator */	
			item = gtk_separator_menu_item_new();
			gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
		}
		/* Edit columns */
		item = gtk_image_menu_item_new_with_label(("Edit Columns"));
		gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
				gtk_image_new_from_stock(GTK_STOCK_EDIT, GTK_ICON_SIZE_MENU));
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
		g_signal_connect(G_OBJECT(item), "activate",
				G_CALLBACK(tag2_song_list_edit_columns), browser);


		/* popup */
		gtk_widget_show_all(GTK_WIDGET(menu));
		gtk_menu_popup(GTK_MENU(menu), NULL, NULL,NULL, NULL, 0, event->time);
		return TRUE;

	}
	return FALSE;
}
/**
 * Handles a double click/activate on a row of the song list
 */
static void tag2_row_activate(GtkTreeView *tree, 
			GtkTreePath *path,
			GtkTreeViewColumn *column,
			tag_browser *browser)
{
	GtkTreeIter iter;
	if(gtk_tree_model_get_iter(gtk_tree_view_get_model(tree), &iter, path))
	{
		gchar *path;
		gtk_tree_model_get(gtk_tree_view_get_model(tree), &iter, MPDDATA_MODEL_COL_PATH, &path, -1);
		if(path)
		{
			/* gcc complains that this function is declared implicit.
			 * This is correct, but it exists in the gmpc program space
			 */
			play_path(path);
			/* free */
			g_free(path);
		}
	}
}

static void tag2_init(void)
{
	/**
	 * if first time used, add 2 example browsers 
	 */
	if(cfg_get_single_value_as_int_with_default(config, "tag2-plugin", "first-use", 1))
	{
		cfg_set_single_value_as_int(config,"tag2-plugin", "first-use",0);

		cfg_set_single_value_as_string(config, "tag2-browsers", "Artist Browser", "artist|album|disc");
		cfg_set_single_value_as_string(config, "tag2-plugin:Artist Browser", "name", "Artist Browser");
		cfg_set_single_value_as_string(config, "tag2-browsers", "Genre Browser", "genre|artist|album|disc");
		cfg_set_single_value_as_string(config, "tag2-plugin:Genre Browser", "name", "Genre Browser");
	}
}

static void tag2_init_browser(tag_browser *browser) {
	gchar *key;
	GtkWidget *sw;
	GmpcMpdDataModel *model = NULL;
	int pp;

	/* create the pane that separates the song list from the browsers */
	key = g_strdup_printf("tag2-plugin:%s", browser->key);
	pp = cfg_get_single_value_as_int_with_default(config, key, "pane-pos",150); 
	browser->tag2_vbox = gtk_vpaned_new(); 
	/* set the previous pane position */
	gtk_paned_set_position(GTK_PANED(browser->tag2_vbox), pp);


	/* box with tag treeviews (browsers) */
	browser->tag_hbox = gtk_hbox_new(TRUE, 6);

	/* Add this to the 1st pane*/
	gtk_paned_add1(GTK_PANED(browser->tag2_vbox), browser->tag_hbox);


	/** Create Songs list view */
	/* create the treeview model, this is a GmpcMpdData model */
	model = gmpc_mpddata_model_new();
	/* create scrolled window to make the treeview scrollable */
	sw = gtk_scrolled_window_new(NULL,NULL);
	/* setup the scrolled window */
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw), GTK_SHADOW_ETCHED_IN);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	/* create the actual treeview, use the GmpcTreeview type, so all the handling is done automatic */

	browser->tag_songlist = (GtkTreeView *)gmpc_mpddata_treeview_new(key,TRUE,GTK_TREE_MODEL(model));
	g_free(key);
	/* add the treeview to the scrolled window */
	gtk_container_add(GTK_CONTAINER(sw), GTK_WIDGET(browser->tag_songlist));
	/* add the scrolled window to the 2nd pane */
	gtk_paned_add2(GTK_PANED(browser->tag2_vbox),sw);
	/* connect some of the signals */
	g_signal_connect(G_OBJECT(browser->tag_songlist), "row-activated", G_CALLBACK(tag2_row_activate), browser);
	g_signal_connect(G_OBJECT(browser->tag_songlist), "button-press-event", G_CALLBACK(tag2_song_list_button_press_event), browser);
	g_signal_connect(G_OBJECT(browser->tag_songlist), "button-release-event", G_CALLBACK(tag2_song_list_button_release_event), browser);
	/* Create an extra reference to the paned window containing everything, this way
	 * I can add/remove it from gmpc's container withouth it being destroyed by gtk
	 */
	g_object_ref(browser->tag2_vbox);
	/* show everything */
	gtk_widget_show_all(browser->tag2_vbox);

	tag2_connection_changed_foreach(browser, NULL);
}
static gboolean tag2_custom_find(tag_browser *a  ,gchar *key)
{
	return strcmp(a->key, key);
}


static void tag2_browser_selected(GtkWidget *container)
{
	GtkTreeView *tree = playlist3_get_category_tree_view();
	GtkTreeModel *model = gtk_tree_view_get_model(tree);
	GtkTreeSelection *sel = gtk_tree_view_get_selection(tree);
	GtkTreeIter iter;
	if(gtk_tree_selection_get_selected(sel, &model, &iter))
	{
		gchar *key;
		gtk_tree_model_get(model, &iter, PL3_CAT_INT_ID, &key, -1);
		if(key)
		{
			GList *node = g_list_find_custom(tag2_ht, key, (GCompareFunc)tag2_custom_find);
				if(node)
				{
					tag_browser *tb = node->data;//g_hash_table_lookup(tag2_ht, key);
					if(tb)
					{
						if(tb->tag2_vbox ==NULL)
							tag2_init_browser(tb);
						gtk_container_add(GTK_CONTAINER(container), tb->tag2_vbox);
						gtk_widget_show_all(container);
						tag2_current = tb->tag2_vbox;
					}
					else{

					}
					g_free(key);
				}
		}
	}
}

static void tag2_browser_unselected(GtkWidget *container)
{
	if(tag2_current)
		gtk_container_remove(GTK_CONTAINER(container), tag2_current);
	tag2_current = NULL;
}

static void tag2_clear(tag_browser *browser)
{
	if(browser->tag2_vbox)
	{
		GtkTreeModel *model  = gtk_tree_view_get_model(GTK_TREE_VIEW(browser->tag_songlist));
		gmpc_mpddata_model_set_mpd_data(GMPC_MPDDATA_MODEL(model), NULL);
		g_list_foreach(browser->tag_lists, (GFunc)tag2_destroy_tag, NULL);
		g_list_free(browser->tag_lists);
		browser->tag_lists = NULL;
	}
}
static void tag2_save_browser(tag_browser *browser)
{
	GString *str = g_string_new("");
	GList *list = browser->tag_lists;
	for(;list;list = g_list_next(list))
	{
		tag_element *te = list->data;
		str = g_string_append(str, mpdTagItemKeys[te->type]);
		if(list->next)
			str = g_string_append(str, "|");
	}

	cfg_set_single_value_as_string(config, "tag2-browsers",browser->key,str->str);
	g_string_free(str, TRUE);
}
static void tag2_connection_changed_foreach(tag_browser *browser, gpointer data)
{
	if(browser->tag2_vbox)
	{
		tag2_create_tags(browser);
		tag_element *te = g_list_nth_data(browser->tag_lists, 0);
		if(te != NULL && mpd_check_connected(connection) )
		{
			MpdData *data;
			mpd_database_search_field_start(connection, te->type);
			data = mpd_database_search_commit(connection);
			gmpc_mpddata_model_set_mpd_data(GMPC_MPDDATA_MODEL(te->model), data);
		}
	}
}
static void tag2_connection_changed(MpdObj *mi, int connect, gpointer data)
{
	if(tag2_ht)
		//g_hash_table_foreach(tag2_ht,(GHFunc)tag2_clear, NULL);
		g_list_foreach(tag2_ht,(GFunc)tag2_clear, NULL);
	/*tag2_clear();*/
	if(connect && tag2_ht)
	{
		/* create tags */
//		g_hash_table_foreach(tag2_ht,(GHFunc)tag2_connection_changed_foreach, NULL);
		g_list_foreach(tag2_ht,(GFunc)tag2_connection_changed_foreach, NULL);

	}

}

static void tag2_browser_selection_changed(GtkWidget *tree, GtkTreeIter *iter)
{
	if(tag2_current)
	{
		GtkWidget *container = gtk_widget_get_parent(tag2_current);
		tag2_browser_unselected(container);
		tag2_browser_selected(container);
	}
}
/**
 * Preferences window
 */
void tag2_pref_destroy(GtkWidget *container)
{
	gtk_container_remove(GTK_CONTAINER(container), pref_vbox);
}
/**
 * Browser selector changed
 */
static void tag2_pref_combo_changed(GtkComboBox *box, GtkTreeModel *model2)
{
	GtkTreeIter iter;
	/* clear old data */
	gtk_list_store_clear(GTK_LIST_STORE(model2));

	if(gtk_combo_box_get_active_iter(box, &iter))
	{	
		GtkTreeModel *model = gtk_combo_box_get_model(box);
		gchar *group;
		gchar *str;
		tag_browser *tb;
		/* get the active tag browser */
		gtk_tree_model_get(model, &iter, 2,&tb,-1);
		/* iterate over all the tag elements and add them to the list */
		if(tb->tag_lists)
		{
			GList *liter = g_list_first(tb->tag_lists);
			for(;liter;liter = g_list_next(liter))
			{
				GtkTreeIter titer;
				tag_element *te = liter->data;
				gtk_list_store_append(GTK_LIST_STORE(model2),&titer);
				gtk_list_store_set(GTK_LIST_STORE(model2), &titer, 0,mpdTagItemKeys[te->type],1,te->type,2,te,-1);

			}
		}
		/* Get browser name and set entry */
		group = g_strdup_printf("tag2-plugin:%s", tb->key);
		str = cfg_get_single_value_as_string_with_default(config,group,"name", "default"); 
		gtk_entry_set_text(GTK_ENTRY(pref_entry), str);
		g_free(str);
		g_free(group);
	}
	else
	{
		/* only clear the text when nothing is selected, else the currently selected is cleared.. 
		 * yeah, longlive signals
		 */
		gtk_entry_set_text(GTK_ENTRY(pref_entry),  "");
	}
}

static void tag2_pref_add_browser_clicked(GtkWidget *but, GtkComboBox *combo)
{
	GtkTreeView *pl3_tree = playlist3_get_category_tree_view();
	GtkTreeIter titer;
	GtkListStore *model = (GtkListStore *)gtk_combo_box_get_model(combo);
	gchar *name = g_strdup_printf("%u", g_random_int());
	cfg_set_single_value_as_string(config, "tag2-browsers",name, "");
	
	

	tag2_browser_add_browser(GTK_WIDGET(pl3_tree),name);

	//tag_browser *tb = g_hash_table_lookup(tag2_ht, name);
	GList *node = g_list_find_custom(tag2_ht, name, (GCompareFunc)tag2_custom_find);
	if(node)
	{
		tag_browser *tb = node->data;
		gtk_list_store_append(model, &titer);
		gtk_list_store_set(model, &titer, 0,name, 1,"default",2,tb,-1);
		gtk_combo_box_set_active_iter(combo, &titer);
		/* change this to store TB in list store) */
	}
	g_free(name);

}
/**
 * Update the title of the browser
 */
static void tag2_pref_entry_changed(GtkWidget *entry, GtkComboBox *combo)
{
	GtkTreeIter iter;
	GtkListStore *model = (GtkListStore *)gtk_combo_box_get_model(combo);
	if(gtk_combo_box_get_active_iter(combo, &iter))
	{
		gchar *group,*key;
		GtkTreePath *path;
		tag_browser *tb;
		const gchar *name = gtk_entry_get_text(GTK_ENTRY(pref_entry));
		gtk_tree_model_get(GTK_TREE_MODEL(model), &iter,0,&key, 2, &tb, -1);
		gtk_list_store_set(model, &iter,1,name,-1); 

		g_free(tb->name);
	   	tb->name	= g_strdup(name);
		if((path = gtk_tree_row_reference_get_path(tb->ref_iter)))
		{
			GtkTreeModel *model = gtk_tree_row_reference_get_model(tb->ref_iter);
			GtkTreeIter iter;

			gtk_tree_model_get_iter(model, &iter, path);	
			gtk_tree_store_set(GTK_TREE_STORE(model), &iter, PL3_CAT_TITLE, name, -1);
			gtk_tree_path_free(path);
		}

		group = g_strdup_printf("tag2-plugin:%s", key);
		cfg_set_single_value_as_string(config, group, "name", (char *)name);
		g_free(group);
	}
}
/**
 * Handles editing of the columns type
 */
static void tag2_pref_column_type_edited(GtkCellRendererText *text, gchar *path, char *new_data, GtkTreeModel *model)
{
	GtkTreeIter iter;	
	if(gtk_tree_model_get_iter_from_string(model, &iter, path))
	{
		int tag = mpd_misc_get_tag_by_name(new_data);
		tag_element *te;
		GList *list;
		gtk_list_store_set(GTK_LIST_STORE(model), &iter, 0, new_data,1,tag, -1);
		gtk_tree_model_get(model, &iter, 2, &te, -1);
		te->type = tag;
		gtk_tree_view_column_set_title(gtk_tree_view_get_column(GTK_TREE_VIEW(te->tree), 0), new_data);
		/* if the first is changed, refill the first.
		 * if any other is changed, make the edited refill by triggering changed signal on the previous.
		 */
		/* index starts at 1 */
		if(te->index != 1)
		{
			list = g_list_nth(te->browser->tag_lists,te->index -2);
			if(list)
			{
				tag_element *te2 = list->data;
				/* update the content */	
				tag2_changed(gtk_tree_view_get_selection(GTK_TREE_VIEW(te2->tree)),te2); 
			}
		}
		else
		{
			MpdData *data;
			list = g_list_first(te->browser->tag_lists);
			if(list)
			{
				tag_element *te2 = list->data;
				mpd_database_search_field_start(connection, te2->type);
				data = mpd_database_search_commit(connection);
				gmpc_mpddata_model_set_mpd_data(GMPC_MPDDATA_MODEL(te2->model), data);
			}
		}

		tag2_save_browser(te->browser);
	}
}
/**
 * Add's a column to the selected tag browser
 */
static void tag2_pref_browser_remove(GtkWidget *but, GtkComboBox *box)
{
	GtkTreeIter iter;
	if(gtk_combo_box_get_active_iter(box, &iter))
	{	
		GtkTreeModel *model = gtk_combo_box_get_model(box);
		gchar *name;
		gchar *key;
		tag_browser *tb;
		gtk_tree_model_get(model, &iter, 2,&tb,-1);
		gtk_list_store_remove(GTK_LIST_STORE(model), &iter);

		key = g_strdup(tb->key);
		/* remove from browser list */
//		g_hash_table_remove(tag2_ht, tb->key);
		tag2_ht = g_list_remove(tag2_ht, tb);
		/* destroy remaining */
		tag2_destroy_browser(tb,NULL);	


		/* TODO delete */
		cfg_del_single_value(config, "tag2-browsers", key);

		name = g_strdup_printf("tag2-plugin:%s", key);
		cfg_remove_class(config, name);
		g_free(name);
		name = g_strdup_printf("tag2-plugin:%s-colsize", key);
		cfg_remove_class(config, name);
		g_free(name);
		name = g_strdup_printf("tag2-plugin:%s-colpos", key);
		cfg_remove_class(config, name);
		g_free(name);
		name = g_strdup_printf("tag2-plugin:%s-colshow", key);
		cfg_remove_class(config, name);
		g_free(name);
		g_free(key);
		pl3_update_go_menu();	
	}
}
/**
 * Add's a column to the selected tag browser
 */
static void tag2_pref_column_add(GtkWidget *but, GtkComboBox *box)
{
	GtkTreeIter iter;
	GtkTreeModel *model2 = g_object_get_data(G_OBJECT(but),"model"); 
	if(gtk_combo_box_get_active_iter(box, &iter))
	{	
		GtkTreeModel *model = gtk_combo_box_get_model(box);
		tag_browser *tb;
		/* TODO: Change to the browser tag editing stuff */
		gtk_tree_model_get(model, &iter, 2,&tb,-1);
		tag2_songlist_add_tag(tb,mpdTagItemKeys[MPD_TAG_ITEM_ARTIST], MPD_TAG_ITEM_ARTIST); 
		tag2_pref_combo_changed(box, model2);
		gtk_widget_show_all(tb->tag_hbox);
		tag2_save_browser(tb);
		/* if it's the first, update */
		if(g_list_length(tb->tag_lists) == 1)
		{
			GList *giter = g_list_first(tb->tag_lists);
			if(giter)
			{
				MpdData *data;
				tag_element *te2 = giter->data;
				/* update the content */	
				mpd_database_search_field_start(connection, te2->type);
				data = mpd_database_search_commit(connection);                        			
				gmpc_mpddata_model_set_mpd_data(GMPC_MPDDATA_MODEL(te2->model), data);	
			}
		}
	}
}
/**
 * Removes the selected column from the tag browser
 */
static void tag2_pref_column_remove(GtkWidget *but, GtkComboBox *box)
{
	GtkTreeIter iter;
	GtkTreeView *tree= g_object_get_data(G_OBJECT(but),"tree"); 
	if(gtk_combo_box_get_active_iter(box, &iter))
	{	
		GtkTreeModel *model = gtk_combo_box_get_model(box);
		GtkTreeSelection *sel = gtk_tree_view_get_selection(tree);
		tag_browser *tb;
		/* get the selected tag browser */
		gtk_tree_model_get(model, &iter, 2,&tb,-1);
		model = gtk_tree_view_get_model(tree);

		/* Get the selected column */
		if(gtk_tree_selection_get_selected(sel, &model,&iter))
		{	
			tag_element *te;
			GList *giter;
			int i;
			/* get tag element */
			gtk_tree_model_get(model, &iter,2, &te, -1); 
			/* remove from pref editor */
			gtk_list_store_remove(GTK_LIST_STORE(model), &iter);
			/* remove from browsers list */	
			tb->tag_lists = g_list_remove(tb->tag_lists, te);

			/**
			 *  renumber, this is needed to keep the filling consistent
			 *  Beware, the numbering starts at 1. not 0. (yes shoot me for this)
			 */
			i=1;
			for(giter = g_list_first(tb->tag_lists);giter;giter = g_list_next(giter))
			{
				tag_element *te = giter->data;
				te->index = i;
				i++;
			}

			/* destroy from interface */
			tag2_destroy_tag(te);
			/**
			 * Refill the complete browser.
			 * TODO: Only refill the part that is needed 
			 */
			giter = g_list_first(te->browser->tag_lists);
			if(giter)
			{
				MpdData *data;
				tag_element *te2 = giter->data;
				/* update the content */	
				mpd_database_search_field_start(connection, te2->type);
				data = mpd_database_search_commit(connection);
				gmpc_mpddata_model_set_mpd_data(GMPC_MPDDATA_MODEL(te2->model), data);

			}


			/* save the browsers content */
			tag2_save_browser(tb);
		}
	}
}


void tag2_pref_construct(GtkWidget *container)
{
	int i;
	GtkTreeIter titer;
	GtkWidget *sw, *tree, *but;	
	GtkCellRenderer *renderer;
	GtkTreeModel *model,*model2;
	GtkWidget *hbox,*label,*vbox;
	GtkWidget *combo = NULL;
	conf_mult_obj *cmo,*iter;
	/**
	 * Create the parent widget where the preferences window is packed in 
	 */
	pref_vbox = gtk_vbox_new(FALSE,6);

	/* select browser to edit */
	hbox = gtk_hbox_new(FALSE, 6);
	model = (GtkTreeModel *)gtk_list_store_new(3, G_TYPE_STRING,G_TYPE_STRING, G_TYPE_POINTER); 
	combo = gtk_combo_box_new_with_model(model);
	renderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combo), renderer, TRUE);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(combo), renderer, "text", 1,NULL);

	gtk_box_pack_start(GTK_BOX(hbox), combo, TRUE, TRUE, 0);

	but = gtk_button_new_from_stock(GTK_STOCK_ADD);
	gtk_box_pack_start(GTK_BOX(hbox),but, FALSE, TRUE, 0);
	g_signal_connect(G_OBJECT(but), "clicked", G_CALLBACK(tag2_pref_add_browser_clicked), combo);
	but = gtk_button_new_from_stock(GTK_STOCK_REMOVE);
	g_signal_connect(G_OBJECT(but), "clicked", G_CALLBACK(tag2_pref_browser_remove), combo);
	gtk_box_pack_start(GTK_BOX(hbox), but, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(pref_vbox), hbox, FALSE, TRUE, 0);


	hbox = gtk_hbox_new(FALSE, 6);
	label = gtk_label_new("Name:");
	gtk_misc_set_alignment(GTK_MISC(label), 1,0.5);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 0);

	pref_entry = gtk_entry_new();
	g_signal_connect(GTK_ENTRY(pref_entry),"changed", G_CALLBACK(tag2_pref_entry_changed), combo);
	gtk_box_pack_start(GTK_BOX(hbox), pref_entry,TRUE, TRUE, 0);

	gtk_box_pack_start(GTK_BOX(pref_vbox), hbox, FALSE, TRUE, 0);




	/* change this to store TB in list store) */
	cmo = cfg_get_key_list(config, "tag2-browsers");
	for(iter=cmo;iter;iter = iter->next)
	{
		GtkTreeIter titer;
		gchar *group = g_strdup_printf("tag2-plugin:%s", iter->key);
		gchar *name = cfg_get_single_value_as_string_with_default(config, group, "name", "default");
		GList *node = g_list_find_custom(tag2_ht, iter->key, (GCompareFunc)tag2_custom_find);
		if(node)
		{
			tag_browser *tb = node->data;//g_hash_table_lookup(tag2_ht, iter->key);
			/* add to list */
			gtk_list_store_append(GTK_LIST_STORE(model), &titer);
			gtk_list_store_set(GTK_LIST_STORE(model), &titer, 0, iter->key,1,name,2,tb,-1);
		}
		/* cleanup */
		g_free(group);
	
		g_free(name);
	}
	if(cmo)
		cfg_free_multiple(cmo);



	/* scrolled window used to pack the treeview */

	hbox = gtk_hbox_new(FALSE, 6);
	sw = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw),GTK_SHADOW_ETCHED_IN);
	/* the treeview */
	tree = gtk_tree_view_new();
	/* the model for the treeview */
	model = (GtkTreeModel *) gtk_list_store_new(3, G_TYPE_STRING,G_TYPE_INT, G_TYPE_POINTER);	
	gtk_tree_view_set_model(GTK_TREE_VIEW(tree), model);

	renderer = gtk_cell_renderer_combo_new();
	g_object_set(G_OBJECT(renderer), "editable", TRUE,NULL);
	model2 = (GtkTreeModel *)gtk_list_store_new(1, G_TYPE_STRING);
	for(i=0;i<MPD_TAG_ITEM_ANY;i++)
	{
		gtk_list_store_append(GTK_LIST_STORE(model2), &titer);
		gtk_list_store_set(GTK_LIST_STORE(model2), &titer, 0, mpdTagItemKeys[i],-1);
	}	
	g_object_set(G_OBJECT(renderer), "model", model2,"text-column", 0,"has-entry", FALSE,NULL);
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree), -1, "text",renderer,"text",0,NULL);
	g_signal_connect(G_OBJECT(renderer), "edited", G_CALLBACK(tag2_pref_column_type_edited),model);

	g_signal_connect(G_OBJECT(combo), "changed", G_CALLBACK(tag2_pref_combo_changed), model);
	/* tree to scrolled window */
	gtk_container_add(GTK_CONTAINER(sw), tree);

	/* vbox */
	vbox = gtk_vbox_new(FALSE,6);

	but = gtk_button_new_from_stock(GTK_STOCK_ADD);
	gtk_box_pack_start(GTK_BOX(vbox), but, FALSE, TRUE, 0);
	g_object_set_data(G_OBJECT(but), "model", model);
	g_signal_connect(G_OBJECT(but), "clicked", G_CALLBACK(tag2_pref_column_add), combo);
	but = gtk_button_new_from_stock(GTK_STOCK_REMOVE);
	g_object_set_data(G_OBJECT(but), "tree", tree);
	g_signal_connect(G_OBJECT(but), "clicked", G_CALLBACK(tag2_pref_column_remove), combo);


	gtk_box_pack_start(GTK_BOX(vbox), but, FALSE, TRUE, 0);

	gtk_box_pack_start(GTK_BOX(hbox), sw, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(pref_vbox), hbox, TRUE, TRUE, 0);

	gtk_container_add(GTK_CONTAINER(container), pref_vbox);
	gtk_widget_show_all(container);
}
static void tag2_browser_activate(GtkWidget *item, tag_browser *browser)
{
	GtkTreeSelection *selec = gtk_tree_view_get_selection((GtkTreeView *)
			playlist3_get_category_tree_view());	

	GtkTreePath *path = gtk_tree_row_reference_get_path(browser->ref_iter); 
	if(path)
	{
		gtk_tree_selection_select_path(selec, path);
		gtk_tree_path_free(path);
	}



}
static void tag2_browser_add_go_menu_foreach(tag_browser *browser, GtkWidget *menu)
{
	GtkWidget *item = gtk_image_menu_item_new_with_label(browser->name);
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item), 
			gtk_image_new_from_icon_name("media-tag", GTK_ICON_SIZE_MENU));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

	gtk_widget_add_accelerator(GTK_WIDGET(item), "activate", gtk_menu_get_accel_group(GTK_MENU(menu)), GDK_F1 +counter,GDK_SHIFT_MASK , GTK_ACCEL_VISIBLE);
	counter++;
	g_signal_connect(G_OBJECT(item), "activate", 
			G_CALLBACK(tag2_browser_activate), browser);
}

static int tag2_browser_add_go_menu(GtkWidget *menu)
{
	if(tag2_get_enabled() == FALSE)
		return 0;
	if(tag2_ht)
	{
		counter = 0;
	//	g_hash_table_foreach(tag2_ht,(GHFunc)tag2_browser_add_go_menu_foreach, menu);
		g_list_foreach(tag2_ht,(GFunc)tag2_browser_add_go_menu_foreach, menu);
	return 1;
	}

	return 0;
}

static void tag2_save_myself(void)
{
	if(tag2_ht)
	{
		GList *iter = g_list_first(tag2_ht);
		while(iter){
			tag_browser *tb = iter->data;
			GtkTreePath *path = gtk_tree_row_reference_get_path(tb->ref_iter);
			if(path)
			{
				gint *indices = gtk_tree_path_get_indices(path);
				gchar *group = g_strdup_printf("tag2-plugin:%s",tb->key);
				debug_printf(DEBUG_INFO,"Saving myself to position: %i\n", indices[0]);
				cfg_set_single_value_as_int(config, group,"position",indices[0]);
				gtk_tree_path_free(path);
				g_free(group);
			}
			iter = g_list_next(iter);
		}	
	}
}
