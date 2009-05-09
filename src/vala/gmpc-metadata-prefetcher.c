
#include "gmpc-metadata-prefetcher.h"
#include <gtktransition.h>
#include <gmpc-connection.h>
#include <libmpd/libmpd.h>
#include <libmpd/libmpdclient.h>
#include <metadata.h>
#include <main.h>
#include <gmpc-meta-watcher.h>




enum  {
	GMPC_PLUGIN_METADATA_PREFETCHER_DUMMY_PROPERTY
};
static gint* gmpc_plugin_metadata_prefetcher_real_get_version (GmpcPluginBase* base, int* result_length1);
static const char* gmpc_plugin_metadata_prefetcher_real_get_name (GmpcPluginBase* base);
static void gmpc_plugin_metadata_prefetcher_status_changed (GmpcPluginMetadataPrefetcher* self, GmpcConnection* gmpcconn, MpdObj* server, ChangedStatusType what);
static void _gmpc_plugin_metadata_prefetcher_status_changed_gmpc_connection_status_changed (GmpcConnection* _sender, MpdObj* server, ChangedStatusType what, gpointer self);
static GObject * gmpc_plugin_metadata_prefetcher_constructor (GType type, guint n_construct_properties, GObjectConstructParam * construct_properties);
static gpointer gmpc_plugin_metadata_prefetcher_parent_class = NULL;



static gint* gmpc_plugin_metadata_prefetcher_real_get_version (GmpcPluginBase* base, int* result_length1) {
	GmpcPluginMetadataPrefetcher * self;
	gint* _tmp0;
	self = (GmpcPluginMetadataPrefetcher*) base;
	_tmp0 = NULL;
	return (_tmp0 = (gint*) GMPC_PLUGIN_METADATA_PREFETCHER_version, *result_length1 = -1, _tmp0);
}


static const char* gmpc_plugin_metadata_prefetcher_real_get_name (GmpcPluginBase* base) {
	GmpcPluginMetadataPrefetcher * self;
	self = (GmpcPluginMetadataPrefetcher*) base;
	return "Metadata pre-fetcher";
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
				MetaDataResult result;
				MetaData* _tmp2;
				MetaDataResult _tmp1;
				MetaData* _tmp0;
				MetaData* _tmp5;
				MetaDataResult _tmp4;
				MetaData* _tmp3;
				met = NULL;
				result = 0;
				g_log ("MetadataPrefetcher", G_LOG_LEVEL_DEBUG, "gmpc-metadata-prefetcher.vala:60: Pre-fetching %s", song->file);
				/* Query artist */
				_tmp2 = NULL;
				_tmp0 = NULL;
				result = (_tmp1 = gmpc_meta_watcher_get_meta_path (gmw, song, META_ARTIST_ART, &_tmp0), met = (_tmp2 = _tmp0, (met == NULL) ? NULL : (met = (meta_data_free (met), NULL)), _tmp2), _tmp1);
				/* Query album art */
				_tmp5 = NULL;
				_tmp3 = NULL;
				result = (_tmp4 = gmpc_meta_watcher_get_meta_path (gmw, song, META_ALBUM_ART, &_tmp3), met = (_tmp5 = _tmp3, (met == NULL) ? NULL : (met = (meta_data_free (met), NULL)), _tmp5), _tmp4);
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
		g_signal_connect_object (gmpcconn, "status-changed", (GCallback) _gmpc_plugin_metadata_prefetcher_status_changed_gmpc_connection_status_changed, self, 0);
	}
	return obj;
}


static void gmpc_plugin_metadata_prefetcher_class_init (GmpcPluginMetadataPrefetcherClass * klass) {
	gmpc_plugin_metadata_prefetcher_parent_class = g_type_class_peek_parent (klass);
	G_OBJECT_CLASS (klass)->constructor = gmpc_plugin_metadata_prefetcher_constructor;
	GMPC_PLUGIN_BASE_CLASS (klass)->get_version = gmpc_plugin_metadata_prefetcher_real_get_version;
	GMPC_PLUGIN_BASE_CLASS (klass)->get_name = gmpc_plugin_metadata_prefetcher_real_get_name;
}


static void gmpc_plugin_metadata_prefetcher_instance_init (GmpcPluginMetadataPrefetcher * self) {
}


GType gmpc_plugin_metadata_prefetcher_get_type (void) {
	static GType gmpc_plugin_metadata_prefetcher_type_id = 0;
	if (gmpc_plugin_metadata_prefetcher_type_id == 0) {
		static const GTypeInfo g_define_type_info = { sizeof (GmpcPluginMetadataPrefetcherClass), (GBaseInitFunc) NULL, (GBaseFinalizeFunc) NULL, (GClassInitFunc) gmpc_plugin_metadata_prefetcher_class_init, (GClassFinalizeFunc) NULL, NULL, sizeof (GmpcPluginMetadataPrefetcher), 0, (GInstanceInitFunc) gmpc_plugin_metadata_prefetcher_instance_init, NULL };
		gmpc_plugin_metadata_prefetcher_type_id = g_type_register_static (GMPC_PLUGIN_TYPE_BASE, "GmpcPluginMetadataPrefetcher", &g_define_type_info, 0);
	}
	return gmpc_plugin_metadata_prefetcher_type_id;
}




