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
#include <gdk/gdkkeysyms.h>
#include <stdlib.h>
#include <string.h>
#include <glade/glade.h>
#include <time.h>
#include <config.h>
#include "main.h"
#include "strfsong.h"
#include "misc.h"
#include "playlist3.h"
#include "config1.h"


extern config_obj *config;










GladeXML *xml_esf  = NULL;
GtkTextBuffer *buffer= NULL;





void esf_render_example(GtkTextBuffer *buffer)
{
	char *filename = _("/path/to/file_name.mp3");
	char *artist = _("I. am Artist");
	char *album = _("To Album or Not To Album");
	char *title = _("Am I a Visible Title");
	char *track = _("01/24");
	char *date = _("25 Feb 2005");
	unsigned int song_time =  645;
	char *stream_name = _("MPD's Streaming server");





	char *result_buffer[1024];
	char *format = NULL;
	GtkTextIter start_iter, stop_iter;
	mpd_Song *song = mpd_newSong();
	song->file = g_strdup(filename);
	song->time = song_time;

	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(glade_xml_get_widget(xml_esf, "ck_artist"))))
	{
		song->artist = g_strdup(artist);
	}
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(glade_xml_get_widget(xml_esf, "ck_album"))))
	{
		song->album = g_strdup(album);
	}                                                                                              	
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(glade_xml_get_widget(xml_esf, "ck_title"))))
	{
		song->title = g_strdup(title);
	}                                 
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(glade_xml_get_widget(xml_esf, "ck_track"))))
	{
		song->track = g_strdup(track);
	}                                 
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(glade_xml_get_widget(xml_esf, "ck_date"))))
	{
		song->date = g_strdup(date);
	}                                 
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(glade_xml_get_widget(xml_esf, "ck_stream"))))
	{
		song->name = g_strdup(stream_name);
	}                                                                                            	


	gtk_text_buffer_get_start_iter(buffer, &start_iter);
	gtk_text_buffer_get_end_iter(buffer, &stop_iter);
	format = gtk_text_buffer_get_text(buffer,&start_iter, &stop_iter, FALSE);
	strfsong ((char *)result_buffer, 1024,format, song);

	gtk_label_set_text(GTK_LABEL(glade_xml_get_widget(xml_esf, "label_example")), (const char *)result_buffer);
	mpd_freeSong(song);
}

void esf_reload()
{
	esf_render_example(buffer);
}




char * edit_song_markup(char *format)
{
	char *str_format = NULL;
	GtkTextIter start_iter, stop_iter;				
	xml_esf= glade_xml_new(GLADE_PATH"gmpc.glade", "esf_dialog",NULL);
	GtkWidget *dialog = glade_xml_get_widget(xml_esf, "esf_dialog");
	buffer= gtk_text_view_get_buffer(GTK_TEXT_VIEW(glade_xml_get_widget(xml_esf, "textview_markup")));
	glade_xml_signal_autoconnect (xml_esf);
	g_signal_connect(G_OBJECT(buffer), "changed", G_CALLBACK(esf_render_example), NULL);
	if(format != NULL)
	{
		gtk_text_buffer_set_text(buffer, format, -1);
	}
	g_free(str_format);

	switch(gtk_dialog_run(GTK_DIALOG(dialog)))
	{
		case GTK_RESPONSE_OK:
			{
				gtk_text_buffer_get_start_iter(buffer, &start_iter);
				gtk_text_buffer_get_end_iter(buffer, &stop_iter);
				str_format = g_strdup(gtk_text_buffer_get_text(buffer,&start_iter, &stop_iter, FALSE));
			}
	}

	gtk_widget_destroy(dialog);
	g_object_unref(xml_esf);
	xml_esf = NULL;
	buffer = NULL;
	return str_format;
}
