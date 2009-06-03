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

#include "gmpc-metadata-browser2.h"
#include <gtktransition.h>
#include <config.h>
#include <metadata.h>
#include <libmpd/libmpd.h>
#include <glib/gi18n-lib.h>
#include <main.h>
#include <gdk/gdk.h>
#include <gmpc-mpddata-model.h>
#include <float.h>
#include <math.h>
#include <gmpc-meta-watcher.h>
#include <plugin.h>
#include <config1.h>
#include <misc.h>
#include <gmpc-metaimage.h>
#include <gmpc-meta-text-view.h>
#include <gmpc-stats-label.h>
#include <gmpc-connection.h>
#include <pango/pango.h>
#include <gdk-pixbuf/gdk-pixdata.h>
#include "gmpc-favorites.h"
#include "gmpc-rating.h"
#include "gmpc-song-links.h"




static glong string_get_length (const char* self);
struct _GmpcWidgetSimilarSongsPrivate {
	mpd_Song* song;
	gboolean filled;
	GtkWidget* pchild;
	guint idle_add;
	MetaData* copy;
	MpdData* item;
	GList* current;
};

#define GMPC_WIDGET_SIMILAR_SONGS_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GMPC_WIDGET_TYPE_SIMILAR_SONGS, GmpcWidgetSimilarSongsPrivate))
enum  {
	GMPC_WIDGET_SIMILAR_SONGS_DUMMY_PROPERTY
};
static GmpcWidgetSimilarSongs* gmpc_widget_similar_songs_construct (GType object_type, const mpd_Song* song);
static GmpcWidgetSimilarSongs* gmpc_widget_similar_songs_new (const mpd_Song* song);
static void _g_list_free_gtk_tree_path_free (GList* self);
static void gmpc_widget_similar_songs_add_clicked (GmpcWidgetSimilarSongs* self, GtkImageMenuItem* item);
static void gmpc_widget_similar_songs_play_clicked (GmpcWidgetSimilarSongs* self, GtkImageMenuItem* item);
static void gmpc_widget_similar_songs_replace_clicked (GmpcWidgetSimilarSongs* self, GtkImageMenuItem* item);
static void gmpc_widget_similar_songs_tree_row_activated (GmpcWidgetSimilarSongs* self, GmpcMpdDataTreeview* tree, const GtkTreePath* path, GtkTreeViewColumn* column);
static void _gmpc_widget_similar_songs_play_clicked_gtk_menu_item_activate (GtkImageMenuItem* _sender, gpointer self);
static void _gmpc_widget_similar_songs_add_clicked_gtk_menu_item_activate (GtkImageMenuItem* _sender, gpointer self);
static void _gmpc_widget_similar_songs_replace_clicked_gtk_menu_item_activate (GtkImageMenuItem* _sender, gpointer self);
static gboolean gmpc_widget_similar_songs_tree_right_menu (GmpcWidgetSimilarSongs* self, GmpcMpdDataTreeview* tree, const GdkEventButton* event);
static gboolean _gmpc_widget_similar_songs_tree_right_menu_gtk_widget_button_release_event (GmpcMpdDataTreeview* _sender, const GdkEventButton* event, gpointer self);
static void _gmpc_widget_similar_songs_tree_row_activated_gtk_tree_view_row_activated (GmpcMpdDataTreeview* _sender, const GtkTreePath* path, GtkTreeViewColumn* column, gpointer self);
static gboolean gmpc_widget_similar_songs_update_sim_song (GmpcWidgetSimilarSongs* self);
static gboolean _gmpc_widget_similar_songs_update_sim_song_gsource_func (gpointer self);
static void gmpc_widget_similar_songs_metadata_changed (GmpcWidgetSimilarSongs* self, GmpcMetaWatcher* gmw, const mpd_Song* song, MetaDataType type, MetaDataResult result, const MetaData* met);
static void _gmpc_widget_similar_songs_metadata_changed_gmpc_meta_watcher_data_changed (GmpcMetaWatcher* _sender, const mpd_Song* song, MetaDataType type, MetaDataResult result, const MetaData* met, gpointer self);
static void gmpc_widget_similar_songs_update (GmpcWidgetSimilarSongs* self);
static void gmpc_widget_similar_songs_real_activate (GtkExpander* base);
static gpointer gmpc_widget_similar_songs_parent_class = NULL;
static void gmpc_widget_similar_songs_finalize (GObject* obj);
struct _GmpcWidgetSimilarArtistPrivate {
	mpd_Song* song;
	GmpcMetadataBrowser* browser;
};

#define GMPC_WIDGET_SIMILAR_ARTIST_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GMPC_WIDGET_TYPE_SIMILAR_ARTIST, GmpcWidgetSimilarArtistPrivate))
enum  {
	GMPC_WIDGET_SIMILAR_ARTIST_DUMMY_PROPERTY
};
static void _g_list_free_g_object_unref (GList* self);
static void gmpc_widget_similar_artist_metadata_changed (GmpcWidgetSimilarArtist* self, GmpcMetaWatcher* gmw, const mpd_Song* song, MetaDataType type, MetaDataResult result, const MetaData* met);
static void gmpc_widget_similar_artist_artist_button_clicked (GmpcWidgetSimilarArtist* self, GtkButton* button);
static gboolean _misc_header_expose_event_gtk_widget_expose_event (GtkEventBox* _sender, const GdkEventExpose* event, gpointer self);
static void _gmpc_widget_similar_artist_artist_button_clicked_gtk_button_clicked (GtkButton* _sender, gpointer self);
static void _gmpc_widget_similar_artist_metadata_changed_gmpc_meta_watcher_data_changed (GmpcMetaWatcher* _sender, const mpd_Song* song, MetaDataType type, MetaDataResult result, const MetaData* met, gpointer self);
static GmpcWidgetSimilarArtist* gmpc_widget_similar_artist_construct (GType object_type, GmpcMetadataBrowser* browser, MpdObj* server, const mpd_Song* song);
static GmpcWidgetSimilarArtist* gmpc_widget_similar_artist_new (GmpcMetadataBrowser* browser, MpdObj* server, const mpd_Song* song);
static gpointer gmpc_widget_similar_artist_parent_class = NULL;
static void gmpc_widget_similar_artist_finalize (GObject* obj);
struct _GmpcWidgetMorePrivate {
	GtkAlignment* ali;
	gint expand_state;
	GtkButton* expand_button;
	gint max_height;
	GtkEventBox* eventbox;
	GtkWidget* pchild;
};

#define GMPC_WIDGET_MORE_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GMPC_WIDGET_TYPE_MORE, GmpcWidgetMorePrivate))
enum  {
	GMPC_WIDGET_MORE_DUMMY_PROPERTY
};
static void gmpc_widget_more_expand (GmpcWidgetMore* self, GtkButton* but);
static void gmpc_widget_more_size_changed (GmpcWidgetMore* self, GtkWidget* child, const GdkRectangle* alloc);
static void gmpc_widget_more_bg_style_changed (GmpcWidgetMore* self, GtkWidget* frame, GtkStyle* style);
static void _gmpc_widget_more_bg_style_changed_gtk_widget_style_set (GmpcWidgetMore* _sender, GtkStyle* previous_style, gpointer self);
static void _gmpc_widget_more_expand_gtk_button_clicked (GtkButton* _sender, gpointer self);
static void _gmpc_widget_more_size_changed_gtk_widget_size_allocate (GtkWidget* _sender, const GdkRectangle* allocation, gpointer self);
static GmpcWidgetMore* gmpc_widget_more_construct (GType object_type, const char* markup, GtkWidget* child);
static GmpcWidgetMore* gmpc_widget_more_new (const char* markup, GtkWidget* child);
static gpointer gmpc_widget_more_parent_class = NULL;
static void gmpc_widget_more_finalize (GObject* obj);
struct _GmpcNowPlayingPrivate {
	GtkTreeRowReference* np_ref;
	GmpcMetadataBrowser* browser;
	GtkScrolledWindow* paned;
	GtkEventBox* container;
	gboolean selected;
	char* song_checksum;
};

#define GMPC_NOW_PLAYING_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GMPC_TYPE_NOW_PLAYING, GmpcNowPlayingPrivate))
enum  {
	GMPC_NOW_PLAYING_DUMMY_PROPERTY
};
static gint* gmpc_now_playing_real_get_version (GmpcPluginBase* base, int* result_length1);
static const char* gmpc_now_playing_real_get_name (GmpcPluginBase* base);
static void gmpc_now_playing_real_save_yourself (GmpcPluginBase* base);
static void gmpc_now_playing_status_changed (GmpcNowPlaying* self, GmpcConnection* conn, MpdObj* server, ChangedStatusType what);
static void gmpc_now_playing_real_browser_add (GmpcPluginBrowserIface* base, GtkWidget* category_tree);
static void gmpc_now_playing_real_browser_selected (GmpcPluginBrowserIface* base, GtkContainer* container);
static void gmpc_now_playing_real_browser_unselected (GmpcPluginBrowserIface* base, GtkContainer* container);
static void gmpc_now_playing_browser_bg_style_changed (GmpcNowPlaying* self, GtkScrolledWindow* bg, GtkStyle* style);
static void _gmpc_now_playing_browser_bg_style_changed_gtk_widget_style_set (GtkScrolledWindow* _sender, GtkStyle* previous_style, gpointer self);
static void gmpc_now_playing_browser_init (GmpcNowPlaying* self);
static void gmpc_now_playing_update (GmpcNowPlaying* self);
static void gmpc_now_playing_select_now_playing_browser (GmpcNowPlaying* self, GtkImageMenuItem* item);
static void _gmpc_now_playing_select_now_playing_browser_gtk_menu_item_activate (GtkImageMenuItem* _sender, gpointer self);
static gint gmpc_now_playing_real_browser_add_go_menu (GmpcPluginBrowserIface* base, GtkMenu* menu);
static void _gmpc_now_playing_status_changed_gmpc_connection_status_changed (GmpcConnection* _sender, MpdObj* server, ChangedStatusType what, gpointer self);
static GObject * gmpc_now_playing_constructor (GType type, guint n_construct_properties, GObjectConstructParam * construct_properties);
static gpointer gmpc_now_playing_parent_class = NULL;
static GmpcPluginBrowserIfaceIface* gmpc_now_playing_gmpc_plugin_browser_iface_parent_iface = NULL;
static void gmpc_now_playing_finalize (GObject* obj);
struct _GmpcMetadataBrowserPrivate {
	gint block_update;
	GtkTreeRowReference* rref;
	GtkPaned* paned;
	GtkBox* browser_box;
	GtkTreeView* tree_artist;
	GmpcMpdDataModel* model_artist;
	GtkTreeModelFilter* model_filter_artist;
	GtkEntry* artist_filter_entry;
	GtkTreeView* tree_album;
	GmpcMpdDataModel* model_albums;
	GtkTreeModelFilter* model_filter_album;
	GtkEntry* album_filter_entry;
	GtkTreeView* tree_songs;
	GmpcMpdDataModel* model_songs;
	GtkScrolledWindow* metadata_sw;
	GtkEventBox* metadata_box;
	guint update_timeout;
	gboolean selected;
};

#define GMPC_METADATA_BROWSER_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GMPC_TYPE_METADATA_BROWSER, GmpcMetadataBrowserPrivate))
enum  {
	GMPC_METADATA_BROWSER_DUMMY_PROPERTY
};
static gint* gmpc_metadata_browser_real_get_version (GmpcPluginBase* base, int* result_length1);
static const char* gmpc_metadata_browser_real_get_name (GmpcPluginBase* base);
static void gmpc_metadata_browser_real_save_yourself (GmpcPluginBase* base);
static void gmpc_metadata_browser_browser_bg_style_changed (GmpcMetadataBrowser* self, GtkScrolledWindow* bg, GtkStyle* style);
static gboolean gmpc_metadata_browser_browser_button_press_event (GmpcMetadataBrowser* self, GtkTreeView* tree, const GdkEventButton* event);
static gboolean gmpc_metadata_browser_visible_func_artist (GmpcMetadataBrowser* self, GtkTreeModel* model, GtkTreeIter* iter);
static gboolean gmpc_metadata_browser_visible_func_album (GmpcMetadataBrowser* self, GtkTreeModel* model, GtkTreeIter* iter);
static gboolean gmpc_metadata_browser_browser_artist_key_press_event (GmpcMetadataBrowser* self, GtkTreeView* widget, const GdkEventKey* event);
static gboolean gmpc_metadata_browser_browser_album_key_press_event (GmpcMetadataBrowser* self, GtkTreeView* widget, const GdkEventKey* event);
static void gmpc_metadata_browser_browser_artist_entry_changed (GmpcMetadataBrowser* self, GtkEntry* entry);
static void gmpc_metadata_browser_browser_album_entry_changed (GmpcMetadataBrowser* self, GtkEntry* entry);
static void _gmpc_metadata_browser_browser_artist_entry_changed_gtk_editable_changed (GtkEntry* _sender, gpointer self);
static gboolean _gmpc_metadata_browser_visible_func_artist_gtk_tree_model_filter_visible_func (GtkTreeModel* model, GtkTreeIter* iter, gpointer self);
static gboolean _gmpc_metadata_browser_browser_button_press_event_gtk_widget_button_press_event (GtkTreeView* _sender, const GdkEventButton* event, gpointer self);
static gboolean _gmpc_metadata_browser_browser_artist_key_press_event_gtk_widget_key_press_event (GtkTreeView* _sender, const GdkEventKey* event, gpointer self);
static void _gmpc_metadata_browser_browser_artist_changed_gtk_tree_selection_changed (GtkTreeSelection* _sender, gpointer self);
static void _gmpc_metadata_browser_browser_album_entry_changed_gtk_editable_changed (GtkEntry* _sender, gpointer self);
static gboolean _gmpc_metadata_browser_visible_func_album_gtk_tree_model_filter_visible_func (GtkTreeModel* model, GtkTreeIter* iter, gpointer self);
static gboolean _gmpc_metadata_browser_browser_album_key_press_event_gtk_widget_key_press_event (GtkTreeView* _sender, const GdkEventKey* event, gpointer self);
static void _gmpc_metadata_browser_browser_album_changed_gtk_tree_selection_changed (GtkTreeSelection* _sender, gpointer self);
static void _gmpc_metadata_browser_browser_songs_changed_gtk_tree_selection_changed (GtkTreeSelection* _sender, gpointer self);
static void _gmpc_metadata_browser_browser_bg_style_changed_gtk_widget_style_set (GtkScrolledWindow* _sender, GtkStyle* previous_style, gpointer self);
static void gmpc_metadata_browser_browser_init (GmpcMetadataBrowser* self);
static void gmpc_metadata_browser_reload_browsers (GmpcMetadataBrowser* self);
static char* gmpc_metadata_browser_browser_get_selected_artist (GmpcMetadataBrowser* self);
static char* gmpc_metadata_browser_browser_get_selected_album (GmpcMetadataBrowser* self);
static mpd_Song* gmpc_metadata_browser_browser_get_selected_song (GmpcMetadataBrowser* self);
static void gmpc_metadata_browser_browser_artist_changed (GmpcMetadataBrowser* self, GtkTreeSelection* sel);
static void gmpc_metadata_browser_browser_album_changed (GmpcMetadataBrowser* self, GtkTreeSelection* album_sel);
static void gmpc_metadata_browser_browser_songs_changed (GmpcMetadataBrowser* self, GtkTreeSelection* song_sel);
static void gmpc_metadata_browser_play_song (GmpcMetadataBrowser* self, GtkButton* button);
static void gmpc_metadata_browser_add_song (GmpcMetadataBrowser* self, GtkButton* button);
static void gmpc_metadata_browser_replace_song (GmpcMetadataBrowser* self, GtkButton* button);
static void gmpc_metadata_browser_add_selected_song (GmpcMetadataBrowser* self, GtkButton* button);
static void gmpc_metadata_browser_replace_selected_song (GmpcMetadataBrowser* self, GtkButton* button);
static void gmpc_metadata_browser_metadata_box_clear (GmpcMetadataBrowser* self);
static void _gmpc_metadata_browser_metadata_find_query_gtk_button_clicked (GtkButton* _sender, gpointer self);
static void _gmpc_metadata_browser_metadata_button_open_file_browser_path_gtk_button_clicked (GtkButton* _sender, gpointer self);
static void _gmpc_metadata_browser_play_song_gtk_button_clicked (GtkButton* _sender, gpointer self);
static void _gmpc_metadata_browser_add_song_gtk_button_clicked (GtkButton* _sender, gpointer self);
static void _gmpc_metadata_browser_replace_song_gtk_button_clicked (GtkButton* _sender, gpointer self);
static void gmpc_metadata_browser_metadata_button_open_file_browser_path (GmpcMetadataBrowser* self, GtkButton* button);
static void gmpc_metadata_browser_metadata_find_query (GmpcMetadataBrowser* self, GtkButton* button);
static void _gmpc_metadata_browser_add_selected_song_gtk_button_clicked (GtkButton* _sender, gpointer self);
static void _gmpc_metadata_browser_replace_selected_song_gtk_button_clicked (GtkButton* _sender, gpointer self);
static void gmpc_metadata_browser_metadata_box_show_album (GmpcMetadataBrowser* self, const char* artist, const char* album);
static void gmpc_metadata_browser_metadata_box_show_artist (GmpcMetadataBrowser* self, const char* artist);
static gboolean _gmpc_metadata_browser_metadata_box_update_real_gsource_func (gpointer self);
static void gmpc_metadata_browser_metadata_box_update (GmpcMetadataBrowser* self);
static gboolean gmpc_metadata_browser_metadata_box_update_real (GmpcMetadataBrowser* self);
static void gmpc_metadata_browser_real_browser_add (GmpcPluginBrowserIface* base, GtkWidget* category_tree);
static void gmpc_metadata_browser_real_browser_selected (GmpcPluginBrowserIface* base, GtkContainer* container);
static void gmpc_metadata_browser_real_browser_unselected (GmpcPluginBrowserIface* base, GtkContainer* container);
static void gmpc_metadata_browser_con_changed (GmpcMetadataBrowser* self, GmpcConnection* conn, MpdObj* server, gint connect);
static void gmpc_metadata_browser_status_changed (GmpcMetadataBrowser* self, GmpcConnection* conn, MpdObj* server, ChangedStatusType what);
static void _gmpc_metadata_browser_con_changed_gmpc_connection_connection_changed (GmpcConnection* _sender, MpdObj* server, gint connect, gpointer self);
static void _gmpc_metadata_browser_status_changed_gmpc_connection_status_changed (GmpcConnection* _sender, MpdObj* server, ChangedStatusType what, gpointer self);
static GObject * gmpc_metadata_browser_constructor (GType type, guint n_construct_properties, GObjectConstructParam * construct_properties);
static gpointer gmpc_metadata_browser_parent_class = NULL;
static GmpcPluginBrowserIfaceIface* gmpc_metadata_browser_gmpc_plugin_browser_iface_parent_iface = NULL;
static void gmpc_metadata_browser_finalize (GObject* obj);
static void _vala_array_free (gpointer array, gint array_length, GDestroyNotify destroy_func);
static gint _vala_array_length (gpointer array);
static int _vala_strcmp0 (const char * str1, const char * str2);



static glong string_get_length (const char* self) {
	g_return_val_if_fail (self != NULL, 0L);
	return g_utf8_strlen (self, -1);
}


static GmpcWidgetSimilarSongs* gmpc_widget_similar_songs_construct (GType object_type, const mpd_Song* song) {
	GmpcWidgetSimilarSongs * self;
	mpd_Song* _tmp1;
	const mpd_Song* _tmp0;
	GtkLabel* label;
	char* _tmp2;
	g_return_val_if_fail (song != NULL, NULL);
	self = g_object_newv (object_type, 0, NULL);
	_tmp1 = NULL;
	_tmp0 = NULL;
	self->priv->song = (_tmp1 = (_tmp0 = song, (_tmp0 == NULL) ? NULL : mpd_songDup (_tmp0)), (self->priv->song == NULL) ? NULL : (self->priv->song = (mpd_freeSong (self->priv->song), NULL)), _tmp1);
	label = g_object_ref_sink ((GtkLabel*) gtk_label_new (_ ("Similar songs")));
	_tmp2 = NULL;
	gtk_label_set_markup (label, _tmp2 = g_strdup_printf ("<b>%s</b>", _ ("Similar songs")));
	_tmp2 = (g_free (_tmp2), NULL);
	gtk_expander_set_label_widget ((GtkExpander*) self, (GtkWidget*) label);
	gtk_widget_show ((GtkWidget*) label);
	return self;
}


static GmpcWidgetSimilarSongs* gmpc_widget_similar_songs_new (const mpd_Song* song) {
	return gmpc_widget_similar_songs_construct (GMPC_WIDGET_TYPE_SIMILAR_SONGS, song);
}


static void _g_list_free_gtk_tree_path_free (GList* self) {
	g_list_foreach (self, (GFunc) gtk_tree_path_free, NULL);
	g_list_free (self);
}


static void gmpc_widget_similar_songs_add_clicked (GmpcWidgetSimilarSongs* self, GtkImageMenuItem* item) {
	GtkTreeView* _tmp0;
	GtkTreeView* tree;
	GtkTreeSelection* _tmp1;
	GtkTreeSelection* sel;
	GtkTreeModel* model;
	GtkTreeIter iter = {0};
	GtkTreeModel* _tmp5;
	GtkTreeModel* _tmp4;
	GList* _tmp3;
	GtkTreeModel* _tmp2;
	GList* list;
	g_return_if_fail (self != NULL);
	g_return_if_fail (item != NULL);
	_tmp0 = NULL;
	tree = (_tmp0 = GTK_TREE_VIEW (self->priv->pchild), (_tmp0 == NULL) ? NULL : g_object_ref (_tmp0));
	_tmp1 = NULL;
	sel = (_tmp1 = gtk_tree_view_get_selection (tree), (_tmp1 == NULL) ? NULL : g_object_ref (_tmp1));
	model = NULL;
	_tmp5 = NULL;
	_tmp4 = NULL;
	_tmp3 = NULL;
	_tmp2 = NULL;
	list = (_tmp3 = gtk_tree_selection_get_selected_rows (sel, &_tmp2), model = (_tmp4 = (_tmp5 = _tmp2, (_tmp5 == NULL) ? NULL : g_object_ref (_tmp5)), (model == NULL) ? NULL : (model = (g_object_unref (model), NULL)), _tmp4), _tmp3);
	{
		GList* path_collection;
		GList* path_it;
		path_collection = list;
		for (path_it = path_collection; path_it != NULL; path_it = path_it->next) {
			const GtkTreePath* _tmp6;
			GtkTreePath* path;
			_tmp6 = NULL;
			path = (_tmp6 = (const GtkTreePath*) path_it->data, (_tmp6 == NULL) ? NULL : gtk_tree_path_copy (_tmp6));
			{
				if (gtk_tree_model_get_iter (model, &iter, path)) {
					const mpd_Song* song;
					song = NULL;
					gtk_tree_model_get (model, &iter, 0, &song, -1, -1);
					if (song != NULL) {
						mpd_playlist_queue_add (connection, song->file);
					}
				}
				(path == NULL) ? NULL : (path = (gtk_tree_path_free (path), NULL));
			}
		}
	}
	mpd_playlist_queue_commit (connection);
	(tree == NULL) ? NULL : (tree = (g_object_unref (tree), NULL));
	(sel == NULL) ? NULL : (sel = (g_object_unref (sel), NULL));
	(model == NULL) ? NULL : (model = (g_object_unref (model), NULL));
	(list == NULL) ? NULL : (list = (_g_list_free_gtk_tree_path_free (list), NULL));
}


static void gmpc_widget_similar_songs_play_clicked (GmpcWidgetSimilarSongs* self, GtkImageMenuItem* item) {
	GtkTreeView* _tmp0;
	GtkTreeView* tree;
	GtkTreeSelection* _tmp1;
	GtkTreeSelection* sel;
	GtkTreeModel* model;
	GtkTreeIter iter = {0};
	GtkTreeModel* _tmp5;
	GtkTreeModel* _tmp4;
	GList* _tmp3;
	GtkTreeModel* _tmp2;
	GList* list;
	g_return_if_fail (self != NULL);
	g_return_if_fail (item != NULL);
	_tmp0 = NULL;
	tree = (_tmp0 = GTK_TREE_VIEW (self->priv->pchild), (_tmp0 == NULL) ? NULL : g_object_ref (_tmp0));
	_tmp1 = NULL;
	sel = (_tmp1 = gtk_tree_view_get_selection (tree), (_tmp1 == NULL) ? NULL : g_object_ref (_tmp1));
	model = NULL;
	_tmp5 = NULL;
	_tmp4 = NULL;
	_tmp3 = NULL;
	_tmp2 = NULL;
	list = (_tmp3 = gtk_tree_selection_get_selected_rows (sel, &_tmp2), model = (_tmp4 = (_tmp5 = _tmp2, (_tmp5 == NULL) ? NULL : g_object_ref (_tmp5)), (model == NULL) ? NULL : (model = (g_object_unref (model), NULL)), _tmp4), _tmp3);
	if (list != NULL) {
		const GtkTreePath* _tmp6;
		GtkTreePath* path;
		_tmp6 = NULL;
		path = (_tmp6 = (const GtkTreePath*) list->data, (_tmp6 == NULL) ? NULL : gtk_tree_path_copy (_tmp6));
		if (gtk_tree_model_get_iter (model, &iter, path)) {
			const mpd_Song* song;
			song = NULL;
			gtk_tree_model_get (model, &iter, 0, &song, -1, -1);
			if (song != NULL) {
				play_path (song->file);
			}
		}
		(path == NULL) ? NULL : (path = (gtk_tree_path_free (path), NULL));
	}
	(tree == NULL) ? NULL : (tree = (g_object_unref (tree), NULL));
	(sel == NULL) ? NULL : (sel = (g_object_unref (sel), NULL));
	(model == NULL) ? NULL : (model = (g_object_unref (model), NULL));
	(list == NULL) ? NULL : (list = (_g_list_free_gtk_tree_path_free (list), NULL));
}


static void gmpc_widget_similar_songs_replace_clicked (GmpcWidgetSimilarSongs* self, GtkImageMenuItem* item) {
	gboolean found;
	GtkTreeView* _tmp0;
	GtkTreeView* tree;
	GtkTreeSelection* _tmp1;
	GtkTreeSelection* sel;
	GtkTreeModel* model;
	GtkTreeIter iter = {0};
	GtkTreeModel* _tmp5;
	GtkTreeModel* _tmp4;
	GList* _tmp3;
	GtkTreeModel* _tmp2;
	GList* list;
	g_return_if_fail (self != NULL);
	g_return_if_fail (item != NULL);
	found = FALSE;
	_tmp0 = NULL;
	tree = (_tmp0 = GTK_TREE_VIEW (self->priv->pchild), (_tmp0 == NULL) ? NULL : g_object_ref (_tmp0));
	_tmp1 = NULL;
	sel = (_tmp1 = gtk_tree_view_get_selection (tree), (_tmp1 == NULL) ? NULL : g_object_ref (_tmp1));
	model = NULL;
	_tmp5 = NULL;
	_tmp4 = NULL;
	_tmp3 = NULL;
	_tmp2 = NULL;
	list = (_tmp3 = gtk_tree_selection_get_selected_rows (sel, &_tmp2), model = (_tmp4 = (_tmp5 = _tmp2, (_tmp5 == NULL) ? NULL : g_object_ref (_tmp5)), (model == NULL) ? NULL : (model = (g_object_unref (model), NULL)), _tmp4), _tmp3);
	{
		GList* path_collection;
		GList* path_it;
		path_collection = list;
		for (path_it = path_collection; path_it != NULL; path_it = path_it->next) {
			const GtkTreePath* _tmp6;
			GtkTreePath* path;
			_tmp6 = NULL;
			path = (_tmp6 = (const GtkTreePath*) path_it->data, (_tmp6 == NULL) ? NULL : gtk_tree_path_copy (_tmp6));
			{
				if (gtk_tree_model_get_iter (model, &iter, path)) {
					const mpd_Song* song;
					song = NULL;
					gtk_tree_model_get (model, &iter, 0, &song, -1, -1);
					if (song != NULL) {
						mpd_playlist_queue_add (connection, song->file);
						found = TRUE;
					}
				}
				(path == NULL) ? NULL : (path = (gtk_tree_path_free (path), NULL));
			}
		}
	}
	if (found) {
		mpd_playlist_clear (connection);
		mpd_playlist_queue_commit (connection);
		mpd_player_play (connection);
	}
	gmpc_widget_similar_songs_play_clicked (self, item);
	(tree == NULL) ? NULL : (tree = (g_object_unref (tree), NULL));
	(sel == NULL) ? NULL : (sel = (g_object_unref (sel), NULL));
	(model == NULL) ? NULL : (model = (g_object_unref (model), NULL));
	(list == NULL) ? NULL : (list = (_g_list_free_gtk_tree_path_free (list), NULL));
}


static void gmpc_widget_similar_songs_tree_row_activated (GmpcWidgetSimilarSongs* self, GmpcMpdDataTreeview* tree, const GtkTreePath* path, GtkTreeViewColumn* column) {
	GtkTreeModel* _tmp0;
	GtkTreeModel* model;
	GtkTreeIter iter = {0};
	g_return_if_fail (self != NULL);
	g_return_if_fail (tree != NULL);
	g_return_if_fail (path != NULL);
	g_return_if_fail (column != NULL);
	_tmp0 = NULL;
	model = (_tmp0 = gtk_tree_view_get_model ((GtkTreeView*) tree), (_tmp0 == NULL) ? NULL : g_object_ref (_tmp0));
	if (gtk_tree_model_get_iter (model, &iter, path)) {
		const mpd_Song* song;
		song = NULL;
		gtk_tree_model_get (model, &iter, 0, &song, -1, -1);
		if (song != NULL) {
			play_path (song->file);
		}
	}
	(model == NULL) ? NULL : (model = (g_object_unref (model), NULL));
}


static void _gmpc_widget_similar_songs_play_clicked_gtk_menu_item_activate (GtkImageMenuItem* _sender, gpointer self) {
	gmpc_widget_similar_songs_play_clicked (self, _sender);
}


static void _gmpc_widget_similar_songs_add_clicked_gtk_menu_item_activate (GtkImageMenuItem* _sender, gpointer self) {
	gmpc_widget_similar_songs_add_clicked (self, _sender);
}


static void _gmpc_widget_similar_songs_replace_clicked_gtk_menu_item_activate (GtkImageMenuItem* _sender, gpointer self) {
	gmpc_widget_similar_songs_replace_clicked (self, _sender);
}


static gboolean gmpc_widget_similar_songs_tree_right_menu (GmpcWidgetSimilarSongs* self, GmpcMpdDataTreeview* tree, const GdkEventButton* event) {
	g_return_val_if_fail (self != NULL, FALSE);
	g_return_val_if_fail (tree != NULL, FALSE);
	if ((*event).button == 3) {
		GtkMenu* menu;
		GtkImageMenuItem* item;
		GtkImageMenuItem* _tmp0;
		GtkImageMenuItem* _tmp1;
		GtkImage* _tmp2;
		gboolean _tmp3;
		menu = g_object_ref_sink ((GtkMenu*) gtk_menu_new ());
		item = g_object_ref_sink ((GtkImageMenuItem*) gtk_image_menu_item_new_from_stock ("gtk-media-play", NULL));
		g_signal_connect_object ((GtkMenuItem*) item, "activate", (GCallback) _gmpc_widget_similar_songs_play_clicked_gtk_menu_item_activate, self, 0);
		gtk_menu_shell_append ((GtkMenuShell*) menu, (GtkWidget*) ((GtkMenuItem*) item));
		_tmp0 = NULL;
		item = (_tmp0 = g_object_ref_sink ((GtkImageMenuItem*) gtk_image_menu_item_new_from_stock ("gtk-add", NULL)), (item == NULL) ? NULL : (item = (g_object_unref (item), NULL)), _tmp0);
		g_signal_connect_object ((GtkMenuItem*) item, "activate", (GCallback) _gmpc_widget_similar_songs_add_clicked_gtk_menu_item_activate, self, 0);
		gtk_menu_shell_append ((GtkMenuShell*) menu, (GtkWidget*) ((GtkMenuItem*) item));
		_tmp1 = NULL;
		item = (_tmp1 = g_object_ref_sink ((GtkImageMenuItem*) gtk_image_menu_item_new_with_mnemonic ("_Replace")), (item == NULL) ? NULL : (item = (g_object_unref (item), NULL)), _tmp1);
		_tmp2 = NULL;
		gtk_image_menu_item_set_image (item, (GtkWidget*) (_tmp2 = g_object_ref_sink ((GtkImage*) gtk_image_new_from_stock ("gtk-redo", GTK_ICON_SIZE_MENU))));
		(_tmp2 == NULL) ? NULL : (_tmp2 = (g_object_unref (_tmp2), NULL));
		g_signal_connect_object ((GtkMenuItem*) item, "activate", (GCallback) _gmpc_widget_similar_songs_replace_clicked_gtk_menu_item_activate, self, 0);
		gtk_menu_shell_append ((GtkMenuShell*) menu, (GtkWidget*) ((GtkMenuItem*) item));
		gtk_menu_popup (menu, NULL, NULL, NULL, NULL, (*event).button, (*event).time);
		gtk_widget_show_all ((GtkWidget*) menu);
		return (_tmp3 = TRUE, (menu == NULL) ? NULL : (menu = (g_object_unref (menu), NULL)), (item == NULL) ? NULL : (item = (g_object_unref (item), NULL)), _tmp3);
	}
	return FALSE;
}


static gboolean _gmpc_widget_similar_songs_tree_right_menu_gtk_widget_button_release_event (GmpcMpdDataTreeview* _sender, const GdkEventButton* event, gpointer self) {
	return gmpc_widget_similar_songs_tree_right_menu (self, _sender, event);
}


static void _gmpc_widget_similar_songs_tree_row_activated_gtk_tree_view_row_activated (GmpcMpdDataTreeview* _sender, const GtkTreePath* path, GtkTreeViewColumn* column, gpointer self) {
	gmpc_widget_similar_songs_tree_row_activated (self, _sender, path, column);
}


static gboolean gmpc_widget_similar_songs_update_sim_song (GmpcWidgetSimilarSongs* self) {
	MetaData* _tmp14;
	g_return_val_if_fail (self != NULL, FALSE);
	if (self->priv->current == NULL) {
		GtkWidget* _tmp0;
		self->priv->current = meta_data_get_text_list (self->priv->copy);
		_tmp0 = NULL;
		self->priv->pchild = (_tmp0 = (GtkWidget*) g_object_ref_sink ((GtkProgressBar*) gtk_progress_bar_new ()), (self->priv->pchild == NULL) ? NULL : (self->priv->pchild = (g_object_unref (self->priv->pchild), NULL)), _tmp0);
		gtk_container_add ((GtkContainer*) self, self->priv->pchild);
		gtk_widget_show_all ((GtkWidget*) self);
	}
	gtk_progress_bar_pulse (GTK_PROGRESS_BAR (self->priv->pchild));
	if (self->priv->current != NULL) {
		const char* _tmp1;
		char* entry;
		_tmp1 = NULL;
		entry = (_tmp1 = (const char*) self->priv->current->data, (_tmp1 == NULL) ? NULL : g_strdup (_tmp1));
		if (entry != NULL) {
			char** _tmp3;
			gint split_size;
			gint split_length1;
			char** _tmp2;
			char** split;
			char** _tmp5;
			gint art_split_size;
			gint art_split_length1;
			char** _tmp4;
			char** art_split;
			MpdData* data;
			_tmp3 = NULL;
			_tmp2 = NULL;
			split = (_tmp3 = _tmp2 = g_strsplit (entry, "::", 2), split_length1 = _vala_array_length (_tmp2), split_size = split_length1, _tmp3);
			mpd_database_search_start (connection, FALSE);
			_tmp5 = NULL;
			_tmp4 = NULL;
			art_split = (_tmp5 = _tmp4 = g_strsplit (split[0], " ", 0), art_split_length1 = _vala_array_length (_tmp4), art_split_size = art_split_length1, _tmp5);
			{
				char** artist_collection;
				int artist_collection_length1;
				int artist_it;
				artist_collection = art_split;
				artist_collection_length1 = art_split_length1;
				for (artist_it = 0; artist_it < art_split_length1; artist_it = artist_it + 1) {
					const char* _tmp6;
					char* artist;
					_tmp6 = NULL;
					artist = (_tmp6 = artist_collection[artist_it], (_tmp6 == NULL) ? NULL : g_strdup (_tmp6));
					{
						mpd_database_search_add_constraint (connection, MPD_TAG_ITEM_ARTIST, artist);
						artist = (g_free (artist), NULL);
					}
				}
			}
			mpd_database_search_add_constraint (connection, MPD_TAG_ITEM_TITLE, split[1]);
			data = mpd_database_search_commit (connection);
			if (data != NULL) {
				MpdData* _tmp7;
				_tmp7 = NULL;
				self->priv->item = mpd_data_concatenate (self->priv->item, (_tmp7 = data, data = NULL, _tmp7));
			}
			split = (_vala_array_free (split, split_length1, (GDestroyNotify) g_free), NULL);
			art_split = (_vala_array_free (art_split, art_split_length1, (GDestroyNotify) g_free), NULL);
			(data == NULL) ? NULL : (data = (mpd_data_free (data), NULL));
		}
		self->priv->current = self->priv->current->next;
		if (self->priv->current != NULL) {
			gboolean _tmp8;
			return (_tmp8 = TRUE, entry = (g_free (entry), NULL), _tmp8);
		}
		entry = (g_free (entry), NULL);
	}
	gtk_object_destroy ((GtkObject*) self->priv->pchild);
	if (self->priv->item != NULL) {
		GmpcMpdDataModel* model;
		MpdData* _tmp9;
		GmpcMpdDataTreeview* tree;
		GtkWidget* _tmp11;
		GtkWidget* _tmp10;
		model = gmpc_mpddata_model_new ();
		_tmp9 = NULL;
		gmpc_mpddata_model_set_mpd_data (model, (_tmp9 = self->priv->item, self->priv->item = NULL, _tmp9));
		tree = g_object_ref_sink (gmpc_mpddata_treeview_new ("similar-song", TRUE, (GtkTreeModel*) model));
		gmpc_mpddata_treeview_enable_click_fix (tree);
		g_signal_connect_object ((GtkWidget*) tree, "button-release-event", (GCallback) _gmpc_widget_similar_songs_tree_right_menu_gtk_widget_button_release_event, self, 0);
		g_signal_connect_object ((GtkTreeView*) tree, "row-activated", (GCallback) _gmpc_widget_similar_songs_tree_row_activated_gtk_tree_view_row_activated, self, 0);
		gtk_container_add ((GtkContainer*) self, (GtkWidget*) tree);
		_tmp11 = NULL;
		_tmp10 = NULL;
		self->priv->pchild = (_tmp11 = (_tmp10 = (GtkWidget*) tree, (_tmp10 == NULL) ? NULL : g_object_ref (_tmp10)), (self->priv->pchild == NULL) ? NULL : (self->priv->pchild = (g_object_unref (self->priv->pchild), NULL)), _tmp11);
		(model == NULL) ? NULL : (model = (g_object_unref (model), NULL));
		(tree == NULL) ? NULL : (tree = (g_object_unref (tree), NULL));
	} else {
		GtkLabel* label;
		GtkWidget* _tmp13;
		GtkWidget* _tmp12;
		label = g_object_ref_sink ((GtkLabel*) gtk_label_new (_ ("Unavailable")));
		gtk_misc_set_alignment ((GtkMisc*) label, 0.0f, 0.0f);
		gtk_container_add ((GtkContainer*) self, (GtkWidget*) label);
		_tmp13 = NULL;
		_tmp12 = NULL;
		self->priv->pchild = (_tmp13 = (_tmp12 = (GtkWidget*) label, (_tmp12 == NULL) ? NULL : g_object_ref (_tmp12)), (self->priv->pchild == NULL) ? NULL : (self->priv->pchild = (g_object_unref (self->priv->pchild), NULL)), _tmp13);
		(label == NULL) ? NULL : (label = (g_object_unref (label), NULL));
	}
	_tmp14 = NULL;
	self->priv->copy = (_tmp14 = NULL, (self->priv->copy == NULL) ? NULL : (self->priv->copy = (meta_data_free (self->priv->copy), NULL)), _tmp14);
	self->priv->idle_add = (guint) 0;
	gtk_widget_show_all ((GtkWidget*) self);
	return FALSE;
}


static gboolean _gmpc_widget_similar_songs_update_sim_song_gsource_func (gpointer self) {
	return gmpc_widget_similar_songs_update_sim_song (self);
}


static void gmpc_widget_similar_songs_metadata_changed (GmpcWidgetSimilarSongs* self, GmpcMetaWatcher* gmw, const mpd_Song* song, MetaDataType type, MetaDataResult result, const MetaData* met) {
	g_return_if_fail (self != NULL);
	g_return_if_fail (gmw != NULL);
	g_return_if_fail (song != NULL);
	if (g_utf8_collate (self->priv->song->artist, song->artist) != 0) {
		return;
	}
	if (type != META_SONG_SIMILAR) {
		return;
	}
	if (self->priv->pchild != NULL) {
		gtk_object_destroy ((GtkObject*) self->priv->pchild);
	}
	if (result == META_DATA_FETCHING) {
		GtkLabel* label;
		GtkWidget* _tmp1;
		GtkWidget* _tmp0;
		label = g_object_ref_sink ((GtkLabel*) gtk_label_new (_ ("Fetching .. ")));
		gtk_misc_set_alignment ((GtkMisc*) label, 0.0f, 0.0f);
		gtk_container_add ((GtkContainer*) self, (GtkWidget*) label);
		_tmp1 = NULL;
		_tmp0 = NULL;
		self->priv->pchild = (_tmp1 = (_tmp0 = (GtkWidget*) label, (_tmp0 == NULL) ? NULL : g_object_ref (_tmp0)), (self->priv->pchild == NULL) ? NULL : (self->priv->pchild = (g_object_unref (self->priv->pchild), NULL)), _tmp1);
		(label == NULL) ? NULL : (label = (g_object_unref (label), NULL));
	} else {
		if (result == META_DATA_UNAVAILABLE) {
			GtkLabel* label;
			GtkWidget* _tmp3;
			GtkWidget* _tmp2;
			label = g_object_ref_sink ((GtkLabel*) gtk_label_new (_ ("Unavailable")));
			gtk_misc_set_alignment ((GtkMisc*) label, 0.0f, 0.0f);
			gtk_container_add ((GtkContainer*) self, (GtkWidget*) label);
			_tmp3 = NULL;
			_tmp2 = NULL;
			self->priv->pchild = (_tmp3 = (_tmp2 = (GtkWidget*) label, (_tmp2 == NULL) ? NULL : g_object_ref (_tmp2)), (self->priv->pchild == NULL) ? NULL : (self->priv->pchild = (g_object_unref (self->priv->pchild), NULL)), _tmp3);
			(label == NULL) ? NULL : (label = (g_object_unref (label), NULL));
		} else {
			if (meta_data_is_text_list (met)) {
				MetaData* _tmp4;
				_tmp4 = NULL;
				self->priv->copy = (_tmp4 = meta_data_dup_steal (met), (self->priv->copy == NULL) ? NULL : (self->priv->copy = (meta_data_free (self->priv->copy), NULL)), _tmp4);
				self->priv->idle_add = g_idle_add (_gmpc_widget_similar_songs_update_sim_song_gsource_func, self);
				return;
			} else {
				GtkLabel* label;
				GtkWidget* _tmp6;
				GtkWidget* _tmp5;
				label = g_object_ref_sink ((GtkLabel*) gtk_label_new (_ ("Unavailable")));
				gtk_misc_set_alignment ((GtkMisc*) label, 0.0f, 0.0f);
				gtk_container_add ((GtkContainer*) self, (GtkWidget*) label);
				_tmp6 = NULL;
				_tmp5 = NULL;
				self->priv->pchild = (_tmp6 = (_tmp5 = (GtkWidget*) label, (_tmp5 == NULL) ? NULL : g_object_ref (_tmp5)), (self->priv->pchild == NULL) ? NULL : (self->priv->pchild = (g_object_unref (self->priv->pchild), NULL)), _tmp6);
				(label == NULL) ? NULL : (label = (g_object_unref (label), NULL));
			}
		}
	}
	gtk_widget_show_all ((GtkWidget*) self);
}


static void _gmpc_widget_similar_songs_metadata_changed_gmpc_meta_watcher_data_changed (GmpcMetaWatcher* _sender, const mpd_Song* song, MetaDataType type, MetaDataResult result, const MetaData* met, gpointer self) {
	gmpc_widget_similar_songs_metadata_changed (self, _sender, song, type, result, met);
}


static void gmpc_widget_similar_songs_update (GmpcWidgetSimilarSongs* self) {
	MetaData* item;
	MetaData* _tmp2;
	MetaDataResult _tmp1;
	MetaData* _tmp0;
	MetaDataResult result;
	g_return_if_fail (self != NULL);
	item = NULL;
	g_signal_connect_object (gmw, "data-changed", (GCallback) _gmpc_widget_similar_songs_metadata_changed_gmpc_meta_watcher_data_changed, self, 0);
	_tmp2 = NULL;
	_tmp0 = NULL;
	result = (_tmp1 = gmpc_meta_watcher_get_meta_path (gmw, self->priv->song, META_SONG_SIMILAR, &_tmp0), item = (_tmp2 = _tmp0, (item == NULL) ? NULL : (item = (meta_data_free (item), NULL)), _tmp2), _tmp1);
	gmpc_widget_similar_songs_metadata_changed (self, gmw, self->priv->song, META_SONG_SIMILAR, result, item);
	(item == NULL) ? NULL : (item = (meta_data_free (item), NULL));
}


static void gmpc_widget_similar_songs_real_activate (GtkExpander* base) {
	GmpcWidgetSimilarSongs * self;
	self = (GmpcWidgetSimilarSongs*) base;
	if (!gtk_expander_get_expanded ((GtkExpander*) self)) {
		gtk_expander_set_expanded ((GtkExpander*) self, TRUE);
		if (!self->priv->filled) {
			gmpc_widget_similar_songs_update (self);
			self->priv->filled = TRUE;
		}
	} else {
		gtk_expander_set_expanded ((GtkExpander*) self, FALSE);
	}
}


static void gmpc_widget_similar_songs_class_init (GmpcWidgetSimilarSongsClass * klass) {
	gmpc_widget_similar_songs_parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (GmpcWidgetSimilarSongsPrivate));
	G_OBJECT_CLASS (klass)->finalize = gmpc_widget_similar_songs_finalize;
	GTK_EXPANDER_CLASS (klass)->activate = gmpc_widget_similar_songs_real_activate;
}


static void gmpc_widget_similar_songs_instance_init (GmpcWidgetSimilarSongs * self) {
	self->priv = GMPC_WIDGET_SIMILAR_SONGS_GET_PRIVATE (self);
	self->priv->song = NULL;
	self->priv->filled = FALSE;
	self->priv->pchild = NULL;
	self->priv->idle_add = (guint) 0;
	self->priv->copy = NULL;
	self->priv->item = NULL;
	self->priv->current = NULL;
}


static void gmpc_widget_similar_songs_finalize (GObject* obj) {
	GmpcWidgetSimilarSongs * self;
	self = GMPC_WIDGET_SIMILAR_SONGS (obj);
	{
		if (self->priv->idle_add > 0) {
			g_source_remove (self->priv->idle_add);
			self->priv->idle_add = (guint) 0;
		}
	}
	(self->priv->song == NULL) ? NULL : (self->priv->song = (mpd_freeSong (self->priv->song), NULL));
	(self->priv->pchild == NULL) ? NULL : (self->priv->pchild = (g_object_unref (self->priv->pchild), NULL));
	(self->priv->copy == NULL) ? NULL : (self->priv->copy = (meta_data_free (self->priv->copy), NULL));
	(self->priv->item == NULL) ? NULL : (self->priv->item = (mpd_data_free (self->priv->item), NULL));
	G_OBJECT_CLASS (gmpc_widget_similar_songs_parent_class)->finalize (obj);
}


GType gmpc_widget_similar_songs_get_type (void) {
	static GType gmpc_widget_similar_songs_type_id = 0;
	if (gmpc_widget_similar_songs_type_id == 0) {
		static const GTypeInfo g_define_type_info = { sizeof (GmpcWidgetSimilarSongsClass), (GBaseInitFunc) NULL, (GBaseFinalizeFunc) NULL, (GClassInitFunc) gmpc_widget_similar_songs_class_init, (GClassFinalizeFunc) NULL, NULL, sizeof (GmpcWidgetSimilarSongs), 0, (GInstanceInitFunc) gmpc_widget_similar_songs_instance_init, NULL };
		gmpc_widget_similar_songs_type_id = g_type_register_static (GTK_TYPE_EXPANDER, "GmpcWidgetSimilarSongs", &g_define_type_info, 0);
	}
	return gmpc_widget_similar_songs_type_id;
}


static void _g_list_free_g_object_unref (GList* self) {
	g_list_foreach (self, (GFunc) g_object_unref, NULL);
	g_list_free (self);
}


static void gmpc_widget_similar_artist_metadata_changed (GmpcWidgetSimilarArtist* self, GmpcMetaWatcher* gmw, const mpd_Song* song, MetaDataType type, MetaDataResult result, const MetaData* met) {
	GError * inner_error;
	GList* child_list;
	gboolean _tmp1;
	gboolean _tmp2;
	g_return_if_fail (self != NULL);
	g_return_if_fail (gmw != NULL);
	g_return_if_fail (song != NULL);
	g_return_if_fail (met != NULL);
	inner_error = NULL;
	if (g_utf8_collate (self->priv->song->artist, song->artist) != 0) {
		return;
	}
	if (type != META_ARTIST_SIMILAR) {
		return;
	}
	/* clear widgets */
	child_list = gtk_container_get_children ((GtkContainer*) self);
	{
		GList* child_collection;
		GList* child_it;
		child_collection = child_list;
		for (child_it = child_collection; child_it != NULL; child_it = child_it->next) {
			GtkWidget* _tmp0;
			GtkWidget* child;
			_tmp0 = NULL;
			child = (_tmp0 = (GtkWidget*) child_it->data, (_tmp0 == NULL) ? NULL : g_object_ref (_tmp0));
			{
				gtk_object_destroy ((GtkObject*) child);
				(child == NULL) ? NULL : (child = (g_object_unref (child), NULL));
			}
		}
	}
	_tmp1 = FALSE;
	_tmp2 = FALSE;
	if (result == META_DATA_UNAVAILABLE) {
		_tmp2 = TRUE;
	} else {
		_tmp2 = meta_data_is_empty (met);
	}
	if (_tmp2) {
		_tmp1 = TRUE;
	} else {
		_tmp1 = !meta_data_is_text_list (met);
	}
	if (_tmp1) {
		GtkLabel* label;
		label = g_object_ref_sink ((GtkLabel*) gtk_label_new (_ ("Unavailable")));
		gtk_table_attach ((GtkTable*) self, (GtkWidget*) label, (guint) 0, (guint) 1, (guint) 0, (guint) 1, GTK_SHRINK, GTK_SHRINK, (guint) 0, (guint) 0);
		(label == NULL) ? NULL : (label = (g_object_unref (label), NULL));
	} else {
		if (result == META_DATA_FETCHING) {
			GtkLabel* label;
			label = g_object_ref_sink ((GtkLabel*) gtk_label_new (_ ("Fetching")));
			gtk_table_attach ((GtkTable*) self, (GtkWidget*) label, (guint) 0, (guint) 1, (guint) 0, (guint) 1, GTK_SHRINK, GTK_SHRINK, (guint) 0, (guint) 0);
			(label == NULL) ? NULL : (label = (g_object_unref (label), NULL));
		} else {
			GList* in_db_list;
			GList* list;
			gint i;
			in_db_list = NULL;
			list = g_list_copy (meta_data_get_text_list (met));
			if (list != NULL) {
				MpdData* data;
				const MpdData* iter;
				mpd_database_search_field_start (connection, MPD_TAG_ITEM_ARTIST);
				data = mpd_database_search_commit (connection);
				iter = mpd_data_get_first (data);
				while (TRUE) {
					gboolean _tmp3;
					gboolean _tmp4;
					_tmp3 = FALSE;
					if (iter != NULL) {
						_tmp3 = list != NULL;
					} else {
						_tmp3 = FALSE;
					}
					if (!_tmp3) {
						break;
					}
					_tmp4 = FALSE;
					if (iter->tag != NULL) {
						_tmp4 = string_get_length (iter->tag) > 0;
					} else {
						_tmp4 = FALSE;
					}
					if (_tmp4) {
						GList* liter;
						char* artist;
						liter = g_list_first (list);
						artist = g_regex_escape_string (iter->tag, -1);
						{
							GRegex* reg;
							gboolean _tmp5;
							reg = g_regex_new (artist, G_REGEX_CASELESS, 0, &inner_error);
							if (inner_error != NULL) {
								goto __catch8_g_error;
								goto __finally8;
							}
							_tmp5 = FALSE;
							do {
								if (_tmp5) {
									gboolean _tmp6;
									_tmp6 = FALSE;
									if (liter != NULL) {
										_tmp6 = (liter = liter->next) != NULL;
									} else {
										_tmp6 = FALSE;
									}
									if (!_tmp6) {
										break;
									}
								}
								_tmp5 = TRUE;
								if (g_regex_match (reg, (const char*) liter->data, 0, NULL)) {
									in_db_list = g_list_prepend (in_db_list, gmpc_widget_similar_artist_new_artist_button (self, iter->tag, TRUE));
									list = g_list_remove (list, (const char*) liter->data);
									liter = NULL;
								}
							} while (TRUE);
							(reg == NULL) ? NULL : (reg = (g_regex_unref (reg), NULL));
						}
						goto __finally8;
						__catch8_g_error:
						{
							GError * E;
							E = inner_error;
							inner_error = NULL;
							{
								g_assert_not_reached ();
								(E == NULL) ? NULL : (E = (g_error_free (E), NULL));
							}
						}
						__finally8:
						if (inner_error != NULL) {
							artist = (g_free (artist), NULL);
							(data == NULL) ? NULL : (data = (mpd_data_free (data), NULL));
							(in_db_list == NULL) ? NULL : (in_db_list = (_g_list_free_g_object_unref (in_db_list), NULL));
							(list == NULL) ? NULL : (list = (g_list_free (list), NULL));
							(child_list == NULL) ? NULL : (child_list = (g_list_free (child_list), NULL));
							g_critical ("file %s: line %d: uncaught error: %s", __FILE__, __LINE__, inner_error->message);
							g_clear_error (&inner_error);
							return;
						}
						artist = (g_free (artist), NULL);
					}
					iter = mpd_data_get_next_real (iter, FALSE);
				}
				(data == NULL) ? NULL : (data = (mpd_data_free (data), NULL));
			}
			{
				GList* artist_collection;
				GList* artist_it;
				artist_collection = list;
				for (artist_it = artist_collection; artist_it != NULL; artist_it = artist_it->next) {
					const char* _tmp7;
					char* artist;
					_tmp7 = NULL;
					artist = (_tmp7 = (const char*) artist_it->data, (_tmp7 == NULL) ? NULL : g_strdup (_tmp7));
					{
						in_db_list = g_list_prepend (in_db_list, gmpc_widget_similar_artist_new_artist_button (self, artist, FALSE));
						artist = (g_free (artist), NULL);
					}
				}
			}
			in_db_list = g_list_reverse (in_db_list);
			i = 0;
			gtk_widget_hide ((GtkWidget*) self);
			{
				GList* item_collection;
				GList* item_it;
				item_collection = in_db_list;
				for (item_it = item_collection; item_it != NULL; item_it = item_it->next) {
					GtkWidget* _tmp9;
					GtkWidget* item;
					_tmp9 = NULL;
					item = (_tmp9 = (GtkWidget*) item_it->data, (_tmp9 == NULL) ? NULL : g_object_ref (_tmp9));
					{
						if (i < 50) {
							gtk_table_attach ((GtkTable*) self, item, (guint) (i % 4), (guint) ((i % 4) + 1), (guint) (i / 4), (guint) ((i / 4) + 1), GTK_EXPAND | GTK_FILL, GTK_SHRINK, (guint) 0, (guint) 0);
						} else {
							GObject* _tmp8;
							_tmp8 = NULL;
							_tmp8 = g_object_ref_sink ((GObject*) item);
							(_tmp8 == NULL) ? NULL : (_tmp8 = (g_object_unref (_tmp8), NULL));
							gtk_object_destroy ((GtkObject*) item);
						}
						i++;
						(item == NULL) ? NULL : (item = (g_object_unref (item), NULL));
					}
				}
			}
			(in_db_list == NULL) ? NULL : (in_db_list = (_g_list_free_g_object_unref (in_db_list), NULL));
			(list == NULL) ? NULL : (list = (g_list_free (list), NULL));
		}
	}
	gtk_widget_show_all ((GtkWidget*) self);
	(child_list == NULL) ? NULL : (child_list = (g_list_free (child_list), NULL));
}


static void gmpc_widget_similar_artist_artist_button_clicked (GmpcWidgetSimilarArtist* self, GtkButton* button) {
	const char* artist;
	g_return_if_fail (self != NULL);
	g_return_if_fail (button != NULL);
	artist = (const char*) g_object_get_data ((GObject*) button, "artist");
	gmpc_metadata_browser_set_artist (self->priv->browser, artist);
}


static gboolean _misc_header_expose_event_gtk_widget_expose_event (GtkEventBox* _sender, const GdkEventExpose* event, gpointer self) {
	return misc_header_expose_event (_sender, event);
}


static void _gmpc_widget_similar_artist_artist_button_clicked_gtk_button_clicked (GtkButton* _sender, gpointer self) {
	gmpc_widget_similar_artist_artist_button_clicked (self, _sender);
}


GtkWidget* gmpc_widget_similar_artist_new_artist_button (GmpcWidgetSimilarArtist* self, const char* artist, gboolean in_db) {
	GtkHBox* hbox;
	GtkEventBox* event;
	GmpcMetaImage* image;
	mpd_Song* song;
	char* _tmp1;
	const char* _tmp0;
	GtkLabel* label;
	GtkWidget* _tmp2;
	g_return_val_if_fail (self != NULL, NULL);
	g_return_val_if_fail (artist != NULL, NULL);
	hbox = g_object_ref_sink ((GtkHBox*) gtk_hbox_new (FALSE, 6));
	gtk_container_set_border_width ((GtkContainer*) hbox, (guint) 6);
	event = g_object_ref_sink ((GtkEventBox*) gtk_event_box_new ());
	g_object_set ((GtkWidget*) event, "app-paintable", TRUE, NULL);
	g_signal_connect ((GtkWidget*) event, "expose-event", (GCallback) _misc_header_expose_event_gtk_widget_expose_event, NULL);
	image = g_object_ref_sink (gmpc_metaimage_new_size (META_ARTIST_ART, 48));
	song = mpd_newSong ();
	_tmp1 = NULL;
	_tmp0 = NULL;
	song->artist = (_tmp1 = (_tmp0 = artist, (_tmp0 == NULL) ? NULL : g_strdup (_tmp0)), song->artist = (g_free (song->artist), NULL), _tmp1);
	gmpc_metaimage_update_cover_from_song_delayed (image, song);
	gtk_box_pack_start ((GtkBox*) hbox, (GtkWidget*) image, FALSE, FALSE, (guint) 0);
	label = g_object_ref_sink ((GtkLabel*) gtk_label_new (artist));
	gtk_misc_set_alignment ((GtkMisc*) label, 0.0f, 0.5f);
	gtk_label_set_ellipsize (label, PANGO_ELLIPSIZE_END);
	gtk_box_pack_start ((GtkBox*) hbox, (GtkWidget*) label, TRUE, TRUE, (guint) 0);
	if (in_db) {
		GtkButton* find;
		find = g_object_ref_sink ((GtkButton*) gtk_button_new_from_stock ("gtk-find"));
		gtk_button_set_relief (find, GTK_RELIEF_NONE);
		gtk_box_pack_start ((GtkBox*) hbox, (GtkWidget*) find, FALSE, FALSE, (guint) 0);
		g_object_set_data_full ((GObject*) find, "artist", (void*) g_strdup_printf ("%s", artist), (GDestroyNotify) g_free);
		g_signal_connect_object (find, "clicked", (GCallback) _gmpc_widget_similar_artist_artist_button_clicked_gtk_button_clicked, self, 0);
		(find == NULL) ? NULL : (find = (g_object_unref (find), NULL));
	}
	gtk_container_add ((GtkContainer*) event, (GtkWidget*) hbox);
	gtk_widget_set_size_request ((GtkWidget*) event, 180, 60);
	_tmp2 = NULL;
	return (_tmp2 = (GtkWidget*) event, (hbox == NULL) ? NULL : (hbox = (g_object_unref (hbox), NULL)), (image == NULL) ? NULL : (image = (g_object_unref (image), NULL)), (song == NULL) ? NULL : (song = (mpd_freeSong (song), NULL)), (label == NULL) ? NULL : (label = (g_object_unref (label), NULL)), _tmp2);
}


static void _gmpc_widget_similar_artist_metadata_changed_gmpc_meta_watcher_data_changed (GmpcMetaWatcher* _sender, const mpd_Song* song, MetaDataType type, MetaDataResult result, const MetaData* met, gpointer self) {
	gmpc_widget_similar_artist_metadata_changed (self, _sender, song, type, result, met);
}


static GmpcWidgetSimilarArtist* gmpc_widget_similar_artist_construct (GType object_type, GmpcMetadataBrowser* browser, MpdObj* server, const mpd_Song* song) {
	GmpcWidgetSimilarArtist * self;
	MetaData* item;
	GmpcMetadataBrowser* _tmp1;
	GmpcMetadataBrowser* _tmp0;
	mpd_Song* _tmp3;
	const mpd_Song* _tmp2;
	MetaData* _tmp6;
	MetaDataResult _tmp5;
	MetaData* _tmp4;
	MetaDataResult result;
	g_return_val_if_fail (browser != NULL, NULL);
	g_return_val_if_fail (server != NULL, NULL);
	g_return_val_if_fail (song != NULL, NULL);
	self = g_object_newv (object_type, 0, NULL);
	item = NULL;
	_tmp1 = NULL;
	_tmp0 = NULL;
	self->priv->browser = (_tmp1 = (_tmp0 = browser, (_tmp0 == NULL) ? NULL : g_object_ref (_tmp0)), (self->priv->browser == NULL) ? NULL : (self->priv->browser = (g_object_unref (self->priv->browser), NULL)), _tmp1);
	_tmp3 = NULL;
	_tmp2 = NULL;
	self->priv->song = (_tmp3 = (_tmp2 = song, (_tmp2 == NULL) ? NULL : mpd_songDup (_tmp2)), (self->priv->song == NULL) ? NULL : (self->priv->song = (mpd_freeSong (self->priv->song), NULL)), _tmp3);
	gtk_table_set_homogeneous ((GtkTable*) self, TRUE);
	gtk_table_set_row_spacings ((GtkTable*) self, (guint) 6);
	gtk_table_set_col_spacings ((GtkTable*) self, (guint) 6);
	g_signal_connect_object (gmw, "data-changed", (GCallback) _gmpc_widget_similar_artist_metadata_changed_gmpc_meta_watcher_data_changed, self, 0);
	_tmp6 = NULL;
	_tmp4 = NULL;
	result = (_tmp5 = gmpc_meta_watcher_get_meta_path (gmw, song, META_ARTIST_SIMILAR, &_tmp4), item = (_tmp6 = _tmp4, (item == NULL) ? NULL : (item = (meta_data_free (item), NULL)), _tmp6), _tmp5);
	if (result == META_DATA_AVAILABLE) {
		gmpc_widget_similar_artist_metadata_changed (self, gmw, self->priv->song, META_ARTIST_SIMILAR, result, item);
	}
	return self;
}


static GmpcWidgetSimilarArtist* gmpc_widget_similar_artist_new (GmpcMetadataBrowser* browser, MpdObj* server, const mpd_Song* song) {
	return gmpc_widget_similar_artist_construct (GMPC_WIDGET_TYPE_SIMILAR_ARTIST, browser, server, song);
}


static void gmpc_widget_similar_artist_class_init (GmpcWidgetSimilarArtistClass * klass) {
	gmpc_widget_similar_artist_parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (GmpcWidgetSimilarArtistPrivate));
	G_OBJECT_CLASS (klass)->finalize = gmpc_widget_similar_artist_finalize;
}


static void gmpc_widget_similar_artist_instance_init (GmpcWidgetSimilarArtist * self) {
	self->priv = GMPC_WIDGET_SIMILAR_ARTIST_GET_PRIVATE (self);
	self->priv->song = NULL;
	self->priv->browser = NULL;
}


static void gmpc_widget_similar_artist_finalize (GObject* obj) {
	GmpcWidgetSimilarArtist * self;
	self = GMPC_WIDGET_SIMILAR_ARTIST (obj);
	(self->priv->song == NULL) ? NULL : (self->priv->song = (mpd_freeSong (self->priv->song), NULL));
	(self->priv->browser == NULL) ? NULL : (self->priv->browser = (g_object_unref (self->priv->browser), NULL));
	G_OBJECT_CLASS (gmpc_widget_similar_artist_parent_class)->finalize (obj);
}


GType gmpc_widget_similar_artist_get_type (void) {
	static GType gmpc_widget_similar_artist_type_id = 0;
	if (gmpc_widget_similar_artist_type_id == 0) {
		static const GTypeInfo g_define_type_info = { sizeof (GmpcWidgetSimilarArtistClass), (GBaseInitFunc) NULL, (GBaseFinalizeFunc) NULL, (GClassInitFunc) gmpc_widget_similar_artist_class_init, (GClassFinalizeFunc) NULL, NULL, sizeof (GmpcWidgetSimilarArtist), 0, (GInstanceInitFunc) gmpc_widget_similar_artist_instance_init, NULL };
		gmpc_widget_similar_artist_type_id = g_type_register_static (GTK_TYPE_TABLE, "GmpcWidgetSimilarArtist", &g_define_type_info, 0);
	}
	return gmpc_widget_similar_artist_type_id;
}


static void gmpc_widget_more_expand (GmpcWidgetMore* self, GtkButton* but) {
	g_return_if_fail (self != NULL);
	g_return_if_fail (but != NULL);
	if (self->priv->expand_state == 0) {
		gtk_button_set_label (but, _ ("(less)"));
		gtk_widget_set_size_request ((GtkWidget*) self->priv->ali, -1, -1);
		self->priv->expand_state = 1;
	} else {
		gtk_button_set_label (but, _ ("(more)"));
		gtk_widget_set_size_request ((GtkWidget*) self->priv->ali, -1, self->priv->max_height);
		self->priv->expand_state = 0;
	}
}


static void gmpc_widget_more_size_changed (GmpcWidgetMore* self, GtkWidget* child, const GdkRectangle* alloc) {
	g_return_if_fail (self != NULL);
	g_return_if_fail (child != NULL);
	if ((*alloc).height < (self->priv->max_height - 12)) {
		gtk_widget_set_size_request ((GtkWidget*) self->priv->ali, -1, -1);
		gtk_widget_hide ((GtkWidget*) self->priv->expand_button);
	} else {
		if (self->priv->expand_state == 0) {
			gtk_widget_set_size_request ((GtkWidget*) self->priv->ali, -1, self->priv->max_height);
		}
		gtk_widget_show ((GtkWidget*) self->priv->expand_button);
	}
}


static void gmpc_widget_more_bg_style_changed (GmpcWidgetMore* self, GtkWidget* frame, GtkStyle* style) {
	GdkColor _tmp0 = {0};
	GdkColor _tmp1 = {0};
	GdkColor _tmp2 = {0};
	GdkColor _tmp3 = {0};
	g_return_if_fail (self != NULL);
	g_return_if_fail (frame != NULL);
	gtk_widget_modify_bg (self->priv->pchild, GTK_STATE_NORMAL, (_tmp0 = gtk_widget_get_style ((GtkWidget*) gtk_widget_get_parent ((GtkWidget*) self))->mid[GTK_STATE_NORMAL], &_tmp0));
	gtk_widget_modify_base (self->priv->pchild, GTK_STATE_NORMAL, (_tmp1 = gtk_widget_get_style ((GtkWidget*) gtk_widget_get_parent ((GtkWidget*) self))->mid[GTK_STATE_NORMAL], &_tmp1));
	gtk_widget_modify_bg ((GtkWidget*) self->priv->eventbox, GTK_STATE_NORMAL, (_tmp2 = gtk_widget_get_style ((GtkWidget*) gtk_widget_get_parent ((GtkWidget*) self))->dark[GTK_STATE_NORMAL], &_tmp2));
	gtk_widget_modify_base ((GtkWidget*) self->priv->eventbox, GTK_STATE_NORMAL, (_tmp3 = gtk_widget_get_style ((GtkWidget*) gtk_widget_get_parent ((GtkWidget*) self))->dark[GTK_STATE_NORMAL], &_tmp3));
}


static void _gmpc_widget_more_bg_style_changed_gtk_widget_style_set (GmpcWidgetMore* _sender, GtkStyle* previous_style, gpointer self) {
	gmpc_widget_more_bg_style_changed (self, _sender, previous_style);
}


static void _gmpc_widget_more_expand_gtk_button_clicked (GtkButton* _sender, gpointer self) {
	gmpc_widget_more_expand (self, _sender);
}


static void _gmpc_widget_more_size_changed_gtk_widget_size_allocate (GtkWidget* _sender, const GdkRectangle* allocation, gpointer self) {
	gmpc_widget_more_size_changed (self, _sender, allocation);
}


static GmpcWidgetMore* gmpc_widget_more_construct (GType object_type, const char* markup, GtkWidget* child) {
	GmpcWidgetMore * self;
	GtkWidget* _tmp1;
	GtkWidget* _tmp0;
	GtkAlignment* _tmp2;
	GtkEventBox* _tmp3;
	GtkHBox* hbox;
	GtkLabel* label;
	GtkButton* _tmp4;
	g_return_val_if_fail (markup != NULL, NULL);
	g_return_val_if_fail (child != NULL, NULL);
	self = g_object_newv (object_type, 0, NULL);
	gtk_frame_set_shadow_type ((GtkFrame*) self, GTK_SHADOW_NONE);
	_tmp1 = NULL;
	_tmp0 = NULL;
	self->priv->pchild = (_tmp1 = (_tmp0 = child, (_tmp0 == NULL) ? NULL : g_object_ref (_tmp0)), (self->priv->pchild == NULL) ? NULL : (self->priv->pchild = (g_object_unref (self->priv->pchild), NULL)), _tmp1);
	_tmp2 = NULL;
	self->priv->ali = (_tmp2 = g_object_ref_sink ((GtkAlignment*) gtk_alignment_new (0.f, 0.f, 1.f, 0.f)), (self->priv->ali == NULL) ? NULL : (self->priv->ali = (g_object_unref (self->priv->ali), NULL)), _tmp2);
	gtk_alignment_set_padding (self->priv->ali, (guint) 1, (guint) 1, (guint) 1, (guint) 1);
	_tmp3 = NULL;
	self->priv->eventbox = (_tmp3 = g_object_ref_sink ((GtkEventBox*) gtk_event_box_new ()), (self->priv->eventbox == NULL) ? NULL : (self->priv->eventbox = (g_object_unref (self->priv->eventbox), NULL)), _tmp3);
	gtk_event_box_set_visible_window (self->priv->eventbox, TRUE);
	gtk_container_add ((GtkContainer*) self, (GtkWidget*) self->priv->eventbox);
	gtk_container_add ((GtkContainer*) self->priv->eventbox, (GtkWidget*) self->priv->ali);
	gtk_widget_set_size_request ((GtkWidget*) self->priv->ali, -1, self->priv->max_height);
	gtk_container_add ((GtkContainer*) self->priv->ali, child);
	g_signal_connect_object ((GtkWidget*) self, "style-set", (GCallback) _gmpc_widget_more_bg_style_changed_gtk_widget_style_set, self, 0);
	hbox = g_object_ref_sink ((GtkHBox*) gtk_hbox_new (FALSE, 6));
	label = g_object_ref_sink ((GtkLabel*) gtk_label_new (""));
	gtk_label_set_markup (label, markup);
	gtk_box_pack_start ((GtkBox*) hbox, (GtkWidget*) label, FALSE, FALSE, (guint) 0);
	_tmp4 = NULL;
	self->priv->expand_button = (_tmp4 = g_object_ref_sink ((GtkButton*) gtk_button_new_with_label (_ ("(more)"))), (self->priv->expand_button == NULL) ? NULL : (self->priv->expand_button = (g_object_unref (self->priv->expand_button), NULL)), _tmp4);
	gtk_button_set_relief (self->priv->expand_button, GTK_RELIEF_NONE);
	g_signal_connect_object (self->priv->expand_button, "clicked", (GCallback) _gmpc_widget_more_expand_gtk_button_clicked, self, 0);
	gtk_box_pack_start ((GtkBox*) hbox, (GtkWidget*) self->priv->expand_button, FALSE, FALSE, (guint) 0);
	gtk_frame_set_label_widget ((GtkFrame*) self, (GtkWidget*) hbox);
	g_signal_connect_object (child, "size-allocate", (GCallback) _gmpc_widget_more_size_changed_gtk_widget_size_allocate, self, 0);
	return self;
}


static GmpcWidgetMore* gmpc_widget_more_new (const char* markup, GtkWidget* child) {
	return gmpc_widget_more_construct (GMPC_WIDGET_TYPE_MORE, markup, child);
}


static void gmpc_widget_more_class_init (GmpcWidgetMoreClass * klass) {
	gmpc_widget_more_parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (GmpcWidgetMorePrivate));
	G_OBJECT_CLASS (klass)->finalize = gmpc_widget_more_finalize;
}


static void gmpc_widget_more_instance_init (GmpcWidgetMore * self) {
	self->priv = GMPC_WIDGET_MORE_GET_PRIVATE (self);
	self->priv->ali = NULL;
	self->priv->expand_state = 0;
	self->priv->expand_button = NULL;
	self->priv->max_height = 100;
	self->priv->eventbox = NULL;
	self->priv->pchild = NULL;
}


static void gmpc_widget_more_finalize (GObject* obj) {
	GmpcWidgetMore * self;
	self = GMPC_WIDGET_MORE (obj);
	(self->priv->ali == NULL) ? NULL : (self->priv->ali = (g_object_unref (self->priv->ali), NULL));
	(self->priv->expand_button == NULL) ? NULL : (self->priv->expand_button = (g_object_unref (self->priv->expand_button), NULL));
	(self->priv->eventbox == NULL) ? NULL : (self->priv->eventbox = (g_object_unref (self->priv->eventbox), NULL));
	(self->priv->pchild == NULL) ? NULL : (self->priv->pchild = (g_object_unref (self->priv->pchild), NULL));
	G_OBJECT_CLASS (gmpc_widget_more_parent_class)->finalize (obj);
}


GType gmpc_widget_more_get_type (void) {
	static GType gmpc_widget_more_type_id = 0;
	if (gmpc_widget_more_type_id == 0) {
		static const GTypeInfo g_define_type_info = { sizeof (GmpcWidgetMoreClass), (GBaseInitFunc) NULL, (GBaseFinalizeFunc) NULL, (GClassInitFunc) gmpc_widget_more_class_init, (GClassFinalizeFunc) NULL, NULL, sizeof (GmpcWidgetMore), 0, (GInstanceInitFunc) gmpc_widget_more_instance_init, NULL };
		gmpc_widget_more_type_id = g_type_register_static (GTK_TYPE_FRAME, "GmpcWidgetMore", &g_define_type_info, 0);
	}
	return gmpc_widget_more_type_id;
}


static gint* gmpc_now_playing_real_get_version (GmpcPluginBase* base, int* result_length1) {
	GmpcNowPlaying * self;
	gint* _tmp0;
	self = (GmpcNowPlaying*) base;
	_tmp0 = NULL;
	return (_tmp0 = GMPC_NOW_PLAYING_version, *result_length1 = G_N_ELEMENTS (GMPC_NOW_PLAYING_version), _tmp0);
}


static const char* gmpc_now_playing_real_get_name (GmpcPluginBase* base) {
	GmpcNowPlaying * self;
	self = (GmpcNowPlaying*) base;
	return N_ ("Now Playing");
}


static void gmpc_now_playing_real_save_yourself (GmpcPluginBase* base) {
	GmpcNowPlaying * self;
	self = (GmpcNowPlaying*) base;
	if (self->priv->paned != NULL) {
		GtkScrolledWindow* _tmp0;
		gtk_object_destroy ((GtkObject*) self->priv->paned);
		_tmp0 = NULL;
		self->priv->paned = (_tmp0 = NULL, (self->priv->paned == NULL) ? NULL : (self->priv->paned = (g_object_unref (self->priv->paned), NULL)), _tmp0);
	}
	if (self->priv->np_ref != NULL) {
		GtkTreePath* path;
		path = gtk_tree_row_reference_get_path (self->priv->np_ref);
		if (path != NULL) {
			gint* _tmp1;
			gint indices_size;
			gint indices_length1;
			gint* indices;
			_tmp1 = NULL;
			indices = (_tmp1 = gtk_tree_path_get_indices (path), indices_length1 = -1, indices_size = indices_length1, _tmp1);
			cfg_set_single_value_as_int (config, gmpc_plugin_base_get_name ((GmpcPluginBase*) self), "position", indices[0]);
		}
		(path == NULL) ? NULL : (path = (gtk_tree_path_free (path), NULL));
	}
}


static void gmpc_now_playing_status_changed (GmpcNowPlaying* self, GmpcConnection* conn, MpdObj* server, ChangedStatusType what) {
	gboolean _tmp0;
	gboolean _tmp1;
	gboolean _tmp2;
	g_return_if_fail (self != NULL);
	g_return_if_fail (conn != NULL);
	g_return_if_fail (server != NULL);
	if (self->priv->paned == NULL) {
		return;
	}
	_tmp0 = FALSE;
	_tmp1 = FALSE;
	_tmp2 = FALSE;
	if ((what & MPD_CST_SONGID) == MPD_CST_SONGID) {
		_tmp2 = TRUE;
	} else {
		_tmp2 = (what & MPD_CST_PLAYLIST) == MPD_CST_PLAYLIST;
	}
	if (_tmp2) {
		_tmp1 = TRUE;
	} else {
		_tmp1 = (what & MPD_CST_STATE) == MPD_CST_STATE;
	}
	if (_tmp1) {
		_tmp0 = self->priv->selected;
	} else {
		_tmp0 = FALSE;
	}
	if (_tmp0) {
		gmpc_now_playing_update (self);
	}
}


/** 
     * Browser Interface bindings
     */
static void gmpc_now_playing_real_browser_add (GmpcPluginBrowserIface* base, GtkWidget* category_tree) {
	GmpcNowPlaying * self;
	GtkTreeView* _tmp0;
	GtkTreeView* tree;
	GtkListStore* _tmp1;
	GtkListStore* store;
	GtkTreeModel* _tmp2;
	GtkTreeModel* model;
	GtkTreeIter iter = {0};
	GtkTreeRowReference* _tmp4;
	GtkTreePath* _tmp3;
	self = (GmpcNowPlaying*) base;
	g_return_if_fail (category_tree != NULL);
	_tmp0 = NULL;
	tree = (_tmp0 = GTK_TREE_VIEW (category_tree), (_tmp0 == NULL) ? NULL : g_object_ref (_tmp0));
	_tmp1 = NULL;
	store = (_tmp1 = GTK_LIST_STORE (gtk_tree_view_get_model (tree)), (_tmp1 == NULL) ? NULL : g_object_ref (_tmp1));
	_tmp2 = NULL;
	model = (_tmp2 = gtk_tree_view_get_model (tree), (_tmp2 == NULL) ? NULL : g_object_ref (_tmp2));
	playlist3_insert_browser (&iter, cfg_get_single_value_as_int_with_default (config, gmpc_plugin_base_get_name ((GmpcPluginBase*) self), "position", 0));
	gtk_list_store_set (store, &iter, 0, ((GmpcPluginBase*) self)->id, 1, _ (gmpc_plugin_base_get_name ((GmpcPluginBase*) self)), 3, "media-audiofile", -1);
	/* Create a row reference */
	_tmp4 = NULL;
	_tmp3 = NULL;
	self->priv->np_ref = (_tmp4 = gtk_tree_row_reference_new (model, _tmp3 = gtk_tree_model_get_path (model, &iter)), (self->priv->np_ref == NULL) ? NULL : (self->priv->np_ref = (gtk_tree_row_reference_free (self->priv->np_ref), NULL)), _tmp4);
	(_tmp3 == NULL) ? NULL : (_tmp3 = (gtk_tree_path_free (_tmp3), NULL));
	(tree == NULL) ? NULL : (tree = (g_object_unref (tree), NULL));
	(store == NULL) ? NULL : (store = (g_object_unref (store), NULL));
	(model == NULL) ? NULL : (model = (g_object_unref (model), NULL));
}


static void gmpc_now_playing_real_browser_selected (GmpcPluginBrowserIface* base, GtkContainer* container) {
	GmpcNowPlaying * self;
	self = (GmpcNowPlaying*) base;
	g_return_if_fail (container != NULL);
	self->priv->selected = TRUE;
	gmpc_now_playing_browser_init (self);
	gtk_container_add (container, (GtkWidget*) self->priv->paned);
	gmpc_now_playing_update (self);
}


static void gmpc_now_playing_real_browser_unselected (GmpcPluginBrowserIface* base, GtkContainer* container) {
	GmpcNowPlaying * self;
	self = (GmpcNowPlaying*) base;
	g_return_if_fail (container != NULL);
	self->priv->selected = FALSE;
	gtk_container_remove (container, (GtkWidget*) self->priv->paned);
}


static void gmpc_now_playing_browser_bg_style_changed (GmpcNowPlaying* self, GtkScrolledWindow* bg, GtkStyle* style) {
	GdkColor _tmp0 = {0};
	g_return_if_fail (self != NULL);
	g_return_if_fail (bg != NULL);
	gtk_widget_modify_bg ((GtkWidget*) self->priv->container, GTK_STATE_NORMAL, (_tmp0 = gtk_widget_get_style ((GtkWidget*) self->priv->paned)->base[GTK_STATE_NORMAL], &_tmp0));
}


static void _gmpc_now_playing_browser_bg_style_changed_gtk_widget_style_set (GtkScrolledWindow* _sender, GtkStyle* previous_style, gpointer self) {
	gmpc_now_playing_browser_bg_style_changed (self, _sender, previous_style);
}


static void gmpc_now_playing_browser_init (GmpcNowPlaying* self) {
	g_return_if_fail (self != NULL);
	if (self->priv->paned == NULL) {
		GtkScrolledWindow* _tmp0;
		GtkEventBox* _tmp1;
		_tmp0 = NULL;
		self->priv->paned = (_tmp0 = g_object_ref_sink ((GtkScrolledWindow*) gtk_scrolled_window_new (NULL, NULL)), (self->priv->paned == NULL) ? NULL : (self->priv->paned = (g_object_unref (self->priv->paned), NULL)), _tmp0);
		gtk_scrolled_window_set_policy (self->priv->paned, GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
		gtk_scrolled_window_set_shadow_type (self->priv->paned, GTK_SHADOW_NONE);
		_tmp1 = NULL;
		self->priv->container = (_tmp1 = g_object_ref_sink ((GtkEventBox*) gtk_event_box_new ()), (self->priv->container == NULL) ? NULL : (self->priv->container = (g_object_unref (self->priv->container), NULL)), _tmp1);
		gtk_event_box_set_visible_window (self->priv->container, TRUE);
		g_signal_connect_object ((GtkWidget*) self->priv->paned, "style-set", (GCallback) _gmpc_now_playing_browser_bg_style_changed_gtk_widget_style_set, self, 0);
		gtk_scrolled_window_add_with_viewport (self->priv->paned, (GtkWidget*) self->priv->container);
	}
}


static void gmpc_now_playing_update (GmpcNowPlaying* self) {
	GError * inner_error;
	const mpd_Song* _tmp0;
	mpd_Song* song;
	gboolean _tmp1;
	g_return_if_fail (self != NULL);
	inner_error = NULL;
	if (self->priv->paned == NULL) {
		return;
	}
	_tmp0 = NULL;
	song = (_tmp0 = mpd_playlist_get_current_song (connection), (_tmp0 == NULL) ? NULL : mpd_songDup (_tmp0));
	_tmp1 = FALSE;
	if (song != NULL) {
		_tmp1 = mpd_player_get_state (connection) != MPD_STATUS_STATE_STOP;
	} else {
		_tmp1 = FALSE;
	}
	if (_tmp1) {
		char* checksum;
		checksum = mpd_song_checksum (song);
		if (_vala_strcmp0 (checksum, self->priv->song_checksum) != 0) {
			GList* list;
			GtkWidget* view;
			char* _tmp4;
			const char* _tmp3;
			/* Clear */
			list = gtk_container_get_children ((GtkContainer*) self->priv->container);
			{
				GList* child_collection;
				GList* child_it;
				child_collection = list;
				for (child_it = child_collection; child_it != NULL; child_it = child_it->next) {
					GtkWidget* _tmp2;
					GtkWidget* child;
					_tmp2 = NULL;
					child = (_tmp2 = (GtkWidget*) child_it->data, (_tmp2 == NULL) ? NULL : g_object_ref (_tmp2));
					{
						gtk_object_destroy ((GtkObject*) child);
						(child == NULL) ? NULL : (child = (g_object_unref (child), NULL));
					}
				}
			}
			view = gmpc_metadata_browser_metadata_box_show_song (self->priv->browser, song);
			gtk_container_add ((GtkContainer*) self->priv->container, view);
			_tmp4 = NULL;
			_tmp3 = NULL;
			self->priv->song_checksum = (_tmp4 = (_tmp3 = checksum, (_tmp3 == NULL) ? NULL : g_strdup (_tmp3)), self->priv->song_checksum = (g_free (self->priv->song_checksum), NULL), _tmp4);
			(list == NULL) ? NULL : (list = (g_list_free (list), NULL));
			(view == NULL) ? NULL : (view = (g_object_unref (view), NULL));
		}
		checksum = (g_free (checksum), NULL);
	} else {
		char* _tmp5;
		GList* list;
		GtkIconTheme* _tmp7;
		GtkIconTheme* it;
		GtkIconInfo* info;
		const char* _tmp8;
		char* path;
		GtkImage* image;
		GtkHBox* hbox;
		GtkLabel* label;
		char* _tmp11;
		GtkAlignment* ali;
		_tmp5 = NULL;
		self->priv->song_checksum = (_tmp5 = NULL, self->priv->song_checksum = (g_free (self->priv->song_checksum), NULL), _tmp5);
		/* Clear */
		list = gtk_container_get_children ((GtkContainer*) self->priv->container);
		{
			GList* child_collection;
			GList* child_it;
			child_collection = list;
			for (child_it = child_collection; child_it != NULL; child_it = child_it->next) {
				GtkWidget* _tmp6;
				GtkWidget* child;
				_tmp6 = NULL;
				child = (_tmp6 = (GtkWidget*) child_it->data, (_tmp6 == NULL) ? NULL : g_object_ref (_tmp6));
				{
					gtk_object_destroy ((GtkObject*) child);
					(child == NULL) ? NULL : (child = (g_object_unref (child), NULL));
				}
			}
		}
		_tmp7 = NULL;
		it = (_tmp7 = gtk_icon_theme_get_default (), (_tmp7 == NULL) ? NULL : g_object_ref (_tmp7));
		info = gtk_icon_theme_lookup_icon (it, "gmpc", 150, 0);
		_tmp8 = NULL;
		path = (_tmp8 = gtk_icon_info_get_filename (info), (_tmp8 == NULL) ? NULL : g_strdup (_tmp8));
		image = NULL;
		if (path != NULL) {
			{
				GdkPixbuf* pb;
				GtkImage* _tmp9;
				pb = gdk_pixbuf_new_from_file_at_scale (path, 150, 150, TRUE, &inner_error);
				if (inner_error != NULL) {
					goto __catch9_g_error;
					goto __finally9;
				}
				_tmp9 = NULL;
				image = (_tmp9 = g_object_ref_sink ((GtkImage*) gtk_image_new_from_pixbuf (pb)), (image == NULL) ? NULL : (image = (g_object_unref (image), NULL)), _tmp9);
				(pb == NULL) ? NULL : (pb = (g_object_unref (pb), NULL));
			}
			goto __finally9;
			__catch9_g_error:
			{
				GError * e;
				e = inner_error;
				inner_error = NULL;
				{
					(e == NULL) ? NULL : (e = (g_error_free (e), NULL));
				}
			}
			__finally9:
			if (inner_error != NULL) {
				(list == NULL) ? NULL : (list = (g_list_free (list), NULL));
				(it == NULL) ? NULL : (it = (g_object_unref (it), NULL));
				path = (g_free (path), NULL);
				(image == NULL) ? NULL : (image = (g_object_unref (image), NULL));
				(song == NULL) ? NULL : (song = (mpd_freeSong (song), NULL));
				g_critical ("file %s: line %d: uncaught error: %s", __FILE__, __LINE__, inner_error->message);
				g_clear_error (&inner_error);
				return;
			}
		}
		if (image == NULL) {
			GtkImage* _tmp10;
			_tmp10 = NULL;
			image = (_tmp10 = g_object_ref_sink ((GtkImage*) gtk_image_new_from_icon_name ("gmpc", GTK_ICON_SIZE_DIALOG)), (image == NULL) ? NULL : (image = (g_object_unref (image), NULL)), _tmp10);
		}
		hbox = g_object_ref_sink ((GtkHBox*) gtk_hbox_new (FALSE, 6));
		label = g_object_ref_sink ((GtkLabel*) gtk_label_new (_ ("Gnome Music Player Client")));
		_tmp11 = NULL;
		gtk_label_set_markup (label, _tmp11 = g_strdup_printf ("<span size='%i' weight='bold'>%s</span>", 28 * PANGO_SCALE, _ ("Gnome Music Player Client")));
		_tmp11 = (g_free (_tmp11), NULL);
		gtk_box_pack_start ((GtkBox*) hbox, (GtkWidget*) image, FALSE, FALSE, (guint) 0);
		gtk_box_pack_start ((GtkBox*) hbox, (GtkWidget*) label, FALSE, FALSE, (guint) 0);
		ali = g_object_ref_sink ((GtkAlignment*) gtk_alignment_new (0.5f, 0.5f, 0.0f, 0.0f));
		gtk_container_add ((GtkContainer*) ali, (GtkWidget*) hbox);
		gtk_container_add ((GtkContainer*) self->priv->container, (GtkWidget*) ali);
		(list == NULL) ? NULL : (list = (g_list_free (list), NULL));
		(it == NULL) ? NULL : (it = (g_object_unref (it), NULL));
		path = (g_free (path), NULL);
		(image == NULL) ? NULL : (image = (g_object_unref (image), NULL));
		(hbox == NULL) ? NULL : (hbox = (g_object_unref (hbox), NULL));
		(label == NULL) ? NULL : (label = (g_object_unref (label), NULL));
		(ali == NULL) ? NULL : (ali = (g_object_unref (ali), NULL));
	}
	gtk_widget_show_all ((GtkWidget*) self->priv->paned);
	(song == NULL) ? NULL : (song = (mpd_freeSong (song), NULL));
}


static void gmpc_now_playing_select_now_playing_browser (GmpcNowPlaying* self, GtkImageMenuItem* item) {
	GtkTreeView* tree;
	GtkTreeSelection* _tmp0;
	GtkTreeSelection* sel;
	GtkTreePath* path;
	g_return_if_fail (self != NULL);
	g_return_if_fail (item != NULL);
	tree = playlist3_get_category_tree_view ();
	_tmp0 = NULL;
	sel = (_tmp0 = gtk_tree_view_get_selection (tree), (_tmp0 == NULL) ? NULL : g_object_ref (_tmp0));
	path = gtk_tree_row_reference_get_path (self->priv->np_ref);
	gtk_tree_selection_select_path (sel, path);
	(sel == NULL) ? NULL : (sel = (g_object_unref (sel), NULL));
	(path == NULL) ? NULL : (path = (gtk_tree_path_free (path), NULL));
}


static void _gmpc_now_playing_select_now_playing_browser_gtk_menu_item_activate (GtkImageMenuItem* _sender, gpointer self) {
	gmpc_now_playing_select_now_playing_browser (self, _sender);
}


static gint gmpc_now_playing_real_browser_add_go_menu (GmpcPluginBrowserIface* base, GtkMenu* menu) {
	GmpcNowPlaying * self;
	GtkImageMenuItem* item;
	GtkImage* _tmp0;
	gint _tmp1;
	self = (GmpcNowPlaying*) base;
	g_return_val_if_fail (menu != NULL, 0);
	item = g_object_ref_sink ((GtkImageMenuItem*) gtk_image_menu_item_new_with_mnemonic (_ ("Now Playing")));
	_tmp0 = NULL;
	gtk_image_menu_item_set_image (item, (GtkWidget*) (_tmp0 = g_object_ref_sink ((GtkImage*) gtk_image_new_from_stock ("gtk-info", GTK_ICON_SIZE_MENU))));
	(_tmp0 == NULL) ? NULL : (_tmp0 = (g_object_unref (_tmp0), NULL));
	g_signal_connect_object ((GtkMenuItem*) item, "activate", (GCallback) _gmpc_now_playing_select_now_playing_browser_gtk_menu_item_activate, self, 0);
	gtk_widget_add_accelerator ((GtkWidget*) item, "activate", gtk_menu_get_accel_group (menu), (guint) 0x069, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
	gtk_menu_shell_append ((GtkMenuShell*) menu, (GtkWidget*) ((GtkMenuItem*) item));
	return (_tmp1 = 1, (item == NULL) ? NULL : (item = (g_object_unref (item), NULL)), _tmp1);
}


/**
 * Now playing uses the MetaDataBrowser plugin to "plot" the view */
GmpcNowPlaying* gmpc_now_playing_construct (GType object_type) {
	GmpcNowPlaying * self;
	self = g_object_newv (object_type, 0, NULL);
	return self;
}


GmpcNowPlaying* gmpc_now_playing_new (void) {
	return gmpc_now_playing_construct (GMPC_TYPE_NOW_PLAYING);
}


static void _gmpc_now_playing_status_changed_gmpc_connection_status_changed (GmpcConnection* _sender, MpdObj* server, ChangedStatusType what, gpointer self) {
	gmpc_now_playing_status_changed (self, _sender, server, what);
}


static GObject * gmpc_now_playing_constructor (GType type, guint n_construct_properties, GObjectConstructParam * construct_properties) {
	GObject * obj;
	GmpcNowPlayingClass * klass;
	GObjectClass * parent_class;
	GmpcNowPlaying * self;
	klass = GMPC_NOW_PLAYING_CLASS (g_type_class_peek (GMPC_TYPE_NOW_PLAYING));
	parent_class = G_OBJECT_CLASS (g_type_class_peek_parent (klass));
	obj = parent_class->constructor (type, n_construct_properties, construct_properties);
	self = GMPC_NOW_PLAYING (obj);
	{
		GmpcMetadataBrowser* _tmp0;
		/* Set the plugin as an internal one and of type pl_browser */
		((GmpcPluginBase*) self)->plugin_type = 2 | 8;
		/*    gmpcconn.connection_changed += con_changed;*/
		g_signal_connect_object (gmpcconn, "status-changed", (GCallback) _gmpc_now_playing_status_changed_gmpc_connection_status_changed, self, 0);
		_tmp0 = NULL;
		self->priv->browser = (_tmp0 = gmpc_metadata_browser_new (), (self->priv->browser == NULL) ? NULL : (self->priv->browser = (g_object_unref (self->priv->browser), NULL)), _tmp0);
	}
	return obj;
}


static void gmpc_now_playing_class_init (GmpcNowPlayingClass * klass) {
	gmpc_now_playing_parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (GmpcNowPlayingPrivate));
	G_OBJECT_CLASS (klass)->constructor = gmpc_now_playing_constructor;
	G_OBJECT_CLASS (klass)->finalize = gmpc_now_playing_finalize;
	GMPC_PLUGIN_BASE_CLASS (klass)->get_version = gmpc_now_playing_real_get_version;
	GMPC_PLUGIN_BASE_CLASS (klass)->get_name = gmpc_now_playing_real_get_name;
	GMPC_PLUGIN_BASE_CLASS (klass)->save_yourself = gmpc_now_playing_real_save_yourself;
}


static void gmpc_now_playing_gmpc_plugin_browser_iface_interface_init (GmpcPluginBrowserIfaceIface * iface) {
	gmpc_now_playing_gmpc_plugin_browser_iface_parent_iface = g_type_interface_peek_parent (iface);
	iface->browser_add = gmpc_now_playing_real_browser_add;
	iface->browser_selected = gmpc_now_playing_real_browser_selected;
	iface->browser_unselected = gmpc_now_playing_real_browser_unselected;
	iface->browser_add_go_menu = gmpc_now_playing_real_browser_add_go_menu;
}


static void gmpc_now_playing_instance_init (GmpcNowPlaying * self) {
	self->priv = GMPC_NOW_PLAYING_GET_PRIVATE (self);
	self->priv->np_ref = NULL;
	self->priv->browser = NULL;
	self->priv->paned = NULL;
	self->priv->container = NULL;
	self->priv->selected = FALSE;
	self->priv->song_checksum = NULL;
}


static void gmpc_now_playing_finalize (GObject* obj) {
	GmpcNowPlaying * self;
	self = GMPC_NOW_PLAYING (obj);
	(self->priv->np_ref == NULL) ? NULL : (self->priv->np_ref = (gtk_tree_row_reference_free (self->priv->np_ref), NULL));
	(self->priv->browser == NULL) ? NULL : (self->priv->browser = (g_object_unref (self->priv->browser), NULL));
	(self->priv->paned == NULL) ? NULL : (self->priv->paned = (g_object_unref (self->priv->paned), NULL));
	(self->priv->container == NULL) ? NULL : (self->priv->container = (g_object_unref (self->priv->container), NULL));
	self->priv->song_checksum = (g_free (self->priv->song_checksum), NULL);
	G_OBJECT_CLASS (gmpc_now_playing_parent_class)->finalize (obj);
}


GType gmpc_now_playing_get_type (void) {
	static GType gmpc_now_playing_type_id = 0;
	if (gmpc_now_playing_type_id == 0) {
		static const GTypeInfo g_define_type_info = { sizeof (GmpcNowPlayingClass), (GBaseInitFunc) NULL, (GBaseFinalizeFunc) NULL, (GClassInitFunc) gmpc_now_playing_class_init, (GClassFinalizeFunc) NULL, NULL, sizeof (GmpcNowPlaying), 0, (GInstanceInitFunc) gmpc_now_playing_instance_init, NULL };
		static const GInterfaceInfo gmpc_plugin_browser_iface_info = { (GInterfaceInitFunc) gmpc_now_playing_gmpc_plugin_browser_iface_interface_init, (GInterfaceFinalizeFunc) NULL, NULL};
		gmpc_now_playing_type_id = g_type_register_static (GMPC_PLUGIN_TYPE_BASE, "GmpcNowPlaying", &g_define_type_info, 0);
		g_type_add_interface_static (gmpc_now_playing_type_id, GMPC_PLUGIN_TYPE_BROWSER_IFACE, &gmpc_plugin_browser_iface_info);
	}
	return gmpc_now_playing_type_id;
}


static gint* gmpc_metadata_browser_real_get_version (GmpcPluginBase* base, int* result_length1) {
	GmpcMetadataBrowser * self;
	gint* _tmp0;
	self = (GmpcMetadataBrowser*) base;
	_tmp0 = NULL;
	return (_tmp0 = GMPC_METADATA_BROWSER_version, *result_length1 = G_N_ELEMENTS (GMPC_METADATA_BROWSER_version), _tmp0);
}


static const char* gmpc_metadata_browser_real_get_name (GmpcPluginBase* base) {
	GmpcMetadataBrowser * self;
	self = (GmpcMetadataBrowser*) base;
	return N_ ("Metadata Browser 2");
}


static void gmpc_metadata_browser_real_save_yourself (GmpcPluginBase* base) {
	GmpcMetadataBrowser * self;
	self = (GmpcMetadataBrowser*) base;
	if (self->priv->paned != NULL) {
		gint pos;
		pos = gtk_paned_get_position (self->priv->paned);
		cfg_set_single_value_as_int (config, gmpc_plugin_base_get_name ((GmpcPluginBase*) self), "pane-pos", pos);
	}
	if (self->priv->model_artist != NULL) {
		gmpc_mpddata_model_set_mpd_data (self->priv->model_artist, NULL);
	}
	if (self->priv->model_albums != NULL) {
		gmpc_mpddata_model_set_mpd_data (self->priv->model_albums, NULL);
	}
	if (self->priv->rref != NULL) {
		GtkTreePath* path;
		path = gtk_tree_row_reference_get_path (self->priv->rref);
		if (path != NULL) {
			gint* _tmp0;
			gint indices_size;
			gint indices_length1;
			gint* indices;
			_tmp0 = NULL;
			indices = (_tmp0 = gtk_tree_path_get_indices (path), indices_length1 = -1, indices_size = indices_length1, _tmp0);
			cfg_set_single_value_as_int (config, gmpc_plugin_base_get_name ((GmpcPluginBase*) self), "position", indices[0]);
		}
		(path == NULL) ? NULL : (path = (gtk_tree_path_free (path), NULL));
	}
}


/**
     * This builds the browser
     */
static void gmpc_metadata_browser_browser_bg_style_changed (GmpcMetadataBrowser* self, GtkScrolledWindow* bg, GtkStyle* style) {
	GdkColor _tmp0 = {0};
	g_return_if_fail (self != NULL);
	g_return_if_fail (bg != NULL);
	gtk_widget_modify_bg ((GtkWidget*) self->priv->metadata_box, GTK_STATE_NORMAL, (_tmp0 = gtk_widget_get_style ((GtkWidget*) self->priv->metadata_sw)->base[GTK_STATE_NORMAL], &_tmp0));
}


/* This hack makes clicking a selected row again, unselect it */
static gboolean gmpc_metadata_browser_browser_button_press_event (GmpcMetadataBrowser* self, GtkTreeView* tree, const GdkEventButton* event) {
	GtkTreePath* path;
	GtkTreePath* _tmp3;
	gboolean _tmp2;
	GtkTreePath* _tmp1;
	gboolean _tmp5;
	g_return_val_if_fail (self != NULL, FALSE);
	g_return_val_if_fail (tree != NULL, FALSE);
	path = NULL;
	if ((*event).button != 1) {
		gboolean _tmp0;
		return (_tmp0 = FALSE, (path == NULL) ? NULL : (path = (gtk_tree_path_free (path), NULL)), _tmp0);
	}
	_tmp3 = NULL;
	_tmp1 = NULL;
	if ((_tmp2 = gtk_tree_view_get_path_at_pos (tree, (gint) (*event).x, (gint) (*event).y, &_tmp1, NULL, NULL, NULL), path = (_tmp3 = _tmp1, (path == NULL) ? NULL : (path = (gtk_tree_path_free (path), NULL)), _tmp3), _tmp2)) {
		if (gtk_tree_selection_path_is_selected (gtk_tree_view_get_selection (tree), path)) {
			gboolean _tmp4;
			gtk_tree_selection_unselect_path (gtk_tree_view_get_selection (tree), path);
			return (_tmp4 = TRUE, (path == NULL) ? NULL : (path = (gtk_tree_path_free (path), NULL)), _tmp4);
		}
	}
	return (_tmp5 = FALSE, (path == NULL) ? NULL : (path = (gtk_tree_path_free (path), NULL)), _tmp5);
}


static gboolean gmpc_metadata_browser_visible_func_artist (GmpcMetadataBrowser* self, GtkTreeModel* model, GtkTreeIter* iter) {
	const char* _tmp0;
	char* text;
	char* str;
	gboolean visible;
	gboolean _tmp2;
	gboolean _tmp7;
	g_return_val_if_fail (self != NULL, FALSE);
	g_return_val_if_fail (model != NULL, FALSE);
	_tmp0 = NULL;
	text = (_tmp0 = gtk_entry_get_text (self->priv->artist_filter_entry), (_tmp0 == NULL) ? NULL : g_strdup (_tmp0));
	/* Visible if row is non-empty and first column is "HI" */
	str = NULL;
	visible = FALSE;
	if (g_utf8_get_char (g_utf8_offset_to_pointer (text, 0)) == '\0') {
		gboolean _tmp1;
		return (_tmp1 = TRUE, text = (g_free (text), NULL), str = (g_free (str), NULL), _tmp1);
	}
	gtk_tree_model_get (model, &(*iter), 7, &str, -1, -1);
	_tmp2 = FALSE;
	if (str != NULL) {
		char* _tmp6;
		char* _tmp5;
		char* _tmp4;
		char* _tmp3;
		_tmp6 = NULL;
		_tmp5 = NULL;
		_tmp4 = NULL;
		_tmp3 = NULL;
		_tmp2 = strstr (_tmp4 = g_utf8_normalize (_tmp3 = g_utf8_casefold (str, -1), -1, G_NORMALIZE_DEFAULT), _tmp6 = g_utf8_normalize (_tmp5 = g_utf8_casefold (text, -1), -1, G_NORMALIZE_DEFAULT)) != NULL;
		_tmp6 = (g_free (_tmp6), NULL);
		_tmp5 = (g_free (_tmp5), NULL);
		_tmp4 = (g_free (_tmp4), NULL);
		_tmp3 = (g_free (_tmp3), NULL);
	} else {
		_tmp2 = FALSE;
	}
	if (_tmp2) {
		visible = TRUE;
	}
	return (_tmp7 = visible, text = (g_free (text), NULL), str = (g_free (str), NULL), _tmp7);
}


static gboolean gmpc_metadata_browser_visible_func_album (GmpcMetadataBrowser* self, GtkTreeModel* model, GtkTreeIter* iter) {
	const char* _tmp0;
	char* text;
	char* str;
	gboolean visible;
	gboolean _tmp2;
	gboolean _tmp7;
	g_return_val_if_fail (self != NULL, FALSE);
	g_return_val_if_fail (model != NULL, FALSE);
	_tmp0 = NULL;
	text = (_tmp0 = gtk_entry_get_text (self->priv->album_filter_entry), (_tmp0 == NULL) ? NULL : g_strdup (_tmp0));
	/* Visible if row is non-empty and first column is "HI" */
	str = NULL;
	visible = FALSE;
	if (g_utf8_get_char (g_utf8_offset_to_pointer (text, 0)) == '\0') {
		gboolean _tmp1;
		return (_tmp1 = TRUE, text = (g_free (text), NULL), str = (g_free (str), NULL), _tmp1);
	}
	gtk_tree_model_get (model, &(*iter), 6, &str, -1, -1);
	_tmp2 = FALSE;
	if (str != NULL) {
		char* _tmp6;
		char* _tmp5;
		char* _tmp4;
		char* _tmp3;
		_tmp6 = NULL;
		_tmp5 = NULL;
		_tmp4 = NULL;
		_tmp3 = NULL;
		_tmp2 = strstr (_tmp4 = g_utf8_normalize (_tmp3 = g_utf8_casefold (str, -1), -1, G_NORMALIZE_DEFAULT), _tmp6 = g_utf8_normalize (_tmp5 = g_utf8_casefold (text, -1), -1, G_NORMALIZE_DEFAULT)) != NULL;
		_tmp6 = (g_free (_tmp6), NULL);
		_tmp5 = (g_free (_tmp5), NULL);
		_tmp4 = (g_free (_tmp4), NULL);
		_tmp3 = (g_free (_tmp3), NULL);
	} else {
		_tmp2 = FALSE;
	}
	if (_tmp2) {
		visible = TRUE;
	}
	return (_tmp7 = visible, text = (g_free (text), NULL), str = (g_free (str), NULL), _tmp7);
}


static gboolean gmpc_metadata_browser_browser_artist_key_press_event (GmpcMetadataBrowser* self, GtkTreeView* widget, const GdkEventKey* event) {
	gunichar uc;
	g_return_val_if_fail (self != NULL, FALSE);
	g_return_val_if_fail (widget != NULL, FALSE);
	uc = (gunichar) gdk_keyval_to_unicode ((*event).keyval);
	if (uc > 0) {
		char* outbuf;
		gint i;
		gboolean _tmp0;
		outbuf = g_strdup ("       ");
		i = g_unichar_to_utf8 (uc, outbuf);
		((gchar*) outbuf)[i] = '\0';
		gtk_entry_set_text (self->priv->artist_filter_entry, outbuf);
		gtk_widget_grab_focus ((GtkWidget*) self->priv->artist_filter_entry);
		gtk_editable_set_position ((GtkEditable*) self->priv->artist_filter_entry, 1);
		return (_tmp0 = TRUE, outbuf = (g_free (outbuf), NULL), _tmp0);
	}
	return FALSE;
}


static gboolean gmpc_metadata_browser_browser_album_key_press_event (GmpcMetadataBrowser* self, GtkTreeView* widget, const GdkEventKey* event) {
	gunichar uc;
	g_return_val_if_fail (self != NULL, FALSE);
	g_return_val_if_fail (widget != NULL, FALSE);
	uc = (gunichar) gdk_keyval_to_unicode ((*event).keyval);
	if (uc > 0) {
		char* outbuf;
		gint i;
		gboolean _tmp0;
		outbuf = g_strdup ("       ");
		i = g_unichar_to_utf8 (uc, outbuf);
		((gchar*) outbuf)[i] = '\0';
		gtk_entry_set_text (self->priv->album_filter_entry, outbuf);
		gtk_widget_grab_focus ((GtkWidget*) self->priv->album_filter_entry);
		gtk_editable_set_position ((GtkEditable*) self->priv->album_filter_entry, 1);
		return (_tmp0 = TRUE, outbuf = (g_free (outbuf), NULL), _tmp0);
	}
	return FALSE;
}


static void gmpc_metadata_browser_browser_artist_entry_changed (GmpcMetadataBrowser* self, GtkEntry* entry) {
	const char* _tmp0;
	char* text;
	g_return_if_fail (self != NULL);
	g_return_if_fail (entry != NULL);
	_tmp0 = NULL;
	text = (_tmp0 = gtk_entry_get_text (entry), (_tmp0 == NULL) ? NULL : g_strdup (_tmp0));
	if (strlen (text) > 0) {
		gtk_widget_show ((GtkWidget*) entry);
	} else {
		gtk_widget_hide ((GtkWidget*) entry);
		gtk_widget_grab_focus ((GtkWidget*) self->priv->tree_artist);
	}
	gtk_tree_model_filter_refilter (self->priv->model_filter_artist);
	text = (g_free (text), NULL);
}


static void gmpc_metadata_browser_browser_album_entry_changed (GmpcMetadataBrowser* self, GtkEntry* entry) {
	const char* _tmp0;
	char* text;
	g_return_if_fail (self != NULL);
	g_return_if_fail (entry != NULL);
	_tmp0 = NULL;
	text = (_tmp0 = gtk_entry_get_text (entry), (_tmp0 == NULL) ? NULL : g_strdup (_tmp0));
	if (strlen (text) > 0) {
		gtk_widget_show ((GtkWidget*) entry);
	} else {
		gtk_widget_hide ((GtkWidget*) entry);
		gtk_widget_grab_focus ((GtkWidget*) self->priv->tree_album);
	}
	gtk_tree_model_filter_refilter (self->priv->model_filter_album);
	text = (g_free (text), NULL);
}


static void _gmpc_metadata_browser_browser_artist_entry_changed_gtk_editable_changed (GtkEntry* _sender, gpointer self) {
	gmpc_metadata_browser_browser_artist_entry_changed (self, _sender);
}


static gboolean _gmpc_metadata_browser_visible_func_artist_gtk_tree_model_filter_visible_func (GtkTreeModel* model, GtkTreeIter* iter, gpointer self) {
	return gmpc_metadata_browser_visible_func_artist (self, model, iter);
}


static gboolean _gmpc_metadata_browser_browser_button_press_event_gtk_widget_button_press_event (GtkTreeView* _sender, const GdkEventButton* event, gpointer self) {
	return gmpc_metadata_browser_browser_button_press_event (self, _sender, event);
}


static gboolean _gmpc_metadata_browser_browser_artist_key_press_event_gtk_widget_key_press_event (GtkTreeView* _sender, const GdkEventKey* event, gpointer self) {
	return gmpc_metadata_browser_browser_artist_key_press_event (self, _sender, event);
}


static void _gmpc_metadata_browser_browser_artist_changed_gtk_tree_selection_changed (GtkTreeSelection* _sender, gpointer self) {
	gmpc_metadata_browser_browser_artist_changed (self, _sender);
}


static void _gmpc_metadata_browser_browser_album_entry_changed_gtk_editable_changed (GtkEntry* _sender, gpointer self) {
	gmpc_metadata_browser_browser_album_entry_changed (self, _sender);
}


static gboolean _gmpc_metadata_browser_visible_func_album_gtk_tree_model_filter_visible_func (GtkTreeModel* model, GtkTreeIter* iter, gpointer self) {
	return gmpc_metadata_browser_visible_func_album (self, model, iter);
}


static gboolean _gmpc_metadata_browser_browser_album_key_press_event_gtk_widget_key_press_event (GtkTreeView* _sender, const GdkEventKey* event, gpointer self) {
	return gmpc_metadata_browser_browser_album_key_press_event (self, _sender, event);
}


static void _gmpc_metadata_browser_browser_album_changed_gtk_tree_selection_changed (GtkTreeSelection* _sender, gpointer self) {
	gmpc_metadata_browser_browser_album_changed (self, _sender);
}


static void _gmpc_metadata_browser_browser_songs_changed_gtk_tree_selection_changed (GtkTreeSelection* _sender, gpointer self) {
	gmpc_metadata_browser_browser_songs_changed (self, _sender);
}


static void _gmpc_metadata_browser_browser_bg_style_changed_gtk_widget_style_set (GtkScrolledWindow* _sender, GtkStyle* previous_style, gpointer self) {
	gmpc_metadata_browser_browser_bg_style_changed (self, _sender, previous_style);
}


static void gmpc_metadata_browser_browser_init (GmpcMetadataBrowser* self) {
	g_return_if_fail (self != NULL);
	if (self->priv->paned == NULL) {
		GtkPaned* _tmp0;
		GtkBox* _tmp1;
		GtkVBox* box;
		GtkEntry* _tmp2;
		GtkScrolledWindow* sw;
		GmpcMpdDataModel* _tmp3;
		GtkTreeModelFilter* _tmp4;
		GtkTreeView* _tmp5;
		GtkTreeViewColumn* column;
		GtkCellRendererPixbuf* prenderer;
		GtkCellRendererText* trenderer;
		GtkVBox* _tmp6;
		GtkEntry* _tmp7;
		GtkScrolledWindow* _tmp8;
		GmpcMpdDataModel* _tmp9;
		GtkTreeModelFilter* _tmp10;
		GtkTreeView* _tmp11;
		GtkTreeViewColumn* _tmp12;
		GtkCellRendererPixbuf* _tmp13;
		GtkTreeViewColumn* _tmp14;
		GtkCellRendererText* _tmp15;
		GtkTreeViewColumn* _tmp16;
		GtkCellRendererText* _tmp17;
		GtkScrolledWindow* _tmp18;
		GmpcMpdDataModel* _tmp19;
		GtkTreeView* _tmp20;
		GtkTreeViewColumn* _tmp21;
		GtkCellRendererPixbuf* _tmp22;
		GtkCellRendererText* _tmp23;
		GtkTreeViewColumn* _tmp24;
		GtkCellRendererText* _tmp25;
		GtkScrolledWindow* _tmp26;
		GtkEventBox* _tmp27;
		_tmp0 = NULL;
		self->priv->paned = (_tmp0 = (GtkPaned*) g_object_ref_sink ((GtkHPaned*) gtk_hpaned_new ()), (self->priv->paned == NULL) ? NULL : (self->priv->paned = (g_object_unref (self->priv->paned), NULL)), _tmp0);
		gtk_paned_set_position (self->priv->paned, cfg_get_single_value_as_int_with_default (config, gmpc_plugin_base_get_name ((GmpcPluginBase*) self), "pane-pos", 150));
		/* Bow with browsers */
		_tmp1 = NULL;
		self->priv->browser_box = (_tmp1 = (GtkBox*) g_object_ref_sink ((GtkVBox*) gtk_vbox_new (TRUE, 6)), (self->priv->browser_box == NULL) ? NULL : (self->priv->browser_box = (g_object_unref (self->priv->browser_box), NULL)), _tmp1);
		gtk_paned_add1 (self->priv->paned, (GtkWidget*) self->priv->browser_box);
		/* Artist list  */
		box = g_object_ref_sink ((GtkVBox*) gtk_vbox_new (FALSE, 6));
		gtk_box_pack_start (self->priv->browser_box, (GtkWidget*) box, TRUE, TRUE, (guint) 0);
		_tmp2 = NULL;
		self->priv->artist_filter_entry = (_tmp2 = g_object_ref_sink ((GtkEntry*) gtk_entry_new ()), (self->priv->artist_filter_entry == NULL) ? NULL : (self->priv->artist_filter_entry = (g_object_unref (self->priv->artist_filter_entry), NULL)), _tmp2);
		gtk_widget_set_no_show_all ((GtkWidget*) self->priv->artist_filter_entry, TRUE);
		g_signal_connect_object ((GtkEditable*) self->priv->artist_filter_entry, "changed", (GCallback) _gmpc_metadata_browser_browser_artist_entry_changed_gtk_editable_changed, self, 0);
		gtk_box_pack_start ((GtkBox*) box, (GtkWidget*) self->priv->artist_filter_entry, FALSE, FALSE, (guint) 0);
		sw = g_object_ref_sink ((GtkScrolledWindow*) gtk_scrolled_window_new (NULL, NULL));
		gtk_scrolled_window_set_policy (sw, GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
		gtk_scrolled_window_set_shadow_type (sw, GTK_SHADOW_ETCHED_IN);
		gtk_box_pack_start ((GtkBox*) box, (GtkWidget*) sw, TRUE, TRUE, (guint) 0);
		_tmp3 = NULL;
		self->priv->model_artist = (_tmp3 = gmpc_mpddata_model_new (), (self->priv->model_artist == NULL) ? NULL : (self->priv->model_artist = (g_object_unref (self->priv->model_artist), NULL)), _tmp3);
		_tmp4 = NULL;
		self->priv->model_filter_artist = (_tmp4 = (GtkTreeModelFilter*) gtk_tree_model_filter_new ((GtkTreeModel*) self->priv->model_artist, NULL), (self->priv->model_filter_artist == NULL) ? NULL : (self->priv->model_filter_artist = (g_object_unref (self->priv->model_filter_artist), NULL)), _tmp4);
		gtk_tree_model_filter_set_visible_func (self->priv->model_filter_artist, _gmpc_metadata_browser_visible_func_artist_gtk_tree_model_filter_visible_func, g_object_ref (self), g_object_unref);
		_tmp5 = NULL;
		self->priv->tree_artist = (_tmp5 = g_object_ref_sink ((GtkTreeView*) gtk_tree_view_new_with_model ((GtkTreeModel*) self->priv->model_filter_artist)), (self->priv->tree_artist == NULL) ? NULL : (self->priv->tree_artist = (g_object_unref (self->priv->tree_artist), NULL)), _tmp5);
		g_signal_connect_object ((GtkWidget*) self->priv->tree_artist, "button-press-event", (GCallback) _gmpc_metadata_browser_browser_button_press_event_gtk_widget_button_press_event, self, 0);
		g_signal_connect_object ((GtkWidget*) self->priv->tree_artist, "key-press-event", (GCallback) _gmpc_metadata_browser_browser_artist_key_press_event_gtk_widget_key_press_event, self, 0);
		gtk_container_add ((GtkContainer*) sw, (GtkWidget*) self->priv->tree_artist);
		/* setup the columns */
		column = g_object_ref_sink (gtk_tree_view_column_new ());
		prenderer = g_object_ref_sink ((GtkCellRendererPixbuf*) gtk_cell_renderer_pixbuf_new ());
		g_object_set ((GObject*) prenderer, "height", self->priv->model_artist->icon_size, NULL);
		gtk_cell_layout_pack_start ((GtkCellLayout*) column, (GtkCellRenderer*) prenderer, FALSE);
		gtk_cell_layout_add_attribute ((GtkCellLayout*) column, (GtkCellRenderer*) prenderer, "pixbuf", 27);
		trenderer = g_object_ref_sink ((GtkCellRendererText*) gtk_cell_renderer_text_new ());
		gtk_cell_layout_pack_start ((GtkCellLayout*) column, (GtkCellRenderer*) trenderer, TRUE);
		gtk_cell_layout_add_attribute ((GtkCellLayout*) column, (GtkCellRenderer*) trenderer, "text", 7);
		gtk_tree_view_append_column (self->priv->tree_artist, column);
		gtk_tree_view_column_set_title (column, _ ("Artist"));
		g_signal_connect_object (gtk_tree_view_get_selection (self->priv->tree_artist), "changed", (GCallback) _gmpc_metadata_browser_browser_artist_changed_gtk_tree_selection_changed, self, 0);
		gtk_tree_view_set_search_column (self->priv->tree_artist, 7);
		/* set fixed height mode */
		gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
		gtk_tree_view_set_fixed_height_mode (self->priv->tree_artist, TRUE);
		/* Album list */
		_tmp6 = NULL;
		box = (_tmp6 = g_object_ref_sink ((GtkVBox*) gtk_vbox_new (FALSE, 6)), (box == NULL) ? NULL : (box = (g_object_unref (box), NULL)), _tmp6);
		gtk_box_pack_start (self->priv->browser_box, (GtkWidget*) box, TRUE, TRUE, (guint) 0);
		_tmp7 = NULL;
		self->priv->album_filter_entry = (_tmp7 = g_object_ref_sink ((GtkEntry*) gtk_entry_new ()), (self->priv->album_filter_entry == NULL) ? NULL : (self->priv->album_filter_entry = (g_object_unref (self->priv->album_filter_entry), NULL)), _tmp7);
		gtk_widget_set_no_show_all ((GtkWidget*) self->priv->album_filter_entry, TRUE);
		g_signal_connect_object ((GtkEditable*) self->priv->album_filter_entry, "changed", (GCallback) _gmpc_metadata_browser_browser_album_entry_changed_gtk_editable_changed, self, 0);
		gtk_box_pack_start ((GtkBox*) box, (GtkWidget*) self->priv->album_filter_entry, FALSE, FALSE, (guint) 0);
		_tmp8 = NULL;
		sw = (_tmp8 = g_object_ref_sink ((GtkScrolledWindow*) gtk_scrolled_window_new (NULL, NULL)), (sw == NULL) ? NULL : (sw = (g_object_unref (sw), NULL)), _tmp8);
		gtk_scrolled_window_set_policy (sw, GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
		gtk_scrolled_window_set_shadow_type (sw, GTK_SHADOW_ETCHED_IN);
		gtk_box_pack_start ((GtkBox*) box, (GtkWidget*) sw, TRUE, TRUE, (guint) 0);
		_tmp9 = NULL;
		self->priv->model_albums = (_tmp9 = gmpc_mpddata_model_new (), (self->priv->model_albums == NULL) ? NULL : (self->priv->model_albums = (g_object_unref (self->priv->model_albums), NULL)), _tmp9);
		_tmp10 = NULL;
		self->priv->model_filter_album = (_tmp10 = (GtkTreeModelFilter*) gtk_tree_model_filter_new ((GtkTreeModel*) self->priv->model_albums, NULL), (self->priv->model_filter_album == NULL) ? NULL : (self->priv->model_filter_album = (g_object_unref (self->priv->model_filter_album), NULL)), _tmp10);
		gtk_tree_model_filter_set_visible_func (self->priv->model_filter_album, _gmpc_metadata_browser_visible_func_album_gtk_tree_model_filter_visible_func, g_object_ref (self), g_object_unref);
		_tmp11 = NULL;
		self->priv->tree_album = (_tmp11 = g_object_ref_sink ((GtkTreeView*) gtk_tree_view_new_with_model ((GtkTreeModel*) self->priv->model_filter_album)), (self->priv->tree_album == NULL) ? NULL : (self->priv->tree_album = (g_object_unref (self->priv->tree_album), NULL)), _tmp11);
		g_signal_connect_object ((GtkWidget*) self->priv->tree_album, "button-press-event", (GCallback) _gmpc_metadata_browser_browser_button_press_event_gtk_widget_button_press_event, self, 0);
		g_signal_connect_object ((GtkWidget*) self->priv->tree_album, "key-press-event", (GCallback) _gmpc_metadata_browser_browser_album_key_press_event_gtk_widget_key_press_event, self, 0);
		gtk_container_add ((GtkContainer*) sw, (GtkWidget*) self->priv->tree_album);
		/* setup the columns */
		_tmp12 = NULL;
		column = (_tmp12 = g_object_ref_sink (gtk_tree_view_column_new ()), (column == NULL) ? NULL : (column = (g_object_unref (column), NULL)), _tmp12);
		_tmp13 = NULL;
		prenderer = (_tmp13 = g_object_ref_sink ((GtkCellRendererPixbuf*) gtk_cell_renderer_pixbuf_new ()), (prenderer == NULL) ? NULL : (prenderer = (g_object_unref (prenderer), NULL)), _tmp13);
		g_object_set ((GObject*) prenderer, "height", self->priv->model_albums->icon_size, NULL);
		gtk_cell_layout_pack_start ((GtkCellLayout*) column, (GtkCellRenderer*) prenderer, FALSE);
		gtk_cell_layout_add_attribute ((GtkCellLayout*) column, (GtkCellRenderer*) prenderer, "pixbuf", 27);
		gtk_tree_view_append_column (self->priv->tree_album, column);
		_tmp14 = NULL;
		column = (_tmp14 = g_object_ref_sink (gtk_tree_view_column_new ()), (column == NULL) ? NULL : (column = (g_object_unref (column), NULL)), _tmp14);
		_tmp15 = NULL;
		trenderer = (_tmp15 = g_object_ref_sink ((GtkCellRendererText*) gtk_cell_renderer_text_new ()), (trenderer == NULL) ? NULL : (trenderer = (g_object_unref (trenderer), NULL)), _tmp15);
		gtk_cell_layout_pack_start ((GtkCellLayout*) column, (GtkCellRenderer*) trenderer, TRUE);
		gtk_cell_layout_add_attribute ((GtkCellLayout*) column, (GtkCellRenderer*) trenderer, "text", 14);
		gtk_tree_view_append_column (self->priv->tree_album, column);
		gtk_tree_view_column_set_title (column, _ ("Year"));
		_tmp16 = NULL;
		column = (_tmp16 = g_object_ref_sink (gtk_tree_view_column_new ()), (column == NULL) ? NULL : (column = (g_object_unref (column), NULL)), _tmp16);
		_tmp17 = NULL;
		trenderer = (_tmp17 = g_object_ref_sink ((GtkCellRendererText*) gtk_cell_renderer_text_new ()), (trenderer == NULL) ? NULL : (trenderer = (g_object_unref (trenderer), NULL)), _tmp17);
		gtk_cell_layout_pack_start ((GtkCellLayout*) column, (GtkCellRenderer*) trenderer, TRUE);
		gtk_cell_layout_add_attribute ((GtkCellLayout*) column, (GtkCellRenderer*) trenderer, "text", 6);
		gtk_tree_view_append_column (self->priv->tree_album, column);
		gtk_tree_view_set_search_column (self->priv->tree_album, 6);
		gtk_tree_view_column_set_title (column, _ ("Album"));
		g_signal_connect_object (gtk_tree_view_get_selection (self->priv->tree_album), "changed", (GCallback) _gmpc_metadata_browser_browser_album_changed_gtk_tree_selection_changed, self, 0);
		/* Song list */
		_tmp18 = NULL;
		sw = (_tmp18 = g_object_ref_sink ((GtkScrolledWindow*) gtk_scrolled_window_new (NULL, NULL)), (sw == NULL) ? NULL : (sw = (g_object_unref (sw), NULL)), _tmp18);
		gtk_scrolled_window_set_policy (sw, GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
		gtk_scrolled_window_set_shadow_type (sw, GTK_SHADOW_ETCHED_IN);
		gtk_box_pack_start (self->priv->browser_box, (GtkWidget*) sw, TRUE, TRUE, (guint) 0);
		_tmp19 = NULL;
		self->priv->model_songs = (_tmp19 = gmpc_mpddata_model_new (), (self->priv->model_songs == NULL) ? NULL : (self->priv->model_songs = (g_object_unref (self->priv->model_songs), NULL)), _tmp19);
		_tmp20 = NULL;
		self->priv->tree_songs = (_tmp20 = g_object_ref_sink ((GtkTreeView*) gtk_tree_view_new_with_model ((GtkTreeModel*) self->priv->model_songs)), (self->priv->tree_songs == NULL) ? NULL : (self->priv->tree_songs = (g_object_unref (self->priv->tree_songs), NULL)), _tmp20);
		g_signal_connect_object ((GtkWidget*) self->priv->tree_songs, "button-press-event", (GCallback) _gmpc_metadata_browser_browser_button_press_event_gtk_widget_button_press_event, self, 0);
		gtk_container_add ((GtkContainer*) sw, (GtkWidget*) self->priv->tree_songs);
		/* setup the columns */
		_tmp21 = NULL;
		column = (_tmp21 = g_object_ref_sink (gtk_tree_view_column_new ()), (column == NULL) ? NULL : (column = (g_object_unref (column), NULL)), _tmp21);
		_tmp22 = NULL;
		prenderer = (_tmp22 = g_object_ref_sink ((GtkCellRendererPixbuf*) gtk_cell_renderer_pixbuf_new ()), (prenderer == NULL) ? NULL : (prenderer = (g_object_unref (prenderer), NULL)), _tmp22);
		gtk_cell_layout_pack_start ((GtkCellLayout*) column, (GtkCellRenderer*) prenderer, FALSE);
		gtk_cell_layout_add_attribute ((GtkCellLayout*) column, (GtkCellRenderer*) prenderer, "icon-name", 23);
		_tmp23 = NULL;
		trenderer = (_tmp23 = g_object_ref_sink ((GtkCellRendererText*) gtk_cell_renderer_text_new ()), (trenderer == NULL) ? NULL : (trenderer = (g_object_unref (trenderer), NULL)), _tmp23);
		gtk_cell_layout_pack_start ((GtkCellLayout*) column, (GtkCellRenderer*) trenderer, FALSE);
		gtk_cell_layout_add_attribute ((GtkCellLayout*) column, (GtkCellRenderer*) trenderer, "text", 10);
		gtk_tree_view_column_set_title (column, _ ("Track"));
		gtk_tree_view_append_column (self->priv->tree_songs, column);
		_tmp24 = NULL;
		column = (_tmp24 = g_object_ref_sink (gtk_tree_view_column_new ()), (column == NULL) ? NULL : (column = (g_object_unref (column), NULL)), _tmp24);
		_tmp25 = NULL;
		trenderer = (_tmp25 = g_object_ref_sink ((GtkCellRendererText*) gtk_cell_renderer_text_new ()), (trenderer == NULL) ? NULL : (trenderer = (g_object_unref (trenderer), NULL)), _tmp25);
		gtk_cell_layout_pack_start ((GtkCellLayout*) column, (GtkCellRenderer*) trenderer, TRUE);
		gtk_cell_layout_add_attribute ((GtkCellLayout*) column, (GtkCellRenderer*) trenderer, "text", 7);
		gtk_tree_view_append_column (self->priv->tree_songs, column);
		gtk_tree_view_set_search_column (self->priv->tree_songs, 7);
		gtk_tree_view_column_set_title (column, _ ("Songs"));
		g_signal_connect_object (gtk_tree_view_get_selection (self->priv->tree_songs), "changed", (GCallback) _gmpc_metadata_browser_browser_songs_changed_gtk_tree_selection_changed, self, 0);
		/* The right view */
		_tmp26 = NULL;
		self->priv->metadata_sw = (_tmp26 = g_object_ref_sink ((GtkScrolledWindow*) gtk_scrolled_window_new (NULL, NULL)), (self->priv->metadata_sw == NULL) ? NULL : (self->priv->metadata_sw = (g_object_unref (self->priv->metadata_sw), NULL)), _tmp26);
		gtk_scrolled_window_set_policy (self->priv->metadata_sw, GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
		g_signal_connect_object ((GtkWidget*) self->priv->metadata_sw, "style-set", (GCallback) _gmpc_metadata_browser_browser_bg_style_changed_gtk_widget_style_set, self, 0);
		_tmp27 = NULL;
		self->priv->metadata_box = (_tmp27 = g_object_ref_sink ((GtkEventBox*) gtk_event_box_new ()), (self->priv->metadata_box == NULL) ? NULL : (self->priv->metadata_box = (g_object_unref (self->priv->metadata_box), NULL)), _tmp27);
		gtk_event_box_set_visible_window (self->priv->metadata_box, TRUE);
		gtk_scrolled_window_add_with_viewport (self->priv->metadata_sw, (GtkWidget*) self->priv->metadata_box);
		gtk_paned_add2 (self->priv->paned, (GtkWidget*) self->priv->metadata_sw);
		gmpc_metadata_browser_reload_browsers (self);
		(box == NULL) ? NULL : (box = (g_object_unref (box), NULL));
		(sw == NULL) ? NULL : (sw = (g_object_unref (sw), NULL));
		(column == NULL) ? NULL : (column = (g_object_unref (column), NULL));
		(prenderer == NULL) ? NULL : (prenderer = (g_object_unref (prenderer), NULL));
		(trenderer == NULL) ? NULL : (trenderer = (g_object_unref (trenderer), NULL));
	}
	gtk_widget_show_all ((GtkWidget*) self->priv->paned);
}


static void gmpc_metadata_browser_reload_browsers (GmpcMetadataBrowser* self) {
	MpdData* data;
	MpdData* _tmp2;
	const MpdData* _tmp1;
	MpdData* _tmp0;
	MpdData* _tmp3;
	g_return_if_fail (self != NULL);
	if (self->priv->paned == NULL) {
		return;
	}
	gmpc_mpddata_model_set_mpd_data (self->priv->model_songs, NULL);
	gmpc_mpddata_model_set_mpd_data (self->priv->model_albums, NULL);
	gmpc_mpddata_model_set_mpd_data (self->priv->model_artist, NULL);
	gtk_entry_set_text (self->priv->artist_filter_entry, "");
	gtk_entry_set_text (self->priv->album_filter_entry, "");
	/* Fill in the first browser */
	mpd_database_search_field_start (connection, MPD_TAG_ITEM_ARTIST);
	data = mpd_database_search_commit (connection);
	_tmp2 = NULL;
	_tmp1 = NULL;
	_tmp0 = NULL;
	data = (_tmp2 = (_tmp1 = misc_sort_mpddata_by_album_disc_track ((_tmp0 = data, data = NULL, _tmp0)), (_tmp1 == NULL) ? NULL :  (_tmp1)), (data == NULL) ? NULL : (data = (mpd_data_free (data), NULL)), _tmp2);
	_tmp3 = NULL;
	gmpc_mpddata_model_set_mpd_data (self->priv->model_artist, (_tmp3 = data, data = NULL, _tmp3));
	(data == NULL) ? NULL : (data = (mpd_data_free (data), NULL));
}


static char* gmpc_metadata_browser_browser_get_selected_artist (GmpcMetadataBrowser* self) {
	GtkTreeIter iter = {0};
	GtkTreeSelection* _tmp0;
	GtkTreeSelection* sel;
	GtkTreeModel* model;
	GtkTreeModel* _tmp4;
	GtkTreeModel* _tmp3;
	gboolean _tmp2;
	GtkTreeModel* _tmp1;
	char* _tmp6;
	g_return_val_if_fail (self != NULL, NULL);
	_tmp0 = NULL;
	sel = (_tmp0 = gtk_tree_view_get_selection (self->priv->tree_artist), (_tmp0 == NULL) ? NULL : g_object_ref (_tmp0));
	model = NULL;
	/*this.model_artist;*/
	_tmp4 = NULL;
	_tmp3 = NULL;
	_tmp1 = NULL;
	if ((_tmp2 = gtk_tree_selection_get_selected (sel, &_tmp1, &iter), model = (_tmp3 = (_tmp4 = _tmp1, (_tmp4 == NULL) ? NULL : g_object_ref (_tmp4)), (model == NULL) ? NULL : (model = (g_object_unref (model), NULL)), _tmp3), _tmp2)) {
		char* artist;
		char* _tmp5;
		artist = NULL;
		gtk_tree_model_get (model, &iter, 7, &artist, -1, -1);
		_tmp5 = NULL;
		return (_tmp5 = artist, (sel == NULL) ? NULL : (sel = (g_object_unref (sel), NULL)), (model == NULL) ? NULL : (model = (g_object_unref (model), NULL)), _tmp5);
	}
	_tmp6 = NULL;
	return (_tmp6 = NULL, (sel == NULL) ? NULL : (sel = (g_object_unref (sel), NULL)), (model == NULL) ? NULL : (model = (g_object_unref (model), NULL)), _tmp6);
}


static char* gmpc_metadata_browser_browser_get_selected_album (GmpcMetadataBrowser* self) {
	GtkTreeIter iter = {0};
	GtkTreeSelection* _tmp0;
	GtkTreeSelection* sel;
	GtkTreeModel* model;
	GtkTreeModel* _tmp4;
	GtkTreeModel* _tmp3;
	gboolean _tmp2;
	GtkTreeModel* _tmp1;
	char* _tmp6;
	g_return_val_if_fail (self != NULL, NULL);
	_tmp0 = NULL;
	sel = (_tmp0 = gtk_tree_view_get_selection (self->priv->tree_album), (_tmp0 == NULL) ? NULL : g_object_ref (_tmp0));
	model = NULL;
	/*this.model_albums;*/
	_tmp4 = NULL;
	_tmp3 = NULL;
	_tmp1 = NULL;
	if ((_tmp2 = gtk_tree_selection_get_selected (sel, &_tmp1, &iter), model = (_tmp3 = (_tmp4 = _tmp1, (_tmp4 == NULL) ? NULL : g_object_ref (_tmp4)), (model == NULL) ? NULL : (model = (g_object_unref (model), NULL)), _tmp3), _tmp2)) {
		char* album;
		char* _tmp5;
		album = NULL;
		gtk_tree_model_get (model, &iter, 6, &album, -1, -1);
		_tmp5 = NULL;
		return (_tmp5 = album, (sel == NULL) ? NULL : (sel = (g_object_unref (sel), NULL)), (model == NULL) ? NULL : (model = (g_object_unref (model), NULL)), _tmp5);
	}
	_tmp6 = NULL;
	return (_tmp6 = NULL, (sel == NULL) ? NULL : (sel = (g_object_unref (sel), NULL)), (model == NULL) ? NULL : (model = (g_object_unref (model), NULL)), _tmp6);
}


static mpd_Song* gmpc_metadata_browser_browser_get_selected_song (GmpcMetadataBrowser* self) {
	GtkTreeIter iter = {0};
	GtkTreeSelection* _tmp0;
	GtkTreeSelection* sel;
	GmpcMpdDataModel* _tmp4;
	GmpcMpdDataModel* _tmp3;
	gboolean _tmp2;
	GtkTreeModel* _tmp1;
	mpd_Song* _tmp7;
	g_return_val_if_fail (self != NULL, NULL);
	_tmp0 = NULL;
	sel = (_tmp0 = gtk_tree_view_get_selection (self->priv->tree_songs), (_tmp0 == NULL) ? NULL : g_object_ref (_tmp0));
	_tmp4 = NULL;
	_tmp3 = NULL;
	_tmp1 = NULL;
	if ((_tmp2 = gtk_tree_selection_get_selected (sel, &_tmp1, &iter), self->priv->model_songs = (_tmp3 = (_tmp4 = (GmpcMpdDataModel*) _tmp1, (_tmp4 == NULL) ? NULL : g_object_ref (_tmp4)), (self->priv->model_songs == NULL) ? NULL : (self->priv->model_songs = (g_object_unref (self->priv->model_songs), NULL)), _tmp3), _tmp2)) {
		const mpd_Song* songs;
		const mpd_Song* _tmp5;
		mpd_Song* _tmp6;
		songs = NULL;
		gtk_tree_model_get ((GtkTreeModel*) self->priv->model_songs, &iter, 0, &songs, -1, -1);
		_tmp5 = NULL;
		_tmp6 = NULL;
		return (_tmp6 = (_tmp5 = songs, (_tmp5 == NULL) ? NULL : mpd_songDup (_tmp5)), (sel == NULL) ? NULL : (sel = (g_object_unref (sel), NULL)), _tmp6);
	}
	_tmp7 = NULL;
	return (_tmp7 = NULL, (sel == NULL) ? NULL : (sel = (g_object_unref (sel), NULL)), _tmp7);
}


static void gmpc_metadata_browser_browser_artist_changed (GmpcMetadataBrowser* self, GtkTreeSelection* sel) {
	char* artist;
	g_return_if_fail (self != NULL);
	g_return_if_fail (sel != NULL);
	gmpc_mpddata_model_set_mpd_data (self->priv->model_albums, NULL);
	gmpc_mpddata_model_set_mpd_data (self->priv->model_songs, NULL);
	gmpc_metadata_browser_metadata_box_clear (self);
	artist = gmpc_metadata_browser_browser_get_selected_artist (self);
	if (artist != NULL) {
		MpdData* data;
		MpdData* _tmp2;
		const MpdData* _tmp1;
		MpdData* _tmp0;
		MpdData* list;
		const MpdData* iter;
		MpdData* _tmp12;
		const MpdData* _tmp11;
		MpdData* _tmp10;
		MpdData* _tmp13;
		MpdData* _tmp14;
		MpdData* _tmp17;
		const MpdData* _tmp16;
		MpdData* _tmp15;
		MpdData* _tmp18;
		/* Fill in the first browser */
		mpd_database_search_field_start (connection, MPD_TAG_ITEM_ALBUM);
		mpd_database_search_add_constraint (connection, MPD_TAG_ITEM_ARTIST, artist);
		data = mpd_database_search_commit (connection);
		_tmp2 = NULL;
		_tmp1 = NULL;
		_tmp0 = NULL;
		data = (_tmp2 = (_tmp1 = misc_sort_mpddata_by_album_disc_track ((_tmp0 = data, data = NULL, _tmp0)), (_tmp1 == NULL) ? NULL :  (_tmp1)), (data == NULL) ? NULL : (data = (mpd_data_free (data), NULL)), _tmp2);
		gmpc_mpddata_model_set_request_artist (self->priv->model_albums, artist);
		list = NULL;
		iter = mpd_data_get_first (data);
		if (iter != NULL) {
			do {
				mpd_Song* _tmp3;
				char* _tmp5;
				const char* _tmp4;
				char* _tmp7;
				const char* _tmp6;
				MpdData* ydata;
				list = mpd_new_data_struct_append (list);
				list->type = MPD_DATA_TYPE_SONG;
				_tmp3 = NULL;
				list->song = (_tmp3 = mpd_newSong (), (list->song == NULL) ? NULL : (list->song = (mpd_freeSong (list->song), NULL)), _tmp3);
				_tmp5 = NULL;
				_tmp4 = NULL;
				list->song->artist = (_tmp5 = (_tmp4 = artist, (_tmp4 == NULL) ? NULL : g_strdup (_tmp4)), list->song->artist = (g_free (list->song->artist), NULL), _tmp5);
				_tmp7 = NULL;
				_tmp6 = NULL;
				list->song->album = (_tmp7 = (_tmp6 = iter->tag, (_tmp6 == NULL) ? NULL : g_strdup (_tmp6)), list->song->album = (g_free (list->song->album), NULL), _tmp7);
				mpd_database_search_field_start (connection, MPD_TAG_ITEM_DATE);
				mpd_database_search_add_constraint (connection, MPD_TAG_ITEM_ARTIST, artist);
				mpd_database_search_add_constraint (connection, MPD_TAG_ITEM_ALBUM, iter->tag);
				ydata = mpd_database_search_commit (connection);
				if (ydata != NULL) {
					char* _tmp9;
					const char* _tmp8;
					_tmp9 = NULL;
					_tmp8 = NULL;
					list->song->date = (_tmp9 = (_tmp8 = ydata->tag, (_tmp8 == NULL) ? NULL : g_strdup (_tmp8)), list->song->date = (g_free (list->song->date), NULL), _tmp9);
				}
				iter = mpd_data_get_next_real (iter, FALSE);
				(ydata == NULL) ? NULL : (ydata = (mpd_data_free (ydata), NULL));
			} while (iter != NULL);
		}
		_tmp12 = NULL;
		_tmp11 = NULL;
		_tmp10 = NULL;
		list = (_tmp12 = (_tmp11 = misc_sort_mpddata_by_album_disc_track ((_tmp10 = list, list = NULL, _tmp10)), (_tmp11 == NULL) ? NULL :  (_tmp11)), (list == NULL) ? NULL : (list = (mpd_data_free (list), NULL)), _tmp12);
		_tmp13 = NULL;
		gmpc_mpddata_model_set_mpd_data (self->priv->model_albums, (_tmp13 = list, list = NULL, _tmp13));
		mpd_database_search_start (connection, TRUE);
		mpd_database_search_add_constraint (connection, MPD_TAG_ITEM_ARTIST, artist);
		_tmp14 = NULL;
		data = (_tmp14 = mpd_database_search_commit (connection), (data == NULL) ? NULL : (data = (mpd_data_free (data), NULL)), _tmp14);
		_tmp17 = NULL;
		_tmp16 = NULL;
		_tmp15 = NULL;
		data = (_tmp17 = (_tmp16 = misc_sort_mpddata_by_album_disc_track ((_tmp15 = data, data = NULL, _tmp15)), (_tmp16 == NULL) ? NULL :  (_tmp16)), (data == NULL) ? NULL : (data = (mpd_data_free (data), NULL)), _tmp17);
		_tmp18 = NULL;
		gmpc_mpddata_model_set_mpd_data (self->priv->model_songs, (_tmp18 = data, data = NULL, _tmp18));
		(data == NULL) ? NULL : (data = (mpd_data_free (data), NULL));
		(list == NULL) ? NULL : (list = (mpd_data_free (list), NULL));
	}
	gmpc_metadata_browser_metadata_box_update (self);
	artist = (g_free (artist), NULL);
}


static void gmpc_metadata_browser_browser_album_changed (GmpcMetadataBrowser* self, GtkTreeSelection* album_sel) {
	char* artist;
	g_return_if_fail (self != NULL);
	g_return_if_fail (album_sel != NULL);
	gmpc_mpddata_model_set_mpd_data (self->priv->model_songs, NULL);
	gmpc_metadata_browser_metadata_box_clear (self);
	artist = gmpc_metadata_browser_browser_get_selected_artist (self);
	if (artist != NULL) {
		char* album;
		char* albumartist;
		MpdData* data;
		MpdData* _tmp4;
		const MpdData* _tmp3;
		MpdData* _tmp2;
		MpdData* _tmp5;
		album = gmpc_metadata_browser_browser_get_selected_album (self);
		albumartist = NULL;
		if (album != NULL) {
			MpdData* ydata;
			mpd_database_search_field_start (connection, MPD_TAG_ITEM_ALBUM_ARTIST);
			mpd_database_search_add_constraint (connection, MPD_TAG_ITEM_ALBUM, album);
			ydata = mpd_database_search_commit (connection);
			if (ydata != NULL) {
				if (string_get_length (ydata->tag) > 0) {
					char* _tmp1;
					const char* _tmp0;
					_tmp1 = NULL;
					_tmp0 = NULL;
					albumartist = (_tmp1 = (_tmp0 = ydata->tag, (_tmp0 == NULL) ? NULL : g_strdup (_tmp0)), albumartist = (g_free (albumartist), NULL), _tmp1);
				}
			}
			(ydata == NULL) ? NULL : (ydata = (mpd_data_free (ydata), NULL));
		}
		/* Fill in the first browser */
		mpd_database_search_start (connection, TRUE);
		if (albumartist != NULL) {
			mpd_database_search_add_constraint (connection, MPD_TAG_ITEM_ALBUM_ARTIST, albumartist);
		} else {
			mpd_database_search_add_constraint (connection, MPD_TAG_ITEM_ARTIST, artist);
		}
		if (album != NULL) {
			mpd_database_search_add_constraint (connection, MPD_TAG_ITEM_ALBUM, album);
		}
		data = mpd_database_search_commit (connection);
		_tmp4 = NULL;
		_tmp3 = NULL;
		_tmp2 = NULL;
		data = (_tmp4 = (_tmp3 = misc_sort_mpddata_by_album_disc_track ((_tmp2 = data, data = NULL, _tmp2)), (_tmp3 == NULL) ? NULL :  (_tmp3)), (data == NULL) ? NULL : (data = (mpd_data_free (data), NULL)), _tmp4);
		_tmp5 = NULL;
		gmpc_mpddata_model_set_mpd_data (self->priv->model_songs, (_tmp5 = data, data = NULL, _tmp5));
		album = (g_free (album), NULL);
		albumartist = (g_free (albumartist), NULL);
		(data == NULL) ? NULL : (data = (mpd_data_free (data), NULL));
	}
	gmpc_metadata_browser_metadata_box_update (self);
	artist = (g_free (artist), NULL);
}


static void gmpc_metadata_browser_browser_songs_changed (GmpcMetadataBrowser* self, GtkTreeSelection* song_sel) {
	g_return_if_fail (self != NULL);
	g_return_if_fail (song_sel != NULL);
	gmpc_metadata_browser_metadata_box_clear (self);
	gmpc_metadata_browser_metadata_box_update (self);
}


/** 
     * Metadata box
     */
static void gmpc_metadata_browser_play_song (GmpcMetadataBrowser* self, GtkButton* button) {
	const mpd_Song* _tmp0;
	mpd_Song* song;
	g_return_if_fail (self != NULL);
	g_return_if_fail (button != NULL);
	_tmp0 = NULL;
	song = (_tmp0 = (const mpd_Song*) g_object_get_data ((GObject*) button, "song"), (_tmp0 == NULL) ? NULL : mpd_songDup (_tmp0));
	if (song != NULL) {
		play_path (song->file);
	}
	(song == NULL) ? NULL : (song = (mpd_freeSong (song), NULL));
}


static void gmpc_metadata_browser_add_song (GmpcMetadataBrowser* self, GtkButton* button) {
	const mpd_Song* _tmp0;
	mpd_Song* song;
	g_return_if_fail (self != NULL);
	g_return_if_fail (button != NULL);
	_tmp0 = NULL;
	song = (_tmp0 = (const mpd_Song*) g_object_get_data ((GObject*) button, "song"), (_tmp0 == NULL) ? NULL : mpd_songDup (_tmp0));
	if (song != NULL) {
		mpd_playlist_add (connection, song->file);
		(song == NULL) ? NULL : (song = (mpd_freeSong (song), NULL));
		return;
	}
	(song == NULL) ? NULL : (song = (mpd_freeSong (song), NULL));
}


static void gmpc_metadata_browser_replace_song (GmpcMetadataBrowser* self, GtkButton* button) {
	const mpd_Song* _tmp0;
	mpd_Song* song;
	g_return_if_fail (self != NULL);
	g_return_if_fail (button != NULL);
	_tmp0 = NULL;
	song = (_tmp0 = (const mpd_Song*) g_object_get_data ((GObject*) button, "song"), (_tmp0 == NULL) ? NULL : mpd_songDup (_tmp0));
	if (song != NULL) {
		mpd_playlist_clear (connection);
		mpd_playlist_add (connection, song->file);
		mpd_player_play (connection);
		(song == NULL) ? NULL : (song = (mpd_freeSong (song), NULL));
		return;
	}
	(song == NULL) ? NULL : (song = (mpd_freeSong (song), NULL));
}


static void gmpc_metadata_browser_add_selected_song (GmpcMetadataBrowser* self, GtkButton* button) {
	char* artist;
	char* album;
	mpd_Song* song;
	g_return_if_fail (self != NULL);
	g_return_if_fail (button != NULL);
	artist = gmpc_metadata_browser_browser_get_selected_artist (self);
	album = gmpc_metadata_browser_browser_get_selected_album (self);
	song = gmpc_metadata_browser_browser_get_selected_song (self);
	if (song != NULL) {
		mpd_playlist_add (connection, song->file);
		artist = (g_free (artist), NULL);
		album = (g_free (album), NULL);
		(song == NULL) ? NULL : (song = (mpd_freeSong (song), NULL));
		return;
	}
	if (artist != NULL) {
		char* albumartist;
		MpdData* data;
		/* Hunt albumartist */
		albumartist = NULL;
		if (album != NULL) {
			MpdData* ydata;
			mpd_database_search_field_start (connection, MPD_TAG_ITEM_ALBUM_ARTIST);
			mpd_database_search_add_constraint (connection, MPD_TAG_ITEM_ALBUM, album);
			ydata = mpd_database_search_commit (connection);
			if (ydata != NULL) {
				if (string_get_length (ydata->tag) > 0) {
					char* _tmp1;
					const char* _tmp0;
					_tmp1 = NULL;
					_tmp0 = NULL;
					albumartist = (_tmp1 = (_tmp0 = ydata->tag, (_tmp0 == NULL) ? NULL : g_strdup (_tmp0)), albumartist = (g_free (albumartist), NULL), _tmp1);
				}
			}
			(ydata == NULL) ? NULL : (ydata = (mpd_data_free (ydata), NULL));
		}
		mpd_database_search_field_start (connection, MPD_TAG_ITEM_FILENAME);
		if (albumartist != NULL) {
			mpd_database_search_add_constraint (connection, MPD_TAG_ITEM_ALBUM_ARTIST, albumartist);
		} else {
			mpd_database_search_add_constraint (connection, MPD_TAG_ITEM_ARTIST, artist);
		}
		if (album != NULL) {
			mpd_database_search_add_constraint (connection, MPD_TAG_ITEM_ALBUM, album);
		}
		data = mpd_database_search_commit (connection);
		if (data != NULL) {
			const MpdData* _tmp2;
			const MpdData* iter;
			_tmp2 = NULL;
			iter = misc_sort_mpddata_by_album_disc_track ((_tmp2 = data, (_tmp2 == NULL) ? NULL :  (_tmp2)));
			do {
				mpd_playlist_queue_add (connection, iter->tag);
			} while ((iter = mpd_data_get_next_real (iter, FALSE)) != NULL);
			mpd_playlist_queue_commit (connection);
		}
		albumartist = (g_free (albumartist), NULL);
		(data == NULL) ? NULL : (data = (mpd_data_free (data), NULL));
	}
	artist = (g_free (artist), NULL);
	album = (g_free (album), NULL);
	(song == NULL) ? NULL : (song = (mpd_freeSong (song), NULL));
}


static void gmpc_metadata_browser_replace_selected_song (GmpcMetadataBrowser* self, GtkButton* button) {
	g_return_if_fail (self != NULL);
	g_return_if_fail (button != NULL);
	mpd_playlist_clear (connection);
	gmpc_metadata_browser_add_selected_song (self, button);
	mpd_player_play (connection);
}


static void gmpc_metadata_browser_metadata_box_clear (GmpcMetadataBrowser* self) {
	GList* list;
	g_return_if_fail (self != NULL);
	list = gtk_container_get_children ((GtkContainer*) self->priv->metadata_box);
	{
		GList* child_collection;
		GList* child_it;
		child_collection = list;
		for (child_it = child_collection; child_it != NULL; child_it = child_it->next) {
			GtkWidget* _tmp0;
			GtkWidget* child;
			_tmp0 = NULL;
			child = (_tmp0 = (GtkWidget*) child_it->data, (_tmp0 == NULL) ? NULL : g_object_ref (_tmp0));
			{
				gtk_object_destroy ((GtkObject*) child);
				(child == NULL) ? NULL : (child = (g_object_unref (child), NULL));
			}
		}
	}
	(list == NULL) ? NULL : (list = (g_list_free (list), NULL));
}


static void _gmpc_metadata_browser_metadata_find_query_gtk_button_clicked (GtkButton* _sender, gpointer self) {
	gmpc_metadata_browser_metadata_find_query (self, _sender);
}


static void _gmpc_metadata_browser_metadata_button_open_file_browser_path_gtk_button_clicked (GtkButton* _sender, gpointer self) {
	gmpc_metadata_browser_metadata_button_open_file_browser_path (self, _sender);
}


static void _gmpc_metadata_browser_play_song_gtk_button_clicked (GtkButton* _sender, gpointer self) {
	gmpc_metadata_browser_play_song (self, _sender);
}


static void _gmpc_metadata_browser_add_song_gtk_button_clicked (GtkButton* _sender, gpointer self) {
	gmpc_metadata_browser_add_song (self, _sender);
}


static void _gmpc_metadata_browser_replace_song_gtk_button_clicked (GtkButton* _sender, gpointer self) {
	gmpc_metadata_browser_replace_song (self, _sender);
}


GtkWidget* gmpc_metadata_browser_metadata_box_show_song (GmpcMetadataBrowser* self, const mpd_Song* song) {
	GtkVBox* vbox;
	GtkLabel* label;
	GtkHBox* hbox;
	GtkAlignment* ali;
	GmpcMetaImage* artist_image;
	GtkTable* info_box;
	gint i;
	GtkLabel* pt_label;
	GtkLabel* _tmp6;
	GtkLabel* _tmp7;
	char* _tmp8;
	GtkLabel* _tmp12;
	GtkLabel* _tmp13;
	char* _tmp14;
	GmpcFavoritesButton* fav_button;
	GtkLabel* _tmp34;
	char* _tmp35;
	GtkAlignment* _tmp36;
	GtkButton* button;
	GtkHBox* _tmp43;
	GtkButton* _tmp44;
	GtkButton* _tmp45;
	GtkImage* _tmp46;
	GmpcMetaTextView* text_view;
	char* _tmp47;
	GmpcWidgetMore* _tmp48;
	GmpcWidgetMore* frame;
	GmpcMetaTextView* _tmp49;
	GmpcWidgetMore* _tmp51;
	char* _tmp50;
	GmpcWidgetSimilarSongs* similar_songs;
	GmpcSongLinks* song_links;
	GtkWidget* _tmp52;
	g_return_val_if_fail (self != NULL, NULL);
	g_return_val_if_fail (song != NULL, NULL);
	vbox = g_object_ref_sink ((GtkVBox*) gtk_vbox_new (FALSE, 6));
	gtk_container_set_border_width ((GtkContainer*) vbox, (guint) 8);
	label = g_object_ref_sink ((GtkLabel*) gtk_label_new (""));
	if (song->title != NULL) {
		char* _tmp0;
		_tmp0 = NULL;
		gtk_label_set_markup (label, _tmp0 = g_markup_printf_escaped ("<span size='xx-large' weight='bold'>%s</span>", song->title));
		_tmp0 = (g_free (_tmp0), NULL);
	} else {
		char* _tmp1;
		_tmp1 = NULL;
		gtk_label_set_markup (label, _tmp1 = g_markup_printf_escaped ("<span size='xx-large' weight='bold'>%s</span>", _ ("Unknown")));
		_tmp1 = (g_free (_tmp1), NULL);
	}
	gtk_misc_set_alignment ((GtkMisc*) label, 0.0f, 0.5f);
	gtk_box_pack_start ((GtkBox*) vbox, (GtkWidget*) label, FALSE, FALSE, (guint) 0);
	/* Artist image */
	hbox = g_object_ref_sink ((GtkHBox*) gtk_hbox_new (FALSE, 6));
	ali = g_object_ref_sink ((GtkAlignment*) gtk_alignment_new (0.f, 0.f, 0.f, 0.f));
	artist_image = g_object_ref_sink (gmpc_metaimage_new_size (META_ALBUM_ART, 250));
	gmpc_metaimage_set_squared (artist_image, FALSE);
	gmpc_metaimage_update_cover_from_song (artist_image, song);
	gtk_container_add ((GtkContainer*) ali, (GtkWidget*) artist_image);
	gtk_box_pack_start ((GtkBox*) hbox, (GtkWidget*) ali, FALSE, FALSE, (guint) 0);
	/* Artist information */
	info_box = g_object_ref_sink ((GtkTable*) gtk_table_new ((guint) 4, (guint) 2, FALSE));
	gtk_table_set_row_spacings (info_box, (guint) 3);
	gtk_table_set_col_spacings (info_box, (guint) 8);
	gtk_box_pack_start ((GtkBox*) hbox, (GtkWidget*) info_box, FALSE, FALSE, (guint) 0);
	i = 0;
	pt_label = NULL;
	if (song->title != NULL) {
		GtkLabel* _tmp2;
		char* _tmp3;
		GtkHBox* dhbox;
		GtkLabel* _tmp4;
		GtkButton* button;
		GtkImage* _tmp5;
		_tmp2 = NULL;
		label = (_tmp2 = g_object_ref_sink ((GtkLabel*) gtk_label_new ("")), (label == NULL) ? NULL : (label = (g_object_unref (label), NULL)), _tmp2);
		gtk_misc_set_alignment ((GtkMisc*) label, 0.0f, 0.5f);
		_tmp3 = NULL;
		gtk_label_set_markup (label, _tmp3 = g_markup_printf_escaped ("<b>%s:</b>", _ ("Title")));
		_tmp3 = (g_free (_tmp3), NULL);
		gtk_table_attach (info_box, (GtkWidget*) label, (guint) 0, (guint) 1, (guint) i, (guint) (i + 1), GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, (guint) 0, (guint) 0);
		dhbox = g_object_ref_sink ((GtkHBox*) gtk_hbox_new (FALSE, 6));
		_tmp4 = NULL;
		pt_label = (_tmp4 = g_object_ref_sink ((GtkLabel*) gtk_label_new (song->title)), (pt_label == NULL) ? NULL : (pt_label = (g_object_unref (pt_label), NULL)), _tmp4);
		gtk_misc_set_alignment ((GtkMisc*) pt_label, 0.0f, 0.5f);
		gtk_label_set_line_wrap (pt_label, TRUE);
		gtk_box_pack_start ((GtkBox*) dhbox, (GtkWidget*) pt_label, FALSE, FALSE, (guint) 0);
		/* Button to search for song with same title */
		button = g_object_ref_sink ((GtkButton*) gtk_button_new ());
		_tmp5 = NULL;
		gtk_container_add ((GtkContainer*) button, (GtkWidget*) (_tmp5 = g_object_ref_sink ((GtkImage*) gtk_image_new_from_stock ("gtk-find", GTK_ICON_SIZE_MENU))));
		(_tmp5 == NULL) ? NULL : (_tmp5 = (g_object_unref (_tmp5), NULL));
		gtk_button_set_relief (button, GTK_RELIEF_NONE);
		g_object_set_data_full ((GObject*) button, "query", (void*) g_strdup_printf ("title=(%s)", song->title), (GDestroyNotify) g_free);
		g_signal_connect_object (button, "clicked", (GCallback) _gmpc_metadata_browser_metadata_find_query_gtk_button_clicked, self, 0);
		gtk_widget_set_tooltip_text ((GtkWidget*) button, _ ("Search songs with similar title"));
		gtk_box_pack_start ((GtkBox*) dhbox, (GtkWidget*) button, FALSE, FALSE, (guint) 0);
		gtk_table_attach (info_box, (GtkWidget*) dhbox, (guint) 1, (guint) 2, (guint) i, (guint) (i + 1), GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, (guint) 0, (guint) 0);
		i++;
		(dhbox == NULL) ? NULL : (dhbox = (g_object_unref (dhbox), NULL));
		(button == NULL) ? NULL : (button = (g_object_unref (button), NULL));
	}
	/* Artist label */
	_tmp6 = NULL;
	pt_label = (_tmp6 = g_object_ref_sink ((GtkLabel*) gtk_label_new (song->artist)), (pt_label == NULL) ? NULL : (pt_label = (g_object_unref (pt_label), NULL)), _tmp6);
	_tmp7 = NULL;
	label = (_tmp7 = g_object_ref_sink ((GtkLabel*) gtk_label_new ("")), (label == NULL) ? NULL : (label = (g_object_unref (label), NULL)), _tmp7);
	gtk_misc_set_alignment ((GtkMisc*) label, 0.0f, 0.5f);
	_tmp8 = NULL;
	gtk_label_set_markup (label, _tmp8 = g_markup_printf_escaped ("<b>%s:</b>", _ ("Artist")));
	_tmp8 = (g_free (_tmp8), NULL);
	gtk_table_attach (info_box, (GtkWidget*) label, (guint) 0, (guint) 1, (guint) i, (guint) (i + 1), GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, (guint) 0, (guint) 0);
	gtk_misc_set_alignment ((GtkMisc*) pt_label, 0.0f, 0.5f);
	gtk_label_set_line_wrap (pt_label, TRUE);
	gtk_table_attach (info_box, (GtkWidget*) pt_label, (guint) 1, (guint) 2, (guint) i, (guint) (i + 1), GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, (guint) 0, (guint) 0);
	i++;
	/* AlbumArtist label */
	if (song->albumartist != NULL) {
		GtkLabel* _tmp9;
		GtkLabel* _tmp10;
		char* _tmp11;
		_tmp9 = NULL;
		pt_label = (_tmp9 = g_object_ref_sink ((GtkLabel*) gtk_label_new (song->albumartist)), (pt_label == NULL) ? NULL : (pt_label = (g_object_unref (pt_label), NULL)), _tmp9);
		_tmp10 = NULL;
		label = (_tmp10 = g_object_ref_sink ((GtkLabel*) gtk_label_new ("")), (label == NULL) ? NULL : (label = (g_object_unref (label), NULL)), _tmp10);
		gtk_misc_set_alignment ((GtkMisc*) label, 0.0f, 0.5f);
		_tmp11 = NULL;
		gtk_label_set_markup (label, _tmp11 = g_markup_printf_escaped ("<b>%s:</b>", _ ("Album artist")));
		_tmp11 = (g_free (_tmp11), NULL);
		gtk_table_attach (info_box, (GtkWidget*) label, (guint) 0, (guint) 1, (guint) i, (guint) (i + 1), GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, (guint) 0, (guint) 0);
		gtk_misc_set_alignment ((GtkMisc*) pt_label, 0.0f, 0.5f);
		gtk_label_set_line_wrap (pt_label, TRUE);
		gtk_table_attach (info_box, (GtkWidget*) pt_label, (guint) 1, (guint) 2, (guint) i, (guint) (i + 1), GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, (guint) 0, (guint) 0);
		i++;
	}
	/* Album */
	_tmp12 = NULL;
	pt_label = (_tmp12 = g_object_ref_sink ((GtkLabel*) gtk_label_new (song->album)), (pt_label == NULL) ? NULL : (pt_label = (g_object_unref (pt_label), NULL)), _tmp12);
	_tmp13 = NULL;
	label = (_tmp13 = g_object_ref_sink ((GtkLabel*) gtk_label_new ("")), (label == NULL) ? NULL : (label = (g_object_unref (label), NULL)), _tmp13);
	gtk_misc_set_alignment ((GtkMisc*) label, 0.0f, 0.5f);
	_tmp14 = NULL;
	gtk_label_set_markup (label, _tmp14 = g_markup_printf_escaped ("<b>%s:</b>", _ ("Album")));
	_tmp14 = (g_free (_tmp14), NULL);
	gtk_table_attach (info_box, (GtkWidget*) label, (guint) 0, (guint) 1, (guint) i, (guint) (i + 1), GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, (guint) 0, (guint) 0);
	gtk_misc_set_alignment ((GtkMisc*) pt_label, 0.0f, 0.5f);
	gtk_label_set_line_wrap (pt_label, TRUE);
	gtk_table_attach (info_box, (GtkWidget*) pt_label, (guint) 1, (guint) 2, (guint) i, (guint) (i + 1), GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, (guint) 0, (guint) 0);
	i++;
	/* track */
	if (song->track != NULL) {
		GtkLabel* _tmp15;
		GtkLabel* _tmp16;
		char* _tmp17;
		_tmp15 = NULL;
		pt_label = (_tmp15 = g_object_ref_sink ((GtkLabel*) gtk_label_new (song->track)), (pt_label == NULL) ? NULL : (pt_label = (g_object_unref (pt_label), NULL)), _tmp15);
		_tmp16 = NULL;
		label = (_tmp16 = g_object_ref_sink ((GtkLabel*) gtk_label_new ("")), (label == NULL) ? NULL : (label = (g_object_unref (label), NULL)), _tmp16);
		gtk_misc_set_alignment ((GtkMisc*) label, 0.0f, 0.5f);
		_tmp17 = NULL;
		gtk_label_set_markup (label, _tmp17 = g_markup_printf_escaped ("<b>%s:</b>", _ ("Track")));
		_tmp17 = (g_free (_tmp17), NULL);
		gtk_table_attach (info_box, (GtkWidget*) label, (guint) 0, (guint) 1, (guint) i, (guint) (i + 1), GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, (guint) 0, (guint) 0);
		gtk_misc_set_alignment ((GtkMisc*) pt_label, 0.0f, 0.5f);
		gtk_label_set_line_wrap (pt_label, TRUE);
		gtk_table_attach (info_box, (GtkWidget*) pt_label, (guint) 1, (guint) 2, (guint) i, (guint) (i + 1), GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, (guint) 0, (guint) 0);
		i++;
	}
	/* date */
	if (song->date != NULL) {
		GtkLabel* _tmp18;
		GtkLabel* _tmp19;
		char* _tmp20;
		_tmp18 = NULL;
		pt_label = (_tmp18 = g_object_ref_sink ((GtkLabel*) gtk_label_new (song->date)), (pt_label == NULL) ? NULL : (pt_label = (g_object_unref (pt_label), NULL)), _tmp18);
		_tmp19 = NULL;
		label = (_tmp19 = g_object_ref_sink ((GtkLabel*) gtk_label_new ("")), (label == NULL) ? NULL : (label = (g_object_unref (label), NULL)), _tmp19);
		gtk_misc_set_alignment ((GtkMisc*) label, 0.0f, 0.5f);
		_tmp20 = NULL;
		gtk_label_set_markup (label, _tmp20 = g_markup_printf_escaped ("<b>%s:</b>", _ ("Date")));
		_tmp20 = (g_free (_tmp20), NULL);
		gtk_table_attach (info_box, (GtkWidget*) label, (guint) 0, (guint) 1, (guint) i, (guint) (i + 1), GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, (guint) 0, (guint) 0);
		gtk_misc_set_alignment ((GtkMisc*) pt_label, 0.0f, 0.5f);
		gtk_label_set_line_wrap (pt_label, TRUE);
		gtk_table_attach (info_box, (GtkWidget*) pt_label, (guint) 1, (guint) 2, (guint) i, (guint) (i + 1), GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, (guint) 0, (guint) 0);
		i++;
	}
	/* performer */
	if (song->performer != NULL) {
		GtkLabel* _tmp21;
		GtkLabel* _tmp22;
		char* _tmp23;
		_tmp21 = NULL;
		pt_label = (_tmp21 = g_object_ref_sink ((GtkLabel*) gtk_label_new (song->performer)), (pt_label == NULL) ? NULL : (pt_label = (g_object_unref (pt_label), NULL)), _tmp21);
		_tmp22 = NULL;
		label = (_tmp22 = g_object_ref_sink ((GtkLabel*) gtk_label_new ("")), (label == NULL) ? NULL : (label = (g_object_unref (label), NULL)), _tmp22);
		gtk_misc_set_alignment ((GtkMisc*) label, 0.0f, 0.5f);
		_tmp23 = NULL;
		gtk_label_set_markup (label, _tmp23 = g_markup_printf_escaped ("<b>%s:</b>", _ ("Performer")));
		_tmp23 = (g_free (_tmp23), NULL);
		gtk_table_attach (info_box, (GtkWidget*) label, (guint) 0, (guint) 1, (guint) i, (guint) (i + 1), GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, (guint) 0, (guint) 0);
		gtk_misc_set_alignment ((GtkMisc*) pt_label, 0.0f, 0.5f);
		gtk_label_set_line_wrap (pt_label, TRUE);
		gtk_table_attach (info_box, (GtkWidget*) pt_label, (guint) 1, (guint) 2, (guint) i, (guint) (i + 1), GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, (guint) 0, (guint) 0);
		i++;
	}
	/* disc */
	if (song->disc != NULL) {
		GtkLabel* _tmp24;
		GtkLabel* _tmp25;
		char* _tmp26;
		_tmp24 = NULL;
		pt_label = (_tmp24 = g_object_ref_sink ((GtkLabel*) gtk_label_new (song->disc)), (pt_label == NULL) ? NULL : (pt_label = (g_object_unref (pt_label), NULL)), _tmp24);
		_tmp25 = NULL;
		label = (_tmp25 = g_object_ref_sink ((GtkLabel*) gtk_label_new ("")), (label == NULL) ? NULL : (label = (g_object_unref (label), NULL)), _tmp25);
		gtk_misc_set_alignment ((GtkMisc*) label, 0.0f, 0.5f);
		_tmp26 = NULL;
		gtk_label_set_markup (label, _tmp26 = g_markup_printf_escaped ("<b>%s:</b>", _ ("Disc")));
		_tmp26 = (g_free (_tmp26), NULL);
		gtk_table_attach (info_box, (GtkWidget*) label, (guint) 0, (guint) 1, (guint) i, (guint) (i + 1), GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, (guint) 0, (guint) 0);
		gtk_misc_set_alignment ((GtkMisc*) pt_label, 0.0f, 0.5f);
		gtk_label_set_line_wrap (pt_label, TRUE);
		gtk_table_attach (info_box, (GtkWidget*) pt_label, (guint) 1, (guint) 2, (guint) i, (guint) (i + 1), GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, (guint) 0, (guint) 0);
		i++;
	}
	/* Genre */
	if (song->genre != NULL) {
		GtkLabel* _tmp27;
		GtkLabel* _tmp28;
		char* _tmp29;
		_tmp27 = NULL;
		pt_label = (_tmp27 = g_object_ref_sink ((GtkLabel*) gtk_label_new (song->genre)), (pt_label == NULL) ? NULL : (pt_label = (g_object_unref (pt_label), NULL)), _tmp27);
		_tmp28 = NULL;
		label = (_tmp28 = g_object_ref_sink ((GtkLabel*) gtk_label_new ("")), (label == NULL) ? NULL : (label = (g_object_unref (label), NULL)), _tmp28);
		gtk_misc_set_alignment ((GtkMisc*) label, 0.0f, 0.5f);
		_tmp29 = NULL;
		gtk_label_set_markup (label, _tmp29 = g_markup_printf_escaped ("<b>%s:</b>", _ ("Genre")));
		_tmp29 = (g_free (_tmp29), NULL);
		gtk_table_attach (info_box, (GtkWidget*) label, (guint) 0, (guint) 1, (guint) i, (guint) (i + 1), GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, (guint) 0, (guint) 0);
		gtk_misc_set_alignment ((GtkMisc*) pt_label, 0.0f, 0.5f);
		gtk_label_set_line_wrap (pt_label, TRUE);
		gtk_table_attach (info_box, (GtkWidget*) pt_label, (guint) 1, (guint) 2, (guint) i, (guint) (i + 1), GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, (guint) 0, (guint) 0);
		i++;
	}
	/* Path */
	if (song->file != NULL) {
		GtkLabel* _tmp30;
		char* _tmp31;
		GtkLabel* _tmp32;
		GtkHBox* dhbox;
		GtkButton* dbutton;
		GtkImage* _tmp33;
		_tmp30 = NULL;
		label = (_tmp30 = g_object_ref_sink ((GtkLabel*) gtk_label_new ("")), (label == NULL) ? NULL : (label = (g_object_unref (label), NULL)), _tmp30);
		gtk_misc_set_alignment ((GtkMisc*) label, 0.0f, 0.0f);
		_tmp31 = NULL;
		gtk_label_set_markup (label, _tmp31 = g_markup_printf_escaped ("<b>%s:</b>", _ ("Path")));
		_tmp31 = (g_free (_tmp31), NULL);
		gtk_table_attach (info_box, (GtkWidget*) label, (guint) 0, (guint) 1, (guint) i, (guint) (i + 1), GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, (guint) 0, (guint) 0);
		_tmp32 = NULL;
		pt_label = (_tmp32 = g_object_ref_sink ((GtkLabel*) gtk_label_new (song->file)), (pt_label == NULL) ? NULL : (pt_label = (g_object_unref (pt_label), NULL)), _tmp32);
		gtk_misc_set_alignment ((GtkMisc*) pt_label, 0.0f, 0.5f);
		gtk_label_set_line_wrap (pt_label, TRUE);
		dhbox = g_object_ref_sink ((GtkHBox*) gtk_hbox_new (FALSE, 6));
		gtk_box_pack_start ((GtkBox*) dhbox, (GtkWidget*) pt_label, FALSE, FALSE, (guint) 0);
		dbutton = g_object_ref_sink ((GtkButton*) gtk_button_new ());
		gtk_button_set_relief (dbutton, GTK_RELIEF_NONE);
		_tmp33 = NULL;
		gtk_container_add ((GtkContainer*) dbutton, (GtkWidget*) (_tmp33 = g_object_ref_sink ((GtkImage*) gtk_image_new_from_stock ("gtk-open", GTK_ICON_SIZE_MENU))));
		(_tmp33 == NULL) ? NULL : (_tmp33 = (g_object_unref (_tmp33), NULL));
		g_object_set_data_full ((GObject*) dbutton, "path", (void*) g_path_get_dirname (song->file), (GDestroyNotify) g_free);
		g_signal_connect_object (dbutton, "clicked", (GCallback) _gmpc_metadata_browser_metadata_button_open_file_browser_path_gtk_button_clicked, self, 0);
		gtk_widget_set_tooltip_text ((GtkWidget*) dbutton, _ ("Open path to song in file browser"));
		gtk_box_pack_start ((GtkBox*) dhbox, (GtkWidget*) dbutton, FALSE, FALSE, (guint) 0);
		gtk_table_attach (info_box, (GtkWidget*) dhbox, (guint) 1, (guint) 2, (guint) i, (guint) (i + 1), GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, (guint) 0, (guint) 0);
		i++;
		(dhbox == NULL) ? NULL : (dhbox = (g_object_unref (dhbox), NULL));
		(dbutton == NULL) ? NULL : (dbutton = (g_object_unref (dbutton), NULL));
	}
	/* Favored button */
	fav_button = g_object_ref_sink (gmpc_favorites_button_new ());
	gmpc_favorites_button_set_song (fav_button, song);
	_tmp34 = NULL;
	label = (_tmp34 = g_object_ref_sink ((GtkLabel*) gtk_label_new ("")), (label == NULL) ? NULL : (label = (g_object_unref (label), NULL)), _tmp34);
	gtk_misc_set_alignment ((GtkMisc*) label, 0.0f, 0.5f);
	_tmp35 = NULL;
	gtk_label_set_markup (label, _tmp35 = g_markup_printf_escaped ("<b>%s:</b>", _ ("Favored")));
	_tmp35 = (g_free (_tmp35), NULL);
	gtk_table_attach (info_box, (GtkWidget*) label, (guint) 0, (guint) 1, (guint) i, (guint) (i + 1), GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, (guint) 0, (guint) 0);
	_tmp36 = NULL;
	ali = (_tmp36 = g_object_ref_sink ((GtkAlignment*) gtk_alignment_new (0.0f, 0.5f, 0.f, 0.f)), (ali == NULL) ? NULL : (ali = (g_object_unref (ali), NULL)), _tmp36);
	gtk_container_add ((GtkContainer*) ali, (GtkWidget*) fav_button);
	gtk_table_attach (info_box, (GtkWidget*) ali, (guint) 1, (guint) 2, (guint) i, (guint) (i + 1), GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, (guint) 0, (guint) 0);
	i++;
	if (mpd_sticker_supported (connection)) {
		GmpcRating* rating_button;
		GtkLabel* _tmp37;
		char* _tmp38;
		GtkAlignment* _tmp39;
		/* Favored button */
		rating_button = g_object_ref_sink (gmpc_rating_new (connection, song));
		_tmp37 = NULL;
		label = (_tmp37 = g_object_ref_sink ((GtkLabel*) gtk_label_new ("")), (label == NULL) ? NULL : (label = (g_object_unref (label), NULL)), _tmp37);
		gtk_misc_set_alignment ((GtkMisc*) label, 0.0f, 0.5f);
		_tmp38 = NULL;
		gtk_label_set_markup (label, _tmp38 = g_markup_printf_escaped ("<b>%s:</b>", _ ("Rating")));
		_tmp38 = (g_free (_tmp38), NULL);
		gtk_table_attach (info_box, (GtkWidget*) label, (guint) 0, (guint) 1, (guint) i, (guint) (i + 1), GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, (guint) 0, (guint) 0);
		_tmp39 = NULL;
		ali = (_tmp39 = g_object_ref_sink ((GtkAlignment*) gtk_alignment_new (0.0f, 0.5f, 0.f, 0.f)), (ali == NULL) ? NULL : (ali = (g_object_unref (ali), NULL)), _tmp39);
		gtk_container_add ((GtkContainer*) ali, (GtkWidget*) rating_button);
		gtk_table_attach (info_box, (GtkWidget*) ali, (guint) 1, (guint) 2, (guint) i, (guint) (i + 1), GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, (guint) 0, (guint) 0);
		i++;
		(rating_button == NULL) ? NULL : (rating_button = (g_object_unref (rating_button), NULL));
	}
	/* Comment */
	if (song->comment != NULL) {
		GtkLabel* _tmp40;
		GtkLabel* _tmp41;
		char* _tmp42;
		_tmp40 = NULL;
		pt_label = (_tmp40 = g_object_ref_sink ((GtkLabel*) gtk_label_new (song->comment)), (pt_label == NULL) ? NULL : (pt_label = (g_object_unref (pt_label), NULL)), _tmp40);
		_tmp41 = NULL;
		label = (_tmp41 = g_object_ref_sink ((GtkLabel*) gtk_label_new ("")), (label == NULL) ? NULL : (label = (g_object_unref (label), NULL)), _tmp41);
		gtk_misc_set_alignment ((GtkMisc*) label, 0.0f, 0.0f);
		_tmp42 = NULL;
		gtk_label_set_markup (label, _tmp42 = g_markup_printf_escaped ("<b>%s:</b>", _ ("Comment")));
		_tmp42 = (g_free (_tmp42), NULL);
		gtk_table_attach (info_box, (GtkWidget*) label, (guint) 0, (guint) 1, (guint) i, (guint) (i + 1), GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, (guint) 0, (guint) 0);
		gtk_misc_set_alignment ((GtkMisc*) pt_label, 0.0f, 0.5f);
		gtk_label_set_line_wrap (pt_label, TRUE);
		gtk_table_attach (info_box, (GtkWidget*) pt_label, (guint) 1, (guint) 2, (guint) i, (guint) (i + 1), GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, (guint) 0, (guint) 0);
		i++;
	}
	gtk_box_pack_start ((GtkBox*) vbox, (GtkWidget*) hbox, FALSE, FALSE, (guint) 0);
	/* Player controls */
	button = g_object_ref_sink ((GtkButton*) gtk_button_new_from_stock ("gtk-media-play"));
	gtk_button_set_relief (button, GTK_RELIEF_NONE);
	g_object_set_data_full ((GObject*) button, "song", mpd_songDup (song), (GDestroyNotify) mpd_freeSong);
	g_signal_connect_object (button, "clicked", (GCallback) _gmpc_metadata_browser_play_song_gtk_button_clicked, self, 0);
	_tmp43 = NULL;
	hbox = (_tmp43 = g_object_ref_sink ((GtkHBox*) gtk_hbox_new (FALSE, 6)), (hbox == NULL) ? NULL : (hbox = (g_object_unref (hbox), NULL)), _tmp43);
	gtk_box_pack_start ((GtkBox*) hbox, (GtkWidget*) button, FALSE, FALSE, (guint) 0);
	_tmp44 = NULL;
	button = (_tmp44 = g_object_ref_sink ((GtkButton*) gtk_button_new_from_stock ("gtk-add")), (button == NULL) ? NULL : (button = (g_object_unref (button), NULL)), _tmp44);
	gtk_button_set_relief (button, GTK_RELIEF_NONE);
	g_object_set_data_full ((GObject*) button, "song", mpd_songDup (song), (GDestroyNotify) mpd_freeSong);
	g_signal_connect_object (button, "clicked", (GCallback) _gmpc_metadata_browser_add_song_gtk_button_clicked, self, 0);
	gtk_box_pack_start ((GtkBox*) hbox, (GtkWidget*) button, FALSE, FALSE, (guint) 0);
	_tmp45 = NULL;
	button = (_tmp45 = g_object_ref_sink ((GtkButton*) gtk_button_new_with_mnemonic ("_Replace")), (button == NULL) ? NULL : (button = (g_object_unref (button), NULL)), _tmp45);
	_tmp46 = NULL;
	gtk_button_set_image (button, (GtkWidget*) (_tmp46 = g_object_ref_sink ((GtkImage*) gtk_image_new_from_stock ("gtk-redo", GTK_ICON_SIZE_BUTTON))));
	(_tmp46 == NULL) ? NULL : (_tmp46 = (g_object_unref (_tmp46), NULL));
	gtk_button_set_relief (button, GTK_RELIEF_NONE);
	g_object_set_data_full ((GObject*) button, "song", mpd_songDup (song), (GDestroyNotify) mpd_freeSong);
	g_signal_connect_object (button, "clicked", (GCallback) _gmpc_metadata_browser_replace_song_gtk_button_clicked, self, 0);
	gtk_box_pack_start ((GtkBox*) hbox, (GtkWidget*) button, FALSE, FALSE, (guint) 0);
	gtk_table_attach (info_box, (GtkWidget*) hbox, (guint) 0, (guint) 2, (guint) i, (guint) (i + 1), GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, (guint) 0, (guint) 0);
	i++;
	/* Lyrics */
	text_view = g_object_ref_sink (gmpc_meta_text_view_new (META_SONG_TXT));
	gtk_text_view_set_left_margin ((GtkTextView*) text_view, 8);
	_tmp47 = NULL;
	_tmp48 = NULL;
	frame = (_tmp48 = g_object_ref_sink (gmpc_widget_more_new (_tmp47 = g_markup_printf_escaped ("<b>%s:</b>", _ ("Lyrics")), (GtkWidget*) text_view)), _tmp47 = (g_free (_tmp47), NULL), _tmp48);
	gmpc_meta_text_view_query_text_from_song (text_view, song);
	gtk_box_pack_start ((GtkBox*) vbox, (GtkWidget*) frame, FALSE, FALSE, (guint) 0);
	/* Guitar Tab */
	_tmp49 = NULL;
	text_view = (_tmp49 = g_object_ref_sink (gmpc_meta_text_view_new (META_SONG_GUITAR_TAB)), (text_view == NULL) ? NULL : (text_view = (g_object_unref (text_view), NULL)), _tmp49);
	gtk_text_view_set_left_margin ((GtkTextView*) text_view, 8);
	_tmp51 = NULL;
	_tmp50 = NULL;
	frame = (_tmp51 = g_object_ref_sink (gmpc_widget_more_new (_tmp50 = g_markup_printf_escaped ("<b>%s:</b>", _ ("Guitar Tabs")), (GtkWidget*) text_view)), (frame == NULL) ? NULL : (frame = (g_object_unref (frame), NULL)), _tmp51);
	_tmp50 = (g_free (_tmp50), NULL);
	gmpc_meta_text_view_query_text_from_song (text_view, song);
	gtk_box_pack_start ((GtkBox*) vbox, (GtkWidget*) frame, FALSE, FALSE, (guint) 0);
	similar_songs = g_object_ref_sink (gmpc_widget_similar_songs_new (song));
	gtk_box_pack_start ((GtkBox*) vbox, (GtkWidget*) similar_songs, FALSE, FALSE, (guint) 0);
	song_links = g_object_ref_sink (gmpc_song_links_new (GMPC_SONG_LINKS_TYPE_SONG, song));
	gtk_box_pack_start ((GtkBox*) vbox, (GtkWidget*) song_links, FALSE, FALSE, (guint) 0);
	/**
	         * Add it to the view
	         
	
	        this.metadata_box.add(vbox);
	        this.metadata_sw.show_all();
	        */
	_tmp52 = NULL;
	return (_tmp52 = (GtkWidget*) vbox, (label == NULL) ? NULL : (label = (g_object_unref (label), NULL)), (hbox == NULL) ? NULL : (hbox = (g_object_unref (hbox), NULL)), (ali == NULL) ? NULL : (ali = (g_object_unref (ali), NULL)), (artist_image == NULL) ? NULL : (artist_image = (g_object_unref (artist_image), NULL)), (info_box == NULL) ? NULL : (info_box = (g_object_unref (info_box), NULL)), (pt_label == NULL) ? NULL : (pt_label = (g_object_unref (pt_label), NULL)), (fav_button == NULL) ? NULL : (fav_button = (g_object_unref (fav_button), NULL)), (button == NULL) ? NULL : (button = (g_object_unref (button), NULL)), (text_view == NULL) ? NULL : (text_view = (g_object_unref (text_view), NULL)), (frame == NULL) ? NULL : (frame = (g_object_unref (frame), NULL)), (similar_songs == NULL) ? NULL : (similar_songs = (g_object_unref (similar_songs), NULL)), (song_links == NULL) ? NULL : (song_links = (g_object_unref (song_links), NULL)), _tmp52);
}


static void gmpc_metadata_browser_metadata_button_open_file_browser_path (GmpcMetadataBrowser* self, GtkButton* button) {
	const char* _tmp0;
	char* path;
	g_return_if_fail (self != NULL);
	g_return_if_fail (button != NULL);
	_tmp0 = NULL;
	path = (_tmp0 = (const char*) g_object_get_data ((GObject*) button, "path"), (_tmp0 == NULL) ? NULL : g_strdup (_tmp0));
	if (path != NULL) {
		pl3_file_browser_open_path (path);
	}
	path = (g_free (path), NULL);
}


static void gmpc_metadata_browser_metadata_find_query (GmpcMetadataBrowser* self, GtkButton* button) {
	const char* _tmp0;
	char* path;
	g_return_if_fail (self != NULL);
	g_return_if_fail (button != NULL);
	_tmp0 = NULL;
	path = (_tmp0 = (const char*) g_object_get_data ((GObject*) button, "query"), (_tmp0 == NULL) ? NULL : g_strdup (_tmp0));
	if (path != NULL) {
		pl3_find2_ec_database (NULL, path);
	}
	path = (g_free (path), NULL);
}


static void _gmpc_metadata_browser_add_selected_song_gtk_button_clicked (GtkButton* _sender, gpointer self) {
	gmpc_metadata_browser_add_selected_song (self, _sender);
}


static void _gmpc_metadata_browser_replace_selected_song_gtk_button_clicked (GtkButton* _sender, gpointer self) {
	gmpc_metadata_browser_replace_selected_song (self, _sender);
}


static void gmpc_metadata_browser_metadata_box_show_album (GmpcMetadataBrowser* self, const char* artist, const char* album) {
	GtkVBox* vbox;
	GtkLabel* label;
	const char* _tmp0;
	const char* _tmp1;
	char* _tmp2;
	GtkHBox* hbox;
	GtkAlignment* ali;
	GmpcMetaImage* artist_image;
	mpd_Song* song;
	char* _tmp4;
	const char* _tmp3;
	char* _tmp6;
	const char* _tmp5;
	GtkTable* info_box;
	gint i;
	GmpcStatsLabel* pt_label;
	GtkLabel* _tmp7;
	char* _tmp8;
	GmpcStatsLabel* _tmp9;
	GtkLabel* _tmp10;
	char* _tmp11;
	GmpcStatsLabel* _tmp12;
	GtkLabel* _tmp13;
	char* _tmp14;
	GmpcStatsLabel* _tmp15;
	GtkLabel* _tmp16;
	char* _tmp17;
	GtkHBox* _tmp18;
	GtkButton* button;
	GtkButton* _tmp19;
	GtkImage* _tmp20;
	GmpcMetaTextView* text_view;
	char* _tmp21;
	GmpcWidgetMore* _tmp22;
	GmpcWidgetMore* frame;
	GtkLabel* _tmp23;
	char* _tmp24;
	GtkScrolledWindow* sw;
	GmpcMpdDataTreeview* song_tree;
	GmpcSongLinks* song_links;
	g_return_if_fail (self != NULL);
	g_return_if_fail (artist != NULL);
	g_return_if_fail (album != NULL);
	vbox = g_object_ref_sink ((GtkVBox*) gtk_vbox_new (FALSE, 6));
	gtk_container_set_border_width ((GtkContainer*) vbox, (guint) 8);
	label = g_object_ref_sink ((GtkLabel*) gtk_label_new (""));
	_tmp0 = NULL;
	if (artist != NULL) {
		_tmp0 = artist;
	} else {
		_tmp0 = _ ("Unknown");
	}
	_tmp1 = NULL;
	if (album != NULL) {
		_tmp1 = album;
	} else {
		_tmp1 = _ ("Unknown");
	}
	_tmp2 = NULL;
	gtk_label_set_markup (label, _tmp2 = g_markup_printf_escaped ("<span size='xx-large' weight='bold'>%s - %s</span>", _tmp0, _tmp1));
	_tmp2 = (g_free (_tmp2), NULL);
	gtk_misc_set_alignment ((GtkMisc*) label, 0.0f, 0.5f);
	gtk_box_pack_start ((GtkBox*) vbox, (GtkWidget*) label, FALSE, FALSE, (guint) 0);
	/* Artist image */
	hbox = g_object_ref_sink ((GtkHBox*) gtk_hbox_new (FALSE, 6));
	ali = g_object_ref_sink ((GtkAlignment*) gtk_alignment_new (0.f, 0.f, 0.f, 0.f));
	artist_image = g_object_ref_sink (gmpc_metaimage_new_size (META_ALBUM_ART, 250));
	gmpc_metaimage_set_squared (artist_image, FALSE);
	song = mpd_newSong ();
	_tmp4 = NULL;
	_tmp3 = NULL;
	song->artist = (_tmp4 = (_tmp3 = artist, (_tmp3 == NULL) ? NULL : g_strdup (_tmp3)), song->artist = (g_free (song->artist), NULL), _tmp4);
	_tmp6 = NULL;
	_tmp5 = NULL;
	song->album = (_tmp6 = (_tmp5 = album, (_tmp5 == NULL) ? NULL : g_strdup (_tmp5)), song->album = (g_free (song->album), NULL), _tmp6);
	gmpc_metaimage_update_cover_from_song (artist_image, song);
	gtk_container_add ((GtkContainer*) ali, (GtkWidget*) artist_image);
	gtk_box_pack_start ((GtkBox*) hbox, (GtkWidget*) ali, FALSE, FALSE, (guint) 0);
	/* Artist information */
	info_box = g_object_ref_sink ((GtkTable*) gtk_table_new ((guint) 4, (guint) 2, FALSE));
	gtk_table_set_row_spacings (info_box, (guint) 3);
	gtk_table_set_col_spacings (info_box, (guint) 8);
	gtk_box_pack_start ((GtkBox*) hbox, (GtkWidget*) info_box, FALSE, FALSE, (guint) 0);
	i = 0;
	/* Genres of songs */
	pt_label = g_object_ref_sink (gmpc_stats_label_new (ALBUM_GENRES_SONGS, song));
	_tmp7 = NULL;
	label = (_tmp7 = g_object_ref_sink ((GtkLabel*) gtk_label_new ("")), (label == NULL) ? NULL : (label = (g_object_unref (label), NULL)), _tmp7);
	gtk_misc_set_alignment ((GtkMisc*) label, 0.0f, 0.5f);
	_tmp8 = NULL;
	gtk_label_set_markup (label, _tmp8 = g_markup_printf_escaped ("<b>%s:</b>", _ ("Genres")));
	_tmp8 = (g_free (_tmp8), NULL);
	gtk_table_attach (info_box, (GtkWidget*) label, (guint) 0, (guint) 1, (guint) i, (guint) (i + 1), GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, (guint) 0, (guint) 0);
	gtk_misc_set_alignment ((GtkMisc*) pt_label, 0.0f, 0.5f);
	gtk_label_set_line_wrap ((GtkLabel*) pt_label, TRUE);
	gtk_table_attach (info_box, (GtkWidget*) pt_label, (guint) 1, (guint) 2, (guint) i, (guint) (i + 1), GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, (guint) 0, (guint) 0);
	i++;
	/* Dates of songs */
	_tmp9 = NULL;
	pt_label = (_tmp9 = g_object_ref_sink (gmpc_stats_label_new (ALBUM_DATES_SONGS, song)), (pt_label == NULL) ? NULL : (pt_label = (g_object_unref (pt_label), NULL)), _tmp9);
	gtk_label_set_line_wrap ((GtkLabel*) pt_label, TRUE);
	_tmp10 = NULL;
	label = (_tmp10 = g_object_ref_sink ((GtkLabel*) gtk_label_new ("")), (label == NULL) ? NULL : (label = (g_object_unref (label), NULL)), _tmp10);
	gtk_misc_set_alignment ((GtkMisc*) label, 0.0f, 0.0f);
	_tmp11 = NULL;
	gtk_label_set_markup (label, _tmp11 = g_markup_printf_escaped ("<b>%s:</b>", _ ("Dates")));
	_tmp11 = (g_free (_tmp11), NULL);
	gtk_table_attach (info_box, (GtkWidget*) label, (guint) 0, (guint) 1, (guint) i, (guint) (i + 1), GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, (guint) 0, (guint) 0);
	gtk_misc_set_alignment ((GtkMisc*) pt_label, 0.0f, 0.5f);
	gtk_table_attach (info_box, (GtkWidget*) pt_label, (guint) 1, (guint) 2, (guint) i, (guint) (i + 1), GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, (guint) 0, (guint) 0);
	i++;
	/* Total number of songs */
	_tmp12 = NULL;
	pt_label = (_tmp12 = g_object_ref_sink (gmpc_stats_label_new (ALBUM_NUM_SONGS, song)), (pt_label == NULL) ? NULL : (pt_label = (g_object_unref (pt_label), NULL)), _tmp12);
	gtk_label_set_line_wrap ((GtkLabel*) pt_label, TRUE);
	_tmp13 = NULL;
	label = (_tmp13 = g_object_ref_sink ((GtkLabel*) gtk_label_new ("")), (label == NULL) ? NULL : (label = (g_object_unref (label), NULL)), _tmp13);
	gtk_misc_set_alignment ((GtkMisc*) label, 0.0f, 0.0f);
	_tmp14 = NULL;
	gtk_label_set_markup (label, _tmp14 = g_markup_printf_escaped ("<b>%s:</b>", _ ("Songs")));
	_tmp14 = (g_free (_tmp14), NULL);
	gtk_table_attach (info_box, (GtkWidget*) label, (guint) 0, (guint) 1, (guint) i, (guint) (i + 1), GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, (guint) 0, (guint) 0);
	gtk_misc_set_alignment ((GtkMisc*) pt_label, 0.0f, 0.5f);
	gtk_table_attach (info_box, (GtkWidget*) pt_label, (guint) 1, (guint) 2, (guint) i, (guint) (i + 1), GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, (guint) 0, (guint) 0);
	i++;
	/* Total playtime */
	_tmp15 = NULL;
	pt_label = (_tmp15 = g_object_ref_sink (gmpc_stats_label_new (ALBUM_PLAYTIME_SONGS, song)), (pt_label == NULL) ? NULL : (pt_label = (g_object_unref (pt_label), NULL)), _tmp15);
	gtk_label_set_line_wrap ((GtkLabel*) pt_label, TRUE);
	_tmp16 = NULL;
	label = (_tmp16 = g_object_ref_sink ((GtkLabel*) gtk_label_new ("")), (label == NULL) ? NULL : (label = (g_object_unref (label), NULL)), _tmp16);
	gtk_misc_set_alignment ((GtkMisc*) label, 0.0f, 0.0f);
	_tmp17 = NULL;
	gtk_label_set_markup (label, _tmp17 = g_markup_printf_escaped ("<b>%s:</b>", _ ("Playtime")));
	_tmp17 = (g_free (_tmp17), NULL);
	gtk_table_attach (info_box, (GtkWidget*) label, (guint) 0, (guint) 1, (guint) i, (guint) (i + 1), GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, (guint) 0, (guint) 0);
	gtk_misc_set_alignment ((GtkMisc*) pt_label, 0.0f, 0.5f);
	gtk_table_attach (info_box, (GtkWidget*) pt_label, (guint) 1, (guint) 2, (guint) i, (guint) (i + 1), GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, (guint) 0, (guint) 0);
	i++;
	gtk_box_pack_start ((GtkBox*) vbox, (GtkWidget*) hbox, FALSE, FALSE, (guint) 0);
	/* Player controls */
	_tmp18 = NULL;
	hbox = (_tmp18 = g_object_ref_sink ((GtkHBox*) gtk_hbox_new (FALSE, 6)), (hbox == NULL) ? NULL : (hbox = (g_object_unref (hbox), NULL)), _tmp18);
	button = g_object_ref_sink ((GtkButton*) gtk_button_new_from_stock ("gtk-add"));
	gtk_button_set_relief (button, GTK_RELIEF_NONE);
	g_signal_connect_object (button, "clicked", (GCallback) _gmpc_metadata_browser_add_selected_song_gtk_button_clicked, self, 0);
	gtk_box_pack_start ((GtkBox*) hbox, (GtkWidget*) button, FALSE, FALSE, (guint) 0);
	_tmp19 = NULL;
	button = (_tmp19 = g_object_ref_sink ((GtkButton*) gtk_button_new_with_mnemonic ("_Replace")), (button == NULL) ? NULL : (button = (g_object_unref (button), NULL)), _tmp19);
	_tmp20 = NULL;
	gtk_button_set_image (button, (GtkWidget*) (_tmp20 = g_object_ref_sink ((GtkImage*) gtk_image_new_from_stock ("gtk-redo", GTK_ICON_SIZE_BUTTON))));
	(_tmp20 == NULL) ? NULL : (_tmp20 = (g_object_unref (_tmp20), NULL));
	gtk_button_set_relief (button, GTK_RELIEF_NONE);
	g_signal_connect_object (button, "clicked", (GCallback) _gmpc_metadata_browser_replace_selected_song_gtk_button_clicked, self, 0);
	gtk_box_pack_start ((GtkBox*) hbox, (GtkWidget*) button, FALSE, FALSE, (guint) 0);
	gtk_table_attach (info_box, (GtkWidget*) hbox, (guint) 0, (guint) 2, (guint) i, (guint) (i + 1), GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, (guint) 0, (guint) 0);
	i++;
	text_view = g_object_ref_sink (gmpc_meta_text_view_new (META_ALBUM_TXT));
	gtk_text_view_set_left_margin ((GtkTextView*) text_view, 8);
	_tmp21 = NULL;
	_tmp22 = NULL;
	frame = (_tmp22 = g_object_ref_sink (gmpc_widget_more_new (_tmp21 = g_markup_printf_escaped ("<b>%s:</b>", _ ("Album information")), (GtkWidget*) text_view)), _tmp21 = (g_free (_tmp21), NULL), _tmp22);
	gmpc_meta_text_view_query_text_from_song (text_view, song);
	gtk_box_pack_start ((GtkBox*) vbox, (GtkWidget*) frame, FALSE, FALSE, (guint) 0);
	/* Song list */
	_tmp23 = NULL;
	label = (_tmp23 = g_object_ref_sink ((GtkLabel*) gtk_label_new ("")), (label == NULL) ? NULL : (label = (g_object_unref (label), NULL)), _tmp23);
	_tmp24 = NULL;
	gtk_label_set_markup (label, _tmp24 = g_strdup_printf ("<b>%s</b>", _ ("Songs")));
	_tmp24 = (g_free (_tmp24), NULL);
	gtk_misc_set_alignment ((GtkMisc*) label, 0.0f, 0.5f);
	gtk_box_pack_start ((GtkBox*) vbox, (GtkWidget*) label, FALSE, FALSE, (guint) 0);
	sw = g_object_ref_sink ((GtkScrolledWindow*) gtk_scrolled_window_new (NULL, NULL));
	gtk_scrolled_window_set_policy (sw, GTK_POLICY_AUTOMATIC, GTK_POLICY_NEVER);
	gtk_scrolled_window_set_shadow_type (sw, GTK_SHADOW_ETCHED_IN);
	song_tree = g_object_ref_sink (gmpc_mpddata_treeview_new ("album-songs", TRUE, (GtkTreeModel*) self->priv->model_songs));
	gtk_container_add ((GtkContainer*) sw, (GtkWidget*) song_tree);
	gtk_box_pack_start ((GtkBox*) vbox, (GtkWidget*) sw, FALSE, FALSE, (guint) 0);
	/* TODO right mouse menu 
	 Song links */
	song_links = g_object_ref_sink (gmpc_song_links_new (GMPC_SONG_LINKS_TYPE_ALBUM, song));
	gtk_box_pack_start ((GtkBox*) vbox, (GtkWidget*) song_links, FALSE, FALSE, (guint) 0);
	/**
	         * Add it to the view
	         */
	gtk_container_add ((GtkContainer*) self->priv->metadata_box, (GtkWidget*) vbox);
	gtk_widget_show_all ((GtkWidget*) self->priv->metadata_sw);
	(vbox == NULL) ? NULL : (vbox = (g_object_unref (vbox), NULL));
	(label == NULL) ? NULL : (label = (g_object_unref (label), NULL));
	(hbox == NULL) ? NULL : (hbox = (g_object_unref (hbox), NULL));
	(ali == NULL) ? NULL : (ali = (g_object_unref (ali), NULL));
	(artist_image == NULL) ? NULL : (artist_image = (g_object_unref (artist_image), NULL));
	(song == NULL) ? NULL : (song = (mpd_freeSong (song), NULL));
	(info_box == NULL) ? NULL : (info_box = (g_object_unref (info_box), NULL));
	(pt_label == NULL) ? NULL : (pt_label = (g_object_unref (pt_label), NULL));
	(button == NULL) ? NULL : (button = (g_object_unref (button), NULL));
	(text_view == NULL) ? NULL : (text_view = (g_object_unref (text_view), NULL));
	(frame == NULL) ? NULL : (frame = (g_object_unref (frame), NULL));
	(sw == NULL) ? NULL : (sw = (g_object_unref (sw), NULL));
	(song_tree == NULL) ? NULL : (song_tree = (g_object_unref (song_tree), NULL));
	(song_links == NULL) ? NULL : (song_links = (g_object_unref (song_links), NULL));
}


/**
     * This fills the view for artist 
     * <artist name>
     * <image> | <array with info>
     *           < buttonss>
     *
     * <artist info text>
     *
     * <similar artists>
     * <links>
     */
static void gmpc_metadata_browser_metadata_box_show_artist (GmpcMetadataBrowser* self, const char* artist) {
	GtkVBox* vbox;
	GtkLabel* label;
	const char* _tmp0;
	char* _tmp1;
	GtkHBox* hbox;
	GtkAlignment* ali;
	GmpcMetaImage* artist_image;
	mpd_Song* song;
	char* _tmp3;
	const char* _tmp2;
	GtkTable* info_box;
	gint i;
	GmpcStatsLabel* pt_label;
	GtkLabel* _tmp4;
	char* _tmp5;
	GmpcStatsLabel* _tmp6;
	GtkLabel* _tmp7;
	char* _tmp8;
	GmpcStatsLabel* _tmp9;
	GtkLabel* _tmp10;
	char* _tmp11;
	GmpcStatsLabel* _tmp12;
	GtkLabel* _tmp13;
	char* _tmp14;
	GtkHBox* _tmp15;
	GtkButton* button;
	GtkButton* _tmp16;
	GtkImage* _tmp17;
	GmpcMetaTextView* text_view;
	char* _tmp18;
	GmpcWidgetMore* _tmp19;
	GmpcWidgetMore* frame;
	GtkLabel* _tmp20;
	char* _tmp21;
	GmpcWidgetSimilarArtist* similar_artist;
	GmpcSongLinks* song_links;
	g_return_if_fail (self != NULL);
	g_return_if_fail (artist != NULL);
	vbox = g_object_ref_sink ((GtkVBox*) gtk_vbox_new (FALSE, 6));
	gtk_container_set_border_width ((GtkContainer*) vbox, (guint) 8);
	label = g_object_ref_sink ((GtkLabel*) gtk_label_new (""));
	_tmp0 = NULL;
	if (artist != NULL) {
		_tmp0 = artist;
	} else {
		_tmp0 = _ ("Unknown");
	}
	_tmp1 = NULL;
	gtk_label_set_markup (label, _tmp1 = g_markup_printf_escaped ("<span size='xx-large' weight='bold'>%s</span>", _tmp0));
	_tmp1 = (g_free (_tmp1), NULL);
	gtk_misc_set_alignment ((GtkMisc*) label, 0.0f, 0.5f);
	gtk_box_pack_start ((GtkBox*) vbox, (GtkWidget*) label, FALSE, FALSE, (guint) 0);
	/* Artist image */
	hbox = g_object_ref_sink ((GtkHBox*) gtk_hbox_new (FALSE, 6));
	ali = g_object_ref_sink ((GtkAlignment*) gtk_alignment_new (0.f, 0.f, 0.f, 0.f));
	artist_image = g_object_ref_sink (gmpc_metaimage_new_size (META_ARTIST_ART, 250));
	gmpc_metaimage_set_squared (artist_image, FALSE);
	song = mpd_newSong ();
	_tmp3 = NULL;
	_tmp2 = NULL;
	song->artist = (_tmp3 = (_tmp2 = artist, (_tmp2 == NULL) ? NULL : g_strdup (_tmp2)), song->artist = (g_free (song->artist), NULL), _tmp3);
	gmpc_metaimage_update_cover_from_song (artist_image, song);
	gtk_container_add ((GtkContainer*) ali, (GtkWidget*) artist_image);
	gtk_box_pack_start ((GtkBox*) hbox, (GtkWidget*) ali, FALSE, FALSE, (guint) 0);
	/* Artist information */
	info_box = g_object_ref_sink ((GtkTable*) gtk_table_new ((guint) 4, (guint) 2, FALSE));
	gtk_table_set_row_spacings (info_box, (guint) 3);
	gtk_table_set_col_spacings (info_box, (guint) 8);
	gtk_box_pack_start ((GtkBox*) hbox, (GtkWidget*) info_box, FALSE, FALSE, (guint) 0);
	i = 0;
	/* Genres of songs */
	pt_label = g_object_ref_sink (gmpc_stats_label_new (ARTIST_GENRES_SONGS, song));
	_tmp4 = NULL;
	label = (_tmp4 = g_object_ref_sink ((GtkLabel*) gtk_label_new ("")), (label == NULL) ? NULL : (label = (g_object_unref (label), NULL)), _tmp4);
	gtk_misc_set_alignment ((GtkMisc*) label, 0.0f, 0.5f);
	_tmp5 = NULL;
	gtk_label_set_markup (label, _tmp5 = g_markup_printf_escaped ("<b>%s:</b>", _ ("Genres")));
	_tmp5 = (g_free (_tmp5), NULL);
	gtk_table_attach (info_box, (GtkWidget*) label, (guint) 0, (guint) 1, (guint) i, (guint) (i + 1), GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, (guint) 0, (guint) 0);
	gtk_misc_set_alignment ((GtkMisc*) pt_label, 0.0f, 0.5f);
	gtk_label_set_line_wrap ((GtkLabel*) pt_label, TRUE);
	gtk_table_attach (info_box, (GtkWidget*) pt_label, (guint) 1, (guint) 2, (guint) i, (guint) (i + 1), GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, (guint) 0, (guint) 0);
	i++;
	/* Dates of songs */
	_tmp6 = NULL;
	pt_label = (_tmp6 = g_object_ref_sink (gmpc_stats_label_new (ARTIST_DATES_SONGS, song)), (pt_label == NULL) ? NULL : (pt_label = (g_object_unref (pt_label), NULL)), _tmp6);
	gtk_label_set_line_wrap ((GtkLabel*) pt_label, TRUE);
	_tmp7 = NULL;
	label = (_tmp7 = g_object_ref_sink ((GtkLabel*) gtk_label_new ("")), (label == NULL) ? NULL : (label = (g_object_unref (label), NULL)), _tmp7);
	gtk_misc_set_alignment ((GtkMisc*) label, 0.0f, 0.0f);
	_tmp8 = NULL;
	gtk_label_set_markup (label, _tmp8 = g_markup_printf_escaped ("<b>%s:</b>", _ ("Dates")));
	_tmp8 = (g_free (_tmp8), NULL);
	gtk_table_attach (info_box, (GtkWidget*) label, (guint) 0, (guint) 1, (guint) i, (guint) (i + 1), GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, (guint) 0, (guint) 0);
	gtk_misc_set_alignment ((GtkMisc*) pt_label, 0.0f, 0.5f);
	gtk_table_attach (info_box, (GtkWidget*) pt_label, (guint) 1, (guint) 2, (guint) i, (guint) (i + 1), GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, (guint) 0, (guint) 0);
	i++;
	/* Total number of songs */
	_tmp9 = NULL;
	pt_label = (_tmp9 = g_object_ref_sink (gmpc_stats_label_new (ARTIST_NUM_SONGS, song)), (pt_label == NULL) ? NULL : (pt_label = (g_object_unref (pt_label), NULL)), _tmp9);
	gtk_label_set_line_wrap ((GtkLabel*) pt_label, TRUE);
	_tmp10 = NULL;
	label = (_tmp10 = g_object_ref_sink ((GtkLabel*) gtk_label_new ("")), (label == NULL) ? NULL : (label = (g_object_unref (label), NULL)), _tmp10);
	gtk_misc_set_alignment ((GtkMisc*) label, 0.0f, 0.0f);
	_tmp11 = NULL;
	gtk_label_set_markup (label, _tmp11 = g_markup_printf_escaped ("<b>%s:</b>", _ ("Songs")));
	_tmp11 = (g_free (_tmp11), NULL);
	gtk_table_attach (info_box, (GtkWidget*) label, (guint) 0, (guint) 1, (guint) i, (guint) (i + 1), GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, (guint) 0, (guint) 0);
	gtk_misc_set_alignment ((GtkMisc*) pt_label, 0.0f, 0.5f);
	gtk_table_attach (info_box, (GtkWidget*) pt_label, (guint) 1, (guint) 2, (guint) i, (guint) (i + 1), GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, (guint) 0, (guint) 0);
	i++;
	/* Total playtime */
	_tmp12 = NULL;
	pt_label = (_tmp12 = g_object_ref_sink (gmpc_stats_label_new (ARTIST_PLAYTIME_SONGS, song)), (pt_label == NULL) ? NULL : (pt_label = (g_object_unref (pt_label), NULL)), _tmp12);
	gtk_label_set_line_wrap ((GtkLabel*) pt_label, TRUE);
	_tmp13 = NULL;
	label = (_tmp13 = g_object_ref_sink ((GtkLabel*) gtk_label_new ("")), (label == NULL) ? NULL : (label = (g_object_unref (label), NULL)), _tmp13);
	gtk_misc_set_alignment ((GtkMisc*) label, 0.0f, 0.0f);
	_tmp14 = NULL;
	gtk_label_set_markup (label, _tmp14 = g_markup_printf_escaped ("<b>%s:</b>", _ ("Playtime")));
	_tmp14 = (g_free (_tmp14), NULL);
	gtk_table_attach (info_box, (GtkWidget*) label, (guint) 0, (guint) 1, (guint) i, (guint) (i + 1), GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, (guint) 0, (guint) 0);
	gtk_misc_set_alignment ((GtkMisc*) pt_label, 0.0f, 0.5f);
	gtk_table_attach (info_box, (GtkWidget*) pt_label, (guint) 1, (guint) 2, (guint) i, (guint) (i + 1), GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, (guint) 0, (guint) 0);
	i++;
	gtk_box_pack_start ((GtkBox*) vbox, (GtkWidget*) hbox, FALSE, FALSE, (guint) 0);
	/* Player controls */
	_tmp15 = NULL;
	hbox = (_tmp15 = g_object_ref_sink ((GtkHBox*) gtk_hbox_new (FALSE, 6)), (hbox == NULL) ? NULL : (hbox = (g_object_unref (hbox), NULL)), _tmp15);
	button = g_object_ref_sink ((GtkButton*) gtk_button_new_from_stock ("gtk-add"));
	gtk_button_set_relief (button, GTK_RELIEF_NONE);
	g_signal_connect_object (button, "clicked", (GCallback) _gmpc_metadata_browser_add_selected_song_gtk_button_clicked, self, 0);
	gtk_box_pack_start ((GtkBox*) hbox, (GtkWidget*) button, FALSE, FALSE, (guint) 0);
	_tmp16 = NULL;
	button = (_tmp16 = g_object_ref_sink ((GtkButton*) gtk_button_new_with_mnemonic ("_Replace")), (button == NULL) ? NULL : (button = (g_object_unref (button), NULL)), _tmp16);
	_tmp17 = NULL;
	gtk_button_set_image (button, (GtkWidget*) (_tmp17 = g_object_ref_sink ((GtkImage*) gtk_image_new_from_stock ("gtk-redo", GTK_ICON_SIZE_BUTTON))));
	(_tmp17 == NULL) ? NULL : (_tmp17 = (g_object_unref (_tmp17), NULL));
	gtk_button_set_relief (button, GTK_RELIEF_NONE);
	g_signal_connect_object (button, "clicked", (GCallback) _gmpc_metadata_browser_replace_selected_song_gtk_button_clicked, self, 0);
	gtk_box_pack_start ((GtkBox*) hbox, (GtkWidget*) button, FALSE, FALSE, (guint) 0);
	gtk_table_attach (info_box, (GtkWidget*) hbox, (guint) 0, (guint) 2, (guint) i, (guint) (i + 1), GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, (guint) 0, (guint) 0);
	i++;
	text_view = g_object_ref_sink (gmpc_meta_text_view_new (META_ARTIST_TXT));
	gtk_text_view_set_left_margin ((GtkTextView*) text_view, 8);
	_tmp18 = NULL;
	_tmp19 = NULL;
	frame = (_tmp19 = g_object_ref_sink (gmpc_widget_more_new (_tmp18 = g_markup_printf_escaped ("<b>%s:</b>", _ ("Artist information")), (GtkWidget*) text_view)), _tmp18 = (g_free (_tmp18), NULL), _tmp19);
	gmpc_meta_text_view_query_text_from_song (text_view, song);
	gtk_box_pack_start ((GtkBox*) vbox, (GtkWidget*) frame, FALSE, FALSE, (guint) 0);
	_tmp20 = NULL;
	label = (_tmp20 = g_object_ref_sink ((GtkLabel*) gtk_label_new (_ ("Similar artist"))), (label == NULL) ? NULL : (label = (g_object_unref (label), NULL)), _tmp20);
	_tmp21 = NULL;
	gtk_label_set_markup (label, _tmp21 = g_strdup_printf ("<span weight='bold'>%s</span>", _ ("Similar artist")));
	_tmp21 = (g_free (_tmp21), NULL);
	gtk_misc_set_alignment ((GtkMisc*) label, 0.0f, 0.0f);
	gtk_box_pack_start ((GtkBox*) vbox, (GtkWidget*) label, FALSE, FALSE, (guint) 0);
	similar_artist = g_object_ref_sink (gmpc_widget_similar_artist_new (self, connection, song));
	gtk_box_pack_start ((GtkBox*) vbox, (GtkWidget*) similar_artist, FALSE, FALSE, (guint) 0);
	song_links = g_object_ref_sink (gmpc_song_links_new (GMPC_SONG_LINKS_TYPE_ARTIST, song));
	gtk_box_pack_start ((GtkBox*) vbox, (GtkWidget*) song_links, FALSE, FALSE, (guint) 0);
	/**
	         * Add it to the view
	         */
	gtk_container_add ((GtkContainer*) self->priv->metadata_box, (GtkWidget*) vbox);
	gtk_widget_show_all ((GtkWidget*) self->priv->metadata_sw);
	(vbox == NULL) ? NULL : (vbox = (g_object_unref (vbox), NULL));
	(label == NULL) ? NULL : (label = (g_object_unref (label), NULL));
	(hbox == NULL) ? NULL : (hbox = (g_object_unref (hbox), NULL));
	(ali == NULL) ? NULL : (ali = (g_object_unref (ali), NULL));
	(artist_image == NULL) ? NULL : (artist_image = (g_object_unref (artist_image), NULL));
	(song == NULL) ? NULL : (song = (mpd_freeSong (song), NULL));
	(info_box == NULL) ? NULL : (info_box = (g_object_unref (info_box), NULL));
	(pt_label == NULL) ? NULL : (pt_label = (g_object_unref (pt_label), NULL));
	(button == NULL) ? NULL : (button = (g_object_unref (button), NULL));
	(text_view == NULL) ? NULL : (text_view = (g_object_unref (text_view), NULL));
	(frame == NULL) ? NULL : (frame = (g_object_unref (frame), NULL));
	(similar_artist == NULL) ? NULL : (similar_artist = (g_object_unref (similar_artist), NULL));
	(song_links == NULL) ? NULL : (song_links = (g_object_unref (song_links), NULL));
}


static gboolean _gmpc_metadata_browser_metadata_box_update_real_gsource_func (gpointer self) {
	return gmpc_metadata_browser_metadata_box_update_real (self);
}


static void gmpc_metadata_browser_metadata_box_update (GmpcMetadataBrowser* self) {
	g_return_if_fail (self != NULL);
	if (self->priv->update_timeout > 0) {
		g_source_remove (self->priv->update_timeout);
	}
	self->priv->update_timeout = g_idle_add (_gmpc_metadata_browser_metadata_box_update_real_gsource_func, self);
}


static gboolean gmpc_metadata_browser_metadata_box_update_real (GmpcMetadataBrowser* self) {
	char* artist;
	char* album;
	mpd_Song* song;
	gboolean _tmp2;
	g_return_val_if_fail (self != NULL, FALSE);
	if (self->priv->block_update > 0) {
		self->priv->update_timeout = (guint) 0;
		return FALSE;
	}
	artist = gmpc_metadata_browser_browser_get_selected_artist (self);
	album = gmpc_metadata_browser_browser_get_selected_album (self);
	song = gmpc_metadata_browser_browser_get_selected_song (self);
	if (song != NULL) {
		GtkWidget* view;
		view = gmpc_metadata_browser_metadata_box_show_song (self, song);
		gtk_container_add ((GtkContainer*) self->priv->metadata_box, view);
		gtk_widget_show_all ((GtkWidget*) self->priv->metadata_sw);
		(view == NULL) ? NULL : (view = (g_object_unref (view), NULL));
	} else {
		gboolean _tmp1;
		_tmp1 = FALSE;
		if (album != NULL) {
			_tmp1 = artist != NULL;
		} else {
			_tmp1 = FALSE;
		}
		if (_tmp1) {
			gmpc_metadata_browser_metadata_box_show_album (self, artist, album);
		} else {
			if (artist != NULL) {
				gmpc_metadata_browser_metadata_box_show_artist (self, artist);
			}
		}
	}
	self->priv->update_timeout = (guint) 0;
	return (_tmp2 = FALSE, artist = (g_free (artist), NULL), album = (g_free (album), NULL), (song == NULL) ? NULL : (song = (mpd_freeSong (song), NULL)), _tmp2);
}


/** 
     * Browser Interface bindings
     */
static void gmpc_metadata_browser_real_browser_add (GmpcPluginBrowserIface* base, GtkWidget* category_tree) {
	GmpcMetadataBrowser * self;
	GtkTreeView* _tmp0;
	GtkTreeView* tree;
	GtkListStore* _tmp1;
	GtkListStore* store;
	GtkTreeModel* _tmp2;
	GtkTreeModel* model;
	GtkTreeIter iter = {0};
	GtkTreeRowReference* _tmp4;
	GtkTreePath* _tmp3;
	self = (GmpcMetadataBrowser*) base;
	g_return_if_fail (category_tree != NULL);
	_tmp0 = NULL;
	tree = (_tmp0 = GTK_TREE_VIEW (category_tree), (_tmp0 == NULL) ? NULL : g_object_ref (_tmp0));
	_tmp1 = NULL;
	store = (_tmp1 = GTK_LIST_STORE (gtk_tree_view_get_model (tree)), (_tmp1 == NULL) ? NULL : g_object_ref (_tmp1));
	_tmp2 = NULL;
	model = (_tmp2 = gtk_tree_view_get_model (tree), (_tmp2 == NULL) ? NULL : g_object_ref (_tmp2));
	playlist3_insert_browser (&iter, cfg_get_single_value_as_int_with_default (config, gmpc_plugin_base_get_name ((GmpcPluginBase*) self), "position", 100));
	gtk_list_store_set (store, &iter, 0, ((GmpcPluginBase*) self)->id, 1, _ (gmpc_plugin_base_get_name ((GmpcPluginBase*) self)), 3, "gtk-info", -1);
	/* Create a row reference */
	_tmp4 = NULL;
	_tmp3 = NULL;
	self->priv->rref = (_tmp4 = gtk_tree_row_reference_new (model, _tmp3 = gtk_tree_model_get_path (model, &iter)), (self->priv->rref == NULL) ? NULL : (self->priv->rref = (gtk_tree_row_reference_free (self->priv->rref), NULL)), _tmp4);
	(_tmp3 == NULL) ? NULL : (_tmp3 = (gtk_tree_path_free (_tmp3), NULL));
	(tree == NULL) ? NULL : (tree = (g_object_unref (tree), NULL));
	(store == NULL) ? NULL : (store = (g_object_unref (store), NULL));
	(model == NULL) ? NULL : (model = (g_object_unref (model), NULL));
}


static void gmpc_metadata_browser_real_browser_selected (GmpcPluginBrowserIface* base, GtkContainer* container) {
	GmpcMetadataBrowser * self;
	char* artist;
	char* _tmp0;
	self = (GmpcMetadataBrowser*) base;
	g_return_if_fail (container != NULL);
	artist = NULL;
	self->priv->selected = TRUE;
	gmpc_metadata_browser_browser_init (self);
	gtk_container_add (container, (GtkWidget*) self->priv->paned);
	/* update if non selected */
	_tmp0 = NULL;
	artist = (_tmp0 = gmpc_metadata_browser_browser_get_selected_artist (self), artist = (g_free (artist), NULL), _tmp0);
	if (artist == NULL) {
		gmpc_metadata_browser_metadata_box_clear (self);
		gmpc_metadata_browser_metadata_box_update (self);
	}
	artist = (g_free (artist), NULL);
}


static void gmpc_metadata_browser_real_browser_unselected (GmpcPluginBrowserIface* base, GtkContainer* container) {
	GmpcMetadataBrowser * self;
	self = (GmpcMetadataBrowser*) base;
	g_return_if_fail (container != NULL);
	self->priv->selected = FALSE;
	gtk_container_remove (container, (GtkWidget*) self->priv->paned);
}


static void gmpc_metadata_browser_con_changed (GmpcMetadataBrowser* self, GmpcConnection* conn, MpdObj* server, gint connect) {
	g_return_if_fail (self != NULL);
	g_return_if_fail (conn != NULL);
	g_return_if_fail (server != NULL);
	if (self->priv->paned == NULL) {
		return;
	}
	gmpc_metadata_browser_reload_browsers (self);
	gmpc_metadata_browser_metadata_box_clear (self);
	gmpc_metadata_browser_metadata_box_update (self);
}


static void gmpc_metadata_browser_status_changed (GmpcMetadataBrowser* self, GmpcConnection* conn, MpdObj* server, ChangedStatusType what) {
	g_return_if_fail (self != NULL);
	g_return_if_fail (conn != NULL);
	g_return_if_fail (server != NULL);
	if (self->priv->paned == NULL) {
		return;
	}
}


void gmpc_metadata_browser_set_artist (GmpcMetadataBrowser* self, const char* artist) {
	GtkTreeIter iter = {0};
	g_return_if_fail (self != NULL);
	g_return_if_fail (artist != NULL);
	self->priv->block_update++;
	gtk_tree_selection_unselect_all (gtk_tree_view_get_selection (self->priv->tree_artist));
	gtk_tree_selection_unselect_all (gtk_tree_view_get_selection (self->priv->tree_album));
	/* clear */
	gtk_entry_set_text (self->priv->artist_filter_entry, "");
	if (gtk_tree_model_get_iter_first ((GtkTreeModel*) self->priv->model_filter_artist, &iter)) {
		do {
			char* lartist;
			gboolean _tmp0;
			lartist = NULL;
			gtk_tree_model_get ((GtkTreeModel*) self->priv->model_filter_artist, &iter, 7, &lartist, -1, -1);
			_tmp0 = FALSE;
			if (lartist != NULL) {
				_tmp0 = g_utf8_collate (lartist, artist) == 0;
			} else {
				_tmp0 = FALSE;
			}
			if (_tmp0) {
				GtkTreePath* _tmp1;
				gtk_tree_selection_select_iter (gtk_tree_view_get_selection (self->priv->tree_artist), &iter);
				_tmp1 = NULL;
				gtk_tree_view_scroll_to_cell (self->priv->tree_artist, _tmp1 = gtk_tree_model_get_path ((GtkTreeModel*) self->priv->model_filter_artist, &iter), NULL, TRUE, 0.5f, 0.f);
				(_tmp1 == NULL) ? NULL : (_tmp1 = (gtk_tree_path_free (_tmp1), NULL));
				self->priv->block_update--;
				gmpc_metadata_browser_metadata_box_clear (self);
				gmpc_metadata_browser_metadata_box_update (self);
				lartist = (g_free (lartist), NULL);
				return;
			}
			lartist = (g_free (lartist), NULL);
		} while (gtk_tree_model_iter_next ((GtkTreeModel*) self->priv->model_filter_artist, &iter));
	}
	self->priv->block_update--;
	gmpc_metadata_browser_metadata_box_clear (self);
	gmpc_metadata_browser_metadata_box_update (self);
}


void gmpc_metadata_browser_set_album (GmpcMetadataBrowser* self, const char* artist, const char* album) {
	GtkTreeIter iter = {0};
	g_return_if_fail (self != NULL);
	g_return_if_fail (artist != NULL);
	g_return_if_fail (album != NULL);
	self->priv->block_update++;
	gmpc_metadata_browser_set_artist (self, artist);
	/* clear */
	gtk_entry_set_text (self->priv->album_filter_entry, "");
	if (gtk_tree_model_get_iter_first ((GtkTreeModel*) self->priv->model_filter_album, &iter)) {
		do {
			char* lalbum;
			gboolean _tmp0;
			lalbum = NULL;
			gtk_tree_model_get ((GtkTreeModel*) self->priv->model_filter_album, &iter, 6, &lalbum, -1, -1);
			_tmp0 = FALSE;
			if (lalbum != NULL) {
				_tmp0 = g_utf8_collate (lalbum, album) == 0;
			} else {
				_tmp0 = FALSE;
			}
			if (_tmp0) {
				GtkTreePath* _tmp1;
				gtk_tree_selection_select_iter (gtk_tree_view_get_selection (self->priv->tree_album), &iter);
				_tmp1 = NULL;
				gtk_tree_view_scroll_to_cell (self->priv->tree_album, _tmp1 = gtk_tree_model_get_path ((GtkTreeModel*) self->priv->model_filter_album, &iter), NULL, TRUE, 0.5f, 0.f);
				(_tmp1 == NULL) ? NULL : (_tmp1 = (gtk_tree_path_free (_tmp1), NULL));
				gtk_tree_selection_unselect_all (gtk_tree_view_get_selection (self->priv->tree_songs));
				self->priv->block_update--;
				gmpc_metadata_browser_metadata_box_update (self);
				lalbum = (g_free (lalbum), NULL);
				return;
			}
			lalbum = (g_free (lalbum), NULL);
		} while (gtk_tree_model_iter_next ((GtkTreeModel*) self->priv->model_filter_album, &iter));
	}
	gtk_tree_selection_unselect_all (gtk_tree_view_get_selection (self->priv->tree_songs));
	self->priv->block_update--;
	gmpc_metadata_browser_metadata_box_clear (self);
	gmpc_metadata_browser_metadata_box_update (self);
}


void gmpc_metadata_browser_set_song (GmpcMetadataBrowser* self, const mpd_Song* song) {
	GtkWidget* view;
	g_return_if_fail (self != NULL);
	g_return_if_fail (song != NULL);
	self->priv->block_update++;
	if (song->artist != NULL) {
		gmpc_metadata_browser_set_artist (self, song->artist);
		if (song->album != NULL) {
			gmpc_metadata_browser_set_album (self, song->artist, song->album);
		}
	}
	self->priv->block_update--;
	gmpc_metadata_browser_metadata_box_clear (self);
	if (self->priv->update_timeout > 0) {
		g_source_remove (self->priv->update_timeout);
		self->priv->update_timeout = (guint) 0;
	}
	view = gmpc_metadata_browser_metadata_box_show_song (self, song);
	gtk_container_add ((GtkContainer*) self->priv->metadata_box, view);
	gtk_widget_show_all ((GtkWidget*) self->priv->metadata_box);
	(view == NULL) ? NULL : (view = (g_object_unref (view), NULL));
}


void gmpc_metadata_browser_select_browser (GmpcMetadataBrowser* self, GtkTreeView* tree) {
	GtkTreePath* path;
	GtkTreeModel* _tmp0;
	GtkTreeModel* model;
	g_return_if_fail (self != NULL);
	g_return_if_fail (tree != NULL);
	path = gtk_tree_row_reference_get_path (self->priv->rref);
	_tmp0 = NULL;
	model = (_tmp0 = gtk_tree_row_reference_get_model (self->priv->rref), (_tmp0 == NULL) ? NULL : g_object_ref (_tmp0));
	if (path != NULL) {
		GtkTreeIter iter = {0};
		if (gtk_tree_model_get_iter (model, &iter, path)) {
			gtk_tree_selection_select_iter (gtk_tree_view_get_selection (tree), &iter);
		}
	}
	(path == NULL) ? NULL : (path = (gtk_tree_path_free (path), NULL));
	(model == NULL) ? NULL : (model = (g_object_unref (model), NULL));
}


GmpcMetadataBrowser* gmpc_metadata_browser_construct (GType object_type) {
	GmpcMetadataBrowser * self;
	self = g_object_newv (object_type, 0, NULL);
	return self;
}


GmpcMetadataBrowser* gmpc_metadata_browser_new (void) {
	return gmpc_metadata_browser_construct (GMPC_TYPE_METADATA_BROWSER);
}


static void _gmpc_metadata_browser_con_changed_gmpc_connection_connection_changed (GmpcConnection* _sender, MpdObj* server, gint connect, gpointer self) {
	gmpc_metadata_browser_con_changed (self, _sender, server, connect);
}


static void _gmpc_metadata_browser_status_changed_gmpc_connection_status_changed (GmpcConnection* _sender, MpdObj* server, ChangedStatusType what, gpointer self) {
	gmpc_metadata_browser_status_changed (self, _sender, server, what);
}


static GObject * gmpc_metadata_browser_constructor (GType type, guint n_construct_properties, GObjectConstructParam * construct_properties) {
	GObject * obj;
	GmpcMetadataBrowserClass * klass;
	GObjectClass * parent_class;
	GmpcMetadataBrowser * self;
	klass = GMPC_METADATA_BROWSER_CLASS (g_type_class_peek (GMPC_TYPE_METADATA_BROWSER));
	parent_class = G_OBJECT_CLASS (g_type_class_peek_parent (klass));
	obj = parent_class->constructor (type, n_construct_properties, construct_properties);
	self = GMPC_METADATA_BROWSER (obj);
	{
		/* Set the plugin as an internal one and of type pl_browser */
		((GmpcPluginBase*) self)->plugin_type = 2 | 8;
		g_signal_connect_object (gmpcconn, "connection-changed", (GCallback) _gmpc_metadata_browser_con_changed_gmpc_connection_connection_changed, self, 0);
		g_signal_connect_object (gmpcconn, "status-changed", (GCallback) _gmpc_metadata_browser_status_changed_gmpc_connection_status_changed, self, 0);
	}
	return obj;
}


static void gmpc_metadata_browser_class_init (GmpcMetadataBrowserClass * klass) {
	gmpc_metadata_browser_parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (GmpcMetadataBrowserPrivate));
	G_OBJECT_CLASS (klass)->constructor = gmpc_metadata_browser_constructor;
	G_OBJECT_CLASS (klass)->finalize = gmpc_metadata_browser_finalize;
	GMPC_PLUGIN_BASE_CLASS (klass)->get_version = gmpc_metadata_browser_real_get_version;
	GMPC_PLUGIN_BASE_CLASS (klass)->get_name = gmpc_metadata_browser_real_get_name;
	GMPC_PLUGIN_BASE_CLASS (klass)->save_yourself = gmpc_metadata_browser_real_save_yourself;
}


static void gmpc_metadata_browser_gmpc_plugin_browser_iface_interface_init (GmpcPluginBrowserIfaceIface * iface) {
	gmpc_metadata_browser_gmpc_plugin_browser_iface_parent_iface = g_type_interface_peek_parent (iface);
	iface->browser_add = gmpc_metadata_browser_real_browser_add;
	iface->browser_selected = gmpc_metadata_browser_real_browser_selected;
	iface->browser_unselected = gmpc_metadata_browser_real_browser_unselected;
}


static void gmpc_metadata_browser_instance_init (GmpcMetadataBrowser * self) {
	self->priv = GMPC_METADATA_BROWSER_GET_PRIVATE (self);
	self->priv->block_update = 0;
	self->priv->rref = NULL;
	self->priv->paned = NULL;
	self->priv->browser_box = NULL;
	self->priv->tree_artist = NULL;
	self->priv->model_artist = NULL;
	self->priv->model_filter_artist = NULL;
	self->priv->artist_filter_entry = NULL;
	self->priv->tree_album = NULL;
	self->priv->model_albums = NULL;
	self->priv->model_filter_album = NULL;
	self->priv->album_filter_entry = NULL;
	self->priv->tree_songs = NULL;
	self->priv->model_songs = NULL;
	self->priv->metadata_sw = NULL;
	self->priv->metadata_box = NULL;
	self->priv->update_timeout = (guint) 0;
	self->priv->selected = FALSE;
}


static void gmpc_metadata_browser_finalize (GObject* obj) {
	GmpcMetadataBrowser * self;
	self = GMPC_METADATA_BROWSER (obj);
	(self->priv->rref == NULL) ? NULL : (self->priv->rref = (gtk_tree_row_reference_free (self->priv->rref), NULL));
	(self->priv->paned == NULL) ? NULL : (self->priv->paned = (g_object_unref (self->priv->paned), NULL));
	(self->priv->browser_box == NULL) ? NULL : (self->priv->browser_box = (g_object_unref (self->priv->browser_box), NULL));
	(self->priv->tree_artist == NULL) ? NULL : (self->priv->tree_artist = (g_object_unref (self->priv->tree_artist), NULL));
	(self->priv->model_artist == NULL) ? NULL : (self->priv->model_artist = (g_object_unref (self->priv->model_artist), NULL));
	(self->priv->model_filter_artist == NULL) ? NULL : (self->priv->model_filter_artist = (g_object_unref (self->priv->model_filter_artist), NULL));
	(self->priv->artist_filter_entry == NULL) ? NULL : (self->priv->artist_filter_entry = (g_object_unref (self->priv->artist_filter_entry), NULL));
	(self->priv->tree_album == NULL) ? NULL : (self->priv->tree_album = (g_object_unref (self->priv->tree_album), NULL));
	(self->priv->model_albums == NULL) ? NULL : (self->priv->model_albums = (g_object_unref (self->priv->model_albums), NULL));
	(self->priv->model_filter_album == NULL) ? NULL : (self->priv->model_filter_album = (g_object_unref (self->priv->model_filter_album), NULL));
	(self->priv->album_filter_entry == NULL) ? NULL : (self->priv->album_filter_entry = (g_object_unref (self->priv->album_filter_entry), NULL));
	(self->priv->tree_songs == NULL) ? NULL : (self->priv->tree_songs = (g_object_unref (self->priv->tree_songs), NULL));
	(self->priv->model_songs == NULL) ? NULL : (self->priv->model_songs = (g_object_unref (self->priv->model_songs), NULL));
	(self->priv->metadata_sw == NULL) ? NULL : (self->priv->metadata_sw = (g_object_unref (self->priv->metadata_sw), NULL));
	(self->priv->metadata_box == NULL) ? NULL : (self->priv->metadata_box = (g_object_unref (self->priv->metadata_box), NULL));
	G_OBJECT_CLASS (gmpc_metadata_browser_parent_class)->finalize (obj);
}


GType gmpc_metadata_browser_get_type (void) {
	static GType gmpc_metadata_browser_type_id = 0;
	if (gmpc_metadata_browser_type_id == 0) {
		static const GTypeInfo g_define_type_info = { sizeof (GmpcMetadataBrowserClass), (GBaseInitFunc) NULL, (GBaseFinalizeFunc) NULL, (GClassInitFunc) gmpc_metadata_browser_class_init, (GClassFinalizeFunc) NULL, NULL, sizeof (GmpcMetadataBrowser), 0, (GInstanceInitFunc) gmpc_metadata_browser_instance_init, NULL };
		static const GInterfaceInfo gmpc_plugin_browser_iface_info = { (GInterfaceInitFunc) gmpc_metadata_browser_gmpc_plugin_browser_iface_interface_init, (GInterfaceFinalizeFunc) NULL, NULL};
		gmpc_metadata_browser_type_id = g_type_register_static (GMPC_PLUGIN_TYPE_BASE, "GmpcMetadataBrowser", &g_define_type_info, 0);
		g_type_add_interface_static (gmpc_metadata_browser_type_id, GMPC_PLUGIN_TYPE_BROWSER_IFACE, &gmpc_plugin_browser_iface_info);
	}
	return gmpc_metadata_browser_type_id;
}


static void _vala_array_free (gpointer array, gint array_length, GDestroyNotify destroy_func) {
	if ((array != NULL) && (destroy_func != NULL)) {
		int i;
		for (i = 0; i < array_length; i = i + 1) {
			if (((gpointer*) array)[i] != NULL) {
				destroy_func (((gpointer*) array)[i]);
			}
		}
	}
	g_free (array);
}


static gint _vala_array_length (gpointer array) {
	int length;
	length = 0;
	if (array) {
		while (((gpointer*) array)[length]) {
			length++;
		}
	}
	return length;
}


static int _vala_strcmp0 (const char * str1, const char * str2) {
	if (str1 == NULL) {
		return -(str1 != str2);
	}
	if (str2 == NULL) {
		return str1 != str2;
	}
	return strcmp (str1, str2);
}




