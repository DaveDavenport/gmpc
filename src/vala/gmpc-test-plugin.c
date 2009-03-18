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

#include "gmpc-test-plugin.h"
#include <plugin.h>
#include <config1.h>
#include <stdio.h>
#include <gmpc-connection.h>
#include <libmpd/libmpd.h>
#include <gdk-pixbuf/gdk-pixdata.h>
#include <main.h>
#include <libmpd/libmpdclient.h>
#include <metadata.h>




struct _GmpcTestPluginPrivate {
	GtkWindow* window;
	GtkListStore* model;
};

#define GMPC_TEST_PLUGIN_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GMPC_TYPE_TEST_PLUGIN, GmpcTestPluginPrivate))
enum  {
	GMPC_TEST_PLUGIN_DUMMY_PROPERTY
};
static gint* gmpc_test_plugin_real_get_version (GmpcPluginBase* base, int* result_length1);
static const char* gmpc_test_plugin_real_get_name (GmpcPluginBase* base);
static void gmpc_test_plugin_real_save_yourself (GmpcPluginBase* base);
static gboolean gmpc_test_plugin_real_get_enabled (GmpcPluginBase* base);
static void gmpc_test_plugin_real_set_enabled (GmpcPluginBase* base, gboolean state);
static void gmpc_test_plugin_real_pane_construct (GmpcPluginPreferencesIface* base, GtkContainer* container);
static void gmpc_test_plugin_real_pane_destroy (GmpcPluginPreferencesIface* base, GtkContainer* container);
static void gmpc_test_plugin_connection_changed (GmpcTestPlugin* self, GmpcConnection* conn, MpdObj* server, gint connect);
static void _gmpc_test_plugin_image_downloaded_gmpc_async_download_callback (const GEADAsyncHandler* handle, GEADStatus status, gpointer self);
static void _gmpc_test_plugin_callback_gmpc_meta_data_callback (GList* list, gpointer self);
static gboolean _gmpc_test_plugin_window_delete_event_gtk_widget_delete_event (GtkWindow* _sender, const GdkEvent* event, gpointer self);
static void _gmpc_test_plugin_menu_activated_gtk_menu_item_activate (GtkMenuItem* _sender, gpointer self);
static gint gmpc_test_plugin_real_tool_menu_integration (GmpcPluginToolMenuIface* base, GtkMenu* menu);
static void _gmpc_test_plugin_connection_changed_gmpc_connection_connection_changed (GmpcConnection* _sender, MpdObj* server, gint connect, gpointer self);
static GObject * gmpc_test_plugin_constructor (GType type, guint n_construct_properties, GObjectConstructParam * construct_properties);
static gpointer gmpc_test_plugin_parent_class = NULL;
static GmpcPluginPreferencesIfaceIface* gmpc_test_plugin_gmpc_plugin_preferences_iface_parent_iface = NULL;
static GmpcPluginToolMenuIfaceIface* gmpc_test_plugin_gmpc_plugin_tool_menu_iface_parent_iface = NULL;
static void gmpc_test_plugin_finalize (GObject* obj);



/*********************************************************************************
     * Plugin base functions 
     * These functions are required.
     ********************************************************************************/
static gint* gmpc_test_plugin_real_get_version (GmpcPluginBase* base, int* result_length1) {
	GmpcTestPlugin * self;
	gint* _tmp0;
	self = (GmpcTestPlugin*) base;
	_tmp0 = NULL;
	return (_tmp0 = (gint*) GMPC_TEST_PLUGIN_version, *result_length1 = -1, _tmp0);
}


/**
     * The name of the plugin
     */
static const char* gmpc_test_plugin_real_get_name (GmpcPluginBase* base) {
	GmpcTestPlugin * self;
	self = (GmpcTestPlugin*) base;
	return "Vala test plugin";
}


/**
     * Tells the plugin to save itself
     */
static void gmpc_test_plugin_real_save_yourself (GmpcPluginBase* base) {
	GmpcTestPlugin * self;
	self = (GmpcTestPlugin*) base;
}


/* nothing to save 
*
     * Get set enabled
     */
static gboolean gmpc_test_plugin_real_get_enabled (GmpcPluginBase* base) {
	GmpcTestPlugin * self;
	self = (GmpcTestPlugin*) base;
	return (gboolean) cfg_get_single_value_as_int_with_default (config, gmpc_plugin_base_get_name ((GmpcPluginBase*) self), "enabled", 1);
}


static void gmpc_test_plugin_real_set_enabled (GmpcPluginBase* base, gboolean state) {
	GmpcTestPlugin * self;
	self = (GmpcTestPlugin*) base;
	cfg_set_single_value_as_int (config, gmpc_plugin_base_get_name ((GmpcPluginBase*) self), "enabled", (gint) state);
}


/*********************************************************************************
     * Plugin preferences functions 
     ********************************************************************************/
static void gmpc_test_plugin_real_pane_construct (GmpcPluginPreferencesIface* base, GtkContainer* container) {
	GmpcTestPlugin * self;
	GtkHBox* box;
	GtkLabel* label;
	self = (GmpcTestPlugin*) base;
	g_return_if_fail (container != NULL);
	box = g_object_ref_sink ((GtkHBox*) gtk_hbox_new (FALSE, 6));
	label = g_object_ref_sink ((GtkLabel*) gtk_label_new ("This is a test preferences pane"));
	gtk_box_pack_start ((GtkBox*) box, (GtkWidget*) label, FALSE, FALSE, (guint) 0);
	gtk_container_add (container, (GtkWidget*) box);
	gtk_widget_show_all ((GtkWidget*) container);
	fprintf (stdout, "%s: Create preferences panel\n", gmpc_plugin_base_get_name ((GmpcPluginBase*) self));
	(box == NULL) ? NULL : (box = (g_object_unref (box), NULL));
	(label == NULL) ? NULL : (label = (g_object_unref (label), NULL));
}


static void gmpc_test_plugin_real_pane_destroy (GmpcPluginPreferencesIface* base, GtkContainer* container) {
	GmpcTestPlugin * self;
	GtkBin* _tmp0;
	GtkBin* bin;
	self = (GmpcTestPlugin*) base;
	g_return_if_fail (container != NULL);
	_tmp0 = NULL;
	bin = (_tmp0 = GTK_BIN (container), (_tmp0 == NULL) ? NULL : g_object_ref (_tmp0));
	gtk_object_destroy ((GtkObject*) bin->child);
	fprintf (stdout, "%s: Destroy preferences panel\n", gmpc_plugin_base_get_name ((GmpcPluginBase*) self));
	(bin == NULL) ? NULL : (bin = (g_object_unref (bin), NULL));
}


/*********************************************************************************
     * Private  
     *******************************************************************************
 Plugin functions */
static void gmpc_test_plugin_connection_changed (GmpcTestPlugin* self, GmpcConnection* conn, MpdObj* server, gint connect) {
	g_return_if_fail (self != NULL);
	g_return_if_fail (conn != NULL);
	g_return_if_fail (server != NULL);
	fprintf (stdout, "%s: Connection changed: %i\n", gmpc_plugin_base_get_name ((GmpcPluginBase*) self), connect);
}


void gmpc_test_plugin_image_downloaded (GmpcTestPlugin* self, const GEADAsyncHandler* handle, GEADStatus status) {
	GError * inner_error;
	g_return_if_fail (self != NULL);
	g_return_if_fail (handle != NULL);
	inner_error = NULL;
	if (status == GEAD_DONE) {
		gint64 length;
		const char* data;
		length = 0LL;
		data = gmpc_easy_handler_get_data (handle, &length);
		{
			GtkTreeIter iter = {0};
			GdkPixbufLoader* load;
			GdkPixbuf* _tmp0;
			GdkPixbuf* pb;
			load = gdk_pixbuf_loader_new ();
			gdk_pixbuf_loader_set_size (load, 150, 150);
			{
				gdk_pixbuf_loader_write (load, (guchar*) data, -1, &inner_error);
				if (inner_error != NULL) {
					goto __catch1_g_error;
					goto __finally1;
				}
			}
			goto __finally1;
			__catch1_g_error:
			{
				GError * e;
				e = inner_error;
				inner_error = NULL;
				{
					fprintf (stdout, "Failed to load file: %s::%s\n", e->message, gmpc_easy_handler_get_uri (handle));
					(e == NULL) ? NULL : (e = (g_error_free (e), NULL));
				}
			}
			__finally1:
			if (inner_error != NULL) {
				(load == NULL) ? NULL : (load = (g_object_unref (load), NULL));
				goto __catch0_g_error;
				goto __finally0;
			}
			gdk_pixbuf_loader_close (load, &inner_error);
			if (inner_error != NULL) {
				(load == NULL) ? NULL : (load = (g_object_unref (load), NULL));
				goto __catch0_g_error;
				goto __finally0;
			}
			_tmp0 = NULL;
			pb = (_tmp0 = gdk_pixbuf_loader_get_pixbuf (load), (_tmp0 == NULL) ? NULL : g_object_ref (_tmp0));
			/*new Gdk.Pixbuf.from_inline((int)length, (uchar[])data, true); */
			gtk_list_store_append (self->priv->model, &iter);
			gtk_list_store_set (self->priv->model, &iter, 0, pb, 1, gmpc_easy_handler_get_uri (handle), -1, -1);
			(load == NULL) ? NULL : (load = (g_object_unref (load), NULL));
			(pb == NULL) ? NULL : (pb = (g_object_unref (pb), NULL));
		}
		goto __finally0;
		__catch0_g_error:
		{
			GError * e;
			e = inner_error;
			inner_error = NULL;
			{
				fprintf (stdout, "Failed to load file: %s::%s\n", e->message, gmpc_easy_handler_get_uri (handle));
				(e == NULL) ? NULL : (e = (g_error_free (e), NULL));
				return;
			}
		}
		__finally0:
		if (inner_error != NULL) {
			g_critical ("file %s: line %d: uncaught error: %s", __FILE__, __LINE__, inner_error->message);
			g_clear_error (&inner_error);
			return;
		}
	}
}


static void _gmpc_test_plugin_image_downloaded_gmpc_async_download_callback (const GEADAsyncHandler* handle, GEADStatus status, gpointer self) {
	gmpc_test_plugin_image_downloaded (self, handle, status);
}


void gmpc_test_plugin_callback (GmpcTestPlugin* self, GList* list) {
	GError * inner_error;
	g_return_if_fail (self != NULL);
	g_return_if_fail (list != NULL);
	inner_error = NULL;
	{
		GList* uri_collection;
		GList* uri_it;
		uri_collection = list;
		for (uri_it = uri_collection; uri_it != NULL; uri_it = uri_it->next) {
			const char* uri;
			uri = (const char*) uri_it->data;
			{
				fprintf (stdout, "Uri: %s\n", uri);
				if (g_utf8_get_char (g_utf8_offset_to_pointer (uri, 0)) == '/') {
					GtkTreeIter iter = {0};
					{
						GdkPixbuf* pb;
						pb = gdk_pixbuf_new_from_file_at_scale (uri, 150, 150, TRUE, &inner_error);
						if (inner_error != NULL) {
							goto __catch2_g_error;
							goto __finally2;
						}
						gtk_list_store_append (self->priv->model, &iter);
						gtk_list_store_set (self->priv->model, &iter, 0, pb, 1, uri, -1, -1);
						(pb == NULL) ? NULL : (pb = (g_object_unref (pb), NULL));
					}
					goto __finally2;
					__catch2_g_error:
					{
						GError * e;
						e = inner_error;
						inner_error = NULL;
						{
							(e == NULL) ? NULL : (e = (g_error_free (e), NULL));
						}
					}
					__finally2:
					if (inner_error != NULL) {
						g_critical ("file %s: line %d: uncaught error: %s", __FILE__, __LINE__, inner_error->message);
						g_clear_error (&inner_error);
						return;
					}
				} else {
					gmpc_easy_async_downloader (uri, _gmpc_test_plugin_image_downloaded_gmpc_async_download_callback, self);
				}
			}
		}
	}
}


gboolean gmpc_test_plugin_window_delete_event (GmpcTestPlugin* self, GtkWindow* win) {
	g_return_val_if_fail (self != NULL, FALSE);
	g_return_val_if_fail (win != NULL, FALSE);
	gtk_widget_hide ((GtkWidget*) self->priv->window);
	return TRUE;
}


static void _gmpc_test_plugin_callback_gmpc_meta_data_callback (GList* list, gpointer self) {
	gmpc_test_plugin_callback (self, list);
}


static gboolean _gmpc_test_plugin_window_delete_event_gtk_widget_delete_event (GtkWindow* _sender, const GdkEvent* event, gpointer self) {
	return gmpc_test_plugin_window_delete_event (self, _sender);
}


void gmpc_test_plugin_menu_activated (GmpcTestPlugin* self, GtkMenuItem* item) {
	GtkWindow* _tmp0;
	GtkListStore* _tmp1;
	GtkScrolledWindow* sw;
	GtkIconView* iv;
	const mpd_Song* song;
	g_return_if_fail (self != NULL);
	g_return_if_fail (item != NULL);
	if (self->priv->window != NULL) {
		const mpd_Song* song;
		gtk_window_present (self->priv->window);
		gtk_widget_show_all ((GtkWidget*) self->priv->window);
		gtk_list_store_clear (self->priv->model);
		song = mpd_playlist_get_current_song (connection);
		if (song != NULL) {
			metadata_get_list (song, META_ALBUM_ART, _gmpc_test_plugin_callback_gmpc_meta_data_callback, self);
		}
		return;
	}
	_tmp0 = NULL;
	self->priv->window = (_tmp0 = g_object_ref_sink ((GtkWindow*) gtk_window_new (GTK_WINDOW_TOPLEVEL)), (self->priv->window == NULL) ? NULL : (self->priv->window = (g_object_unref (self->priv->window), NULL)), _tmp0);
	_tmp1 = NULL;
	self->priv->model = (_tmp1 = gtk_list_store_new (2, GDK_TYPE_PIXBUF, G_TYPE_STRING, NULL), (self->priv->model == NULL) ? NULL : (self->priv->model = (g_object_unref (self->priv->model), NULL)), _tmp1);
	sw = g_object_ref_sink ((GtkScrolledWindow*) gtk_scrolled_window_new (NULL, NULL));
	iv = g_object_ref_sink ((GtkIconView*) gtk_icon_view_new ());
	gtk_icon_view_set_model (iv, (GtkTreeModel*) self->priv->model);
	gtk_icon_view_set_pixbuf_column (iv, 0);
	gtk_container_add ((GtkContainer*) self->priv->window, (GtkWidget*) sw);
	gtk_container_add ((GtkContainer*) sw, (GtkWidget*) iv);
	gtk_widget_show_all ((GtkWidget*) self->priv->window);
	g_signal_connect_object ((GtkWidget*) self->priv->window, "delete-event", (GCallback) _gmpc_test_plugin_window_delete_event_gtk_widget_delete_event, self, 0);
	/*        this.window.hide_on_delete();*/
	song = mpd_playlist_get_current_song (connection);
	if (song != NULL) {
		metadata_get_list (song, META_ALBUM_ART, _gmpc_test_plugin_callback_gmpc_meta_data_callback, self);
	}
	(sw == NULL) ? NULL : (sw = (g_object_unref (sw), NULL));
	(iv == NULL) ? NULL : (iv = (g_object_unref (iv), NULL));
}


static void _gmpc_test_plugin_menu_activated_gtk_menu_item_activate (GtkMenuItem* _sender, gpointer self) {
	gmpc_test_plugin_menu_activated (self, _sender);
}


static gint gmpc_test_plugin_real_tool_menu_integration (GmpcPluginToolMenuIface* base, GtkMenu* menu) {
	GmpcTestPlugin * self;
	GtkMenuItem* item;
	gint _tmp0;
	self = (GmpcTestPlugin*) base;
	g_return_val_if_fail (menu != NULL, 0);
	item = g_object_ref_sink ((GtkMenuItem*) gtk_menu_item_new_with_label ("Test plugin"));
	gtk_menu_shell_append ((GtkMenuShell*) menu, (GtkWidget*) item);
	g_signal_connect_object (item, "activate", (GCallback) _gmpc_test_plugin_menu_activated_gtk_menu_item_activate, self, 0);
	return (_tmp0 = 0, (item == NULL) ? NULL : (item = (g_object_unref (item), NULL)), _tmp0);
}


GmpcTestPlugin* gmpc_test_plugin_construct (GType object_type) {
	GmpcTestPlugin * self;
	self = g_object_newv (object_type, 0, NULL);
	return self;
}


GmpcTestPlugin* gmpc_test_plugin_new (void) {
	return gmpc_test_plugin_construct (GMPC_TYPE_TEST_PLUGIN);
}


static void _gmpc_test_plugin_connection_changed_gmpc_connection_connection_changed (GmpcConnection* _sender, MpdObj* server, gint connect, gpointer self) {
	gmpc_test_plugin_connection_changed (self, _sender, server, connect);
}


static GObject * gmpc_test_plugin_constructor (GType type, guint n_construct_properties, GObjectConstructParam * construct_properties) {
	GObject * obj;
	GmpcTestPluginClass * klass;
	GObjectClass * parent_class;
	GmpcTestPlugin * self;
	klass = GMPC_TEST_PLUGIN_CLASS (g_type_class_peek (GMPC_TYPE_TEST_PLUGIN));
	parent_class = G_OBJECT_CLASS (g_type_class_peek_parent (klass));
	obj = parent_class->constructor (type, n_construct_properties, construct_properties);
	self = GMPC_TEST_PLUGIN (obj);
	{
		fprintf (stdout, "create %s\n", gmpc_plugin_base_get_name ((GmpcPluginBase*) self));
		g_signal_connect_object (gmpcconn, "connection-changed", (GCallback) _gmpc_test_plugin_connection_changed_gmpc_connection_connection_changed, self, 0);
	}
	return obj;
}


static void gmpc_test_plugin_class_init (GmpcTestPluginClass * klass) {
	gmpc_test_plugin_parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (GmpcTestPluginPrivate));
	G_OBJECT_CLASS (klass)->constructor = gmpc_test_plugin_constructor;
	G_OBJECT_CLASS (klass)->finalize = gmpc_test_plugin_finalize;
	GMPC_PLUGIN_BASE_CLASS (klass)->get_version = gmpc_test_plugin_real_get_version;
	GMPC_PLUGIN_BASE_CLASS (klass)->get_name = gmpc_test_plugin_real_get_name;
	GMPC_PLUGIN_BASE_CLASS (klass)->save_yourself = gmpc_test_plugin_real_save_yourself;
	GMPC_PLUGIN_BASE_CLASS (klass)->get_enabled = gmpc_test_plugin_real_get_enabled;
	GMPC_PLUGIN_BASE_CLASS (klass)->set_enabled = gmpc_test_plugin_real_set_enabled;
}


static void gmpc_test_plugin_gmpc_plugin_preferences_iface_interface_init (GmpcPluginPreferencesIfaceIface * iface) {
	gmpc_test_plugin_gmpc_plugin_preferences_iface_parent_iface = g_type_interface_peek_parent (iface);
	iface->pane_construct = gmpc_test_plugin_real_pane_construct;
	iface->pane_destroy = gmpc_test_plugin_real_pane_destroy;
}


static void gmpc_test_plugin_gmpc_plugin_tool_menu_iface_interface_init (GmpcPluginToolMenuIfaceIface * iface) {
	gmpc_test_plugin_gmpc_plugin_tool_menu_iface_parent_iface = g_type_interface_peek_parent (iface);
	iface->tool_menu_integration = gmpc_test_plugin_real_tool_menu_integration;
}


static void gmpc_test_plugin_instance_init (GmpcTestPlugin * self) {
	self->priv = GMPC_TEST_PLUGIN_GET_PRIVATE (self);
	self->priv->window = NULL;
	self->priv->model = NULL;
}


static void gmpc_test_plugin_finalize (GObject* obj) {
	GmpcTestPlugin * self;
	self = GMPC_TEST_PLUGIN (obj);
	{
		fprintf (stdout, "Destroying %s\n", gmpc_plugin_base_get_name ((GmpcPluginBase*) self));
	}
	(self->priv->window == NULL) ? NULL : (self->priv->window = (g_object_unref (self->priv->window), NULL));
	(self->priv->model == NULL) ? NULL : (self->priv->model = (g_object_unref (self->priv->model), NULL));
	G_OBJECT_CLASS (gmpc_test_plugin_parent_class)->finalize (obj);
}


GType gmpc_test_plugin_get_type (void) {
	static GType gmpc_test_plugin_type_id = 0;
	if (gmpc_test_plugin_type_id == 0) {
		static const GTypeInfo g_define_type_info = { sizeof (GmpcTestPluginClass), (GBaseInitFunc) NULL, (GBaseFinalizeFunc) NULL, (GClassInitFunc) gmpc_test_plugin_class_init, (GClassFinalizeFunc) NULL, NULL, sizeof (GmpcTestPlugin), 0, (GInstanceInitFunc) gmpc_test_plugin_instance_init, NULL };
		static const GInterfaceInfo gmpc_plugin_preferences_iface_info = { (GInterfaceInitFunc) gmpc_test_plugin_gmpc_plugin_preferences_iface_interface_init, (GInterfaceFinalizeFunc) NULL, NULL};
		static const GInterfaceInfo gmpc_plugin_tool_menu_iface_info = { (GInterfaceInitFunc) gmpc_test_plugin_gmpc_plugin_tool_menu_iface_interface_init, (GInterfaceFinalizeFunc) NULL, NULL};
		gmpc_test_plugin_type_id = g_type_register_static (GMPC_PLUGIN_TYPE_BASE, "GmpcTestPlugin", &g_define_type_info, 0);
		g_type_add_interface_static (gmpc_test_plugin_type_id, GMPC_PLUGIN_TYPE_PREFERENCES_IFACE, &gmpc_plugin_preferences_iface_info);
		g_type_add_interface_static (gmpc_test_plugin_type_id, GMPC_PLUGIN_TYPE_TOOL_MENU_IFACE, &gmpc_plugin_tool_menu_iface_info);
	}
	return gmpc_test_plugin_type_id;
}




