/* Gnome Music Player Client (GMPC)
 * Copyright (C) 2004-2009 Qball Cow <qball@sarine.nl>
 * Project homepage: http://gmpc.wikia.com/
 
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
#include <glade/glade.h>

extern GtkTreeModel *pl3_tree;
extern GtkListStore *pl3_store;


void pl3_show_window(void);
void pl3_toggle_hidden(void);
void create_playlist3(void);
void pl3_push_statusbar_message(const char *mesg);
void pl3_push_rsb_message(const gchar *string);
int pl3_hide(void);
int pl3_cat_get_selected_browser(void);

typedef enum {
	PLAYLIST_NO_ZOOM,
	PLAYLIST_SMALL,
	PLAYLIST_MINI,
	PLAYLIST_ZOOM_LEVELS
}PlaylistZoom;

void playlist3_destroy(void);
gboolean playlist3_show_playtime(gulong playtime);


void playlist_editor_fill_list(void);

int pl3_window_key_press_event(GtkWidget *, GdkEventKey *);

GtkWidget * playlist3_get_window(void);

extern GladeXML *pl3_xml;
/**
 * Server information 
 */

void serverinformation_show_popup(void);
#endif
