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

#ifndef __PLAYLIST3_H__
#define __PLAYLIST3_H__

#include <libmpd/libmpd.h>

extern GtkTreeModel *pl3_tree;
extern GtkListStore *pl3_store;


void pl3_show_window(void);
void pl3_toggle_hidden(void);
int pl3_cat_get_selected_browser(void);

typedef enum {
	PLAYLIST_NO_ZOOM,
	PLAYLIST_SMALL,
	PLAYLIST_MINI,
	PLAYLIST_ZOOM_LEVELS
}PlaylistZoom;
extern int pl3_zoom;

void playlist3_destroy(void);
gboolean playlist3_show_playtime(gulong playtime);


void playlist_editor_fill_list(void);

int pl3_window_key_press_event(GtkWidget *, GdkEventKey *);


extern GtkBuilder *pl3_xml;
/**
 * Server information
 */

void serverinformation_show_popup(void);

void pl3_window_fullscreen(void);
GtkAccelGroup *playlist3_get_accel_group(void);

GtkWidget* playlist3_get_widget_by_id(const char *id);

void open_local_file(void);
#endif
