/* Gnome Music Player Client (GMPC)
 * Copyright (C) 2004-2011 Qball Cow <qball@gmpclient.org>
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

#ifndef __MPDINTERACTION_H__
#define __MPDINTERACTION_H__
int play_song(void);
int pause_song(void);
int stop_song(void);
int next_song(void);
int prev_song(void);
void random_pl(GtkToggleButton *tb);
void repeat_pl(GtkToggleButton *tb);
void random_toggle(void);
void repeat_toggle(void);
void repeat_single_toggle(void);
void consume_toggle(void);

int  seek_ns(int n);
int  seek_ps(int n);
int connect_to_mpd(void);
void disconnect_from_mpd();
void song_fastforward(void);
void song_fastbackward(void);
void volume_up(void);
void volume_down(void);
void volume_mute(void);
void volume_unmute(void);
void volume_toggle_mute(void);
int update_mpd_status(void);

extern gmpcPlugin server_plug;
/**
 * Connection stuff 
 */
void connection_set_password(char *password);
int connection_use_auth(void);
char *connection_get_hostname(void);
int connection_get_port(void);
char *connection_get_password(void);
char *connection_get_current_profile(void);
void connection_set_current_profile(const char *uid);

/**
 * Helper functions
 */

void play_path(const gchar *path);
void add_artist(const gchar *artist);
void add_album(const gchar *artist,const gchar *album);
void add_genre(const gchar *genre);
void add_directory(const gchar *path);

/**
 * Helper menu functions *
 */
void submenu_for_song(GtkWidget *menu, mpd_Song *song);
#endif
