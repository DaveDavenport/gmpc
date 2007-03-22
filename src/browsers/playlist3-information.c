#include <gdk/gdkkeysyms.h>
#include <stdio.h>
#include <string.h>
#include <regex.h>
#include <math.h>
#include "main.h"
#include "misc.h"
#include "gmpc-clicklabel.h"

/**
 * TODO; Move to header file 
 */

extern GladeXML *pl3_xml;

static void info3_add(GtkWidget *);
static void info3_selected(GtkWidget *);
static void info3_unselected(GtkWidget *);
static int info3_add_go_menu(GtkWidget *);
static int info3_get_enabled(void);
static void info3_set_enabled(int);
static void remove_container_entries(GtkContainer *);
static void info3_fill_view(void);
static int info3_key_press_event(GtkWidget *mw, GdkEventKey *event, int type);
static GtkWidget *resizer_vbox= NULL;
static GtkWidget *info3_vbox = NULL,*title_vbox=NULL;
static GtkWidget *title_event=NULL;
static GtkWidget *scrolled_window = NULL;
static void info3_show_artist(GtkWidget *button, gpointer data);
static void pl3_info_browser_song_play(GtkButton *but, gpointer data);
static void info3_show_album(GtkWidget *button, gpointer data);
static int current_id = 0;

typedef struct {
	GtkWidget *widget;
	gint id;
	gchar *name;
}PassData;

/* Needed plugin_wp stuff */
gmpcPlBrowserPlugin info3_gbp = {
	info3_add,		/** add */
	info3_selected,		/** selected */
	info3_unselected,	/** unselected */
	NULL,			/** changed */
	NULL,			/** row expand */
	NULL,			/** cat right mouse menu */ 
	NULL,			/** cat key press */
	info3_add_go_menu,	/** add go menu */
	info3_key_press_event	/** key press event */		
};
gmpcPlugin info3_plugin = {
	"Song Information",
	{0,0,1},
	GMPC_PLUGIN_PL_BROWSER,
	0,
	NULL, /* path */
	NULL, /*init */
        NULL, /* Destroy */
	&info3_gbp,
	NULL, /* status changed */
	NULL, /* connection changed */
	NULL,
	NULL, /* cover */
	info3_get_enabled,
	info3_set_enabled
};


/* Playlist window row reference */
static GtkTreeRowReference *info3_ref = NULL;


static void pl3_info_browser_song_play(GtkButton *but, gpointer data)
{
	gchar *path = g_object_get_data(G_OBJECT(but), "path");
	play_path(path);
}

static void remove_container_entries (GtkContainer * widget)
{
	GList *children, *node; 
	children = node = gtk_container_get_children (widget);
	if (children == NULL)
		return;

	do {
		GtkWidget *child = GTK_WIDGET (children->data);
		gtk_container_remove (GTK_CONTAINER (widget), GTK_WIDGET (child));
	} while (NULL != (children = g_list_next (children)));
	g_list_free(node);
}

static void info3_widget_clear_children(GtkWidget *wid)
{
	GList *list, *node;
	/**
	 * Remove all the remaining widgets in the view
	 */
	list = gtk_container_get_children(GTK_CONTAINER(wid));
	if(list)
	{
		for(node = g_list_first(list);
				node;
				node = g_list_next(node))
		{
			gtk_widget_destroy(node->data);	
		}
		g_list_free(list);
	}
}

static void info3_prepare_view()
{
	GtkAdjustment *h = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(scrolled_window));
	info3_widget_clear_children(resizer_vbox);
	gtk_adjustment_set_value(h, 0.0);

	/**
	 *  new id
	 */
	current_id = g_random_int();
}

static void info3_cover_txt_fetched(mpd_Song *song,MetaDataResult ret, char *path,PassData *pd)
{
	GtkWidget *vbox= pd->widget;
	/*GtkWidget *ali = NULL;*/
	if(pd->id != current_id)
	{
		if(ret != META_DATA_FETCHING)q_free(pd);
		return;
	}
	if(ret == META_DATA_AVAILABLE)
	{
		gsize size;
		char *content = NULL;
		GtkWidget *expander = NULL;
		GtkWidget *label = NULL;
		gchar *labstr= g_strdup_printf(_("<b>%s:</b>"), pd->name);
		remove_container_entries(GTK_CONTAINER(vbox));

		expander = gtk_expander_new(labstr);	
		gtk_expander_set_use_markup(GTK_EXPANDER(expander), TRUE);
		gtk_box_pack_start(GTK_BOX(vbox), expander, FALSE, FALSE, 0);		
		q_free(labstr);
		if(cfg_get_single_value_as_int_with_default(config, "Song Information", "auto-expand-lyric", FALSE))
		{
			gtk_expander_set_expanded(GTK_EXPANDER(expander), TRUE);
		}

		label = gtk_label_new("");
		/*		ali = gtk_alignment_new(0,0.5,0,0);*/
		gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);		
		/*		gtk_alignment_set_padding(GTK_ALIGNMENT(ali),0,0,6,0);*/
		/*		gtk_container_add(GTK_CONTAINER(ali), label);*/
		gtk_misc_set_alignment(GTK_MISC(label),0,0.5);
		gtk_misc_set_padding(GTK_MISC(label),6,6);
		gtk_container_add(GTK_CONTAINER(expander), label);		
//		gtk_box_pack_start(GTK_BOX(vbox), label,FALSE,TRUE,0);
		g_file_get_contents(path, &content, &size,NULL);
		gtk_label_set_text(GTK_LABEL(label), content);
		gtk_label_set_selectable(GTK_LABEL(label), TRUE);
		gtk_widget_show_all(vbox);
		q_free(content);
	}
	else if(ret == META_DATA_UNAVAILABLE)
	{
		remove_container_entries(GTK_CONTAINER(vbox));
	}
	else if(ret == META_DATA_FETCHING)
	{
		GtkWidget *label = NULL;
		gchar *labstr= g_strdup_printf(_("<i>Fetching %s</i>"),pd->name);
		remove_container_entries(GTK_CONTAINER(vbox));
		label = gtk_label_new("");
		gtk_misc_set_alignment(GTK_MISC(label),0,0.5);
		gtk_label_set_markup(GTK_LABEL(label),labstr); 
		gtk_container_add(GTK_CONTAINER(vbox),label);
		gtk_widget_show_all(vbox);
		q_free(labstr);
	}

	if(ret != META_DATA_FETCHING){
		if(pd->name) q_free(pd->name);
		q_free(pd);
	}
}
static void info3_add_table_item(GtkWidget *table,char *name, char *value, int i)
{
	GtkWidget *label/*, *ali*/;
	label = gtk_label_new("");
	gtk_label_set_markup(GTK_LABEL(label), name);
	gtk_misc_set_alignment(GTK_MISC(label),0,0.5);
	/*	ali = gtk_alignment_new(0,0.5,0,0);

		gtk_container_add(GTK_CONTAINER(ali), label);
		*/
	gtk_table_attach(GTK_TABLE(table), label,0,1,i,i+1,GTK_SHRINK|GTK_FILL, GTK_SHRINK|GTK_FILL,0,0);
	label = gtk_label_new(value);
	gtk_misc_set_alignment(GTK_MISC(label), 0,0.5);
	/*ali = gtk_alignment_new(0,0.5,1,0);*/
	gtk_label_set_ellipsize(GTK_LABEL(label), PANGO_ELLIPSIZE_END);
	gtk_label_set_selectable(GTK_LABEL(label), TRUE);
	/*gtk_container_add(GTK_CONTAINER(ali), label);*/
	gtk_table_attach(GTK_TABLE(table), label,1,2,i,i+1,GTK_EXPAND|GTK_FILL, GTK_SHRINK|GTK_FILL,0,0);
	gtk_label_set_ellipsize(GTK_LABEL(label), PANGO_ELLIPSIZE_END);
}


/** 
 * Song View 
 */
static void info3_fill_view() 
{
	GtkWidget *table2 	= NULL;
	GtkWidget *table 	= NULL;
	GtkWidget *image	= NULL;
	GtkWidget *ali 		= NULL;	
	GtkWidget *vbox		= NULL;
	GtkWidget *hbox		= NULL;
	mpd_Song *song 		= NULL;
	GtkWidget *label 	= NULL;
	PassData *pd 		= NULL;
	char *markup 		= NULL;
	int state 		= mpd_player_get_state(connection);
	/** 
	 * Clear the view
	 */


	info3_prepare_view();
	/* remove the title */
	remove_container_entries(GTK_CONTAINER(title_vbox));

	song = mpd_playlist_get_current_song(connection);
	if(song == NULL || state == MPD_PLAYER_STOP)
	{
		/* Set label */
		label = gtk_label_new("");
		markup = g_markup_printf_escaped("<span size='xx-large' weight='bold'>%s - %s</span>", _("Song Information"),_("Not Playing"));
		gtk_label_set_markup(GTK_LABEL(label),markup);
		gtk_misc_set_alignment(GTK_MISC(label), 0,0.5);
		q_free(markup);
		gtk_box_pack_start(GTK_BOX(title_vbox), label,FALSE,TRUE,0);	

		gtk_widget_show_all(title_vbox);

		return;
	}

	/** 
	 * Title Label
	 */
	/* Set label */
	label = gtk_label_new("");
	if(song->title) {

		markup = g_markup_printf_escaped("<span size='xx-large' weight='bold'>%s - %s</span>", _("Song Information"),song->title);
	} else {
		/* Set label */
		gchar *filename = g_path_get_basename(song->file);
		markup = g_markup_printf_escaped("<span size='xx-large' weight='bold'>%s - %s</span>", _("Song Information"),filename);
		q_free(filename);
	}
	gtk_label_set_markup(GTK_LABEL(label),markup);
	gtk_label_set_ellipsize(GTK_LABEL(label), PANGO_ELLIPSIZE_END);
	gtk_misc_set_alignment(GTK_MISC(label), 0,0.5);
	q_free(markup);

	gtk_box_pack_start(GTK_BOX(title_vbox), label,TRUE,TRUE,0);	

	gtk_widget_show_all(title_vbox);


	/**
	 * Set album image
	 */
	table= gtk_table_new(2,2,FALSE);
	ali = gtk_alignment_new(0,0.5,1,0);
	gtk_container_set_border_width(GTK_CONTAINER(ali), 8);


	image = gmpc_metaimage_new(META_ALBUM_ART);
	gmpc_metaimage_set_size(GMPC_METAIMAGE(image), 200);
	gmpc_metaimage_set_draw_shadow(GMPC_METAIMAGE(image), TRUE);
	gmpc_metaimage_update_cover_from_song(GMPC_METAIMAGE(image), song);


	gtk_table_attach(GTK_TABLE(table), image, 0,1,0,2,GTK_SHRINK|GTK_FILL, GTK_SHRINK|GTK_FILL,0,0);


	/** pack the table and add to view */
	gtk_container_add(GTK_CONTAINER(ali), table);
	gtk_box_pack_start(GTK_BOX(resizer_vbox), ali, FALSE, FALSE,0);


	/*
	 * Make a table for the album info
	 */
	table2 = gtk_table_new(2,2,0);
	gtk_container_set_border_width(GTK_CONTAINER(table2), 8);

	gtk_table_set_col_spacings(GTK_TABLE(table2), 6);

	gtk_table_attach(GTK_TABLE(table), table2, 1,2,0,1,GTK_EXPAND|GTK_FILL, GTK_EXPAND|GTK_FILL,0,0);

	int i =0;
	if(song->artist) {
		label = gtk_label_new("");
		gtk_label_set_markup(GTK_LABEL(label), _("<b>Artist:</b>"));
		gtk_misc_set_alignment(GTK_MISC(label),0,0.5);
		gtk_table_attach(GTK_TABLE(table2), label,0,1,i,i+1,GTK_SHRINK|GTK_FILL, GTK_SHRINK|GTK_FILL,0,0);
		label = gmpc_clicklabel_new(song->artist);
		g_object_set_data_full(G_OBJECT(label), "artist", g_strdup(song->artist), g_free);
		g_signal_connect(G_OBJECT(label), "clicked", G_CALLBACK(info3_show_artist),NULL);
		gtk_table_attach(GTK_TABLE(table2), label,1,2,i,i+1,GTK_EXPAND|GTK_FILL, GTK_SHRINK|GTK_FILL,0,0);
		i++;
	}
	if(song->album) {
		if(song->artist) {
			label = gtk_label_new("");
			gtk_label_set_markup(GTK_LABEL(label), _("<b>Album:</b>"));
			gtk_misc_set_alignment(GTK_MISC(label),0,0.5);
			gtk_table_attach(GTK_TABLE(table2), label,0,1,i,i+1,GTK_SHRINK|GTK_FILL, GTK_SHRINK|GTK_FILL,0,0);
			label = gmpc_clicklabel_new(song->album);
			g_object_set_data_full(G_OBJECT(label), "artist", g_strdup(song->artist), g_free);
			g_object_set_data_full(G_OBJECT(label), "album", g_strdup(song->album), g_free);
			g_signal_connect(G_OBJECT(label), "clicked", G_CALLBACK(info3_show_album), NULL);
			gtk_table_attach(GTK_TABLE(table2), label,1,2,i,i+1,GTK_EXPAND|GTK_FILL, GTK_SHRINK|GTK_FILL,0,0);
		}
		else
			info3_add_table_item(table2,_("<b>Album:</b>"),song->album,i);
		i++;
	}
	if(song->genre) {
		info3_add_table_item(table2,_("<b>Genre:</b>"),song->genre,i);
		i++;
	}
	if(song->date) {
		info3_add_table_item(table2,_("<b>Date:</b>"),song->date,i);
		i++;
	}
	if(song->track) {
		info3_add_table_item(table2,_("<b>Track:</b>"),song->track,i);
		i++;
	}
	if(song->composer) {
		info3_add_table_item(table2,_("<b>Composer:</b>"),song->composer,i);
		i++;
	}
	if(song->performer) {
		info3_add_table_item(table2,_("<b>Performer:</b>"),song->performer,i);
		i++;
	}

	if(song->file) {
		/*** Dirname */		
		char *dirname = g_path_get_dirname(song->file);
		info3_add_table_item(table2,_("<b>Dirname:</b>"),dirname,i);
		i++;
		q_free(dirname);
	}
	if(song->comment) {
		info3_add_table_item(table2,_("<b>Comment:</b>"),song->comment,i);
		i++;
	}

	hbox = gtk_hbox_new(FALSE, 24);
	gtk_box_pack_start(GTK_BOX(resizer_vbox), hbox, FALSE, FALSE,0);
	

	vbox = gtk_vbox_new(FALSE, 6);
	/* some sort of list */
	if(song->album && song->artist)
	{
		MpdData *data = NULL;

		mpd_database_search_start(connection,TRUE);
		mpd_database_search_add_constraint(connection, MPD_TAG_ITEM_ARTIST,song->artist);
		mpd_database_search_add_constraint(connection, MPD_TAG_ITEM_ALBUM, song->album);
		data = mpd_database_search_commit(connection);
		if(data) {
			int i=0;
			GtkWidget *label;
			label = gtk_label_new("");
			gtk_label_set_markup(GTK_LABEL(label), _("<b>Album tracks:</b>"));
			gtk_misc_set_alignment(GTK_MISC(label), 0,0.5);
			gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, TRUE,0);	
			for(;data;data = mpd_data_get_next(data))
			{
				if(data->type == MPD_DATA_TYPE_SONG)
				{
					i++;
					gchar *markup = g_strdup_printf("%i. %s",i, data->song->title);
					label = gmpc_clicklabel_new(markup);
					g_object_set_data_full(G_OBJECT(label), "path",g_strdup(data->song->file), g_free);
					gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, TRUE,0);	

					gtk_widget_set_size_request(GTK_WIDGET(label /*button*/), 250,-1);
					g_signal_connect(G_OBJECT(label), "clicked", G_CALLBACK(pl3_info_browser_song_play), NULL);

					q_free(markup);
				}
			}
		}
	}


	gtk_box_pack_start(GTK_BOX(hbox),vbox, FALSE, FALSE,0);


	/**
	 * The lyric display
	 */
	vbox = gtk_vbox_new(FALSE, 6);
	pd = g_malloc0(sizeof(*pd));
	pd->widget = vbox;
	pd->id = current_id;     		
	pd->name = g_strdup(_("lyric"));
	meta_data_get_path_callback(song, META_SONG_TXT, (MetaDataCallback)info3_cover_txt_fetched, pd);


	gtk_box_pack_start(GTK_BOX(hbox), vbox, FALSE, FALSE,0);
	gtk_container_set_border_width(GTK_CONTAINER(hbox), 6);

	if(song->artist)
	{
		int i=0,items = 0;
		MpdData *data = NULL;
		GString *string = NULL;
		/** Artist label */
		label = gtk_label_new("");
		markup =  g_markup_printf_escaped ("<span size=\"xx-large\" weight=\"bold\" style=\"italic\">%s</span>",song->artist);
		gtk_label_set_markup(GTK_LABEL(label),markup);
		gtk_label_set_ellipsize(GTK_LABEL(label), PANGO_ELLIPSIZE_END);
		q_free(markup);
		gtk_misc_set_alignment(GTK_MISC(label), 0,0.5);
		gtk_misc_set_padding(GTK_MISC(label),8,8);
		gtk_box_pack_start(GTK_BOX(resizer_vbox),label,FALSE, FALSE,0);

		/**
		 * Blomb 
		 */
		table = gtk_hbox_new(FALSE,6);

		image = gmpc_metaimage_new(META_ARTIST_ART);
		gmpc_metaimage_set_squared(GMPC_METAIMAGE(image),FALSE);
		gmpc_metaimage_set_size(GMPC_METAIMAGE(image), 150);
		gmpc_metaimage_set_draw_shadow(GMPC_METAIMAGE(image), TRUE);
		gmpc_metaimage_update_cover_from_song(GMPC_METAIMAGE(image), song);

		gtk_box_pack_start(GTK_BOX(table), image, FALSE,FALSE,0);
		gtk_box_pack_start(GTK_BOX(resizer_vbox), table, FALSE,FALSE,0);

		table2 = gtk_table_new(2,3,0);
		/** Genres */
		mpd_database_search_field_start(connection, MPD_TAG_ITEM_GENRE);
		mpd_database_search_add_constraint(connection, MPD_TAG_ITEM_ARTIST, song->artist);
		string = g_string_new("");
		items = 0;
		for(data = mpd_database_search_commit(connection);data != NULL ;data= mpd_data_get_next(data))
		{
			g_string_append_printf(string, "%s%s",data->tag, (mpd_data_is_last(data))?"":", "); 
			items++;
		}
		if(string->len >0)
		{
			info3_add_table_item(table2,(items>1)?_("<b>Genres: </b>"):_("<b>Genre: </b>"), string->str, i);
			i++;
		}
		g_string_free(string, TRUE);
		/* Dates */
		mpd_database_search_field_start(connection, MPD_TAG_ITEM_DATE);
		mpd_database_search_add_constraint(connection, MPD_TAG_ITEM_ARTIST, song->artist);
		string = g_string_new("");
		items= 0;
		for(data = mpd_database_search_commit(connection);data != NULL ;data= mpd_data_get_next(data))
		{
			g_string_append_printf(string, "%s%s",data->tag, (mpd_data_is_last(data))?"":", "); 
			items++;
		}
		if(string->len >0)
		{
			info3_add_table_item(table2, (items >1)?_("<b>Dates: </b>"):_("<b>Date: </b>"), string->str, i);
			i++;
		}
		g_string_free(string, TRUE);

		gtk_box_pack_start(GTK_BOX(table), table2,TRUE,TRUE,0);

		vbox = gtk_vbox_new(FALSE, 6);
		gtk_container_set_border_width(GTK_CONTAINER(vbox), 6);
		pd = g_malloc0(sizeof(*pd));
		pd->widget = vbox;
		pd->id = current_id;     		
		pd->name = g_strdup(_("Artist Info"));
		meta_data_get_path_callback(song, META_ARTIST_TXT, (MetaDataCallback)info3_cover_txt_fetched, pd);
		gtk_box_pack_start(GTK_BOX(resizer_vbox), vbox, FALSE, FALSE,0);
	}


	table2  = gtk_table_new(1,1,TRUE);
	if( song->artist)
	{
		MpdData *data = NULL;
		int i=0;
		mpd_database_search_field_start(connection,MPD_TAG_ITEM_ALBUM);
		mpd_database_search_add_constraint(connection, MPD_TAG_ITEM_ARTIST,song->artist);
		data = mpd_database_search_commit(connection);
		if(data) 
		{
			GList *list = NULL;
			GtkWidget *label = NULL, *hbox = NULL, *image = NULL, *button = NULL;
			mpd_Song *song2 = mpd_newSong();
			label = gtk_label_new("");
			gtk_label_set_markup(GTK_LABEL(label), "<b>Albums by artist</b>");
			gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
			gtk_misc_set_padding(GTK_MISC(label), 6,0);
			gtk_box_pack_start(GTK_BOX(resizer_vbox), label, FALSE, TRUE,6);	
			for(;data;data = mpd_data_get_next(data))
			{
				hbox = gtk_hbox_new(FALSE, 6);	
				song2->artist = song->artist;
				song2->album = data->tag;

				button = gmpc_clicklabel_new(song2->album);
				g_object_set_data_full(G_OBJECT(button), "artist", g_strdup(song->artist), g_free);
				g_object_set_data_full(G_OBJECT(button), "album", g_strdup(data->tag), g_free);
				g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(info3_show_album), NULL);

				image = gmpc_metaimage_new(META_ALBUM_ART);                       		
				gmpc_metaimage_set_squared(GMPC_METAIMAGE(image),TRUE);
				gmpc_metaimage_set_size(GMPC_METAIMAGE(image), 64);
				gmpc_metaimage_set_draw_shadow(GMPC_METAIMAGE(image), TRUE);
				gmpc_metaimage_update_cover_from_song(GMPC_METAIMAGE(image), song2);
				gtk_box_pack_start(GTK_BOX(hbox), image, FALSE, TRUE,0);	
						
				gtk_box_pack_start(GTK_BOX(hbox),button, TRUE, TRUE,0);	
	
				gtk_widget_set_size_request(GTK_WIDGET(hbox), 200,-1);

				list = g_list_append(list, hbox);
				song2->artist = NULL;
				song2->album = NULL;
			}
			mpd_freeSong(song2);

			if(list)
			{
				int a=0;
				i = g_list_length(list);
				gtk_table_resize(GTK_TABLE(table2), (int)ceil(i/3.0), 3);
				do{
					gtk_table_attach_defaults(GTK_TABLE(table2), list->data, a%3, a%3+1, a/3,a/3+1);
					a++;
				}while((list = g_list_next(list)));
				g_list_free(list);
			}

			gtk_box_pack_start(GTK_BOX(resizer_vbox), table2, FALSE, TRUE,0);
		}
	}

	gtk_widget_show_all(info3_vbox);
}

static void info3_update_status_changed(GmpcConnection *gc, MpdObj *mi, ChangedStatusType what, gpointer data)
{
	if(what&(MPD_CST_SONGPOS|MPD_CST_SONGID|MPD_CST_PLAYLIST|MPD_CST_STATE))
	{
		if(MPD_CST_STATE && mpd_player_get_state(mi) == MPD_STATUS_STATE_PAUSE)
			return;
		info3_fill_view();
	}
}
static void pl3_metabrowser_bg_style_changed(GtkWidget *vbox, GtkStyle *style,  GtkWidget *vp)
{
	gtk_widget_modify_bg(vp,GTK_STATE_NORMAL, &(GTK_WIDGET(vbox)->style->base[GTK_STATE_NORMAL]));
}
static void pl3_metabrowser_header_style_changed(GtkWidget *vbox, GtkStyle *style,  GtkWidget *vp)
{
	gtk_widget_modify_bg(vp,GTK_STATE_NORMAL, &(GTK_WIDGET(vbox)->style->light[GTK_STATE_SELECTED]));
}


static void info3_init()
{
	GtkWidget *vp = NULL;;
	GtkWidget *ali/*,*event*/,*vbox;
	/**
	 * main widget used to pack the browser
	 */
	info3_vbox = gtk_frame_new(NULL);


	gtk_widget_set_name(info3_vbox, "gtk_scrolled_window");
	gtk_frame_set_shadow_type(GTK_FRAME(info3_vbox), GTK_SHADOW_ETCHED_IN);
	vbox = gtk_vbox_new(FALSE, 0);

	gtk_container_add(GTK_CONTAINER(info3_vbox), vbox);

	g_signal_connect(G_OBJECT(gmpcconn), "status_changed", G_CALLBACK(info3_update_status_changed), NULL);
	/**
	 * Header 
	 */

	title_event = gtk_event_box_new();
	title_vbox = gtk_hbox_new(FALSE,6);
	ali = gtk_alignment_new(0,0.5,1,1);
	gtk_alignment_set_padding(GTK_ALIGNMENT(ali), 6,6,0,0);
	gtk_container_set_border_width(GTK_CONTAINER(ali),8);
	gtk_container_add(GTK_CONTAINER(ali), title_vbox);
	gtk_container_add(GTK_CONTAINER(title_event), ali);

	g_signal_connect(G_OBJECT(vbox), "style-set", G_CALLBACK(pl3_metabrowser_header_style_changed), title_event);
	gtk_box_pack_start(GTK_BOX(vbox), title_event, FALSE, TRUE,0);
	gtk_box_pack_start(GTK_BOX(vbox), gtk_hseparator_new(), FALSE, TRUE,0);

	/**
	 * The resizer's vbox
	 */
	resizer_vbox = gtk_vbox_new(FALSE, 6);
	/**
	 * The scrolled window to pack the resizer
	 */
	scrolled_window = gtk_scrolled_window_new (NULL, NULL);
	vp = gtk_viewport_new(gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(scrolled_window)),
			gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(scrolled_window)));

	/* hack to change bg color with theme change */
	g_signal_connect(G_OBJECT(vbox), "style-set", G_CALLBACK(pl3_metabrowser_bg_style_changed), vp);

	gtk_viewport_set_shadow_type(GTK_VIEWPORT(vp), GTK_SHADOW_NONE);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
			GTK_POLICY_NEVER,
			GTK_POLICY_AUTOMATIC);



	gtk_container_add(GTK_CONTAINER(vp), resizer_vbox);
	gtk_container_add(GTK_CONTAINER(scrolled_window), vp);
	gtk_box_pack_start_defaults(GTK_BOX(vbox), scrolled_window);
	/**
	 * setup the scrolled window
	 */ 
	GtkAdjustment *adjustment =
		gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW
				(scrolled_window));
	g_object_set (adjustment, "step-increment", (double) 20, NULL);

	gtk_container_set_focus_vadjustment (GTK_CONTAINER (resizer_vbox),
			gtk_scrolled_window_get_vadjustment
			(GTK_SCROLLED_WINDOW (scrolled_window)));



	gtk_widget_show_all(info3_vbox);

	g_object_ref(G_OBJECT(info3_vbox));
	info3_fill_view();
}

static void info3_add(GtkWidget *cat_tree)
{
	GtkTreePath *path = NULL;
	GtkTreeStore *pl3_tree = (GtkTreeStore *)gtk_tree_view_get_model(GTK_TREE_VIEW(cat_tree));	
	GtkTreeIter iter;
	if(!cfg_get_single_value_as_int_with_default(config, "info3-plugin", "enable", 1)) return;
	gtk_tree_store_append(pl3_tree, &iter, NULL);
	gtk_tree_store_set(pl3_tree, &iter, 
			PL3_CAT_TYPE, info3_plugin.id,
			PL3_CAT_TITLE, _("Song Information"),
			PL3_CAT_INT_ID, "/",
			PL3_CAT_ICON_ID, GTK_STOCK_DIALOG_INFO,
			PL3_CAT_PROC, TRUE,
			PL3_CAT_ICON_SIZE,GTK_ICON_SIZE_DND,-1);


	if (info3_ref) {
		gtk_tree_row_reference_free(info3_ref);
		info3_ref = NULL;
	}

	path = gtk_tree_model_get_path(GTK_TREE_MODEL(playlist3_get_category_tree_store()), &iter);
	if (path) {
		info3_ref = gtk_tree_row_reference_new(GTK_TREE_MODEL(playlist3_get_category_tree_store()), path);
		gtk_tree_path_free(path);
	}
}

static void info3_selected(GtkWidget *container)
{
	if(info3_vbox== NULL) {
		info3_init();
	}
	gtk_container_add(GTK_CONTAINER(container), info3_vbox);
	gtk_widget_show_all(info3_vbox);
}

static void info3_unselected(GtkWidget *container)
{
	gtk_container_remove(GTK_CONTAINER(container),info3_vbox);
}

static void info3_set_enabled(int enabled)
{
	cfg_set_single_value_as_int(config, "info3-plugin", "enable", enabled);
	if (enabled)
	{
		if(info3_ref == NULL)
		{
			info3_add(GTK_WIDGET(playlist3_get_category_tree_view()));
		}
	}
	else if (info3_ref)
	{
		GtkTreePath *path = gtk_tree_row_reference_get_path(info3_ref);
		if (path){
			GtkTreeIter iter;
			if (gtk_tree_model_get_iter(GTK_TREE_MODEL(playlist3_get_category_tree_store()), &iter, path)){
				gtk_tree_store_remove(playlist3_get_category_tree_store(), &iter);
			}
			gtk_tree_path_free(path);
			gtk_tree_row_reference_free(info3_ref);
			info3_ref = NULL;
		}                                                                                                  	
	}                                                                                                      	
	pl3_update_go_menu();
}

static int info3_get_enabled()
{
	return 	cfg_get_single_value_as_int_with_default(config, "info3-plugin", "enable", 1);
}


static void info3_activate()
{
	GtkTreeSelection *selec = gtk_tree_view_get_selection((GtkTreeView *)
			playlist3_get_category_tree_view());	

	GtkTreePath *path = gtk_tree_row_reference_get_path(info3_ref);
	if(path)
	{
		gtk_tree_selection_select_path(selec, path);
		gtk_tree_path_free(path);
	}
}

static int info3_add_go_menu(GtkWidget *menu)
{
	GtkWidget *item = NULL;
	if(!cfg_get_single_value_as_int_with_default(config, "info3-plugin", "enable", 1)) return 0;
	item = gtk_image_menu_item_new_with_label(_("Song Information"));
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item), 
			gtk_image_new_from_icon_name(GTK_STOCK_DIALOG_INFO, GTK_ICON_SIZE_MENU));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	g_signal_connect(G_OBJECT(item), "activate", 
			G_CALLBACK(info3_activate), NULL);
	return 1;
}

static int info3_key_press_event(GtkWidget *mw, GdkEventKey *event, int type)
{
	if (event->keyval == GDK_F6)
	{
		info3_activate();
		info3_fill_view();
		return TRUE;
	}

	return FALSE;
}

static void info3_show_album(GtkWidget *button, gpointer data)
{
  char *artist = g_object_get_data(G_OBJECT(button), "artist");
  char *album = g_object_get_data(G_OBJECT(button), "album");
  if(artist  && album)
  {
		info2_activate();
    info2_fill_album_view(artist, album);
  }
}
static void info3_show_artist(GtkWidget *button, gpointer data)
{
  char *artist = g_object_get_data(G_OBJECT(button), "artist");
  if(artist)
  {
		info2_activate();
    info2_fill_artist_view(artist);
  }
}
