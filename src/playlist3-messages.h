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

#ifndef __PLAYLIST_MESSAGES_H__
#define __PLAYLIST_MESSAGES_H__

gboolean playlist3_close_error(void);

typedef enum {
	ERROR_INFO,
	ERROR_WARNING,
	ERROR_CRITICAL
} ErrorLevel;

void playlist3_show_error_message(const gchar *message, ErrorLevel el);
void playlist3_error_add_widget(GtkWidget *widget);



void playlist3_message_init(void);
void playlist3_message_destroy(void);

#endif
