/*
 * Copyright (C) 2004-2005 Qball Cow <Qball@qballcow.nl>
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
#include <time.h>
#include <unistd.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <config.h>
#include "main.h"

#include "plugin.h"




#include "misc.h"
#include "playlist3.h"
#include "open-location.h"
#include "vfs_download.h"
#include "config1.h"


extern config_obj *config;
extern GtkTreeStore *pl3_tree;
extern GtkListStore *pl3_store;
extern GladeXML *pl3_xml;
GtkTreePath *path = NULL;
/************************************
 * XIPH BROWSER
 */

void pl3_osb_browser_add()
{
#ifdef ENABLE_GNOME_VFS
	GtkTreeIter iter,child;
	conf_mult_obj *list;

	if(!cfg_get_single_value_as_int_with_default(config, "osb", "enable", FALSE))
	{
		return;
	}
	gtk_tree_store_append(pl3_tree, &iter, NULL);
	gtk_tree_store_set(pl3_tree, &iter, 
			PL3_CAT_TYPE, PL3_BROWSE_XIPH,
			PL3_CAT_TITLE, "Online Streams",
			PL3_CAT_INT_ID, "",
			PL3_CAT_ICON_ID, "icecast",
			PL3_CAT_PROC, FALSE,          	
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
					PL3_CAT_TYPE, PL3_BROWSE_XIPH,
					PL3_CAT_TITLE, data->key,
					PL3_CAT_INT_ID, data->value,
					PL3_CAT_ICON_ID, "icecast",
					PL3_CAT_PROC, FALSE,          	
					PL3_CAT_ICON_SIZE,GTK_ICON_SIZE_DND,-1);
			}
			data = data->next;
		}while(data  != NULL);
		cfg_free_multiple(list);
	}
#endif
}


void pl3_osb_browser_fill_view(char *buffer)
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
			gtk_list_store_append(pl3_store, &iter);
			gtk_list_store_set (pl3_store, &iter,
					PL3_SONG_POS, PL3_ENTRY_STREAM, 
					PL3_SONG_STOCK_ID, "media-stream", 
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
					gtk_list_store_set(pl3_store, &iter, PL3_SONG_ID, string, -1);
					xmlFree(string);
				}

				cur1 = cur1->next;
			}
			string = g_strdup_printf("Station: %s\nGenre: %s\nBitrate: %s", name,genre, bitrate);
			gtk_list_store_set(pl3_store, &iter, PL3_SONG_TITLE, string, -1);
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

void pl3_osb_browser_view_browser(gchar *url,gchar *name)
{
#ifdef ENABLE_GNOME_VFS
	gchar *string = g_strdup_printf("%s/.gmpc/%s", g_get_home_dir(), name);
	gtk_list_store_clear(pl3_store);
	if(g_file_test(string, G_FILE_TEST_EXISTS))
	{
		pl3_osb_browser_fill_view(NULL);
	}
	else
	{
		start_transfer(url,(void *)pl3_osb_browser_fill_view,NULL, glade_xml_get_widget(pl3_xml, "pl3_win"));
	}
	g_free(string);
#endif
}



void pl3_osb_browser_refresh()
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
	pl3_osb_browser_view_browser(url, name);
}

void pl3_osb_browser_del_source()
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

void pl3_osb_browser_add_source()
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
						PL3_CAT_TYPE, PL3_BROWSE_XIPH,
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
void pl3_osb_browser_row_activated(GtkTreeView *tree, GtkTreePath *tp)
{
//   pl3_browse_add_selected();
}

void pl3_osb_browser_category_selection_changed(GtkTreeView *tree, GtkTreeIter *iter)
{
	gchar *url =NULL,*name = NULL;
	gtk_tree_view_set_model(tree, GTK_TREE_MODEL(pl3_store));
	gtk_tree_model_get(GTK_TREE_MODEL(pl3_tree), iter, PL3_CAT_INT_ID, &url, PL3_CAT_TITLE, &name,-1);

	if(url != NULL && strlen(url) > 0) 
	{
		pl3_osb_browser_view_browser(url,name);
	}
	else
	{
		gtk_list_store_clear(pl3_store);
	}
	gtk_statusbar_push(GTK_STATUSBAR(glade_xml_get_widget(pl3_xml, "statusbar2")),0, "");
}
