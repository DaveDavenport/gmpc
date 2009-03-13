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

#include "gmpc-plugin.h"




enum  {
	GMPC_PLUGIN_BASE_DUMMY_PROPERTY
};
static gint* gmpc_plugin_base_real_get_version (GmpcPluginBase* self, int* result_length1);
static const char* gmpc_plugin_base_real_get_name (GmpcPluginBase* self);
static void gmpc_plugin_base_real_save_yourself (GmpcPluginBase* self);
static gboolean gmpc_plugin_base_real_get_enabled (GmpcPluginBase* self);
static void gmpc_plugin_base_real_set_enabled (GmpcPluginBase* self, gboolean state);
static gpointer gmpc_plugin_base_parent_class = NULL;
static void gmpc_plugin_base_finalize (GObject* obj);



static gint* gmpc_plugin_base_real_get_version (GmpcPluginBase* self, int* result_length1) {
	g_return_val_if_fail (self != NULL, NULL);
	g_critical ("Type `%s' does not implement abstract method `gmpc_plugin_base_get_version'", g_type_name (G_TYPE_FROM_INSTANCE (self)));
	return NULL;
}


/* The version */
gint* gmpc_plugin_base_get_version (GmpcPluginBase* self, int* result_length1) {
	return GMPC_PLUGIN_BASE_GET_CLASS (self)->get_version (self, result_length1);
}


static const char* gmpc_plugin_base_real_get_name (GmpcPluginBase* self) {
	g_return_val_if_fail (self != NULL, NULL);
	g_critical ("Type `%s' does not implement abstract method `gmpc_plugin_base_get_name'", g_type_name (G_TYPE_FROM_INSTANCE (self)));
	return NULL;
}


/* = {0,0,1};*/
const char* gmpc_plugin_base_get_name (GmpcPluginBase* self) {
	return GMPC_PLUGIN_BASE_GET_CLASS (self)->get_name (self);
}


static void gmpc_plugin_base_real_save_yourself (GmpcPluginBase* self) {
	g_return_if_fail (self != NULL);
	g_critical ("Type `%s' does not implement abstract method `gmpc_plugin_base_save_yourself'", g_type_name (G_TYPE_FROM_INSTANCE (self)));
	return;
}


void gmpc_plugin_base_save_yourself (GmpcPluginBase* self) {
	GMPC_PLUGIN_BASE_GET_CLASS (self)->save_yourself (self);
}


static gboolean gmpc_plugin_base_real_get_enabled (GmpcPluginBase* self) {
	g_return_val_if_fail (self != NULL, FALSE);
	g_critical ("Type `%s' does not implement abstract method `gmpc_plugin_base_get_enabled'", g_type_name (G_TYPE_FROM_INSTANCE (self)));
	return FALSE;
}


gboolean gmpc_plugin_base_get_enabled (GmpcPluginBase* self) {
	return GMPC_PLUGIN_BASE_GET_CLASS (self)->get_enabled (self);
}


static void gmpc_plugin_base_real_set_enabled (GmpcPluginBase* self, gboolean state) {
	g_return_if_fail (self != NULL);
	g_critical ("Type `%s' does not implement abstract method `gmpc_plugin_base_set_enabled'", g_type_name (G_TYPE_FROM_INSTANCE (self)));
	return;
}


void gmpc_plugin_base_set_enabled (GmpcPluginBase* self, gboolean state) {
	GMPC_PLUGIN_BASE_GET_CLASS (self)->set_enabled (self, state);
}


static void gmpc_plugin_base_class_init (GmpcPluginBaseClass * klass) {
	gmpc_plugin_base_parent_class = g_type_class_peek_parent (klass);
	G_OBJECT_CLASS (klass)->finalize = gmpc_plugin_base_finalize;
	GMPC_PLUGIN_BASE_CLASS (klass)->get_version = gmpc_plugin_base_real_get_version;
	GMPC_PLUGIN_BASE_CLASS (klass)->get_name = gmpc_plugin_base_real_get_name;
	GMPC_PLUGIN_BASE_CLASS (klass)->save_yourself = gmpc_plugin_base_real_save_yourself;
	GMPC_PLUGIN_BASE_CLASS (klass)->get_enabled = gmpc_plugin_base_real_get_enabled;
	GMPC_PLUGIN_BASE_CLASS (klass)->set_enabled = gmpc_plugin_base_real_set_enabled;
}


static void gmpc_plugin_base_instance_init (GmpcPluginBase * self) {
	self->plugin_type = 1;
}


static void gmpc_plugin_base_finalize (GObject* obj) {
	GmpcPluginBase * self;
	self = GMPC_PLUGIN_BASE (obj);
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


gint gmpc_plugin_meta_data_iface_get_data (GmpcPluginMetaDataIface* self) {
	return GMPC_PLUGIN_META_DATA_IFACE_GET_INTERFACE (self)->get_data (self);
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
void gmpc_plugin_browser_iface_browser_selected (GmpcPluginBrowserIface* self, GtkWidget* container) {
	GMPC_PLUGIN_BROWSER_IFACE_GET_INTERFACE (self)->browser_selected (self, container);
}


/* Plugin should remove itself from container */
void gmpc_plugin_browser_iface_browser_unselected (GmpcPluginBrowserIface* self, GtkWidget* container) {
	GMPC_PLUGIN_BROWSER_IFACE_GET_INTERFACE (self)->browser_unselected (self, container);
}


static void gmpc_plugin_browser_iface_base_init (GmpcPluginBrowserIfaceIface * iface) {
	static gboolean initialized = FALSE;
	if (!initialized) {
		initialized = TRUE;
	}
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


void gmpc_plugin_preferences_iface_pane_construct (GmpcPluginPreferencesIface* self, GtkContainer* container) {
	GMPC_PLUGIN_PREFERENCES_IFACE_GET_INTERFACE (self)->pane_construct (self, container);
}


void gmpc_plugin_preferences_iface_pane_destroy (GmpcPluginPreferencesIface* self, GtkContainer* container) {
	GMPC_PLUGIN_PREFERENCES_IFACE_GET_INTERFACE (self)->pane_destroy (self, container);
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




