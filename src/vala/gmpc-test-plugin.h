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

#ifndef __GMPC_TEST_PLUGIN_H__
#define __GMPC_TEST_PLUGIN_H__

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include <gmpc_easy_download.h>
#include <gmpc-plugin.h>
#include <stdlib.h>
#include <string.h>

G_BEGIN_DECLS


#define TYPE_SONG_WINDOW (song_window_get_type ())
#define SONG_WINDOW(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_SONG_WINDOW, SongWindow))
#define SONG_WINDOW_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_SONG_WINDOW, SongWindowClass))
#define IS_SONG_WINDOW(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_SONG_WINDOW))
#define IS_SONG_WINDOW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_SONG_WINDOW))
#define SONG_WINDOW_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_SONG_WINDOW, SongWindowClass))

typedef struct _SongWindow SongWindow;
typedef struct _SongWindowClass SongWindowClass;
typedef struct _SongWindowPrivate SongWindowPrivate;

#define GMPC_TYPE_TEST_PLUGIN (gmpc_test_plugin_get_type ())
#define GMPC_TEST_PLUGIN(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GMPC_TYPE_TEST_PLUGIN, GmpcTestPlugin))
#define GMPC_TEST_PLUGIN_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GMPC_TYPE_TEST_PLUGIN, GmpcTestPluginClass))
#define GMPC_IS_TEST_PLUGIN(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GMPC_TYPE_TEST_PLUGIN))
#define GMPC_IS_TEST_PLUGIN_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GMPC_TYPE_TEST_PLUGIN))
#define GMPC_TEST_PLUGIN_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GMPC_TYPE_TEST_PLUGIN, GmpcTestPluginClass))

typedef struct _GmpcTestPlugin GmpcTestPlugin;
typedef struct _GmpcTestPluginClass GmpcTestPluginClass;
typedef struct _GmpcTestPluginPrivate GmpcTestPluginPrivate;

struct _SongWindow {
	GtkWindow parent_instance;
	SongWindowPrivate * priv;
};

struct _SongWindowClass {
	GtkWindowClass parent_class;
};

struct _GmpcTestPlugin {
	GmpcPluginBase parent_instance;
	GmpcTestPluginPrivate * priv;
};

struct _GmpcTestPluginClass {
	GmpcPluginBaseClass parent_class;
};


void song_window_image_downloaded (SongWindow* self, const GEADAsyncHandler* handle, GEADStatus status);
void song_window_callback (SongWindow* self, void* handle, GList* list);
void song_window_store_image (SongWindow* self, const GEADAsyncHandler* handle, GEADStatus status);
void song_window_destroy_popup (SongWindow* self, GtkButton* button);
GType song_window_get_type (void);
void gmpc_test_plugin_menu_activated_album (GmpcTestPlugin* self, GtkMenuItem* item);
void gmpc_test_plugin_menu_activated_artist (GmpcTestPlugin* self, GtkMenuItem* item);
GmpcTestPlugin* gmpc_test_plugin_construct (GType object_type);
GmpcTestPlugin* gmpc_test_plugin_new (void);
GType gmpc_test_plugin_get_type (void);

static const gint GMPC_TEST_PLUGIN_version[] = {0, 0, 2};

G_END_DECLS

#endif
