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

#ifndef __MISC_H__
#define __MISC_H__

#define RANGE(l,u,v) (((v)<(l))?(l):(((v)<(u))?(v):(u)))
/**
 * format time into 
 * Total time: %i days %i hours %i minutes
 */

gchar * format_time(unsigned long seconds);
gchar * format_time_real(unsigned long seconds, const gchar *data);

void mpd_song_markup_escaped(char *buffer, int size, char *markup, mpd_Song *song);

/**
 * this draws a 1 pixel border around a pixbuf.
 * It doesn't work for all color depths (I think)
 */
void screenshot_add_border (GdkPixbuf **src);

void open_uri(const gchar *uri);
int *split_version(const char *uri);

MpdData * misc_sort_mpddata(MpdData *data, GCompareDataFunc func, void *user_data);
MpdData * misc_sort_mpddata_by_album_disc_track(MpdData *data);
MpdData *misc_mpddata_remove_duplicate_songs(MpdData *data);


gchar ** tokenize_string(const gchar *string);


gboolean misc_header_expose_event(GtkWidget *widget, GdkEventExpose *event, gpointer data);

#endif
