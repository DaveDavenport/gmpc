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

#ifndef __GMPC_PLUGIN2_H__
#define __GMPC_PLUGIN2_H__

#include <glib.h>
#include <glib-object.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS


#define GMPC_TYPE_PLUGIN_BASE (gmpc_plugin_base_get_type ())
#define GMPC_PLUGIN_BASE(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GMPC_TYPE_PLUGIN_BASE, GmpcPluginBase))
#define GMPC_PLUGIN_BASE_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GMPC_TYPE_PLUGIN_BASE, GmpcPluginBaseClass))
#define GMPC_IS_PLUGIN_BASE(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GMPC_TYPE_PLUGIN_BASE))
#define GMPC_IS_PLUGIN_BASE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GMPC_TYPE_PLUGIN_BASE))
#define GMPC_PLUGIN_BASE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GMPC_TYPE_PLUGIN_BASE, GmpcPluginBaseClass))

typedef struct _GmpcPluginBase GmpcPluginBase;
typedef struct _GmpcPluginBaseClass GmpcPluginBaseClass;
typedef struct _GmpcPluginBasePrivate GmpcPluginBasePrivate;

#define GMPC_PLUGIN2_TYPE_META_DATA (gmpc_plugin2_meta_data_get_type ())
#define GMPC_PLUGIN2_META_DATA(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GMPC_PLUGIN2_TYPE_META_DATA, GmpcPlugin2MetaData))
#define GMPC_PLUGIN2_IS_META_DATA(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GMPC_PLUGIN2_TYPE_META_DATA))
#define GMPC_PLUGIN2_META_DATA_GET_INTERFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), GMPC_PLUGIN2_TYPE_META_DATA, GmpcPlugin2MetaDataIface))

typedef struct _GmpcPlugin2MetaData GmpcPlugin2MetaData;
typedef struct _GmpcPlugin2MetaDataIface GmpcPlugin2MetaDataIface;

#define GMPC_PLUGIN2_TYPE_BROWSER (gmpc_plugin2_browser_get_type ())
#define GMPC_PLUGIN2_BROWSER(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GMPC_PLUGIN2_TYPE_BROWSER, GmpcPlugin2Browser))
#define GMPC_PLUGIN2_IS_BROWSER(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GMPC_PLUGIN2_TYPE_BROWSER))
#define GMPC_PLUGIN2_BROWSER_GET_INTERFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), GMPC_PLUGIN2_TYPE_BROWSER, GmpcPlugin2BrowserIface))

typedef struct _GmpcPlugin2Browser GmpcPlugin2Browser;
typedef struct _GmpcPlugin2BrowserIface GmpcPlugin2BrowserIface;

#define GMPC_PLUGIN2_TYPE_PREFERENCES (gmpc_plugin2_preferences_get_type ())
#define GMPC_PLUGIN2_PREFERENCES(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GMPC_PLUGIN2_TYPE_PREFERENCES, GmpcPlugin2Preferences))
#define GMPC_PLUGIN2_IS_PREFERENCES(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GMPC_PLUGIN2_TYPE_PREFERENCES))
#define GMPC_PLUGIN2_PREFERENCES_GET_INTERFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), GMPC_PLUGIN2_TYPE_PREFERENCES, GmpcPlugin2PreferencesIface))

typedef struct _GmpcPlugin2Preferences GmpcPlugin2Preferences;
typedef struct _GmpcPlugin2PreferencesIface GmpcPlugin2PreferencesIface;

#define GMPC_PLUGIN2_TYPE_SONG_LIST (gmpc_plugin2_song_list_get_type ())
#define GMPC_PLUGIN2_SONG_LIST(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GMPC_PLUGIN2_TYPE_SONG_LIST, GmpcPlugin2SongList))
#define GMPC_PLUGIN2_IS_SONG_LIST(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GMPC_PLUGIN2_TYPE_SONG_LIST))
#define GMPC_PLUGIN2_SONG_LIST_GET_INTERFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), GMPC_PLUGIN2_TYPE_SONG_LIST, GmpcPlugin2SongListIface))

typedef struct _GmpcPlugin2SongList GmpcPlugin2SongList;
typedef struct _GmpcPlugin2SongListIface GmpcPlugin2SongListIface;

struct _GmpcPluginBase {
	GObject parent_instance;
	GmpcPluginBasePrivate * priv;
	gint id;
	gint plugin_type;
};

struct _GmpcPluginBaseClass {
	GObjectClass parent_class;
	gint* (*get_version) (GmpcPluginBase* self, int* result_length1);
	const char* (*get_name) (GmpcPluginBase* self);
	void (*save_yourself) (GmpcPluginBase* self);
	gboolean (*get_enabled) (GmpcPluginBase* self);
	void (*set_enabled) (GmpcPluginBase* self, gboolean state);
};

struct _GmpcPlugin2MetaDataIface {
	GTypeInterface parent_iface;
	gint (*get_data) (GmpcPlugin2MetaData* self);
	gint (*get_priority) (GmpcPlugin2MetaData* self);
	void (*set_priority) (GmpcPlugin2MetaData* self, gint priority);
};

struct _GmpcPlugin2BrowserIface {
	GTypeInterface parent_iface;
	void (*add) (GmpcPlugin2Browser* self, GtkWidget* category_tree);
	void (*selected) (GmpcPlugin2Browser* self, GtkWidget* container);
	void (*unselected) (GmpcPlugin2Browser* self, GtkWidget* container);
};

struct _GmpcPlugin2PreferencesIface {
	GTypeInterface parent_iface;
	void (*pane_construct) (GmpcPlugin2Preferences* self, GtkContainer* container);
	void (*pane_destroy) (GmpcPlugin2Preferences* self, GtkContainer* container);
};

struct _GmpcPlugin2SongListIface {
	GTypeInterface parent_iface;
	gint (*song_list) (GmpcPlugin2SongList* self, GtkWidget* tree, GtkMenu* menu);
};


gint* gmpc_plugin_base_get_version (GmpcPluginBase* self, int* result_length1);
const char* gmpc_plugin_base_get_name (GmpcPluginBase* self);
void gmpc_plugin_base_save_yourself (GmpcPluginBase* self);
gboolean gmpc_plugin_base_get_enabled (GmpcPluginBase* self);
void gmpc_plugin_base_set_enabled (GmpcPluginBase* self, gboolean state);
GType gmpc_plugin_base_get_type (void);
gint gmpc_plugin2_meta_data_get_data (GmpcPlugin2MetaData* self);
gint gmpc_plugin2_meta_data_get_priority (GmpcPlugin2MetaData* self);
void gmpc_plugin2_meta_data_set_priority (GmpcPlugin2MetaData* self, gint priority);
GType gmpc_plugin2_meta_data_get_type (void);
void gmpc_plugin2_browser_add (GmpcPlugin2Browser* self, GtkWidget* category_tree);
void gmpc_plugin2_browser_selected (GmpcPlugin2Browser* self, GtkWidget* container);
void gmpc_plugin2_browser_unselected (GmpcPlugin2Browser* self, GtkWidget* container);
GType gmpc_plugin2_browser_get_type (void);
void gmpc_plugin2_preferences_pane_construct (GmpcPlugin2Preferences* self, GtkContainer* container);
void gmpc_plugin2_preferences_pane_destroy (GmpcPlugin2Preferences* self, GtkContainer* container);
GType gmpc_plugin2_preferences_get_type (void);
gint gmpc_plugin2_song_list_song_list (GmpcPlugin2SongList* self, GtkWidget* tree, GtkMenu* menu);
GType gmpc_plugin2_song_list_get_type (void);


G_END_DECLS

#endif
