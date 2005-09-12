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
#include <string.h>
#include <glade/glade.h>
#include "main.h"

GladeXML *xml_id3_window = NULL;
GList *songs = NULL;
void set_text (GList * node);
void id3_next_song ();
void id3_last_song ();


/* function to remove the id3 info screen and unref the xml tree */

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
	if(song->id == mpd_ob_player_get_current_song_id(connection) && song->id != MPD_SONG_NO_ID)
	{
		char *temp = g_strdup_printf("%i kb/s", mpd_ob_status_get_bitrate(connection));
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
		if(song->id == mpd_ob_player_get_current_song_id(connection))
		{
			buf1 = g_strdup_printf ("%02i:%02i/%02i:%02i",
					mpd_ob_status_get_elapsed_song_time(connection)/60,
					mpd_ob_status_get_elapsed_song_time(connection)%60,
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
	mpd_Song *songstr = mpd_ob_playlist_get_song(connection, song);
	if(songstr == NULL)
	{
		return;
	}
	
	xml_id3_window = glade_xml_new (GLADE_PATH "gmpc.glade", "id3_info_window", NULL);

	/* check for errors and axit when there is no gui file */
	if (xml_id3_window == NULL)
	{
		g_error ("Couldnt initialize GUI. Please check installation\n");
	}
	glade_xml_signal_autoconnect (xml_id3_window);

	songs = g_list_append (songs, songstr);
	set_text (songs);
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

	if(song->id == mpd_ob_player_get_current_song_id(connection) && song->id != MPD_SONG_NO_ID)
	{
		char *temp = g_strdup_printf("%i kbps", mpd_ob_status_get_bitrate(connection));
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
		xml_id3_window = glade_xml_new (GLADE_PATH "gmpc.glade", "id3_info_window", NULL);

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
		mpd_Song  *songstr = mpd_ob_playlist_get_song(connection, song);
		if(songstr != NULL)
		{
			songs = g_list_append (songs,songstr);
			songs = g_list_last (songs);
			set_text (songs);
		}
		gtk_window_present(GTK_WINDOW(glade_xml_get_widget(xml_id3_window, "id3_info_window")));
	}
}
