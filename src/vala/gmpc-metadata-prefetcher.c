
#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include <gtktransition.h>
#include <gmpc-plugin.h>
#include <stdlib.h>
#include <string.h>
#include <libmpd/libmpd.h>
#include <libmpd/libmpdclient.h>
#include <metadata.h>
#include <main.h>
#include <gmpc-meta-watcher.h>
#include <gmpc-connection.h>


#define GMPC_PLUGIN_TYPE_METADATA_PREFETCHER (gmpc_plugin_metadata_prefetcher_get_type ())
#define GMPC_PLUGIN_METADATA_PREFETCHER(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GMPC_PLUGIN_TYPE_METADATA_PREFETCHER, GmpcPluginMetadataPrefetcher))
#define GMPC_PLUGIN_METADATA_PREFETCHER_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GMPC_PLUGIN_TYPE_METADATA_PREFETCHER, GmpcPluginMetadataPrefetcherClass))
#define GMPC_PLUGIN_IS_METADATA_PREFETCHER(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GMPC_PLUGIN_TYPE_METADATA_PREFETCHER))
#define GMPC_PLUGIN_IS_METADATA_PREFETCHER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GMPC_PLUGIN_TYPE_METADATA_PREFETCHER))
#define GMPC_PLUGIN_METADATA_PREFETCHER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GMPC_PLUGIN_TYPE_METADATA_PREFETCHER, GmpcPluginMetadataPrefetcherClass))

typedef struct _GmpcPluginMetadataPrefetcher GmpcPluginMetadataPrefetcher;
typedef struct _GmpcPluginMetadataPrefetcherClass GmpcPluginMetadataPrefetcherClass;
typedef struct _GmpcPluginMetadataPrefetcherPrivate GmpcPluginMetadataPrefetcherPrivate;

struct _GmpcPluginMetadataPrefetcher {
	GmpcPluginBase parent_instance;
	GmpcPluginMetadataPrefetcherPrivate * priv;
	gint* version;
	gint version_length1;
};

struct _GmpcPluginMetadataPrefetcherClass {
	GmpcPluginBaseClass parent_class;
};


static gpointer gmpc_plugin_metadata_prefetcher_parent_class = NULL;

#define use_transition TRUE
GType gmpc_plugin_metadata_prefetcher_get_type (void);
enum  {
	GMPC_PLUGIN_METADATA_PREFETCHER_DUMMY_PROPERTY
};
static gint* gmpc_plugin_metadata_prefetcher_real_get_version (GmpcPluginBase* base, int* result_length1);
static const char* gmpc_plugin_metadata_prefetcher_real_get_name (GmpcPluginBase* base);
static void gmpc_plugin_metadata_prefetcher_status_changed (GmpcPluginMetadataPrefetcher* self, GmpcConnection* gmpcconn, MpdObj* server, ChangedStatusType what);
GmpcPluginMetadataPrefetcher* gmpc_plugin_metadata_prefetcher_new (void);
GmpcPluginMetadataPrefetcher* gmpc_plugin_metadata_prefetcher_construct (GType object_type);
static void _gmpc_plugin_metadata_prefetcher_status_changed_gmpc_connection_status_changed (GmpcConnection* _sender, MpdObj* server, ChangedStatusType what, gpointer self);
static GObject * gmpc_plugin_metadata_prefetcher_constructor (GType type, guint n_construct_properties, GObjectConstructParam * construct_properties);
static void gmpc_plugin_metadata_prefetcher_finalize (GObject* obj);



static gint* gmpc_plugin_metadata_prefetcher_real_get_version (GmpcPluginBase* base, int* result_length1) {
	GmpcPluginMetadataPrefetcher * self;
	gint* result;
	gint* _tmp0_;
	self = (GmpcPluginMetadataPrefetcher*) base;
	_tmp0_ = NULL;
	result = (_tmp0_ = self->version, *result_length1 = self->version_length1, _tmp0_);
	return result;
}


static const char* gmpc_plugin_metadata_prefetcher_real_get_name (GmpcPluginBase* base) {
	GmpcPluginMetadataPrefetcher * self;
	const char* result;
	self = (GmpcPluginMetadataPrefetcher*) base;
	result = "Metadata pre-fetcher";
	return result;
}


static void gmpc_plugin_metadata_prefetcher_status_changed (GmpcPluginMetadataPrefetcher* self, GmpcConnection* gmpcconn, MpdObj* server, ChangedStatusType what) {
	g_return_if_fail (self != NULL);
	g_return_if_fail (gmpcconn != NULL);
	g_return_if_fail (server != NULL);
	if (!gmpc_plugin_base_get_enabled ((GmpcPluginBase*) self)) {
		return;
	}
	if ((what & MPD_CST_NEXTSONG) == MPD_CST_NEXTSONG) {
		gint next_song_id;
		next_song_id = mpd_player_get_next_song_id (server);
		if (next_song_id > 0) {
			mpd_Song* song;
			song = mpd_playlist_get_song (server, next_song_id);
			if (song != NULL) {
				MetaData* met;
				MetaDataResult md_result;
				MetaData* _tmp2_;
				MetaDataResult _tmp1_;
				MetaData* _tmp0_;
				MetaData* _tmp5_;
				MetaDataResult _tmp4_;
				MetaData* _tmp3_;
				met = NULL;
				md_result = 0;
				g_log ("MetadataPrefetcher", G_LOG_LEVEL_DEBUG, "gmpc-metadata-prefetcher.vala:58: Pre-fetching %s", song->file);
				/* Query artist */
				_tmp2_ = NULL;
				_tmp0_ = NULL;
				md_result = (_tmp1_ = gmpc_meta_watcher_get_meta_path (gmw, song, META_ARTIST_ART, &_tmp0_), met = (_tmp2_ = _tmp0_, (met == NULL) ? NULL : (met = (meta_data_free (met), NULL)), _tmp2_), _tmp1_);
				/* Query album art */
				_tmp5_ = NULL;
				_tmp3_ = NULL;
				md_result = (_tmp4_ = gmpc_meta_watcher_get_meta_path (gmw, song, META_ALBUM_ART, &_tmp3_), met = (_tmp5_ = _tmp3_, (met == NULL) ? NULL : (met = (meta_data_free (met), NULL)), _tmp5_), _tmp4_);
				(met == NULL) ? NULL : (met = (meta_data_free (met), NULL));
			}
			(song == NULL) ? NULL : (song = (mpd_freeSong (song), NULL));
		}
	}
}


GmpcPluginMetadataPrefetcher* gmpc_plugin_metadata_prefetcher_construct (GType object_type) {
	GmpcPluginMetadataPrefetcher * self;
	self = g_object_newv (object_type, 0, NULL);
	return self;
}


GmpcPluginMetadataPrefetcher* gmpc_plugin_metadata_prefetcher_new (void) {
	return gmpc_plugin_metadata_prefetcher_construct (GMPC_PLUGIN_TYPE_METADATA_PREFETCHER);
}


static void _gmpc_plugin_metadata_prefetcher_status_changed_gmpc_connection_status_changed (GmpcConnection* _sender, MpdObj* server, ChangedStatusType what, gpointer self) {
	gmpc_plugin_metadata_prefetcher_status_changed (self, _sender, server, what);
}


static GObject * gmpc_plugin_metadata_prefetcher_constructor (GType type, guint n_construct_properties, GObjectConstructParam * construct_properties) {
	GObject * obj;
	GmpcPluginMetadataPrefetcherClass * klass;
	GObjectClass * parent_class;
	GmpcPluginMetadataPrefetcher * self;
	klass = GMPC_PLUGIN_METADATA_PREFETCHER_CLASS (g_type_class_peek (GMPC_PLUGIN_TYPE_METADATA_PREFETCHER));
	parent_class = G_OBJECT_CLASS (g_type_class_peek_parent (klass));
	obj = parent_class->constructor (type, n_construct_properties, construct_properties);
	self = GMPC_PLUGIN_METADATA_PREFETCHER (obj);
	{
		/* Mark the plugin as an internal dummy */
		((GmpcPluginBase*) self)->plugin_type = 8 + 4;
		/* Attach status changed signal */
		g_signal_connect_object (gmpcconn, "status-changed", (GCallback) _gmpc_plugin_metadata_prefetcher_status_changed_gmpc_connection_status_changed, self, 0);
	}
	return obj;
}


static void gmpc_plugin_metadata_prefetcher_class_init (GmpcPluginMetadataPrefetcherClass * klass) {
	gmpc_plugin_metadata_prefetcher_parent_class = g_type_class_peek_parent (klass);
	GMPC_PLUGIN_BASE_CLASS (klass)->get_version = gmpc_plugin_metadata_prefetcher_real_get_version;
	GMPC_PLUGIN_BASE_CLASS (klass)->get_name = gmpc_plugin_metadata_prefetcher_real_get_name;
	G_OBJECT_CLASS (klass)->constructor = gmpc_plugin_metadata_prefetcher_constructor;
	G_OBJECT_CLASS (klass)->finalize = gmpc_plugin_metadata_prefetcher_finalize;
}


static void gmpc_plugin_metadata_prefetcher_instance_init (GmpcPluginMetadataPrefetcher * self) {
	gint* _tmp0_;
	self->version = (_tmp0_ = g_new0 (gint, 3), _tmp0_[0] = 0, _tmp0_[1] = 0, _tmp0_[2] = 2, _tmp0_);
	self->version_length1 = 3;
	_tmp0_ = NULL;
}


static void gmpc_plugin_metadata_prefetcher_finalize (GObject* obj) {
	GmpcPluginMetadataPrefetcher * self;
	self = GMPC_PLUGIN_METADATA_PREFETCHER (obj);
	self->version = (g_free (self->version), NULL);
	G_OBJECT_CLASS (gmpc_plugin_metadata_prefetcher_parent_class)->finalize (obj);
}


GType gmpc_plugin_metadata_prefetcher_get_type (void) {
	static GType gmpc_plugin_metadata_prefetcher_type_id = 0;
	if (gmpc_plugin_metadata_prefetcher_type_id == 0) {
		static const GTypeInfo g_define_type_info = { sizeof (GmpcPluginMetadataPrefetcherClass), (GBaseInitFunc) NULL, (GBaseFinalizeFunc) NULL, (GClassInitFunc) gmpc_plugin_metadata_prefetcher_class_init, (GClassFinalizeFunc) NULL, NULL, sizeof (GmpcPluginMetadataPrefetcher), 0, (GInstanceInitFunc) gmpc_plugin_metadata_prefetcher_instance_init, NULL };
		gmpc_plugin_metadata_prefetcher_type_id = g_type_register_static (GMPC_PLUGIN_TYPE_BASE, "GmpcPluginMetadataPrefetcher", &g_define_type_info, 0);
	}
	return gmpc_plugin_metadata_prefetcher_type_id;
}




