/*
 * Copyright (C) 2004-2006 Qball Cow <Qball@qballcow.nl>
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
#include <string.h>
#include <glade/glade.h>
#include <config.h>
#include "main.h"
#include "misc.h"
extern config_obj *cover_index;
GladeXML *xml_id3_window = NULL;
GList *songs = NULL;
void set_text (GList * node);
void id3_next_song ();
void id3_last_song ();


void id3_save_album_txt()
{
	GtkTextIter end, start;
	gchar *content = NULL;
	char *temp = NULL, *path = NULL;
	GtkTextBuffer *buffer = NULL;
	GtkTextView *tv = NULL;
	mpd_Song *current;
	if(songs == NULL || songs->data == NULL) return;
	current = songs->data;
	if(current->artist == NULL)
		return;
	tv = (GtkTextView *)glade_xml_get_widget(xml_id3_window, "album_tv");
	buffer = gtk_text_view_get_buffer(tv);
		

	temp = g_strdup_printf("albumtxt:%s", current->album);

	path = cfg_get_single_value_as_string(cover_index, current->artist, temp);
	
	if(path == NULL || strlen(path) == 0)
	{
		path = g_strdup_printf("%s/.covers/%s-%s.albuminfo", 
				g_get_home_dir(),
				current->artist,
				current->album);
		cfg_set_single_value_as_string(cover_index, current->artist, temp,path);
	}
	gtk_text_buffer_get_start_iter(buffer, &start);
	gtk_text_buffer_get_end_iter(buffer, &end);
	content = gtk_text_buffer_get_text(buffer,&start, &end,TRUE); 
	g_file_set_contents(path, content, -1, NULL);	



	g_free(temp);
	g_free(path);
	g_free(content);

}






void id3_save_artist_txt()
{
	GtkTextIter end, start;
	gchar *content = NULL;
	char *temp = NULL, *path = NULL;
	GtkTextBuffer *buffer = NULL;
	GtkTextView *tv = NULL;
	mpd_Song *current;
	if(songs == NULL || songs->data == NULL) return;
	current = songs->data;
	if(current->artist == NULL)
		return;
	tv = (GtkTextView *)glade_xml_get_widget(xml_id3_window, "artist_tv");
	buffer = gtk_text_view_get_buffer(tv);
		

	temp = g_strdup_printf("biography");

	path = cfg_get_single_value_as_string(cover_index, current->artist, temp);
	
	if(path == NULL || strlen(path) == 0)
	{
		path = g_strdup_printf("%s/.covers/%s.artistinfo", 
				g_get_home_dir(),
				current->artist);
		cfg_set_single_value_as_string(cover_index, current->artist, temp,path);
	}
	gtk_text_buffer_get_start_iter(buffer, &start);
	gtk_text_buffer_get_end_iter(buffer, &end);
	content = gtk_text_buffer_get_text(buffer,&start, &end,TRUE); 
	g_file_set_contents(path, content, -1, NULL);	



	g_free(temp);
	g_free(path);
	g_free(content);

}


void id3_txt_fetched(mpd_Song *song,MetaDataResult ret, char *path,GtkTextView *view)
{
	mpd_Song *current = NULL;
	if(songs == NULL || song == NULL) return;
	if(songs->data== NULL) return;
	current = songs->data;
	if(current->file)
	{
		if(!strcmp(current->file,song->file))
		{
			if(ret == META_DATA_FETCHING)
			{


				gtk_text_buffer_set_text(GTK_TEXT_BUFFER(gtk_text_view_get_buffer(view)),
						_("Working...."), -1);
				gtk_widget_set_sensitive(GTK_WIDGET(view), FALSE);


			}
			else if(ret == META_DATA_AVAILABLE)
			{
				gsize size;
				char *content = NULL;

				g_file_get_contents(path, &content, &size,NULL);
				gtk_text_buffer_set_text(GTK_TEXT_BUFFER(gtk_text_view_get_buffer(view)),
						content, size);
				g_free(content);

				gtk_widget_set_sensitive(GTK_WIDGET(view), TRUE);

			}
			else if(ret == META_DATA_UNAVAILABLE)
			{
				gtk_text_buffer_set_text(GTK_TEXT_BUFFER(gtk_text_view_get_buffer(view)),
						"", -1);
				gtk_widget_set_sensitive(GTK_WIDGET(view), TRUE);

			}
		}
	}
}






void remove_id3_window ()
{
	GtkWidget *window =
		glade_xml_get_widget (xml_id3_window, "id3_info_window");
	/* destroy and free memory */
	if (window)
		gtk_widget_destroy (window);
	if (xml_id3_window != NULL)
		g_object_unref (xml_id3_window);
	xml_id3_window = NULL;
	while ((songs = g_list_next (songs)))
	{
		mpd_freeSong (songs->data);
		songs->data = NULL;
	}
	g_list_free (songs);
	songs = NULL;
}


void id3_status_update()
{
	mpd_Song *song = NULL;
	if(xml_id3_window == NULL) return;
	if(songs == NULL) return;
	song =  songs->data;
	if(song == NULL) return;
	if(song->id == mpd_player_get_current_song_id(connection) && song->id != MPD_SONG_NO_ID)
	{
		char *temp = g_strdup_printf("%i kbps", mpd_status_get_bitrate(connection));
		gtk_label_set_text(GTK_LABEL(glade_xml_get_widget(xml_id3_window,"bitrate_label")),temp);
		g_free(temp);

	}
	else
	{
		gtk_label_set_markup(GTK_LABEL(glade_xml_get_widget(xml_id3_window,"bitrate_label")),"<i>only available for playing song</i>");
	}
	if (song->time != MPD_SONG_NO_TIME)
	{
		gint min = (int) (song->time / 60);
		gint sec = song->time - min * 60;
		gchar *buf1 =NULL;
		if(song->id == mpd_player_get_current_song_id(connection))
		{
			buf1 = g_strdup_printf ("%02i:%02i/%02i:%02i",
					mpd_status_get_elapsed_song_time(connection)/60,
					mpd_status_get_elapsed_song_time(connection)%60,
					min, sec);
		}
		else
		{
			buf1= g_strdup_printf ("%02i:%02i", min, sec);
		}
		gtk_label_set_text (GTK_LABEL
				(glade_xml_get_widget
				 (xml_id3_window, "length_label")), buf1);
		g_free (buf1);
	}
	else
	{
		gtk_label_set_text (GTK_LABEL(glade_xml_get_widget(xml_id3_window, "length_label")), "");
	}

}

void create_window (int song)
{
	char *path;
	mpd_Song *songstr = mpd_playlist_get_song(connection, song);

	if(songstr == NULL)
	{
		return;
	}
	path = gmpc_get_full_glade_path("gmpc.glade");	
	xml_id3_window = glade_xml_new (path,"id3_info_window", NULL);
	g_free(path);
	/* check for errors and axit when there is no gui file */
	if (xml_id3_window == NULL)
	{
		g_error ("Couldnt initialize GUI. Please check installation\n");
	}
	gmpc_metaimage_set_image_type(GMPC_METAIMAGE(glade_xml_get_widget(xml_id3_window, "metaimage_cover_image")),META_ALBUM_ART);
	gmpc_metaimage_set_size(GMPC_METAIMAGE(glade_xml_get_widget(xml_id3_window, "metaimage_cover_image")),300);
	gmpc_metaimage_set_image_type(GMPC_METAIMAGE(glade_xml_get_widget(xml_id3_window, "metaimage_artist_image")),META_ARTIST_ART);
	gmpc_metaimage_set_size(GMPC_METAIMAGE(glade_xml_get_widget(xml_id3_window, "metaimage_artist_image")),300);                 	
	
	glade_xml_signal_autoconnect (xml_id3_window);

	songs = g_list_append (songs, songstr);
	set_text (songs);
}
void id3_cover_art_fetched(mpd_Song *song,MetaDataResult ret, char *path, gpointer data )
{
	GtkImage *image = data;
	mpd_Song *current = NULL;
	if(songs == NULL || song == NULL) return;
	if(songs->data== NULL) return;
	current = songs->data;
	if(current->artist && current->album)
	{
		if(!strcmp(current->artist,song->artist) &&
				!strcmp(current->album, song->album))
		{
			GdkPixbuf *pb = NULL;
			if(ret == META_DATA_AVAILABLE)
			{
				pb = gdk_pixbuf_new_from_file_at_size(path, 300,300, NULL);
			}
			else if(ret == META_DATA_FETCHING)
			{
				pb = gtk_widget_render_icon(GTK_WIDGET(image),"media-loading-cover", -1,NULL);
			}
			if(!pb){
				pb = gtk_widget_render_icon(GTK_WIDGET(image),"media-no-cover", -1,NULL);
			}
			screenshot_add_border(&pb);
			//draw_pixbuf_border(pb);
			gtk_image_set_from_pixbuf(GTK_IMAGE(image), pb);	
			g_object_unref(pb);
			/*			set_text(songs);	*/
		}
	}
}



void set_text (GList * node)
{
	mpd_Song *song;
	if (node == NULL)
	{
		remove_id3_window ();
		return;
	}
	song = node->data;

	gmpc_metaimage_update_cover_from_song(GMPC_METAIMAGE(glade_xml_get_widget(xml_id3_window, "metaimage_cover_image")),song);
	gmpc_metaimage_update_cover_from_song(GMPC_METAIMAGE(glade_xml_get_widget(xml_id3_window, "metaimage_artist_image")),song);

	if (song->artist != NULL)
	{
		gtk_label_set_text (GTK_LABEL(glade_xml_get_widget(xml_id3_window, "artist_label")), song->artist);
	}
	else
	{
		gtk_label_set_text (GTK_LABEL(glade_xml_get_widget(xml_id3_window, "artist_label")), "");
	}
	if (song->title != NULL)
	{
		gtk_label_set_text (GTK_LABEL(glade_xml_get_widget(xml_id3_window, "title_label")), song->title);
	}
	else
	{
		gtk_label_set_text (GTK_LABEL(glade_xml_get_widget(xml_id3_window, "title_label")), "");
	}
	if (song->album != NULL)
	{
		gtk_label_set_text (GTK_LABEL(glade_xml_get_widget(xml_id3_window, "album_label")), song->album);
	}
	else if (song->name != NULL)
	{
		gtk_label_set_text (GTK_LABEL(glade_xml_get_widget(xml_id3_window, "album_label")), song->name);
	}
	else
	{
		gtk_label_set_text (GTK_LABEL(glade_xml_get_widget(xml_id3_window, "album_label")), "");
	}
	if (song->date != NULL)
	{
		gtk_label_set_text (GTK_LABEL(glade_xml_get_widget(xml_id3_window, "date_label")), song->date);
	}
	else
	{
		gtk_label_set_text (GTK_LABEL(glade_xml_get_widget(xml_id3_window, "date_label")), "");
	}

	if (song->track != NULL)
	{
		gtk_label_set_text (GTK_LABEL(glade_xml_get_widget(xml_id3_window, "track_label")), song->track);
	}
	else
	{
		gtk_label_set_text (GTK_LABEL(glade_xml_get_widget(xml_id3_window, "track_label")), "");
	}
	if (song->genre != NULL)
	{
		gtk_label_set_text (GTK_LABEL(glade_xml_get_widget(xml_id3_window, "genre_label")), song->genre);
	}
	else
	{
		gtk_label_set_text (GTK_LABEL(glade_xml_get_widget(xml_id3_window, "genre_label")), "");
	}
	if (song->disc != NULL)
	{
		gtk_label_set_text (GTK_LABEL(glade_xml_get_widget(xml_id3_window, "disc_label")), song->disc);
	}
	else
	{
		gtk_label_set_text (GTK_LABEL(glade_xml_get_widget(xml_id3_window, "disc_label")), "");
	}

	if (song->comment != NULL)
	{
		gtk_label_set_text (GTK_LABEL(glade_xml_get_widget(xml_id3_window, "comment_label")), song->comment);
	}
	else                                                                                                   	
	{
		gtk_label_set_text (GTK_LABEL(glade_xml_get_widget(xml_id3_window, "comment_label")), "");
	}                                                                                                      	


	if (song->composer != NULL)
	{
		gtk_label_set_text (GTK_LABEL(glade_xml_get_widget(xml_id3_window, "composer_label")), song->composer);
	}
	else
	{
		gtk_label_set_text (GTK_LABEL(glade_xml_get_widget(xml_id3_window, "composer_label")), "");
	}
	if (song->file != NULL)
	{
		gchar *buf1 = g_path_get_basename (song->file);
		gtk_label_set_text (GTK_LABEL(glade_xml_get_widget(xml_id3_window, "filename_label")), buf1);
		g_free (buf1);
	}
	else
	{
		gtk_label_set_text (GTK_LABEL(glade_xml_get_widget(xml_id3_window, "filename_label")), "");
	}
	if (song->file != NULL)
	{
		gchar *buf1 = g_path_get_dirname (song->file);
		gtk_label_set_text (GTK_LABEL(glade_xml_get_widget(xml_id3_window, "path_label")), buf1);
		g_free (buf1);
	}
	else
	{
		gtk_label_set_text (GTK_LABEL(glade_xml_get_widget(xml_id3_window, "path_label")), "");
	}

	if(song->id == mpd_player_get_current_song_id(connection) && song->id != MPD_SONG_NO_ID)
	{
		char *temp = g_strdup_printf("%i kbps", mpd_status_get_bitrate(connection));
		gtk_label_set_text(GTK_LABEL(glade_xml_get_widget(xml_id3_window,"bitrate_label")),temp);
		g_free(temp);

	}
	else
	{
		gtk_label_set_markup(GTK_LABEL(glade_xml_get_widget(xml_id3_window,"bitrate_label")),"<i>only available for playing song</i>");
	}

	if (song->time != MPD_SONG_NO_TIME)
	{
		gint min = (int) (song->time / 60);
		gint sec = song->time - min * 60;
		gchar *buf1 = g_strdup_printf ("%02i:%02i", min, sec);
		gtk_label_set_text (GTK_LABEL
				(glade_xml_get_widget
				 (xml_id3_window, "length_label")), buf1);
		g_free (buf1);
	}
	else
	{
		gtk_label_set_text (GTK_LABEL(glade_xml_get_widget(xml_id3_window, "length_label")), "");
	}
	if (g_list_previous (songs) == NULL)
	{
		gtk_widget_set_sensitive (glade_xml_get_widget(xml_id3_window, "button_back"), FALSE);
	}
	else
	{
		gtk_widget_set_sensitive (glade_xml_get_widget(xml_id3_window, "button_back"), TRUE);
	}
	if (g_list_next (songs) == NULL)
	{
		gtk_widget_set_sensitive (glade_xml_get_widget(xml_id3_window, "button_next"), FALSE);
	}
	else
	{
		gtk_widget_set_sensitive (glade_xml_get_widget(xml_id3_window, "button_next"), TRUE);
	}
	//gtk_widget_show_all(glade_xml_get_widget(xml_id3_window, "cover_event"));

	if(song){
		meta_data_get_path_callback(song, META_ARTIST_TXT, (MetaDataCallback)id3_txt_fetched, 
				glade_xml_get_widget(xml_id3_window, "artist_tv"));
		meta_data_get_path_callback(song, META_ALBUM_TXT, (MetaDataCallback)id3_txt_fetched, 
				glade_xml_get_widget(xml_id3_window, "album_tv"));
		
	}
}



void id3_next_song ()
{
	songs = g_list_next (songs);
	set_text (songs);
}

void id3_last_song ()
{
	songs = g_list_previous (songs);
	set_text (songs);
}



void call_id3_window_song(mpd_Song *songstr)
{
	if(songstr == NULL)
	{
		return;
	}
	if(xml_id3_window == NULL)
	{
		char *path = gmpc_get_full_glade_path("gmpc.glade");
		xml_id3_window = glade_xml_new (path, "id3_info_window", NULL);
		g_free(path);

		/* check for errors and axit when there is no gui file */
		if (xml_id3_window == NULL)
			g_error ("Couldnt initialize GUI. Please check installation\n");
		glade_xml_signal_autoconnect (xml_id3_window);
	}





	songs = g_list_append (songs,songstr);
	songs = g_list_last (songs);
	set_text (songs);


}
void call_id3_window (int song)
{
	if (xml_id3_window == NULL)
	{
		create_window (song);
		return;
	}
	else
	{
		mpd_Song  *songstr = mpd_playlist_get_song(connection, song);
		if(songstr != NULL)
		{
			songs = g_list_append (songs,songstr);
			songs = g_list_last (songs);
			set_text (songs);
		}
		gtk_window_present(GTK_WINDOW(glade_xml_get_widget(xml_id3_window, "id3_info_window")));
	}
}


void id3_edit_cover_art_fetched(mpd_Song *song,MetaDataResult ret, char *path, gpointer data )
{
	GtkImage *image = data;
	mpd_Song *current = NULL;
	if(songs == NULL || song == NULL) return;
	if(songs->data== NULL) return;
	current = songs->data;
	if(current->artist && current->album)
	{
		if(!strcmp(current->artist,song->artist) &&
				!strcmp(current->album, song->album))
		{
			GdkPixbuf *pb = NULL;
			if(ret == META_DATA_AVAILABLE)
			{
				pb = gdk_pixbuf_new_from_file_at_size(path, 300,300, NULL);
			}
			else if(ret == META_DATA_FETCHING)
			{
				pb = gtk_widget_render_icon(GTK_WIDGET(image),"media-loading-cover", -1,NULL);
			}
			if(!pb)
			{
				pb = gtk_widget_render_icon(GTK_WIDGET(image),"media-no-cover", -1,NULL);
			}
			screenshot_add_border(&pb);
//			draw_pixbuf_border(pb);
			gtk_image_set_from_pixbuf(GTK_IMAGE(image), pb);	
			g_object_unref(pb);
			/*			set_text(songs);	*/
			if(ret != META_DATA_FETCHING)
			{
				GmpcStatusChangedCallback(connection, MPD_CST_SONGID, 	NULL);

			}
		}
	}
}

void id3_info_clear_album_image()
{
	meta_data_set_cache(songs->data, META_ALBUM_ART, META_DATA_UNAVAILABLE, NULL);
	gmpc_metaimage_set_cover_na(GMPC_METAIMAGE(glade_xml_get_widget(xml_id3_window, "metaimage_cover_image")));	
	GmpcStatusChangedCallback(connection, MPD_CST_SONGID, 	NULL);
}

void id3_album_image_file_selector(GtkFileChooser *chooser)
{
	gchar *path = gtk_file_chooser_get_filename(chooser);
	if(path)
	{
		meta_data_set_cache(songs->data, META_ALBUM_ART, META_DATA_AVAILABLE, path);
		gmpc_metaimage_set_cover_from_path(GMPC_METAIMAGE(glade_xml_get_widget(xml_id3_window, "metaimage_cover_image")),path);	
		GmpcStatusChangedCallback(connection, MPD_CST_SONGID, 	NULL);
		g_free(path);
	}
}
void id3_reget_album_art()
{
	if(songs)
	{
		if(songs->data)
		{

			gmpc_metaimage_set_image_type(GMPC_METAIMAGE(glade_xml_get_widget(xml_id3_window, "metaimage_cover_image")),
					META_ALBUM_ART|META_QUERY_NO_CACHE);
			gmpc_metaimage_update_cover_from_song(
								GMPC_METAIMAGE(glade_xml_get_widget(xml_id3_window, "metaimage_cover_image")),
								songs->data);	
			gmpc_metaimage_set_image_type(GMPC_METAIMAGE(glade_xml_get_widget(xml_id3_window, "metaimage_cover_image")),META_ALBUM_ART);
			
		}
	}
}

void id3_info_clear_artist_image()
{
	meta_data_set_cache(songs->data, META_ARTIST_ART, META_DATA_UNAVAILABLE, NULL);
	gmpc_metaimage_set_cover_na(GMPC_METAIMAGE(glade_xml_get_widget(xml_id3_window, "metaimage_artist_image")));	
	GmpcStatusChangedCallback(connection, MPD_CST_SONGID, 	NULL);
}

void id3_artist_image_file_selector(GtkFileChooser *chooser)
{
	gchar *path = gtk_file_chooser_get_filename(chooser);
	if(path)
	{

		gmpc_metaimage_set_cover_from_path(GMPC_METAIMAGE(glade_xml_get_widget(xml_id3_window, "metaimage_artist_image")),path);	
		meta_data_set_cache(songs->data, META_ARTIST_ART, META_DATA_AVAILABLE, path);
		GmpcStatusChangedCallback(connection, MPD_CST_SONGID, 	NULL);         		
	}
}
void id3_reget_artist_art()
{
	if(songs)
	{
		if(songs->data)
		{
			gmpc_metaimage_set_image_type(GMPC_METAIMAGE(glade_xml_get_widget(xml_id3_window, "metaimage_artist_image")),
					META_ARTIST_ART|META_QUERY_NO_CACHE);
			gmpc_metaimage_update_cover_from_song(
								GMPC_METAIMAGE(glade_xml_get_widget(xml_id3_window, "metaimage_artist_image")),
								songs->data);	
			gmpc_metaimage_set_image_type(GMPC_METAIMAGE(glade_xml_get_widget(xml_id3_window, "metaimage_artist_image")),
					META_ARTIST_ART);
		}
	}
}
