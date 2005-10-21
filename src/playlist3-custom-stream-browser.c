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
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <config.h>
#include "main.h"

#include "plugin.h"

#include "strfsong.h"
#include "misc.h"
#include "playlist3.h"
#include "open-location.h"
#include "vfs_download.h"
#include "config1.h"
#include "playlist3-custom-stream-browser.h"

extern config_obj *config;
extern GladeXML *pl3_xml;
extern GtkTreeStore *pl3_tree;
extern GtkListStore *pl3_store;
extern GtkListStore *pl2_store;


/***********************************
 * Custom Streams
 */
void pl3_custom_stream_add()
{
#ifdef ENABLE_GNOME_VFS
	GtkTreeIter iter,child;
	if(!cfg_get_single_value_as_int_with_default(config, "playlist", "custom_stream_enable", TRUE))
	{
		return;
	}
	gtk_tree_store_append(pl3_tree, &iter, NULL);
	gtk_tree_store_set(pl3_tree, &iter, 
			PL3_CAT_TYPE, PL3_BROWSE_CUSTOM_STREAM,
			PL3_CAT_TITLE, "Custom Streams",
			PL3_CAT_INT_ID, "",
			PL3_CAT_ICON_ID, "media-stream",
			PL3_CAT_PROC, FALSE,          	
			PL3_CAT_ICON_SIZE,GTK_ICON_SIZE_DND,-1);
	gtk_tree_store_append(pl3_tree, &child, &iter);
	gtk_tree_store_set(pl3_tree, &child, 
			PL3_CAT_TYPE, PL3_BROWSE_CUSTOM_STREAM,
			PL3_CAT_TITLE, "Add a Stream",
			PL3_CAT_INT_ID, "add",
			PL3_CAT_ICON_ID, "icecast",
			PL3_CAT_PROC, FALSE,          	
			PL3_CAT_ICON_SIZE,GTK_ICON_SIZE_DND,-1);
#endif
}

void pl3_custom_stream_view_browser()
{

	/* make this path configurable, we don't use gnome-vfs for nothing */
	gchar *path = g_strdup_printf("/%s/.gmpc.cst",g_get_home_dir());
	if(g_file_test(path, G_FILE_TEST_EXISTS))
	{
		xmlDocPtr xmldoc = xmlParseFile(path);
		xmlNodePtr root = xmlDocGetRootElement(xmldoc);
		xmlNodePtr cur = root->xmlChildrenNode;
		while(cur != NULL)
		{
			if(xmlStrEqual(cur->name, (xmlChar *)"entry"))
			{
				xmlNodePtr cur1 = cur->xmlChildrenNode;
				GtkTreeIter iter;
				char *name=NULL;
				gtk_list_store_append(pl3_store, &iter);
				gtk_list_store_set (pl3_store, &iter,
						PL3_SONG_POS, PL3_ENTRY_STREAM, 
						PL3_SONG_STOCK_ID, "media-stream", 
						-1);
				while(cur1 != NULL)
				{
					if(xmlStrEqual(cur1->name,(xmlChar *)"name"))
					{
						gtk_list_store_set(pl3_store, &iter, PL3_SONG_TITLE, xmlNodeGetContent(cur1), -1);
						name = (char *)xmlNodeGetContent(cur1);
					}
					else if(xmlStrEqual(cur1->name, (xmlChar *)"listen_url"))
					{
						gtk_list_store_set(pl3_store, &iter, PL3_SONG_ID, xmlNodeGetContent(cur1), -1);
					}
					cur1 = cur1->next;
				}

			}

			cur = cur->next;
		}
		xmlFreeDoc(xmldoc);
		xmlCleanupParser();
	}
	g_free(path);
}


void pl3_custom_stream_add_url_changed(GtkEntry *entry, GtkWidget *button)
{
	if(strstr(gtk_entry_get_text(entry), "://"))
	{
		gtk_widget_set_sensitive(button, TRUE);
	}	
	else
	{
		gtk_widget_set_sensitive(button, FALSE);
	}


}

void pl3_custom_stream_add_stream(gchar *name, gchar *url)
{
	GladeXML *xml = glade_xml_new(GLADE_PATH"playlist3.glade", "add_stream",NULL);
	GtkWidget *dialog = glade_xml_get_widget(xml, "add_stream");
	gtk_window_set_transient_for(GTK_WINDOW(dialog),GTK_WINDOW(glade_xml_get_widget(pl3_xml, "pl3_win")));
	g_signal_connect(G_OBJECT(glade_xml_get_widget(xml, "entry_url")),"changed", G_CALLBACK(pl3_custom_stream_add_url_changed), 
			glade_xml_get_widget(xml, "button_add"));
	gtk_widget_show_all(dialog);
	if(name != NULL)
	{
		gtk_entry_set_text(GTK_ENTRY(glade_xml_get_widget(xml, "entry_name")),name);
	}
	if(url != NULL)
	{
		gtk_entry_set_text(GTK_ENTRY(glade_xml_get_widget(xml, "entry_url")),url);
	}                                                                                   	
	switch(gtk_dialog_run(GTK_DIALOG(dialog)))
	{
		case GTK_RESPONSE_OK:
			{
				gchar *path = g_strdup_printf("%s/.gmpc.cst",g_get_home_dir());
				xmlDocPtr xmldoc;
				xmlNodePtr newn,new2,root;
				if(g_file_test(path, G_FILE_TEST_EXISTS))
				{
					xmldoc = xmlParseFile(path);
					root = xmlDocGetRootElement(xmldoc);
				}
				else
				{
					xmldoc = xmlNewDoc((xmlChar *)"1.0");
					root = xmlNewDocNode(xmldoc, NULL, (xmlChar *)"streams",NULL);
					xmlDocSetRootElement(xmldoc, root);

				}
				newn = xmlNewChild(root, NULL, (xmlChar *)"entry",NULL);
				new2 = xmlNewChild(newn, NULL, (xmlChar *)"name", (xmlChar *)gtk_entry_get_text(GTK_ENTRY(glade_xml_get_widget(xml, "entry_name"))));
				new2 = xmlNewChild(newn, NULL, (xmlChar *)"listen_url", (xmlChar *)gtk_entry_get_text(GTK_ENTRY(glade_xml_get_widget(xml, "entry_url"))));

				xmlSaveFile(path, xmldoc);	


				g_free(path);
			}
		default:
			break;	
	}
	gtk_widget_destroy(dialog);
	g_object_unref(xml);
}


/**/
void pl3_custom_stream_save_tree()
{
	gchar *path = g_strdup_printf("%s/.gmpc.cst",g_get_home_dir());
	xmlDocPtr xmldoc;
	xmlNodePtr newn,new2,root;                        
	GtkTreeIter iter;		

	xmldoc = xmlNewDoc((xmlChar *)"1.0");
	root = xmlNewDocNode(xmldoc, NULL, (xmlChar *)"streams",NULL);
	xmlDocSetRootElement(xmldoc, root);
	if(gtk_tree_model_get_iter_first(GTK_TREE_MODEL(pl3_store), &iter))
	{
		do
		{
			gchar *name, *lurl;
			gtk_tree_model_get(GTK_TREE_MODEL(pl3_store), &iter,SONG_ID, &lurl, SONG_TITLE, &name, -1);
			newn = xmlNewChild(root, NULL, (xmlChar *)"entry",NULL);
			new2 = xmlNewChild(newn, NULL, (xmlChar *)"name",(xmlChar *)name); 
			new2 = xmlNewChild(newn, NULL, (xmlChar *)"listen_url", (xmlChar *)lurl);
		}while(gtk_tree_model_iter_next(GTK_TREE_MODEL(pl3_store), &iter));

	}
	xmlSaveFile(path, xmldoc);	
}





/* where going todo this the dirty way.
 * Delete the requested streams then read the info from the tree
 */
void pl3_custom_stream_remove()
{
	GtkTreeModel *model = GTK_TREE_MODEL(pl3_store);
	/* grab the selection from the tree */
	GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW(glade_xml_get_widget (pl3_xml, "playlist_tree")));
	/* check if where connected */
	if (check_connection_state ())
		return;
	/* see if there is a row selected */
	if (gtk_tree_selection_count_selected_rows (selection) > 0)
	{
		GList *list = NULL, *llist = NULL;
		/* grab the selected songs */
		list = gtk_tree_selection_get_selected_rows (selection, &model);
		/* grab the last song that is selected */
		llist = g_list_last (list);
		/* remove every selected song one by one */
		do
		{
			GtkTreeIter iter;
			gtk_tree_model_get_iter (model, &iter,(GtkTreePath *) llist->data);
			gtk_list_store_remove (pl3_store, &iter);

		}
		while ((llist = g_list_previous (llist)));

		/* free list */
		g_list_foreach (list, (GFunc) gtk_tree_path_free, NULL);
		g_list_free (list);

		check_for_errors ();
	}
	pl3_custom_stream_save_tree();
}

void pl3_custom_stream_row_activated(GtkTreeView *tree, GtkTreePath *tp)
{
//   pl3_browse_add_selected();
}

void pl3_custom_stream_category_selection_changed(GtkTreeView *tree,GtkTreeIter *iter)
{
	char *id;
	GtkTreeIter parent;
	GtkTreeSelection *selec = gtk_tree_view_get_selection((GtkTreeView *)glade_xml_get_widget (pl3_xml, "cat_tree"));
	gtk_tree_view_set_model(tree, GTK_TREE_MODEL(pl3_store));
	gtk_tree_model_get(GTK_TREE_MODEL(pl3_tree), iter,PL3_CAT_INT_ID , &id, -1);
	if(strlen(id) != 0)
	{
		pl3_custom_stream_add_stream(NULL,NULL);
		gtk_tree_model_iter_parent(GTK_TREE_MODEL(pl3_tree), &parent, iter);
		gtk_tree_selection_select_iter(selec, &parent);   					

	}
	else
	{	
		gtk_list_store_clear(pl3_store);
		pl3_custom_stream_view_browser();
		gtk_statusbar_push(GTK_STATUSBAR(glade_xml_get_widget(pl3_xml, "statusbar2")),0, "");
	}
}






































