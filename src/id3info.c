/*
 *Copyright (C) 2004 Qball Cow <Qball@qballcow.nl>
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
#include "libmpdclient.h"
#include "main.h"

GladeXML *xml_id3_window = NULL;
GList *songs = NULL;
void set_text (GList * node);
void id3_next_song ();
void id3_last_song ();


/* function to remove the id3 info screen and unref the xml tree */

	void
remove_id3_window ()
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

	void
create_window (int song)
{
	mpd_InfoEntity *ent = NULL;
	if (info.connection == NULL)
		return;
	if (info.status->state == MPD_STATUS_STATE_UNKNOWN)
		return;
	if (info.status->playlistLength == 0)
		return;
	xml_id3_window =
		glade_xml_new (GLADE_PATH "gmpc.glade", "id3_info_window", NULL);

	/* check for errors and axit when there is no gui file */
	if (xml_id3_window == NULL)
		g_error ("Couldnt initialize GUI. Please check installation\n");
	glade_xml_signal_autoconnect (xml_id3_window);

	/* set info from struct */
	mpd_sendPlaylistInfoCommand (info.connection, song);
	ent = mpd_getNextInfoEntity (info.connection);
	if (ent != NULL)
	{
		songs = g_list_append (songs, mpd_songDup (ent->info.song));
		mpd_freeInfoEntity (ent);
	}
	set_text (songs);
}

	void
set_text (GList * node)
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
		gtk_entry_set_text (GTK_ENTRY
				(glade_xml_get_widget
				 (xml_id3_window, "artist_entry")), song->artist);
	}
	else
	{
		gtk_entry_set_text (GTK_ENTRY
				(glade_xml_get_widget
				 (xml_id3_window, "artist_entry")), "");
	}
	if (song->title != NULL)
	{
		gtk_entry_set_text (GTK_ENTRY
				(glade_xml_get_widget
				 (xml_id3_window, "title_entry")), song->title);
	}
	else
	{
		gtk_entry_set_text (GTK_ENTRY
				(glade_xml_get_widget
				 (xml_id3_window, "title_entry")), "");
	}
	if (song->album != NULL)
	{
		gtk_entry_set_text (GTK_ENTRY
				(glade_xml_get_widget
				 (xml_id3_window, "album_entry")), song->album);
	}
	else if (song->name != NULL)
	{
		gtk_entry_set_text (GTK_ENTRY
				(glade_xml_get_widget
				 (xml_id3_window, "album_entry")), song->name);
	}
	else
	{
		gtk_entry_set_text (GTK_ENTRY
				(glade_xml_get_widget
				 (xml_id3_window, "album_entry")), "");
	}
	if (song->date != NULL)
	{
		gtk_entry_set_text (GTK_ENTRY
				(glade_xml_get_widget
				 (xml_id3_window, "date_entry")), song->date);
	}
	else
	{
		gtk_entry_set_text (GTK_ENTRY
				(glade_xml_get_widget
				 (xml_id3_window, "date_entry")), "");
	}

	if (song->track != NULL)
	{
		gtk_entry_set_text (GTK_ENTRY
				(glade_xml_get_widget
				 (xml_id3_window, "track_entry")), song->track);
	}
	else
	{
		gtk_entry_set_text (GTK_ENTRY
				(glade_xml_get_widget
				 (xml_id3_window, "track_entry")), "");
	}
	if (song->file != NULL)
	{
		gchar *buf1 = g_path_get_basename (song->file);
		gtk_entry_set_text (GTK_ENTRY
				(glade_xml_get_widget
				 (xml_id3_window, "filename_entry")), buf1);
		g_free (buf1);
	}
	else
	{
		gtk_entry_set_text (GTK_ENTRY
				(glade_xml_get_widget
				 (xml_id3_window, "filename_entry")), "");
	}
	if (song->time != MPD_SONG_NO_TIME)
	{
		gint min = (int) (song->time / 60);
		gint sec = song->time - min * 60;
		gchar *buf1 = g_strdup_printf ("%02i:%02i", min, sec);
		gtk_entry_set_text (GTK_ENTRY
				(glade_xml_get_widget
				 (xml_id3_window, "length_entry")), buf1);
		g_free (buf1);
	}
	else
	{
		gtk_entry_set_text (GTK_ENTRY
				(glade_xml_get_widget
				 (xml_id3_window, "length_entry")), "");
	}
	if (g_list_previous (songs) == NULL)
	{
		gtk_widget_set_sensitive (glade_xml_get_widget
				(xml_id3_window, "button_back"), FALSE);
	}
	else
		gtk_widget_set_sensitive (glade_xml_get_widget
				(xml_id3_window, "button_back"), TRUE);
	if (g_list_next (songs) == NULL)
	{
		gtk_widget_set_sensitive (glade_xml_get_widget
				(xml_id3_window, "button_next"), FALSE);
	}
	else
		gtk_widget_set_sensitive (glade_xml_get_widget
				(xml_id3_window, "button_next"), TRUE);
}

	void
id3_next_song ()
{
	songs = g_list_next (songs);
	set_text (songs);
}

	void
id3_last_song ()
{
	songs = g_list_previous (songs);
	set_text (songs);
}

	void
call_id3_window (int song)
{
	if (xml_id3_window == NULL)
	{
		create_window (song);
		return;
	}
	else
	{
		mpd_InfoEntity *ent = NULL;
		mpd_sendPlaylistInfoCommand (info.connection, song);
		ent = mpd_getNextInfoEntity (info.connection);
		if (ent != NULL)
		{
			songs = g_list_append (songs, mpd_songDup (ent->info.song));
			songs = g_list_last (songs);
			if (songs == NULL)
				if (debug)
					g_print ("Oeps.. error\n");
			set_text (songs);
			mpd_freeInfoEntity (ent);
		}
	}
}
