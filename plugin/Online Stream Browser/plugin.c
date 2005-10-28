#include <stdio.h>
#include <string.h>
#include <gdk/gdkkeysyms.h>
#include <glade/glade.h>
#include "vfs_download.h"
#include "open-location.h"
#include "../../src/plugin.h"
#include "../../config.h"

GtkWidget *osb_pref_vbox = NULL;

void osb_add(GtkWidget *cat_tree);
void osb_selected(GtkWidget *container);
void osb_unselected(GtkWidget *container);
void osb_changed(GtkWidget *tree, GtkTreeIter *iter);
void osb_browser_view_browser(gchar *url,gchar *name);
void osb_browser_add_selected();
void osb_browser_replace_selected();
void osb_browser_row_activated(GtkTreeView *tree, GtkTreePath *tp);
int osb_browser_playlist_key_press(GtkWidget *tree, GdkEventKey *event);
void osb_browser_button_release_event(GtkWidget *but, GdkEventButton *event);
int osb_cat_popup(GtkWidget *menu, int type, GtkWidget *tree, GdkEventButton *event);
/* preferences */
void osb_construct(GtkWidget *container);
void osb_destroy(GtkWidget *container);

/* */
GtkWidget *pl3_osb_sw = NULL;
GtkWidget *pl3_osb_tree = NULL;
GtkListStore *pl3_osb_store = NULL;
GtkTreePath *path = NULL;

extern GtkTreeStore *pl3_tree;
extern GladeXML *pl3_xml;


enum{
	PL3_OSB_PATH,
	PL3_OSB_TYPE,
	PL3_OSB_TITLE,
	PL3_OSB_ICON,
	PL3_OSB_ROWS
};


/* Plugin info */
gmpcPrefPlugin osb_gpp = {
	osb_construct,
	osb_destroy
};

/* Needed plugin_wp stuff */
gmpcPlBrowserPlugin osb_gbp = {
	osb_add,
	osb_selected,
	osb_unselected,
	osb_changed,
	NULL,
	osb_cat_popup,
	NULL
};

gmpcPlugin plugin = {
	"Online Stream Browser",
	{0,0,1},
	GMPC_PLUGIN_PL_BROWSER,
	0,
	&osb_gbp,
	NULL,
	&osb_gpp
};

void osb_browser_refresh()
{
	gchar *name, *string, *url;
	GtkTreeModel *model = GTK_TREE_MODEL(pl3_tree);
	GtkTreeIter iter;
	GtkTreeSelection *selec = gtk_tree_view_get_selection((GtkTreeView *)glade_xml_get_widget (pl3_xml, "cat_tree"));
	if(!gtk_tree_selection_get_selected(selec,&model, &iter))
	{
		return;
	}
	gtk_tree_model_get(model, &iter,PL3_CAT_INT_ID, &url, PL3_CAT_TITLE, &name, -1);
	if(url == NULL || name == NULL ) return;
	string = g_strdup_printf("%s/.gmpc/%s", g_get_home_dir(), name);
	unlink(string);
	g_free(string);
	osb_browser_view_browser(url, name);
}


void osb_browser_del_source()
{
	GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(glade_xml_get_widget(pl3_xml,"pl3_win")),
			GTK_DIALOG_MODAL,
			GTK_MESSAGE_WARNING,
			GTK_BUTTONS_YES_NO,
			"Are you sure you want to delete this source?");

	switch(gtk_dialog_run(GTK_DIALOG(dialog)))
	{
		case GTK_RESPONSE_YES:
			{
				GtkTreeSelection *selec = gtk_tree_view_get_selection((GtkTreeView *)glade_xml_get_widget (pl3_xml, "cat_tree"));
				GtkTreeModel *model = GTK_TREE_MODEL(pl3_tree);
				GtkTreeIter iter;
				char *id;

				if(gtk_tree_selection_get_selected(selec,&model, &iter))
				{
					gtk_tree_model_get(model, &iter, PL3_CAT_TITLE, &id, -1);
					cfg_del_multiple_value(config, "osb", "streams",id);
					gtk_tree_store_remove(pl3_tree, &iter);
				}
				default:
				break;
			}
	}
	gtk_widget_destroy(dialog);
}

void osb_browser_add_source()
{
	GladeXML *gxml =glade_xml_new (GLADE_PATH "playlist3.glade", "osb_add_dialog", NULL);
	GtkWidget *dialog = glade_xml_get_widget(gxml, "osb_add_dialog");

	switch(gtk_dialog_run(GTK_DIALOG(dialog)))
	{
		case GTK_RESPONSE_YES:
			{
				GtkTreeIter iter,child;
				const gchar *key = gtk_entry_get_text(GTK_ENTRY(glade_xml_get_widget(gxml, "entry_name")));
				const gchar *value = gtk_entry_get_text(GTK_ENTRY(glade_xml_get_widget(gxml, "entry_url")));
				cfg_set_multiple_value_as_string(config,"osb", "streams", (gchar *)key, (gchar *)value);
				gtk_tree_model_get_iter(GTK_TREE_MODEL(pl3_tree), &iter, path);
				gtk_tree_store_append(pl3_tree, &child, &iter);
				gtk_tree_store_set(pl3_tree, &child, 
						PL3_CAT_TYPE, plugin.id,
						PL3_CAT_TITLE, key,
						PL3_CAT_INT_ID,value,
						PL3_CAT_ICON_ID, "icecast",
						PL3_CAT_PROC, FALSE,          	
						PL3_CAT_ICON_SIZE,GTK_ICON_SIZE_DND,-1);

			}
		default:
			break;

	}

	gtk_widget_destroy(dialog);
	g_object_unref(gxml);
}

int osb_browser_button_press_event(GtkTreeView *tree, GdkEventButton *event)
{
	GtkTreeSelection *sel = gtk_tree_view_get_selection(tree);
	if(event->button != 3 || gtk_tree_selection_count_selected_rows(sel) < 2|| !mpd_check_connected(connection))	
	{
		return FALSE;                                                                                           	
	}
	return TRUE;
}

void osb_browser_fill_view(char *buffer)
{
	xmlDocPtr xmldoc;
	xmlNodePtr root;
	xmlNodePtr cur;
	gchar *name, *string;
	GtkTreeModel *model = GTK_TREE_MODEL(pl3_tree);
	GtkTreeIter iter;
	GtkTreeSelection *selec = gtk_tree_view_get_selection((GtkTreeView *)glade_xml_get_widget (pl3_xml, "cat_tree"));
	if(!gtk_tree_selection_get_selected(selec,&model, &iter))
	{
		return;
	}

	gtk_tree_model_get(model, &iter, PL3_CAT_TITLE, &name, -1);
	string = g_strdup_printf("%s/.gmpc/%s", g_get_home_dir(), name);
	g_free(name);
	if(buffer != NULL)
	{
		FILE *fp = fopen(string, "w");
		if(fp != NULL)
		{
			fputs(buffer,fp);
			fclose(fp);
		}
		else
		{
			g_free(string);	
			return;
		}
	}
	xmldoc = xmlParseFile(string);
	g_free(string);
	root = xmlDocGetRootElement(xmldoc);
	cur = root->xmlChildrenNode;
	while(cur != NULL)
	{
		if(xmlStrEqual(cur->name, (xmlChar *)"entry"))
		{
			xmlNodePtr cur1 = cur->xmlChildrenNode;
			GtkTreeIter iter;
			char *string=NULL, *name=NULL, *bitrate=NULL, *genre=NULL;
			gtk_list_store_append(pl3_osb_store, &iter);
			gtk_list_store_set (pl3_osb_store, &iter,
					PL3_OSB_TYPE, PL3_ENTRY_STREAM, 
					PL3_OSB_ICON, "media-stream", 
					-1);

			while(cur1 != NULL)
			{
				if(xmlStrEqual(cur1->name, (xmlChar *)"server_name"))
				{
					name = (char *)xmlNodeGetContent(cur1);
				}
				else if(xmlStrEqual(cur1->name, (xmlChar *)"genre"))
				{
					genre = (char *)xmlNodeGetContent(cur1);
				}
				else if (xmlStrEqual(cur1->name,(xmlChar *) "bitrate"))
				{
					bitrate = (char *)xmlNodeGetContent(cur1);
				}
				else if(xmlStrEqual(cur1->name, (xmlChar *)"listen_url"))
				{
					string = (char *)xmlNodeGetContent(cur1);
					gtk_list_store_set(pl3_osb_store, &iter, PL3_OSB_PATH,string, -1);
					xmlFree(string);
				}

				cur1 = cur1->next;
			}
			string = g_strdup_printf("Station: %s\nGenre: %s\nBitrate: %s", name,genre, bitrate);
			gtk_list_store_set(pl3_osb_store, &iter, PL3_OSB_TITLE, string, -1);
			g_free(string);
			xmlFree(name);
			xmlFree(genre);
			xmlFree(bitrate);

		}

		cur = cur->next;
	}
	xmlFreeDoc(xmldoc);
	xmlCleanupParser();
}

void osb_browser_view_browser(gchar *url,gchar *name)
{
	gchar *string = g_strdup_printf("%s/.gmpc/%s", g_get_home_dir(), name);
	gtk_list_store_clear(pl3_osb_store);
	if(g_file_test(string, G_FILE_TEST_EXISTS))
	{
		osb_browser_fill_view(NULL);
	}
	else
	{
		start_transfer(url,(void *)osb_browser_fill_view,NULL, NULL);
	}
	g_free(string);
}

void osb_changed(GtkWidget *tree, GtkTreeIter *iter)
{
	gchar *url =NULL,*name = NULL;
	gtk_tree_model_get(gtk_tree_view_get_model(GTK_TREE_VIEW(tree)), iter, 
			PL3_CAT_INT_ID, &url, PL3_CAT_TITLE, &name,-1);

	if(url != NULL && strlen(url) > 0) 
	{
		osb_browser_view_browser(url,name);
	}
	else
	{
		gtk_list_store_clear(pl3_osb_store);
	}
	pl3_push_rsb_message("");
}

void osb_init()
{
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column = NULL;
	GValue value;
	pl3_osb_store = gtk_list_store_new (PL3_OSB_ROWS, 
			GTK_TYPE_STRING, /* path to file */
			GTK_TYPE_INT,	/* type, FILE/PLAYLIST/FOLDER  */
			GTK_TYPE_STRING,	/* title to display */
			GTK_TYPE_STRING); /* icon type */



	renderer = gtk_cell_renderer_pixbuf_new ();

	column = gtk_tree_view_column_new ();
	gtk_tree_view_column_pack_start (column, renderer, FALSE);
	gtk_tree_view_column_set_attributes (column,renderer,"stock-id", PL3_OSB_ICON,NULL);
	memset(&value, 0, sizeof(value));
	/* set value for ALL */
	g_value_init(&value, G_TYPE_FLOAT);
	g_value_set_float(&value, 0.0);
	g_object_set_property(G_OBJECT(renderer), "yalign", &value); 

	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_set_attributes (column,renderer,"text", PL3_OSB_TITLE, NULL);


	/* set up the tree */
	pl3_osb_tree= gtk_tree_view_new_with_model(GTK_TREE_MODEL(pl3_osb_store));
	/* insert the column in the tree */
	gtk_tree_view_append_column (GTK_TREE_VIEW (pl3_osb_tree), column);                                         	
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(pl3_osb_tree), FALSE);
	gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(pl3_osb_tree), TRUE);
	gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW(pl3_osb_tree)), GTK_SELECTION_SINGLE);

	/* setup signals */
	g_signal_connect(G_OBJECT(pl3_osb_tree), "row-activated",G_CALLBACK(osb_browser_row_activated), NULL); 
	g_signal_connect(G_OBJECT(pl3_osb_tree), "button-press-event", G_CALLBACK(osb_browser_button_press_event), NULL);
	g_signal_connect(G_OBJECT(pl3_osb_tree), "button-release-event", G_CALLBACK(osb_browser_button_release_event), NULL);
	g_signal_connect(G_OBJECT(pl3_osb_tree), "key-press-event", G_CALLBACK(osb_browser_playlist_key_press), NULL);
	/* set up the scrolled window */
	pl3_osb_sw = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(pl3_osb_sw), GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(pl3_osb_sw), GTK_SHADOW_ETCHED_IN);
	gtk_container_add(GTK_CONTAINER(pl3_osb_sw), pl3_osb_tree);

	/* set initial state */
	printf("initialized Online Stream Browser treeview\n");
	g_object_ref(G_OBJECT(pl3_osb_sw));
}	


void osb_add(GtkWidget *cat_tree)
{
	GtkTreeIter iter,child;
	
	conf_mult_obj *list;

	GtkTreeStore *pl3_tree = (GtkTreeStore *)gtk_tree_view_get_model(GTK_TREE_VIEW(cat_tree));	
	if(!cfg_get_single_value_as_int_with_default(config, "osb", "enable", 0)) return;
	printf("adding plugin_osb: %i '%s'\n", plugin.id, plugin.name);
	gtk_tree_store_append(pl3_tree, &iter, NULL);
	gtk_tree_store_set(pl3_tree, &iter, 
			PL3_CAT_TYPE, plugin.id,
			PL3_CAT_TITLE, "Online Stream Browser",
			PL3_CAT_INT_ID, "",
			PL3_CAT_ICON_ID, "icecast",
			PL3_CAT_PROC, TRUE,
			PL3_CAT_ICON_SIZE,GTK_ICON_SIZE_DND,-1);
	path = gtk_tree_model_get_path(GTK_TREE_MODEL(pl3_tree), &iter);
	list = cfg_get_multiple_as_string(config, "osb", "streams");
	if(list != NULL)
	{
		conf_mult_obj *data = list;
		do{
			if(data->key != NULL && data->value != NULL)
			{
				gtk_tree_store_append(pl3_tree, &child, &iter);
				gtk_tree_store_set(pl3_tree, &child, 
						PL3_CAT_TYPE, plugin.id,
						PL3_CAT_TITLE, data->key,
						PL3_CAT_INT_ID, data->value,
						PL3_CAT_ICON_ID, "icecast",
						PL3_CAT_PROC, TRUE,          	
						PL3_CAT_ICON_SIZE,GTK_ICON_SIZE_DND,-1);
			}
			data = data->next;
		}while(data  != NULL);
		cfg_free_multiple(list);
	}
}

void osb_selected(GtkWidget *container)
{

	if(pl3_osb_sw== NULL)
	{
		osb_init();
	}

	gtk_container_add(GTK_CONTAINER(container), pl3_osb_sw);
	gtk_widget_show_all(pl3_osb_sw);
	while (gtk_events_pending ())
		gtk_main_iteration ();

}

void osb_unselected(GtkWidget *container)
{
	gtk_container_remove(GTK_CONTAINER(container),pl3_osb_sw);
}

void osb_browser_replace_selected()
{
	mpd_playlist_clear(connection);
	osb_browser_add_selected();
	mpd_player_play(connection);	

}
void osb_browser_add_selected()
{
	GtkTreeIter iter;
	GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW(pl3_osb_tree));
	GtkTreeModel *model = GTK_TREE_MODEL (pl3_osb_store);
	GList *rows = gtk_tree_selection_get_selected_rows (selection, &model);
	int songs=0;
	gchar *message;
	if(rows != NULL)
	{
		gchar *name;
		gint type;
		GList *node = g_list_first(rows);
		do
		{
			GtkTreePath *path = node->data;
			gtk_tree_model_get_iter (model, &iter, path);
			gtk_tree_model_get (model, &iter, PL3_OSB_PATH,&name, PL3_OSB_TYPE, &type, -1);	  
			/* does this bitmask thingy works ok? I think it hsould */
			if(type&(PL3_ENTRY_STREAM))
			{
				/* add them to the add list */
//				mpd_playlist_queue_add(connection, name);
				ol_create_url(NULL,name);
				songs++;
			}
		}while((node = g_list_next(node)) != NULL);
	}
	/* if there are items in the add list add them to the playlist */
//	mpd_playlist_queue_commit(connection);
	if(songs != 0)
	{
		message = g_strdup_printf("Added %i song%s", songs, (songs != 1)? "s":"");
		pl3_push_statusbar_message(message);
		g_free(message);                                       	
	}

	g_list_foreach (rows, (GFunc) gtk_tree_path_free, NULL);
	g_list_free (rows);
}
void osb_browser_row_activated(GtkTreeView *tree, GtkTreePath *tp)
{
	osb_browser_add_selected();
}


int osb_browser_playlist_key_press(GtkWidget *tree, GdkEventKey *event)
{
	if(event->state == GDK_CONTROL_MASK && event->keyval == GDK_Insert)
	{
		osb_browser_replace_selected();		
	}
	else if(event->keyval == GDK_Insert)
	{
		osb_browser_add_selected();		
	}
	else
	{
		return pl3_window_key_press_event(tree,event);
	}
	return TRUE;
}


void osb_browser_button_release_event(GtkWidget *but, GdkEventButton *event)
{
	if(event->button != 3) return;
	GtkWidget *item;
	GtkWidget *menu = gtk_menu_new();

	/* add the add widget */
	item = gtk_image_menu_item_new_from_stock(GTK_STOCK_ADD,NULL);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(osb_browser_add_selected), NULL);

	/* add the replace widget */
	item = gtk_image_menu_item_new_with_label("Replace");
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
			gtk_image_new_from_stock(GTK_STOCK_REDO, GTK_ICON_SIZE_MENU));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(osb_browser_replace_selected), NULL);

	gtk_widget_show_all(menu);
	gtk_menu_popup(GTK_MENU(menu), NULL, NULL,NULL, NULL, event->button, event->time);	
	return;
}
void osb_enable_toggle(GtkWidget *wid)
{
	int kk = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(wid));
	cfg_set_single_value_as_int(config, "osb", "enable", kk);
	pl3_reinitialize_tree();
}

/* PREFERENCES */
void osb_destroy(GtkWidget *container)
{
	gtk_container_remove(GTK_CONTAINER(container), osb_pref_vbox);
}

void osb_construct(GtkWidget *container)
{
	GtkWidget *enable_cg = gtk_check_button_new_with_mnemonic("_Enable Online Stream Browser");
	GtkWidget *label = NULL;
	osb_pref_vbox = gtk_vbox_new(FALSE,6);

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(enable_cg), 	
			cfg_get_single_value_as_int_with_default(config, "osb", "enable", 0));
	
	g_signal_connect(G_OBJECT(enable_cg), "toggled", G_CALLBACK(osb_enable_toggle), NULL);
	gtk_box_pack_start(GTK_BOX(osb_pref_vbox), enable_cg, FALSE, FALSE, 0);
	gtk_container_add(GTK_CONTAINER(container), osb_pref_vbox);
	gtk_widget_show_all(container);
}

int osb_cat_popup(GtkWidget *menu, int type, GtkWidget *tree, GdkEventButton *event)
{

	if(type == 0)
	{
		GtkWidget *item;
		item = gtk_image_menu_item_new_with_label("Add Location");
		gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
				gtk_image_new_from_stock(GTK_STOCK_ADD, GTK_ICON_SIZE_MENU));
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
		g_signal_connect_swapped(G_OBJECT(item), "activate", G_CALLBACK(ol_create), NULL);
		return 1;
	}
	if(type == plugin.id)
	{
		/* here we have:  Add. Replace*/
		GtkWidget *item;
		GtkTreeSelection *selec = gtk_tree_view_get_selection((GtkTreeView *)glade_xml_get_widget (pl3_xml, "cat_tree"));
		GtkTreeModel *model = GTK_TREE_MODEL(pl3_tree);
		GtkTreeIter iter;
		char *id;

		/* add the add widget */
		item = gtk_image_menu_item_new_from_stock(GTK_STOCK_ADD,NULL);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
		g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(osb_browser_add_source), NULL);		

		if(gtk_tree_selection_get_selected(selec,&model, &iter))
		{
			gtk_tree_model_get(model, &iter, PL3_CAT_INT_ID, &id, -1);
			if(strlen(id) > 0)
			{
				item = gtk_image_menu_item_new_from_stock(GTK_STOCK_REMOVE,NULL);
				gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
				g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(osb_browser_del_source), NULL);		

				item = gtk_image_menu_item_new_from_stock(GTK_STOCK_REFRESH,NULL);
				gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
				g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(osb_browser_refresh), NULL);		
			}
		}
		/* show everything and popup */
		return 1;
	}
	return 0;
}
