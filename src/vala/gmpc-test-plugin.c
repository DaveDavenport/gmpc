
#include "gmpc-test-plugin.h"
#include <stdio.h>
#include <gmpc-connection.h>
#include <libmpd/libmpd.h>
#include <main.h>




enum  {
	GMPC_TEST_PLUGIN_DUMMY_PROPERTY
};
static const char* gmpc_test_plugin_real_get_name (GmpcPluginBase* base);
static void gmpc_test_plugin_real_save_yourself (GmpcPluginBase* base);
static gboolean gmpc_test_plugin_real_get_enabled (GmpcPluginBase* base);
static void gmpc_test_plugin_real_set_enabled (GmpcPluginBase* base, gboolean state);
static void gmpc_test_plugin_connection_changed (GmpcTestPlugin* self, GmpcConnection* conn, MpdObj* server, gint connect);
static void _gmpc_test_plugin_connection_changed_gmpc_connection_connection_changed (GmpcConnection* _sender, MpdObj* server, gint connect, gpointer self);
static GObject * gmpc_test_plugin_constructor (GType type, guint n_construct_properties, GObjectConstructParam * construct_properties);
static gpointer gmpc_test_plugin_parent_class = NULL;
static void gmpc_test_plugin_finalize (GObject* obj);



static const char* gmpc_test_plugin_real_get_name (GmpcPluginBase* base) {
	GmpcTestPlugin * self;
	self = (GmpcTestPlugin*) base;
	return "Vala test plugin";
}


static void gmpc_test_plugin_real_save_yourself (GmpcPluginBase* base) {
	GmpcTestPlugin * self;
	self = (GmpcTestPlugin*) base;
	fprintf (stdout, "Vala plugin save myself\n");
}


/* nothing to save */
static gboolean gmpc_test_plugin_real_get_enabled (GmpcPluginBase* base) {
	GmpcTestPlugin * self;
	self = (GmpcTestPlugin*) base;
	return TRUE;
}


static void gmpc_test_plugin_real_set_enabled (GmpcPluginBase* base, gboolean state) {
	GmpcTestPlugin * self;
	self = (GmpcTestPlugin*) base;
}


/* Plugin functions */
static void gmpc_test_plugin_connection_changed (GmpcTestPlugin* self, GmpcConnection* conn, MpdObj* server, gint connect) {
	g_return_if_fail (self != NULL);
	g_return_if_fail (conn != NULL);
	g_return_if_fail (server != NULL);
	fprintf (stdout, "Connection changed: %i\n", connect);
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
	G_OBJECT_CLASS (klass)->constructor = gmpc_test_plugin_constructor;
	G_OBJECT_CLASS (klass)->finalize = gmpc_test_plugin_finalize;
	GMPC_PLUGIN_BASE_CLASS (klass)->get_name = gmpc_test_plugin_real_get_name;
	GMPC_PLUGIN_BASE_CLASS (klass)->save_yourself = gmpc_test_plugin_real_save_yourself;
	GMPC_PLUGIN_BASE_CLASS (klass)->get_enabled = gmpc_test_plugin_real_get_enabled;
	GMPC_PLUGIN_BASE_CLASS (klass)->set_enabled = gmpc_test_plugin_real_set_enabled;
}


static void gmpc_test_plugin_instance_init (GmpcTestPlugin * self) {
}


static void gmpc_test_plugin_finalize (GObject* obj) {
	GmpcTestPlugin * self;
	self = GMPC_TEST_PLUGIN (obj);
	{
		fprintf (stdout, "Destroy vala plugin\n");
	}
	G_OBJECT_CLASS (gmpc_test_plugin_parent_class)->finalize (obj);
}


GType gmpc_test_plugin_get_type (void) {
	static GType gmpc_test_plugin_type_id = 0;
	if (gmpc_test_plugin_type_id == 0) {
		static const GTypeInfo g_define_type_info = { sizeof (GmpcTestPluginClass), (GBaseInitFunc) NULL, (GBaseFinalizeFunc) NULL, (GClassInitFunc) gmpc_test_plugin_class_init, (GClassFinalizeFunc) NULL, NULL, sizeof (GmpcTestPlugin), 0, (GInstanceInitFunc) gmpc_test_plugin_instance_init, NULL };
		gmpc_test_plugin_type_id = g_type_register_static (GMPC_TYPE_PLUGIN_BASE, "GmpcTestPlugin", &g_define_type_info, 0);
	}
	return gmpc_test_plugin_type_id;
}




