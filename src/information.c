#include <stdio.h>
#include <string.h>
#include <glade/glade.h>
#include "plugin.h"
#include "cover-art.h"

GtkWidget *info_pref_vbox = NULL;
void info_add(GtkWidget *cat_tree);
void info_selected(GtkWidget *container);
void info_unselected(GtkWidget *container);
void info_changed(GtkWidget *tree, GtkTreeIter *iter);

GtkWidget *moz = NULL;
GtkWidget *info_vbox = NULL;

void info_construct(GtkWidget *container);
void info_destroy(GtkWidget *container);


extern GladeXML *pl3_xml;

gmpcPrefPlugin info_gpp = {
	info_construct,
	info_destroy
};

/* Needed plugin_wp stuff */
gmpcPlBrowserPlugin info_gbp = {
	info_add,
	info_selected,
	info_unselected,
	info_changed,
	NULL,
	NULL,
	NULL
};

int plugin_api_version = PLUGIN_API_VERSION;

gmpcPlugin info_plugin = {
	"Info",
	{0,0,2},
	GMPC_PLUGIN_PL_BROWSER,
	0,
	NULL,
	NULL,
	&info_gbp,
	NULL,
	NULL,
	&info_gpp,
	NULL
};

/* save the tree location */
GtkTreeRowReference *tree_ref = NULL;


void info_show_song(mpd_Song *song);

static void info_show_current_song() {
	mpd_Song *song = mpd_playlist_get_current_song(connection);
	if(song){
		info_show_song(song);
	}
}

void info_changed(GtkWidget *tree, GtkTreeIter *iter){


}

/** FUNCTIONS FOR GENERATING HTML **/

/* getting cover images
 * We use gmpc cover art support now 
 */

static gchar * info_get_cover_image_url(mpd_Song *song)
{
	gchar *path = NULL;
	int retv  = 0;
	retv = cover_art_fetch_image_path(song, &path);
	if(retv == COVER_ART_OK_LOCAL) return path;
	if(path) g_free(path);
	return NULL;
}

static void pl3_pixbuf_border(GdkPixbuf *pb)
{
	int x,y,width,height;
	int pixel;
	int n_channels = gdk_pixbuf_get_n_channels(pb);
	int rowstride = gdk_pixbuf_get_rowstride(pb);	
	guchar *pixels;
	width = gdk_pixbuf_get_width (pb);
	height = gdk_pixbuf_get_height (pb);
	pixels = gdk_pixbuf_get_pixels(pb);

	for(y=0;y<height;y++)
	{
		for(x=0;x<width;x++)
		{
			if(y == 0 || y == (height-1) || x == 0 || x == (width-1))
			{
				for(pixel=0; pixel < n_channels;pixel++)
				{
					pixels[x*n_channels+y*rowstride+pixel] = 0;
				}
			}
		}
	}
}

void info_show_song(mpd_Song *song)
{
	MpdData *data = NULL;
	GtkTextIter iter, start, stop;
	gchar *string = NULL;
	GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(moz));
	gtk_text_buffer_get_start_iter(buffer, &start);
	gtk_text_buffer_get_end_iter(buffer, &stop);
	gtk_text_buffer_delete(buffer,&start,&stop);

	if(song->title == NULL || song->artist == NULL) return;
	gtk_text_buffer_get_start_iter(buffer, &iter);

	if(song->title)
	{
		string = g_strdup_printf("%s\n", song->title);

		gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, string, -1,"title","bold",NULL);
		g_free(string);
	}
	if(song->artist)
	{
		if(song->album)
		{
			string = g_strdup_printf(" %s - %s\n", song->artist, song->album);
		}
		else
		{
			string = g_strdup_printf(" %s\n", song->artist);
		}
		gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, string, -1,"artist",NULL);
		g_free(string);                                                                  	
	}
	gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "\n", -1, "bar", NULL);
	gtk_text_buffer_insert(buffer, &iter, "\n", -1);
	string = info_get_cover_image_url(song);
	if(string)
	{
		GdkPixbuf *pb = gdk_pixbuf_new_from_file_at_size(string,164,164,NULL);
		if(pb)
		{
			pl3_pixbuf_border(pb);

			gtk_text_buffer_insert_pixbuf(buffer, &iter, pb);	
			gtk_text_buffer_get_end_iter(buffer, &start);
			gtk_text_iter_backward_char(&start);
			gtk_text_buffer_get_end_iter(buffer, &stop);
			gtk_text_buffer_apply_tag_by_name(buffer, "item-value", &start, &stop);
			gtk_text_buffer_insert(buffer, &iter, "\n\n", -1);
			g_object_unref(pb);
		}
		g_free(string);
	}
	if(song->album)
	{
		gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "Album: ", -1,"item","bold",NULL);
		gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, song->album, -1,"item-value",NULL);
		gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "\n", -1,"item-value",NULL);
	}
	if(song->genre)
	{
		gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "Genre: ", -1,"item","bold",NULL);
		gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, song->genre, -1,"item-value",NULL);
		gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "\n", -1,"item-value",NULL);
	}
	if(song->date)
	{
		gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "Date: ", -1,"item","bold",NULL);
		gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, song->date, -1,"item-value",NULL);
		gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "\n", -1,"item-value",NULL);
	}
	if(song->composer)
	{
		gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "Composer: ", -1,"item","bold",NULL);
		gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, song->composer, -1,"item-value",NULL);
		gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "\n", -1,"item-value",NULL);
	}
	if(song->track)
	{
		gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "Track: ", -1,"item","bold",NULL);
		gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, song->track, -1,"item-value",NULL);
		gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "\n", -1,"item-value",NULL);
	}
	if(song->time)
	{
		string = g_strdup_printf("%02i:%02i", (song->time)/60, (song->time)%60);
		gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "Length: ", -1,"item","bold",NULL);
		gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, string, -1,"item-value",NULL);
		gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "\n", -1,"item-value",NULL);
		g_free(string);
	}
	if(song->artist && song->album && mpd_server_check_version(connection, 0,12,0))
	{
		data = mpd_database_find_adv(connection,TRUE, 
				MPD_TAG_ITEM_ARTIST, song->artist,
				MPD_TAG_ITEM_ALBUM, song->album, 
				-1);
		if(data)
		{
			gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "\nAlbum Tracklist:\n", -1,"item","bold",NULL);
			for(;data != NULL; data = mpd_data_get_next(data))
			{
				mpd_Song *song = data->song;
				if(song->track) {
					gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, song->track, -1,"item-value",NULL);
					gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, ": ", -1,"item-value",NULL);
				}
				if(song->title)
				{
					gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, song->title, -1,"item-value",NULL);
				}
				gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "\n", -1,"item-value",NULL);
			}
		}

	}	
	if(song->artist && song->album )
	{
		data = mpd_database_get_albums(connection, song->artist);
		if(data)
		{
			int albums = 0;
			
			for(;data != NULL; data = mpd_data_get_next(data))
			{
				char *falbum = data->tag;
				if(strcmp(song->album,falbum)) {
					char *path = NULL;
					GdkPixbuf *pb = NULL;
					albums++;
					if(albums == 1)
					{
						gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "\nOther Albums:\n", -1,"item","bold",NULL);
					}
					int ret = cover_art_fetch_image_path_aa(song->artist, falbum, &path);
					if(ret == COVER_ART_OK_LOCAL)
					{
						pb= gdk_pixbuf_new_from_file_at_size(path,64,64,NULL);
					}
					if(!pb)
					{
						pb = gtk_widget_render_icon(moz, "media-no-cover" , -1, NULL);
						if(pb == NULL)
						{
							printf("icon theme icon not found\n");
						}
					}
					if(pb)
					{
						pl3_pixbuf_border(pb);
						gtk_text_buffer_insert_pixbuf(buffer, &iter, pb);

						gtk_text_buffer_get_end_iter(buffer, &start);
						gtk_text_iter_backward_char(&start);
						gtk_text_buffer_get_end_iter(buffer, &stop);
						gtk_text_buffer_apply_tag_by_name(buffer, "small-album", &start, &stop);

						g_object_unref(pb);
					}


					if(path)g_free(path);
					gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "  ", -1,"album-value",NULL);
					gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, falbum, -1,"album-value",NULL);
					
					gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "\n", -1,"album-value",NULL);
				}
				
			}
		}

	}	
}

static void info_init()
{
	GtkTextTag *tag = NULL;
	GtkTextBuffer *buffer = NULL;
	GtkWidget *tmp = NULL,*hbox = NULL,*label = NULL;
	GtkWidget *button = NULL;
	GtkWidget *sw =gtk_scrolled_window_new(NULL, NULL);
	info_vbox = gtk_vbox_new(FALSE, 6);
	/* Mozilla Browser widget */
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw), GTK_SHADOW_ETCHED_IN);
	moz = gtk_text_view_new();
	gtk_container_set_border_width(GTK_CONTAINER(sw), 6);
	gtk_container_add(GTK_CONTAINER(sw), moz);
	gtk_box_pack_start_defaults(GTK_BOX(info_vbox), sw);
	/* Update button */
	hbox = gtk_hbox_new(FALSE, 6);
	tmp = gtk_alignment_new(0,0.5,0,1);
	button = gtk_button_new_with_mnemonic("_Load current song");
	g_signal_connect(G_OBJECT(button),"clicked", G_CALLBACK(info_show_current_song),NULL);
	gtk_container_add(GTK_CONTAINER(tmp), button);
	gtk_box_pack_start(GTK_BOX(info_vbox), hbox, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, TRUE, 0);


	label = gtk_label_new("");
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 0);

	gtk_text_view_set_editable(GTK_TEXT_VIEW(moz), FALSE);

	/* Create some default "tags" */
	buffer = gtk_text_buffer_new(NULL);
	gtk_text_view_set_buffer(GTK_TEXT_VIEW(moz),buffer);
	if(buffer == NULL) printf("help\n");

	tag = gtk_text_buffer_create_tag(buffer, "bold", 
			"weight", PANGO_WEIGHT_BOLD,
			NULL);               


	tag = gtk_text_buffer_create_tag(buffer, "title", 
			"size-points", (gdouble)24.0,
			"paragraph-background-gdk", &(info_vbox->style->bg[GTK_STATE_SELECTED]),
			"foreground-gdk", &(info_vbox->style->fg[GTK_STATE_SELECTED]),
			NULL);

	tag = gtk_text_buffer_create_tag(buffer, "artist", 
			"size-points", (gdouble)14.0,
			"paragraph-background-gdk",&(info_vbox->style->bg[GTK_STATE_SELECTED]),
			"foreground-gdk",&(info_vbox->style->fg[GTK_STATE_SELECTED]),
			NULL);                            	
	tag = gtk_text_buffer_create_tag(buffer, "bar", 
			"size-points", (gdouble)3.0,
			"paragraph-background-gdk",&(info_vbox->style->fg[GTK_STATE_NORMAL]),
			NULL);                            	
	tag = gtk_text_buffer_create_tag(buffer, "album-value", 
			"rise", 26*PANGO_SCALE,
			"size-points", (gdouble)10.0,			
			NULL);                            	

	tag = gtk_text_buffer_create_tag(buffer, "item", 
			"size-points", (gdouble)10.0,			
			"left-margin", 6,
			NULL);               
	tag = gtk_text_buffer_create_tag(buffer, "item-value", 
			"size-points", (gdouble)10.0,			
			"left-margin", 6,
			NULL);              

	tag = gtk_text_buffer_create_tag(buffer, "small-album", 
			"size-points", (gdouble)10.0,			
			"left-margin", 6,
			"pixels-below-lines", 6,
			NULL);              



	gtk_widget_show_all(info_vbox);
	g_object_ref(G_OBJECT(info_vbox));

}

void info_add(GtkWidget *cat_tree)
{
	GtkTreePath *path;
	GtkTreeStore *pl3_tree = (GtkTreeStore *)gtk_tree_view_get_model(GTK_TREE_VIEW(cat_tree));
	GtkTreeIter iter;
	if(!cfg_get_single_value_as_int_with_default(config, "information", "enable", TRUE)) return;
	printf("adding plugin_wp: %i '%s'\n", info_plugin.id, info_plugin.name);
	gtk_tree_store_append(pl3_tree, &iter, NULL);
	gtk_tree_store_set(pl3_tree, &iter,
			PL3_CAT_TYPE, info_plugin.id,
			PL3_CAT_TITLE, "Song Information",
			PL3_CAT_INT_ID, "/",
			PL3_CAT_ICON_ID, "gtk-info",
			PL3_CAT_PROC, TRUE,
			PL3_CAT_ICON_SIZE,GTK_ICON_SIZE_DND,-1);

	if(tree_ref)
	{
		gtk_tree_row_reference_free(tree_ref);
		tree_ref = NULL;
	}
	path = gtk_tree_model_get_path(GTK_TREE_MODEL(pl3_tree), &iter);
	if(path)
	{
		tree_ref = gtk_tree_row_reference_new(GTK_TREE_MODEL(pl3_tree),path);
		gtk_tree_path_free(path);
	}


}

void info_selected(GtkWidget *container)
{

	if(info_vbox== NULL)
	{
		info_init();
	}

	gtk_container_add(GTK_CONTAINER(container), info_vbox);
	gtk_widget_show_all(info_vbox);
	while (gtk_events_pending ())
		gtk_main_iteration ();

}

void info_unselected(GtkWidget *container)
{
	gtk_widget_hide(moz);
	gtk_container_remove(GTK_CONTAINER(container),info_vbox);
}

static void info_enable_toggle(GtkWidget *wid)
{
	int kk = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(wid));
	cfg_set_single_value_as_int(config, "information", "enable", kk);
	/* if the user prefers not to show the plugin, and we are in the list,
	 * remove it from the list */
	if(pl3_xml == NULL) return;
	if(kk == FALSE && tree_ref != NULL)
	{
		GtkTreePath *path = gtk_tree_row_reference_get_path(tree_ref);
		GtkTreeIter iter;
		if(path)
		{
			if(gtk_tree_model_get_iter(gtk_tree_row_reference_get_model(tree_ref),
						&iter,
						path))
			{
				gtk_tree_store_remove(GTK_TREE_STORE(
							gtk_tree_row_reference_get_model(tree_ref)),
						&iter);
			}
			gtk_tree_path_free(path);
		}
		gtk_tree_row_reference_free(tree_ref);
		tree_ref = NULL;
	}
	else if (kk == TRUE && tree_ref == NULL)
	{
		GtkWidget *cat_tree = glade_xml_get_widget(pl3_xml, "cat_tree");
		info_add(cat_tree);
	}
}
void info_destroy(GtkWidget *container)
{
	gtk_container_remove(GTK_CONTAINER(container), info_pref_vbox);
}


void info_construct(GtkWidget *container)
{
	GtkWidget *enable_cg = gtk_check_button_new_with_mnemonic("_Enable Info");
	info_pref_vbox = gtk_vbox_new(FALSE,6);

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(enable_cg),
			cfg_get_single_value_as_int_with_default(config, "information", "enable", TRUE));

	g_signal_connect(G_OBJECT(enable_cg), "toggled", G_CALLBACK(info_enable_toggle), NULL);
	gtk_box_pack_start(GTK_BOX(info_pref_vbox), enable_cg, FALSE, FALSE, 0);
	gtk_container_add(GTK_CONTAINER(container), info_pref_vbox);

	gtk_widget_show_all(container);
}
