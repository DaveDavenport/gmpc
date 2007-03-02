#include <gdk/gdkkeysyms.h>
#include <stdio.h>
#include <string.h>
#include <regex.h>
#include "main.h"
#include "misc.h"

extern GladeXML *pl3_xml;

static void info2_add(GtkWidget *);
static void info2_selected(GtkWidget *);
static void info2_unselected(GtkWidget *);
static int info2_add_go_menu(GtkWidget *);
static int info2_get_enabled(void);
static void info2_set_enabled(int);
static void remove_container_entries(GtkContainer *);
static void info2_fill_artist_view(char *);
static void info2_fill_album_view(char *, char *);
static void info2_fill_song_view(char *);
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

static int current_id = 0;

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

static void
resize_table (GtkTable * table, gint columns, GList * launcher_list)
{
	remove_container_entries (GTK_CONTAINER (table));
	if(!columns)
		return;
	float rows =
		((float) g_list_length (launcher_list)) / (float) columns;
	float remainder = rows - ((int) rows);

	if (remainder != 0.0)
		rows += 1;

	gtk_table_resize (table, (int) rows, columns);
}

static void
relayout_table (GtkTable * table, GList * element_list)
{
	gint maxcols = (GTK_TABLE (table))->ncols;
	gint row = 0, col = 0;

	do {
		GtkWidget *element = GTK_WIDGET (element_list->data);

		gtk_table_attach (table, element, col, col + 1, row, row + 1,
				  GTK_SHRINK | GTK_FILL,
				  GTK_SHRINK | GTK_FILL, 0, 0);
		col++;
		if (col == maxcols) {
			col = 0;
			row++;
		}
	}
	while (NULL != (element_list = g_list_next (element_list)));
}
static void info2_widget_clear_children(GtkWidget *wid)
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

static void info2_prepare_view()
{
	GtkAdjustment *h = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(scrolled_window));
	info2_widget_clear_children(resizer_vbox);
	gtk_adjustment_set_value(h, 0.0);
	
	/**
	 *  new id
	 */
	
	current_id = g_random_int();
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


static void info2_cover_txt_fetched(mpd_Song *song,MetaDataResult ret, char *path,PassData *pd)
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

		label = gtk_label_new("");
/*		ali = gtk_alignment_new(0,0.5,0,0);
*/		gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);		
		gtk_misc_set_alignment(GTK_MISC(label), 0,0.5);
		gtk_misc_set_padding(GTK_MISC(label), 6,6);
/*		gtk_alignment_set_padding(GTK_ALIGNMENT(ali),0,0,6,0);
		gtk_container_add(GTK_CONTAINER(ali), label);
*/		gtk_container_add(GTK_CONTAINER(expander), label);		
		g_file_get_contents(path, &content, &size,NULL);
		gtk_label_set_text(GTK_LABEL(label), content);
		gtk_label_set_selectable(GTK_LABEL(label), TRUE);
		gtk_widget_show_all(vbox);
		q_free(content);
	}
	else if(ret == META_DATA_UNAVAILABLE)
	{
		GtkWidget *label = NULL;
		gchar *labstr= g_strdup_printf(_("<i>No %s found</i>"), pd->name);
		remove_container_entries(GTK_CONTAINER(vbox));
		label = gtk_label_new("");
		gtk_misc_set_alignment(GTK_MISC(label),0,0.5);
		gtk_label_set_markup(GTK_LABEL(label), labstr);
		gtk_container_add(GTK_CONTAINER(vbox), label);
		gtk_widget_show_all(vbox);
		q_free(labstr);
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
	GtkWidget *label/*, *ali*/;
	label = gtk_label_new("");
	gtk_label_set_markup(GTK_LABEL(label), name);
/*	ali = gtk_alignment_new(0,0.5,0,0);
	gtk_container_add(GTK_CONTAINER(ali), label);
*/
	gtk_misc_set_alignment(GTK_MISC(label),0,0.5);
	gtk_table_attach(GTK_TABLE(table), label,0,1,i,i+1,GTK_SHRINK|GTK_FILL, GTK_SHRINK|GTK_FILL,0,0);
	label = gtk_label_new(value);
	gtk_misc_set_alignment(GTK_MISC(label),0,0.5);
/*	ali = gtk_alignment_new(0,0.5,1,0);*/
	gtk_label_set_selectable(GTK_LABEL(label), TRUE);
/*	gtk_container_add(GTK_CONTAINER(ali), label);*/
	gtk_table_attach(GTK_TABLE(table),label,1,2,i,i+1,GTK_EXPAND|GTK_FILL, GTK_SHRINK|GTK_FILL,0,0);
	gtk_label_set_ellipsize(GTK_LABEL(label), PANGO_ELLIPSIZE_END);
}

/**
 * Create an artist "button"
 */

static GtkWidget *info2_create_artist_button(mpd_Song *song)
{
	GtkWidget *label,*button,*event;
	GtkWidget *table;
	GtkWidget *metaimage;
	/* Button bg drawing code */
	event = gtk_event_box_new();
	gtk_widget_set_app_paintable(GTK_WIDGET(event), TRUE);
	gtk_widget_set_app_paintable(GTK_WIDGET(event), TRUE);
	g_signal_connect(G_OBJECT(event), "expose-event", G_CALLBACK(info2_row_expose_event), NULL);

	/** Create table */
	table = gtk_table_new(2,2, FALSE); 
	gtk_container_set_border_width(GTK_CONTAINER(table),4);

	/** Create artist image */	
	metaimage = gmpc_metaimage_new(META_ARTIST_ART);
	gmpc_metaimage_set_size(GMPC_METAIMAGE(metaimage), 64);
	gmpc_metaimage_update_cover_from_song(GMPC_METAIMAGE(metaimage), song);
	gtk_table_attach(GTK_TABLE(table), metaimage, 0,1,0,2,GTK_SHRINK|GTK_FILL, GTK_SHRINK|GTK_FILL,0,0);

	/** Label */
	label = gtk_label_new(song->artist);
	gtk_label_set_ellipsize(GTK_LABEL(label),PANGO_ELLIPSIZE_END);
	gtk_misc_set_alignment(GTK_MISC(label), 0,0.5);	
	gtk_table_attach(GTK_TABLE(table), label, 1,2,0,1,GTK_SHRINK|GTK_FILL, GTK_SHRINK|GTK_FILL,6,0);	

	/** Button box */
	label = gtk_hbox_new(FALSE,6);
	gtk_table_attach(GTK_TABLE(table), label, 1,2,1,2,GTK_EXPAND|GTK_FILL, GTK_SHRINK/*|GTK_FILL*/,6,0);
	/* Play button */
	button = gtk_button_new_with_label(_("Replace"));
	gtk_button_set_image(GTK_BUTTON(button), gtk_image_new_from_stock(GTK_STOCK_REDO,GTK_ICON_SIZE_BUTTON));
	gtk_button_set_relief(GTK_BUTTON(button), GTK_RELIEF_NONE);
	g_object_set_data_full(G_OBJECT(button), "artist",g_strdup(song->artist), g_free);
	g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(as_artist_clicked), GINT_TO_POINTER(1));
	gtk_box_pack_start(GTK_BOX(label), button,FALSE,TRUE,0);
	/* Add */
	button = gtk_button_new_from_stock(GTK_STOCK_ADD);
	gtk_button_set_relief(GTK_BUTTON(button), GTK_RELIEF_NONE);
	g_object_set_data_full(G_OBJECT(button), "artist",g_strdup(song->artist), g_free);
	g_signal_connect(G_OBJECT(button), "clicked",G_CALLBACK(as_artist_clicked),GINT_TO_POINTER(0));
	gtk_box_pack_start(GTK_BOX(label),button,FALSE,TRUE,0);
	/* View */
	button = gtk_button_new_with_label("View");
	gtk_button_set_image(GTK_BUTTON(button), gtk_image_new_from_stock(GTK_STOCK_FIND, GTK_ICON_SIZE_BUTTON));
	gtk_button_set_relief(GTK_BUTTON(button), GTK_RELIEF_NONE);
	g_object_set_data_full(G_OBJECT(button), "artist",g_strdup(song->artist), g_free);
	g_signal_connect(G_OBJECT(button), "clicked",G_CALLBACK(as_artist_viewed_clicked),NULL);
	gtk_box_pack_start(GTK_BOX(label), button, FALSE,TRUE,0);


	/** Add button to bg container */
	gtk_container_add(GTK_CONTAINER(event), table);
	return event;
}

/** 
 * Song View 
 */
static void info2_fill_song_view(char *path) 
{
	GtkWidget *ali = NULL;
	GtkWidget *vbox= NULL;
	mpd_Song *song = NULL;
	GtkWidget *button = NULL;
	GtkWidget *label = NULL;
	PassData *pd = NULL;
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
	markup = g_markup_printf_escaped ("<span size=\"xx-large\" weight=\"bold\">%s</span>"
			, song->artist);
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
	markup =  g_markup_printf_escaped ("<span size=\"xx-large\" weight=\"bold\">%s</span>"
			, song->album);

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
/*	ali = gtk_alignment_new(0,0.5,0,0);*/
/*	gtk_container_set_border_width(GTK_CONTAINER(ali), 8);*/
	markup =  g_markup_printf_escaped ("<span size=\"xx-large\" weight=\"bold\" style=\"italic\">%s</span>"
			, song->title);
	gtk_label_set_markup(GTK_LABEL(label),markup);
	gtk_misc_set_alignment(GTK_MISC(label), 0,0.5);
	gtk_misc_set_padding(GTK_MISC(label),8,8);
	q_free(markup);
/*	gtk_container_add(GTK_CONTAINER(ali),label);*/
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



	vbox = gtk_vbox_new(FALSE, 6);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 6);
	pd = g_malloc0(sizeof(*pd));
	pd->widget = vbox;
	pd->id = current_id;     		
	pd->name = g_strdup(_("lyric"));
	meta_data_get_path_callback(song, META_SONG_TXT, (MetaDataCallback)info2_cover_txt_fetched, pd);
	gtk_box_pack_start(GTK_BOX(resizer_vbox), vbox, FALSE, FALSE,0);

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
	button = gtk_button_new_from_stock(GTK_STOCK_ADD);
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

		if(mpd_server_check_command_allowed(connection, "addid") == MPD_SERVER_COMMAND_ALLOWED){
			int songid = mpd_playlist_add_get_id(connection, file);
			if(songid >= 0) {
				mpd_player_play_id(connection, songid);
			}
		} else{
			mpd_playlist_add(connection, file);
			if(clear)
			{
				mpd_player_play(connection);
			}
		}
	}
}

static gboolean info2_row_expose_event(GtkWidget *widget, GdkEventExpose *event, gpointer data)
{

	GdkColor *color = NULL;
	cairo_pattern_t *pat;
	cairo_t *cr= gdk_cairo_create(GTK_WIDGET(widget)->window);
	int width = widget->allocation.width;
	int height = widget->allocation.height;
	
	gdk_cairo_set_source_color(cr, 	&(widget->style->base[GTK_STATE_NORMAL]));
	cairo_rectangle(cr,0,0,width,height);
	cairo_fill(cr);


	cairo_set_line_width (cr, 1.5);

	cairo_rectangle(cr, 0,0,width,height);
	
	cairo_close_path (cr);
	pat = cairo_pattern_create_linear (width/2, 0.0,width/2, height);
	color = &(widget->style->base[GTK_STATE_SELECTED]);
	cairo_pattern_add_color_stop_rgba (pat, 1, color->red/65535.0, color->green/65535.0, color->blue/65535.0, 1);
	color = &(widget->style->base[GTK_STATE_NORMAL]);
	cairo_pattern_add_color_stop_rgba (pat, 0, color->red/65535.0, color->green/65535.0, color->blue/65535.0, 1);
	cairo_set_source (cr, pat);

	cairo_fill_preserve (cr);
	gdk_cairo_set_source_color(cr, 	&(widget->style->dark[GTK_STATE_SELECTED]));
	cairo_stroke (cr);
	cairo_pattern_destroy(pat);
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
	GList *list = NULL;
	GtkTreeModel *model =gtk_entry_completion_get_model(GTK_ENTRY_COMPLETION(entry_completion));
	GtkTreeIter iter;
	/**
	 * Remove all the remaining widgets in the view
	 */
	remove_container_entries(GTK_CONTAINER(table));

	/**
	 *  new id
	 */
	current_id = g_random_int();

	/** get text
	*/
	text = gtk_entry_get_text(entry);
	if(strlen(text) && !regcomp(&regt, text, REG_EXTENDED|REG_ICASE|REG_NOSUB))
	{
		int skip = 0;
		int num_cols = 2;
		int songs = 0;
		int tile_size = 310;
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
		
			if(songs < 50 && !regexec(&regt,data->tag, 0,NULL,0))
			{
				GtkWidget *button;
				song->artist = data->tag;
				/* Create button */
				button = info2_create_artist_button(song);
				/* Add button to list (so it gets added to the table) */
				list = g_list_append(list, button);				
				/* cleanup pointer*/
				song->artist = NULL;	
				songs++;
			}

		}
		/* if there is an "overflow" show a message */
		if(songs >= 50)
		{
			GtkWidget *box = gtk_hbox_new(FALSE, 6);
			GtkWidget *temp = gtk_image_new_from_stock(GTK_STOCK_DIALOG_WARNING, GTK_ICON_SIZE_DIALOG);
			gtk_box_pack_start(GTK_BOX(box), temp, FALSE, TRUE, 0);
			temp = gtk_label_new(_("More then 50 results found, please refine your search query"));
			gtk_label_set_line_wrap(GTK_LABEL(temp), TRUE);
			gtk_box_pack_start(GTK_BOX(box), temp, TRUE, TRUE, 0);
			num_cols = 1;
			info2_create_artist_button(song);
			
			g_list_foreach(list, gtk_widget_destroy, NULL);
			g_list_free(list);
			list = NULL;
			list = g_list_append(list, box);
		}
		mpd_freeSong(song);
		if(list)
		{
			resize_table(GTK_TABLE(table), num_cols, list);
			relayout_table(GTK_TABLE(table), list);
			g_list_free(list);
		}

		regfree(&regt);
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

	artist_table = gtk_table_new(1,1, TRUE);
	gtk_table_set_row_spacings(GTK_TABLE(artist_table), 6);
	gtk_table_set_col_spacings(GTK_TABLE(artist_table), 6);
	gtk_container_set_border_width(GTK_CONTAINER(artist_table), 6);
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

static void info2_fill_artist_view(char *artist)
{
	GtkWidget *vbox = NULL;
	int i=0,items=0;
	GString *string = NULL;
	MpdData *data = NULL;
	GtkWidget *ali = NULL, *table2 = NULL;
	mpd_Song *song2 = NULL;
	PassData *pd = NULL;
	info2_prepare_view();
	song2 = mpd_newSong();
	song2->artist = g_strdup(artist);

	/**
	 * Set artist image
	 */
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
	pd = g_malloc0(sizeof(*pd));
	pd->widget = vbox;
	pd->id = current_id;     		
	pd->name = g_strdup(_("artist info"));
	meta_data_get_path_callback(song2, META_ARTIST_TXT, (MetaDataCallback)info2_cover_txt_fetched, pd);
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
/*	ali = gtk_alignment_new(0,0.5,0,0);*/
	gtk_misc_set_alignment(GTK_MISC(label), 0,0.5);
	gtk_misc_set_padding(GTK_MISC(label),8,8);
/*	gtk_container_set_border_width(GTK_CONTAINER(ali), 8);
	gtk_container_add(GTK_CONTAINER(ali), label);
*/	gtk_box_pack_start(GTK_BOX(resizer_vbox), label, FALSE, FALSE,0);



	if(song2 && song2->artist)
	{
		GList *list = NULL;
		int num_cols = 2;
		int tile_size = 450;
		int i=0;

		/**
		 * and event box, so I can draw a background color 
		 */
		GtkWidget *button = NULL;
		GtkWidget *ali = NULL;
		GtkWidget *table2 = gtk_table_new(1,1,TRUE);
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
		 * set a table
		 */
		gtk_table_set_col_spacings(GTK_TABLE(table2), 6);
		gtk_table_set_row_spacings(GTK_TABLE(table2), 6);
		/**
		 * Image
		 */
		MpdData *data = mpd_database_get_albums(connection, song2->artist);
		for(;data;data = mpd_data_get_next(data))
		{
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
			GtkWidget *table, *image,*event;
			table = gtk_table_new(2,2,FALSE);
			gtk_table_set_col_spacings(GTK_TABLE(table),6);
			image = gmpc_metaimage_new(META_ALBUM_ART);
			gmpc_metaimage_set_size(GMPC_METAIMAGE(image), 120);
			gmpc_metaimage_update_cover_from_song(GMPC_METAIMAGE(image), song);

			gtk_container_set_border_width(GTK_CONTAINER(table), 8);
			gtk_table_attach(GTK_TABLE(table), image, 0,1,0,2,GTK_SHRINK|GTK_FILL, GTK_SHRINK|GTK_FILL,0,0);

			/**
			 * The meta data printed out in alot of data
			 */
			table2 = gtk_table_new(8,2,FALSE);
			gtk_table_set_col_spacings(GTK_TABLE(table2),6);
			i=0;
			if(song->album)
			{
				info2_add_table_item(table2,_("<b>Album:</b>"), song->album,i);
				i++;
			}
			if(song->genre)
			{
				info2_add_table_item(table2, _("<b>Genre:</b>"), song->genre,i);
				i++;
			}

			if(song->date)
			{
				info2_add_table_item(table2, _("<b>Date:</b>"), song->date,i);
				i++;
			}
			if(tracks)
			{
				char *str = g_strdup_printf("%i", tracks);
				info2_add_table_item(table2, _("<b>Tracks:</b>"), str,i);
				q_free(str);
				i++;
			}
			if(song->file)
			{
				gchar *dirname = g_path_get_dirname(song->file);
				info2_add_table_item(table2,_("<b>Directory:</b>"),dirname,i);
				i++;
				q_free(dirname);
			}
			gtk_table_attach(GTK_TABLE(table), table2, 1,2,0,1,GTK_EXPAND|GTK_FILL, GTK_SHRINK/*|GTK_FILL*/,6,0);
			/**
			 * Play Button 
			 */
			GtkWidget *hbox = gtk_hbox_new(FALSE,6);
			GtkWidget *button = gtk_button_new_with_label(_("Replace"));
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
			gtk_table_attach(GTK_TABLE(table), hbox, 1,2,1,2,GTK_EXPAND|GTK_FILL, GTK_SHRINK/*|GTK_FILL*/,6,0);

			event = gtk_event_box_new();
			gtk_widget_set_app_paintable(GTK_WIDGET(event), TRUE);
			g_signal_connect(G_OBJECT(event), "expose-event", G_CALLBACK(info2_row_expose_event), NULL);

			ali = gtk_alignment_new(0,0.5,0,0);
			gtk_alignment_set_padding(GTK_ALIGNMENT(ali), 0,0,10,10);
			gtk_widget_set_size_request(event, tile_size,-1);
			gtk_container_add(GTK_CONTAINER(ali), event);


			gtk_container_add(GTK_CONTAINER(event), table);



			list = g_list_append(list, ali);
			mpd_data_free(data2);
		}

		gtk_box_pack_start(GTK_BOX(resizer_vbox),table2,FALSE, TRUE, 0);
		num_cols = (resizer_vbox->allocation.width-20)/(tile_size+6);
		if(list)
		{
			resize_table(GTK_TABLE(table2), num_cols, list);
			relayout_table(GTK_TABLE(table2), list);
			g_list_free(list);
		}                                           		


	}
	mpd_freeSong(song2);
	gtk_widget_show_all(info2_vbox);
}


/*****
 * View Album
 */
static void info2_fill_album_view(char *artist,char *album)
{
	GtkWidget *button = NULL;
	GtkWidget *ali = NULL;
	mpd_Song *song2 = NULL;
	PassData *pd = NULL;
	GtkWidget *label = NULL; 
	char *markup = NULL;

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



	GtkWidget *vbox = gtk_vbox_new(FALSE, 6);
	pd = g_malloc0(sizeof(*pd));
	pd->widget = vbox;
	pd->id = current_id;     		
	pd->name = g_strdup(_("album info"));
	meta_data_get_path_callback(song2, META_ALBUM_TXT, (MetaDataCallback)info2_cover_txt_fetched, pd);
	gtk_box_pack_start(GTK_BOX(resizer_vbox), vbox, FALSE, FALSE,0);

	/**
	 * Play Button 
	 */
	hbox = gtk_hbox_new(FALSE,6);
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
/*	ali = gtk_alignment_new(0,0.5,0,0);
	gtk_container_set_border_width(GTK_CONTAINER(ali), 8);
	gtk_container_add(GTK_CONTAINER(ali), label);
*/
	gtk_misc_set_alignment(GTK_MISC(label), 0,0.5);
	gtk_misc_set_padding(GTK_MISC(label),8,8);

	gtk_box_pack_start(GTK_BOX(resizer_vbox), label, FALSE, FALSE,0);

	if(song2 && song2->artist)
	{
		MpdData *data = NULL;
		GString *string = NULL;
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

		table2 = gtk_table_new(4, tracks,0);
		/**
		 * set a table
		 */
		gtk_table_set_col_spacings(GTK_TABLE(table2), 0);
		gtk_table_set_row_spacings(GTK_TABLE(table2), 0);
		i=1;
		for(data = mpd_data_get_first(data);data;data = mpd_data_get_next(data))
		{
			markup =  g_markup_printf_escaped ("%i:\t%s",
					i, data->song->title);
			label = gtk_label_new("");
			gtk_label_set_markup(GTK_LABEL(label),markup);
			gtk_misc_set_alignment(GTK_MISC(label), 0,0.5);
			gtk_table_attach(GTK_TABLE(table2), label, 0,1,i,i+1,GTK_SHRINK|GTK_FILL, GTK_SHRINK|GTK_FILL,0,0);
			button = gtk_button_new();
			gtk_button_set_relief(GTK_BUTTON(button), GTK_RELIEF_NONE);
			gtk_button_set_image(GTK_BUTTON(button), gtk_image_new_from_stock(GTK_STOCK_REDO, GTK_ICON_SIZE_BUTTON));
			g_object_set_data_full(G_OBJECT(button), "file",g_strdup(data->song->file), g_free);
			g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(as_song_clicked), GINT_TO_POINTER(1));
			gtk_table_attach(GTK_TABLE(table2), button, 1,2,i,i+1,GTK_SHRINK|GTK_FILL, GTK_SHRINK|GTK_FILL,0,0);
			button = gtk_button_new();
			gtk_button_set_relief(GTK_BUTTON(button), GTK_RELIEF_NONE);
			gtk_button_set_image(GTK_BUTTON(button), gtk_image_new_from_stock(GTK_STOCK_ADD, GTK_ICON_SIZE_BUTTON));
			g_object_set_data_full(G_OBJECT(button), "file",g_strdup(data->song->file), g_free);
			g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(as_song_clicked), GINT_TO_POINTER(0));
			gtk_table_attach(GTK_TABLE(table2), button, 2,3,i,i+1,GTK_SHRINK|GTK_FILL, GTK_SHRINK|GTK_FILL,0,0);
			button = gtk_button_new();
			gtk_button_set_relief(GTK_BUTTON(button), GTK_RELIEF_NONE);
			gtk_button_set_image(GTK_BUTTON(button), gtk_image_new_from_stock(GTK_STOCK_FIND, GTK_ICON_SIZE_BUTTON));
			gtk_table_attach(GTK_TABLE(table2), button, 3,4,i,i+1,GTK_SHRINK|GTK_FILL, GTK_SHRINK|GTK_FILL,0,0);
			g_object_set_data_full(G_OBJECT(button), "file",g_strdup(data->song->file), g_free);
			g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(as_song_viewed_clicked), GINT_TO_POINTER(1));

			q_free(markup);
			i++;
		}
		ali = gtk_alignment_new(0,0.5,0,0);
		gtk_alignment_set_padding(GTK_ALIGNMENT(ali), 0,0,10,0);
		gtk_container_add(GTK_CONTAINER(ali), table2);
		gtk_box_pack_start(GTK_BOX(resizer_vbox),ali,FALSE, TRUE, 0);

	}
	mpd_freeSong(song2);
	gtk_widget_show_all(info2_vbox);
}

static void pl3_metabrowser_bg_style_changed(GtkWidget *vbox, GtkStyle *style,  GtkWidget *vp)
{
	gtk_widget_modify_bg(vp,GTK_STATE_NORMAL, &(GTK_WIDGET(vbox)->style->base[GTK_STATE_NORMAL]));
}
static void pl3_metabrowser_header_style_changed(GtkWidget *vbox, GtkStyle *style,  GtkWidget *vp)
{
	gtk_widget_modify_bg(vp,GTK_STATE_NORMAL, &(GTK_WIDGET(vbox)->style->light[GTK_STATE_SELECTED]));
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


static void info2_activate()
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
			gtk_image_new_from_stock("gtk-info", GTK_ICON_SIZE_MENU));
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
