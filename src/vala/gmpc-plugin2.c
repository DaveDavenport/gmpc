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

#include "gmpc-plugin2.h"




enum  {
	GMPC_PLUGIN_BASE_DUMMY_PROPERTY
};
static const char* gmpc_plugin_base_real_get_name (GmpcPluginBase* self);
static void gmpc_plugin_base_real_save_yourself (GmpcPluginBase* self);
static gboolean gmpc_plugin_base_real_get_enabled (GmpcPluginBase* self);
static void gmpc_plugin_base_real_set_enabled (GmpcPluginBase* self, gboolean state);
static gpointer gmpc_plugin_base_parent_class = NULL;
static void gmpc_plugin_base_finalize (GObject* obj);



static const char* gmpc_plugin_base_real_get_name (GmpcPluginBase* self) {
	g_return_val_if_fail (self != NULL, NULL);
	g_critical ("Type `%s' does not implement abstract method `gmpc_plugin_base_get_name'", g_type_name (G_TYPE_FROM_INSTANCE (self)));
	return NULL;
}


const char* gmpc_plugin_base_get_name (GmpcPluginBase* self) {
	return GMPC_PLUGIN_BASE_GET_CLASS (self)->get_name (self);
}


static void gmpc_plugin_base_real_save_yourself (GmpcPluginBase* self) {
	g_return_if_fail (self != NULL);
	g_critical ("Type `%s' does not implement abstract method `gmpc_plugin_base_save_yourself'", g_type_name (G_TYPE_FROM_INSTANCE (self)));
	return;
}


/*    public abstract weak int[3] get_version ();*/
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


gint gmpc_plugin2_meta_data_get_data (GmpcPlugin2MetaData* self) {
	return GMPC_PLUGIN2_META_DATA_GET_INTERFACE (self)->get_data (self);
}


/* Set get priority */
gint gmpc_plugin2_meta_data_get_priority (GmpcPlugin2MetaData* self) {
	return GMPC_PLUGIN2_META_DATA_GET_INTERFACE (self)->get_priority (self);
}


void gmpc_plugin2_meta_data_set_priority (GmpcPlugin2MetaData* self, gint priority) {
	GMPC_PLUGIN2_META_DATA_GET_INTERFACE (self)->set_priority (self, priority);
}


static void gmpc_plugin2_meta_data_base_init (GmpcPlugin2MetaDataIface * iface) {
	static gboolean initialized = FALSE;
	if (!initialized) {
		initialized = TRUE;
	}
}


GType gmpc_plugin2_meta_data_get_type (void) {
	static GType gmpc_plugin2_meta_data_type_id = 0;
	if (gmpc_plugin2_meta_data_type_id == 0) {
		static const GTypeInfo g_define_type_info = { sizeof (GmpcPlugin2MetaDataIface), (GBaseInitFunc) gmpc_plugin2_meta_data_base_init, (GBaseFinalizeFunc) NULL, (GClassInitFunc) NULL, (GClassFinalizeFunc) NULL, NULL, 0, 0, (GInstanceInitFunc) NULL, NULL };
		gmpc_plugin2_meta_data_type_id = g_type_register_static (G_TYPE_INTERFACE, "GmpcPlugin2MetaData", &g_define_type_info, 0);
		g_type_interface_add_prerequisite (gmpc_plugin2_meta_data_type_id, GMPC_TYPE_PLUGIN_BASE);
	}
	return gmpc_plugin2_meta_data_type_id;
}


/* Function is called by gmpc, the plugin should then insert itself in the left tree  */
void gmpc_plugin2_browser_add (GmpcPlugin2Browser* self, GtkWidget* category_tree) {
	GMPC_PLUGIN2_BROWSER_GET_INTERFACE (self)->add (self, category_tree);
}


/* This gets called, the plugin should add it view in container */
void gmpc_plugin2_browser_selected (GmpcPlugin2Browser* self, GtkWidget* container) {
	GMPC_PLUGIN2_BROWSER_GET_INTERFACE (self)->selected (self, container);
}


/* Plugin should remove itself from container */
void gmpc_plugin2_browser_unselected (GmpcPlugin2Browser* self, GtkWidget* container) {
	GMPC_PLUGIN2_BROWSER_GET_INTERFACE (self)->unselected (self, container);
}


static void gmpc_plugin2_browser_base_init (GmpcPlugin2BrowserIface * iface) {
	static gboolean initialized = FALSE;
	if (!initialized) {
		initialized = TRUE;
	}
}


GType gmpc_plugin2_browser_get_type (void) {
	static GType gmpc_plugin2_browser_type_id = 0;
	if (gmpc_plugin2_browser_type_id == 0) {
		static const GTypeInfo g_define_type_info = { sizeof (GmpcPlugin2BrowserIface), (GBaseInitFunc) gmpc_plugin2_browser_base_init, (GBaseFinalizeFunc) NULL, (GClassInitFunc) NULL, (GClassFinalizeFunc) NULL, NULL, 0, 0, (GInstanceInitFunc) NULL, NULL };
		gmpc_plugin2_browser_type_id = g_type_register_static (G_TYPE_INTERFACE, "GmpcPlugin2Browser", &g_define_type_info, 0);
		g_type_interface_add_prerequisite (gmpc_plugin2_browser_type_id, GMPC_TYPE_PLUGIN_BASE);
	}
	return gmpc_plugin2_browser_type_id;
}


GtkWidget* gmpc_plugin2_preferences_pref_construct (GmpcPlugin2Preferences* self) {
	return GMPC_PLUGIN2_PREFERENCES_GET_INTERFACE (self)->pref_construct (self);
}


GtkWidget* gmpc_plugin2_preferences_pref_destroy (GmpcPlugin2Preferences* self) {
	return GMPC_PLUGIN2_PREFERENCES_GET_INTERFACE (self)->pref_destroy (self);
}


static void gmpc_plugin2_preferences_base_init (GmpcPlugin2PreferencesIface * iface) {
	static gboolean initialized = FALSE;
	if (!initialized) {
		initialized = TRUE;
	}
}


GType gmpc_plugin2_preferences_get_type (void) {
	static GType gmpc_plugin2_preferences_type_id = 0;
	if (gmpc_plugin2_preferences_type_id == 0) {
		static const GTypeInfo g_define_type_info = { sizeof (GmpcPlugin2PreferencesIface), (GBaseInitFunc) gmpc_plugin2_preferences_base_init, (GBaseFinalizeFunc) NULL, (GClassInitFunc) NULL, (GClassFinalizeFunc) NULL, NULL, 0, 0, (GInstanceInitFunc) NULL, NULL };
		gmpc_plugin2_preferences_type_id = g_type_register_static (G_TYPE_INTERFACE, "GmpcPlugin2Preferences", &g_define_type_info, 0);
		g_type_interface_add_prerequisite (gmpc_plugin2_preferences_type_id, GMPC_TYPE_PLUGIN_BASE);
	}
	return gmpc_plugin2_preferences_type_id;
}


gint gmpc_plugin2_song_list_song_list (GmpcPlugin2SongList* self, GtkWidget* tree, GtkMenu* menu) {
	return GMPC_PLUGIN2_SONG_LIST_GET_INTERFACE (self)->song_list (self, tree, menu);
}


static void gmpc_plugin2_song_list_base_init (GmpcPlugin2SongListIface * iface) {
	static gboolean initialized = FALSE;
	if (!initialized) {
		initialized = TRUE;
	}
}


GType gmpc_plugin2_song_list_get_type (void) {
	static GType gmpc_plugin2_song_list_type_id = 0;
	if (gmpc_plugin2_song_list_type_id == 0) {
		static const GTypeInfo g_define_type_info = { sizeof (GmpcPlugin2SongListIface), (GBaseInitFunc) gmpc_plugin2_song_list_base_init, (GBaseFinalizeFunc) NULL, (GClassInitFunc) NULL, (GClassFinalizeFunc) NULL, NULL, 0, 0, (GInstanceInitFunc) NULL, NULL };
		gmpc_plugin2_song_list_type_id = g_type_register_static (G_TYPE_INTERFACE, "GmpcPlugin2SongList", &g_define_type_info, 0);
		g_type_interface_add_prerequisite (gmpc_plugin2_song_list_type_id, GMPC_TYPE_PLUGIN_BASE);
	}
	return gmpc_plugin2_song_list_type_id;
}




