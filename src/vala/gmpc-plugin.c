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

#include <glib.h>
#include <glib-object.h>
#include <stdlib.h>
#include <string.h>
#include <plugin.h>
#include <config1.h>
#include <gtk/gtk.h>
#include <metadata.h>
#include <libmpd/libmpdclient.h>
#include <libmpd/libmpd.h>


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

/**
         * This is the base class that a plugin should inherit from.
         *
         */
struct _GmpcPluginBase {
	GObject parent_instance;
	GmpcPluginBasePrivate * priv;
	const char* translation_domain;
	char* path;
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

/**
         * This interface allows the plugin to add one, or more, entries in the Tools menu.
         * If need to remove or undate an entry call pl3_tool_menu_update(). This will tell gmpc
         * To clear the menu, and call this function again on every plugin.
         */
struct _GmpcPluginToolMenuIfaceIface {
	GTypeInterface parent_iface;
	gint (*tool_menu_integration) (GmpcPluginToolMenuIface* self, GtkMenu* menu);
};

typedef void (*GmpcPluginMetaDataCallback) (GList* list, void* user_data);
/* untested */
struct _GmpcPluginMetaDataIfaceIface {
	GTypeInterface parent_iface;
	void (*get_data) (GmpcPluginMetaDataIface* self, const mpd_Song* song, MetaDataType type, GmpcPluginMetaDataCallback callback, void* callback_target);
	gint (*get_priority) (GmpcPluginMetaDataIface* self);
	void (*set_priority) (GmpcPluginMetaDataIface* self, gint priority);
};

struct _GmpcPluginBrowserIfaceIface {
	GTypeInterface parent_iface;
	void (*browser_add) (GmpcPluginBrowserIface* self, GtkWidget* category_tree);
	void (*browser_selected) (GmpcPluginBrowserIface* self, GtkContainer* container);
	void (*browser_unselected) (GmpcPluginBrowserIface* self, GtkContainer* container);
	gint (*browser_option_menu) (GmpcPluginBrowserIface* self, GtkMenu* menu);
	gint (*browser_add_go_menu) (GmpcPluginBrowserIface* self, GtkMenu* menu);
};

struct _GmpcPluginPreferencesIfaceIface {
	GTypeInterface parent_iface;
	void (*preferences_pane_construct) (GmpcPluginPreferencesIface* self, GtkContainer* container);
	void (*preferences_pane_destroy) (GmpcPluginPreferencesIface* self, GtkContainer* container);
};

/* untested */
struct _GmpcPluginSongListIfaceIface {
	GTypeInterface parent_iface;
	gint (*song_list) (GmpcPluginSongListIface* self, GtkWidget* tree, GtkMenu* menu);
};



GType gmpc_plugin_base_get_type (void);
enum  {
	GMPC_PLUGIN_BASE_DUMMY_PROPERTY
};
gint* gmpc_plugin_base_get_version (GmpcPluginBase* self, int* result_length1);
static gint* gmpc_plugin_base_real_get_version (GmpcPluginBase* self, int* result_length1);
const char* gmpc_plugin_base_get_name (GmpcPluginBase* self);
static const char* gmpc_plugin_base_real_get_name (GmpcPluginBase* self);
void gmpc_plugin_base_save_yourself (GmpcPluginBase* self);
static void gmpc_plugin_base_real_save_yourself (GmpcPluginBase* self);
gboolean gmpc_plugin_base_get_enabled (GmpcPluginBase* self);
static gboolean gmpc_plugin_base_real_get_enabled (GmpcPluginBase* self);
void gmpc_plugin_base_set_enabled (GmpcPluginBase* self, gboolean state);
static void gmpc_plugin_base_real_set_enabled (GmpcPluginBase* self, gboolean state);
static gpointer gmpc_plugin_base_parent_class = NULL;
static void gmpc_plugin_base_finalize (GObject* obj);
GType gmpc_plugin_tool_menu_iface_get_type (void);
gint gmpc_plugin_tool_menu_iface_tool_menu_integration (GmpcPluginToolMenuIface* self, GtkMenu* menu);
GType gmpc_plugin_meta_data_iface_get_type (void);
void gmpc_plugin_meta_data_iface_get_data (GmpcPluginMetaDataIface* self, const mpd_Song* song, MetaDataType type, GmpcPluginMetaDataCallback callback, void* callback_target);
gint gmpc_plugin_meta_data_iface_get_priority (GmpcPluginMetaDataIface* self);
void gmpc_plugin_meta_data_iface_set_priority (GmpcPluginMetaDataIface* self, gint priority);
GType gmpc_plugin_browser_iface_get_type (void);
void gmpc_plugin_browser_iface_browser_add (GmpcPluginBrowserIface* self, GtkWidget* category_tree);
void gmpc_plugin_browser_iface_browser_selected (GmpcPluginBrowserIface* self, GtkContainer* container);
void gmpc_plugin_browser_iface_browser_unselected (GmpcPluginBrowserIface* self, GtkContainer* container);
gint gmpc_plugin_browser_iface_browser_option_menu (GmpcPluginBrowserIface* self, GtkMenu* menu);
static gint gmpc_plugin_browser_iface_real_browser_option_menu (GmpcPluginBrowserIface* self, GtkMenu* menu);
gint gmpc_plugin_browser_iface_browser_add_go_menu (GmpcPluginBrowserIface* self, GtkMenu* menu);
static gint gmpc_plugin_browser_iface_real_browser_add_go_menu (GmpcPluginBrowserIface* self, GtkMenu* menu);
GType gmpc_plugin_preferences_iface_get_type (void);
void gmpc_plugin_preferences_iface_preferences_pane_construct (GmpcPluginPreferencesIface* self, GtkContainer* container);
void gmpc_plugin_preferences_iface_preferences_pane_destroy (GmpcPluginPreferencesIface* self, GtkContainer* container);
GType gmpc_plugin_song_list_iface_get_type (void);
gint gmpc_plugin_song_list_iface_song_list (GmpcPluginSongListIface* self, GtkWidget* tree, GtkMenu* menu);



static gint* gmpc_plugin_base_real_get_version (GmpcPluginBase* self, int* result_length1) {
	g_return_val_if_fail (self != NULL, NULL);
	g_critical ("Type `%s' does not implement abstract method `gmpc_plugin_base_get_version'", g_type_name (G_TYPE_FROM_INSTANCE (self)));
	return NULL;
}


/**
             * Function should return the version of the plugin
             */
gint* gmpc_plugin_base_get_version (GmpcPluginBase* self, int* result_length1) {
	return GMPC_PLUGIN_BASE_GET_CLASS (self)->get_version (self, result_length1);
}


static const char* gmpc_plugin_base_real_get_name (GmpcPluginBase* self) {
	g_return_val_if_fail (self != NULL, NULL);
	g_critical ("Type `%s' does not implement abstract method `gmpc_plugin_base_get_name'", g_type_name (G_TYPE_FROM_INSTANCE (self)));
	return NULL;
}


/**
             * Return the name of the plugin
             */
const char* gmpc_plugin_base_get_name (GmpcPluginBase* self) {
	return GMPC_PLUGIN_BASE_GET_CLASS (self)->get_name (self);
}


/**
             * This is called before the plugin is destroyed. Plugins should save it state here.
             *
             * A Browser plugin should store the position in the side-tree here.
             * Optional function. 
             */
static void gmpc_plugin_base_real_save_yourself (GmpcPluginBase* self) {
	g_return_if_fail (self != NULL);
}


void gmpc_plugin_base_save_yourself (GmpcPluginBase* self) {
	GMPC_PLUGIN_BASE_GET_CLASS (self)->save_yourself (self);
}


/**
             * Function used by gmpc to check if the plugin is enabled.
             * By default it is stored in the get_name() category under the enabled key.
             * 
             * @return The state (true or false)
             */
static gboolean gmpc_plugin_base_real_get_enabled (GmpcPluginBase* self) {
	g_return_val_if_fail (self != NULL, FALSE);
	if (gmpc_plugin_base_get_name (self) == NULL) {
		return FALSE;
	}
	return (gboolean) cfg_get_single_value_as_int_with_default (config, gmpc_plugin_base_get_name (self), "enabled", 1);
}


gboolean gmpc_plugin_base_get_enabled (GmpcPluginBase* self) {
	return GMPC_PLUGIN_BASE_GET_CLASS (self)->get_enabled (self);
}


/**
             * Function to enable/disable the plugin
             * @param state the enable state to set the plugin in. (true or false)
             * 
             * Function used by gmpc to enable/disable the plugin. 
             * By default it is stored in the get_name() category under the enabled key.
             * If something needs to be done on enable/disable override this function.
             */
static void gmpc_plugin_base_real_set_enabled (GmpcPluginBase* self, gboolean state) {
	g_return_if_fail (self != NULL);
	if (gmpc_plugin_base_get_name (self) != NULL) {
		cfg_set_single_value_as_int (config, gmpc_plugin_base_get_name (self), "enabled", (gint) state);
	}
}


void gmpc_plugin_base_set_enabled (GmpcPluginBase* self, gboolean state) {
	GMPC_PLUGIN_BASE_GET_CLASS (self)->set_enabled (self, state);
}


static void gmpc_plugin_base_class_init (GmpcPluginBaseClass * klass) {
	gmpc_plugin_base_parent_class = g_type_class_peek_parent (klass);
	GMPC_PLUGIN_BASE_CLASS (klass)->get_version = gmpc_plugin_base_real_get_version;
	GMPC_PLUGIN_BASE_CLASS (klass)->get_name = gmpc_plugin_base_real_get_name;
	GMPC_PLUGIN_BASE_CLASS (klass)->save_yourself = gmpc_plugin_base_real_save_yourself;
	GMPC_PLUGIN_BASE_CLASS (klass)->get_enabled = gmpc_plugin_base_real_get_enabled;
	GMPC_PLUGIN_BASE_CLASS (klass)->set_enabled = gmpc_plugin_base_real_set_enabled;
	G_OBJECT_CLASS (klass)->finalize = gmpc_plugin_base_finalize;
}


static void gmpc_plugin_base_instance_init (GmpcPluginBase * self) {
	self->translation_domain = NULL;
	self->plugin_type = 1;
}


static void gmpc_plugin_base_finalize (GObject* obj) {
	GmpcPluginBase * self;
	self = GMPC_PLUGIN_BASE (obj);
	self->path = (g_free (self->path), NULL);
	G_OBJECT_CLASS (gmpc_plugin_base_parent_class)->finalize (obj);
}


GType gmpc_plugin_base_get_type (void) {
	static GType gmpc_plugin_base_type_id = 0;
	if (gmpc_plugin_base_type_id == 0) {
		static const GTypeInfo g_define_type_info = { sizeof (GmpcPluginBaseClass), (GBaseInitFunc) NULL, (GBaseFinalizeFunc) NULL, (GClassInitFunc) gmpc_plugin_base_class_init, (GClassFinalizeFunc) NULL, NULL, sizeof (GmpcPluginBase), 0, (GInstanceInitFunc) gmpc_plugin_base_instance_init, NULL };
		gmpc_plugin_base_type_id = g_type_register_static (G_TYPE_OBJECT, "GmpcPluginBase", &g_define_type_info, G_TYPE_FLAG_ABSTRACT);
	}
	return gmpc_plugin_base_type_id;
}


gint gmpc_plugin_tool_menu_iface_tool_menu_integration (GmpcPluginToolMenuIface* self, GtkMenu* menu) {
	return GMPC_PLUGIN_TOOL_MENU_IFACE_GET_INTERFACE (self)->tool_menu_integration (self, menu);
}


static void gmpc_plugin_tool_menu_iface_base_init (GmpcPluginToolMenuIfaceIface * iface) {
	static gboolean initialized = FALSE;
	if (!initialized) {
		initialized = TRUE;
	}
}


GType gmpc_plugin_tool_menu_iface_get_type (void) {
	static GType gmpc_plugin_tool_menu_iface_type_id = 0;
	if (gmpc_plugin_tool_menu_iface_type_id == 0) {
		static const GTypeInfo g_define_type_info = { sizeof (GmpcPluginToolMenuIfaceIface), (GBaseInitFunc) gmpc_plugin_tool_menu_iface_base_init, (GBaseFinalizeFunc) NULL, (GClassInitFunc) NULL, (GClassFinalizeFunc) NULL, NULL, 0, 0, (GInstanceInitFunc) NULL, NULL };
		gmpc_plugin_tool_menu_iface_type_id = g_type_register_static (G_TYPE_INTERFACE, "GmpcPluginToolMenuIface", &g_define_type_info, 0);
		g_type_interface_add_prerequisite (gmpc_plugin_tool_menu_iface_type_id, GMPC_PLUGIN_TYPE_BASE);
	}
	return gmpc_plugin_tool_menu_iface_type_id;
}


void gmpc_plugin_meta_data_iface_get_data (GmpcPluginMetaDataIface* self, const mpd_Song* song, MetaDataType type, GmpcPluginMetaDataCallback callback, void* callback_target) {
	GMPC_PLUGIN_META_DATA_IFACE_GET_INTERFACE (self)->get_data (self, song, type, callback, callback_target);
}


/* Set get priority */
gint gmpc_plugin_meta_data_iface_get_priority (GmpcPluginMetaDataIface* self) {
	return GMPC_PLUGIN_META_DATA_IFACE_GET_INTERFACE (self)->get_priority (self);
}


void gmpc_plugin_meta_data_iface_set_priority (GmpcPluginMetaDataIface* self, gint priority) {
	GMPC_PLUGIN_META_DATA_IFACE_GET_INTERFACE (self)->set_priority (self, priority);
}


static void gmpc_plugin_meta_data_iface_base_init (GmpcPluginMetaDataIfaceIface * iface) {
	static gboolean initialized = FALSE;
	if (!initialized) {
		initialized = TRUE;
	}
}


GType gmpc_plugin_meta_data_iface_get_type (void) {
	static GType gmpc_plugin_meta_data_iface_type_id = 0;
	if (gmpc_plugin_meta_data_iface_type_id == 0) {
		static const GTypeInfo g_define_type_info = { sizeof (GmpcPluginMetaDataIfaceIface), (GBaseInitFunc) gmpc_plugin_meta_data_iface_base_init, (GBaseFinalizeFunc) NULL, (GClassInitFunc) NULL, (GClassFinalizeFunc) NULL, NULL, 0, 0, (GInstanceInitFunc) NULL, NULL };
		gmpc_plugin_meta_data_iface_type_id = g_type_register_static (G_TYPE_INTERFACE, "GmpcPluginMetaDataIface", &g_define_type_info, 0);
		g_type_interface_add_prerequisite (gmpc_plugin_meta_data_iface_type_id, GMPC_PLUGIN_TYPE_BASE);
	}
	return gmpc_plugin_meta_data_iface_type_id;
}


/* Function is called by gmpc, the plugin should then insert itself in the left tree  */
void gmpc_plugin_browser_iface_browser_add (GmpcPluginBrowserIface* self, GtkWidget* category_tree) {
	GMPC_PLUGIN_BROWSER_IFACE_GET_INTERFACE (self)->browser_add (self, category_tree);
}


/* This gets called, the plugin should add it view in container */
void gmpc_plugin_browser_iface_browser_selected (GmpcPluginBrowserIface* self, GtkContainer* container) {
	GMPC_PLUGIN_BROWSER_IFACE_GET_INTERFACE (self)->browser_selected (self, container);
}


/* Plugin should remove itself from container */
void gmpc_plugin_browser_iface_browser_unselected (GmpcPluginBrowserIface* self, GtkContainer* container) {
	GMPC_PLUGIN_BROWSER_IFACE_GET_INTERFACE (self)->browser_unselected (self, container);
}


/* Option menu */
static gint gmpc_plugin_browser_iface_real_browser_option_menu (GmpcPluginBrowserIface* self, GtkMenu* menu) {
	g_return_val_if_fail (menu != NULL, 0);
	return 0;
}


gint gmpc_plugin_browser_iface_browser_option_menu (GmpcPluginBrowserIface* self, GtkMenu* menu) {
	return GMPC_PLUGIN_BROWSER_IFACE_GET_INTERFACE (self)->browser_option_menu (self, menu);
}


/* Go menu */
static gint gmpc_plugin_browser_iface_real_browser_add_go_menu (GmpcPluginBrowserIface* self, GtkMenu* menu) {
	g_return_val_if_fail (menu != NULL, 0);
	return 0;
}


gint gmpc_plugin_browser_iface_browser_add_go_menu (GmpcPluginBrowserIface* self, GtkMenu* menu) {
	return GMPC_PLUGIN_BROWSER_IFACE_GET_INTERFACE (self)->browser_add_go_menu (self, menu);
}


static void gmpc_plugin_browser_iface_base_init (GmpcPluginBrowserIfaceIface * iface) {
	static gboolean initialized = FALSE;
	if (!initialized) {
		initialized = TRUE;
	}
	iface->browser_option_menu = gmpc_plugin_browser_iface_real_browser_option_menu;
	iface->browser_add_go_menu = gmpc_plugin_browser_iface_real_browser_add_go_menu;
}


GType gmpc_plugin_browser_iface_get_type (void) {
	static GType gmpc_plugin_browser_iface_type_id = 0;
	if (gmpc_plugin_browser_iface_type_id == 0) {
		static const GTypeInfo g_define_type_info = { sizeof (GmpcPluginBrowserIfaceIface), (GBaseInitFunc) gmpc_plugin_browser_iface_base_init, (GBaseFinalizeFunc) NULL, (GClassInitFunc) NULL, (GClassFinalizeFunc) NULL, NULL, 0, 0, (GInstanceInitFunc) NULL, NULL };
		gmpc_plugin_browser_iface_type_id = g_type_register_static (G_TYPE_INTERFACE, "GmpcPluginBrowserIface", &g_define_type_info, 0);
		g_type_interface_add_prerequisite (gmpc_plugin_browser_iface_type_id, GMPC_PLUGIN_TYPE_BASE);
	}
	return gmpc_plugin_browser_iface_type_id;
}


void gmpc_plugin_preferences_iface_preferences_pane_construct (GmpcPluginPreferencesIface* self, GtkContainer* container) {
	GMPC_PLUGIN_PREFERENCES_IFACE_GET_INTERFACE (self)->preferences_pane_construct (self, container);
}


void gmpc_plugin_preferences_iface_preferences_pane_destroy (GmpcPluginPreferencesIface* self, GtkContainer* container) {
	GMPC_PLUGIN_PREFERENCES_IFACE_GET_INTERFACE (self)->preferences_pane_destroy (self, container);
}


static void gmpc_plugin_preferences_iface_base_init (GmpcPluginPreferencesIfaceIface * iface) {
	static gboolean initialized = FALSE;
	if (!initialized) {
		initialized = TRUE;
	}
}


GType gmpc_plugin_preferences_iface_get_type (void) {
	static GType gmpc_plugin_preferences_iface_type_id = 0;
	if (gmpc_plugin_preferences_iface_type_id == 0) {
		static const GTypeInfo g_define_type_info = { sizeof (GmpcPluginPreferencesIfaceIface), (GBaseInitFunc) gmpc_plugin_preferences_iface_base_init, (GBaseFinalizeFunc) NULL, (GClassInitFunc) NULL, (GClassFinalizeFunc) NULL, NULL, 0, 0, (GInstanceInitFunc) NULL, NULL };
		gmpc_plugin_preferences_iface_type_id = g_type_register_static (G_TYPE_INTERFACE, "GmpcPluginPreferencesIface", &g_define_type_info, 0);
		g_type_interface_add_prerequisite (gmpc_plugin_preferences_iface_type_id, GMPC_PLUGIN_TYPE_BASE);
	}
	return gmpc_plugin_preferences_iface_type_id;
}


gint gmpc_plugin_song_list_iface_song_list (GmpcPluginSongListIface* self, GtkWidget* tree, GtkMenu* menu) {
	return GMPC_PLUGIN_SONG_LIST_IFACE_GET_INTERFACE (self)->song_list (self, tree, menu);
}


static void gmpc_plugin_song_list_iface_base_init (GmpcPluginSongListIfaceIface * iface) {
	static gboolean initialized = FALSE;
	if (!initialized) {
		initialized = TRUE;
	}
}


GType gmpc_plugin_song_list_iface_get_type (void) {
	static GType gmpc_plugin_song_list_iface_type_id = 0;
	if (gmpc_plugin_song_list_iface_type_id == 0) {
		static const GTypeInfo g_define_type_info = { sizeof (GmpcPluginSongListIfaceIface), (GBaseInitFunc) gmpc_plugin_song_list_iface_base_init, (GBaseFinalizeFunc) NULL, (GClassInitFunc) NULL, (GClassFinalizeFunc) NULL, NULL, 0, 0, (GInstanceInitFunc) NULL, NULL };
		gmpc_plugin_song_list_iface_type_id = g_type_register_static (G_TYPE_INTERFACE, "GmpcPluginSongListIface", &g_define_type_info, 0);
		g_type_interface_add_prerequisite (gmpc_plugin_song_list_iface_type_id, GMPC_PLUGIN_TYPE_BASE);
	}
	return gmpc_plugin_song_list_iface_type_id;
}




