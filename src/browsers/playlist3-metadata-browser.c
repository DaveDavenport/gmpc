#include <gdk/gdkkeysyms.h>
#include <stdio.h>
#include <string.h>
#include <regex.h>
/*#include <math.h>*/
#include "main.h"
#include "misc.h"

#include "gob/gmpc-clicklabel.h"
#include "gmpc-meta-text-view.h"

static GtkTargetEntry target_table[] = {
	{ "internal-drop",GTK_TARGET_SAME_APP,99}
};




extern GladeXML *pl3_xml;

static void info2_add(GtkWidget *);
static void info2_selected(GtkWidget *);
static void info2_unselected(GtkWidget *);
static int info2_add_go_menu(GtkWidget *);
static int info2_get_enabled(void);
static void info2_set_enabled(int);
static void remove_container_entries(GtkContainer *);

static void info2_fill_view(void);
static int info2_key_press_event(GtkWidget *mw, GdkEventKey *event, int type);
static void as_song_clicked(GtkButton *button, gpointer data);
static gboolean info2_row_expose_event(GtkWidget *widget, GdkEventExpose *event, gpointer data);

static GtkWidget *info2_create_artist_button(mpd_Song *song);
static GtkWidget *resizer_vbox= NULL;
static GtkWidget *info2_vbox = NULL,*title_vbox=NULL;
static GtkWidget *title_event=NULL;
static GtkWidget *scrolled_window = NULL;
static GtkEntryCompletion *entry_completion = NULL;


/**
 * playing 
 */

typedef struct {
	GtkWidget *widget;
	gint id;
	gchar *name;
}PassData;

/* Needed plugin_wp stuff */
gmpcPlBrowserPlugin info2_gbp = {
	info2_add,		/** add */
	info2_selected,		/** selected */
	info2_unselected,	/** unselected */
	NULL,			/** changed */
	NULL,			/** row expand */
	NULL,			/** cat right mouse menu */ 
	NULL,			/** cat key press */
	info2_add_go_menu,	/** add go menu */
	info2_key_press_event	/** key press event */		
};
gmpcPlugin metab_plugin = {
	"Metadata Browser",
	{0,0,1},
	GMPC_PLUGIN_PL_BROWSER,
	0,
	NULL,                   /* parth */
	NULL,                   /* init */
        NULL,                   /* Destroy */
	&info2_gbp,             /* Browser */
	NULL,                   /* status changed */
	NULL,                   /* connection changed */
	NULL,                   /* Preferences */
	NULL,                   /* cover */
	info2_get_enabled,
	info2_set_enabled
};

/* Playlist window row reference */
static GtkTreeRowReference *info2_ref = NULL;


/**
 * Drag test
 */
static void info2_album_drag_data_get(GtkWidget *event, GdkDragContext *context, GtkSelectionData *sel_data, guint time, guint info,gpointer udata)
{
	gchar *data = g_strdup_printf("artist:%s\nalbum:%s", 
			(gchar *)g_object_get_data(G_OBJECT(event), "artist"), 
			(gchar *)g_object_get_data(G_OBJECT(event), "album"));

	gtk_selection_data_set (sel_data, GDK_TARGET_STRING, 8,
			(const guchar *) data, strlen(data));
	g_free(data);

}
static void info2_artist_drag_data_get(GtkWidget *event, GdkDragContext *context, GtkSelectionData *sel_data, guint time, guint info,gpointer udata)
{
	gchar *data = g_strdup_printf("artist:%s", 
			(gchar *)g_object_get_data(G_OBJECT(event), "artist"));

	gtk_selection_data_set (sel_data, GDK_TARGET_STRING, 8,
			(const guchar *) data, strlen(data));
	g_free(data);

}
/**
 * Callback functions to propperly react too changes in theme/style
 */
static void pl3_metabrowser_bg_style_changed(GtkWidget *vbox, GtkStyle *style,  GtkWidget *vp)
{
	gtk_widget_modify_bg(vp,GTK_STATE_NORMAL, &(GTK_WIDGET(vbox)->style->base[GTK_STATE_NORMAL]));
}
static void pl3_metabrowser_header_style_changed(GtkWidget *vbox, GtkStyle *style,  GtkWidget *vp)
{
	gtk_widget_modify_bg(vp,GTK_STATE_NORMAL, &(GTK_WIDGET(vbox)->style->light[GTK_STATE_SELECTED]));
}



/**
 * Helper functions that can fill and refill a table
 */

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


/**
 * Clears the content
 */
static void info2_widget_clear_children(GtkWidget *wid)
{
	GList *list, *node;
	/**
	 * Remove all the remaining widgets in the view
	 */
	list = gtk_container_get_children(GTK_CONTAINER(wid));
	if(list)
	{
		for(node = g_list_first(list); node; node = g_list_next(node))
		{
			gtk_container_remove(GTK_CONTAINER(wid), node->data);
		}
		g_list_free(list);
	}
}

/**
 * Resets the view
 */
static void info2_prepare_view()
{
	GtkAdjustment *h = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(scrolled_window));
	info2_widget_clear_children(resizer_vbox);
	gtk_adjustment_set_value(h, 0.0);
	/**
	 * Make sure that if there is still a custom cursor it's cleared
	 */
	if(resizer_vbox && GTK_WIDGET(resizer_vbox)->window) {
		gdk_window_set_cursor(GTK_WIDGET(resizer_vbox)->window,NULL); 
	}
}


static void as_album_clicked(GtkButton *button, gpointer data)
{
	int clear = GPOINTER_TO_INT(data);
	char *artist = g_object_get_data(G_OBJECT(button), "artist");
	char *album =  g_object_get_data(G_OBJECT(button), "album");
	if(artist)
	{
		MpdData *data = NULL;
		if(clear)
			mpd_playlist_clear(connection);

		for(data = mpd_database_find(connection, MPD_TAG_ITEM_ARTIST, artist, TRUE)
				;data;data = mpd_data_get_next(data))
		{
			if(data->type == MPD_DATA_TYPE_SONG)
			{
				if(data->song->album && !strcmp(data->song->album,album))
				{
					mpd_playlist_queue_add(connection, data->song->file);
				}
			}
		}
		mpd_playlist_queue_commit(connection);
		if(clear)
			mpd_player_play(connection);

	}

}

static void as_artist_viewed_clicked(GtkButton *button, gpointer data)
{
	char *artist = g_strdup(g_object_get_data(G_OBJECT(button), "artist"));
	info2_fill_artist_view(artist);
	q_free(artist);
}

static void as_album_viewed_clicked(GtkButton *button, gpointer data)
{
	char *artist = g_strdup(g_object_get_data(G_OBJECT(button), "artist"));
	char *album = g_strdup(g_object_get_data(G_OBJECT(button), "album"));
	info2_fill_album_view(artist,album);
	q_free(artist);
	q_free(album);
}

static void as_song_viewed_clicked(GtkButton *button, gpointer data)
{
	char *artist = g_strdup(g_object_get_data(G_OBJECT(button), "file"));
	if(artist == NULL) {
		printf("CRAP NO PATH\n");
	} else {
		info2_fill_song_view(artist);
	}
	q_free(artist);
}
static void as_artist_clicked(GtkButton *button, gpointer data)
{
	int clear = GPOINTER_TO_INT(data);
	char *artist = g_object_get_data(G_OBJECT(button), "artist");
	if(artist)
	{
		MpdData *data = NULL;
		if(clear)
			mpd_playlist_clear(connection);

		for(data = mpd_database_find(connection, MPD_TAG_ITEM_ARTIST, artist, TRUE)
				;data;data = mpd_data_get_next(data))
		{
			if(data->type == MPD_DATA_TYPE_SONG)
			{
				mpd_playlist_queue_add(connection, data->song->file);
			}
		}
		mpd_playlist_queue_commit(connection);
		if(clear)
			mpd_player_play(connection);
	}
}

static void info2_add_table_item(GtkWidget *table,char *name, char *value, int i)
{
	GtkWidget *label;
	label = gtk_label_new("");
	gtk_label_set_markup(GTK_LABEL(label), name);
	gtk_misc_set_alignment(GTK_MISC(label),0,0.5);
	gtk_table_attach(GTK_TABLE(table), label,0,1,i,i+1,GTK_SHRINK|GTK_FILL, GTK_SHRINK|GTK_FILL,0,0);
	label = gtk_label_new(value);
	gtk_misc_set_alignment(GTK_MISC(label),0,0.5);
	gtk_table_attach(GTK_TABLE(table),label,1,2,i,i+1,GTK_EXPAND|GTK_FILL, GTK_SHRINK|GTK_FILL,0,0);
	gtk_label_set_ellipsize(GTK_LABEL(label), PANGO_ELLIPSIZE_END);
}

/**
 * Create an artist "button"
 */

static GtkWidget *info2_create_artist_button(mpd_Song *song)
{
	GtkWidget *metaimage,*vbox,*ali,*label,*button,*event,*table;
	int i = 0,items;
	gchar *buffer;
	GString *string;
	MpdData *data;

	MpdDBStats *stats = NULL;
	
	/* Button bg drawing code */
	event = gtk_event_box_new();
	gtk_widget_set_app_paintable(GTK_WIDGET(event), TRUE);
	g_signal_connect(G_OBJECT(event), "expose-event", G_CALLBACK(info2_row_expose_event), NULL);


	vbox = gtk_hbox_new(FALSE, 6);
	gtk_container_set_border_width(GTK_CONTAINER(vbox),4);

	/** Create artist image */	
	metaimage = gmpc_metaimage_new(META_ARTIST_ART);
	gmpc_metaimage_set_size(GMPC_METAIMAGE(metaimage), 100);
	gmpc_metaimage_update_cover_from_song(GMPC_METAIMAGE(metaimage), song);
	gtk_box_pack_start(GTK_BOX(vbox), metaimage, FALSE, FALSE,0);

	/** Create table */
	table = gtk_table_new(2,2, FALSE); 
	gtk_table_set_col_spacings(GTK_TABLE(table),6);
	/**
	 *  Artist 
	 */
	if(song->artist) {
		info2_add_table_item(table,_("<b>Artist:</b>"),song->artist,i);
		i++;
	}
	/**
	 * Songs list 
	 */
	mpd_database_search_stats_start(connection);
	mpd_database_search_add_constraint(connection, MPD_TAG_ITEM_ARTIST, song->artist);
	stats = mpd_database_search_stats_commit(connection);
	buffer = g_strdup_printf("%i", stats->numberOfSongs);
	info2_add_table_item(table,_("<b>Songs:</b>"),buffer,i);
	i++;
	q_free(buffer); 
	/**
	 * Playtime
	 */
	buffer = format_time_real(stats->playTime,"");
	info2_add_table_item(table,_("<b>Playtime:</b>"),buffer,i);
	i++;                                                    	
	q_free(buffer); 
	mpd_database_search_free_stats(stats);
	/**
	 * Genre
	 */
	mpd_database_search_field_start(connection, MPD_TAG_ITEM_GENRE);
	mpd_database_search_add_constraint(connection, MPD_TAG_ITEM_ARTIST, song->artist);
	string = g_string_new("");
	for(data = mpd_database_search_commit(connection);data != NULL ;data= mpd_data_get_next(data))
	{
		g_string_append_printf(string, "%s%s",data->tag, (mpd_data_is_last(data))?"":", "); 
	}
	if(string->len >0)
	{
		info2_add_table_item(table, _("<b>Genre:</b>"), string->str, i);
		i++;
	}
	g_string_free(string, TRUE);
	/**
	 *  Dates 
	 */
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
		info2_add_table_item(table, (items >1)?_("<b>Dates: </b>"):_("<b>Date: </b>"), string->str, i);
		i++;
	}
	g_string_free(string, TRUE);




	gtk_box_pack_start(GTK_BOX(vbox), table, TRUE,TRUE,0);


	/** Button box */
	label = gtk_vbox_new(FALSE,0);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE,0);
	/* Play button */
	button = gtk_button_new_with_label(_("Replace"));
	gtk_button_set_image(GTK_BUTTON(button), gtk_image_new_from_stock(GTK_STOCK_REDO,GTK_ICON_SIZE_BUTTON));
	gtk_button_set_relief(GTK_BUTTON(button), GTK_RELIEF_NONE);
	g_object_set_data_full(G_OBJECT(button), "artist",g_strdup(song->artist), g_free);
	g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(as_artist_clicked), GINT_TO_POINTER(1));
	ali = gtk_alignment_new(0,0.5,0,0);
	gtk_container_add(GTK_CONTAINER(ali), button);
	gtk_box_pack_start(GTK_BOX(label), ali,FALSE,TRUE,0);
	/* Add */
	button = gtk_button_new_from_stock(GTK_STOCK_ADD);
	gtk_button_set_relief(GTK_BUTTON(button), GTK_RELIEF_NONE);
	g_object_set_data_full(G_OBJECT(button), "artist",g_strdup(song->artist), g_free);
	g_signal_connect(G_OBJECT(button), "clicked",G_CALLBACK(as_artist_clicked),GINT_TO_POINTER(0));
	ali = gtk_alignment_new(0,0.5,0,0);
	gtk_container_add(GTK_CONTAINER(ali), button);
	gtk_box_pack_start(GTK_BOX(label),ali,FALSE,TRUE,0);
	/* View */
	button = gtk_button_new_with_label("View");
	gtk_button_set_image(GTK_BUTTON(button), gtk_image_new_from_stock(GTK_STOCK_FIND, GTK_ICON_SIZE_BUTTON));
	gtk_button_set_relief(GTK_BUTTON(button), GTK_RELIEF_NONE);
	g_object_set_data_full(G_OBJECT(button), "artist",g_strdup(song->artist), g_free);
	g_signal_connect(G_OBJECT(button), "clicked",G_CALLBACK(as_artist_viewed_clicked),NULL);
	ali = gtk_alignment_new(0,0.5,0,0);
	gtk_container_add(GTK_CONTAINER(ali), button);
	gtk_box_pack_start(GTK_BOX(label), ali, FALSE,TRUE,0);


	/** Add button to bg container */
	gtk_container_add(GTK_CONTAINER(event), vbox);

	/** Setup dragging */
	gtk_drag_source_set(event, GDK_BUTTON1_MASK,target_table, 1,GDK_ACTION_COPY|GDK_ACTION_MOVE);
	g_signal_connect(G_OBJECT(event), "drag-data-get", G_CALLBACK(info2_artist_drag_data_get), NULL);
	g_object_set_data_full(G_OBJECT(event), "artist",g_strdup(song->artist), g_free);
	gtk_drag_source_set_icon_name(event, "media-artist");

	return event;
}

/** 
 * Song View 
 */
void info2_fill_song_view(char *path) 
{
	GtkWidget *expander, *gmtv;
	GtkWidget *ali = NULL;
	mpd_Song *song = NULL;
	GtkWidget *button = NULL;
	GtkWidget *label = NULL;
	char *markup = NULL;

	/** 
	 * Clear the view
	 */
	info2_prepare_view();

	song = mpd_database_get_fileinfo(connection, path);
	if(!song)
		return;

	/**
	 * Clear header
	 */
	info2_widget_clear_children(title_vbox);

	/**
	 * Collection 
	 */
	button = gtk_button_new();
	label = gtk_label_new("");
	gtk_label_set_markup(GTK_LABEL(label),"<span size='xx-large' weight='bold'>Collection</span>");
	gtk_misc_set_alignment(GTK_MISC(label), 0,0.5);
	gtk_button_set_relief(GTK_BUTTON(button), GTK_RELIEF_NONE);
	g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(info2_fill_view), NULL);
	gtk_container_add(GTK_CONTAINER(button),label);
	gtk_widget_show_all(button);
	gtk_box_pack_start(GTK_BOX(title_vbox),button, FALSE, TRUE,0);
	/** 
	 * artist
	 */

	label = gtk_label_new("");
	gtk_label_set_markup(GTK_LABEL(label),"<span size='xx-large' weight='bold'>/</span>");
	gtk_misc_set_alignment(GTK_MISC(label), 0,0.5);
	gtk_box_pack_start(GTK_BOX(title_vbox), label, FALSE, TRUE,0);                        		
	button = gtk_button_new();
	label = gtk_label_new("");
	markup = g_markup_printf_escaped ("<span size=\"xx-large\" weight=\"bold\">%s</span>", song->artist);
	gtk_misc_set_alignment(GTK_MISC(label), 0,0.5);
	gtk_label_set_markup(GTK_LABEL(label),markup);
	gtk_button_set_relief(GTK_BUTTON(button), GTK_RELIEF_NONE);
	g_object_set_data_full(G_OBJECT(button), "artist",g_strdup(song->artist), g_free);
	g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(as_artist_viewed_clicked), NULL);
	gtk_container_add(GTK_CONTAINER(button),label);
	gtk_widget_show_all(button);

	gtk_box_pack_start(GTK_BOX(title_vbox),button, FALSE, TRUE,0);
	q_free(markup);
	/**
	 * album
	 */		
	button = gtk_button_new();
	label = gtk_label_new("");
	gtk_label_set_markup(GTK_LABEL(label),"<span size='xx-large' weight='bold'>/</span>");
	gtk_misc_set_alignment(GTK_MISC(label), 0,0.5);
	gtk_box_pack_start(GTK_BOX(title_vbox), label, FALSE, TRUE,0);
	markup =  g_markup_printf_escaped ("<span size=\"xx-large\" weight=\"bold\">%s</span>", song->album);
	label = gtk_label_new("");
	gtk_misc_set_alignment(GTK_MISC(label), 0,0.5);
	gtk_label_set_markup(GTK_LABEL(label),markup);
	gtk_label_set_ellipsize(GTK_LABEL(label), PANGO_ELLIPSIZE_END);
	gtk_button_set_relief(GTK_BUTTON(button), GTK_RELIEF_NONE);
	g_object_set_data_full(G_OBJECT(button), "artist",g_strdup(song->artist), g_free);
	g_object_set_data_full(G_OBJECT(button), "album",g_strdup(song->album), g_free);
	g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(as_album_viewed_clicked), NULL);
	gtk_container_add(GTK_CONTAINER(button),label);
	gtk_widget_show_all(button);
	gtk_box_pack_start(GTK_BOX(title_vbox), button, TRUE, TRUE,0);
	q_free(markup);

	/** 
	 * Title Label
	 */
	label = gtk_label_new("");
	markup =  g_markup_printf_escaped ("<span size=\"xx-large\" weight=\"bold\" style=\"italic\">%s</span>", song->title);
	gtk_label_set_markup(GTK_LABEL(label),markup);
	gtk_misc_set_alignment(GTK_MISC(label), 0,0.5);
	gtk_misc_set_padding(GTK_MISC(label),8,8);
	q_free(markup);
	gtk_box_pack_start(GTK_BOX(resizer_vbox), label, FALSE, FALSE,0);
	gtk_misc_set_alignment(GTK_MISC(label), 0,0.5);
	/**
	 * Set album image
	 */
	GtkWidget *hbox = NULL;
	GtkWidget *table2,*table = gtk_table_new(2,2,FALSE);
	GtkWidget *image = NULL;
	ali = gtk_alignment_new(0,0.5,1,0);
	gtk_container_set_border_width(GTK_CONTAINER(ali), 8);
	image = gmpc_metaimage_new(META_ALBUM_ART);
	gmpc_metaimage_set_size(GMPC_METAIMAGE(image), 150);
	gmpc_metaimage_set_draw_shadow(GMPC_METAIMAGE(image), TRUE);
	gmpc_metaimage_update_cover_from_song(GMPC_METAIMAGE(image), song);
	gtk_table_attach(GTK_TABLE(table), image, 0,1,0,2,GTK_SHRINK|GTK_FILL, GTK_SHRINK|GTK_FILL,0,0);

	/** pack the table and add to view */
	gtk_container_add(GTK_CONTAINER(ali), table);
	gtk_box_pack_start(GTK_BOX(resizer_vbox), ali, FALSE, FALSE,0);

	/***
	 * Lyrics
	 */
	/* label */
	expander= gtk_label_new(_("Lyric:"));
	gtk_label_set_markup(GTK_LABEL(expander), _("<b>Lyric:</b>"));
	gtk_misc_set_alignment(GTK_MISC(expander), 0,0.5);
	gtk_misc_set_padding(GTK_MISC(expander), 8,0);

	GtkWidget *event = gtk_event_box_new();
	GtkWidget *ali2 = gtk_alignment_new(0,0,0.5,0);
	ali = gtk_alignment_new(0,0,0.5,0);
	/* the lyric */
	gmtv = gmpc_meta_text_view_new(META_SONG_TXT);
	gtk_text_view_set_left_margin(GTK_TEXT_VIEW(gmtv), 8);
	gtk_widget_modify_base(gmtv, GTK_STATE_NORMAL, &(resizer_vbox->style->mid[GTK_STATE_NORMAL]));
	gmpc_meta_text_view_query_text_from_song(GMPC_META_TEXT_VIEW(gmtv), song);


	gtk_box_pack_start(GTK_BOX(resizer_vbox), expander, FALSE, FALSE,0);
	gtk_widget_modify_bg(event, GTK_STATE_NORMAL, &(resizer_vbox->style->dark[GTK_STATE_NORMAL]));
	gtk_alignment_set_padding(GTK_ALIGNMENT(ali), 1,1,1,1);
	gtk_alignment_set(GTK_ALIGNMENT(ali), 0,0,1,0);
	gtk_alignment_set_padding(GTK_ALIGNMENT(ali2), 0,0,12,0);

	gtk_container_add(GTK_CONTAINER(ali), gmtv);
	gtk_container_add(GTK_CONTAINER(event), ali);
	gtk_container_add(GTK_CONTAINER(ali2), event);
	gtk_box_pack_start(GTK_BOX(resizer_vbox), ali2, FALSE,FALSE,0);


	/**
	 * Play Button 
	 */
	hbox = gtk_hbox_new(FALSE,6);
	button = gtk_button_new_with_label(_("Replace"));
	gtk_button_set_image(GTK_BUTTON(button),gtk_image_new_from_stock(GTK_STOCK_REDO,GTK_ICON_SIZE_BUTTON));
	g_object_set_data_full(G_OBJECT(button), "file",g_strdup(song->file), g_free);
	g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(as_song_clicked), GINT_TO_POINTER(1));
	gtk_button_set_relief(GTK_BUTTON(button), GTK_RELIEF_NONE);
	ali = gtk_alignment_new(0,0.5,0,0);
	gtk_container_add(GTK_CONTAINER(ali), button);
	gtk_box_pack_start(GTK_BOX(hbox), ali, FALSE,TRUE,0);
	/**
	 * Add Button 
	 */
	button = gtk_button_new_from_stock(GTK_STOCK_MEDIA_PLAY);
	g_object_set_data_full(G_OBJECT(button), "file",g_strdup(song->file), g_free);
	g_signal_connect(G_OBJECT(button), "clicked",G_CALLBACK(as_song_clicked),GINT_TO_POINTER(0));
	gtk_button_set_relief(GTK_BUTTON(button), GTK_RELIEF_NONE);
	ali = gtk_alignment_new(0,0.5,0,0);
	gtk_container_add(GTK_CONTAINER(ali), button);
	gtk_box_pack_start(GTK_BOX(hbox), ali, FALSE,TRUE,0);

	table2 = gtk_table_new(2,2,0);
	gtk_container_set_border_width(GTK_CONTAINER(table2), 8);

	gtk_table_set_col_spacings(GTK_TABLE(table2), 6);

	gtk_table_attach(GTK_TABLE(table), table2, 1,2,0,1,GTK_EXPAND|GTK_FILL, GTK_EXPAND|GTK_FILL,0,0);
	gtk_table_attach(GTK_TABLE(table), hbox, 1,2,1,2,GTK_SHRINK|GTK_FILL, GTK_SHRINK|GTK_FILL,0,0);


	int i =0;
	if(song->artist) {
		info2_add_table_item(table2,_("<b>Artist:</b>"),song->artist,i);
		i++;
	}
	if(song->album) {
		info2_add_table_item(table2,_("<b>Album:</b>"),song->album,i);
		i++;
	}
	if(song->genre) {
		info2_add_table_item(table2,_("<b>Genre:</b>"),song->genre,i);
		i++;
	}
	if(song->date) {
		info2_add_table_item(table2,_("<b>Date:</b>"),song->date,i);
		i++;
	}
	if(song->track) {
		info2_add_table_item(table2,_("<b>Track:</b>"),song->track,i);
		i++;
	}
	if(song->composer) {
		info2_add_table_item(table2,_("<b>Composer:</b>"),song->composer,i);
		i++;
	}
	if(song->performer) {
		info2_add_table_item(table2,_("<b>Performer:</b>"),song->performer,i);
		i++;
	}

	if(song->file) {
		/*** Dirname */		
		char *dirname = g_path_get_dirname(song->file);
		info2_add_table_item(table2,_("<b>Dirname:</b>"),dirname,i);
		i++;
		q_free(dirname);
	}

	mpd_freeSong(song);
	gtk_widget_show_all(info2_vbox);
}


static void as_song_clicked(GtkButton *button, gpointer data)
{
	int clear = GPOINTER_TO_INT(data);
	char *file = g_object_get_data(G_OBJECT(button), "file");
	if(file)
	{
		if(clear)
			mpd_playlist_clear(connection);
		play_path(file);
	}
}

static gboolean info2_row_expose_event(GtkWidget *widget, GdkEventExpose *event, gpointer data)
{

	cairo_t *cr= gdk_cairo_create(GTK_WIDGET(widget)->window);
	int width = widget->allocation.width;
	int height = widget->allocation.height;
	cairo_set_line_width (cr, 1.0);
	cairo_rectangle(cr, 0,0,width,height);
	cairo_close_path (cr);
	gdk_cairo_set_source_color(cr, 	&(widget->style->mid[GTK_STATE_NORMAL]));
	cairo_fill_preserve(cr);
	gdk_cairo_set_source_color(cr, 	&(widget->style->dark[GTK_STATE_NORMAL]));
	cairo_stroke (cr);
	cairo_destroy(cr);
	return FALSE;
}
/***
 * Collection view
 */
static void info2_fill_view_entry_activate(GtkEntry *entry, GtkWidget *table)
{
	regex_t regt;
	const char *text = NULL;
	GtkTreeModel *model =gtk_entry_completion_get_model(GTK_ENTRY_COMPLETION(entry_completion));
	GtkTreeIter iter;
	/**
	 * Remove all the remaining widgets in the view
	 */
	remove_container_entries(GTK_CONTAINER(table));


	/** get text
	*/
	text = gtk_entry_get_text(entry);
	if(strlen(text) && !regcomp(&regt, text, REG_EXTENDED|REG_ICASE|REG_NOSUB))
	{
		int skip = 0;
		int num_cols = 2;
		int songs = 0;
		int tile_size = 300;
		MpdData *data = NULL;
		mpd_Song *song;
		/**
		 * 		update completion
		 */
		if(gtk_tree_model_get_iter_first(model, &iter))
		{	
			do{
				char *oldname = NULL;
				gtk_tree_model_get(model, &iter, 0,&oldname,-1);
				if(!strcmp(text, oldname))  skip=TRUE;
				q_free(oldname);
			}while(gtk_tree_model_iter_next(model, &iter) && skip);
		}
		if(!skip)
		{
			gtk_list_store_insert_with_values(GTK_LIST_STORE(model), &iter, 0, 0, text, -1);
		}

		data = mpd_database_get_artists(connection);
		num_cols = (int)(resizer_vbox->allocation.width-20)/(tile_size+6);
		song = mpd_newSong();
		for(;data;data = mpd_data_get_next(data))
		{
			if(songs < 20 && !regexec(&regt,data->tag, 0,NULL,0))
			{
				GtkWidget *button;
				song->artist = data->tag;
				button = info2_create_artist_button(song);
				gtk_box_pack_start(GTK_BOX(table), button, FALSE, FALSE,0);
				song->artist = NULL;				
				songs++;
			}
		}

		/* if there is an "overflow" show a message */
		if(songs >= 20)
		{
			GtkWidget *box = gtk_hbox_new(FALSE, 6);
			GtkWidget *temp = gtk_image_new_from_stock(GTK_STOCK_DIALOG_WARNING, GTK_ICON_SIZE_DIALOG);
			gtk_box_pack_start(GTK_BOX(box), temp, FALSE, TRUE, 0);
			temp = gtk_label_new(_("Only the first 20 result displayed, please refine your search query"));
			gtk_misc_set_alignment(GTK_MISC(temp), 0,0.5);
			gtk_label_set_line_wrap(GTK_LABEL(temp), TRUE);
			gtk_box_pack_start(GTK_BOX(box), temp, TRUE, TRUE, 0);

			gtk_box_pack_start(GTK_BOX(table), box, FALSE, FALSE,0);
		}

		regfree(&regt);
		mpd_freeSong(song);
	}
	gtk_widget_show_all(resizer_vbox);
}

static void info2_fill_view()
{
	GtkWidget *hbox, *label, *entry, *button;
	GtkWidget *artist_table = NULL;


	info2_prepare_view();
	/** Nothing is selected so we are in the basic view
	*/
	/**
	 * setup the title 
	 */

	info2_widget_clear_children(title_vbox);
	/** add buttons */
	button = gtk_button_new();
	label = gtk_label_new("");
	gtk_label_set_markup(GTK_LABEL(label),"<span size=\"xx-large\" weight=\"bold\">Collection</span>");
	gtk_button_set_relief(GTK_BUTTON(button), GTK_RELIEF_NONE);
	gtk_misc_set_alignment(GTK_MISC(label), 0,0.5);
	g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(info2_fill_view), NULL);
	gtk_container_add(GTK_CONTAINER(button),label);
	gtk_box_pack_start(GTK_BOX(title_vbox), button, FALSE, TRUE, 0);
	gtk_widget_show_all(title_vbox);

	/**
	 * Set up the search Row 
	 */
	hbox = gtk_hbox_new(FALSE, 6);
	gtk_container_set_border_width(GTK_CONTAINER(hbox), 8);
	/* The label */
	label = gtk_label_new("Find Artist:");
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 0);
	/* The Entry */
	entry = gtk_entry_new();

	gtk_box_pack_start(GTK_BOX(hbox), entry, TRUE, TRUE, 0);
	/* button */
	gtk_box_pack_start(GTK_BOX(resizer_vbox), hbox, FALSE, TRUE, 0);	

	artist_table = gtk_vbox_new(FALSE,8);
	gtk_container_set_border_width(GTK_CONTAINER(artist_table), 8);
	gtk_box_pack_start(GTK_BOX(resizer_vbox), artist_table, FALSE, TRUE, 0);	
	g_signal_connect(G_OBJECT(entry), "activate", G_CALLBACK(info2_fill_view_entry_activate), artist_table);
	/**
	 * Entry completion
	 */

	if(!entry_completion)
	{
		GtkListStore *store = gtk_list_store_new(1, G_TYPE_STRING);
		entry_completion = gtk_entry_completion_new();
		gtk_entry_completion_set_model(GTK_ENTRY_COMPLETION(entry_completion), GTK_TREE_MODEL(store));
		gtk_entry_completion_set_text_column(GTK_ENTRY_COMPLETION(entry_completion), 0);
		gtk_entry_completion_set_inline_completion(GTK_ENTRY_COMPLETION(entry_completion), TRUE);
	}
	else{
		GtkTreeModel *model =gtk_entry_completion_get_model(GTK_ENTRY_COMPLETION(entry_completion));
		GtkTreeIter iter;
		if(gtk_tree_model_get_iter_first(model, &iter))
		{
			gchar *text= NULL;
			gtk_tree_model_get(model, &iter,0, &text, -1);
			if(text){
				gtk_entry_set_text(GTK_ENTRY(entry), text);
				gtk_widget_activate(GTK_WIDGET(entry));
				q_free(text);
			}
		}
	}
	gtk_entry_set_completion(GTK_ENTRY(entry), GTK_ENTRY_COMPLETION(entry_completion));

	gtk_widget_show_all(resizer_vbox);
}

/*******
 * ARTIST VIEW
 */

void info2_fill_artist_view(char *artist)
{
	GtkWidget *vbox = NULL;
	int i=0,items=0;
	GString *string = NULL;
	MpdData *data = NULL;
	GtkWidget *ali = NULL, *table2 = NULL;
	mpd_Song *song2 = NULL;

	/** 
	 * clean the view 
	 */
	info2_prepare_view();
	/** 
	 * Create a song to use for metadata widgets
	 */
	song2 = mpd_newSong();
	song2->artist = g_strdup(artist);

	/**
	 * Set artist image
	 */
	GtkWidget *expander, *gmtv;
	GtkWidget *hbox = NULL;
	GtkWidget *table = gtk_table_new(2,2,FALSE);
	GtkWidget *image = NULL; 
	gtk_table_set_col_spacings(GTK_TABLE(table),6);
	gtk_container_set_border_width(GTK_CONTAINER(table), 8);

	image = gmpc_metaimage_new(META_ARTIST_ART);
	gmpc_metaimage_set_size(GMPC_METAIMAGE(image), 150);
	gmpc_metaimage_set_draw_shadow(GMPC_METAIMAGE(image), TRUE);
	gmpc_metaimage_update_cover_from_song(GMPC_METAIMAGE(image), song2);

	gtk_table_attach(GTK_TABLE(table), image, 0,1,0,2,GTK_SHRINK|GTK_FILL, GTK_SHRINK|GTK_FILL,0,0);

	/** pack the table and add to view */
	gtk_box_pack_start(GTK_BOX(resizer_vbox), table, FALSE, TRUE,0);

	vbox = gtk_vbox_new(FALSE, 6);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 6);

	/**
	 * Artist Information 
	 */
	expander = gtk_expander_new(_("Artist info:"));
	gmtv = gmpc_meta_text_view_new(META_ARTIST_TXT);
	gtk_text_view_set_left_margin(GTK_TEXT_VIEW(gmtv), 12);
	/* expander */
	gtk_expander_set_use_markup(GTK_EXPANDER(expander),TRUE);
	gtk_expander_set_label(GTK_EXPANDER(expander), _("<b>Artist info:</b>"));
	/* query */
	gmpc_meta_text_view_query_text_from_song(GMPC_META_TEXT_VIEW(gmtv), song2);
	gtk_container_add(GTK_CONTAINER(expander), gmtv);
	gtk_box_pack_start(GTK_BOX(vbox), expander, TRUE,TRUE,0);	
	gtk_box_pack_start(GTK_BOX(resizer_vbox), vbox, FALSE, FALSE,0);

	/**
	 * Play Button 
	 */
	hbox = gtk_hbox_new(FALSE,6);
	GtkWidget *button = gtk_button_new_with_label(_("Replace"));
	gtk_button_set_image(GTK_BUTTON(button),gtk_image_new_from_stock(GTK_STOCK_REDO,GTK_ICON_SIZE_BUTTON));
	g_object_set_data_full(G_OBJECT(button), "artist",g_strdup(artist), g_free);
	g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(as_artist_clicked), GINT_TO_POINTER(1));
	gtk_button_set_relief(GTK_BUTTON(button), GTK_RELIEF_NONE);
	ali = gtk_alignment_new(0,0.5,0,0);
	gtk_container_add(GTK_CONTAINER(ali), button);
	gtk_box_pack_start(GTK_BOX(hbox), ali, FALSE,TRUE,0);
	/**
	 * Add Button 
	 */
	button = gtk_button_new_from_stock(GTK_STOCK_ADD);
	g_object_set_data_full(G_OBJECT(button), "artist",g_strdup(artist), g_free);
	g_signal_connect(G_OBJECT(button), "clicked",G_CALLBACK(as_artist_clicked),GINT_TO_POINTER(0));
	gtk_button_set_relief(GTK_BUTTON(button), GTK_RELIEF_NONE);
	ali = gtk_alignment_new(0,0.5,0,0);
	gtk_container_add(GTK_CONTAINER(ali), button);
	gtk_box_pack_start(GTK_BOX(hbox), ali, FALSE,TRUE,0);

	/** Song info */
	table2 = gtk_table_new(2,2,0);
	gtk_table_set_col_spacings(GTK_TABLE(table2),6);
	i=0;
	/* Genre field */
	mpd_database_search_field_start(connection, MPD_TAG_ITEM_GENRE);
	mpd_database_search_add_constraint(connection, MPD_TAG_ITEM_ARTIST, song2->artist);
	string = g_string_new("");
	items = 0;
	for(data = mpd_database_search_commit(connection);data != NULL ;data= mpd_data_get_next(data))
	{
		g_string_append_printf(string, "%s%s",data->tag, (mpd_data_is_last(data))?"":", "); 
		items++;
	}
	if(string->len >0)
	{
		info2_add_table_item(table2,(items>1)?_("<b>Genres: </b>"):_("<b>Genre: </b>"), string->str, i);
		i++;
	}
	g_string_free(string, TRUE);
	/* Dates */
	mpd_database_search_field_start(connection, MPD_TAG_ITEM_DATE);
	mpd_database_search_add_constraint(connection, MPD_TAG_ITEM_ARTIST, song2->artist);
	string = g_string_new("");
	items= 0;
	for(data = mpd_database_search_commit(connection);data != NULL ;data= mpd_data_get_next(data))
	{
		g_string_append_printf(string, "%s%s",data->tag, (mpd_data_is_last(data))?"":", "); 
		items++;
	}
	if(string->len >0)
	{
		info2_add_table_item(table2, (items >1)?_("<b>Dates: </b>"):_("<b>Date: </b>"), string->str, i);
		i++;
	}
	g_string_free(string, TRUE);
	gtk_table_attach(GTK_TABLE(table),table2, 1,2,0,1,GTK_EXPAND|GTK_FILL, GTK_EXPAND|GTK_FILL,0,0);
	gtk_table_attach(GTK_TABLE(table), hbox, 1,2,1,2,GTK_SHRINK|GTK_FILL, GTK_SHRINK|GTK_FILL,0,0);

	/**
	 * label
	 */
	GtkWidget *label= gtk_label_new("");
	gtk_label_set_markup(GTK_LABEL(label), "<span size=\"x-large\" weight=\"bold\">Albums:</span>");
	gtk_misc_set_alignment(GTK_MISC(label), 0,0.5);
	gtk_misc_set_padding(GTK_MISC(label),8,8);
	gtk_box_pack_start(GTK_BOX(resizer_vbox), label, FALSE, FALSE,0);



	if(song2 && song2->artist)
	{
		int i=0;
		/**
		 * and event box, so I can draw a background color 
		 */
		GtkWidget *button = NULL;
		GtkWidget *ali = NULL;
		GtkWidget *vbox1 =NULL;
		GtkWidget *label = gtk_label_new("");
		char *markup = NULL;
		info2_widget_clear_children(title_vbox);
		button = gtk_button_new();
		label = gtk_label_new("");
		gtk_label_set_markup(GTK_LABEL(label),"<span size='xx-large' weight='bold'>Collection</span>");
		gtk_misc_set_alignment(GTK_MISC(label), 0,0.5);
		gtk_button_set_relief(GTK_BUTTON(button), GTK_RELIEF_NONE);
		g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(info2_fill_view), NULL);
		gtk_container_add(GTK_CONTAINER(button),label);
		gtk_widget_show_all(button);
		gtk_box_pack_start(GTK_BOX(title_vbox),button, FALSE, TRUE,0);
		label = gtk_label_new("");
		gtk_label_set_markup(GTK_LABEL(label),"<span size='xx-large' weight='bold'>/</span>");
		gtk_box_pack_start(GTK_BOX(title_vbox), label, FALSE, TRUE,0);
		markup =  g_markup_printf_escaped ("<span size=\"xx-large\" weight=\"bold\">%s</span>"
				, song2->artist);

		label = gtk_label_new("");
		gtk_label_set_markup(GTK_LABEL(label),markup);
		gtk_widget_show(label);

		gtk_box_pack_start(GTK_BOX(title_vbox), label, FALSE, TRUE,0);
		q_free(markup);
		/**
		 * set a vbox 
		 */
		vbox1 = gtk_vbox_new(FALSE, 6);
		/**
		 * Image
		 */
		MpdData *data = mpd_database_get_albums(connection, song2->artist);
		for(;data;data = mpd_data_get_next(data))
		{
			GtkWidget *table, *image,*event;
			GtkWidget *hbox;
			GtkWidget *button;
			MpdData *data2 = NULL;
			mpd_Song *song  = NULL;
			int tracks = 0;
			GtkWidget *table2= NULL;
			if(!data->tag)
				continue;

			mpd_database_search_start(connection, TRUE);
			mpd_database_search_add_constraint(connection, MPD_TAG_ITEM_ARTIST, song2->artist);
			mpd_database_search_add_constraint(connection, MPD_TAG_ITEM_ALBUM, data->tag);
			data2 = mpd_database_search_commit(connection);

			if(!data2)
				continue;
			song = data2->song;
			for(data2 = mpd_data_get_first(data2);!mpd_data_is_last(data2);data2= mpd_data_get_next(data2)) tracks++;
			tracks++;
			/** 
			 * Create cover art image 
			 */
			table = gtk_hbox_new(FALSE, 6); 

			image = gmpc_metaimage_new(META_ALBUM_ART);
			gmpc_metaimage_set_size(GMPC_METAIMAGE(image), 100);
			gmpc_metaimage_update_cover_from_song(GMPC_METAIMAGE(image), song);

			gtk_container_set_border_width(GTK_CONTAINER(table), 8);
			gtk_box_pack_start(GTK_BOX(table), image, FALSE, TRUE,0);
			/**
			 * The meta data printed out in alot of data
			 */
			table2 = gtk_table_new(8,2,FALSE);
			gtk_table_set_col_spacings(GTK_TABLE(table2),6);
			i=0;
			if(song->album) {
				info2_add_table_item(table2,_("<b>Album:</b>"), song->album,i);
				i++;
			}
			if(song->genre) {
				info2_add_table_item(table2, _("<b>Genre:</b>"), song->genre,i);
				i++;
			}
			if(song->date) {
				info2_add_table_item(table2, _("<b>Date:</b>"), song->date,i);
				i++;
			}
			if(tracks) {
				char *str = g_strdup_printf("%i", tracks);
				info2_add_table_item(table2, _("<b>Tracks:</b>"), str,i);
				q_free(str);
				i++;
			}
			if(song->file) {
				gchar *dirname = g_path_get_dirname(song->file);
				info2_add_table_item(table2,_("<b>Directory:</b>"),dirname,i);
				i++;
				q_free(dirname);
			}
			gtk_box_pack_start(GTK_BOX(table), table2, TRUE, TRUE,0);
			/**
			 * Play Button 
			 */
			hbox = gtk_vbox_new(FALSE,0);
			button = gtk_button_new_with_label(_("Replace"));
			gtk_button_set_image(GTK_BUTTON(button),gtk_image_new_from_stock(GTK_STOCK_REDO,GTK_ICON_SIZE_BUTTON));
			g_object_set_data_full(G_OBJECT(button), "artist",g_strdup(song->artist), g_free);
			g_object_set_data_full(G_OBJECT(button), "album",g_strdup(song->album), g_free);
			g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(as_album_clicked), GINT_TO_POINTER(1));
			gtk_button_set_relief(GTK_BUTTON(button), GTK_RELIEF_NONE);
			ali = gtk_alignment_new(0,0.5,0,0);
			gtk_container_add(GTK_CONTAINER(ali), button);
			gtk_box_pack_start(GTK_BOX(hbox), ali, FALSE,TRUE,0);
			/**
			 * Add Button 
			 */
			button = gtk_button_new_from_stock(GTK_STOCK_ADD);
			g_object_set_data_full(G_OBJECT(button), "artist",g_strdup(song->artist), g_free);
			g_object_set_data_full(G_OBJECT(button), "album",g_strdup(song->album), g_free);
			g_signal_connect(G_OBJECT(button), "clicked",G_CALLBACK(as_album_clicked),GINT_TO_POINTER(0));
			gtk_button_set_relief(GTK_BUTTON(button), GTK_RELIEF_NONE);
			ali = gtk_alignment_new(0,0.5,0,0);
			gtk_container_add(GTK_CONTAINER(ali), button);
			gtk_box_pack_start(GTK_BOX(hbox), ali, FALSE,TRUE,0);
			/**
			 * View Button
			 */
			button = gtk_button_new_with_label("View");
			gtk_button_set_image(GTK_BUTTON(button), gtk_image_new_from_stock(GTK_STOCK_FIND, GTK_ICON_SIZE_BUTTON));
			g_object_set_data_full(G_OBJECT(button), "artist",g_strdup(song->artist), g_free);
			g_object_set_data_full(G_OBJECT(button), "album",g_strdup(song->album), g_free);

			g_signal_connect(G_OBJECT(button), "clicked",G_CALLBACK(as_album_viewed_clicked),GINT_TO_POINTER(0));
			gtk_button_set_relief(GTK_BUTTON(button), GTK_RELIEF_NONE);
			ali = gtk_alignment_new(0,0.5,0,0);
			gtk_container_add(GTK_CONTAINER(ali), button);
			gtk_box_pack_start(GTK_BOX(hbox), ali, FALSE,TRUE,0);
			gtk_box_pack_start(GTK_BOX(table), hbox, FALSE, TRUE,0);

			event = gtk_event_box_new();
			gtk_widget_set_app_paintable(GTK_WIDGET(event), TRUE);
			g_signal_connect(G_OBJECT(event), "expose-event", G_CALLBACK(info2_row_expose_event), NULL);


			gtk_container_add(GTK_CONTAINER(event), table);
			gtk_box_pack_start(GTK_BOX(vbox1),event, FALSE,TRUE,0);
			/**
			 * Drag testing 
			 */

			gtk_drag_source_set(event, GDK_BUTTON1_MASK,target_table, 1, GDK_ACTION_COPY|GDK_ACTION_MOVE);
			g_signal_connect(G_OBJECT(event), "drag-data-get", G_CALLBACK(info2_album_drag_data_get), NULL);
			g_object_set_data_full(G_OBJECT(event), "artist",g_strdup(song->artist), g_free);
			g_object_set_data_full(G_OBJECT(event), "album",g_strdup(song->album), g_free);
			gtk_drag_source_set_icon_name(event, "gmpc-no-cover");


			mpd_data_free(data2);
		}
		gtk_container_set_border_width(GTK_CONTAINER(vbox1), 8);
		gtk_box_pack_start(GTK_BOX(resizer_vbox),vbox1,FALSE, TRUE, 0);
	}
	mpd_freeSong(song2);
	gtk_widget_show_all(info2_vbox);
}


/*****
 * View Album
 */
void info2_fill_album_view(char *artist,char *album)
{
	GtkWidget *vbox 	= NULL;
	GtkWidget *gmtv		= NULL;
	GtkWidget *expander = NULL;
	GtkWidget *button	= NULL;
	GtkWidget *ali 		= NULL;
	mpd_Song *song2 	= NULL;
	GtkWidget *label 	= NULL; 
	char *markup 		= NULL;

	info2_prepare_view();
	song2 = mpd_newSong();
	song2->artist = g_strdup(artist);
	song2->album = g_strdup(album);

	info2_widget_clear_children(title_vbox);


	/**
	 * Collection 
	 */
	button = gtk_button_new();
	label = gtk_label_new("");
	gtk_misc_set_alignment(GTK_MISC(label), 0,0.5);
	gtk_label_set_markup(GTK_LABEL(label),"<span size='xx-large' weight='bold'>Collection</span>");
	gtk_button_set_relief(GTK_BUTTON(button), GTK_RELIEF_NONE);
	g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(info2_fill_view), NULL);
	gtk_container_add(GTK_CONTAINER(button),label);
	gtk_widget_show_all(button);
	gtk_box_pack_start(GTK_BOX(title_vbox),button, FALSE, TRUE,0);
	/** 
	 * artist
	 */

	label = gtk_label_new("");
	gtk_misc_set_alignment(GTK_MISC(label), 0,0.5);
	gtk_label_set_markup(GTK_LABEL(label),"<span size='xx-large' weight='bold'>/</span>");
	gtk_box_pack_start(GTK_BOX(title_vbox), label, FALSE, TRUE,0);                        		
	button = gtk_button_new();
	label = gtk_label_new("");
	gtk_misc_set_alignment(GTK_MISC(label), 0,0.5);
	markup = g_markup_printf_escaped ("<span size=\"xx-large\" weight=\"bold\">%s</span>"
			, song2->artist);
	gtk_label_set_markup(GTK_LABEL(label),markup);
	gtk_button_set_relief(GTK_BUTTON(button), GTK_RELIEF_NONE);
	g_object_set_data_full(G_OBJECT(button), "artist",g_strdup(song2->artist), g_free);
	g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(as_artist_viewed_clicked), NULL);
	gtk_container_add(GTK_CONTAINER(button),label);
	gtk_widget_show_all(button);

	gtk_box_pack_start(GTK_BOX(title_vbox),button, FALSE, TRUE,0);
	q_free(markup);
	/**
	 * album
	 */		
	label = gtk_label_new("");
	gtk_misc_set_alignment(GTK_MISC(label), 0,0.5);
	gtk_label_set_markup(GTK_LABEL(label),"<span size='xx-large' weight='bold'>/</span>");
	gtk_box_pack_start(GTK_BOX(title_vbox), label, FALSE, TRUE,0);
	/* but title */
	markup =  g_markup_printf_escaped ("<span size=\"xx-large\" weight=\"bold\">%s</span>"
			, song2->album);
	label = gtk_label_new("");
	gtk_misc_set_alignment(GTK_MISC(label), 0,0.5);
	gtk_label_set_markup(GTK_LABEL(label),markup);
	gtk_label_set_ellipsize(GTK_LABEL(label), PANGO_ELLIPSIZE_END);
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(title_vbox), label, TRUE, TRUE,0);
	q_free(markup);

	/**
	 * Set album image
	 */
	GtkWidget *hbox = NULL;
	GtkWidget *table2,*table = gtk_table_new(2,2,FALSE);
	GtkWidget *image = NULL; 
	ali = gtk_alignment_new(0,0.5,1,0);
	gtk_container_set_border_width(GTK_CONTAINER(ali), 8);

	image = gmpc_metaimage_new(META_ALBUM_ART);
	gmpc_metaimage_set_size(GMPC_METAIMAGE(image), 150);
	gmpc_metaimage_set_draw_shadow(GMPC_METAIMAGE(image), TRUE);
	gmpc_metaimage_update_cover_from_song(GMPC_METAIMAGE(image), song2);


	gtk_table_attach(GTK_TABLE(table), image, 0,1,0,2,GTK_SHRINK|GTK_FILL, GTK_SHRINK|GTK_FILL,0,0);

	/** pack the table and add to view */
	gtk_container_add(GTK_CONTAINER(ali), table);
	gtk_box_pack_start(GTK_BOX(resizer_vbox), ali, FALSE, FALSE,0);



	vbox = gtk_vbox_new(FALSE, 6);

	/**
	 * Album Information 
	 */
	expander = gtk_expander_new(_("Album info:"));
	gmtv = gmpc_meta_text_view_new(META_ALBUM_TXT);
	gtk_text_view_set_left_margin(GTK_TEXT_VIEW(gmtv), 12);
	/* expander */
	gtk_expander_set_use_markup(GTK_EXPANDER(expander),TRUE);
	gtk_expander_set_label(GTK_EXPANDER(expander), _("<b>Album info:</b>"));
	/* query */
	gmpc_meta_text_view_query_text_from_song(GMPC_META_TEXT_VIEW(gmtv), song2);
	gtk_container_add(GTK_CONTAINER(expander), gmtv);
	gtk_box_pack_start(GTK_BOX(vbox), expander, TRUE,TRUE,0);	



	gtk_box_pack_start(GTK_BOX(resizer_vbox), vbox, FALSE, FALSE,0);

	/**
	 * Play Button 
	 */
	hbox = gtk_hbox_new(FALSE,0);
	button = gtk_button_new_with_label(_("Replace"));
	gtk_button_set_image(GTK_BUTTON(button),gtk_image_new_from_stock(GTK_STOCK_REDO,GTK_ICON_SIZE_BUTTON));
	g_object_set_data_full(G_OBJECT(button), "artist",g_strdup(artist), g_free);
	g_object_set_data_full(G_OBJECT(button), "album",g_strdup(album), g_free);
	g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(as_album_clicked), GINT_TO_POINTER(1));
	gtk_button_set_relief(GTK_BUTTON(button), GTK_RELIEF_NONE);
	ali = gtk_alignment_new(0,0.5,0,0);
	gtk_container_add(GTK_CONTAINER(ali), button);
	gtk_box_pack_start(GTK_BOX(hbox), ali, FALSE,TRUE,0);
	/**
	 * Add Button 
	 */
	button = gtk_button_new_from_stock(GTK_STOCK_ADD);
	g_object_set_data_full(G_OBJECT(button), "artist",g_strdup(artist), g_free);
	g_object_set_data_full(G_OBJECT(button), "album",g_strdup(album), g_free);
	g_signal_connect(G_OBJECT(button), "clicked",G_CALLBACK(as_album_clicked),GINT_TO_POINTER(0));
	gtk_button_set_relief(GTK_BUTTON(button), GTK_RELIEF_NONE);
	ali = gtk_alignment_new(0,0.5,0,0);
	gtk_container_add(GTK_CONTAINER(ali), button);
	gtk_box_pack_start(GTK_BOX(hbox), ali, FALSE,TRUE,0);



	table2 = gtk_table_new(2,2,0);
	gtk_container_set_border_width(GTK_CONTAINER(table2), 8);

	gtk_table_set_col_spacings(GTK_TABLE(table2), 6);

	gtk_table_attach(GTK_TABLE(table), table2, 1,2,0,1,GTK_EXPAND|GTK_FILL, GTK_EXPAND|GTK_FILL,0,0);
	gtk_table_attach(GTK_TABLE(table), hbox, 1,2,1,2,GTK_SHRINK|GTK_FILL, GTK_SHRINK|GTK_FILL,0,0);




	/**
	 * label
	 */
	label= gtk_label_new("");
	gtk_label_set_markup(GTK_LABEL(label), "<span size=\"x-large\" weight=\"bold\">Songs:</span>");

	gtk_misc_set_alignment(GTK_MISC(label), 0,0.5);
	gtk_misc_set_padding(GTK_MISC(label),8,8);

	gtk_box_pack_start(GTK_BOX(resizer_vbox), label, FALSE, FALSE,0);

	if(song2 && song2->artist)
	{
		MpdData *data = NULL;
		GString *string = NULL;
		mpd_Song *cursong = NULL;
		int i=1;
		int tracks = 0;
		/** Album name */
		if(song2->album){
			info2_add_table_item(table2, _("<b>Album:</b>"), song2->album, i);
			i++;
		}
		/**
		 * Do Genre Search 
		 */
		mpd_database_search_field_start(connection, MPD_TAG_ITEM_GENRE);
		mpd_database_search_add_constraint(connection, MPD_TAG_ITEM_ARTIST, song2->artist);
		mpd_database_search_add_constraint(connection, MPD_TAG_ITEM_ALBUM,song2->album);
		string = g_string_new("");
		for(data = mpd_database_search_commit(connection);data != NULL ;data= mpd_data_get_next(data))
		{
			g_string_append_printf(string, "%s%s",data->tag, (mpd_data_is_last(data))?"":", "); 
		}
		if(string->len >0)
		{
			info2_add_table_item(table2, _("<b>Genre:</b>"), string->str, i);
			i++;
		}
		g_string_free(string, TRUE);



		mpd_database_search_start(connection, TRUE);
		mpd_database_search_add_constraint(connection, MPD_TAG_ITEM_ARTIST, song2->artist);
		mpd_database_search_add_constraint(connection, MPD_TAG_ITEM_ALBUM,song2->album);
		data = mpd_database_search_commit(connection);
		if(data)
		{
			MpdData *data2;
			mpd_Song *song = data->song;
			for(data2 = mpd_data_get_first(data);!mpd_data_is_last(data2);data2= mpd_data_get_next(data2)) tracks++;
			tracks++;

			if(song->date) {
				info2_add_table_item(table2, _("<b>Date:</b>"), song->date, i);
				i++;
			}
			if(tracks) {
				char *str = g_strdup_printf("%i", tracks);
				info2_add_table_item(table2, _("<b>Tracks:</b>"), str, i);
				q_free(str);
				i++;
			}
			if(song->file) {
				char *filename = g_path_get_dirname(song->file);
				info2_add_table_item(table2, _("<b>Directory:</b>"), filename, i);
				q_free(filename);
				i++;
			}
		}

		table2 = gtk_table_new(2, tracks,0);
		/**
		 * set a table
		 */
		gtk_table_set_col_spacings(GTK_TABLE(table2), 0);
		gtk_table_set_row_spacings(GTK_TABLE(table2), 0);
		i=1;
		cursong = mpd_playlist_get_current_song(connection);
		for(data = mpd_data_get_first(data);data;data = mpd_data_get_next(data))
		{
			markup =  g_strdup_printf("%02i: %s",i, data->song->title);
			label = gmpc_clicklabel_new(markup);
			if(cursong && strcmp(data->song->file,cursong->file) == 0)
			{
				gmpc_clicklabel_set_do_italic(GMPC_CLICKLABEL(label), TRUE);
			}	
			g_object_set_data_full(G_OBJECT(label), "file",g_strdup(data->song->file), g_free);
			g_signal_connect(G_OBJECT(label), "clicked", G_CALLBACK(as_song_viewed_clicked), GINT_TO_POINTER(1));
			gtk_table_attach(GTK_TABLE(table2), label, 0,1,i,i+1,GTK_EXPAND|GTK_FILL, GTK_SHRINK|GTK_FILL,0,0);
			q_free(markup);
			i++;
		}
		ali = gtk_alignment_new(0,0.5,1,0);
		gtk_alignment_set_padding(GTK_ALIGNMENT(ali), 0,0,10,0);
		gtk_container_add(GTK_CONTAINER(ali), table2);
		gtk_box_pack_start(GTK_BOX(resizer_vbox),ali,FALSE, TRUE, 0);

	}
	mpd_freeSong(song2);
	gtk_widget_show_all(info2_vbox);
}

static void info2_init()
{
	GtkWidget *vp = NULL;;
	GtkWidget *ali/*,*event*/,*vbox;
	/**
	 * main widget used to pack the browser
	 */
	info2_vbox = gtk_frame_new(NULL);


	gtk_widget_set_name(info2_vbox, "gtk_scrolled_window");
	gtk_frame_set_shadow_type(GTK_FRAME(info2_vbox), GTK_SHADOW_ETCHED_IN);
	vbox = gtk_vbox_new(FALSE, 0);

	gtk_container_add(GTK_CONTAINER(info2_vbox), vbox);

	/**
	 * Header 
	 */

	title_event = gtk_event_box_new();
	title_vbox = gtk_hbox_new(FALSE,6);
	ali = gtk_alignment_new(0,0.5,1,1);
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



	gtk_widget_show_all(info2_vbox);

	g_object_ref(G_OBJECT(info2_vbox));
	info2_fill_view();
}

static void info2_add(GtkWidget *cat_tree)
{
	GtkTreePath *path = NULL;
	GtkTreeStore *pl3_tree = (GtkTreeStore *)gtk_tree_view_get_model(GTK_TREE_VIEW(cat_tree));	
	GtkTreeIter iter;
	if(!cfg_get_single_value_as_int_with_default(config, "info2-plugin", "enable", 1)) return;
	gtk_tree_store_append(pl3_tree, &iter, NULL);
	gtk_tree_store_set(pl3_tree, &iter, 
			PL3_CAT_TYPE, metab_plugin.id,
			PL3_CAT_TITLE, _("Metadata Browser"),
			PL3_CAT_INT_ID, "/",
			PL3_CAT_ICON_ID, "gtk-info",
			PL3_CAT_PROC, TRUE,
			PL3_CAT_ICON_SIZE,GTK_ICON_SIZE_DND,-1);
	if (info2_ref) {
		gtk_tree_row_reference_free(info2_ref);
		info2_ref = NULL;
	}

	path = gtk_tree_model_get_path(GTK_TREE_MODEL(playlist3_get_category_tree_store()), &iter);
	if (path) {
		info2_ref = gtk_tree_row_reference_new(GTK_TREE_MODEL(playlist3_get_category_tree_store()), path);
		gtk_tree_path_free(path);
	}
}

static void info2_selected(GtkWidget *container)
{
	if(info2_vbox== NULL) {
		info2_init();
	}
	gtk_container_add(GTK_CONTAINER(container), info2_vbox);
	gtk_widget_show_all(info2_vbox);

}

static void info2_unselected(GtkWidget *container)
{
	gtk_container_remove(GTK_CONTAINER(container),info2_vbox);
}

static void info2_set_enabled(int enabled)
{
	cfg_set_single_value_as_int(config, "info2-plugin", "enable", enabled);
	if (enabled)
	{
		if(info2_ref == NULL)
		{
			info2_add(GTK_WIDGET(playlist3_get_category_tree_view()));
		}
	}
	else if (info2_ref)
	{
		GtkTreePath *path = gtk_tree_row_reference_get_path(info2_ref);
		if (path){
			GtkTreeIter iter;
			if (gtk_tree_model_get_iter(GTK_TREE_MODEL(playlist3_get_category_tree_store()), &iter, path)){
				gtk_tree_store_remove(playlist3_get_category_tree_store(), &iter);
			}
			gtk_tree_path_free(path);
			gtk_tree_row_reference_free(info2_ref);
			info2_ref = NULL;
		}                                                                                                  	
	}                                                                                                      	
	pl3_update_go_menu();
}

static int info2_get_enabled()
{
	return 	cfg_get_single_value_as_int_with_default(config, "info2-plugin", "enable", 1);
}


void info2_activate(void)
{
	GtkTreeSelection *selec = gtk_tree_view_get_selection((GtkTreeView *)
			playlist3_get_category_tree_view());	

	/**
	 * Fix this to be nnot static
	 */	
	GtkTreePath *path = gtk_tree_row_reference_get_path(info2_ref);
	if(path)
	{
		gtk_tree_selection_select_path(selec, path);
		gtk_tree_path_free(path);
	}
}

static int info2_add_go_menu(GtkWidget *menu)
{
	GtkWidget *item = NULL;
	if(!cfg_get_single_value_as_int_with_default(config, "info2-plugin", "enable", 1)) return 0;
	item = gtk_image_menu_item_new_with_label(_("Metadata Browser"));
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item), 
			gtk_image_new_from_icon_name("gtk-info", GTK_ICON_SIZE_MENU));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	g_signal_connect(G_OBJECT(item), "activate", 
			G_CALLBACK(info2_activate), NULL);
	return 1;
}

static int info2_key_press_event(GtkWidget *mw, GdkEventKey *event, int type)
{
	/** Global keybinding */
	if (event->keyval == GDK_F7)
	{
		info2_activate();
		info2_fill_view();
		return TRUE;
	}
	return FALSE;
}
