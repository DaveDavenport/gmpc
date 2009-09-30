
#ifndef __GMPC_DATABASE_UPDATE_TRACKER_H__
#define __GMPC_DATABASE_UPDATE_TRACKER_H__

#include <glib.h>
#include <gmpc-plugin.h>

G_BEGIN_DECLS


#define GMPC_PLUGIN_TYPE_DATABASE_UPDATE_TRACKER (gmpc_plugin_database_update_tracker_get_type ())
#define GMPC_PLUGIN_DATABASE_UPDATE_TRACKER(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GMPC_PLUGIN_TYPE_DATABASE_UPDATE_TRACKER, GmpcPluginDatabaseUpdateTracker))
#define GMPC_PLUGIN_DATABASE_UPDATE_TRACKER_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GMPC_PLUGIN_TYPE_DATABASE_UPDATE_TRACKER, GmpcPluginDatabaseUpdateTrackerClass))
#define GMPC_PLUGIN_IS_DATABASE_UPDATE_TRACKER(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GMPC_PLUGIN_TYPE_DATABASE_UPDATE_TRACKER))
#define GMPC_PLUGIN_IS_DATABASE_UPDATE_TRACKER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GMPC_PLUGIN_TYPE_DATABASE_UPDATE_TRACKER))
#define GMPC_PLUGIN_DATABASE_UPDATE_TRACKER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GMPC_PLUGIN_TYPE_DATABASE_UPDATE_TRACKER, GmpcPluginDatabaseUpdateTrackerClass))

typedef struct _GmpcPluginDatabaseUpdateTracker GmpcPluginDatabaseUpdateTracker;
typedef struct _GmpcPluginDatabaseUpdateTrackerClass GmpcPluginDatabaseUpdateTrackerClass;
typedef struct _GmpcPluginDatabaseUpdateTrackerPrivate GmpcPluginDatabaseUpdateTrackerPrivate;

struct _GmpcPluginDatabaseUpdateTracker {
	GmpcPluginBase parent_instance;
	GmpcPluginDatabaseUpdateTrackerPrivate * priv;
	gint* version;
	gint version_length1;
};

struct _GmpcPluginDatabaseUpdateTrackerClass {
	GmpcPluginBaseClass parent_class;
};


GType gmpc_plugin_database_update_tracker_get_type (void);
GmpcPluginDatabaseUpdateTracker* gmpc_plugin_database_update_tracker_new (void);
GmpcPluginDatabaseUpdateTracker* gmpc_plugin_database_update_tracker_construct (GType object_type);


G_END_DECLS

#endif
