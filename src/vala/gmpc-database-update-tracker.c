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
#include <gtk/gtk.h>
#include <gtktransition.h>
#include <config.h>
#include <gmpc-plugin.h>
#include <stdlib.h>
#include <string.h>
#include <glib/gi18n-lib.h>
#include <time.h>
#include <playlist3-messages.h>
#include <plugin.h>
#include <gmpc-profiles.h>
#include <libmpd/libmpd.h>
#include <gmpc-connection.h>
#include <main.h>


#define GMPC_PLUGIN_TYPE_DATABASE_UPDATE_TRACKER (gmpc_plugin_database_update_tracker_get_type ())
#define GMPC_PLUGIN_DATABASE_UPDATE_TRACKER(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GMPC_PLUGIN_TYPE_DATABASE_UPDATE_TRACKER, GmpcPluginDatabaseUpdateTracker))
#define GMPC_PLUGIN_DATABASE_UPDATE_TRACKER_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GMPC_PLUGIN_TYPE_DATABASE_UPDATE_TRACKER, GmpcPluginDatabaseUpdateTrackerClass))
#define GMPC_PLUGIN_IS_DATABASE_UPDATE_TRACKER(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GMPC_PLUGIN_TYPE_DATABASE_UPDATE_TRACKER))
#define GMPC_PLUGIN_IS_DATABASE_UPDATE_TRACKER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GMPC_PLUGIN_TYPE_DATABASE_UPDATE_TRACKER))
#define GMPC_PLUGIN_DATABASE_UPDATE_TRACKER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GMPC_PLUGIN_TYPE_DATABASE_UPDATE_TRACKER, GmpcPluginDatabaseUpdateTrackerClass))

typedef struct _GmpcPluginDatabaseUpdateTracker GmpcPluginDatabaseUpdateTracker;
typedef struct _GmpcPluginDatabaseUpdateTrackerClass GmpcPluginDatabaseUpdateTrackerClass;
typedef struct _GmpcPluginDatabaseUpdateTrackerPrivate GmpcPluginDatabaseUpdateTrackerPrivate;
#define _g_object_unref0(var) ((var == NULL) ? NULL : (var = (g_object_unref (var), NULL)))
#define _g_free0(var) (var = (g_free (var), NULL))

struct _GmpcPluginDatabaseUpdateTracker {
	GmpcPluginBase parent_instance;
	GmpcPluginDatabaseUpdateTrackerPrivate * priv;
	gint* version;
	gint version_length1;
};

struct _GmpcPluginDatabaseUpdateTrackerClass {
	GmpcPluginBaseClass parent_class;
};

struct _GmpcPluginDatabaseUpdateTrackerPrivate {
	GtkImage* image;
};


static gpointer gmpc_plugin_database_update_tracker_parent_class = NULL;

#define use_transition TRUE
#define some_unique_name_mb VERSION
GType gmpc_plugin_database_update_tracker_get_type (void);
#define GMPC_PLUGIN_DATABASE_UPDATE_TRACKER_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GMPC_PLUGIN_TYPE_DATABASE_UPDATE_TRACKER, GmpcPluginDatabaseUpdateTrackerPrivate))
enum  {
	GMPC_PLUGIN_DATABASE_UPDATE_TRACKER_DUMMY_PROPERTY
};
static gint* gmpc_plugin_database_update_tracker_real_get_version (GmpcPluginBase* base, int* result_length1);
static const char* gmpc_plugin_database_update_tracker_real_get_name (GmpcPluginBase* base);
static void gmpc_plugin_database_update_tracker_start_updating (GmpcPluginDatabaseUpdateTracker* self);
static void gmpc_plugin_database_update_tracker_stop_updating (GmpcPluginDatabaseUpdateTracker* self);
static void gmpc_plugin_database_update_tracker_show_message (GmpcPluginDatabaseUpdateTracker* self, gint db_time);
static void gmpc_plugin_database_update_tracker_connection_changed (GmpcPluginDatabaseUpdateTracker* self, GmpcConnection* gc, MpdObj* server, gint connection);
static void gmpc_plugin_database_update_tracker_status_changed (GmpcPluginDatabaseUpdateTracker* self, GmpcConnection* gc, MpdObj* server, ChangedStatusType what);
GmpcPluginDatabaseUpdateTracker* gmpc_plugin_database_update_tracker_new (void);
GmpcPluginDatabaseUpdateTracker* gmpc_plugin_database_update_tracker_construct (GType object_type);
static void _gmpc_plugin_database_update_tracker_status_changed_gmpc_connection_status_changed (GmpcConnection* _sender, MpdObj* server, ChangedStatusType what, gpointer self);
static void _gmpc_plugin_database_update_tracker_connection_changed_gmpc_connection_connection_changed (GmpcConnection* _sender, MpdObj* server, gint connect, gpointer self);
static GObject * gmpc_plugin_database_update_tracker_constructor (GType type, guint n_construct_properties, GObjectConstructParam * construct_properties);
static void gmpc_plugin_database_update_tracker_finalize (GObject* obj);



static gint* gmpc_plugin_database_update_tracker_real_get_version (GmpcPluginBase* base, int* result_length1) {
	GmpcPluginDatabaseUpdateTracker * self;
	gint* result;
	gint* _tmp0_;
	self = (GmpcPluginDatabaseUpdateTracker*) base;
	result = (_tmp0_ = self->version, *result_length1 = self->version_length1, _tmp0_);
	return result;
}


static const char* gmpc_plugin_database_update_tracker_real_get_name (GmpcPluginBase* base) {
	GmpcPluginDatabaseUpdateTracker * self;
	const char* result;
	self = (GmpcPluginDatabaseUpdateTracker*) base;
	result = "Database Update Tracker";
	return result;
}


static void gmpc_plugin_database_update_tracker_start_updating (GmpcPluginDatabaseUpdateTracker* self) {
	GtkImage* _tmp0_;
	g_return_if_fail (self != NULL);
	self->priv->image = (_tmp0_ = g_object_ref_sink ((GtkImage*) gtk_image_new_from_icon_name ("gtk-refresh", GTK_ICON_SIZE_MENU)), _g_object_unref0 (self->priv->image), _tmp0_);
	gtk_widget_show ((GtkWidget*) self->priv->image);
	gtk_widget_set_tooltip_text ((GtkWidget*) self->priv->image, _ ("MPD is rescanning the database"));
	main_window_add_status_icon ((GtkWidget*) self->priv->image);
}


static void gmpc_plugin_database_update_tracker_stop_updating (GmpcPluginDatabaseUpdateTracker* self) {
	GtkImage* _tmp0_;
	g_return_if_fail (self != NULL);
	if (self->priv->image == NULL) {
		return;
	}
	gtk_container_remove (gtk_widget_get_parent ((GtkWidget*) self->priv->image), (GtkWidget*) self->priv->image);
	self->priv->image = (_tmp0_ = NULL, _g_object_unref0 (self->priv->image), _tmp0_);
}


static void g_time_local (time_t time, struct tm* result) {
	struct tm _result_ = {0};
	localtime_r (&time, &_result_);
	*result = _result_;
	return;
}


static char* g_time_format (struct tm *self, const char* format) {
	char* result;
	gchar* _tmp0_;
	gint buffer_size;
	gint buffer_length1;
	gchar* buffer;
	g_return_val_if_fail (format != NULL, NULL);
	buffer = (_tmp0_ = g_new0 (gchar, 64), buffer_length1 = 64, buffer_size = buffer_length1, _tmp0_);
	strftime (buffer, buffer_length1, format, &(*self));
	result = g_strdup ((const char*) buffer);
	buffer = (g_free (buffer), NULL);
	return result;
}


static void gmpc_plugin_database_update_tracker_show_message (GmpcPluginDatabaseUpdateTracker* self, gint db_time) {
	time_t r_time;
	char* message;
	struct tm _tmp0_ = {0};
	struct tm tm;
	char* _tmp2_;
	char* _tmp1_;
	g_return_if_fail (self != NULL);
	r_time = (time_t) db_time;
	message = NULL;
	tm = (g_time_local (r_time, &_tmp0_), _tmp0_);
	message = (_tmp2_ = g_strdup_printf ("%s %s", _ ("MPD Database has been updated at:"), _tmp1_ = g_time_format (&tm, "%c")), _g_free0 (message), _tmp2_);
	_g_free0 (_tmp1_);
	playlist3_show_error_message ((const char*) message, ERROR_INFO);
	_g_free0 (message);
}


static void gmpc_plugin_database_update_tracker_connection_changed (GmpcPluginDatabaseUpdateTracker* self, GmpcConnection* gc, MpdObj* server, gint connection) {
	g_return_if_fail (self != NULL);
	g_return_if_fail (gc != NULL);
	g_return_if_fail (server != NULL);
	if (connection == 1) {
		char* id;
		id = gmpc_profiles_get_current (gmpc_profiles);
		if (id != NULL) {
			gint dut;
			gint serv_dut;
			dut = gmpc_profiles_get_db_update_time (gmpc_profiles, id);
			serv_dut = mpd_server_get_database_update_time (server);
			if (dut != serv_dut) {
				gmpc_plugin_database_update_tracker_show_message (self, serv_dut);
				gmpc_profiles_set_db_update_time (gmpc_profiles, id, serv_dut);
			}
		}
		if (mpd_status_db_is_updating (server)) {
			gmpc_plugin_database_update_tracker_start_updating (self);
		}
		_g_free0 (id);
	}
}


static void gmpc_plugin_database_update_tracker_status_changed (GmpcPluginDatabaseUpdateTracker* self, GmpcConnection* gc, MpdObj* server, ChangedStatusType what) {
	g_return_if_fail (self != NULL);
	g_return_if_fail (gc != NULL);
	g_return_if_fail (server != NULL);
	if (!gmpc_plugin_base_get_enabled ((GmpcPluginBase*) self)) {
		return;
	}
	if ((what & MPD_CST_UPDATING) == MPD_CST_UPDATING) {
		if (mpd_status_db_is_updating (server)) {
			gmpc_plugin_database_update_tracker_start_updating (self);
		} else {
			gmpc_plugin_database_update_tracker_stop_updating (self);
		}
	}
	if ((what & MPD_CST_DATABASE) == MPD_CST_DATABASE) {
		char* id;
		id = gmpc_profiles_get_current (gmpc_profiles);
		if (id != NULL) {
			gint dut;
			gint serv_dut;
			dut = gmpc_profiles_get_db_update_time (gmpc_profiles, id);
			serv_dut = mpd_server_get_database_update_time (connection);
			if (dut != serv_dut) {
				gmpc_plugin_database_update_tracker_show_message (self, serv_dut);
				gmpc_profiles_set_db_update_time (gmpc_profiles, id, serv_dut);
			}
			gmpc_plugin_database_update_tracker_stop_updating (self);
		}
		_g_free0 (id);
	}
}


GmpcPluginDatabaseUpdateTracker* gmpc_plugin_database_update_tracker_construct (GType object_type) {
	GmpcPluginDatabaseUpdateTracker * self;
	self = g_object_newv (object_type, 0, NULL);
	return self;
}


GmpcPluginDatabaseUpdateTracker* gmpc_plugin_database_update_tracker_new (void) {
	return gmpc_plugin_database_update_tracker_construct (GMPC_PLUGIN_TYPE_DATABASE_UPDATE_TRACKER);
}


static void _gmpc_plugin_database_update_tracker_status_changed_gmpc_connection_status_changed (GmpcConnection* _sender, MpdObj* server, ChangedStatusType what, gpointer self) {
	gmpc_plugin_database_update_tracker_status_changed (self, _sender, server, what);
}


static void _gmpc_plugin_database_update_tracker_connection_changed_gmpc_connection_connection_changed (GmpcConnection* _sender, MpdObj* server, gint connect, gpointer self) {
	gmpc_plugin_database_update_tracker_connection_changed (self, _sender, server, connect);
}


static GObject * gmpc_plugin_database_update_tracker_constructor (GType type, guint n_construct_properties, GObjectConstructParam * construct_properties) {
	GObject * obj;
	GObjectClass * parent_class;
	GmpcPluginDatabaseUpdateTracker * self;
	parent_class = G_OBJECT_CLASS (gmpc_plugin_database_update_tracker_parent_class);
	obj = parent_class->constructor (type, n_construct_properties, construct_properties);
	self = GMPC_PLUGIN_DATABASE_UPDATE_TRACKER (obj);
	{
		((GmpcPluginBase*) self)->plugin_type = 8 + 4;
		g_signal_connect_object (gmpcconn, "status-changed", (GCallback) _gmpc_plugin_database_update_tracker_status_changed_gmpc_connection_status_changed, self, 0);
		g_signal_connect_object (gmpcconn, "connection-changed", (GCallback) _gmpc_plugin_database_update_tracker_connection_changed_gmpc_connection_connection_changed, self, 0);
	}
	return obj;
}


static void gmpc_plugin_database_update_tracker_class_init (GmpcPluginDatabaseUpdateTrackerClass * klass) {
	gmpc_plugin_database_update_tracker_parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (GmpcPluginDatabaseUpdateTrackerPrivate));
	GMPC_PLUGIN_BASE_CLASS (klass)->get_version = gmpc_plugin_database_update_tracker_real_get_version;
	GMPC_PLUGIN_BASE_CLASS (klass)->get_name = gmpc_plugin_database_update_tracker_real_get_name;
	G_OBJECT_CLASS (klass)->constructor = gmpc_plugin_database_update_tracker_constructor;
	G_OBJECT_CLASS (klass)->finalize = gmpc_plugin_database_update_tracker_finalize;
}


static void gmpc_plugin_database_update_tracker_instance_init (GmpcPluginDatabaseUpdateTracker * self) {
	gint* _tmp0_;
	self->priv = GMPC_PLUGIN_DATABASE_UPDATE_TRACKER_GET_PRIVATE (self);
	self->priv->image = NULL;
	self->version = (_tmp0_ = g_new0 (gint, 3), _tmp0_[0] = 0, _tmp0_[1] = 0, _tmp0_[2] = 2, _tmp0_);
	self->version_length1 = 3;
	_tmp0_ = NULL;
}


static void gmpc_plugin_database_update_tracker_finalize (GObject* obj) {
	GmpcPluginDatabaseUpdateTracker * self;
	self = GMPC_PLUGIN_DATABASE_UPDATE_TRACKER (obj);
	_g_object_unref0 (self->priv->image);
	self->version = (g_free (self->version), NULL);
	G_OBJECT_CLASS (gmpc_plugin_database_update_tracker_parent_class)->finalize (obj);
}


GType gmpc_plugin_database_update_tracker_get_type (void) {
	static GType gmpc_plugin_database_update_tracker_type_id = 0;
	if (gmpc_plugin_database_update_tracker_type_id == 0) {
		static const GTypeInfo g_define_type_info = { sizeof (GmpcPluginDatabaseUpdateTrackerClass), (GBaseInitFunc) NULL, (GBaseFinalizeFunc) NULL, (GClassInitFunc) gmpc_plugin_database_update_tracker_class_init, (GClassFinalizeFunc) NULL, NULL, sizeof (GmpcPluginDatabaseUpdateTracker), 0, (GInstanceInitFunc) gmpc_plugin_database_update_tracker_instance_init, NULL };
		gmpc_plugin_database_update_tracker_type_id = g_type_register_static (GMPC_PLUGIN_TYPE_BASE, "GmpcPluginDatabaseUpdateTracker", &g_define_type_info, 0);
	}
	return gmpc_plugin_database_update_tracker_type_id;
}




