/* Gnome Music Player Client (GMPC)
 * Copyright (C) 2004-2012 Qball Cow <qball@gmpclient.org>
 * Project homepage: http://gmpclient.org/
 
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
#include <libmpd/libmpd.h>
#include "metadata.h"
/**
 * format time into 
 * Total time: %i days %i hours %i minutes
 */
gchar * format_time(unsigned long seconds);

/**
 * Allows you to prepend a string
 */
gchar * format_time_real(unsigned long seconds, const gchar *data);

void mpd_song_markup_escaped(char *buffer, int size, char *markup, mpd_Song *song);

/**
 * this draws a 1 pixel border around a pixbuf.
 * It doesn't work for all color depths (I think)
 */
void screenshot_add_border (GdkPixbuf *src);

void open_uri(const gchar *uri);
void open_help(const gchar *uri);
int *split_version(const char *uri);

MpdData * misc_sort_mpddata(MpdData *data, GCompareDataFunc func, void *user_data);
MpdData * misc_sort_mpddata_by_album_disc_track(MpdData *data);
MpdData *misc_mpddata_remove_duplicate_songs(MpdData *data);


gchar ** tokenize_string(const gchar *string);


gboolean misc_header_expose_event(GtkWidget *widget, GdkEventExpose *event);
void misc_header_style_set_process_containers(GtkWidget *container, GtkStyle *old_style, gpointer data);

gchar * mpd_song_checksum(const mpd_Song *song);
gchar *mpd_song_checksum_type(const mpd_Song * song, MetaDataType type);

void
colorshift_pixbuf(GdkPixbuf *dest, GdkPixbuf *src, int shift);
void decolor_pixbuf(GdkPixbuf *dest, GdkPixbuf *src);
void darken_pixbuf(GdkPixbuf * dest, guint factor);

void create_gmpc_paths(void);
mpd_Song * mpd_songDup0(const mpd_Song *song);
#endif
