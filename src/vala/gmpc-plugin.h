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

#ifndef __GMPC_PLUGIN_H__
#define __GMPC_PLUGIN_H__

#include <glib.h>
#include <glib-object.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS


#define GMPC_PLUGIN_TYPE_BASE (gmpc_plugin_base_get_type ())
#define GMPC_PLUGIN_BASE(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GMPC_PLUGIN_TYPE_BASE, GmpcPluginBase))
#define GMPC_PLUGIN_BASE_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GMPC_PLUGIN_TYPE_BASE, GmpcPluginBaseClass))
#define GMPC_PLUGIN_IS_BASE(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GMPC_PLUGIN_TYPE_BASE))
#define GMPC_PLUGIN_IS_BASE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GMPC_PLUGIN_TYPE_BASE))
#define GMPC_PLUGIN_BASE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GMPC_PLUGIN_TYPE_BASE, GmpcPluginBaseClass))

typedef struct _GmpcPluginBase GmpcPluginBase;
typedef struct _GmpcPluginBaseClass GmpcPluginBaseClass;
typedef struct _GmpcPluginBasePrivate GmpcPluginBasePrivate;

#define GMPC_PLUGIN_TYPE_TOOL_MENU_IFACE (gmpc_plugin_tool_menu_iface_get_type ())
#define GMPC_PLUGIN_TOOL_MENU_IFACE(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GMPC_PLUGIN_TYPE_TOOL_MENU_IFACE, GmpcPluginToolMenuIface))
#define GMPC_PLUGIN_IS_TOOL_MENU_IFACE(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GMPC_PLUGIN_TYPE_TOOL_MENU_IFACE))
#define GMPC_PLUGIN_TOOL_MENU_IFACE_GET_INTERFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), GMPC_PLUGIN_TYPE_TOOL_MENU_IFACE, GmpcPluginToolMenuIfaceIface))

typedef struct _GmpcPluginToolMenuIface GmpcPluginToolMenuIface;
typedef struct _GmpcPluginToolMenuIfaceIface GmpcPluginToolMenuIfaceIface;

#define GMPC_PLUGIN_TYPE_META_DATA_IFACE (gmpc_plugin_meta_data_iface_get_type ())
#define GMPC_PLUGIN_META_DATA_IFACE(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GMPC_PLUGIN_TYPE_META_DATA_IFACE, GmpcPluginMetaDataIface))
#define GMPC_PLUGIN_IS_META_DATA_IFACE(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GMPC_PLUGIN_TYPE_META_DATA_IFACE))
#define GMPC_PLUGIN_META_DATA_IFACE_GET_INTERFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), GMPC_PLUGIN_TYPE_META_DATA_IFACE, GmpcPluginMetaDataIfaceIface))

typedef struct _GmpcPluginMetaDataIface GmpcPluginMetaDataIface;
typedef struct _GmpcPluginMetaDataIfaceIface GmpcPluginMetaDataIfaceIface;

#define GMPC_PLUGIN_TYPE_BROWSER_IFACE (gmpc_plugin_browser_iface_get_type ())
#define GMPC_PLUGIN_BROWSER_IFACE(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GMPC_PLUGIN_TYPE_BROWSER_IFACE, GmpcPluginBrowserIface))
#define GMPC_PLUGIN_IS_BROWSER_IFACE(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GMPC_PLUGIN_TYPE_BROWSER_IFACE))
#define GMPC_PLUGIN_BROWSER_IFACE_GET_INTERFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), GMPC_PLUGIN_TYPE_BROWSER_IFACE, GmpcPluginBrowserIfaceIface))

typedef struct _GmpcPluginBrowserIface GmpcPluginBrowserIface;
typedef struct _GmpcPluginBrowserIfaceIface GmpcPluginBrowserIfaceIface;

#define GMPC_PLUGIN_TYPE_PREFERENCES_IFACE (gmpc_plugin_preferences_iface_get_type ())
#define GMPC_PLUGIN_PREFERENCES_IFACE(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GMPC_PLUGIN_TYPE_PREFERENCES_IFACE, GmpcPluginPreferencesIface))
#define GMPC_PLUGIN_IS_PREFERENCES_IFACE(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GMPC_PLUGIN_TYPE_PREFERENCES_IFACE))
#define GMPC_PLUGIN_PREFERENCES_IFACE_GET_INTERFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), GMPC_PLUGIN_TYPE_PREFERENCES_IFACE, GmpcPluginPreferencesIfaceIface))

typedef struct _GmpcPluginPreferencesIface GmpcPluginPreferencesIface;
typedef struct _GmpcPluginPreferencesIfaceIface GmpcPluginPreferencesIfaceIface;

#define GMPC_PLUGIN_TYPE_SONG_LIST_IFACE (gmpc_plugin_song_list_iface_get_type ())
#define GMPC_PLUGIN_SONG_LIST_IFACE(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GMPC_PLUGIN_TYPE_SONG_LIST_IFACE, GmpcPluginSongListIface))
#define GMPC_PLUGIN_IS_SONG_LIST_IFACE(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GMPC_PLUGIN_TYPE_SONG_LIST_IFACE))
#define GMPC_PLUGIN_SONG_LIST_IFACE_GET_INTERFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), GMPC_PLUGIN_TYPE_SONG_LIST_IFACE, GmpcPluginSongListIfaceIface))

typedef struct _GmpcPluginSongListIface GmpcPluginSongListIface;
typedef struct _GmpcPluginSongListIfaceIface GmpcPluginSongListIfaceIface;

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

struct _GmpcPluginToolMenuIfaceIface {
	GTypeInterface parent_iface;
	gint (*tool_menu_integration) (GmpcPluginToolMenuIface* self, GtkMenu* menu);
};

struct _GmpcPluginMetaDataIfaceIface {
	GTypeInterface parent_iface;
	gint (*get_data) (GmpcPluginMetaDataIface* self);
	gint (*get_priority) (GmpcPluginMetaDataIface* self);
	void (*set_priority) (GmpcPluginMetaDataIface* self, gint priority);
};

struct _GmpcPluginBrowserIfaceIface {
	GTypeInterface parent_iface;
	void (*browser_add) (GmpcPluginBrowserIface* self, GtkWidget* category_tree);
	void (*browser_selected) (GmpcPluginBrowserIface* self, GtkWidget* container);
	void (*browser_unselected) (GmpcPluginBrowserIface* self, GtkWidget* container);
};

struct _GmpcPluginPreferencesIfaceIface {
	GTypeInterface parent_iface;
	void (*pane_construct) (GmpcPluginPreferencesIface* self, GtkContainer* container);
	void (*pane_destroy) (GmpcPluginPreferencesIface* self, GtkContainer* container);
};

struct _GmpcPluginSongListIfaceIface {
	GTypeInterface parent_iface;
	gint (*song_list) (GmpcPluginSongListIface* self, GtkWidget* tree, GtkMenu* menu);
};


gint* gmpc_plugin_base_get_version (GmpcPluginBase* self, int* result_length1);
const char* gmpc_plugin_base_get_name (GmpcPluginBase* self);
void gmpc_plugin_base_save_yourself (GmpcPluginBase* self);
gboolean gmpc_plugin_base_get_enabled (GmpcPluginBase* self);
void gmpc_plugin_base_set_enabled (GmpcPluginBase* self, gboolean state);
GType gmpc_plugin_base_get_type (void);
gint gmpc_plugin_tool_menu_iface_tool_menu_integration (GmpcPluginToolMenuIface* self, GtkMenu* menu);
GType gmpc_plugin_tool_menu_iface_get_type (void);
gint gmpc_plugin_meta_data_iface_get_data (GmpcPluginMetaDataIface* self);
gint gmpc_plugin_meta_data_iface_get_priority (GmpcPluginMetaDataIface* self);
void gmpc_plugin_meta_data_iface_set_priority (GmpcPluginMetaDataIface* self, gint priority);
GType gmpc_plugin_meta_data_iface_get_type (void);
void gmpc_plugin_browser_iface_browser_add (GmpcPluginBrowserIface* self, GtkWidget* category_tree);
void gmpc_plugin_browser_iface_browser_selected (GmpcPluginBrowserIface* self, GtkWidget* container);
void gmpc_plugin_browser_iface_browser_unselected (GmpcPluginBrowserIface* self, GtkWidget* container);
GType gmpc_plugin_browser_iface_get_type (void);
void gmpc_plugin_preferences_iface_pane_construct (GmpcPluginPreferencesIface* self, GtkContainer* container);
void gmpc_plugin_preferences_iface_pane_destroy (GmpcPluginPreferencesIface* self, GtkContainer* container);
GType gmpc_plugin_preferences_iface_get_type (void);
gint gmpc_plugin_song_list_iface_song_list (GmpcPluginSongListIface* self, GtkWidget* tree, GtkMenu* menu);
GType gmpc_plugin_song_list_iface_get_type (void);


G_END_DECLS

#endif
