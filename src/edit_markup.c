/* Gnome Music Player Client (GMPC)
 * Copyright (C) 2004-2009 Qball Cow <qball@sarine.nl>
 * Project homepage: http://gmpcwiki.sarine.nl/
 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "main.h"
#include "misc.h"
#include "playlist3.h"
#include "config1.h"



static GtkBuilder *xml_esf  = NULL;
GtkTextBuffer *buffer= NULL;

void esf_reload(void);

static void esf_render_example(GtkTextBuffer *buf)
{
	char *filename = _("/path/to/file_name.mp3");
	char *artist = _("Jonny Singer");
	char *album = _("Sing or Swing you want");
	char *title = _("The kernel jazz");
	char *track = _("01/24");
	char *date = _("2 Feb 2006");
	unsigned int song_time =  645;
	char *stream_name = _("MPD's Streaming server");
	char *result_buffer[1024];
	char *format = NULL;
	GtkTextIter start_iter, stop_iter;
	mpd_Song *song = mpd_newSong();
	song->file = g_strdup(filename);
	song->time = song_time;

	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(xml_esf, "ck_artist"))))
	{
		song->artist = g_strdup(artist);
	}
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(xml_esf, "ck_album"))))
	{
		song->album = g_strdup(album);
	}
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(xml_esf, "ck_title"))))
	{
		song->title = g_strdup(title);
	}
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(xml_esf, "ck_track"))))
	{
		song->track = g_strdup(track);
	}
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(xml_esf, "ck_date"))))
	{
		song->date = g_strdup(date);
	}
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(xml_esf, "ck_stream"))))
	{
		song->name = g_strdup(stream_name);
	}


	gtk_text_buffer_get_start_iter(buffer, &start_iter);
	gtk_text_buffer_get_end_iter(buffer, &stop_iter);
	format = gtk_text_buffer_get_text(buffer,&start_iter, &stop_iter, FALSE);
	mpd_song_markup((char *)result_buffer, 1024,format, song);

	gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(xml_esf, "label_example")), (const char *)result_buffer);
	mpd_freeSong(song);
}
void esf_reload(void)
{
	esf_render_example(buffer);
}
char * edit_song_markup(char *format)
{
	char *str_format = NULL;
	GtkTextIter start_iter, stop_iter;
	GtkWidget *dialog;
    GError *error = NULL;
	char *path = gmpc_get_full_glade_path("preferences-esf-dialog.ui");
	xml_esf= gtk_builder_new();
    printf("ui path: %s\n", path);
    gtk_builder_add_from_file(xml_esf, path, &error);
    if(error) {
        debug_printf(DEBUG_ERROR, error->message);
        g_error_free(error);
        error = NULL;
    }
	q_free(path);
	dialog = (GtkWidget *) gtk_builder_get_object(xml_esf, "esf_dialog");
	buffer= gtk_text_view_get_buffer(GTK_TEXT_VIEW(gtk_builder_get_object(xml_esf, "textview_markup")));
    gtk_builder_connect_signals(xml_esf, NULL);	
	g_signal_connect(G_OBJECT(buffer), "changed", G_CALLBACK(esf_render_example), NULL);
	if(format != NULL)
	{
		gtk_text_buffer_set_text(buffer, format, -1);
	}
	q_free(str_format);

	switch(gtk_dialog_run(GTK_DIALOG(dialog)))
	{
		case GTK_RESPONSE_OK:
			{
				gtk_text_buffer_get_start_iter(buffer, &start_iter);
				gtk_text_buffer_get_end_iter(buffer, &stop_iter);
				str_format = g_strdup(gtk_text_buffer_get_text(buffer,&start_iter, &stop_iter, FALSE));
			}
        default:
            break;
	}

	gtk_widget_destroy(dialog);
	g_object_unref(xml_esf);
	xml_esf = NULL;
	buffer = NULL;
	return str_format;
}
