
#ifndef __GMPC_TEST_PLUGIN_H__
#define __GMPC_TEST_PLUGIN_H__

#include <glib.h>
#include <glib-object.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include "gmpc-plugin2.h"

G_BEGIN_DECLS


#define GMPC_TYPE_TEST_PLUGIN (gmpc_test_plugin_get_type ())
#define GMPC_TEST_PLUGIN(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GMPC_TYPE_TEST_PLUGIN, GmpcTestPlugin))
#define GMPC_TEST_PLUGIN_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GMPC_TYPE_TEST_PLUGIN, GmpcTestPluginClass))
#define GMPC_IS_TEST_PLUGIN(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GMPC_TYPE_TEST_PLUGIN))
#define GMPC_IS_TEST_PLUGIN_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GMPC_TYPE_TEST_PLUGIN))
#define GMPC_TEST_PLUGIN_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GMPC_TYPE_TEST_PLUGIN, GmpcTestPluginClass))

typedef struct _GmpcTestPlugin GmpcTestPlugin;
typedef struct _GmpcTestPluginClass GmpcTestPluginClass;
typedef struct _GmpcTestPluginPrivate GmpcTestPluginPrivate;

struct _GmpcTestPlugin {
	GmpcPluginBase parent_instance;
	GmpcTestPluginPrivate * priv;
};

struct _GmpcTestPluginClass {
	GmpcPluginBaseClass parent_class;
};


GmpcTestPlugin* gmpc_test_plugin_construct (GType object_type);
GmpcTestPlugin* gmpc_test_plugin_new (void);
GType gmpc_test_plugin_get_type (void);

static const gint GMPC_TEST_PLUGIN_version[] = {0, 0, 2};

G_END_DECLS

#endif
