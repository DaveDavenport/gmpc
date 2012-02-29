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

#ifndef __FAVORITES_H__
#define __FAVORITES_H__

#define FAV_CONFIG "playlist-plugin"

void playlist_editor_init(void);
void playlist_editor_destroy(void);
/* connection changed callback */
void playlist_editor_conn_changed(MpdObj *mi, int connect, void *userdata);
/* browser */
void playlist_editor_browser_add(GtkWidget *cat_tree);
void playlist_editor_browser_selected(GtkWidget *container);
void playlist_editor_browser_unselected(GtkWidget *container);
void playlist_editor_browser_changed(GtkWidget *tree, GtkTreeIter *iter);
int playlist_editor_browser_cat_menu(GtkWidget *menu, int type, GtkWidget *tree, GdkEventButton *event);
void playlist_editor_browser_row_activated(GtkTreeView *tree, GtkTreePath *path, GtkTreeViewColumn *col, gpointer data);
/**
 */

void playlist_editor_right_mouse(GtkWidget *menu, void (*add_to_playlist)(GtkWidget *menu, gpointer data), gpointer cb_data);
void playlist_editor_set_disabled(void);
#endif
