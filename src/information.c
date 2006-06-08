#include <stdio.h>
#include <string.h>
#include <glade/glade.h>
#include <gdk/gdkkeysyms.h>
#include "main.h"
#include "plugin.h"
#include "misc.h"


/** TODO: Cleanup */

GtkWidget *info_pref_vbox = NULL;
void info_add(GtkWidget *cat_tree);
void info_selected(GtkWidget *container);
void info_unselected(GtkWidget *container);
void info_changed(GtkWidget *tree, GtkTreeIter *iter);


void show_artist_info(mpd_Song *song);
void show_album_info(mpd_Song *song);


void info_clear_display();


GtkWidget *info_text_view = NULL;
GtkWidget *info_vbox = NULL;
GtkWidget *info_pimage = NULL;
GdkPixbuf *info_spinner_pixbuf = NULL;
int spin_offset = 0;

void info_construct(GtkWidget *container);
void info_destroy(GtkWidget *container);

gboolean hovering_over_link = FALSE;
GdkCursor *hand_cursor = NULL;
GdkCursor *regular_cursor = NULL;


GList *tag_list = NULL;
static mpd_Song *current_song = NULL;
static guint32 current_id = 0;
static guint32 info_fetching = 0;

typedef struct {
	GtkTextMark *mark;
	guint32 id;
}info_pass_data ;

extern GladeXML *pl3_xml;
void info_status_changed(MpdObj *mi, ChangedStatusType what,gpointer data);
int info_key_press_event(GtkWidget *mw, GdkEventKey *event, int type);
int info_add_go_menu(GtkWidget *menu);
/*
GtkWidget *info_tooltip = NULL;
GtkWidget *info_label = NULL;
*/


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
	NULL,
	info_add_go_menu,
	info_key_press_event
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
	info_status_changed,
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

static void info_show_current_artist() {
	mpd_Song *song = mpd_playlist_get_current_song(connection);
	if(song){
		show_artist_info(song);
	}
}
static void info_show_current_album() {
	mpd_Song *song = mpd_playlist_get_current_song(connection);
	if(song){
		show_album_info(song);
	}
}


void info_fetching_changed()
{
	printf("%i\n",info_fetching);
	if(info_fetching)
	{
		gtk_widget_show(info_pimage);

	}
	else
	{
		gtk_widget_hide(info_pimage);
	}
}




void info_changed(GtkWidget *tree, GtkTreeIter *iter)
{

}
void info_status_changed(MpdObj *mi, ChangedStatusType what, gpointer data)
{
	/**
	 * Todo Make this optional
	 */
	if(what&MPD_CST_SONGID)
	{
		if(info_text_view)
		{
//			info_show_current_song();
		}
	}

}


void info_cover_art_fetched(mpd_Song *song,MetaDataResult ret, char *path,gpointer data)
{
	info_pass_data	*ipd = data;
	GtkTextMark *mark = ipd->mark;
	GtkTextIter iter,start,stop;
	GtkTextBuffer *buffer = gtk_text_mark_get_buffer(mark);
	/** Don't do anything when still fetching */
	if(ret == META_DATA_FETCHING) return;
	info_fetching--;
	info_fetching_changed();


	/* check if where checking for the correct song */
	if(!current_song || ipd->id != current_id)
	{
		g_free(ipd);	
		return;
	}
	if(gtk_text_mark_get_deleted(mark))
	{
		g_free(ipd);	   
		return;
	}
	gtk_text_buffer_get_iter_at_mark(buffer, &iter, mark);

	if(ret == META_DATA_AVAILABLE)
	{	
		GdkPixbuf *pb = gdk_pixbuf_new_from_file_at_size(path,164,164,NULL);
		if(pb)
		{
			draw_pixbuf_border(pb);

			gtk_text_buffer_insert_pixbuf(buffer, &iter, pb);	
			gtk_text_buffer_get_iter_at_mark(buffer, &stop, mark);
			gtk_text_iter_forward_char(&stop);
			gtk_text_buffer_get_iter_at_mark(buffer, &start, mark);
			gtk_text_buffer_apply_tag_by_name(buffer, "item-value", &start, &stop);

			g_object_unref(pb);
		}
		else
		{
		}
	}
	g_free(ipd);	
}

void info_cover_album_mini_art_fetched(mpd_Song *song,MetaDataResult ret, char *path,gpointer data)
{
	info_pass_data	*ipd = data;
	GtkTextMark *mark= ipd->mark;
	GtkTextIter iter,start,stop;
	GtkTextBuffer *buffer = gtk_text_mark_get_buffer(mark);
	/* check if where checking for the correct song */
	/** Don't do anything when still fetching */
	if(ret == META_DATA_FETCHING) return;
	info_fetching--;
	info_fetching_changed();




	if( current_song == NULL|| ipd->id!=current_id)
	{
		g_free(ipd);	
		return; 
	}
	if(gtk_text_mark_get_deleted(mark))
	{
		g_free(ipd);
		return;
	}
	gtk_text_buffer_get_iter_at_mark(buffer, &iter, mark);

	if(ret == META_DATA_AVAILABLE)
	{	
		GdkPixbuf *pb = gdk_pixbuf_new_from_file_at_size(path,64,64,NULL);
		if(pb)
		{
			draw_pixbuf_border(pb);

			gtk_text_buffer_insert_pixbuf(buffer, &iter, pb);	
			gtk_text_buffer_get_iter_at_mark(buffer, &stop, mark);
			gtk_text_iter_forward_char(&stop);
			gtk_text_buffer_get_iter_at_mark(buffer, &start, mark);
			gtk_text_buffer_apply_tag_by_name(buffer, "item-value", &start, &stop);

			g_object_unref(pb);
		}
		else
		{
		}
	}
	else if(ret == META_DATA_UNAVAILABLE){
		GdkPixbuf *pb = gtk_widget_render_icon(GTK_WIDGET(info_text_view), "media-no-cover", -1, NULL);
		gtk_text_buffer_insert_pixbuf(buffer, &iter, pb);	
		gtk_text_buffer_get_iter_at_mark(buffer, &stop, mark);
		gtk_text_iter_forward_char(&stop);
		gtk_text_buffer_get_iter_at_mark(buffer, &start, mark);                		
		gtk_text_buffer_apply_tag_by_name(buffer, "item-value", &start, &stop);
		g_object_unref(pb);
	}
	g_free(ipd);
}

void info_cover_album_txt_fetched(mpd_Song *song,MetaDataResult ret, char *path,gpointer data)
{
	info_pass_data *ipd = data;
	GtkTextMark *mark = ipd->mark;
	GtkTextIter iter;
	GtkTextBuffer *buffer = gtk_text_mark_get_buffer(mark);
	/* check if where checking for the correct song */
	/** Don't do anything when still fetching */
	if(ret == META_DATA_FETCHING) return;
	info_fetching--;
	info_fetching_changed();


	if(!current_song|| ipd->id != current_id){
		g_free(ipd);
	 	return;  
	}
	if(gtk_text_mark_get_deleted(mark))
	{
		g_free(ipd);
		return;
	}
	gtk_text_buffer_get_iter_at_mark(buffer, &iter, mark);


	if(ret == META_DATA_AVAILABLE)
	{
		gsize size = 0;	
		gchar *content = NULL;
		if(g_file_get_contents(path, &content, &size, NULL))
		{
			gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "Album Info:\n", -1,"item","bold",NULL);
			gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, content, size, "item", NULL);
			gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "\n\n", -1,"item-value",NULL);
			g_free(content);
		}
	}
	g_free(ipd);
}


void info_cover_lyric_txt_fetched(mpd_Song *song,MetaDataResult ret, char *path,gpointer data)
{
	info_pass_data *ipd = data;
	GtkTextMark *mark = ipd->mark;
	GtkTextIter iter;
	GtkTextBuffer *buffer = gtk_text_mark_get_buffer(mark);
	/* check if where checking for the correct song */
	/** Don't do anything when still fetching */
	if(ret == META_DATA_FETCHING)
	{
		return;
	}
	info_fetching--;
	info_fetching_changed();

	if(!current_song|| ipd->id != current_id)
	{
		g_free(ipd);
		return;
	}
	if(gtk_text_mark_get_deleted(mark))
	{
		g_free(ipd);
		return;
	}
	gtk_text_buffer_get_iter_at_mark(buffer, &iter, mark);


	if(ret == META_DATA_AVAILABLE)
	{
		gsize size = 0;	
		gchar *content = NULL;
		if(g_file_get_contents(path, &content, &size, NULL))
		{
			gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "Lyrics:\n", -1,"item","bold",NULL);
			gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, content, size, "item", NULL);
			gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "\n\n", -1,"item-value",NULL);
			g_free(content);
		}
	}
	g_free(ipd);
}


void info_cover_txt_fetched(mpd_Song *song,MetaDataResult ret, char *path,gpointer data)
{
	info_pass_data *ipd = data;
	GtkTextMark *mark = ipd->mark;
	GtkTextIter iter;
	GtkTextBuffer *buffer = gtk_text_mark_get_buffer(mark);
	/* check if where checking for the correct song */

	/** Don't do anything when still fetching */
	if(ret == META_DATA_FETCHING) return;
	info_fetching--;
 	info_fetching_changed();


	if(!current_song || ipd->id != current_id)
	{
		g_free(ipd);
		return;
	}
	if(gtk_text_mark_get_deleted(mark))
	{
		g_free(ipd);
		return;
	}
	gtk_text_buffer_get_iter_at_mark(buffer, &iter, mark);


	if(ret == META_DATA_AVAILABLE)
	{
		gsize size = 0;	
		gchar *content = NULL;
		if(g_file_get_contents(path, &content, &size, NULL))
		{
			gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "\n", -1,"item-value",NULL);
			gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, content, size, "item", NULL);
			gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "\n\n", -1,"item-value",NULL);
			g_free(content);
		}
	}
	g_free(ipd);
}

void add_default_tags(GtkTextBuffer *buffer)
{
	PangoTabArray *pta = pango_tab_array_new_with_positions(1, TRUE,PANGO_TAB_LEFT, 80); 
	GtkTextTag *tag;
	tag = gtk_text_buffer_create_tag(buffer, "bold", 
			"weight", PANGO_WEIGHT_BOLD,
			NULL);               


	tag = gtk_text_buffer_create_tag(buffer, "title", 
			"size-points", (gdouble)24.0,
			"paragraph-background-gdk", &(info_text_view->style->bg[GTK_STATE_SELECTED]),
			"foreground-gdk", &(info_text_view->style->fg[GTK_STATE_SELECTED]),
			NULL);

	tag = gtk_text_buffer_create_tag(buffer, "artist", 
			"size-points", (gdouble)14.0,
			"paragraph-background-gdk",&(info_text_view->style->bg[GTK_STATE_SELECTED]),
			"foreground-gdk",&(info_text_view->style->fg[GTK_STATE_SELECTED]),
			NULL);                            	
	tag = gtk_text_buffer_create_tag(buffer, "bar", 
			"size-points", (gdouble)3.0,
			"paragraph-background-gdk",&(info_text_view->style->fg[GTK_STATE_NORMAL]),
			NULL);                            	
	tag = gtk_text_buffer_create_tag(buffer, "album-value", 
			"rise", 26*PANGO_SCALE,
			"size-points", (gdouble)10.0,			
			NULL);                            	

	tag = gtk_text_buffer_create_tag(buffer, "item", 
			"size-points", (gdouble)10.0,			
			"left-margin", 6,
			"tabs", pta,
			"tabs-set", TRUE,
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

	tag= gtk_text_buffer_create_tag(buffer, "link", "underline", TRUE,NULL);

}


void info_clear_display()
{
	GtkTextTagTable *table = NULL;
	GtkTextIter start, stop;
	/* Delete text */
	GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(info_text_view));
	gtk_text_buffer_get_start_iter(buffer, &start);
	gtk_text_buffer_get_end_iter(buffer, &stop);
	gtk_text_buffer_delete(buffer,&start,&stop);

	/* Delete Old Song url */
	if(current_song)
	{
		mpd_freeSong(current_song);
		current_song = NULL;
	}
	
	/* delete old tags */
	/** THIS WILL DELETE ALL TAGS */
	table = gtk_text_buffer_get_tag_table (buffer);
	if(tag_list)
	{
		GList *titer = NULL;

		for(titer = tag_list; titer; titer = g_list_next(titer))        	
		{
			gtk_text_tag_table_remove(table, GTK_TEXT_TAG(titer->data));
		}
		g_list_free(tag_list);
		tag_list = NULL;                                                	
	}
	/** add default again */
	/*add_default_tags(buffer);	*/
}



void info_show_song(mpd_Song *song)
{
	info_pass_data * ipd = NULL;
	GtkTextMark *mark= NULL;
	GtkTextTag *tag = NULL;
	GtkTextIter iter;
	gchar *string = NULL;
	GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(info_text_view));
	info_clear_display();
	current_song = mpd_songDup(song);
	current_id = g_random_int();     	

	/* if song not valid stop then...  We want nice message here */
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


	mark = gtk_text_buffer_create_mark(buffer, "album-art",&iter, TRUE);
	ipd = g_malloc0(sizeof(*ipd));
	ipd->mark = mark;
	ipd->id = current_id;
	info_fetching++;
	info_fetching_changed();	
	meta_data_get_path_callback(song, META_ALBUM_ART, info_cover_art_fetched, ipd);
	
	
	gtk_text_buffer_get_end_iter(buffer, &iter);
	gtk_text_buffer_insert(buffer, &iter, "\n\n", -1);

	/** ARTIST **/
	if(song->album)
	{
		/* Set tag for link */
		tag = gtk_text_buffer_create_tag(buffer, "album-url", NULL);
		tag_list = g_list_append(tag_list, tag);
		g_object_set_data_full(G_OBJECT(tag), "url", g_strdup_printf("album:%s", song->album), g_free);

		gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "Album:\t", -1,"item","bold",NULL);
		gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, song->album, -1,"item-value","link","album-url",NULL);
		gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "\n", -1,"item-value",NULL);
	}
	if(song->genre)
	{
		/* Set tag for link */
		tag = gtk_text_buffer_create_tag(buffer, "genre-url", NULL);
		tag_list = g_list_append(tag_list, tag);
		g_object_set_data_full(G_OBJECT(tag), "url", g_strdup("genre:"), g_free);


		gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "Genre:\t", -1,"item","bold",NULL);
		gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, song->genre, -1,"item-value","genre-url","link",NULL);
		gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "\n", -1,"item-value",NULL);
	}
	if(song->date)
	{
		gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "Date:\t", -1,"item","bold",NULL);
		gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, song->date, -1,"item-value",NULL);
		gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "\n", -1,"item-value",NULL);
	}
	if(song->composer)
	{
		gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "Composer:\t", -1,"item","bold",NULL);
		gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, song->composer, -1,"item-value",NULL);
		gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "\n", -1,"item-value",NULL);
	}
	if(song->track)
	{
		gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "Track:\t", -1,"item","bold",NULL);
		gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, song->track, -1,"item-value",NULL);
		gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "\n", -1,"item-value",NULL);
	}
	if(song->time)
	{
		string = g_strdup_printf("%02i:%02i", (song->time)/60, (song->time)%60);
		gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "Length:\t", -1,"item","bold",NULL);
		gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, string, -1,"item-value",NULL);
		gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "\n", -1,"item-value",NULL);
		g_free(string);
	}
	if(song->file)
	{
		string = g_path_get_basename(song->file);
		gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "Filename:\t", -1,"item","bold",NULL);
		gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, string, -1,"item-value",NULL);
		gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "\n", -1,"item-value",NULL);
		g_free(string);
		/**
		 * Directory name
		 */
		string = g_path_get_dirname(song->file);
		gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "Dirname:\t", -1,"item","bold",NULL);
		gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, string, -1,"item-value",NULL);
		gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "\n", -1,"item-value",NULL);         		
		g_free(string);                                                                              		
	}	

	if(song->artist && song->title)
	{
	
		mark = gtk_text_buffer_create_mark(buffer, "artist-txt",&iter, TRUE);
		ipd = g_malloc0(sizeof(*ipd));
		ipd->mark = mark;
		ipd->id = current_id;         		
		
		info_fetching++;
		info_fetching_changed();
		meta_data_get_path_callback(song, META_SONG_TXT, info_cover_lyric_txt_fetched, ipd);
		gtk_text_buffer_get_end_iter(buffer, &iter);
	}
}


void show_album_info(mpd_Song *song)
{
	info_pass_data *ipd = NULL;
	gchar 			*string = NULL;
	GtkTextIter 		iter;
	GtkTextTag 		*tag = NULL;
	GtkTextMark 		*mark = NULL;
	GtkTextBuffer 		*buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(info_text_view));
	info_clear_display();
	current_song = mpd_songDup(song);
	current_id = g_random_int();     	

	if(song->artist == NULL || song->album == NULL) return;

	gtk_text_buffer_get_start_iter(buffer, &iter);
	string = g_strdup_printf(" %s - %s\n", song->artist, song->album);
	gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, string, -1,"title","bold",NULL);
	g_free(string);                                                                  	
	gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "\n", -1, "bar", NULL);
	gtk_text_buffer_insert(buffer, &iter, "\n", -1);

	mark = gtk_text_buffer_create_mark(buffer, "album-art",&iter, TRUE);
	ipd = g_malloc0(sizeof(*ipd));
	ipd->mark = mark;
	ipd->id = current_id;
	
	
	info_fetching++;
	info_fetching_changed();
	meta_data_get_path_callback(song, META_ALBUM_ART, info_cover_art_fetched, ipd);
	gtk_text_buffer_get_end_iter(buffer, &iter);                                                    	
	gtk_text_buffer_insert(buffer, &iter, "\n\n", -1);
	/** ARTIST **/
	if(song->album)
	{
		/* Set tag for link */
		tag = gtk_text_buffer_create_tag(buffer, "album-url", NULL);
		tag_list = g_list_append(tag_list, tag);
		g_object_set_data_full(G_OBJECT(tag), "url", g_strdup_printf("album:%s", song->album), g_free);

		gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "Album: ", -1,"item","bold",NULL);
		gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, song->album, -1,"item-value","link","album-url",NULL);
		gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "\n", -1,"item-value",NULL);
	}
	if(song->genre)
	{
		/* Set tag for link */
		tag = gtk_text_buffer_create_tag(buffer, "genre-url", NULL);
		tag_list = g_list_append(tag_list, tag);
		g_object_set_data_full(G_OBJECT(tag), "url", g_strdup("genre:"), g_free);


		gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "Genre: ", -1,"item","bold",NULL);
		gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, song->genre, -1,"item-value","genre-url","link",NULL);
		gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "\n", -1,"item-value",NULL);
	}
	if(song->date)
	{
		gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "Date: ", -1,"item","bold",NULL);
		gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, song->date, -1,"item-value",NULL);
		gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "\n", -1,"item-value",NULL);
	}
	if(song->artist && song->album)
	{

		mark = gtk_text_buffer_create_mark(buffer, "artist-txt",&iter, TRUE);
		ipd = g_malloc0(sizeof(*ipd));
		ipd->mark = mark;
		ipd->id = current_id;

		
		info_fetching++;
		info_fetching_changed();
		meta_data_get_path_callback(song, META_ALBUM_TXT, info_cover_album_txt_fetched, ipd);
		gtk_text_buffer_get_end_iter(buffer, &iter);
	}
	if(song->artist && song->album && mpd_server_check_version(connection, 0,12,0))
	{
		MpdData *data = mpd_database_find_adv(connection,TRUE, 
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






}
void show_artist_info(mpd_Song *song)
{
	info_pass_data 	*ipd = NULL;
	gchar 			*string = NULL;
	GtkTextIter 		iter;
	GtkTextTagTable 	*table = NULL;
	GtkTextTag 		*tag = NULL;
	GtkTextMark 		*mark = NULL;
	GtkTextBuffer 		*buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(info_text_view));

	/**
 	 *  Get table 
 	 *  Needed for tag lookup
 	 */
	table = gtk_text_buffer_get_tag_table (buffer);

	info_clear_display();
	current_song = mpd_songDup(song);
	current_id = g_random_int();     	

	if(song->artist == NULL)
	{
		return;
	}
	gtk_text_buffer_get_start_iter(buffer, &iter);
	string = g_strdup_printf(" %s\n", song->artist);
	gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, string, -1,"title","bold",NULL);
	g_free(string);                                                                  	
	gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "\n", -1, "bar", NULL);
	gtk_text_buffer_insert(buffer, &iter, "\n", -1);

	/* Set tag for link */
	mark = gtk_text_buffer_create_mark(buffer, "artist-art",&iter, TRUE);
	ipd = g_malloc0(sizeof(*ipd));
	ipd->mark = mark;
	ipd->id = current_id;
	
	
	info_fetching++;
	info_fetching_changed();
	meta_data_get_path_callback(song, META_ARTIST_ART, info_cover_art_fetched, ipd);
	gtk_text_buffer_get_end_iter(buffer, &iter);
	/*		gtk_text_buffer_insert(buffer, &iter, "\n\n", -1);*/

	mark = gtk_text_buffer_create_mark(buffer, "artist-txt",&iter, TRUE);
	ipd = g_malloc0(sizeof(*ipd));
	ipd->mark = mark;
	ipd->id = current_id;         	
	
	info_fetching++;
	info_fetching_changed();
	meta_data_get_path_callback(song, META_ARTIST_TXT, info_cover_txt_fetched, ipd);
	gtk_text_buffer_get_end_iter(buffer, &iter);

	if(song->artist && song->album )
	{
		MpdData *data = mpd_database_get_albums(connection, song->artist);
		if(data)
		{
			int albums = 0;
			for(;data != NULL; data = mpd_data_get_next(data))
			{
				char *falbum = data->tag;
				mpd_Song *qsong = mpd_newSong();
				albums++;
				if(albums == 1)
				{
					string =  g_strdup_printf("\n\nAlbums by %s:\n", song->artist);
					gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, string, -1,"item","bold",NULL);
					g_free(string);
				}

				qsong->artist = g_strdup(song->artist);
				qsong->album = g_strdup(falbum);
				mark = gtk_text_buffer_create_mark(buffer,NULL,&iter, TRUE);
				ipd = g_malloc0(sizeof(*ipd));
				ipd->mark = mark;
				ipd->id = current_id;


				info_fetching++;
				info_fetching_changed();
				meta_data_get_path_callback(qsong, META_ALBUM_ART, info_cover_album_mini_art_fetched, ipd);
				gtk_text_buffer_get_end_iter(buffer, &iter);
				mpd_freeSong(qsong);

				gtk_text_buffer_get_end_iter(buffer, &iter);
				/* Set tag for link */
				tag = gtk_text_buffer_create_tag(buffer,NULL, NULL);
				tag_list = g_list_append(tag_list, tag);
				g_object_set_data_full(G_OBJECT(tag), "url", g_strdup_printf("album:%s", falbum), g_free);


				gtk_text_buffer_insert_with_tags(buffer, &iter, "  ", -1,
						gtk_text_tag_table_lookup(table,"album-value"),
						tag,
						NULL);
				gtk_text_buffer_insert_with_tags(buffer, &iter, falbum, -1,                                                 	
						gtk_text_tag_table_lookup(table,"album-value"),
						gtk_text_tag_table_lookup(table,"link"),
						tag,
						NULL);
				gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, "\n", -1,"album-value",NULL);

			}
		}

	}	
}

/* Looks at all tags covering the position (x, y) in the text view, 
 * and if one of them is a link, change the cursor to the "hands" cursor
 * typically used by web browsers.
 */
static void set_cursor_if_appropriate (GtkTextView *text_view,	gint x,gint y)
{
	GSList *tags = NULL, *tagp = NULL;
	GtkTextBuffer *buffer;
	GtkTextIter iter;
	char *url = NULL;
	gboolean hovering = FALSE;

	buffer = gtk_text_view_get_buffer (text_view);

	gtk_text_view_get_iter_at_location (text_view, &iter, x, y);

	tags = gtk_text_iter_get_tags (&iter);
	for (tagp = tags;  tagp != NULL;  tagp = tagp->next)
	{
		GtkTextTag *tag = tagp->data;
		gchar *name= g_object_get_data(G_OBJECT(tag), "url");

		/*gint page = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (tag), "page")); */
		if (name/*page != 0*/) 
		{
			hovering = TRUE;
			url = name;
			break;
		}
	}

	if (hovering != hovering_over_link)
	{
		hovering_over_link = hovering;

		if (hovering_over_link)
		{
			gdk_window_set_cursor (gtk_text_view_get_window (text_view, GTK_TEXT_WINDOW_TEXT), hand_cursor);
		}
		else
		{

			gdk_window_set_cursor (gtk_text_view_get_window (text_view, GTK_TEXT_WINDOW_TEXT), regular_cursor);
		}
	}

	if (tags) 
		g_slist_free (tags);
}

/* Update the cursor image if the pointer moved. 
*/
	static gboolean
motion_notify_event (GtkWidget      *text_view,
		GdkEventMotion *event)
{
	gint x, y;

	gtk_text_view_window_to_buffer_coords (GTK_TEXT_VIEW (text_view), 
			GTK_TEXT_WINDOW_WIDGET,
			event->x, event->y, &x, &y);

	set_cursor_if_appropriate (GTK_TEXT_VIEW (text_view), x, y);

	gdk_window_get_pointer (text_view->window, NULL, NULL, NULL);
	return FALSE;
}

/* Also update the cursor image if the window becomes visible
 * (e.g. when a window covering it got iconified).
 */
	static gboolean
visibility_notify_event (GtkWidget          *text_view,
		GdkEventVisibility *event)
{
	gint wx, wy, bx, by;

	gdk_window_get_pointer (text_view->window, &wx, &wy, NULL);

	gtk_text_view_window_to_buffer_coords (GTK_TEXT_VIEW (text_view), 
			GTK_TEXT_WINDOW_WIDGET,
			wx, wy, &bx, &by);

	set_cursor_if_appropriate (GTK_TEXT_VIEW (text_view), bx, by);

	return FALSE;
}

void follow_if_link(GtkWidget *text_view, GtkTextIter *iter)
{
	int loaded = FALSE;
	MpdData *data = NULL;
	GSList *titer, *tags = gtk_text_iter_get_tags(iter);

	for(titer = tags;titer != NULL && !loaded; titer = g_slist_next(titer))
	{
		GtkTextTag *tag = GTK_TEXT_TAG(titer->data);
		char *url = g_object_get_data(G_OBJECT(tag), "url");
		if(url)
		{
			if(!strncmp("album:", url, 6))
			{
				char *album = &url[6];
				data = mpd_database_find_adv(connection, TRUE, 
						MPD_TAG_ITEM_ARTIST, current_song->artist,
						MPD_TAG_ITEM_ALBUM, album, -1);
			}
			else if (!strncmp("artist:", url, 7))
			{
				data = mpd_database_find_adv(connection, TRUE, 
						MPD_TAG_ITEM_ARTIST, current_song->artist,-1);
			}
			else if (!strncmp("genre:", url, 6) && current_song->genre)
			{
				data = mpd_database_find_adv(connection, TRUE, 
						MPD_TAG_ITEM_GENRE, current_song->genre,-1);
			}                                                         			
			if(data)
			{	
				mpd_playlist_clear(connection);
				for(;data; data = mpd_data_get_next(data))
				{
					mpd_playlist_queue_add(connection, data->song->file);
				}
				mpd_playlist_queue_commit(connection);
				mpd_player_play(connection);
				data= NULL;
				loaded = TRUE;
			}

		}



	}

	/* Free the list */	
	if (tags) 
	{
		g_slist_free (tags);
	}
	if(loaded)
	{
		info_show_current_song();
	}
}

/* Links can be activated by pressing Enter.
*/
	static gboolean
key_press_event (GtkWidget *text_view,
		GdkEventKey *event)
{
	GtkTextIter iter;
	GtkTextBuffer *buffer;
	switch (event->keyval)
	{
		case GDK_Return: 
		case GDK_KP_Enter:
			buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (text_view));
			gtk_text_buffer_get_iter_at_mark (buffer, &iter, 
					gtk_text_buffer_get_insert (buffer));
			follow_if_link (text_view, &iter);
			break;

		default:
			break;
	}
	return FALSE;
}


/* Links can also be activated by clicking.
*/
static gboolean event_after (GtkWidget *text_view,
		GdkEvent  *ev)
{
	GtkTextIter start, end, iter;
	GtkTextBuffer *buffer;
	GdkEventButton *event;
	gint x, y;

	if (ev->type != GDK_BUTTON_RELEASE)
		return FALSE;

	event = (GdkEventButton *)ev;

	if (event->button != 1)
		return FALSE;

	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (text_view));

	/* we shouldn't follow a link if the user has selected something */
	gtk_text_buffer_get_selection_bounds (buffer, &start, &end);
	if (gtk_text_iter_get_offset (&start) != gtk_text_iter_get_offset (&end))
		return FALSE;

	gtk_text_view_window_to_buffer_coords (GTK_TEXT_VIEW (text_view), 
			GTK_TEXT_WINDOW_WIDGET,
			event->x, event->y, &x, &y);

	gtk_text_view_get_iter_at_location (GTK_TEXT_VIEW (text_view), &iter, x, y);


	follow_if_link (text_view, &iter);
	return FALSE;
}
int info_set_spinner()
{
	int row = spin_offset/5;
	int column = spin_offset%5;
	GdkPixbuf *frame = gdk_pixbuf_new_subpixbuf(info_spinner_pixbuf, column*36, row*36,36,36);
	gtk_image_set_from_pixbuf(GTK_IMAGE(info_pimage), frame);
	g_object_unref(frame);                                                   	
	spin_offset++;
	if(spin_offset >= 35) spin_offset = 0;
	return TRUE;
}

static void info_init()
{
	GtkTextBuffer *buffer = NULL;
	GtkWidget *tmp = NULL,*hbox = NULL,*label = NULL;
	GtkWidget *button = NULL;
	GtkWidget *sw =gtk_scrolled_window_new(NULL, NULL);
	info_vbox = gtk_vbox_new(FALSE, 6);
	/* Mozilla Browser widget */
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw), GTK_SHADOW_ETCHED_IN);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw), GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);
	info_text_view = gtk_text_view_new();
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(info_text_view), GTK_WRAP_WORD);
	gtk_container_set_border_width(GTK_CONTAINER(sw), 6);
	gtk_container_add(GTK_CONTAINER(sw), info_text_view);
	gtk_box_pack_start_defaults(GTK_BOX(info_vbox), sw);
	/* Update button */
	hbox = gtk_hbox_new(FALSE, 6);
	gtk_box_pack_start(GTK_BOX(info_vbox), hbox, FALSE, TRUE, 0);
	/**
	 * Song button
	 */
	tmp = gtk_alignment_new(0,0.5,0,1);
	button = gtk_button_new_with_mnemonic("_Song");
	gtk_button_set_image(GTK_BUTTON(button), gtk_image_new_from_stock(GTK_STOCK_EDIT, GTK_ICON_SIZE_BUTTON));

	g_signal_connect(G_OBJECT(button),"clicked", G_CALLBACK(info_show_current_song),NULL);
	gtk_container_add(GTK_CONTAINER(tmp), button);

	gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, TRUE, 0);

	/**
	 * Album button
	 */
	tmp = gtk_alignment_new(0,0.5,0,1);
	button = gtk_button_new_with_mnemonic("_Album");
	gtk_button_set_image(GTK_BUTTON(button), gtk_image_new_from_stock("media-album", GTK_ICON_SIZE_BUTTON));

	g_signal_connect(G_OBJECT(button),"clicked", G_CALLBACK(info_show_current_album),NULL);
	gtk_container_add(GTK_CONTAINER(tmp), button);

	gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, TRUE, 0);

	/**
	 * Artist button
	 */
	tmp = gtk_alignment_new(0,0.5,0,1);
	button = gtk_button_new_with_mnemonic("_Artist");
	gtk_button_set_image(GTK_BUTTON(button), gtk_image_new_from_stock("media-artist", GTK_ICON_SIZE_BUTTON));

	g_signal_connect(G_OBJECT(button),"clicked", G_CALLBACK(info_show_current_artist),NULL);
	gtk_container_add(GTK_CONTAINER(tmp), button);

	gtk_box_pack_start(GTK_BOX(hbox), tmp, FALSE, TRUE, 0);








	label = gtk_label_new("");
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 0);

	gtk_text_view_set_editable(GTK_TEXT_VIEW(info_text_view), FALSE);

	/**
	 * TODO: make this spinner 
	 */
	char *file = gmpc_get_full_image_path("spinner.png");
	info_spinner_pixbuf = gdk_pixbuf_new_from_file(file, NULL);
	info_pimage = gtk_image_new();
	g_timeout_add(2000/35, info_set_spinner, NULL);
	//tk_image_set_from_pixbuf(info_pimage, pba);
	gtk_box_pack_end(GTK_BOX(hbox), info_pimage, FALSE, TRUE, 0);
	g_free(file);

	/* Create some default "tags" */
	buffer = gtk_text_buffer_new(NULL);
	gtk_text_view_set_buffer(GTK_TEXT_VIEW(info_text_view),buffer);
	/* Add a default set of tags. Think of bold, and some default layout stuff
	 * These are persistent of program existance
	 */
	add_default_tags(buffer);



	gtk_widget_show_all(info_vbox);
	gtk_widget_hide(info_pimage);
	g_object_ref(G_OBJECT(info_vbox));





	hand_cursor = gdk_cursor_new (GDK_HAND2);
	regular_cursor = gdk_cursor_new (GDK_XTERM);




	g_signal_connect (info_text_view, "key-press-event", 
			G_CALLBACK (key_press_event), NULL);
	g_signal_connect (info_text_view, "event-after", 
			G_CALLBACK (event_after), NULL);
	g_signal_connect (info_text_view, "motion-notify-event", 
			G_CALLBACK (motion_notify_event), NULL);
	g_signal_connect (info_text_view, "visibility-notify-event", 
			G_CALLBACK (visibility_notify_event), NULL);






}

void info_add(GtkWidget *cat_tree)
{
	GtkTreePath *path;
	GtkTreeStore *pl3_tree = (GtkTreeStore *)gtk_tree_view_get_model(GTK_TREE_VIEW(cat_tree));
	GtkTreeIter iter;
	if(!cfg_get_single_value_as_int_with_default(config, "information", "enable", TRUE)) return;
	gtk_tree_store_append(pl3_tree, &iter, NULL);
	gtk_tree_store_set(pl3_tree, &iter,
			PL3_CAT_TYPE, info_plugin.id,
			PL3_CAT_TITLE, "Information",
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

	/**
	 * Window with information about the song.
	 *
	 gtk_tree_store_append(pl3_tree, &child,&iter);
	 gtk_tree_store_set(pl3_tree, &child,
	 PL3_CAT_TYPE, info_plugin.id,
	 PL3_CAT_TITLE, "Song",
	 PL3_CAT_INT_ID, "/Song",
	 PL3_CAT_ICON_ID, "gtk-edit",
	 PL3_CAT_PROC, TRUE,
	 PL3_CAT_ICON_SIZE,GTK_ICON_SIZE_DND,-1);	
	 gtk_tree_store_append(pl3_tree, &child,&iter);
	 gtk_tree_store_set(pl3_tree, &child,
	 PL3_CAT_TYPE, info_plugin.id,
	 PL3_CAT_TITLE, "Album",
	 PL3_CAT_INT_ID, "/Album",
	 PL3_CAT_ICON_ID, "media-album",
	 PL3_CAT_PROC, TRUE,
	 PL3_CAT_ICON_SIZE,GTK_ICON_SIZE_DND,-1);	
	 gtk_tree_store_append(pl3_tree, &child,&iter);
	 gtk_tree_store_set(pl3_tree, &child,
	 PL3_CAT_TYPE, info_plugin.id,
	 PL3_CAT_TITLE, "Artist",
	 PL3_CAT_INT_ID, "/Artist",                  	
	 PL3_CAT_ICON_ID, "media-artist",
	 PL3_CAT_PROC, TRUE,
	 PL3_CAT_ICON_SIZE,GTK_ICON_SIZE_DND,-1);		
	 */

}

void info_selected(GtkWidget *container)
{
	if(info_vbox== NULL) {
		info_init();
	}

	gtk_container_add(GTK_CONTAINER(container), info_vbox);
	gtk_widget_show_all(info_vbox);
	gtk_widget_hide(info_pimage);
	while (gtk_events_pending ())
		gtk_main_iteration ();

}

void info_unselected(GtkWidget *container)
{
	gtk_widget_hide(info_text_view);
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

void info_browser_activate()
{
	GtkTreeSelection *selec = gtk_tree_view_get_selection((GtkTreeView *)
			glade_xml_get_widget (pl3_xml, "cat_tree"));

	/**
	 * Fix this to be nnot static
	 */	
	GtkTreePath *path = gtk_tree_path_new_from_string("4"); 
	if(path)
	{
		gtk_tree_selection_select_path(selec, path);
	}
}


int info_add_go_menu(GtkWidget *menu)
{
	GtkWidget *item = NULL;

	item = gtk_image_menu_item_new_with_label(_("Information"));
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item), 
			gtk_image_new_from_stock("gtk-info", GTK_ICON_SIZE_MENU));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	g_signal_connect(G_OBJECT(item), "activate", 
			G_CALLBACK(info_browser_activate), NULL);

	return 1;
}

int info_key_press_event(GtkWidget *mw, GdkEventKey *event, int type)
{
	if (event->keyval == GDK_F5)
	{
		info_browser_activate();
		if(event->state&GDK_CONTROL_MASK)
		{
			info_show_current_song();	
		}
		return TRUE;
	}                                       
	/** if they key isn't for me, stop parsing */	
	if(type != info_plugin.id) return FALSE;
	if (event->keyval == GDK_1)
	{
		info_show_current_song();
		return TRUE;
	}
	if (event->keyval == GDK_2)
	{
		info_show_current_album();
		return TRUE;               	
	}
	if (event->keyval == GDK_3)
	{
		info_show_current_artist();
		return TRUE;               	
	}
	return FALSE;
}
