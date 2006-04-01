#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <gdk/gdkkeysyms.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "TreeSearchWidget.h"
#include "main.h"
#include "plugin.h"

GladeXML *cam_pref_xml = NULL;
extern config_obj *cover_index;
extern int errno;

void cover_art_manager_close(GtkWidget *widget);
void cover_art_pref_construct(GtkWidget *container);
void cover_art_pref_destroy(GtkWidget *container);
void cover_art_manager_load_tree(GtkTreeStore *cam_ts);
void cover_art_manager_load_albums(GtkTreeView *tree, GtkTreeIter *iter);

gmpcPrefPlugin cover_art_gpp = {
	cover_art_pref_construct,
	cover_art_pref_destroy
};

gmpcPlugin cover_art_plug = {
	"Cover Art",
	{1,1,1},
	GMPC_INTERNALL,
	0,
	NULL,
	NULL,
	NULL,
	NULL,	
	NULL,	
	&cover_art_gpp
};

int cover_art_manager_key_release(GtkWidget *tree, GdkEventKey *event, GtkWidget *tree_search)
{
	if(event->keyval == GDK_f)
	{
		treesearch_start(TREESEARCH(tree_search));
		return TRUE;
	}
	else if (event->keyval == GDK_Escape)
	{
		cover_art_manager_close(tree);

	}
	return FALSE;
}


void cover_art_manager_close(GtkWidget *widget)
{
	GladeXML *camxml = glade_get_widget_tree(widget);
	if(camxml){
		gtk_widget_destroy(glade_xml_get_widget(camxml, "cam-win"));
		g_object_unref(camxml);
	}

}

void cover_art_manager_row_activated(GtkTreeView *tree, GtkTreePath *path)
{
	GtkTreeIter iter;
	if(gtk_tree_model_get_iter(gtk_tree_view_get_model(tree), &iter, path))
	{
		gint type;

		gtk_tree_model_get(gtk_tree_view_get_model(tree), &iter, 4, &type, -1);
		if(type == 1)
		{
			gchar *artist,*album;
			gtk_tree_model_get(gtk_tree_view_get_model(tree), &iter, 2,&artist, 3, &album, -1);
			if(artist && album)
			{
				gchar *path = NULL;
				GdkPixbuf *pb = NULL;
				if(cover_art_edit_cover(artist, album))
				{
					path = cfg_get_single_value_as_string(cover_index, artist, album);
					if(path){
						int size = cfg_get_single_value_as_int_with_default(config,
								"cover-art", "browser-size",64);
						pb =gdk_pixbuf_new_from_file_at_size(path,size,size,NULL);
					}
					gtk_tree_store_set(GTK_TREE_STORE(gtk_tree_view_get_model(tree)), &iter, 1, pb,-1);
					if(pb)g_object_unref(pb);
				}
			}
			g_free(artist);
			g_free(album);
		}
		else if (type == 0){
			if(gtk_tree_view_row_expanded(tree, path))
			{
				gtk_tree_view_collapse_row(tree, path);
			}
			else{
				gtk_tree_view_expand_row(tree, path,TRUE);
			}

		}
	}
}

void cover_art_manager_create()
{
	GtkTreeStore *cam_ts = NULL;
	GtkTreeViewColumn *column = NULL;
	GtkCellRenderer *renderer = NULL;
	GtkWidget *label = NULL;
	GtkWidget *tree_search = NULL;
	GladeXML *camxml = NULL;
	gchar *camp = gmpc_get_full_glade_path("gmpc.glade");
	camxml = glade_xml_new(camp, "cam-win",NULL);
	g_free(camp);
	if(!camp) return;
	/* create storage */
	cam_ts = gtk_tree_store_new(6,
			G_TYPE_STRING, /* name */
			GDK_TYPE_PIXBUF, /*cover art */
			G_TYPE_STRING, /*artist */
			G_TYPE_STRING, /*album */
			G_TYPE_INT, /* type of row */
			G_TYPE_BOOLEAN
			);
	/* set the model */
	gtk_tree_view_set_model(
			GTK_TREE_VIEW(glade_xml_get_widget(camxml, "camtree")),
			GTK_TREE_MODEL(cam_ts));
	gtk_tree_view_set_enable_search(GTK_TREE_VIEW(glade_xml_get_widget(camxml, "camtree")), FALSE);

	column = gtk_tree_view_column_new ();
	renderer = gtk_cell_renderer_pixbuf_new();
	gtk_tree_view_column_pack_start (column, renderer, FALSE);
	gtk_tree_view_column_set_attributes (column,renderer,"pixbuf",1, NULL);
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_set_attributes (column,renderer,"markup", 0, NULL);
	gtk_tree_view_append_column (
			GTK_TREE_VIEW (glade_xml_get_widget(camxml, "camtree")), column);
	gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_column_set_spacing(column,6);

	gtk_tree_view_column_set_sort_column_id (column, 0);


	tree_search = treesearch_new(GTK_TREE_VIEW(glade_xml_get_widget(camxml, "camtree")),0);
	gtk_box_pack_end(GTK_BOX(glade_xml_get_widget(camxml, "cam-vbox")), tree_search, FALSE, TRUE, 0);

	label = glade_xml_get_widget(camxml, "title_label_box");
	gtk_widget_modify_bg(label, GTK_STATE_NORMAL, &label->style->bg[GTK_STATE_SELECTED]);
	label = glade_xml_get_widget(camxml, "title_label");
	gtk_widget_modify_fg(label, GTK_STATE_NORMAL, &label->style->fg[GTK_STATE_SELECTED]);

	cover_art_manager_load_tree(cam_ts);
	glade_xml_signal_autoconnect(camxml);
	g_signal_connect(G_OBJECT(glade_xml_get_widget(camxml, "camtree")), "key-press-event", G_CALLBACK(cover_art_manager_key_release),tree_search);
	g_signal_connect(G_OBJECT(glade_xml_get_widget(camxml, "camtree")), "row-activated", G_CALLBACK(cover_art_manager_row_activated),NULL);
	g_signal_connect(G_OBJECT(glade_xml_get_widget(camxml, "camtree")), "row-expanded", G_CALLBACK(cover_art_manager_load_albums), NULL);
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(cam_ts), 0, GTK_SORT_ASCENDING);
}



void cover_art_manager_load_albums(GtkTreeView *tree, GtkTreeIter *iter)
{
	GtkTreeIter child;
	GtkTreeStore *cam_ts = (GtkTreeStore *)gtk_tree_view_get_model(tree);
	gchar *artist;
	int checked;
	gtk_tree_model_get(GTK_TREE_MODEL(cam_ts), iter, 5,&checked, 2, &artist, -1);
	if(checked){
		g_free(artist);      
		return;
	}
	if(gtk_tree_model_iter_children(GTK_TREE_MODEL(cam_ts), &child, iter))
	{
		int used = 0;
		conf_mult_obj *mult2 = cfg_get_key_list(cover_index, artist);


		if(mult2)
		{
			while(mult2 != NULL){
				gchar *string = g_markup_printf_escaped("<i>%s</i>",mult2->key);
				int size = cfg_get_single_value_as_int_with_default(config,
						"cover-art", "browser-size",64);

				GdkPixbuf *pb = NULL;
				if(mult2->value != NULL)
				{
					pb =gdk_pixbuf_new_from_file_at_size(mult2->value,size,size,NULL);
				}
				if(used) gtk_tree_store_append(cam_ts, &child,iter);
				gtk_tree_store_set(cam_ts, &child,
						0, string,/*name*/
						1, pb, /* No Image */
						2, artist, /* artist*/
						3, mult2->key, /*album */
						4, 1,
						-1);
				if(pb)g_object_unref(pb);
				g_free(string);
				if(mult2->next == NULL){
					cfg_free_multiple(mult2);
					mult2 = NULL;
				}
				else{
					mult2 = mult2->next;
				}
				used = 1;
			}
		}
		if(!used) gtk_tree_store_remove(cam_ts, &child);
	}
	gtk_tree_store_set(GTK_TREE_STORE(cam_ts), iter, 5,TRUE, -1);
	g_free(artist);      

}
void cover_art_manager_load_tree(GtkTreeStore *cam_ts) 
{
	if(cover_index){
		conf_mult_obj *mult = cfg_get_class_list(cover_index);
		while(mult){
			GtkTreeIter iter,child;
			gchar *string = g_markup_printf_escaped("<b>%s</b>",mult->key);

			gtk_tree_store_append(cam_ts, &iter, NULL);
			gtk_tree_store_set(cam_ts, &iter,
					0, string,/*name*/
					1, NULL, /* No Image */
					2, mult->key, /* artist*/
					3, NULL, /*album */
					4, 0,
					5, FALSE, /*if processed */
					-1);
			/* add zomby */
			gtk_tree_store_append(cam_ts, &child,&iter);
			g_free(string);

			if(mult->next == NULL)
			{
				cfg_free_multiple(mult);
				mult = NULL;
			}
			else{
				mult = mult->next;
			}
		}
	}
}




void cover_art_pref_destroy(GtkWidget *container)
{
	if(cam_pref_xml)
	{
		GtkWidget *vbox = glade_xml_get_widget(cam_pref_xml, "cam-vbox");
		gtk_container_remove(GTK_CONTAINER(container),vbox);
		g_object_unref(cam_pref_xml);
		cam_pref_xml = NULL;
	}
}
void cover_art_pref_toggle_enable(GtkToggleButton *tog)
{
	cfg_set_single_value_as_int(config, "cover-art", "enable", gtk_toggle_button_get_active(tog));
}

void cover_art_cover_manager(GtkButton *but)
{
	cover_art_manager_create();
}

void cover_art_clear_cache(GtkButton *but)
{
	if(cover_index){
		gchar *url = g_strdup_printf("%s/.covers/covers.db", g_get_home_dir());
		/* close the cache */
		cfg_close(cover_index);
		/* remove th file */
		if(g_file_test(url, G_FILE_TEST_EXISTS))
		{
			if(unlink(url) < 0){
				debug_printf(DEBUG_ERROR, "Failed to remove cover art cache: %s\n",
						strerror(errno));
			}
		}
		cover_index = cfg_open(url);
		g_free(url);
	}
}


void cover_art_pref_construct(GtkWidget *container)
{
	gchar *path = gmpc_get_full_glade_path("gmpc.glade");
	cam_pref_xml = glade_xml_new(path, "cam-vbox",NULL);
	g_free(path);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(glade_xml_get_widget(cam_pref_xml, "tb-enable-cam")), 
			cfg_get_single_value_as_int_with_default(config,"cover-art", "enable", TRUE));

	gtk_container_add(GTK_CONTAINER(container),glade_xml_get_widget(cam_pref_xml, "cam-vbox"));
	glade_xml_signal_autoconnect(cam_pref_xml);
}

void cover_art_remove_image(GtkWidget *button){
	GladeXML *cae_xml = glade_get_widget_tree(button);
	gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(glade_xml_get_widget(cae_xml, "filechooser_location")),"");
	gtk_image_set_from_stock(GTK_IMAGE(glade_xml_get_widget(cae_xml, "cover_image")),GTK_STOCK_DIALOG_QUESTION, GTK_ICON_SIZE_DIALOG);
}
void cover_art_edit_path_changed(GtkWidget *filechooser)
{
	GladeXML *cae_xml = glade_get_widget_tree(filechooser);
	gchar * path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(glade_xml_get_widget(cae_xml, "filechooser_location")));
	if(path){
		GdkPixbuf *pb = gdk_pixbuf_new_from_file_at_size(path, 250,250,NULL);
		if(pb)
		{
			gtk_image_set_from_pixbuf(GTK_IMAGE(glade_xml_get_widget(cae_xml, "cover_image")), pb);
			g_object_unref(pb);                                                                    		
		}         
	}
	else{
		gtk_image_set_from_stock(GTK_IMAGE(glade_xml_get_widget(cae_xml, "cover_image")),GTK_STOCK_DIALOG_QUESTION, GTK_ICON_SIZE_DIALOG);
	}

}

void cover_art_query_providers(GtkWidget *button)
{
	GladeXML *cae_xml = glade_get_widget_tree(button);
	gchar *path = NULL;
	const gchar *artist = 	gtk_entry_get_text(GTK_ENTRY(glade_xml_get_widget(cae_xml, "entry_artist")));
	const gchar *album = 		gtk_entry_get_text(GTK_ENTRY(glade_xml_get_widget(cae_xml, "entry_album")));
	int result = cover_art_fetch_image_path_aa_no_cache((char *)artist, (char *)album, &path);

	if(result == COVER_ART_OK_LOCAL)
	{
		if(path){
			GdkPixbuf *pb = gdk_pixbuf_new_from_file_at_size(path, 250,250,NULL);
			if(pb)
			{
				gtk_image_set_from_pixbuf(GTK_IMAGE(glade_xml_get_widget(cae_xml, "cover_image")), pb);
				g_object_unref(pb);                                                                    		
			}         
		}
		else{
			gtk_image_set_from_stock(GTK_IMAGE(glade_xml_get_widget(cae_xml, "cover_image")),GTK_STOCK_DIALOG_QUESTION, GTK_ICON_SIZE_DIALOG);
		}
	}
	if(path) g_free(path);
	
}

int cover_art_edit_cover(gchar *artist, gchar *album)
{
	gchar *path = gmpc_get_full_glade_path("gmpc.glade");
#if GTK_CHECK_VERSION(2,6,0)
	GtkFileFilter *filter = NULL;
#endif
	GladeXML *cae_xml = glade_xml_new(path, "cover-art-edit",NULL);
	g_free(path);

	gtk_entry_set_text(GTK_ENTRY(glade_xml_get_widget(cae_xml, "entry_artist")), artist);
	gtk_entry_set_text(GTK_ENTRY(glade_xml_get_widget(cae_xml, "entry_album")), album);
	if(artist && album) {
		path = cfg_get_single_value_as_string(cover_index, artist, album);
		if(path){
			GdkPixbuf *pb = NULL;
			gtk_file_chooser_select_filename(GTK_FILE_CHOOSER(glade_xml_get_widget(cae_xml, "filechooser_location")),
					path);
			pb = gdk_pixbuf_new_from_file_at_size(path, 250,250,NULL);
			if(pb)
			{
				gtk_image_set_from_pixbuf(GTK_IMAGE(glade_xml_get_widget(cae_xml, "cover_image")), pb);
				g_object_unref(pb);
			}
		}
	}

#if GTK_CHECK_VERSION(2,6,0)
	filter = gtk_file_filter_new();
	gtk_file_filter_add_pixbuf_formats(filter);
	gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(glade_xml_get_widget(cae_xml, "filechooser_location")),
			filter);

#endif


	glade_xml_signal_autoconnect(cae_xml);

	gtk_widget_show_all(glade_xml_get_widget(cae_xml, "cover-art-edit"));
	switch(gtk_dialog_run(GTK_DIALOG(glade_xml_get_widget(cae_xml, "cover-art-edit"))))
	{
		case GTK_RESPONSE_OK:
			{
				gchar * path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(glade_xml_get_widget(cae_xml, "filechooser_location")));
				const gchar *artist =  gtk_entry_get_text(GTK_ENTRY(glade_xml_get_widget(cae_xml, "entry_artist")));
				const gchar *album = gtk_entry_get_text(GTK_ENTRY(glade_xml_get_widget(cae_xml, "entry_album")));
				if(artist != NULL && album != NULL)
				{
					if(path){
						cfg_set_single_value_as_string(cover_index,(char *) artist,(char *)album,path);
					}else{
						cfg_set_single_value_as_string(cover_index,(char *)artist,(char *)album,"");
					}
				}
				gtk_widget_destroy(glade_xml_get_widget(cae_xml, "cover-art-edit"));
				g_object_unref(cae_xml);
				return TRUE;
			}
		default:
			break;
	}
	gtk_widget_destroy(glade_xml_get_widget(cae_xml, "cover-art-edit"));
	g_object_unref(cae_xml);
	return FALSE;
}

