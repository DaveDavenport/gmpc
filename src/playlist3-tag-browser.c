#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <glade/glade.h>

#include "plugin.h"

#include "main.h"
#include "misc.h"
#include "playlist3.h"
#include "playlist3-tag-browser.h"

static void pref_id3b_fill(void);
static void pl3_custom_tag_browser_fill_tree(GtkWidget *,GtkTreeIter *);
static void pl3_tag_browser_selected(GtkWidget *container);
static void pl3_tag_browser_unselected(GtkWidget *container);
static void pl3_custom_tag_browser_add(void);
static int pl3_custom_tag_browser_right_mouse_menu(GtkWidget *menu, int type, GtkWidget *tree, GdkEventButton *event);
static long unsigned pl3_custom_tag_browser_view_folder(GtkTreeIter *iter_cat);
static void pl3_custom_tag_browser_category_selection_changed(GtkWidget *tree,GtkTreeIter *iter);

static void tag_connection(MpdObj *mi, int connect,gpointer user);

// Glade prototypes, would be static otherwise
void pref_id3b_row_remove(void);
void pref_id3b_row_changed(GtkTreeView *tree);
void pref_id3b_add_entry(void);

static void pref_id3b_fill(void);
static void pref_id3b_init(void);

/* Connection settings plugin */
static void tag_pref_construct(GtkWidget *container);
static void tag_pref_destroy(GtkWidget *container);


GtkTreeRowReference *pl3_tag_tree_ref = NULL;

static int pl3_custom_tag_add_go_menu(GtkWidget *menu);

GladeXML *tag_pref_xml = NULL;
gmpcPrefPlugin tag_gpp = {
	tag_pref_construct,
	tag_pref_destroy
};


gmpcPlBrowserPlugin tag_gbp = {
	pl3_custom_tag_browser_add,
	pl3_tag_browser_selected,
	pl3_tag_browser_unselected,
	pl3_custom_tag_browser_category_selection_changed,
	pl3_custom_tag_browser_fill_tree,
	pl3_custom_tag_browser_right_mouse_menu,
	NULL,
	pl3_custom_tag_add_go_menu
};

gmpcPlugin tag_plug = {
	"Tag Browser",
	{1,1,1},
	GMPC_PLUGIN_PL_BROWSER,
	0,
	NULL,			/* name*/
	NULL,			/* init */
	&tag_gbp,		/* Browser */
	NULL,			/* status changed */
	&tag_connection,	/* connection */
	&tag_gpp,		/* Preferences */
	NULL,			/* MetaData */
	NULL,
	NULL
};

static void pl3_custom_tag_browser_add_single(GtkTreeIter *piter, char *title, char *format);
static void pl3_custom_tag_browser_list_add(GtkTreeIter *iter);


static void pl3_custom_tag_browser_row_activated(GtkTreeView *tree, GtkTreePath *tp);
static void pl3_custom_tag_browser_add_folder(void);
static void pl3_custom_tag_browser_replace_folder(void);
static void pl3_tag_browser_add_selected(void);
static void pl3_tag_browser_replace_selected(void);
static void pl3_custom_tag_browser_button_release_event(GtkWidget *wid, GdkEventButton *event);
static void pl3_tag_browser_show_info(void);
static int pl3_tag_browser_playlist_key_press(GtkWidget *tree, GdkEventKey *event);

extern GladeXML *pl3_xml;


enum{
	PL3_TB_PATH,
	PL3_TB_TYPE,
	PL3_TB_TITLE,
	PL3_TB_ICON,
	PL3_TB_ROWS
};


/* internal */
GtkWidget *pl3_tb_tree = NULL;
GtkListStore *pl3_tb_store = NULL;
GtkWidget *pl3_tb_sw = NULL;


static int pl3_tag_browser_button_press_event(GtkTreeView *tree, GdkEventButton *event)
{
	GtkTreeSelection *sel = gtk_tree_view_get_selection(tree);
	if(event->button != 3 ||
		gtk_tree_selection_count_selected_rows(sel) < 2|| !mpd_check_connected(connection))
	{
		return FALSE;
	}
	return TRUE;
}


static void pl3_tag_browser_init()
{
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column = NULL;
	GValue value;
	pl3_tb_store = gtk_list_store_new (PL3_TB_ROWS, 
			G_TYPE_STRING, /* path to file */
			G_TYPE_INT,	/* type, FILE/PLAYLIST/FOLDER  */
			G_TYPE_STRING,	/* title to display */
			G_TYPE_STRING); /* icon type */



	renderer = gtk_cell_renderer_pixbuf_new ();

	column = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_column_pack_start (column, renderer, FALSE);
	gtk_tree_view_column_set_attributes (column,renderer,"stock-id", PL3_TB_ICON,NULL);
	memset(&value, 0, sizeof(value));
	/* set value for ALL */
	g_value_init(&value, G_TYPE_FLOAT);
	g_value_set_float(&value, 0.0);
	g_object_set_property(G_OBJECT(renderer), "yalign", &value); 

	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_set_attributes (column,renderer,"text", PL3_TB_TITLE, NULL);


	/* set up the tree */
	pl3_tb_tree= gtk_tree_view_new_with_model(GTK_TREE_MODEL(pl3_tb_store));
	/* insert the column in the tree */
	gtk_tree_view_append_column (GTK_TREE_VIEW (pl3_tb_tree), column);
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(pl3_tb_tree), FALSE);
	gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(pl3_tb_tree), TRUE);
	gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW(pl3_tb_tree)), GTK_SELECTION_MULTIPLE);

	/* setup signals */
	g_signal_connect(G_OBJECT(pl3_tb_tree), "row-activated",
			G_CALLBACK(pl3_custom_tag_browser_row_activated), NULL); 
	g_signal_connect(G_OBJECT(pl3_tb_tree), "button-press-event",
			G_CALLBACK(pl3_tag_browser_button_press_event), NULL);
	g_signal_connect(G_OBJECT(pl3_tb_tree), "button-release-event",
			G_CALLBACK(pl3_custom_tag_browser_button_release_event), NULL);
	g_signal_connect(G_OBJECT(pl3_tb_tree), "key-press-event",
			G_CALLBACK(pl3_tag_browser_playlist_key_press), NULL);

	/* set up the scrolled window */
	pl3_tb_sw = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(pl3_tb_sw), GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(pl3_tb_sw), GTK_SHADOW_ETCHED_IN);
	gtk_container_add(GTK_CONTAINER(pl3_tb_sw), pl3_tb_tree);

	/* set initial state */
	g_object_ref(G_OBJECT(pl3_tb_sw));
}


static void pl3_custom_tag_browser_reload()
{
	GtkTreeIter iter;
	if(pl3_tree == NULL )
	{
		return;
	}

	if(gtk_tree_model_get_iter_first(GTK_TREE_MODEL(pl3_tree),&iter))
	{
		gchar *title = NULL;
		do{
			gtk_tree_model_get(GTK_TREE_MODEL(pl3_tree), &iter, PL3_CAT_TITLE, &title, -1);
			if(!strcmp(title, _("Tag Browser")))
			{
				GtkTreeIter child;
				if(gtk_tree_model_iter_children(GTK_TREE_MODEL(pl3_tree), &child, &iter))
				{
					do{
						gtk_tree_store_remove(pl3_tree, &child);

					}while(gtk_tree_store_iter_is_valid(pl3_tree, &child));
				}
				pl3_custom_tag_browser_list_add(&iter);
				return;
			}
			g_free(title);
		}while(gtk_tree_model_iter_next(GTK_TREE_MODEL(pl3_tree), &iter));
	}
}

static void pl3_custom_tag_browser_list_add(GtkTreeIter *iter)
{
	conf_mult_obj *list;
	/* get the configured tag browsers */
	list = cfg_get_multiple_as_string(config, "playlist", "advbrows");
	if(list != NULL)
	{
		conf_mult_obj *data = list;
		do{
			if(strlen(data->key) && strlen(data->value))
			{
				pl3_custom_tag_browser_add_single(iter,data->key, data->value);
			}
			data = data->next;
		}while(data != NULL);
		cfg_free_multiple(list);
	}

}
static void pl3_custom_tag_browser_add()
{
	GtkTreePath *path;
	if(mpd_server_check_version(connection,0,12,0))
	{
		GtkTreeIter iter;


		/* add the root node to tag-browser */
		gtk_tree_store_append(pl3_tree, &iter, NULL);
		gtk_tree_store_set(pl3_tree, &iter,
				PL3_CAT_TYPE, tag_plug.id,
				PL3_CAT_TITLE, _("Tag Browser"),
				PL3_CAT_INT_ID, "",
				PL3_CAT_ICON_ID, "media-artist",
				PL3_CAT_PROC, TRUE,
				PL3_CAT_ICON_SIZE,GTK_ICON_SIZE_DND,
				PL3_CAT_BROWSE_FORMAT, "",
				-1);
		pl3_custom_tag_browser_list_add(&iter);

		if(pl3_tag_tree_ref)
		{
			gtk_tree_row_reference_free(pl3_tag_tree_ref);
			pl3_tag_tree_ref = NULL;
		}
		path = gtk_tree_model_get_path(GTK_TREE_MODEL(pl3_tree), &iter);
		if(path)
		{
			pl3_tag_tree_ref = gtk_tree_row_reference_new(GTK_TREE_MODEL(pl3_tree),path);
			gtk_tree_path_free(path);
		}

	}

}


static void pl3_custom_tag_browser_add_single(GtkTreeIter *piter, char *title, char *format)
{
	GtkTreeIter iter,child;
	gtk_tree_store_append(pl3_tree, &iter, piter);
	gtk_tree_store_set(pl3_tree, &iter, 
			PL3_CAT_TYPE, tag_plug.id,
			PL3_CAT_TITLE, title,        	
			PL3_CAT_INT_ID, "",
			PL3_CAT_ICON_ID, "media-artist",
			PL3_CAT_PROC, FALSE,
			PL3_CAT_ICON_SIZE,GTK_ICON_SIZE_DND,
			PL3_CAT_BROWSE_FORMAT, format,
			-1);
	/* add fantom child for lazy tree */
	gtk_tree_store_append(pl3_tree, &child, &iter);
}


static void pl3_custom_tag_browser_fill_tree(GtkWidget *tree, GtkTreeIter *iter)
{
	char *first_tag, *second_tag;
	char *format;
	char **tk_format= NULL;
	GtkTreePath *path = NULL;
	int depth = 0;
	int len=0;
	GtkTreeIter child,child2;


	path = gtk_tree_model_get_path(GTK_TREE_MODEL(pl3_tree), iter);
	if(path == NULL)
	{
		return;
	}
	depth = gtk_tree_path_get_depth(path) -2;
	gtk_tree_path_free(path);	
	if (!mpd_check_connected(connection) || depth < 0)
	{
		return;
	}


	gtk_tree_model_get(GTK_TREE_MODEL(pl3_tree),iter, 1, &first_tag,2,&second_tag,PL3_CAT_BROWSE_FORMAT, &format, -1);
	gtk_tree_store_set(pl3_tree, iter, PL3_CAT_PROC, TRUE, -1);


	tk_format = g_strsplit(format, "|",0);
	if(tk_format ==NULL)
	{
		if(first_tag)g_free(first_tag);
		if(second_tag)g_free(second_tag);
		if(format)g_free(format);
		return;
	}
	for(len=0;tk_format[len] != NULL;len++)	
	{
		if(mpd_misc_get_tag_by_name(tk_format[len])== -1)
		{
			if(first_tag)g_free(first_tag);
			if(second_tag)g_free(second_tag);
			if(format)g_free(format);        			
			g_strfreev(tk_format);
			return;
		}
	}
	/* the things we do when on level 0 */
	if(depth == 0)
	{
		MpdData *data = mpd_database_get_unique_tags(connection,mpd_misc_get_tag_by_name(tk_format[0]),-1);

		while(data != NULL)
		{	
			gtk_tree_store_append (pl3_tree, &child, iter);
			gtk_tree_store_set (pl3_tree, &child,
					0, tag_plug.id,
					1, data->tag, /* the field */
					2, data->tag, /* the artist name, if(1 and 2 together its an artist field) */
					3, "media-album",
					4, FALSE,
					PL3_CAT_ICON_SIZE,1,
					PL3_CAT_BROWSE_FORMAT, format,
					-1);
			if(len > 1)
			{
				gtk_tree_store_append(pl3_tree, &child2, &child);
			}

			data = mpd_data_get_next(data);
		}
		if(gtk_tree_model_iter_children(GTK_TREE_MODEL(pl3_tree), &child, iter))
		{
			gtk_tree_store_remove(pl3_tree, &child); 
		}
	}
	/* if where inside a artist */
	else if(depth == 1)
	{
		MpdData *data = mpd_database_get_unique_tags(connection,mpd_misc_get_tag_by_name(tk_format[1]),
				mpd_misc_get_tag_by_name(tk_format[0]),first_tag,-1 );
		if(data == NULL)
		{
		}
		while(data != NULL){
			gtk_tree_store_append (pl3_tree, &child, iter);
			gtk_tree_store_set (pl3_tree, &child,
					0, tag_plug.id,
					1, data->tag,
					2, first_tag,
					3, "media-artist", 
					4, FALSE, 
					PL3_CAT_ICON_SIZE,1,
					PL3_CAT_BROWSE_FORMAT, format,
					-1);
			data = mpd_data_get_next(data);
			if(len > 2)
			{
				gtk_tree_store_append(pl3_tree, &child2, &child);			
			}                                                        		
		}
		if(gtk_tree_model_iter_children(GTK_TREE_MODEL(pl3_tree), &child, iter))
		{
			gtk_tree_store_remove(pl3_tree, &child); 
		}
	}
	else if(depth == 2 && len >= 2)
	{
		MpdData *data = mpd_database_get_unique_tags(connection,
				mpd_misc_get_tag_by_name(tk_format[2]),
				mpd_misc_get_tag_by_name(tk_format[1]),first_tag,
				mpd_misc_get_tag_by_name(tk_format[0]),second_tag,
				-1);
		if(data == NULL)
		{
			debug_printf(DEBUG_WARNING, "pl3_custom_tag_browser_fill_tree: no sub data %s %s %s %s %s\n",
					tk_format[2],
					tk_format[1],first_tag,
					tk_format[0],second_tag);
		}

		while(data != NULL){
			gtk_tree_store_append (pl3_tree, &child, iter);
			gtk_tree_store_set (pl3_tree, &child,
					0, tag_plug.id,
					1, data->tag,
					2, first_tag,
					3, "media-artist", 
					4, TRUE, 
					PL3_CAT_ICON_SIZE,1,
					PL3_CAT_BROWSE_FORMAT, format,
					-1);
			data = mpd_data_get_next(data);
		}
		if(gtk_tree_model_iter_children(GTK_TREE_MODEL(pl3_tree), &child, iter))
		{
			gtk_tree_store_remove(pl3_tree, &child); 
		}                                                                                       		
	}
	g_strfreev(tk_format);
	if(first_tag)g_free(first_tag);
	if(second_tag)g_free(second_tag);
	if(format)g_free(format);
}

static long unsigned pl3_custom_tag_browser_view_folder(GtkTreeIter *iter_cat)
{
	char *first_tag, *second_tag, *format;
	char **tk_format;
	int i = 0, depth;
	GtkTreePath *path;
	GtkTreeIter iter;
	long unsigned time =0;


	if (!mpd_check_connected(connection))
	{
		return 0;
	}
	gtk_tree_model_get(GTK_TREE_MODEL(pl3_tree), iter_cat, 2 , &first_tag, 1,&second_tag,PL3_CAT_BROWSE_FORMAT, &format, -1);
	path = gtk_tree_model_get_path(GTK_TREE_MODEL(pl3_tree), iter_cat);
	if(path == NULL)
	{
		if(first_tag)g_free(first_tag);
		if(second_tag)g_free(second_tag);
		if(format)g_free(format);


		return 0;
	}
	depth = gtk_tree_path_get_depth(path) -2;

	tk_format = g_strsplit(format, "|",0);
	if(tk_format ==NULL)
	{
		gtk_tree_path_free(path);	
		if(first_tag)g_free(first_tag);
		if(second_tag)g_free(second_tag);
		if(format)g_free(format);

		return 0;
	}


	for(i=0;tk_format[i] != NULL;i++)	
	{

		if(mpd_misc_get_tag_by_name(tk_format[i])== -1)
		{

			g_strfreev(tk_format);
			gtk_tree_path_free(path);	
			return 0;
		}

	}
	if(depth == -1)
	{
		conf_mult_obj *list;
		list = cfg_get_multiple_as_string(config, "playlist", "advbrows");
		if(list != NULL)
		{
			conf_mult_obj *data = list;
			do{
				if(strlen(data->key) && strlen(data->value))
				{
					gtk_list_store_append (pl3_tb_store, &iter);
					gtk_list_store_set (pl3_tb_store, &iter,
							PL3_TB_TITLE,data->key,
							PL3_TB_PATH, data->key,
							PL3_TB_TYPE, PL3_ENTRY_ALBUM,
							PL3_TB_ICON,"media-artist",
							-1);


				}
				data = data->next;
			}while(data != NULL);
			cfg_free_multiple(list);
		}
	}
	if(depth == 0)
	{
		MpdData *data = mpd_database_get_unique_tags(connection,mpd_misc_get_tag_by_name(tk_format[0]),-1);

		while(data != NULL)
		{	

			gtk_list_store_append (pl3_tb_store, &iter);
			gtk_list_store_set (pl3_tb_store, &iter,
					PL3_TB_TITLE,data->tag,
					PL3_TB_PATH, data->tag,
					PL3_TB_TYPE, PL3_ENTRY_ALBUM,
					PL3_TB_ICON,"media-album",
					-1);
			data = mpd_data_get_next(data);
		}
	}
	if(depth == 1)
	{
		MpdData *data = mpd_database_get_unique_tags(connection,
				mpd_misc_get_tag_by_name(tk_format[1]),
				mpd_misc_get_tag_by_name(tk_format[0]),first_tag,
				-1);
		while(data != NULL){
			gtk_list_store_append (pl3_tb_store, &iter);
			gtk_list_store_set (pl3_tb_store, &iter,
					PL3_TB_TITLE, data->tag,
					PL3_TB_PATH, data->tag,
					PL3_TB_TYPE, PL3_ENTRY_ALBUM,
					PL3_TB_ICON,"media-artist",
					-1);

			data = mpd_data_get_next(data);
		}




		data = mpd_database_find_adv(connection,TRUE, 
				mpd_misc_get_tag_by_name(tk_format[0]), first_tag, -1);
		/* lowest level selected*/
		while(data != NULL)
		{
			gchar buffer[1024];
			char *markdata = cfg_get_single_value_as_string_with_default(config, "playlist", "browser_markup",DEFAULT_MARKUP_BROWSER);
			mpd_song_markup(buffer, 1024,markdata,data->song);
			cfg_free_string(markdata);
			if(data->song->time != MPD_SONG_NO_TIME)
			{
				time += data->song->time;
			}
			if(data->song->file == NULL)
			{
				debug_printf(DEBUG_WARNING,"pl3_browser_view_folder: crap mpdSong has no file attribute.\n");
			}
			gtk_list_store_append (pl3_tb_store, &iter);
			gtk_list_store_set (pl3_tb_store, &iter,
					PL3_TB_TITLE, buffer,
					PL3_TB_PATH, data->song->file,
					PL3_TB_TYPE, PL3_ENTRY_SONG,
					PL3_TB_ICON,"media-audiofile",
					-1);
			data = mpd_data_get_next(data);
		}
	}
	else if(depth == 2)
	{
		MpdData *data = mpd_database_get_unique_tags(connection,
				mpd_misc_get_tag_by_name(tk_format[2]),
				mpd_misc_get_tag_by_name(tk_format[1]),second_tag,
				mpd_misc_get_tag_by_name(tk_format[0]),first_tag,
				-1);
		while(data != NULL){
			gtk_list_store_append (pl3_tb_store, &iter);
			gtk_list_store_set (pl3_tb_store, &iter,
					PL3_TB_TITLE, data->tag,
					PL3_TB_PATH, data->tag,
					PL3_TB_TYPE, PL3_ENTRY_ALBUM,
					PL3_TB_ICON,"media-album",
					-1);

			data = mpd_data_get_next(data);
		}



		/* second level */
		data = mpd_database_find_adv(connection,TRUE,
				mpd_misc_get_tag_by_name(tk_format[0]),first_tag,
				mpd_misc_get_tag_by_name(tk_format[1]), second_tag, -1);
		while (data != NULL)
		{
			gchar buffer[1024];
			char *markdata = cfg_get_single_value_as_string_with_default(config, "playlist", "browser_markup",DEFAULT_MARKUP_BROWSER);
			mpd_song_markup(buffer, 1024,markdata,data->song);
			cfg_free_string(markdata);
			if(data->song->time != MPD_SONG_NO_TIME)
			{
				time += data->song->time;
			}
			if(data->song->file == NULL)
			{
				debug_printf(DEBUG_WARNING,"pl3_browser_view_folder: crap mpdSong has no file attribute.\n");
			}
			gtk_list_store_append (pl3_tb_store, &iter);
			gtk_list_store_set (pl3_tb_store, &iter,
					PL3_TB_TITLE, buffer,
					PL3_TB_PATH, data->song->file,
					PL3_TB_TYPE, PL3_ENTRY_SONG,
					PL3_TB_ICON,"media-audiofile",
					-1);

			data = mpd_data_get_next(data);
		}

	}
	else if (depth == 3)
	{
		char *first;
		/* take the upper one */
		/* go 2 up */
		if(gtk_tree_path_up(path) && gtk_tree_path_up(path))
		{
			MpdData *data  = NULL;
			gtk_tree_model_get_iter(GTK_TREE_MODEL(pl3_tree), &iter, path);
			gtk_tree_model_get(GTK_TREE_MODEL(pl3_tree), &iter, 1, &first, -1);

			/* artist and album is selected */
			data = mpd_database_find_adv(connection,TRUE,
					mpd_misc_get_tag_by_name(tk_format[0]),	first,
					mpd_misc_get_tag_by_name(tk_format[1]), first_tag,
					mpd_misc_get_tag_by_name(tk_format[2]), second_tag,-1);
			while (data != NULL)
			{
				gchar buffer[1024];
				char *markdata = cfg_get_single_value_as_string_with_default(config, "playlist", "browser_markup",
						DEFAULT_MARKUP_BROWSER);
				mpd_song_markup(buffer, 1024,markdata,data->song);
				cfg_free_string(markdata);
				if(data->song->time != MPD_SONG_NO_TIME)
				{
					time += data->song->time;
				}
				if(data->song->file == NULL)
				{
					debug_printf(DEBUG_WARNING,"pl3_browser_view_folder: crap mpdSong has no file attribute.\n");
				}
				gtk_list_store_append (pl3_tb_store, &iter);
				gtk_list_store_set (pl3_tb_store, &iter,
						PL3_TB_TITLE, buffer,
						PL3_TB_PATH, data->song->file,
						PL3_TB_TYPE, PL3_ENTRY_SONG,
						PL3_TB_ICON,"media-audiofile",
						-1);

				data = mpd_data_get_next(data);
			}
			g_free(first);
		}
	}
	gtk_tree_path_free(path);	
	g_strfreev(tk_format);
	if(first_tag)g_free(first_tag);
	if(second_tag)g_free(second_tag);
	if(format)g_free(format);

	return time;
}


static int  pl3_custom_tag_browser_right_mouse_menu(GtkWidget *menu, int type, GtkWidget *tree, GdkEventButton *event)
{
	/* we need an model and a iter */
	if(type == tag_plug.id)
	{
		GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(tree));
		GtkTreeIter iter;
		int depth = 0;
		GtkTreePath *path;	
		/* se need a GtkTreeSelection to know what is selected */
		GtkTreeSelection *selection = gtk_tree_view_get_selection((GtkTreeView *)glade_xml_get_widget (pl3_xml, "cat_tree"));

		/* get and check for selected */
		if(!gtk_tree_selection_get_selected(selection,&model, &iter))
		{
			/* Nothin selected? then we don't have todo anything */
			return 0;
		}
		/* now we want to know what level we are, and what we need to show */
		path = gtk_tree_model_get_path(GTK_TREE_MODEL(pl3_tree), &iter);
		if(path == NULL)
		{
			debug_printf(DEBUG_INFO,"Failed to get path\n");
			return 0;
		}
		depth = gtk_tree_path_get_depth(path);

		gtk_tree_path_free(path);	
		if(depth > 2)
		{
			/* here we have:  Add. Replace*/
			GtkWidget *item;
			/* add the add widget */
			item = gtk_image_menu_item_new_from_stock(GTK_STOCK_ADD,NULL);
			gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
			g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(pl3_custom_tag_browser_add_folder), NULL);		


			/* add the replace widget */
			item = gtk_image_menu_item_new_with_label("Replace");
			gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
					gtk_image_new_from_stock(GTK_STOCK_REDO, GTK_ICON_SIZE_MENU));                   	
			gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
			g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(pl3_custom_tag_browser_replace_folder), NULL);

			return 1;
		}
	}
	return 0;
}
static void pl3_custom_tag_browser_replace_folder()
{
	/* clear the playlist */
	mpd_playlist_clear(connection);
	/* add the songs */
	pl3_custom_tag_browser_add_folder();
	/* play */
	mpd_player_play(connection);
}
static void pl3_custom_tag_browser_add_folder()
{
	char *first_tag, *second_tag, *format;
	char **tk_format;
	int i = 0, depth;
	GtkTreePath *path;
	GtkTreeModel *model = GTK_TREE_MODEL(pl3_tree);
	GtkTreeIter iter,iter_cat;
	GtkTreeSelection *selection = gtk_tree_view_get_selection((GtkTreeView *)glade_xml_get_widget (pl3_xml, "cat_tree"));
	/* get and check for selected */
	if(!gtk_tree_selection_get_selected(selection,&model, &iter_cat))
	{
		/* Nothin selected? then we don't have todo anything */
		return;
	}

	if (!mpd_check_connected(connection))
	{
		return;
	}


	path = gtk_tree_model_get_path(GTK_TREE_MODEL(pl3_tree), &iter_cat);
	if(path == NULL)
	{
		return;
	}
	depth = gtk_tree_path_get_depth(path) -2;


	if(depth <= 0)
	{
		/*lowest level, do nothing */
		gtk_tree_path_free(path);	
		return;
	}                                    	
	gtk_tree_model_get(GTK_TREE_MODEL(pl3_tree), &iter_cat, 2 , &first_tag, 1,&second_tag,PL3_CAT_BROWSE_FORMAT, &format, -1);
	tk_format = g_strsplit(format, "|",0);
	if(tk_format ==NULL)
	{
		if(second_tag) g_free(second_tag);
		if(first_tag) g_free(first_tag);
		if(format) g_free(format);
		gtk_tree_path_free(path);	
		return;
	}


	for(i=0;tk_format[i] != NULL;i++)	
	{

		if(mpd_misc_get_tag_by_name(tk_format[i])== -1)
		{

			g_strfreev(tk_format);
			gtk_tree_path_free(path);	
			if(second_tag) g_free(second_tag);
			if(first_tag) g_free(first_tag);
			if(format) g_free(format);

			return;
		}

	}

	if(depth == 1)
	{
		MpdData *data = mpd_database_find_adv(connection,TRUE, 
				mpd_misc_get_tag_by_name(tk_format[0]), first_tag, -1);
		/* lowest level selected*/
		if(data != NULL)
		{
			while(data != NULL)
			{
				mpd_playlist_queue_add(connection,data->song->file);
				data = mpd_data_get_next(data);
			}
			mpd_playlist_queue_commit(connection);
		}
	}
	else if(depth == 2)
	{
		/* second level */
		MpdData *data = mpd_database_find_adv(connection,TRUE,
				mpd_misc_get_tag_by_name(tk_format[0]),first_tag,
				mpd_misc_get_tag_by_name(tk_format[1]), second_tag, -1);

		if(data != NULL)
		{
			while(data != NULL)
			{
				mpd_playlist_queue_add(connection,data->song->file);
				data = mpd_data_get_next(data);
			}
			mpd_playlist_queue_commit(connection);
		}

	}
	else if (depth == 3)
	{
		char *first;
		/* take the upper one */
		/* go 2 up */
		if(gtk_tree_path_up(path) && gtk_tree_path_up(path))
		{
			MpdData *data  = NULL;
			gtk_tree_model_get_iter(GTK_TREE_MODEL(pl3_tree), &iter, path);
			gtk_tree_model_get(GTK_TREE_MODEL(pl3_tree), &iter, 1, &first, -1);

			/* artist and album is selected */
			data = mpd_database_find_adv(connection,TRUE,
					mpd_misc_get_tag_by_name(tk_format[0]),	first,
					mpd_misc_get_tag_by_name(tk_format[1]), first_tag,
					mpd_misc_get_tag_by_name(tk_format[2]), second_tag,-1);

			if(data != NULL)
			{
				while(data != NULL)
				{
					mpd_playlist_queue_add(connection,data->song->file);
					data = mpd_data_get_next(data);
				}
				mpd_playlist_queue_commit(connection);
			}
			g_free(first);
		}
	}
	gtk_tree_path_free(path);	
	g_strfreev(tk_format);
	if(second_tag) g_free(second_tag);
	if(first_tag) g_free(first_tag);
	if(format) g_free(format);
	return ;
}

static void pl3_custom_tag_browser_row_activated(GtkTreeView *tree, GtkTreePath *tp)
{
	GtkTreeIter iter;
	gchar *song_path;
	int depth =0;
	int type = 0;
	int playlist_length = mpd_playlist_get_playlist_length(connection);



	/* if we fail to get the path, bail out */
	depth = gtk_tree_path_get_depth(tp);

	if(!gtk_tree_model_get_iter(gtk_tree_view_get_model(tree), &iter, tp))
		return;
	gtk_tree_model_get(gtk_tree_view_get_model(tree), &iter, PL3_TB_PATH,&song_path, PL3_TB_TYPE,&type, -1);

	if(song_path == NULL) return;
	if(type == PL3_ENTRY_SONG)
	{
		pl3_push_statusbar_message("Added a song");
		mpd_playlist_add(connection, song_path);
		/* if there was no song in the playlist, play it */
		if(playlist_length == 0)
		{
			mpd_player_play(connection);
		}
	}
	else if (type == PL3_ENTRY_ALBUM)
	{
		GtkWidget *tree = glade_xml_get_widget (pl3_xml, "cat_tree");
		GtkTreeSelection *selec = gtk_tree_view_get_selection((GtkTreeView *)tree);
		GtkTreeModel *model = GTK_TREE_MODEL(pl3_tree);
		GtkTreeIter iter;

		if(gtk_tree_selection_get_selected(selec,&model, &iter))
		{
			GtkTreeIter citer;
			GtkTreePath *path = gtk_tree_model_get_path(model, &iter);
			gtk_tree_view_expand_row(GTK_TREE_VIEW(tree), path, FALSE);
			gtk_tree_path_free(path);
			if(gtk_tree_model_iter_children(model, &citer, &iter))
			{
				do{
					char *name = NULL;
					gtk_tree_model_get(model, &citer, PL3_CAT_TITLE, &name, -1);
					if(!strcmp(name, song_path))
					{
						gtk_tree_selection_select_iter(selec,&citer);
						path = gtk_tree_model_get_path(model, &citer);
						gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(tree),
								path,NULL,TRUE,0.5,0);
						gtk_tree_path_free(path);
						g_free(name);
						break;
					}
					g_free(name);
				}while(gtk_tree_model_iter_next(model, &citer));
			}
		}
	}
	/* free memory */
	g_free(song_path);
}

static void pl3_custom_tag_browser_category_selection_changed(GtkWidget *tree,GtkTreeIter *iter)
{
	long unsigned time= 0;
	gchar *string;        			
	gtk_list_store_clear(pl3_tb_store);	
	time = pl3_custom_tag_browser_view_folder(iter);
	string = format_time(time);
	pl3_push_rsb_message(string);
	g_free(string);
}
static void pl3_tag_browser_selected(GtkWidget *container)
{
	if(pl3_tb_tree == NULL)
	{
		pl3_tag_browser_init();
	}

	gtk_container_add(GTK_CONTAINER(container), pl3_tb_sw);
	gtk_widget_grab_focus(pl3_tb_tree);
	gtk_widget_show_all(pl3_tb_sw);
}
static void pl3_tag_browser_unselected(GtkWidget *container)
{
	gtk_container_remove(GTK_CONTAINER(container),pl3_tb_sw);
}


static void pl3_custom_tag_browser_button_release_event(GtkWidget *wid, GdkEventButton *event)
{
	if(event->button == 3)
	{
		/* we need an model and a iter */
		GtkTreeModel *model = GTK_TREE_MODEL(pl3_tree);
		GtkTreeIter iter;
		int depth = 0;
		GtkTreePath *path;	
		/* se need a GtkTreeSelection to know what is selected */
		GtkTreeSelection *selection = gtk_tree_view_get_selection((GtkTreeView *)glade_xml_get_widget (pl3_xml, "cat_tree"));

		/* get and check for selected */
		if(!gtk_tree_selection_get_selected(selection,&model, &iter))
		{
			/* Nothin selected? then we don't have todo anything */
			return;
		}
		/* now we want to know what level we are, and what we need to show */
		path = gtk_tree_model_get_path(GTK_TREE_MODEL(pl3_tree), &iter);
		if(path == NULL)
		{
			debug_printf(DEBUG_INFO,"Failed to get path\n");
			return;
		}
		depth = gtk_tree_path_get_depth(path);
		
		gtk_tree_path_free(path);	
		if(depth > 1)
		{
			/* here we have:  Add. Replace*/
			GtkWidget *item;
			GtkWidget *menu = gtk_menu_new();	
			/* add the add widget */
			item = gtk_image_menu_item_new_from_stock(GTK_STOCK_ADD,NULL);
			gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
			g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(pl3_tag_browser_add_selected), NULL);		


			/* add the replace widget */
			item = gtk_image_menu_item_new_with_label("Replace");
			gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
					gtk_image_new_from_stock(GTK_STOCK_REDO, GTK_ICON_SIZE_MENU));                   	
			gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
			g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(pl3_tag_browser_replace_selected), NULL);

			gtk_widget_show_all(GTK_WIDGET(menu));
			gtk_menu_popup(GTK_MENU(menu), NULL, NULL,NULL, NULL, event->button, event->time);

		}
	}
}


static void pl3_tag_browser_replace_selected()
{
	mpd_playlist_clear(connection);
	pl3_tag_browser_add_selected();
	mpd_player_play(connection);	

}
static void pl3_tag_browser_add_folder_to_queue(char *name)
{
	char *first_tag, *second_tag, *format;
	char **tk_format;
	int i = 0, depth;
	GtkTreePath *path;
	GtkTreeModel *model = GTK_TREE_MODEL(pl3_tree);
	GtkTreeIter iter,iter_cat;
	GtkTreeSelection *selection = gtk_tree_view_get_selection((GtkTreeView *)glade_xml_get_widget (pl3_xml, "cat_tree"));
	/* get and check for selected */
	if(!gtk_tree_selection_get_selected(selection,&model, &iter_cat))
	{
		/* Nothin selected? then we don't have todo anything */
		return;
	}

	if (!mpd_check_connected(connection))
	{
		return;
	}


	path = gtk_tree_model_get_path(GTK_TREE_MODEL(pl3_tree), &iter_cat);
	if(path == NULL)
	{
		return;
	}
	depth = gtk_tree_path_get_depth(path) -2;


	if(depth < 0)
	{
		/*lowest level, do nothing */
		gtk_tree_path_free(path);	
		return;
	}                                    	
	gtk_tree_model_get(GTK_TREE_MODEL(pl3_tree), &iter_cat, 2 , &first_tag, 1,&second_tag,PL3_CAT_BROWSE_FORMAT, &format, -1);
	tk_format = g_strsplit(format, "|",0);
	if(tk_format ==NULL)
	{
		if(second_tag) g_free(second_tag);
		if(first_tag) g_free(first_tag);
		if(format) g_free(format);
		gtk_tree_path_free(path);	
		return;
	}


	for(i=0;tk_format[i] != NULL;i++)	
	{

		if(mpd_misc_get_tag_by_name(tk_format[i])== -1)
		{

			g_strfreev(tk_format);
			gtk_tree_path_free(path);	
			if(second_tag) g_free(second_tag);
			if(first_tag) g_free(first_tag);
			if(format) g_free(format);

			return;
		}

	}
	if(depth == 0)
	{
		MpdData *data = mpd_database_find_adv(connection,TRUE, 
				mpd_misc_get_tag_by_name(tk_format[0]), name, 	
				-1);
		/* lowest level selected*/
		if(data != NULL)
		{
			while(data != NULL)
			{
				mpd_playlist_queue_add(connection,data->song->file);
				data = mpd_data_get_next(data);
			}
		}
	}

	if(depth == 1)
	{
		MpdData *data = mpd_database_find_adv(connection,TRUE, 
				mpd_misc_get_tag_by_name(tk_format[0]), first_tag, 
				mpd_misc_get_tag_by_name(tk_format[1]), name, 	
				-1);
		/* lowest level selected*/
		if(data != NULL)
		{
			while(data != NULL)
			{
				mpd_playlist_queue_add(connection,data->song->file);
				data = mpd_data_get_next(data);
			}
		}
	}
	else if(depth == 2)
	{
		/* second level */
		MpdData *data = mpd_database_find_adv(connection,TRUE,
				mpd_misc_get_tag_by_name(tk_format[0]),first_tag,
				mpd_misc_get_tag_by_name(tk_format[1]), second_tag,
				mpd_misc_get_tag_by_name(tk_format[2]), name,       
				-1);

		if(data != NULL)
		{
			while(data != NULL)
			{
				mpd_playlist_queue_add(connection,data->song->file);
				data = mpd_data_get_next(data);
			}
		}

	}
	else if (depth == 3)
	{
		char *first;
		/* take the upper one */
		/* go 2 up */
		if(gtk_tree_path_up(path) && gtk_tree_path_up(path))
		{
			MpdData *data  = NULL;
			gtk_tree_model_get_iter(GTK_TREE_MODEL(pl3_tree), &iter, path);
			gtk_tree_model_get(GTK_TREE_MODEL(pl3_tree), &iter, 1, &first, -1);

			/* artist and album is selected */
			data = mpd_database_find_adv(connection,TRUE,
					mpd_misc_get_tag_by_name(tk_format[0]),	first,
					mpd_misc_get_tag_by_name(tk_format[1]), first_tag,
					mpd_misc_get_tag_by_name(tk_format[2]), second_tag,
					mpd_misc_get_tag_by_name(tk_format[3]), name,
					-1);

			if(data != NULL)
			{
				while(data != NULL)
				{
					mpd_playlist_queue_add(connection,data->song->file);
					data = mpd_data_get_next(data);
				}
			}
			g_free(first);
		}
	}
	gtk_tree_path_free(path);	
	g_strfreev(tk_format);
	if(second_tag) g_free(second_tag);
	if(first_tag) g_free(first_tag);
	if(format) g_free(format);
	return ;
}




static void pl3_tag_browser_add_selected()
{
	GtkTreeIter iter;
	GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW(pl3_tb_tree));
	GtkTreeModel *model = GTK_TREE_MODEL (pl3_tb_store);
	GList *rows = gtk_tree_selection_get_selected_rows (selection, &model);
	int songs=0;
	gchar *message;
	if(rows != NULL)
	{

		gint type;
		GList *node = g_list_first(rows);
		do
		{
			gchar *name;
			GtkTreePath *path = node->data;
			gtk_tree_model_get_iter (model, &iter, path);
			gtk_tree_model_get (model, &iter, PL3_TB_PATH,&name, PL3_TB_TYPE, &type, -1);	  
			/* does this bitmask thingy works ok? I think it hsould */
			if(type&(PL3_ENTRY_SONG))
			{
				/* add them to the add list */
				mpd_playlist_queue_add(connection, name);
				songs++;
			}
			else if(type&(PL3_ENTRY_ALBUM|PL3_ENTRY_ARTIST))
			{
				/** 
				 * TODO 
				 */
				pl3_tag_browser_add_folder_to_queue(name);







			}
			g_free(name);
		}while((node = g_list_next(node)) != NULL);
	}
	/* if there are items in the add list add them to the playlist */
	mpd_playlist_queue_commit(connection);
	if(songs != 0)
	{
		message = g_strdup_printf("Added %i song%s", songs, (songs != 1)? "s":"");
		pl3_push_statusbar_message(message);
		g_free(message);                                       	
	}

	g_list_foreach (rows, (GFunc) gtk_tree_path_free, NULL);
	g_list_free (rows);
}

static void pl3_tag_browser_show_info()
{
	GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW(pl3_tb_tree));
	GtkTreeSelection *selection =gtk_tree_view_get_selection (GTK_TREE_VIEW(pl3_tb_tree));
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
			char *path;
			MpdData *data;
			gtk_tree_model_get_iter (model, &iter, (GtkTreePath *) list->data);
			gtk_tree_model_get (GTK_TREE_MODEL(pl3_tb_store), &iter, PL3_TB_PATH, &path, -1);
			data = mpd_database_find_adv(connection,TRUE,MPD_TAG_ITEM_FILENAME,path,-1);
			while(data != NULL)
			{
				if(data->type == MPD_DATA_TYPE_SONG)
				{
					call_id3_window_song(mpd_songDup(data->song));
				}
				data = mpd_data_get_next(data);
			}
			g_free(path);
		}
		while ((list = g_list_previous (list)) && mpd_check_connected(connection));
		/* free list */
		g_list_foreach (list, (GFunc) gtk_tree_path_free, NULL);
		g_list_free (list);
	}
}

static int pl3_tag_browser_playlist_key_press(GtkWidget *tree, GdkEventKey *event)
{
	if(event->state&GDK_CONTROL_MASK && event->keyval == GDK_Insert)
	{
		pl3_tag_browser_replace_selected();
	}
	else if(event->keyval == GDK_Insert)
	{
		pl3_tag_browser_add_selected();
	}
	else if(event->keyval == GDK_i)
	{
		pl3_tag_browser_show_info();
	}
	else
	{
		return pl3_window_key_press_event(tree,event);
	}
	return TRUE;
}



/* PREFERENCES */
void pref_id3b_row_remove()
{
	GtkWidget *tree = glade_xml_get_widget(tag_pref_xml,"id3b_tree");
	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));
	GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(tree));
	GtkTreeIter iter;
	if(gtk_tree_selection_get_selected(selection,&model,&iter))
	{
		gchar *title;
		gtk_tree_model_get(model,&iter,0,&title,-1);
		cfg_del_multiple_value(config, "playlist", "advbrows",title);
		pref_id3b_fill();
		pl3_custom_tag_browser_reload();
		g_free(title);
	}

}

void pref_id3b_row_changed(GtkTreeView *tree)
{
	GtkTreeSelection *selection = gtk_tree_view_get_selection(tree);
	GtkTreeModel *model = gtk_tree_view_get_model(tree);
	GtkTreeIter iter;
	if(gtk_tree_selection_get_selected(selection,&model,&iter))
	{
		gchar *format, *title;
		gchar ** tk_format = NULL;
		gtk_tree_model_get(model,&iter,0,&title,1,&format, -1);
		gtk_entry_set_text(GTK_ENTRY(glade_xml_get_widget(tag_pref_xml, "id3b_entry")),title);
		tk_format = g_strsplit(format, "|",0);
		if(tk_format ==NULL)
		{
			debug_printf(DEBUG_INFO,"pref_id3b_row_changed: failed to split\n");
			g_free(format);
			g_free(title);
			return;
		}
		else
		{
			gtk_combo_box_set_active(GTK_COMBO_BOX(glade_xml_get_widget(tag_pref_xml, "id3b_cb1")),0);
			gtk_combo_box_set_active(GTK_COMBO_BOX(glade_xml_get_widget(tag_pref_xml, "id3b_cb2")),0);
			gtk_combo_box_set_active(GTK_COMBO_BOX(glade_xml_get_widget(tag_pref_xml, "id3b_cb3")),0);

			if(tk_format[0] != NULL)
			{
				gtk_combo_box_set_active(GTK_COMBO_BOX(glade_xml_get_widget(tag_pref_xml, "id3b_cb1")),
						mpd_misc_get_tag_by_name(tk_format[0])+1);
				if(tk_format[1] != NULL)
				{
					gtk_combo_box_set_active(GTK_COMBO_BOX(glade_xml_get_widget(tag_pref_xml, "id3b_cb2")),
							mpd_misc_get_tag_by_name(tk_format[1])+1);
					if(tk_format[2] != NULL)
					{
						gtk_combo_box_set_active(GTK_COMBO_BOX(glade_xml_get_widget(tag_pref_xml, "id3b_cb3")),
								mpd_misc_get_tag_by_name(tk_format[2])+1);		
					}
				}
			}
			g_strfreev(tk_format);

		}
		g_free(format);
		g_free(title);


	}
	else
	{
		gtk_entry_set_text(GTK_ENTRY(glade_xml_get_widget(tag_pref_xml, "id3b_entry")),"");
		gtk_combo_box_set_active(GTK_COMBO_BOX(glade_xml_get_widget(tag_pref_xml, "id3b_cb1")),0);
		gtk_combo_box_set_active(GTK_COMBO_BOX(glade_xml_get_widget(tag_pref_xml, "id3b_cb2")),0);
		gtk_combo_box_set_active(GTK_COMBO_BOX(glade_xml_get_widget(tag_pref_xml, "id3b_cb3")),0);
	}
}

void pref_id3b_add_entry()
{
	GString *format = g_string_new("");
	if(gtk_combo_box_get_active(GTK_COMBO_BOX(glade_xml_get_widget(tag_pref_xml, "id3b_cb1"))))
	{
		GtkTreeIter iter;
		char *string;
		if(gtk_combo_box_get_active_iter(GTK_COMBO_BOX(glade_xml_get_widget(tag_pref_xml, "id3b_cb1")),&iter))
		{
			gtk_tree_model_get(GTK_TREE_MODEL(gtk_combo_box_get_model(
							GTK_COMBO_BOX(glade_xml_get_widget(tag_pref_xml, "id3b_cb1")))),&iter, 0,&string, -1);
			g_string_append_printf(format, "%s|",string);
			g_free(string);
		}
	}
	if(gtk_combo_box_get_active(GTK_COMBO_BOX(glade_xml_get_widget(tag_pref_xml, "id3b_cb2"))))
	{
		GtkTreeIter iter;
		char *string;
		if(gtk_combo_box_get_active_iter(GTK_COMBO_BOX(glade_xml_get_widget(tag_pref_xml, "id3b_cb2")),&iter))
		{
			gtk_tree_model_get(GTK_TREE_MODEL(gtk_combo_box_get_model(
							GTK_COMBO_BOX(glade_xml_get_widget(tag_pref_xml, "id3b_cb2")))),&iter, 0,&string, -1);
			g_string_append_printf(format, "%s|",string);
			g_free(string);
		}
	}
	if(gtk_combo_box_get_active(GTK_COMBO_BOX(glade_xml_get_widget(tag_pref_xml, "id3b_cb3"))))
	{
		GtkTreeIter iter;
		char *string;
		if(gtk_combo_box_get_active_iter(GTK_COMBO_BOX(glade_xml_get_widget(tag_pref_xml, "id3b_cb3")),&iter))
		{
			gtk_tree_model_get(GTK_TREE_MODEL(gtk_combo_box_get_model(
							GTK_COMBO_BOX(glade_xml_get_widget(tag_pref_xml, "id3b_cb3")))),&iter, 0,&string, -1);
			g_string_append_printf(format, "%s|",string);
			g_free(string);
		}
	}
	if(format->len)
	{
		g_string_erase(format, format->len-1,-1);
		if(strlen(gtk_entry_get_text(GTK_ENTRY(glade_xml_get_widget(tag_pref_xml, "id3b_entry")))))
		{
			cfg_set_multiple_value_as_string(config, "playlist", "advbrows",
					(char *)gtk_entry_get_text(GTK_ENTRY(glade_xml_get_widget(tag_pref_xml, "id3b_entry"))),
					format->str);
		}
	}
	g_string_free(format, TRUE);
	pref_id3b_fill();
	pl3_custom_tag_browser_reload();
}



static void pref_id3b_fill()
{
	GtkWidget *tree = glade_xml_get_widget(tag_pref_xml,"id3b_tree");
	conf_mult_obj *list;
	GtkListStore *ls = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(tree)));
	list = cfg_get_multiple_as_string(config, "playlist", "advbrows");
	gtk_list_store_clear(ls);	
	if(list != NULL)
	{
		conf_mult_obj *data = list;
		do{
			if(strlen(data->key) && strlen(data->value))
			{
				GtkTreeIter iter;
				gtk_list_store_append(ls, &iter);
				gtk_list_store_set(ls,&iter,0,data->key,1,data->value,2,TRUE,-1);
			}
			data = data->next;
		}
		while(data);
		cfg_free_multiple(list);
	}

}
static void pref_id3b_init()
{
	GtkCellRenderer *renderer = NULL;
	GtkListStore *ab_lstore = NULL;
	GtkWidget *tree = glade_xml_get_widget(tag_pref_xml,"id3b_tree");

	/* create model to store the data in */
	ab_lstore = gtk_list_store_new(3, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_BOOLEAN);
	/* add model to tree */
	gtk_tree_view_set_model(GTK_TREE_VIEW(tree), GTK_TREE_MODEL(ab_lstore)); 

	/* add columns */
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree),0,_("Name"), renderer,
			"text",0,
			"editable",2,
			NULL);
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree),1,_("Format"), renderer,
			"text",1,
			NULL);          
	gtk_combo_box_set_active(GTK_COMBO_BOX(glade_xml_get_widget(tag_pref_xml, "id3b_cb1")),0);
	gtk_combo_box_set_active(GTK_COMBO_BOX(glade_xml_get_widget(tag_pref_xml, "id3b_cb2")),0);
	gtk_combo_box_set_active(GTK_COMBO_BOX(glade_xml_get_widget(tag_pref_xml, "id3b_cb3")),0);
	pref_id3b_fill();

}
static void tag_pref_destroy(GtkWidget *container)
{
	if(tag_pref_xml)
	{
		GtkWidget *vbox = glade_xml_get_widget(tag_pref_xml, "tag-vbox");
		gtk_container_remove(GTK_CONTAINER(container),vbox);
		g_object_unref(tag_pref_xml);
		tag_pref_xml = NULL;
	}
}
static void tag_pref_construct(GtkWidget *container)
{
	gchar *path = gmpc_get_full_glade_path("gmpc.glade");
	tag_pref_xml = glade_xml_new(path, "tag-vbox",NULL);
	g_free(path);
	if(tag_pref_xml)
	{
		GtkWidget *vbox = glade_xml_get_widget(tag_pref_xml, "tag-vbox");
		pref_id3b_init();
		gtk_container_add(GTK_CONTAINER(container),vbox);
		glade_xml_signal_autoconnect(tag_pref_xml);
	}
}

static void tag_connection(MpdObj *mi, int connect, gpointer data)
{
	if(!pl3_tree) return;
	if(connect) {
		if(!pl3_tag_tree_ref) {
			/* add tag list */
			pl3_custom_tag_browser_add();
		}
	}else{
		GtkTreeIter iter;
		if(pl3_tag_tree_ref) {
			/* remove tag list */
			GtkTreePath *path = gtk_tree_row_reference_get_path(pl3_tag_tree_ref);
			if(path){
				if(gtk_tree_model_get_iter(GTK_TREE_MODEL(pl3_tree), &iter, path)) {
					gtk_tree_store_remove(pl3_tree, &iter);
				}
				gtk_tree_path_free(path);
			}
			gtk_tree_row_reference_free(pl3_tag_tree_ref);
			pl3_tag_tree_ref = NULL;
		}
	}
	pl3_update_go_menu();
}


static void pl3_custom_tag_browser_activate()
{
	GtkTreeSelection *selec = gtk_tree_view_get_selection((GtkTreeView *)
			glade_xml_get_widget (pl3_xml, "cat_tree"));

	GtkTreePath *path = gtk_tree_row_reference_get_path(pl3_tag_tree_ref);
	if(path)
	{
		gtk_tree_selection_select_path(selec, path);
	}
}


static int pl3_custom_tag_add_go_menu(GtkWidget *menu)
{
	if(mpd_server_check_version(connection, 0,12,0))
	{
		GtkWidget *item = NULL;

		item = gtk_image_menu_item_new_with_label(_("Tag Browser"));
		gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item), 
				gtk_image_new_from_stock("media-artist", GTK_ICON_SIZE_MENU));
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
		g_signal_connect(G_OBJECT(item), "activate", 
				G_CALLBACK(pl3_custom_tag_browser_activate), NULL);
		return 1;
	}
	return 0;
}
