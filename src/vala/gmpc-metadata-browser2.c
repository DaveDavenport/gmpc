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
#include <libmpd/libmpdclient.h>
#include <libmpd/libmpd.h>
#include <metadata.h>
#include <stdlib.h>
#include <string.h>
#include <glib/gi18n-lib.h>
#include <main.h>
#include <mpdinteraction.h>
#include <gdk/gdk.h>
#include <gmpc-mpddata-model.h>
#include <gmpc-meta-watcher.h>
#include <gmpc-plugin.h>
#include <misc.h>
#include <gmpc-metaimage.h>
#include <pango/pango.h>
#include <plugin.h>
#include <config1.h>
#include <gmpc-connection.h>
#include <float.h>
#include <math.h>
#include <gdk-pixbuf/gdk-pixdata.h>
#include <gmpc-paned-size-group.h>
#include <gmpc-mpddata-treeview-tooltip.h>
#include <time.h>
#include <stdio.h>
#include <gmpc-favorites.h>
#include <gmpc-rating.h>
#include <gmpc-meta-text-view.h>
#include <gmpc-song-links.h>
#include <gmpc-stats-label.h>


#define GMPC_WIDGET_TYPE_SIMILAR_SONGS (gmpc_widget_similar_songs_get_type ())
#define GMPC_WIDGET_SIMILAR_SONGS(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GMPC_WIDGET_TYPE_SIMILAR_SONGS, GmpcWidgetSimilarSongs))
#define GMPC_WIDGET_SIMILAR_SONGS_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GMPC_WIDGET_TYPE_SIMILAR_SONGS, GmpcWidgetSimilarSongsClass))
#define GMPC_WIDGET_IS_SIMILAR_SONGS(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GMPC_WIDGET_TYPE_SIMILAR_SONGS))
#define GMPC_WIDGET_IS_SIMILAR_SONGS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GMPC_WIDGET_TYPE_SIMILAR_SONGS))
#define GMPC_WIDGET_SIMILAR_SONGS_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GMPC_WIDGET_TYPE_SIMILAR_SONGS, GmpcWidgetSimilarSongsClass))

typedef struct _GmpcWidgetSimilarSongs GmpcWidgetSimilarSongs;
typedef struct _GmpcWidgetSimilarSongsClass GmpcWidgetSimilarSongsClass;
typedef struct _GmpcWidgetSimilarSongsPrivate GmpcWidgetSimilarSongsPrivate;

#define GMPC_WIDGET_TYPE_SIMILAR_ARTIST (gmpc_widget_similar_artist_get_type ())
#define GMPC_WIDGET_SIMILAR_ARTIST(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GMPC_WIDGET_TYPE_SIMILAR_ARTIST, GmpcWidgetSimilarArtist))
#define GMPC_WIDGET_SIMILAR_ARTIST_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GMPC_WIDGET_TYPE_SIMILAR_ARTIST, GmpcWidgetSimilarArtistClass))
#define GMPC_WIDGET_IS_SIMILAR_ARTIST(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GMPC_WIDGET_TYPE_SIMILAR_ARTIST))
#define GMPC_WIDGET_IS_SIMILAR_ARTIST_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GMPC_WIDGET_TYPE_SIMILAR_ARTIST))
#define GMPC_WIDGET_SIMILAR_ARTIST_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GMPC_WIDGET_TYPE_SIMILAR_ARTIST, GmpcWidgetSimilarArtistClass))

typedef struct _GmpcWidgetSimilarArtist GmpcWidgetSimilarArtist;
typedef struct _GmpcWidgetSimilarArtistClass GmpcWidgetSimilarArtistClass;
typedef struct _GmpcWidgetSimilarArtistPrivate GmpcWidgetSimilarArtistPrivate;

#define GMPC_TYPE_METADATA_BROWSER (gmpc_metadata_browser_get_type ())
#define GMPC_METADATA_BROWSER(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GMPC_TYPE_METADATA_BROWSER, GmpcMetadataBrowser))
#define GMPC_METADATA_BROWSER_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GMPC_TYPE_METADATA_BROWSER, GmpcMetadataBrowserClass))
#define GMPC_IS_METADATA_BROWSER(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GMPC_TYPE_METADATA_BROWSER))
#define GMPC_IS_METADATA_BROWSER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GMPC_TYPE_METADATA_BROWSER))
#define GMPC_METADATA_BROWSER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GMPC_TYPE_METADATA_BROWSER, GmpcMetadataBrowserClass))

typedef struct _GmpcMetadataBrowser GmpcMetadataBrowser;
typedef struct _GmpcMetadataBrowserClass GmpcMetadataBrowserClass;

#define GMPC_WIDGET_TYPE_MORE (gmpc_widget_more_get_type ())
#define GMPC_WIDGET_MORE(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GMPC_WIDGET_TYPE_MORE, GmpcWidgetMore))
#define GMPC_WIDGET_MORE_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GMPC_WIDGET_TYPE_MORE, GmpcWidgetMoreClass))
#define GMPC_WIDGET_IS_MORE(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GMPC_WIDGET_TYPE_MORE))
#define GMPC_WIDGET_IS_MORE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GMPC_WIDGET_TYPE_MORE))
#define GMPC_WIDGET_MORE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GMPC_WIDGET_TYPE_MORE, GmpcWidgetMoreClass))

typedef struct _GmpcWidgetMore GmpcWidgetMore;
typedef struct _GmpcWidgetMoreClass GmpcWidgetMoreClass;
typedef struct _GmpcWidgetMorePrivate GmpcWidgetMorePrivate;

#define GMPC_TYPE_NOW_PLAYING (gmpc_now_playing_get_type ())
#define GMPC_NOW_PLAYING(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GMPC_TYPE_NOW_PLAYING, GmpcNowPlaying))
#define GMPC_NOW_PLAYING_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GMPC_TYPE_NOW_PLAYING, GmpcNowPlayingClass))
#define GMPC_IS_NOW_PLAYING(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GMPC_TYPE_NOW_PLAYING))
#define GMPC_IS_NOW_PLAYING_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GMPC_TYPE_NOW_PLAYING))
#define GMPC_NOW_PLAYING_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GMPC_TYPE_NOW_PLAYING, GmpcNowPlayingClass))

typedef struct _GmpcNowPlaying GmpcNowPlaying;
typedef struct _GmpcNowPlayingClass GmpcNowPlayingClass;
typedef struct _GmpcNowPlayingPrivate GmpcNowPlayingPrivate;
typedef struct _GmpcMetadataBrowserPrivate GmpcMetadataBrowserPrivate;

#define GMPC_METADATA_BROWSER_TYPE_HITEM (gmpc_metadata_browser_hitem_get_type ())

#define GMPC_METADATA_BROWSER_TYPE_HITEM_TYPE (gmpc_metadata_browser_hitem_type_get_type ())
typedef struct _GmpcMetadataBrowserHitem GmpcMetadataBrowserHitem;

struct _GmpcWidgetSimilarSongs {
	GtkExpander parent_instance;
	GmpcWidgetSimilarSongsPrivate * priv;
};

struct _GmpcWidgetSimilarSongsClass {
	GtkExpanderClass parent_class;
};

struct _GmpcWidgetSimilarSongsPrivate {
	mpd_Song* song;
	gboolean filled;
	GtkWidget* pchild;
	guint idle_add;
	MetaData* copy;
	MpdData* item;
	GList* current;
};

struct _GmpcWidgetSimilarArtist {
	GtkTable parent_instance;
	GmpcWidgetSimilarArtistPrivate * priv;
};

struct _GmpcWidgetSimilarArtistClass {
	GtkTableClass parent_class;
};

struct _GmpcWidgetSimilarArtistPrivate {
	mpd_Song* song;
	GmpcMetadataBrowser* browser;
};

/**
 * The "More" Widget. This collapses it child and adds a more/less button.
 * Using the unique_id it stores the state for the next time.
 */
struct _GmpcWidgetMore {
	GtkFrame parent_instance;
	GmpcWidgetMorePrivate * priv;
};

struct _GmpcWidgetMoreClass {
	GtkFrameClass parent_class;
};

struct _GmpcWidgetMorePrivate {
	GtkAlignment* ali;
	gint expand_state;
	GtkButton* expand_button;
	gint max_height;
	GtkEventBox* eventbox;
	GtkWidget* pchild;
	char* unique_id;
};

/**
 * Now playing uses the MetaDataBrowser plugin to "plot" the view */
struct _GmpcNowPlaying {
	GmpcPluginBase parent_instance;
	GmpcNowPlayingPrivate * priv;
};

struct _GmpcNowPlayingClass {
	GmpcPluginBaseClass parent_class;
};

struct _GmpcNowPlayingPrivate {
	GtkTreeRowReference* np_ref;
	GmpcMetadataBrowser* browser;
	GtkScrolledWindow* paned;
	GtkEventBox* container;
	gboolean selected;
	char* song_checksum;
};

struct _GmpcMetadataBrowser {
	GmpcPluginBase parent_instance;
	GmpcMetadataBrowserPrivate * priv;
};

struct _GmpcMetadataBrowserClass {
	GmpcPluginBaseClass parent_class;
};

/**
     * History 
     */
typedef enum  {
	GMPC_METADATA_BROWSER_HITEM_TYPE_CLEAR,
	GMPC_METADATA_BROWSER_HITEM_TYPE_ARTIST,
	GMPC_METADATA_BROWSER_HITEM_TYPE_ALBUM,
	GMPC_METADATA_BROWSER_HITEM_TYPE_SONG
} GmpcMetadataBrowserHitemType;

struct _GmpcMetadataBrowserHitem {
	GmpcMetadataBrowserHitemType type;
	mpd_Song* song;
};

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
	GList* history;
	GList* current;
};


static gpointer gmpc_widget_similar_songs_parent_class = NULL;
static gpointer gmpc_widget_similar_artist_parent_class = NULL;
static gpointer gmpc_widget_more_parent_class = NULL;
static gpointer gmpc_now_playing_parent_class = NULL;
static GmpcPluginBrowserIfaceIface* gmpc_now_playing_gmpc_plugin_browser_iface_parent_iface = NULL;
static gpointer gmpc_metadata_browser_parent_class = NULL;
static GmpcPluginBrowserIfaceIface* gmpc_metadata_browser_gmpc_plugin_browser_iface_parent_iface = NULL;
static GmpcPluginPreferencesIfaceIface* gmpc_metadata_browser_gmpc_plugin_preferences_iface_parent_iface = NULL;

#define use_transition_mb TRUE
#define some_unique_name_mb VERSION
GType gmpc_widget_similar_songs_get_type (void);
#define GMPC_WIDGET_SIMILAR_SONGS_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GMPC_WIDGET_TYPE_SIMILAR_SONGS, GmpcWidgetSimilarSongsPrivate))
enum  {
	GMPC_WIDGET_SIMILAR_SONGS_DUMMY_PROPERTY
};
static GmpcWidgetSimilarSongs* gmpc_widget_similar_songs_new (const mpd_Song* song);
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
static void gmpc_widget_similar_songs_metadata_changed (GmpcWidgetSimilarSongs* self, GmpcMetaWatcher* gmw, const mpd_Song* song, MetaDataType type, MetaDataResult _result_, const MetaData* met);
static void _gmpc_widget_similar_songs_metadata_changed_gmpc_meta_watcher_data_changed (GmpcMetaWatcher* _sender, const mpd_Song* song, MetaDataType type, MetaDataResult _result_, const MetaData* met, gpointer self);
static void gmpc_widget_similar_songs_update (GmpcWidgetSimilarSongs* self);
static void gmpc_widget_similar_songs_real_activate (GtkExpander* base);
static void gmpc_widget_similar_songs_finalize (GObject* obj);
GType gmpc_widget_similar_artist_get_type (void);
GType gmpc_metadata_browser_get_type (void);
#define GMPC_WIDGET_SIMILAR_ARTIST_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GMPC_WIDGET_TYPE_SIMILAR_ARTIST, GmpcWidgetSimilarArtistPrivate))
enum  {
	GMPC_WIDGET_SIMILAR_ARTIST_DUMMY_PROPERTY
};
GtkWidget* gmpc_widget_similar_artist_new_artist_button (GmpcWidgetSimilarArtist* self, const char* artist, gboolean in_db);
static void _g_list_free_g_object_unref (GList* self);
static void gmpc_widget_similar_artist_metadata_changed (GmpcWidgetSimilarArtist* self, GmpcMetaWatcher* gmw, const mpd_Song* song, MetaDataType type, MetaDataResult _result_, const MetaData* met);
void gmpc_metadata_browser_set_artist (GmpcMetadataBrowser* self, const char* artist);
static void gmpc_widget_similar_artist_artist_button_clicked (GmpcWidgetSimilarArtist* self, GtkButton* button);
static gboolean _misc_header_expose_event_gtk_widget_expose_event (GtkWidget* _sender, const GdkEventExpose* event, gpointer self);
static void _gmpc_widget_similar_artist_artist_button_clicked_gtk_button_clicked (GtkButton* _sender, gpointer self);
static void _gmpc_widget_similar_artist_metadata_changed_gmpc_meta_watcher_data_changed (GmpcMetaWatcher* _sender, const mpd_Song* song, MetaDataType type, MetaDataResult _result_, const MetaData* met, gpointer self);
static GmpcWidgetSimilarArtist* gmpc_widget_similar_artist_new (GmpcMetadataBrowser* browser, MpdObj* server, const mpd_Song* song);
static GmpcWidgetSimilarArtist* gmpc_widget_similar_artist_construct (GType object_type, GmpcMetadataBrowser* browser, MpdObj* server, const mpd_Song* song);
static GmpcWidgetSimilarArtist* gmpc_widget_similar_artist_new (GmpcMetadataBrowser* browser, MpdObj* server, const mpd_Song* song);
static void gmpc_widget_similar_artist_finalize (GObject* obj);
GType gmpc_widget_more_get_type (void);
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
static GmpcWidgetMore* gmpc_widget_more_new (const char* unique_id, const char* markup, GtkWidget* child);
static GmpcWidgetMore* gmpc_widget_more_construct (GType object_type, const char* unique_id, const char* markup, GtkWidget* child);
static GmpcWidgetMore* gmpc_widget_more_new (const char* unique_id, const char* markup, GtkWidget* child);
static void gmpc_widget_more_finalize (GObject* obj);
GType gmpc_now_playing_get_type (void);
#define GMPC_NOW_PLAYING_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GMPC_TYPE_NOW_PLAYING, GmpcNowPlayingPrivate))
enum  {
	GMPC_NOW_PLAYING_DUMMY_PROPERTY
};
static gint gmpc_now_playing_real_get_version (GmpcPluginBase* base, int* result_length1);
static const char* gmpc_now_playing_real_get_name (GmpcPluginBase* base);
static void gmpc_now_playing_real_save_yourself (GmpcPluginBase* base);
static void gmpc_now_playing_update (GmpcNowPlaying* self);
static void gmpc_now_playing_status_changed (GmpcNowPlaying* self, GmpcConnection* conn, MpdObj* server, ChangedStatusType what);
static void gmpc_now_playing_real_browser_add (GmpcPluginBrowserIface* base, GtkWidget* category_tree);
static void gmpc_now_playing_browser_init (GmpcNowPlaying* self);
static void gmpc_now_playing_real_browser_selected (GmpcPluginBrowserIface* base, GtkContainer* container);
static void gmpc_now_playing_real_browser_unselected (GmpcPluginBrowserIface* base, GtkContainer* container);
static void gmpc_now_playing_browser_bg_style_changed (GmpcNowPlaying* self, GtkScrolledWindow* bg, GtkStyle* style);
static gboolean gmpc_now_playing_browser_key_release_event (GmpcNowPlaying* self, const GdkEventKey* event);
static void _gmpc_now_playing_browser_bg_style_changed_gtk_widget_style_set (GtkScrolledWindow* _sender, GtkStyle* previous_style, gpointer self);
static gboolean _gmpc_now_playing_browser_key_release_event_gtk_widget_key_release_event (GtkScrolledWindow* _sender, const GdkEventKey* event, gpointer self);
GtkWidget* gmpc_metadata_browser_metadata_box_show_song (GmpcMetadataBrowser* self, const mpd_Song* song, gboolean show_controls);
static void gmpc_now_playing_select_now_playing_browser (GmpcNowPlaying* self, GtkImageMenuItem* item);
static void _gmpc_now_playing_select_now_playing_browser_gtk_menu_item_activate (GtkImageMenuItem* _sender, gpointer self);
static gint gmpc_now_playing_real_browser_add_go_menu (GmpcPluginBrowserIface* base, GtkMenu* menu);
GmpcNowPlaying* gmpc_now_playing_new (void);
GmpcNowPlaying* gmpc_now_playing_construct (GType object_type);
GmpcNowPlaying* gmpc_now_playing_new (void);
static void _gmpc_now_playing_status_changed_gmpc_connection_status_changed (GmpcConnection* _sender, MpdObj* server, ChangedStatusType what, gpointer self);
GmpcMetadataBrowser* gmpc_metadata_browser_new (void);
GmpcMetadataBrowser* gmpc_metadata_browser_construct (GType object_type);
static GObject * gmpc_now_playing_constructor (GType type, guint n_construct_properties, GObjectConstructParam * construct_properties);
static void gmpc_now_playing_finalize (GObject* obj);
static GType gmpc_metadata_browser_hitem_get_type (void);
static GType gmpc_metadata_browser_hitem_type_get_type (void);
static GmpcMetadataBrowserHitem* gmpc_metadata_browser_hitem_dup (const GmpcMetadataBrowserHitem* self);
static void gmpc_metadata_browser_hitem_free (GmpcMetadataBrowserHitem* self);
static void gmpc_metadata_browser_hitem_copy (const GmpcMetadataBrowserHitem* self, GmpcMetadataBrowserHitem* dest);
static void gmpc_metadata_browser_hitem_destroy (GmpcMetadataBrowserHitem* self);
#define GMPC_METADATA_BROWSER_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GMPC_TYPE_METADATA_BROWSER, GmpcMetadataBrowserPrivate))
enum  {
	GMPC_METADATA_BROWSER_DUMMY_PROPERTY
};
static void _g_list_free_gmpc_metadata_browser_hitem_free (GList* self);
static gint gmpc_metadata_browser_real_get_version (GmpcPluginBase* base, int* result_length1);
static const char* gmpc_metadata_browser_real_get_name (GmpcPluginBase* base);
static void gmpc_metadata_browser_real_save_yourself (GmpcPluginBase* base);
void gmpc_metadata_browser_select_browser (GmpcMetadataBrowser* self, GtkTreeView* tree);
static void gmpc_metadata_browser_select_metadata_browser (GmpcMetadataBrowser* self, GtkImageMenuItem* item);
static void _gmpc_metadata_browser_select_metadata_browser_gtk_menu_item_activate (GtkImageMenuItem* _sender, gpointer self);
static gint gmpc_metadata_browser_real_browser_add_go_menu (GmpcPluginBrowserIface* base, GtkMenu* menu);
static void gmpc_metadata_browser_browser_bg_style_changed (GmpcMetadataBrowser* self, GtkScrolledWindow* bg, GtkStyle* style);
static gboolean gmpc_metadata_browser_browser_button_press_event (GmpcMetadataBrowser* self, GtkTreeView* tree, const GdkEventButton* event);
static void gmpc_metadata_browser_browser_artist_entry_changed (GmpcMetadataBrowser* self, GtkEntry* entry);
static char* gmpc_metadata_browser_browser_get_selected_artist (GmpcMetadataBrowser* self);
static void gmpc_metadata_browser_artist_add_clicked (GmpcMetadataBrowser* self, GtkImageMenuItem* item);
static void gmpc_metadata_browser_artist_replace_clicked (GmpcMetadataBrowser* self, GtkImageMenuItem* item);
static void _gmpc_metadata_browser_artist_add_clicked_gtk_menu_item_activate (GtkImageMenuItem* _sender, gpointer self);
static void _gmpc_metadata_browser_artist_replace_clicked_gtk_menu_item_activate (GtkImageMenuItem* _sender, gpointer self);
static gboolean gmpc_metadata_browser_artist_browser_button_release_event (GmpcMetadataBrowser* self, GtkTreeView* tree, const GdkEventButton* event);
static gboolean gmpc_metadata_browser_visible_func_artist (GmpcMetadataBrowser* self, GtkTreeModel* model, GtkTreeIter* iter);
static gboolean gmpc_metadata_browser_browser_artist_key_press_event (GmpcMetadataBrowser* self, GtkTreeView* widget, const GdkEventKey* event);
static char* gmpc_metadata_browser_browser_get_selected_album (GmpcMetadataBrowser* self);
static void gmpc_metadata_browser_album_add_clicked (GmpcMetadataBrowser* self, GtkImageMenuItem* item);
static void gmpc_metadata_browser_album_replace_clicked (GmpcMetadataBrowser* self, GtkImageMenuItem* item);
static void _gmpc_metadata_browser_album_add_clicked_gtk_menu_item_activate (GtkImageMenuItem* _sender, gpointer self);
static void _gmpc_metadata_browser_album_replace_clicked_gtk_menu_item_activate (GtkImageMenuItem* _sender, gpointer self);
static gboolean gmpc_metadata_browser_album_browser_button_release_event (GmpcMetadataBrowser* self, GtkTreeView* tree, const GdkEventButton* event);
static gboolean gmpc_metadata_browser_visible_func_album (GmpcMetadataBrowser* self, GtkTreeModel* model, GtkTreeIter* iter);
static gboolean gmpc_metadata_browser_browser_album_key_press_event (GmpcMetadataBrowser* self, GtkTreeView* widget, const GdkEventKey* event);
static void gmpc_metadata_browser_browser_album_entry_changed (GmpcMetadataBrowser* self, GtkEntry* entry);
static mpd_Song* gmpc_metadata_browser_browser_get_selected_song (GmpcMetadataBrowser* self);
static void gmpc_metadata_browser_song_add_clicked (GmpcMetadataBrowser* self, GtkImageMenuItem* item);
static void gmpc_metadata_browser_song_replace_clicked (GmpcMetadataBrowser* self, GtkImageMenuItem* item);
static void _gmpc_metadata_browser_song_add_clicked_gtk_menu_item_activate (GtkImageMenuItem* _sender, gpointer self);
static void _gmpc_metadata_browser_song_replace_clicked_gtk_menu_item_activate (GtkImageMenuItem* _sender, gpointer self);
static gboolean gmpc_metadata_browser_song_browser_button_release_event (GmpcMetadataBrowser* self, GtkTreeView* tree, const GdkEventButton* event);
static void gmpc_metadata_browser_history_previous (GmpcMetadataBrowser* self);
static void gmpc_metadata_browser_history_next (GmpcMetadataBrowser* self);
static gboolean gmpc_metadata_browser_browser_button_release_event (GmpcMetadataBrowser* self, GtkWidget* widget, const GdkEventButton* event);
static void _gmpc_metadata_browser_browser_artist_entry_changed_gtk_editable_changed (GtkEntry* _sender, gpointer self);
static gboolean _gmpc_metadata_browser_visible_func_artist_gtk_tree_model_filter_visible_func (GtkTreeModel* model, GtkTreeIter* iter, gpointer self);
static gboolean _gmpc_metadata_browser_browser_button_press_event_gtk_widget_button_press_event (GtkTreeView* _sender, const GdkEventButton* event, gpointer self);
static gboolean _gmpc_metadata_browser_artist_browser_button_release_event_gtk_widget_button_release_event (GtkTreeView* _sender, const GdkEventButton* event, gpointer self);
static gboolean _gmpc_metadata_browser_browser_artist_key_press_event_gtk_widget_key_press_event (GtkTreeView* _sender, const GdkEventKey* event, gpointer self);
static void gmpc_metadata_browser_browser_artist_changed (GmpcMetadataBrowser* self, GtkTreeSelection* sel);
static void _gmpc_metadata_browser_browser_artist_changed_gtk_tree_selection_changed (GtkTreeSelection* _sender, gpointer self);
static void _gmpc_metadata_browser_browser_album_entry_changed_gtk_editable_changed (GtkEntry* _sender, gpointer self);
static gboolean _gmpc_metadata_browser_visible_func_album_gtk_tree_model_filter_visible_func (GtkTreeModel* model, GtkTreeIter* iter, gpointer self);
static gboolean _gmpc_metadata_browser_album_browser_button_release_event_gtk_widget_button_release_event (GtkTreeView* _sender, const GdkEventButton* event, gpointer self);
static gboolean _gmpc_metadata_browser_browser_album_key_press_event_gtk_widget_key_press_event (GtkTreeView* _sender, const GdkEventKey* event, gpointer self);
static void gmpc_metadata_browser_browser_album_changed (GmpcMetadataBrowser* self, GtkTreeSelection* album_sel);
static void _gmpc_metadata_browser_browser_album_changed_gtk_tree_selection_changed (GtkTreeSelection* _sender, gpointer self);
static gboolean _gmpc_metadata_browser_song_browser_button_release_event_gtk_widget_button_release_event (GtkTreeView* _sender, const GdkEventButton* event, gpointer self);
static void gmpc_metadata_browser_browser_songs_changed (GmpcMetadataBrowser* self, GtkTreeSelection* song_sel);
static void _gmpc_metadata_browser_browser_songs_changed_gtk_tree_selection_changed (GtkTreeSelection* _sender, gpointer self);
static void _gmpc_metadata_browser_browser_bg_style_changed_gtk_widget_style_set (GtkScrolledWindow* _sender, GtkStyle* previous_style, gpointer self);
static gboolean _gmpc_metadata_browser_browser_button_release_event_gtk_widget_button_release_event (GtkWidget* _sender, const GdkEventButton* event, gpointer self);
static void gmpc_metadata_browser_reload_browsers (GmpcMetadataBrowser* self);
static void gmpc_metadata_browser_browser_init (GmpcMetadataBrowser* self);
static void gmpc_metadata_browser_metadata_box_clear (GmpcMetadataBrowser* self);
static void gmpc_metadata_browser_metadata_box_update (GmpcMetadataBrowser* self);
static void gmpc_metadata_browser_play_song (GmpcMetadataBrowser* self, GtkButton* button);
static void gmpc_metadata_browser_add_song (GmpcMetadataBrowser* self, GtkButton* button);
static void gmpc_metadata_browser_replace_song (GmpcMetadataBrowser* self, GtkButton* button);
static void gmpc_metadata_browser_add_selected_song (GmpcMetadataBrowser* self, GtkButton* button);
static void gmpc_metadata_browser_replace_selected_song (GmpcMetadataBrowser* self, GtkButton* button);
static void gmpc_metadata_browser_add_entry (GmpcMetadataBrowser* self, GtkTable* table, const char* entry_label, const char* value, GtkWidget* extra, gint* i);
static void gmpc_metadata_browser_artist_button_clicked (GmpcMetadataBrowser* self, GtkButton* button);
static void gmpc_metadata_browser_album_button_clicked (GmpcMetadataBrowser* self, GtkButton* button);
static GtkHBox* gmpc_metadata_browser_history_buttons (GmpcMetadataBrowser* self);
static void gmpc_metadata_browser_metadata_find_query (GmpcMetadataBrowser* self, GtkButton* button);
static void _gmpc_metadata_browser_metadata_find_query_gtk_button_clicked (GtkButton* _sender, gpointer self);
static void _gmpc_metadata_browser_artist_button_clicked_gtk_button_clicked (GtkButton* _sender, gpointer self);
static void _gmpc_metadata_browser_album_button_clicked_gtk_button_clicked (GtkButton* _sender, gpointer self);
static void gmpc_metadata_browser_metadata_button_open_file_browser_path (GmpcMetadataBrowser* self, GtkButton* button);
static void _gmpc_metadata_browser_metadata_button_open_file_browser_path_gtk_button_clicked (GtkButton* _sender, gpointer self);
static void _gmpc_metadata_browser_play_song_gtk_button_clicked (GtkButton* _sender, gpointer self);
static void _gmpc_metadata_browser_add_song_gtk_button_clicked (GtkButton* _sender, gpointer self);
static void _gmpc_metadata_browser_replace_song_gtk_button_clicked (GtkButton* _sender, gpointer self);
static void gmpc_metadata_browser_album_song_tree_row_activated (GmpcMetadataBrowser* self, GtkTreeView* tree, const GtkTreePath* path, GtkTreeViewColumn* column);
static void gmpc_metadata_browser_album_song_browser_play_clicked (GmpcMetadataBrowser* self, GtkImageMenuItem* item);
static void gmpc_metadata_browser_album_song_browser_add_clicked (GmpcMetadataBrowser* self, GtkImageMenuItem* item);
static void gmpc_metadata_browser_album_song_browser_replace_clicked (GmpcMetadataBrowser* self, GtkImageMenuItem* item);
static void _gmpc_metadata_browser_album_song_browser_play_clicked_gtk_menu_item_activate (GtkImageMenuItem* _sender, gpointer self);
static void _gmpc_metadata_browser_album_song_browser_add_clicked_gtk_menu_item_activate (GtkImageMenuItem* _sender, gpointer self);
static void _gmpc_metadata_browser_album_song_browser_replace_clicked_gtk_menu_item_activate (GtkImageMenuItem* _sender, gpointer self);
static gboolean gmpc_metadata_browser_album_song_tree_button_press_event (GmpcMetadataBrowser* self, GmpcMpdDataTreeview* tree, const GdkEventButton* event);
static void _gmpc_metadata_browser_add_selected_song_gtk_button_clicked (GtkButton* _sender, gpointer self);
static void _gmpc_metadata_browser_replace_selected_song_gtk_button_clicked (GtkButton* _sender, gpointer self);
static gboolean _gmpc_metadata_browser_album_song_tree_button_press_event_gtk_widget_button_release_event (GmpcMpdDataTreeview* _sender, const GdkEventButton* event, gpointer self);
static void _gmpc_metadata_browser_album_song_tree_row_activated_gtk_tree_view_row_activated (GmpcMpdDataTreeview* _sender, const GtkTreePath* path, GtkTreeViewColumn* column, gpointer self);
static void gmpc_metadata_browser_metadata_box_show_album (GmpcMetadataBrowser* self, const char* artist, const char* album);
static void gmpc_metadata_browser_metadata_box_show_artist (GmpcMetadataBrowser* self, const char* artist);
static gboolean gmpc_metadata_browser_metadata_box_update_real (GmpcMetadataBrowser* self);
static gboolean _gmpc_metadata_browser_metadata_box_update_real_gsource_func (gpointer self);
static void gmpc_metadata_browser_history_add (GmpcMetadataBrowser* self, const GmpcMetadataBrowserHitem* hi);
static void gmpc_metadata_browser_real_browser_add (GmpcPluginBrowserIface* base, GtkWidget* category_tree);
static void gmpc_metadata_browser_real_browser_selected (GmpcPluginBrowserIface* base, GtkContainer* container);
static void gmpc_metadata_browser_real_browser_unselected (GmpcPluginBrowserIface* base, GtkContainer* container);
static void gmpc_metadata_browser_history_clear (GmpcMetadataBrowser* self);
static void gmpc_metadata_browser_con_changed (GmpcMetadataBrowser* self, GmpcConnection* conn, MpdObj* server, gint connect);
static void gmpc_metadata_browser_show_hitem (GmpcMetadataBrowser* self, const GmpcMetadataBrowserHitem* hi);
static void gmpc_metadata_browser_status_changed (GmpcMetadataBrowser* self, GmpcConnection* conn, MpdObj* server, ChangedStatusType what);
void gmpc_metadata_browser_set_album (GmpcMetadataBrowser* self, const char* artist, const char* album);
void gmpc_metadata_browser_set_song (GmpcMetadataBrowser* self, const mpd_Song* song);
static void gmpc_metadata_browser_history_show_list_clicked (GmpcMetadataBrowser* self, GtkMenuItem* item);
static void _gmpc_metadata_browser_history_show_list_clicked_gtk_menu_item_activate (GtkMenuItem* _sender, gpointer self);
static void gmpc_metadata_browser_history_show_list (GmpcMetadataBrowser* self);
static void _gmpc_metadata_browser_history_next_gtk_button_clicked (GtkButton* _sender, gpointer self);
static void _gmpc_metadata_browser_history_show_list_gtk_button_clicked (GtkButton* _sender, gpointer self);
static void _gmpc_metadata_browser_history_previous_gtk_button_clicked (GtkButton* _sender, gpointer self);
static void _lambda0_ (GtkToggleButton* source, GmpcMetadataBrowser* self);
static void __lambda0__gtk_toggle_button_toggled (GtkToggleButton* _sender, gpointer self);
static void _lambda1_ (GtkToggleButton* source, GmpcMetadataBrowser* self);
static void __lambda1__gtk_toggle_button_toggled (GtkToggleButton* _sender, gpointer self);
static void _lambda2_ (GtkToggleButton* source, GmpcMetadataBrowser* self);
static void __lambda2__gtk_toggle_button_toggled (GtkToggleButton* _sender, gpointer self);
static void _lambda3_ (GtkToggleButton* source, GmpcMetadataBrowser* self);
static void __lambda3__gtk_toggle_button_toggled (GtkToggleButton* _sender, gpointer self);
static void _lambda4_ (GtkToggleButton* source, GmpcMetadataBrowser* self);
static void __lambda4__gtk_toggle_button_toggled (GtkToggleButton* _sender, gpointer self);
static void _lambda5_ (GtkToggleButton* source, GmpcMetadataBrowser* self);
static void __lambda5__gtk_toggle_button_toggled (GtkToggleButton* _sender, gpointer self);
static void _lambda6_ (GtkToggleButton* source, GmpcMetadataBrowser* self);
static void __lambda6__gtk_toggle_button_toggled (GtkToggleButton* _sender, gpointer self);
static void gmpc_metadata_browser_real_preferences_pane_construct (GmpcPluginPreferencesIface* base, GtkContainer* container);
static void gmpc_metadata_browser_real_preferences_pane_destroy (GmpcPluginPreferencesIface* base, GtkContainer* container);
GmpcMetadataBrowser* gmpc_metadata_browser_new (void);
static void _gmpc_metadata_browser_con_changed_gmpc_connection_connection_changed (GmpcConnection* _sender, MpdObj* server, gint connect, gpointer self);
static void _gmpc_metadata_browser_status_changed_gmpc_connection_status_changed (GmpcConnection* _sender, MpdObj* server, ChangedStatusType what, gpointer self);
static GObject * gmpc_metadata_browser_constructor (GType type, guint n_construct_properties, GObjectConstructParam * construct_properties);
static void gmpc_metadata_browser_finalize (GObject* obj);
static void _vala_array_destroy (gpointer array, gint array_length, GDestroyNotify destroy_func);
static void _vala_array_free (gpointer array, gint array_length, GDestroyNotify destroy_func);
static gint _vala_array_length (gpointer array);
static int _vala_strcmp0 (const char * str1, const char * str2);

static const gint GMPC_NOW_PLAYING_version[] = {0, 0, 0};
static const gint GMPC_METADATA_BROWSER_version[] = {0, 0, 0};


static GmpcWidgetSimilarSongs* gmpc_widget_similar_songs_construct (GType object_type, const mpd_Song* song) {
	GmpcWidgetSimilarSongs * self;
	mpd_Song* _tmp1_;
	const mpd_Song* _tmp0_;
	GtkLabel* label;
	char* _tmp2_;
	g_return_val_if_fail (song != NULL, NULL);
	self = g_object_newv (object_type, 0, NULL);
	_tmp1_ = NULL;
	_tmp0_ = NULL;
	self->priv->song = (_tmp1_ = (_tmp0_ = song, (_tmp0_ == NULL) ? NULL : mpd_songDup (_tmp0_)), (self->priv->song == NULL) ? NULL : (self->priv->song = (mpd_freeSong (self->priv->song), NULL)), _tmp1_);
	label = g_object_ref_sink ((GtkLabel*) gtk_label_new (_ ("Similar songs")));
	_tmp2_ = NULL;
	gtk_label_set_markup (label, _tmp2_ = g_strdup_printf ("<b>%s</b>", _ ("Similar songs")));
	_tmp2_ = (g_free (_tmp2_), NULL);
	gtk_expander_set_label_widget ((GtkExpander*) self, (GtkWidget*) label);
	gtk_widget_show ((GtkWidget*) label);
	(label == NULL) ? NULL : (label = (g_object_unref (label), NULL));
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
	GtkTreeView* _tmp0_;
	GtkTreeView* tree;
	GtkTreeSelection* _tmp1_;
	GtkTreeSelection* sel;
	GtkTreeModel* model;
	GtkTreeIter iter = {0};
	GtkTreeModel* _tmp5_;
	GtkTreeModel* _tmp4_;
	GList* _tmp3_;
	GtkTreeModel* _tmp2_;
	GList* list;
	g_return_if_fail (self != NULL);
	g_return_if_fail (item != NULL);
	_tmp0_ = NULL;
	tree = (_tmp0_ = GTK_TREE_VIEW (self->priv->pchild), (_tmp0_ == NULL) ? NULL : g_object_ref (_tmp0_));
	_tmp1_ = NULL;
	sel = (_tmp1_ = gtk_tree_view_get_selection (tree), (_tmp1_ == NULL) ? NULL : g_object_ref (_tmp1_));
	model = NULL;
	_tmp5_ = NULL;
	_tmp4_ = NULL;
	_tmp3_ = NULL;
	_tmp2_ = NULL;
	list = (_tmp3_ = gtk_tree_selection_get_selected_rows (sel, &_tmp2_), model = (_tmp4_ = (_tmp5_ = _tmp2_, (_tmp5_ == NULL) ? NULL : g_object_ref (_tmp5_)), (model == NULL) ? NULL : (model = (g_object_unref (model), NULL)), _tmp4_), _tmp3_);
	{
		GList* path_collection;
		GList* path_it;
		path_collection = list;
		for (path_it = path_collection; path_it != NULL; path_it = path_it->next) {
			const GtkTreePath* _tmp6_;
			GtkTreePath* path;
			_tmp6_ = NULL;
			path = (_tmp6_ = (const GtkTreePath*) path_it->data, (_tmp6_ == NULL) ? NULL : gtk_tree_path_copy (_tmp6_));
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
	GtkTreeView* _tmp0_;
	GtkTreeView* tree;
	GtkTreeSelection* _tmp1_;
	GtkTreeSelection* sel;
	GtkTreeModel* model;
	GtkTreeIter iter = {0};
	GtkTreeModel* _tmp5_;
	GtkTreeModel* _tmp4_;
	GList* _tmp3_;
	GtkTreeModel* _tmp2_;
	GList* list;
	g_return_if_fail (self != NULL);
	g_return_if_fail (item != NULL);
	_tmp0_ = NULL;
	tree = (_tmp0_ = GTK_TREE_VIEW (self->priv->pchild), (_tmp0_ == NULL) ? NULL : g_object_ref (_tmp0_));
	_tmp1_ = NULL;
	sel = (_tmp1_ = gtk_tree_view_get_selection (tree), (_tmp1_ == NULL) ? NULL : g_object_ref (_tmp1_));
	model = NULL;
	_tmp5_ = NULL;
	_tmp4_ = NULL;
	_tmp3_ = NULL;
	_tmp2_ = NULL;
	list = (_tmp3_ = gtk_tree_selection_get_selected_rows (sel, &_tmp2_), model = (_tmp4_ = (_tmp5_ = _tmp2_, (_tmp5_ == NULL) ? NULL : g_object_ref (_tmp5_)), (model == NULL) ? NULL : (model = (g_object_unref (model), NULL)), _tmp4_), _tmp3_);
	if (list != NULL) {
		const GtkTreePath* _tmp6_;
		GtkTreePath* path;
		_tmp6_ = NULL;
		path = (_tmp6_ = (const GtkTreePath*) list->data, (_tmp6_ == NULL) ? NULL : gtk_tree_path_copy (_tmp6_));
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
	GtkTreeView* _tmp0_;
	GtkTreeView* tree;
	GtkTreeSelection* _tmp1_;
	GtkTreeSelection* sel;
	GtkTreeModel* model;
	GtkTreeIter iter = {0};
	GtkTreeModel* _tmp5_;
	GtkTreeModel* _tmp4_;
	GList* _tmp3_;
	GtkTreeModel* _tmp2_;
	GList* list;
	g_return_if_fail (self != NULL);
	g_return_if_fail (item != NULL);
	found = FALSE;
	_tmp0_ = NULL;
	tree = (_tmp0_ = GTK_TREE_VIEW (self->priv->pchild), (_tmp0_ == NULL) ? NULL : g_object_ref (_tmp0_));
	_tmp1_ = NULL;
	sel = (_tmp1_ = gtk_tree_view_get_selection (tree), (_tmp1_ == NULL) ? NULL : g_object_ref (_tmp1_));
	model = NULL;
	_tmp5_ = NULL;
	_tmp4_ = NULL;
	_tmp3_ = NULL;
	_tmp2_ = NULL;
	list = (_tmp3_ = gtk_tree_selection_get_selected_rows (sel, &_tmp2_), model = (_tmp4_ = (_tmp5_ = _tmp2_, (_tmp5_ == NULL) ? NULL : g_object_ref (_tmp5_)), (model == NULL) ? NULL : (model = (g_object_unref (model), NULL)), _tmp4_), _tmp3_);
	{
		GList* path_collection;
		GList* path_it;
		path_collection = list;
		for (path_it = path_collection; path_it != NULL; path_it = path_it->next) {
			const GtkTreePath* _tmp6_;
			GtkTreePath* path;
			_tmp6_ = NULL;
			path = (_tmp6_ = (const GtkTreePath*) path_it->data, (_tmp6_ == NULL) ? NULL : gtk_tree_path_copy (_tmp6_));
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
	GtkTreeModel* _tmp0_;
	GtkTreeModel* model;
	GtkTreeIter iter = {0};
	g_return_if_fail (self != NULL);
	g_return_if_fail (tree != NULL);
	g_return_if_fail (path != NULL);
	g_return_if_fail (column != NULL);
	_tmp0_ = NULL;
	model = (_tmp0_ = gtk_tree_view_get_model ((GtkTreeView*) tree), (_tmp0_ == NULL) ? NULL : g_object_ref (_tmp0_));
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
	gboolean result;
	g_return_val_if_fail (self != NULL, FALSE);
	g_return_val_if_fail (tree != NULL, FALSE);
	if ((*event).button == 3) {
		GtkMenu* menu;
		GtkImageMenuItem* item;
		GtkImageMenuItem* _tmp0_;
		GtkImageMenuItem* _tmp1_;
		GtkImage* _tmp2_;
		menu = g_object_ref_sink ((GtkMenu*) gtk_menu_new ());
		item = g_object_ref_sink ((GtkImageMenuItem*) gtk_image_menu_item_new_from_stock ("gtk-media-play", NULL));
		g_signal_connect_object ((GtkMenuItem*) item, "activate", (GCallback) _gmpc_widget_similar_songs_play_clicked_gtk_menu_item_activate, self, 0);
		gtk_menu_shell_append ((GtkMenuShell*) menu, (GtkWidget*) ((GtkMenuItem*) item));
		_tmp0_ = NULL;
		item = (_tmp0_ = g_object_ref_sink ((GtkImageMenuItem*) gtk_image_menu_item_new_from_stock ("gtk-add", NULL)), (item == NULL) ? NULL : (item = (g_object_unref (item), NULL)), _tmp0_);
		g_signal_connect_object ((GtkMenuItem*) item, "activate", (GCallback) _gmpc_widget_similar_songs_add_clicked_gtk_menu_item_activate, self, 0);
		gtk_menu_shell_append ((GtkMenuShell*) menu, (GtkWidget*) ((GtkMenuItem*) item));
		_tmp1_ = NULL;
		item = (_tmp1_ = g_object_ref_sink ((GtkImageMenuItem*) gtk_image_menu_item_new_with_mnemonic (_ ("_Replace"))), (item == NULL) ? NULL : (item = (g_object_unref (item), NULL)), _tmp1_);
		_tmp2_ = NULL;
		gtk_image_menu_item_set_image (item, (GtkWidget*) (_tmp2_ = g_object_ref_sink ((GtkImage*) gtk_image_new_from_stock ("gtk-redo", GTK_ICON_SIZE_MENU))));
		(_tmp2_ == NULL) ? NULL : (_tmp2_ = (g_object_unref (_tmp2_), NULL));
		g_signal_connect_object ((GtkMenuItem*) item, "activate", (GCallback) _gmpc_widget_similar_songs_replace_clicked_gtk_menu_item_activate, self, 0);
		gtk_menu_shell_append ((GtkMenuShell*) menu, (GtkWidget*) ((GtkMenuItem*) item));
		gtk_menu_popup (menu, NULL, NULL, NULL, NULL, (*event).button, (*event).time);
		gtk_widget_show_all ((GtkWidget*) menu);
		result = TRUE;
		(menu == NULL) ? NULL : (menu = (g_object_unref (menu), NULL));
		(item == NULL) ? NULL : (item = (g_object_unref (item), NULL));
		return result;
	}
	result = FALSE;
	return result;
}


static gboolean _gmpc_widget_similar_songs_tree_right_menu_gtk_widget_button_release_event (GmpcMpdDataTreeview* _sender, const GdkEventButton* event, gpointer self) {
	return gmpc_widget_similar_songs_tree_right_menu (self, _sender, event);
}


static void _gmpc_widget_similar_songs_tree_row_activated_gtk_tree_view_row_activated (GmpcMpdDataTreeview* _sender, const GtkTreePath* path, GtkTreeViewColumn* column, gpointer self) {
	gmpc_widget_similar_songs_tree_row_activated (self, _sender, path, column);
}


static gboolean gmpc_widget_similar_songs_update_sim_song (GmpcWidgetSimilarSongs* self) {
	gboolean result;
	MetaData* _tmp13_;
	g_return_val_if_fail (self != NULL, FALSE);
	if (self->priv->current == NULL) {
		GtkWidget* _tmp0_;
		self->priv->current = meta_data_get_text_list (self->priv->copy);
		_tmp0_ = NULL;
		self->priv->pchild = (_tmp0_ = (GtkWidget*) g_object_ref_sink ((GtkProgressBar*) gtk_progress_bar_new ()), (self->priv->pchild == NULL) ? NULL : (self->priv->pchild = (g_object_unref (self->priv->pchild), NULL)), _tmp0_);
		gtk_container_add ((GtkContainer*) self, self->priv->pchild);
		gtk_widget_show_all ((GtkWidget*) self);
	}
	gtk_progress_bar_pulse (GTK_PROGRESS_BAR (self->priv->pchild));
	if (self->priv->current != NULL) {
		const char* _tmp1_;
		char* entry;
		_tmp1_ = NULL;
		entry = (_tmp1_ = (const char*) self->priv->current->data, (_tmp1_ == NULL) ? NULL : g_strdup (_tmp1_));
		if (entry != NULL) {
			char** _tmp3_;
			gint split_size;
			gint split_length1;
			char** _tmp2_;
			char** split;
			char** _tmp5_;
			gint art_split_size;
			gint art_split_length1;
			char** _tmp4_;
			char** art_split;
			MpdData* data;
			_tmp3_ = NULL;
			_tmp2_ = NULL;
			split = (_tmp3_ = _tmp2_ = g_strsplit (entry, "::", 2), split_length1 = _vala_array_length (_tmp2_), split_size = split_length1, _tmp3_);
			mpd_database_search_start (connection, FALSE);
			_tmp5_ = NULL;
			_tmp4_ = NULL;
			art_split = (_tmp5_ = _tmp4_ = g_strsplit (split[0], " ", 0), art_split_length1 = _vala_array_length (_tmp4_), art_split_size = art_split_length1, _tmp5_);
			{
				char** artist_collection;
				int artist_collection_length1;
				int artist_it;
				artist_collection = art_split;
				artist_collection_length1 = art_split_length1;
				for (artist_it = 0; artist_it < art_split_length1; artist_it = artist_it + 1) {
					const char* _tmp6_;
					char* artist;
					_tmp6_ = NULL;
					artist = (_tmp6_ = artist_collection[artist_it], (_tmp6_ == NULL) ? NULL : g_strdup (_tmp6_));
					{
						mpd_database_search_add_constraint (connection, MPD_TAG_ITEM_ARTIST, artist);
						artist = (g_free (artist), NULL);
					}
				}
			}
			mpd_database_search_add_constraint (connection, MPD_TAG_ITEM_TITLE, split[1]);
			data = mpd_database_search_commit (connection);
			if (data != NULL) {
				MpdData* _tmp7_;
				_tmp7_ = NULL;
				self->priv->item = mpd_data_concatenate (self->priv->item, (_tmp7_ = data, data = NULL, _tmp7_));
			}
			split = (_vala_array_free (split, split_length1, (GDestroyNotify) g_free), NULL);
			art_split = (_vala_array_free (art_split, art_split_length1, (GDestroyNotify) g_free), NULL);
			(data == NULL) ? NULL : (data = (mpd_data_free (data), NULL));
		}
		self->priv->current = self->priv->current->next;
		if (self->priv->current != NULL) {
			result = TRUE;
			entry = (g_free (entry), NULL);
			return result;
		}
		entry = (g_free (entry), NULL);
	}
	gtk_object_destroy ((GtkObject*) self->priv->pchild);
	if (self->priv->item != NULL) {
		GmpcMpdDataModel* model;
		MpdData* _tmp8_;
		GmpcMpdDataTreeview* tree;
		GtkWidget* _tmp10_;
		GtkWidget* _tmp9_;
		model = gmpc_mpddata_model_new ();
		_tmp8_ = NULL;
		gmpc_mpddata_model_set_mpd_data (model, (_tmp8_ = self->priv->item, self->priv->item = NULL, _tmp8_));
		tree = g_object_ref_sink (gmpc_mpddata_treeview_new ("similar-song", TRUE, (GtkTreeModel*) model));
		gmpc_mpddata_treeview_enable_click_fix (tree);
		g_signal_connect_object ((GtkWidget*) tree, "button-release-event", (GCallback) _gmpc_widget_similar_songs_tree_right_menu_gtk_widget_button_release_event, self, 0);
		g_signal_connect_object ((GtkTreeView*) tree, "row-activated", (GCallback) _gmpc_widget_similar_songs_tree_row_activated_gtk_tree_view_row_activated, self, 0);
		gtk_container_add ((GtkContainer*) self, (GtkWidget*) tree);
		_tmp10_ = NULL;
		_tmp9_ = NULL;
		self->priv->pchild = (_tmp10_ = (_tmp9_ = (GtkWidget*) tree, (_tmp9_ == NULL) ? NULL : g_object_ref (_tmp9_)), (self->priv->pchild == NULL) ? NULL : (self->priv->pchild = (g_object_unref (self->priv->pchild), NULL)), _tmp10_);
		(model == NULL) ? NULL : (model = (g_object_unref (model), NULL));
		(tree == NULL) ? NULL : (tree = (g_object_unref (tree), NULL));
	} else {
		GtkLabel* label;
		GtkWidget* _tmp12_;
		GtkWidget* _tmp11_;
		label = g_object_ref_sink ((GtkLabel*) gtk_label_new (_ ("Unavailable")));
		gtk_misc_set_alignment ((GtkMisc*) label, 0.0f, 0.0f);
		gtk_container_add ((GtkContainer*) self, (GtkWidget*) label);
		_tmp12_ = NULL;
		_tmp11_ = NULL;
		self->priv->pchild = (_tmp12_ = (_tmp11_ = (GtkWidget*) label, (_tmp11_ == NULL) ? NULL : g_object_ref (_tmp11_)), (self->priv->pchild == NULL) ? NULL : (self->priv->pchild = (g_object_unref (self->priv->pchild), NULL)), _tmp12_);
		(label == NULL) ? NULL : (label = (g_object_unref (label), NULL));
	}
	_tmp13_ = NULL;
	self->priv->copy = (_tmp13_ = NULL, (self->priv->copy == NULL) ? NULL : (self->priv->copy = (meta_data_free (self->priv->copy), NULL)), _tmp13_);
	self->priv->idle_add = (guint) 0;
	gtk_widget_show_all ((GtkWidget*) self);
	result = FALSE;
	return result;
}


static gboolean _gmpc_widget_similar_songs_update_sim_song_gsource_func (gpointer self) {
	return gmpc_widget_similar_songs_update_sim_song (self);
}


static void gmpc_widget_similar_songs_metadata_changed (GmpcWidgetSimilarSongs* self, GmpcMetaWatcher* gmw, const mpd_Song* song, MetaDataType type, MetaDataResult _result_, const MetaData* met) {
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
	if (_result_ == META_DATA_FETCHING) {
		GtkLabel* label;
		GtkWidget* _tmp1_;
		GtkWidget* _tmp0_;
		label = g_object_ref_sink ((GtkLabel*) gtk_label_new (_ ("Fetching .. ")));
		gtk_misc_set_alignment ((GtkMisc*) label, 0.0f, 0.0f);
		gtk_container_add ((GtkContainer*) self, (GtkWidget*) label);
		_tmp1_ = NULL;
		_tmp0_ = NULL;
		self->priv->pchild = (_tmp1_ = (_tmp0_ = (GtkWidget*) label, (_tmp0_ == NULL) ? NULL : g_object_ref (_tmp0_)), (self->priv->pchild == NULL) ? NULL : (self->priv->pchild = (g_object_unref (self->priv->pchild), NULL)), _tmp1_);
		(label == NULL) ? NULL : (label = (g_object_unref (label), NULL));
	} else {
		if (_result_ == META_DATA_UNAVAILABLE) {
			GtkLabel* label;
			GtkWidget* _tmp3_;
			GtkWidget* _tmp2_;
			label = g_object_ref_sink ((GtkLabel*) gtk_label_new (_ ("Unavailable")));
			gtk_misc_set_alignment ((GtkMisc*) label, 0.0f, 0.0f);
			gtk_container_add ((GtkContainer*) self, (GtkWidget*) label);
			_tmp3_ = NULL;
			_tmp2_ = NULL;
			self->priv->pchild = (_tmp3_ = (_tmp2_ = (GtkWidget*) label, (_tmp2_ == NULL) ? NULL : g_object_ref (_tmp2_)), (self->priv->pchild == NULL) ? NULL : (self->priv->pchild = (g_object_unref (self->priv->pchild), NULL)), _tmp3_);
			(label == NULL) ? NULL : (label = (g_object_unref (label), NULL));
		} else {
			if (meta_data_is_text_list (met)) {
				MetaData* _tmp4_;
				_tmp4_ = NULL;
				self->priv->copy = (_tmp4_ = meta_data_dup_steal (met), (self->priv->copy == NULL) ? NULL : (self->priv->copy = (meta_data_free (self->priv->copy), NULL)), _tmp4_);
				self->priv->idle_add = g_idle_add (_gmpc_widget_similar_songs_update_sim_song_gsource_func, self);
				return;
			} else {
				GtkLabel* label;
				GtkWidget* _tmp6_;
				GtkWidget* _tmp5_;
				label = g_object_ref_sink ((GtkLabel*) gtk_label_new (_ ("Unavailable")));
				gtk_misc_set_alignment ((GtkMisc*) label, 0.0f, 0.0f);
				gtk_container_add ((GtkContainer*) self, (GtkWidget*) label);
				_tmp6_ = NULL;
				_tmp5_ = NULL;
				self->priv->pchild = (_tmp6_ = (_tmp5_ = (GtkWidget*) label, (_tmp5_ == NULL) ? NULL : g_object_ref (_tmp5_)), (self->priv->pchild == NULL) ? NULL : (self->priv->pchild = (g_object_unref (self->priv->pchild), NULL)), _tmp6_);
				(label == NULL) ? NULL : (label = (g_object_unref (label), NULL));
			}
		}
	}
	gtk_widget_show_all ((GtkWidget*) self);
}


static void _gmpc_widget_similar_songs_metadata_changed_gmpc_meta_watcher_data_changed (GmpcMetaWatcher* _sender, const mpd_Song* song, MetaDataType type, MetaDataResult _result_, const MetaData* met, gpointer self) {
	gmpc_widget_similar_songs_metadata_changed (self, _sender, song, type, _result_, met);
}


static void gmpc_widget_similar_songs_update (GmpcWidgetSimilarSongs* self) {
	MetaData* item;
	MetaData* _tmp2_;
	MetaDataResult _tmp1_;
	MetaData* _tmp0_;
	MetaDataResult gm_result;
	g_return_if_fail (self != NULL);
	item = NULL;
	g_signal_connect_object (gmw, "data-changed", (GCallback) _gmpc_widget_similar_songs_metadata_changed_gmpc_meta_watcher_data_changed, self, 0);
	_tmp2_ = NULL;
	_tmp0_ = NULL;
	gm_result = (_tmp1_ = gmpc_meta_watcher_get_meta_path (gmw, self->priv->song, META_SONG_SIMILAR, &_tmp0_), item = (_tmp2_ = _tmp0_, (item == NULL) ? NULL : (item = (meta_data_free (item), NULL)), _tmp2_), _tmp1_);
	gmpc_widget_similar_songs_metadata_changed (self, gmw, self->priv->song, META_SONG_SIMILAR, gm_result, item);
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
	GTK_EXPANDER_CLASS (klass)->activate = gmpc_widget_similar_songs_real_activate;
	G_OBJECT_CLASS (klass)->finalize = gmpc_widget_similar_songs_finalize;
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


/**
     * Handle signals from the metadata object.
     */
static void gmpc_widget_similar_artist_metadata_changed (GmpcWidgetSimilarArtist* self, GmpcMetaWatcher* gmw, const mpd_Song* song, MetaDataType type, MetaDataResult _result_, const MetaData* met) {
	GList* child_list;
	gboolean _tmp1_;
	gboolean _tmp2_;
	g_return_if_fail (self != NULL);
	g_return_if_fail (gmw != NULL);
	g_return_if_fail (song != NULL);
	/* only listen to the same artist and the same type */
	if (type != META_ARTIST_SIMILAR) {
		return;
	}
	if (g_utf8_collate (self->priv->song->artist, song->artist) != 0) {
		return;
	}
	/* clear widgets */
	child_list = gtk_container_get_children ((GtkContainer*) self);
	{
		GList* child_collection;
		GList* child_it;
		child_collection = child_list;
		for (child_it = child_collection; child_it != NULL; child_it = child_it->next) {
			GtkWidget* _tmp0_;
			GtkWidget* child;
			_tmp0_ = NULL;
			child = (_tmp0_ = (GtkWidget*) child_it->data, (_tmp0_ == NULL) ? NULL : g_object_ref (_tmp0_));
			{
				gtk_object_destroy ((GtkObject*) child);
				(child == NULL) ? NULL : (child = (g_object_unref (child), NULL));
			}
		}
	}
	_tmp1_ = FALSE;
	_tmp2_ = FALSE;
	if (_result_ == META_DATA_UNAVAILABLE) {
		_tmp2_ = TRUE;
	} else {
		_tmp2_ = meta_data_is_empty (met);
	}
	if (_tmp2_) {
		_tmp1_ = TRUE;
	} else {
		_tmp1_ = !meta_data_is_text_list (met);
	}
	/* if unavailable set that in a label*/
	if (_tmp1_) {
		GtkLabel* label;
		label = g_object_ref_sink ((GtkLabel*) gtk_label_new (_ ("Unavailable")));
		gtk_table_attach ((GtkTable*) self, (GtkWidget*) label, (guint) 0, (guint) 1, (guint) 0, (guint) 1, GTK_SHRINK, GTK_SHRINK, (guint) 0, (guint) 0);
		(label == NULL) ? NULL : (label = (g_object_unref (label), NULL));
	} else {
		if (_result_ == META_DATA_FETCHING) {
			GtkLabel* label;
			label = g_object_ref_sink ((GtkLabel*) gtk_label_new (_ ("Fetching")));
			gtk_table_attach ((GtkTable*) self, (GtkWidget*) label, (guint) 0, (guint) 1, (guint) 0, (guint) 1, GTK_SHRINK, GTK_SHRINK, (guint) 0, (guint) 0);
			(label == NULL) ? NULL : (label = (g_object_unref (label), NULL));
		} else {
			GList* in_db_list;
			GList* list;
			gint items;
			gint i;
			guint llength;
			gint columns;
			/* Set result */
			in_db_list = NULL;
			list = g_list_copy (meta_data_get_text_list (met));
			list = g_list_sort (list, (GCompareFunc) g_utf8_collate);
			items = 30;
			i = 0;
			if (list != NULL) {
				GList* liter;
				MpdData* data;
				gint q;
				const MpdData* iter;
				char* artist;
				liter = NULL;
				mpd_database_search_field_start (connection, MPD_TAG_ITEM_ARTIST);
				data = mpd_database_search_commit (connection);
				q = 0;
				data = misc_sort_mpddata_by_album_disc_track (data);
				iter = mpd_data_get_first (data);
				liter = g_list_first (list);
				artist = g_utf8_strdown (iter->tag, -1);
				{
					gboolean _tmp3_;
					_tmp3_ = TRUE;
					while (TRUE) {
						char* _tmp6_;
						gint _tmp7_;
						gint res;
						if (!_tmp3_) {
							gboolean _tmp4_;
							gboolean _tmp5_;
							_tmp4_ = FALSE;
							_tmp5_ = FALSE;
							if (iter != NULL) {
								_tmp5_ = liter != NULL;
							} else {
								_tmp5_ = FALSE;
							}
							if (_tmp5_) {
								_tmp4_ = i < items;
							} else {
								_tmp4_ = FALSE;
							}
							if (!_tmp4_) {
								break;
							}
						}
						_tmp3_ = FALSE;
						_tmp6_ = NULL;
						res = (_tmp7_ = g_utf8_collate (_tmp6_ = g_utf8_strdown ((const char*) liter->data, -1), artist), _tmp6_ = (g_free (_tmp6_), NULL), _tmp7_);
						q++;
						if (res == 0) {
							const char* _tmp8_;
							char* d;
							char* _tmp9_;
							in_db_list = g_list_prepend (in_db_list, gmpc_widget_similar_artist_new_artist_button (self, iter->tag, TRUE));
							i++;
							_tmp8_ = NULL;
							d = (_tmp8_ = (const char*) liter->data, (_tmp8_ == NULL) ? NULL : g_strdup (_tmp8_));
							liter = liter->next;
							list = g_list_remove (list, d);
							/*liter = null;*/
							iter = mpd_data_get_next_real (iter, FALSE);
							_tmp9_ = NULL;
							artist = (_tmp9_ = g_utf8_casefold (iter->tag, -1), artist = (g_free (artist), NULL), _tmp9_);
							d = (g_free (d), NULL);
						} else {
							if (res > 0) {
								/*list.remove(liter.data);*/
								iter = mpd_data_get_next_real (iter, FALSE);
								if (iter != NULL) {
									char* _tmp10_;
									_tmp10_ = NULL;
									artist = (_tmp10_ = g_utf8_casefold (iter->tag, -1), artist = (g_free (artist), NULL), _tmp10_);
								}
							} else {
								liter = liter->next;
							}
						}
					}
				}
				liter = g_list_first (list);
				while (TRUE) {
					gboolean _tmp11_;
					char* _tmp13_;
					const char* _tmp12_;
					_tmp11_ = FALSE;
					if (liter != NULL) {
						_tmp11_ = i < items;
					} else {
						_tmp11_ = FALSE;
					}
					if (!_tmp11_) {
						break;
					}
					_tmp13_ = NULL;
					_tmp12_ = NULL;
					artist = (_tmp13_ = (_tmp12_ = (const char*) liter->data, (_tmp12_ == NULL) ? NULL : g_strdup (_tmp12_)), artist = (g_free (artist), NULL), _tmp13_);
					in_db_list = g_list_prepend (in_db_list, gmpc_widget_similar_artist_new_artist_button (self, artist, FALSE));
					i++;
					liter = liter->next;
				}
				(data == NULL) ? NULL : (data = (mpd_data_free (data), NULL));
				artist = (g_free (artist), NULL);
			}
			in_db_list = g_list_reverse (in_db_list);
			i = 0;
			gtk_widget_hide ((GtkWidget*) self);
			llength = g_list_length (in_db_list);
			columns = 3;
			gtk_table_resize ((GtkTable*) self, (llength / columns) + 1, (guint) columns);
			{
				GList* item_collection;
				GList* item_it;
				item_collection = in_db_list;
				for (item_it = item_collection; item_it != NULL; item_it = item_it->next) {
					GtkWidget* _tmp14_;
					GtkWidget* item;
					_tmp14_ = NULL;
					item = (_tmp14_ = (GtkWidget*) item_it->data, (_tmp14_ == NULL) ? NULL : g_object_ref (_tmp14_));
					{
						gtk_table_attach ((GtkTable*) self, item, (guint) (i % columns), (guint) ((i % columns) + 1), (guint) (i / columns), (guint) ((i / columns) + 1), GTK_EXPAND | GTK_FILL, GTK_SHRINK, (guint) 0, (guint) 0);
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


static gboolean _misc_header_expose_event_gtk_widget_expose_event (GtkWidget* _sender, const GdkEventExpose* event, gpointer self) {
	return misc_header_expose_event (_sender, event);
}


static void _gmpc_widget_similar_artist_artist_button_clicked_gtk_button_clicked (GtkButton* _sender, gpointer self) {
	gmpc_widget_similar_artist_artist_button_clicked (self, _sender);
}


GtkWidget* gmpc_widget_similar_artist_new_artist_button (GmpcWidgetSimilarArtist* self, const char* artist, gboolean in_db) {
	GtkWidget* result;
	GtkHBox* hbox;
	GtkEventBox* event;
	GmpcMetaImage* image;
	mpd_Song* song;
	char* _tmp1_;
	const char* _tmp0_;
	GtkLabel* label;
	g_return_val_if_fail (self != NULL, NULL);
	g_return_val_if_fail (artist != NULL, NULL);
	hbox = g_object_ref_sink ((GtkHBox*) gtk_hbox_new (FALSE, 6));
	gtk_container_set_border_width ((GtkContainer*) hbox, (guint) 4);
	/*
	        var event = new Gtk.Frame(null);
	        */
	event = g_object_ref_sink ((GtkEventBox*) gtk_event_box_new ());
	g_object_set ((GtkWidget*) event, "app-paintable", TRUE, NULL);
	gtk_event_box_set_visible_window (event, TRUE);
	g_signal_connect ((GtkWidget*) event, "expose-event", (GCallback) _misc_header_expose_event_gtk_widget_expose_event, NULL);
	gtk_widget_set_size_request ((GtkWidget*) event, 200, 60);
	image = g_object_ref_sink (gmpc_metaimage_new_size (META_ARTIST_ART, 48));
	song = mpd_newSong ();
	_tmp1_ = NULL;
	_tmp0_ = NULL;
	song->artist = (_tmp1_ = (_tmp0_ = artist, (_tmp0_ == NULL) ? NULL : g_strdup (_tmp0_)), song->artist = (g_free (song->artist), NULL), _tmp1_);
	gmpc_metaimage_set_squared (image, TRUE);
	gmpc_metaimage_update_cover_from_song_delayed (image, song);
	gtk_box_pack_start ((GtkBox*) hbox, (GtkWidget*) image, FALSE, FALSE, (guint) 0);
	label = g_object_ref_sink ((GtkLabel*) gtk_label_new (artist));
	gtk_widget_set_tooltip_text ((GtkWidget*) label, artist);
	gtk_label_set_selectable (label, TRUE);
	gtk_misc_set_alignment ((GtkMisc*) label, 0.0f, 0.5f);
	gtk_label_set_ellipsize (label, PANGO_ELLIPSIZE_END);
	gtk_box_pack_start ((GtkBox*) hbox, (GtkWidget*) label, TRUE, TRUE, (guint) 0);
	if (in_db) {
		GtkButton* find;
		GtkImage* _tmp2_;
		find = g_object_ref_sink ((GtkButton*) gtk_button_new ());
		_tmp2_ = NULL;
		gtk_container_add ((GtkContainer*) find, (GtkWidget*) (_tmp2_ = g_object_ref_sink ((GtkImage*) gtk_image_new_from_stock ("gtk-find", GTK_ICON_SIZE_MENU))));
		(_tmp2_ == NULL) ? NULL : (_tmp2_ = (g_object_unref (_tmp2_), NULL));
		gtk_button_set_relief (find, GTK_RELIEF_NONE);
		gtk_box_pack_start ((GtkBox*) hbox, (GtkWidget*) find, FALSE, FALSE, (guint) 0);
		g_object_set_data_full ((GObject*) find, "artist", (void*) g_strdup_printf ("%s", artist), (GDestroyNotify) g_free);
		g_signal_connect_object (find, "clicked", (GCallback) _gmpc_widget_similar_artist_artist_button_clicked_gtk_button_clicked, self, 0);
		(find == NULL) ? NULL : (find = (g_object_unref (find), NULL));
	}
	gtk_container_add ((GtkContainer*) event, (GtkWidget*) hbox);
	result = (GtkWidget*) event;
	(hbox == NULL) ? NULL : (hbox = (g_object_unref (hbox), NULL));
	(image == NULL) ? NULL : (image = (g_object_unref (image), NULL));
	(song == NULL) ? NULL : (song = (mpd_freeSong (song), NULL));
	(label == NULL) ? NULL : (label = (g_object_unref (label), NULL));
	return result;
}


static void _gmpc_widget_similar_artist_metadata_changed_gmpc_meta_watcher_data_changed (GmpcMetaWatcher* _sender, const mpd_Song* song, MetaDataType type, MetaDataResult _result_, const MetaData* met, gpointer self) {
	gmpc_widget_similar_artist_metadata_changed (self, _sender, song, type, _result_, met);
}


static GmpcWidgetSimilarArtist* gmpc_widget_similar_artist_construct (GType object_type, GmpcMetadataBrowser* browser, MpdObj* server, const mpd_Song* song) {
	GmpcWidgetSimilarArtist * self;
	MetaData* item;
	GmpcMetadataBrowser* _tmp1_;
	GmpcMetadataBrowser* _tmp0_;
	mpd_Song* _tmp3_;
	const mpd_Song* _tmp2_;
	MetaData* _tmp6_;
	MetaDataResult _tmp5_;
	MetaData* _tmp4_;
	MetaDataResult gm_result;
	g_return_val_if_fail (browser != NULL, NULL);
	g_return_val_if_fail (server != NULL, NULL);
	g_return_val_if_fail (song != NULL, NULL);
	self = g_object_newv (object_type, 0, NULL);
	item = NULL;
	_tmp1_ = NULL;
	_tmp0_ = NULL;
	self->priv->browser = (_tmp1_ = (_tmp0_ = browser, (_tmp0_ == NULL) ? NULL : g_object_ref (_tmp0_)), (self->priv->browser == NULL) ? NULL : (self->priv->browser = (g_object_unref (self->priv->browser), NULL)), _tmp1_);
	_tmp3_ = NULL;
	_tmp2_ = NULL;
	self->priv->song = (_tmp3_ = (_tmp2_ = song, (_tmp2_ == NULL) ? NULL : mpd_songDup (_tmp2_)), (self->priv->song == NULL) ? NULL : (self->priv->song = (mpd_freeSong (self->priv->song), NULL)), _tmp3_);
	gtk_table_set_homogeneous ((GtkTable*) self, TRUE);
	gtk_table_set_row_spacings ((GtkTable*) self, (guint) 6);
	gtk_table_set_col_spacings ((GtkTable*) self, (guint) 6);
	g_signal_connect_object (gmw, "data-changed", (GCallback) _gmpc_widget_similar_artist_metadata_changed_gmpc_meta_watcher_data_changed, self, 0);
	_tmp6_ = NULL;
	_tmp4_ = NULL;
	gm_result = (_tmp5_ = gmpc_meta_watcher_get_meta_path (gmw, song, META_ARTIST_SIMILAR, &_tmp4_), item = (_tmp6_ = _tmp4_, (item == NULL) ? NULL : (item = (meta_data_free (item), NULL)), _tmp6_), _tmp5_);
	if (gm_result == META_DATA_AVAILABLE) {
		gmpc_widget_similar_artist_metadata_changed (self, gmw, self->priv->song, META_ARTIST_SIMILAR, gm_result, item);
	}
	(item == NULL) ? NULL : (item = (meta_data_free (item), NULL));
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


/**
     * Expand/collaps the view
     */
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


/* if hte size of the child is small enough to fit in the 
     * small mode don't show the more/less button */
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
	GdkColor _tmp0_ = {0};
	GdkColor _tmp1_ = {0};
	GdkColor _tmp2_ = {0};
	GdkColor _tmp3_ = {0};
	g_return_if_fail (self != NULL);
	g_return_if_fail (frame != NULL);
	gtk_widget_modify_bg (self->priv->pchild, GTK_STATE_NORMAL, (_tmp0_ = gtk_widget_get_style ((GtkWidget*) gtk_widget_get_parent ((GtkWidget*) self))->mid[GTK_STATE_NORMAL], &_tmp0_));
	gtk_widget_modify_base (self->priv->pchild, GTK_STATE_NORMAL, (_tmp1_ = gtk_widget_get_style ((GtkWidget*) gtk_widget_get_parent ((GtkWidget*) self))->mid[GTK_STATE_NORMAL], &_tmp1_));
	gtk_widget_modify_bg ((GtkWidget*) self->priv->eventbox, GTK_STATE_NORMAL, (_tmp2_ = gtk_widget_get_style ((GtkWidget*) gtk_widget_get_parent ((GtkWidget*) self))->dark[GTK_STATE_NORMAL], &_tmp2_));
	gtk_widget_modify_base ((GtkWidget*) self->priv->eventbox, GTK_STATE_NORMAL, (_tmp3_ = gtk_widget_get_style ((GtkWidget*) gtk_widget_get_parent ((GtkWidget*) self))->dark[GTK_STATE_NORMAL], &_tmp3_));
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


/** 
     * @param unique_id  a string used to store/restore state. 
     * @parem markup     a string using following PangoMarkup to show in the label.
     * @param child      a Gtk.Widget that packs into it as child.
     * 
     * @returns a Gmpc.Widget.More object.
     */
static GmpcWidgetMore* gmpc_widget_more_construct (GType object_type, const char* unique_id, const char* markup, GtkWidget* child) {
	GmpcWidgetMore * self;
	char* _tmp1_;
	const char* _tmp0_;
	GtkWidget* _tmp3_;
	GtkWidget* _tmp2_;
	GtkAlignment* _tmp4_;
	GtkEventBox* _tmp5_;
	GtkHBox* hbox;
	GtkLabel* label;
	const char* _tmp6_;
	GtkButton* _tmp7_;
	g_return_val_if_fail (unique_id != NULL, NULL);
	g_return_val_if_fail (markup != NULL, NULL);
	g_return_val_if_fail (child != NULL, NULL);
	self = g_object_newv (object_type, 0, NULL);
	_tmp1_ = NULL;
	_tmp0_ = NULL;
	self->priv->unique_id = (_tmp1_ = (_tmp0_ = unique_id, (_tmp0_ == NULL) ? NULL : g_strdup (_tmp0_)), self->priv->unique_id = (g_free (self->priv->unique_id), NULL), _tmp1_);
	gtk_frame_set_shadow_type ((GtkFrame*) self, GTK_SHADOW_NONE);
	_tmp3_ = NULL;
	_tmp2_ = NULL;
	self->priv->pchild = (_tmp3_ = (_tmp2_ = child, (_tmp2_ == NULL) ? NULL : g_object_ref (_tmp2_)), (self->priv->pchild == NULL) ? NULL : (self->priv->pchild = (g_object_unref (self->priv->pchild), NULL)), _tmp3_);
	_tmp4_ = NULL;
	self->priv->ali = (_tmp4_ = g_object_ref_sink ((GtkAlignment*) gtk_alignment_new (0.f, 0.f, 1.f, 0.f)), (self->priv->ali == NULL) ? NULL : (self->priv->ali = (g_object_unref (self->priv->ali), NULL)), _tmp4_);
	gtk_alignment_set_padding (self->priv->ali, (guint) 1, (guint) 1, (guint) 1, (guint) 1);
	_tmp5_ = NULL;
	self->priv->eventbox = (_tmp5_ = g_object_ref_sink ((GtkEventBox*) gtk_event_box_new ()), (self->priv->eventbox == NULL) ? NULL : (self->priv->eventbox = (g_object_unref (self->priv->eventbox), NULL)), _tmp5_);
	gtk_event_box_set_visible_window (self->priv->eventbox, TRUE);
	gtk_container_add ((GtkContainer*) self, (GtkWidget*) self->priv->eventbox);
	gtk_container_add ((GtkContainer*) self->priv->eventbox, (GtkWidget*) self->priv->ali);
	if (cfg_get_single_value_as_int_with_default (config, "MoreWidget", unique_id, 0) == 1) {
		self->priv->expand_state = 1;
		gtk_widget_set_size_request ((GtkWidget*) self->priv->ali, -1, -1);
	} else {
		gtk_widget_set_size_request ((GtkWidget*) self->priv->ali, -1, self->priv->max_height);
	}
	gtk_container_add ((GtkContainer*) self->priv->ali, child);
	g_signal_connect_object ((GtkWidget*) self, "style-set", (GCallback) _gmpc_widget_more_bg_style_changed_gtk_widget_style_set, self, 0);
	hbox = g_object_ref_sink ((GtkHBox*) gtk_hbox_new (FALSE, 6));
	label = g_object_ref_sink ((GtkLabel*) gtk_label_new (""));
	gtk_label_set_selectable (label, TRUE);
	gtk_label_set_markup (label, markup);
	gtk_box_pack_start ((GtkBox*) hbox, (GtkWidget*) label, FALSE, FALSE, (guint) 0);
	_tmp6_ = NULL;
	if (self->priv->expand_state == 0) {
		_tmp6_ = _ ("(more)");
	} else {
		_tmp6_ = _ ("(less)");
	}
	_tmp7_ = NULL;
	self->priv->expand_button = (_tmp7_ = g_object_ref_sink ((GtkButton*) gtk_button_new_with_label (_tmp6_)), (self->priv->expand_button == NULL) ? NULL : (self->priv->expand_button = (g_object_unref (self->priv->expand_button), NULL)), _tmp7_);
	gtk_button_set_relief (self->priv->expand_button, GTK_RELIEF_NONE);
	g_signal_connect_object (self->priv->expand_button, "clicked", (GCallback) _gmpc_widget_more_expand_gtk_button_clicked, self, 0);
	gtk_box_pack_start ((GtkBox*) hbox, (GtkWidget*) self->priv->expand_button, FALSE, FALSE, (guint) 0);
	gtk_frame_set_label_widget ((GtkFrame*) self, (GtkWidget*) hbox);
	g_signal_connect_object (child, "size-allocate", (GCallback) _gmpc_widget_more_size_changed_gtk_widget_size_allocate, self, 0);
	(hbox == NULL) ? NULL : (hbox = (g_object_unref (hbox), NULL));
	(label == NULL) ? NULL : (label = (g_object_unref (label), NULL));
	return self;
}


static GmpcWidgetMore* gmpc_widget_more_new (const char* unique_id, const char* markup, GtkWidget* child) {
	return gmpc_widget_more_construct (GMPC_WIDGET_TYPE_MORE, unique_id, markup, child);
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
	self->priv->unique_id = NULL;
}


static void gmpc_widget_more_finalize (GObject* obj) {
	GmpcWidgetMore * self;
	self = GMPC_WIDGET_MORE (obj);
	{
		if (self->priv->unique_id != NULL) {
			cfg_set_single_value_as_int (config, "MoreWidget", self->priv->unique_id, self->priv->expand_state);
		}
	}
	(self->priv->ali == NULL) ? NULL : (self->priv->ali = (g_object_unref (self->priv->ali), NULL));
	(self->priv->expand_button == NULL) ? NULL : (self->priv->expand_button = (g_object_unref (self->priv->expand_button), NULL));
	(self->priv->eventbox == NULL) ? NULL : (self->priv->eventbox = (g_object_unref (self->priv->eventbox), NULL));
	(self->priv->pchild == NULL) ? NULL : (self->priv->pchild = (g_object_unref (self->priv->pchild), NULL));
	self->priv->unique_id = (g_free (self->priv->unique_id), NULL);
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


static gint gmpc_now_playing_real_get_version (GmpcPluginBase* base, int* result_length1) {
	GmpcNowPlaying * self;
	gint result;
	gint* _tmp0_;
	self = (GmpcNowPlaying*) base;
	_tmp0_ = NULL;
	result = (_tmp0_ = GMPC_NOW_PLAYING_version, *result_length1 = G_N_ELEMENTS (GMPC_NOW_PLAYING_version), _tmp0_);
	return result;
}


/* Name */
static const char* gmpc_now_playing_real_get_name (GmpcPluginBase* base) {
	GmpcNowPlaying * self;
	const char* result;
	self = (GmpcNowPlaying*) base;
	result = N_ ("Now Playing");
	return result;
}


/* Save our position in the side-bar */
static void gmpc_now_playing_real_save_yourself (GmpcPluginBase* base) {
	GmpcNowPlaying * self;
	self = (GmpcNowPlaying*) base;
	if (self->priv->paned != NULL) {
		GtkScrolledWindow* _tmp0_;
		gtk_object_destroy ((GtkObject*) self->priv->paned);
		_tmp0_ = NULL;
		self->priv->paned = (_tmp0_ = NULL, (self->priv->paned == NULL) ? NULL : (self->priv->paned = (g_object_unref (self->priv->paned), NULL)), _tmp0_);
	}
	if (self->priv->np_ref != NULL) {
		GtkTreePath* path;
		path = gtk_tree_row_reference_get_path (self->priv->np_ref);
		if (path != NULL) {
			gint* _tmp1_;
			gint indices_size;
			gint indices_length1;
			gint* indices;
			_tmp1_ = NULL;
			indices = (_tmp1_ = gtk_tree_path_get_indices (path), indices_length1 = -1, indices_size = indices_length1, _tmp1_);
			cfg_set_single_value_as_int (config, gmpc_plugin_base_get_name ((GmpcPluginBase*) self), "position", indices[0]);
		}
		(path == NULL) ? NULL : (path = (gtk_tree_path_free (path), NULL));
	}
}


static void gmpc_now_playing_status_changed (GmpcNowPlaying* self, GmpcConnection* conn, MpdObj* server, ChangedStatusType what) {
	gboolean _tmp0_;
	gboolean _tmp1_;
	gboolean _tmp2_;
	g_return_if_fail (self != NULL);
	g_return_if_fail (conn != NULL);
	g_return_if_fail (server != NULL);
	if (self->priv->paned == NULL) {
		return;
	}
	_tmp0_ = FALSE;
	_tmp1_ = FALSE;
	_tmp2_ = FALSE;
	if ((what & MPD_CST_SONGID) == MPD_CST_SONGID) {
		_tmp2_ = TRUE;
	} else {
		_tmp2_ = (what & MPD_CST_PLAYLIST) == MPD_CST_PLAYLIST;
	}
	if (_tmp2_) {
		_tmp1_ = TRUE;
	} else {
		_tmp1_ = (what & MPD_CST_STATE) == MPD_CST_STATE;
	}
	if (_tmp1_) {
		_tmp0_ = self->priv->selected;
	} else {
		_tmp0_ = FALSE;
	}
	if (_tmp0_) {
		gmpc_now_playing_update (self);
	}
}


/** 
     * Browser Interface bindings
     */
static void gmpc_now_playing_real_browser_add (GmpcPluginBrowserIface* base, GtkWidget* category_tree) {
	GmpcNowPlaying * self;
	GtkTreeView* _tmp0_;
	GtkTreeView* tree;
	GtkListStore* _tmp1_;
	GtkListStore* store;
	GtkTreeModel* _tmp2_;
	GtkTreeModel* model;
	GtkTreeIter iter = {0};
	GtkTreeRowReference* _tmp4_;
	GtkTreePath* _tmp3_;
	self = (GmpcNowPlaying*) base;
	g_return_if_fail (category_tree != NULL);
	_tmp0_ = NULL;
	tree = (_tmp0_ = GTK_TREE_VIEW (category_tree), (_tmp0_ == NULL) ? NULL : g_object_ref (_tmp0_));
	_tmp1_ = NULL;
	store = (_tmp1_ = GTK_LIST_STORE (gtk_tree_view_get_model (tree)), (_tmp1_ == NULL) ? NULL : g_object_ref (_tmp1_));
	_tmp2_ = NULL;
	model = (_tmp2_ = gtk_tree_view_get_model (tree), (_tmp2_ == NULL) ? NULL : g_object_ref (_tmp2_));
	playlist3_insert_browser (&iter, cfg_get_single_value_as_int_with_default (config, gmpc_plugin_base_get_name ((GmpcPluginBase*) self), "position", 0));
	gtk_list_store_set (store, &iter, 0, ((GmpcPluginBase*) self)->id, 1, _ (gmpc_plugin_base_get_name ((GmpcPluginBase*) self)), 3, "media-audiofile", -1);
	/* Create a row reference */
	_tmp4_ = NULL;
	_tmp3_ = NULL;
	self->priv->np_ref = (_tmp4_ = gtk_tree_row_reference_new (model, _tmp3_ = gtk_tree_model_get_path (model, &iter)), (self->priv->np_ref == NULL) ? NULL : (self->priv->np_ref = (gtk_tree_row_reference_free (self->priv->np_ref), NULL)), _tmp4_);
	(_tmp3_ == NULL) ? NULL : (_tmp3_ = (gtk_tree_path_free (_tmp3_), NULL));
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
	GdkColor _tmp0_ = {0};
	g_return_if_fail (self != NULL);
	g_return_if_fail (bg != NULL);
	gtk_widget_modify_bg ((GtkWidget*) self->priv->container, GTK_STATE_NORMAL, (_tmp0_ = gtk_widget_get_style ((GtkWidget*) self->priv->paned)->base[GTK_STATE_NORMAL], &_tmp0_));
}


/* Handle buttons presses, f.e. for scrolling */
static gboolean gmpc_now_playing_browser_key_release_event (GmpcNowPlaying* self, const GdkEventKey* event) {
	gboolean result;
	GtkAdjustment* _tmp0_;
	GtkAdjustment* adj;
	double incr;
	g_return_val_if_fail (self != NULL, FALSE);
	_tmp0_ = NULL;
	adj = (_tmp0_ = gtk_scrolled_window_get_vadjustment (self->priv->paned), (_tmp0_ == NULL) ? NULL : g_object_ref (_tmp0_));
	incr = (double) 20;
	g_object_get ((GObject*) adj, "step-increment", &incr, NULL);
	if ((*event).keyval == 0xff55) {
		/* GDK_Page_Up*/
		gtk_adjustment_set_value (adj, gtk_adjustment_get_value (adj) - incr);
		result = TRUE;
		(adj == NULL) ? NULL : (adj = (g_object_unref (adj), NULL));
		return result;
	} else {
		if ((*event).keyval == 0xff56) {
			/* GDK_Page_Down*/
			gtk_adjustment_set_value (adj, gtk_adjustment_get_value (adj) + incr);
			result = TRUE;
			(adj == NULL) ? NULL : (adj = (g_object_unref (adj), NULL));
			return result;
		}
	}
	result = FALSE;
	(adj == NULL) ? NULL : (adj = (g_object_unref (adj), NULL));
	return result;
}


static void _gmpc_now_playing_browser_bg_style_changed_gtk_widget_style_set (GtkScrolledWindow* _sender, GtkStyle* previous_style, gpointer self) {
	gmpc_now_playing_browser_bg_style_changed (self, _sender, previous_style);
}


static gboolean _gmpc_now_playing_browser_key_release_event_gtk_widget_key_release_event (GtkScrolledWindow* _sender, const GdkEventKey* event, gpointer self) {
	return gmpc_now_playing_browser_key_release_event (self, event);
}


static void gmpc_now_playing_browser_init (GmpcNowPlaying* self) {
	g_return_if_fail (self != NULL);
	if (self->priv->paned == NULL) {
		GtkScrolledWindow* _tmp0_;
		GtkEventBox* _tmp1_;
		_tmp0_ = NULL;
		self->priv->paned = (_tmp0_ = g_object_ref_sink ((GtkScrolledWindow*) gtk_scrolled_window_new (NULL, NULL)), (self->priv->paned == NULL) ? NULL : (self->priv->paned = (g_object_unref (self->priv->paned), NULL)), _tmp0_);
		gtk_scrolled_window_set_policy (self->priv->paned, GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
		gtk_scrolled_window_set_shadow_type (self->priv->paned, GTK_SHADOW_NONE);
		_tmp1_ = NULL;
		self->priv->container = (_tmp1_ = g_object_ref_sink ((GtkEventBox*) gtk_event_box_new ()), (self->priv->container == NULL) ? NULL : (self->priv->container = (g_object_unref (self->priv->container), NULL)), _tmp1_);
		gtk_event_box_set_visible_window (self->priv->container, TRUE);
		g_signal_connect_object ((GtkWidget*) self->priv->paned, "style-set", (GCallback) _gmpc_now_playing_browser_bg_style_changed_gtk_widget_style_set, self, 0);
		gtk_scrolled_window_add_with_viewport (self->priv->paned, (GtkWidget*) self->priv->container);
		g_object_set ((GObject*) gtk_scrolled_window_get_vadjustment (self->priv->paned), "step-increment", 20.0, NULL);
		/* Bind keys */
		g_signal_connect_object ((GtkWidget*) self->priv->paned, "key-release-event", (GCallback) _gmpc_now_playing_browser_key_release_event_gtk_widget_key_release_event, self, 0);
	}
}


static void gmpc_now_playing_update (GmpcNowPlaying* self) {
	GError * _inner_error_;
	const mpd_Song* _tmp0_;
	mpd_Song* song;
	gboolean _tmp1_;
	g_return_if_fail (self != NULL);
	_inner_error_ = NULL;
	if (self->priv->paned == NULL) {
		return;
	}
	_tmp0_ = NULL;
	song = (_tmp0_ = mpd_playlist_get_current_song (connection), (_tmp0_ == NULL) ? NULL : mpd_songDup (_tmp0_));
	_tmp1_ = FALSE;
	if (song != NULL) {
		_tmp1_ = mpd_player_get_state (connection) != MPD_STATUS_STATE_STOP;
	} else {
		_tmp1_ = FALSE;
	}
	if (_tmp1_) {
		char* checksum;
		checksum = mpd_song_checksum (song);
		if (_vala_strcmp0 (checksum, self->priv->song_checksum) != 0) {
			GList* list;
			GtkWidget* view;
			char* _tmp4_;
			const char* _tmp3_;
			/* Clear */
			list = gtk_container_get_children ((GtkContainer*) self->priv->container);
			{
				GList* child_collection;
				GList* child_it;
				child_collection = list;
				for (child_it = child_collection; child_it != NULL; child_it = child_it->next) {
					GtkWidget* _tmp2_;
					GtkWidget* child;
					_tmp2_ = NULL;
					child = (_tmp2_ = (GtkWidget*) child_it->data, (_tmp2_ == NULL) ? NULL : g_object_ref (_tmp2_));
					{
						gtk_object_destroy ((GtkObject*) child);
						(child == NULL) ? NULL : (child = (g_object_unref (child), NULL));
					}
				}
			}
			view = gmpc_metadata_browser_metadata_box_show_song (self->priv->browser, song, FALSE);
			gtk_container_add ((GtkContainer*) self->priv->container, view);
			_tmp4_ = NULL;
			_tmp3_ = NULL;
			self->priv->song_checksum = (_tmp4_ = (_tmp3_ = checksum, (_tmp3_ == NULL) ? NULL : g_strdup (_tmp3_)), self->priv->song_checksum = (g_free (self->priv->song_checksum), NULL), _tmp4_);
			(list == NULL) ? NULL : (list = (g_list_free (list), NULL));
			(view == NULL) ? NULL : (view = (g_object_unref (view), NULL));
		}
		checksum = (g_free (checksum), NULL);
	} else {
		char* _tmp5_;
		GList* list;
		GtkIconTheme* _tmp7_;
		GtkIconTheme* it;
		GtkIconInfo* info;
		const char* _tmp8_;
		char* path;
		GtkImage* image;
		GtkHBox* hbox;
		GtkLabel* label;
		char* _tmp11_;
		GtkAlignment* ali;
		_tmp5_ = NULL;
		self->priv->song_checksum = (_tmp5_ = NULL, self->priv->song_checksum = (g_free (self->priv->song_checksum), NULL), _tmp5_);
		/* Clear */
		list = gtk_container_get_children ((GtkContainer*) self->priv->container);
		{
			GList* child_collection;
			GList* child_it;
			child_collection = list;
			for (child_it = child_collection; child_it != NULL; child_it = child_it->next) {
				GtkWidget* _tmp6_;
				GtkWidget* child;
				_tmp6_ = NULL;
				child = (_tmp6_ = (GtkWidget*) child_it->data, (_tmp6_ == NULL) ? NULL : g_object_ref (_tmp6_));
				{
					gtk_object_destroy ((GtkObject*) child);
					(child == NULL) ? NULL : (child = (g_object_unref (child), NULL));
				}
			}
		}
		_tmp7_ = NULL;
		it = (_tmp7_ = gtk_icon_theme_get_default (), (_tmp7_ == NULL) ? NULL : g_object_ref (_tmp7_));
		info = gtk_icon_theme_lookup_icon (it, "gmpc", 150, 0);
		_tmp8_ = NULL;
		path = (_tmp8_ = gtk_icon_info_get_filename (info), (_tmp8_ == NULL) ? NULL : g_strdup (_tmp8_));
		image = NULL;
		if (path != NULL) {
			{
				GdkPixbuf* pb;
				GtkImage* _tmp9_;
				pb = gdk_pixbuf_new_from_file_at_scale (path, 150, 150, TRUE, &_inner_error_);
				if (_inner_error_ != NULL) {
					goto __catch0_g_error;
					goto __finally0;
				}
				_tmp9_ = NULL;
				image = (_tmp9_ = g_object_ref_sink ((GtkImage*) gtk_image_new_from_pixbuf (pb)), (image == NULL) ? NULL : (image = (g_object_unref (image), NULL)), _tmp9_);
				(pb == NULL) ? NULL : (pb = (g_object_unref (pb), NULL));
			}
			goto __finally0;
			__catch0_g_error:
			{
				GError * e;
				e = _inner_error_;
				_inner_error_ = NULL;
				{
					(e == NULL) ? NULL : (e = (g_error_free (e), NULL));
				}
			}
			__finally0:
			if (_inner_error_ != NULL) {
				(list == NULL) ? NULL : (list = (g_list_free (list), NULL));
				(it == NULL) ? NULL : (it = (g_object_unref (it), NULL));
				(info == NULL) ? NULL : (info = (gtk_icon_info_free (info), NULL));
				path = (g_free (path), NULL);
				(image == NULL) ? NULL : (image = (g_object_unref (image), NULL));
				(song == NULL) ? NULL : (song = (mpd_freeSong (song), NULL));
				g_critical ("file %s: line %d: uncaught error: %s", __FILE__, __LINE__, _inner_error_->message);
				g_clear_error (&_inner_error_);
				return;
			}
		}
		if (image == NULL) {
			GtkImage* _tmp10_;
			_tmp10_ = NULL;
			image = (_tmp10_ = g_object_ref_sink ((GtkImage*) gtk_image_new_from_icon_name ("gmpc", GTK_ICON_SIZE_DIALOG)), (image == NULL) ? NULL : (image = (g_object_unref (image), NULL)), _tmp10_);
		}
		hbox = g_object_ref_sink ((GtkHBox*) gtk_hbox_new (FALSE, 6));
		label = g_object_ref_sink ((GtkLabel*) gtk_label_new (_ ("Gnome Music Player Client")));
		gtk_label_set_selectable (label, TRUE);
		_tmp11_ = NULL;
		gtk_label_set_markup (label, _tmp11_ = g_strdup_printf ("<span size='%i' weight='bold'>%s</span>", 28 * PANGO_SCALE, _ ("Gnome Music Player Client")));
		_tmp11_ = (g_free (_tmp11_), NULL);
		gtk_box_pack_start ((GtkBox*) hbox, (GtkWidget*) image, FALSE, FALSE, (guint) 0);
		gtk_box_pack_start ((GtkBox*) hbox, (GtkWidget*) label, FALSE, FALSE, (guint) 0);
		ali = g_object_ref_sink ((GtkAlignment*) gtk_alignment_new (0.5f, 0.5f, 0.0f, 0.0f));
		gtk_container_add ((GtkContainer*) ali, (GtkWidget*) hbox);
		gtk_container_add ((GtkContainer*) self->priv->container, (GtkWidget*) ali);
		(list == NULL) ? NULL : (list = (g_list_free (list), NULL));
		(it == NULL) ? NULL : (it = (g_object_unref (it), NULL));
		(info == NULL) ? NULL : (info = (gtk_icon_info_free (info), NULL));
		path = (g_free (path), NULL);
		(image == NULL) ? NULL : (image = (g_object_unref (image), NULL));
		(hbox == NULL) ? NULL : (hbox = (g_object_unref (hbox), NULL));
		(label == NULL) ? NULL : (label = (g_object_unref (label), NULL));
		(ali == NULL) ? NULL : (ali = (g_object_unref (ali), NULL));
	}
	gtk_widget_show_all ((GtkWidget*) self->priv->paned);
	(song == NULL) ? NULL : (song = (mpd_freeSong (song), NULL));
}


/**
     * Makes gmpc jump to the now playing browser 
     */
static void gmpc_now_playing_select_now_playing_browser (GmpcNowPlaying* self, GtkImageMenuItem* item) {
	GtkTreeView* tree;
	GtkTreeSelection* _tmp0_;
	GtkTreeSelection* sel;
	GtkTreePath* path;
	g_return_if_fail (self != NULL);
	g_return_if_fail (item != NULL);
	tree = playlist3_get_category_tree_view ();
	_tmp0_ = NULL;
	sel = (_tmp0_ = gtk_tree_view_get_selection (tree), (_tmp0_ == NULL) ? NULL : g_object_ref (_tmp0_));
	path = gtk_tree_row_reference_get_path (self->priv->np_ref);
	if (path != NULL) {
		gtk_tree_selection_select_path (sel, path);
	}
	(sel == NULL) ? NULL : (sel = (g_object_unref (sel), NULL));
	(path == NULL) ? NULL : (path = (gtk_tree_path_free (path), NULL));
}


static void _gmpc_now_playing_select_now_playing_browser_gtk_menu_item_activate (GtkImageMenuItem* _sender, gpointer self) {
	gmpc_now_playing_select_now_playing_browser (self, _sender);
}


/**
     * Gmpc.Plugin.BrowserIface.add_go_menu 
     */
static gint gmpc_now_playing_real_browser_add_go_menu (GmpcPluginBrowserIface* base, GtkMenu* menu) {
	GmpcNowPlaying * self;
	gint result;
	self = (GmpcNowPlaying*) base;
	g_return_val_if_fail (menu != NULL, 0);
	if (gmpc_plugin_base_get_enabled ((GmpcPluginBase*) self)) {
		GtkImageMenuItem* item;
		GtkImage* _tmp0_;
		item = g_object_ref_sink ((GtkImageMenuItem*) gtk_image_menu_item_new_with_mnemonic (_ ("Now Playing")));
		_tmp0_ = NULL;
		gtk_image_menu_item_set_image (item, (GtkWidget*) (_tmp0_ = g_object_ref_sink ((GtkImage*) gtk_image_new_from_icon_name ("media-audiofile", GTK_ICON_SIZE_MENU))));
		(_tmp0_ == NULL) ? NULL : (_tmp0_ = (g_object_unref (_tmp0_), NULL));
		g_signal_connect_object ((GtkMenuItem*) item, "activate", (GCallback) _gmpc_now_playing_select_now_playing_browser_gtk_menu_item_activate, self, 0);
		gtk_widget_add_accelerator ((GtkWidget*) item, "activate", gtk_menu_get_accel_group (menu), (guint) 0x069, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
		gtk_menu_shell_append ((GtkMenuShell*) menu, (GtkWidget*) ((GtkMenuItem*) item));
		result = 1;
		(item == NULL) ? NULL : (item = (g_object_unref (item), NULL));
		return result;
	}
	result = 0;
	return result;
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
		GmpcMetadataBrowser* _tmp0_;
		/* Set the plugin as an internal one and of type pl_browser */
		((GmpcPluginBase*) self)->plugin_type = 2 | 8;
		/* Track changed status */
		g_signal_connect_object (gmpcconn, "status-changed", (GCallback) _gmpc_now_playing_status_changed_gmpc_connection_status_changed, self, 0);
		/* Create a metadata browser plugin, we abuse for the view */
		_tmp0_ = NULL;
		self->priv->browser = (_tmp0_ = gmpc_metadata_browser_new (), (self->priv->browser == NULL) ? NULL : (self->priv->browser = (g_object_unref (self->priv->browser), NULL)), _tmp0_);
	}
	return obj;
}


static void gmpc_now_playing_class_init (GmpcNowPlayingClass * klass) {
	gmpc_now_playing_parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (GmpcNowPlayingPrivate));
	GMPC_PLUGIN_BASE_CLASS (klass)->get_version = gmpc_now_playing_real_get_version;
	GMPC_PLUGIN_BASE_CLASS (klass)->get_name = gmpc_now_playing_real_get_name;
	GMPC_PLUGIN_BASE_CLASS (klass)->save_yourself = gmpc_now_playing_real_save_yourself;
	G_OBJECT_CLASS (klass)->constructor = gmpc_now_playing_constructor;
	G_OBJECT_CLASS (klass)->finalize = gmpc_now_playing_finalize;
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



static GType gmpc_metadata_browser_hitem_type_get_type (void) {
	static GType gmpc_metadata_browser_hitem_type_type_id = 0;
	if (G_UNLIKELY (gmpc_metadata_browser_hitem_type_type_id == 0)) {
		static const GEnumValue values[] = {{GMPC_METADATA_BROWSER_HITEM_TYPE_CLEAR, "GMPC_METADATA_BROWSER_HITEM_TYPE_CLEAR", "clear"}, {GMPC_METADATA_BROWSER_HITEM_TYPE_ARTIST, "GMPC_METADATA_BROWSER_HITEM_TYPE_ARTIST", "artist"}, {GMPC_METADATA_BROWSER_HITEM_TYPE_ALBUM, "GMPC_METADATA_BROWSER_HITEM_TYPE_ALBUM", "album"}, {GMPC_METADATA_BROWSER_HITEM_TYPE_SONG, "GMPC_METADATA_BROWSER_HITEM_TYPE_SONG", "song"}, {0, NULL, NULL}};
		gmpc_metadata_browser_hitem_type_type_id = g_enum_register_static ("GmpcMetadataBrowserHitemType", values);
	}
	return gmpc_metadata_browser_hitem_type_type_id;
}


static void _g_list_free_gmpc_metadata_browser_hitem_free (GList* self) {
	g_list_foreach (self, (GFunc) gmpc_metadata_browser_hitem_free, NULL);
	g_list_free (self);
}


static gint gmpc_metadata_browser_real_get_version (GmpcPluginBase* base, int* result_length1) {
	GmpcMetadataBrowser * self;
	gint result;
	gint* _tmp0_;
	self = (GmpcMetadataBrowser*) base;
	_tmp0_ = NULL;
	result = (_tmp0_ = GMPC_METADATA_BROWSER_version, *result_length1 = G_N_ELEMENTS (GMPC_METADATA_BROWSER_version), _tmp0_);
	return result;
}


static const char* gmpc_metadata_browser_real_get_name (GmpcPluginBase* base) {
	GmpcMetadataBrowser * self;
	const char* result;
	self = (GmpcMetadataBrowser*) base;
	result = N_ ("Metadata Browser");
	return result;
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
			gint* _tmp0_;
			gint indices_size;
			gint indices_length1;
			gint* indices;
			_tmp0_ = NULL;
			indices = (_tmp0_ = gtk_tree_path_get_indices (path), indices_length1 = -1, indices_size = indices_length1, _tmp0_);
			cfg_set_single_value_as_int (config, gmpc_plugin_base_get_name ((GmpcPluginBase*) self), "position", indices[0]);
		}
		(path == NULL) ? NULL : (path = (gtk_tree_path_free (path), NULL));
	}
}


/**
     * Makes gmpc jump to the metadata browser 
     */
static void gmpc_metadata_browser_select_metadata_browser (GmpcMetadataBrowser* self, GtkImageMenuItem* item) {
	g_return_if_fail (self != NULL);
	g_return_if_fail (item != NULL);
	gmpc_metadata_browser_select_browser (self, NULL);
}


static void _gmpc_metadata_browser_select_metadata_browser_gtk_menu_item_activate (GtkImageMenuItem* _sender, gpointer self) {
	gmpc_metadata_browser_select_metadata_browser (self, _sender);
}


/**
     * Gmpc.Plugin.BrowserIface.add_go_menu 
     */
static gint gmpc_metadata_browser_real_browser_add_go_menu (GmpcPluginBrowserIface* base, GtkMenu* menu) {
	GmpcMetadataBrowser * self;
	gint result;
	self = (GmpcMetadataBrowser*) base;
	g_return_val_if_fail (menu != NULL, 0);
	if (gmpc_plugin_base_get_enabled ((GmpcPluginBase*) self)) {
		GtkImageMenuItem* item;
		GtkImage* _tmp0_;
		item = g_object_ref_sink ((GtkImageMenuItem*) gtk_image_menu_item_new_with_mnemonic (_ (gmpc_plugin_base_get_name ((GmpcPluginBase*) self))));
		_tmp0_ = NULL;
		gtk_image_menu_item_set_image (item, (GtkWidget*) (_tmp0_ = g_object_ref_sink ((GtkImage*) gtk_image_new_from_stock ("gtk-info", GTK_ICON_SIZE_MENU))));
		(_tmp0_ == NULL) ? NULL : (_tmp0_ = (g_object_unref (_tmp0_), NULL));
		g_signal_connect_object ((GtkMenuItem*) item, "activate", (GCallback) _gmpc_metadata_browser_select_metadata_browser_gtk_menu_item_activate, self, 0);
		gtk_widget_add_accelerator ((GtkWidget*) item, "activate", gtk_menu_get_accel_group (menu), (guint) 0xffc1, 0, GTK_ACCEL_VISIBLE);
		gtk_menu_shell_append ((GtkMenuShell*) menu, (GtkWidget*) ((GtkMenuItem*) item));
		result = 1;
		(item == NULL) ? NULL : (item = (g_object_unref (item), NULL));
		return result;
	}
	result = 0;
	return result;
}


/**
     * This builds the browser
     */
static void gmpc_metadata_browser_browser_bg_style_changed (GmpcMetadataBrowser* self, GtkScrolledWindow* bg, GtkStyle* style) {
	GdkColor _tmp0_ = {0};
	g_return_if_fail (self != NULL);
	g_return_if_fail (bg != NULL);
	gtk_widget_modify_bg ((GtkWidget*) self->priv->metadata_box, GTK_STATE_NORMAL, (_tmp0_ = gtk_widget_get_style ((GtkWidget*) self->priv->metadata_sw)->base[GTK_STATE_NORMAL], &_tmp0_));
}


/* This hack makes clicking a selected row again, unselect it */
static gboolean gmpc_metadata_browser_browser_button_press_event (GmpcMetadataBrowser* self, GtkTreeView* tree, const GdkEventButton* event) {
	gboolean result;
	GtkTreePath* path;
	GtkTreePath* _tmp2_;
	gboolean _tmp1_;
	GtkTreePath* _tmp0_;
	g_return_val_if_fail (self != NULL, FALSE);
	g_return_val_if_fail (tree != NULL, FALSE);
	path = NULL;
	if ((*event).button != 1) {
		result = FALSE;
		(path == NULL) ? NULL : (path = (gtk_tree_path_free (path), NULL));
		return result;
	}
	_tmp2_ = NULL;
	_tmp0_ = NULL;
	if ((_tmp1_ = gtk_tree_view_get_path_at_pos (tree, (gint) (*event).x, (gint) (*event).y, &_tmp0_, NULL, NULL, NULL), path = (_tmp2_ = _tmp0_, (path == NULL) ? NULL : (path = (gtk_tree_path_free (path), NULL)), _tmp2_), _tmp1_)) {
		if (gtk_tree_selection_path_is_selected (gtk_tree_view_get_selection (tree), path)) {
			gtk_tree_selection_unselect_path (gtk_tree_view_get_selection (tree), path);
			result = TRUE;
			(path == NULL) ? NULL : (path = (gtk_tree_path_free (path), NULL));
			return result;
		}
	}
	result = FALSE;
	(path == NULL) ? NULL : (path = (gtk_tree_path_free (path), NULL));
	return result;
}


/**
     * Artist tree view functions */
static void gmpc_metadata_browser_browser_artist_entry_changed (GmpcMetadataBrowser* self, GtkEntry* entry) {
	const char* _tmp0_;
	char* text;
	g_return_if_fail (self != NULL);
	g_return_if_fail (entry != NULL);
	_tmp0_ = NULL;
	text = (_tmp0_ = gtk_entry_get_text (entry), (_tmp0_ == NULL) ? NULL : g_strdup (_tmp0_));
	if (strlen (text) > 0) {
		gtk_widget_show ((GtkWidget*) entry);
	} else {
		gtk_widget_hide ((GtkWidget*) entry);
		gtk_widget_grab_focus ((GtkWidget*) self->priv->tree_artist);
	}
	gtk_tree_model_filter_refilter (self->priv->model_filter_artist);
	text = (g_free (text), NULL);
}


static void gmpc_metadata_browser_artist_add_clicked (GmpcMetadataBrowser* self, GtkImageMenuItem* item) {
	char* artist;
	g_return_if_fail (self != NULL);
	g_return_if_fail (item != NULL);
	artist = gmpc_metadata_browser_browser_get_selected_artist (self);
	if (artist != NULL) {
		MpdData* data;
		mpd_database_search_start (connection, TRUE);
		mpd_database_search_add_constraint (connection, MPD_TAG_ITEM_ARTIST, artist);
		data = mpd_database_search_commit (connection);
		data = misc_sort_mpddata_by_album_disc_track (data);
		if (data != NULL) {
			MpdData* _tmp1_;
			const MpdData* _tmp0_;
			_tmp1_ = NULL;
			_tmp0_ = NULL;
			data = (_tmp1_ = (_tmp0_ = mpd_data_get_first (data), (_tmp0_ == NULL) ? NULL :  (_tmp0_)), (data == NULL) ? NULL : (data = (mpd_data_free (data), NULL)), _tmp1_);
			{
				gboolean _tmp2_;
				_tmp2_ = TRUE;
				while (TRUE) {
					if (!_tmp2_) {
						if (!(data != NULL)) {
							break;
						}
					}
					_tmp2_ = FALSE;
					mpd_playlist_queue_add (connection, data->song->file);
					data = mpd_data_get_next (data);
				}
			}
			mpd_playlist_queue_commit (connection);
		}
		(data == NULL) ? NULL : (data = (mpd_data_free (data), NULL));
	}
	artist = (g_free (artist), NULL);
}


static void gmpc_metadata_browser_artist_replace_clicked (GmpcMetadataBrowser* self, GtkImageMenuItem* item) {
	g_return_if_fail (self != NULL);
	g_return_if_fail (item != NULL);
	mpd_playlist_clear (connection);
	gmpc_metadata_browser_artist_add_clicked (self, item);
	mpd_player_play (connection);
}


static void _gmpc_metadata_browser_artist_add_clicked_gtk_menu_item_activate (GtkImageMenuItem* _sender, gpointer self) {
	gmpc_metadata_browser_artist_add_clicked (self, _sender);
}


static void _gmpc_metadata_browser_artist_replace_clicked_gtk_menu_item_activate (GtkImageMenuItem* _sender, gpointer self) {
	gmpc_metadata_browser_artist_replace_clicked (self, _sender);
}


/* Handle right mouse click */
static gboolean gmpc_metadata_browser_artist_browser_button_release_event (GmpcMetadataBrowser* self, GtkTreeView* tree, const GdkEventButton* event) {
	gboolean result;
	g_return_val_if_fail (self != NULL, FALSE);
	g_return_val_if_fail (tree != NULL, FALSE);
	if ((*event).button == 3) {
		if (gtk_tree_selection_count_selected_rows (gtk_tree_view_get_selection (tree)) > 0) {
			GtkMenu* menu;
			GtkImageMenuItem* item;
			GtkImageMenuItem* _tmp0_;
			GtkImage* _tmp1_;
			menu = g_object_ref_sink ((GtkMenu*) gtk_menu_new ());
			item = g_object_ref_sink ((GtkImageMenuItem*) gtk_image_menu_item_new_from_stock ("gtk-add", NULL));
			g_signal_connect_object ((GtkMenuItem*) item, "activate", (GCallback) _gmpc_metadata_browser_artist_add_clicked_gtk_menu_item_activate, self, 0);
			gtk_menu_shell_append ((GtkMenuShell*) menu, (GtkWidget*) ((GtkMenuItem*) item));
			_tmp0_ = NULL;
			item = (_tmp0_ = g_object_ref_sink ((GtkImageMenuItem*) gtk_image_menu_item_new_with_mnemonic (_ ("_Replace"))), (item == NULL) ? NULL : (item = (g_object_unref (item), NULL)), _tmp0_);
			_tmp1_ = NULL;
			gtk_image_menu_item_set_image (item, (GtkWidget*) (_tmp1_ = g_object_ref_sink ((GtkImage*) gtk_image_new_from_stock ("gtk-redo", GTK_ICON_SIZE_MENU))));
			(_tmp1_ == NULL) ? NULL : (_tmp1_ = (g_object_unref (_tmp1_), NULL));
			g_signal_connect_object ((GtkMenuItem*) item, "activate", (GCallback) _gmpc_metadata_browser_artist_replace_clicked_gtk_menu_item_activate, self, 0);
			gtk_menu_shell_append ((GtkMenuShell*) menu, (GtkWidget*) ((GtkMenuItem*) item));
			gtk_menu_popup (menu, NULL, NULL, NULL, NULL, (*event).button, (*event).time);
			gtk_widget_show_all ((GtkWidget*) menu);
			result = TRUE;
			(menu == NULL) ? NULL : (menu = (g_object_unref (menu), NULL));
			(item == NULL) ? NULL : (item = (g_object_unref (item), NULL));
			return result;
		}
	}
	result = FALSE;
	return result;
}


static gboolean gmpc_metadata_browser_visible_func_artist (GmpcMetadataBrowser* self, GtkTreeModel* model, GtkTreeIter* iter) {
	gboolean result;
	const char* _tmp0_;
	char* text;
	char* str;
	gboolean visible;
	gboolean _tmp1_;
	g_return_val_if_fail (self != NULL, FALSE);
	g_return_val_if_fail (model != NULL, FALSE);
	_tmp0_ = NULL;
	text = (_tmp0_ = gtk_entry_get_text (self->priv->artist_filter_entry), (_tmp0_ == NULL) ? NULL : g_strdup (_tmp0_));
	/* Visible if row is non-empty and first column is "HI" */
	str = NULL;
	visible = FALSE;
	if (g_utf8_get_char (g_utf8_offset_to_pointer (text, 0)) == '\0') {
		result = TRUE;
		text = (g_free (text), NULL);
		str = (g_free (str), NULL);
		return result;
	}
	gtk_tree_model_get (model, &(*iter), 7, &str, -1, -1);
	_tmp1_ = FALSE;
	if (str != NULL) {
		char* _tmp5_;
		char* _tmp4_;
		char* _tmp3_;
		char* _tmp2_;
		_tmp5_ = NULL;
		_tmp4_ = NULL;
		_tmp3_ = NULL;
		_tmp2_ = NULL;
		_tmp1_ = strstr (_tmp3_ = g_utf8_normalize (_tmp2_ = g_utf8_casefold (str, -1), -1, G_NORMALIZE_DEFAULT), _tmp5_ = g_utf8_normalize (_tmp4_ = g_utf8_casefold (text, -1), -1, G_NORMALIZE_DEFAULT)) != NULL;
		_tmp5_ = (g_free (_tmp5_), NULL);
		_tmp4_ = (g_free (_tmp4_), NULL);
		_tmp3_ = (g_free (_tmp3_), NULL);
		_tmp2_ = (g_free (_tmp2_), NULL);
	} else {
		_tmp1_ = FALSE;
	}
	if (_tmp1_) {
		visible = TRUE;
	}
	result = visible;
	text = (g_free (text), NULL);
	str = (g_free (str), NULL);
	return result;
}


static gboolean gmpc_metadata_browser_browser_artist_key_press_event (GmpcMetadataBrowser* self, GtkTreeView* widget, const GdkEventKey* event) {
	gboolean result;
	gunichar uc;
	g_return_val_if_fail (self != NULL, FALSE);
	g_return_val_if_fail (widget != NULL, FALSE);
	uc = (gunichar) gdk_keyval_to_unicode ((*event).keyval);
	if (uc > 0) {
		char* outbuf;
		gint i;
		outbuf = g_strdup ("       ");
		i = g_unichar_to_utf8 (uc, outbuf);
		((gchar*) outbuf)[i] = '\0';
		gtk_entry_set_text (self->priv->artist_filter_entry, outbuf);
		gtk_widget_grab_focus ((GtkWidget*) self->priv->artist_filter_entry);
		gtk_editable_set_position ((GtkEditable*) self->priv->artist_filter_entry, 1);
		result = TRUE;
		outbuf = (g_free (outbuf), NULL);
		return result;
	}
	result = FALSE;
	return result;
}


static glong string_get_length (const char* self) {
	glong result;
	g_return_val_if_fail (self != NULL, 0L);
	result = g_utf8_strlen (self, -1);
	return result;
}


/** 
      * Album tree view
      */
static void gmpc_metadata_browser_album_add_clicked (GmpcMetadataBrowser* self, GtkImageMenuItem* item) {
	char* artist;
	g_return_if_fail (self != NULL);
	g_return_if_fail (item != NULL);
	artist = gmpc_metadata_browser_browser_get_selected_artist (self);
	if (artist != NULL) {
		char* albumartist;
		char* album;
		MpdData* data;
		albumartist = NULL;
		album = gmpc_metadata_browser_browser_get_selected_album (self);
		if (album != NULL) {
			MpdData* ydata;
			mpd_database_search_field_start (connection, MPD_TAG_ITEM_ALBUM_ARTIST);
			mpd_database_search_add_constraint (connection, MPD_TAG_ITEM_ALBUM, album);
			ydata = mpd_database_search_commit (connection);
			if (ydata != NULL) {
				if (string_get_length (ydata->tag) > 0) {
					char* _tmp1_;
					const char* _tmp0_;
					_tmp1_ = NULL;
					_tmp0_ = NULL;
					albumartist = (_tmp1_ = (_tmp0_ = ydata->tag, (_tmp0_ == NULL) ? NULL : g_strdup (_tmp0_)), albumartist = (g_free (albumartist), NULL), _tmp1_);
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
		data = misc_sort_mpddata_by_album_disc_track (data);
		if (data != NULL) {
			{
				gboolean _tmp2_;
				_tmp2_ = TRUE;
				while (TRUE) {
					if (!_tmp2_) {
						if (!(data != NULL)) {
							break;
						}
					}
					_tmp2_ = FALSE;
					mpd_playlist_queue_add (connection, data->song->file);
					data = mpd_data_get_next (data);
				}
			}
			mpd_playlist_queue_commit (connection);
		}
		albumartist = (g_free (albumartist), NULL);
		album = (g_free (album), NULL);
		(data == NULL) ? NULL : (data = (mpd_data_free (data), NULL));
	}
	artist = (g_free (artist), NULL);
}


static void gmpc_metadata_browser_album_replace_clicked (GmpcMetadataBrowser* self, GtkImageMenuItem* item) {
	g_return_if_fail (self != NULL);
	g_return_if_fail (item != NULL);
	mpd_playlist_clear (connection);
	gmpc_metadata_browser_album_add_clicked (self, item);
	mpd_player_play (connection);
}


static void _gmpc_metadata_browser_album_add_clicked_gtk_menu_item_activate (GtkImageMenuItem* _sender, gpointer self) {
	gmpc_metadata_browser_album_add_clicked (self, _sender);
}


static void _gmpc_metadata_browser_album_replace_clicked_gtk_menu_item_activate (GtkImageMenuItem* _sender, gpointer self) {
	gmpc_metadata_browser_album_replace_clicked (self, _sender);
}


/* Handle right mouse click */
static gboolean gmpc_metadata_browser_album_browser_button_release_event (GmpcMetadataBrowser* self, GtkTreeView* tree, const GdkEventButton* event) {
	gboolean result;
	g_return_val_if_fail (self != NULL, FALSE);
	g_return_val_if_fail (tree != NULL, FALSE);
	if ((*event).button == 3) {
		if (gtk_tree_selection_count_selected_rows (gtk_tree_view_get_selection (tree)) > 0) {
			GtkMenu* menu;
			GtkImageMenuItem* item;
			GtkImageMenuItem* _tmp0_;
			GtkImage* _tmp1_;
			menu = g_object_ref_sink ((GtkMenu*) gtk_menu_new ());
			item = g_object_ref_sink ((GtkImageMenuItem*) gtk_image_menu_item_new_from_stock ("gtk-add", NULL));
			g_signal_connect_object ((GtkMenuItem*) item, "activate", (GCallback) _gmpc_metadata_browser_album_add_clicked_gtk_menu_item_activate, self, 0);
			gtk_menu_shell_append ((GtkMenuShell*) menu, (GtkWidget*) ((GtkMenuItem*) item));
			_tmp0_ = NULL;
			item = (_tmp0_ = g_object_ref_sink ((GtkImageMenuItem*) gtk_image_menu_item_new_with_mnemonic (_ ("_Replace"))), (item == NULL) ? NULL : (item = (g_object_unref (item), NULL)), _tmp0_);
			_tmp1_ = NULL;
			gtk_image_menu_item_set_image (item, (GtkWidget*) (_tmp1_ = g_object_ref_sink ((GtkImage*) gtk_image_new_from_stock ("gtk-redo", GTK_ICON_SIZE_MENU))));
			(_tmp1_ == NULL) ? NULL : (_tmp1_ = (g_object_unref (_tmp1_), NULL));
			g_signal_connect_object ((GtkMenuItem*) item, "activate", (GCallback) _gmpc_metadata_browser_album_replace_clicked_gtk_menu_item_activate, self, 0);
			gtk_menu_shell_append ((GtkMenuShell*) menu, (GtkWidget*) ((GtkMenuItem*) item));
			gtk_menu_popup (menu, NULL, NULL, NULL, NULL, (*event).button, (*event).time);
			gtk_widget_show_all ((GtkWidget*) menu);
			result = TRUE;
			(menu == NULL) ? NULL : (menu = (g_object_unref (menu), NULL));
			(item == NULL) ? NULL : (item = (g_object_unref (item), NULL));
			return result;
		}
	}
	result = FALSE;
	return result;
}


static gboolean gmpc_metadata_browser_visible_func_album (GmpcMetadataBrowser* self, GtkTreeModel* model, GtkTreeIter* iter) {
	gboolean result;
	const char* _tmp0_;
	char* text;
	char* str;
	gboolean visible;
	gboolean _tmp1_;
	g_return_val_if_fail (self != NULL, FALSE);
	g_return_val_if_fail (model != NULL, FALSE);
	_tmp0_ = NULL;
	text = (_tmp0_ = gtk_entry_get_text (self->priv->album_filter_entry), (_tmp0_ == NULL) ? NULL : g_strdup (_tmp0_));
	/* Visible if row is non-empty and first column is "HI" */
	str = NULL;
	visible = FALSE;
	if (g_utf8_get_char (g_utf8_offset_to_pointer (text, 0)) == '\0') {
		result = TRUE;
		text = (g_free (text), NULL);
		str = (g_free (str), NULL);
		return result;
	}
	gtk_tree_model_get (model, &(*iter), 6, &str, -1, -1);
	_tmp1_ = FALSE;
	if (str != NULL) {
		char* _tmp5_;
		char* _tmp4_;
		char* _tmp3_;
		char* _tmp2_;
		_tmp5_ = NULL;
		_tmp4_ = NULL;
		_tmp3_ = NULL;
		_tmp2_ = NULL;
		_tmp1_ = strstr (_tmp3_ = g_utf8_normalize (_tmp2_ = g_utf8_casefold (str, -1), -1, G_NORMALIZE_DEFAULT), _tmp5_ = g_utf8_normalize (_tmp4_ = g_utf8_casefold (text, -1), -1, G_NORMALIZE_DEFAULT)) != NULL;
		_tmp5_ = (g_free (_tmp5_), NULL);
		_tmp4_ = (g_free (_tmp4_), NULL);
		_tmp3_ = (g_free (_tmp3_), NULL);
		_tmp2_ = (g_free (_tmp2_), NULL);
	} else {
		_tmp1_ = FALSE;
	}
	if (_tmp1_) {
		visible = TRUE;
	}
	result = visible;
	text = (g_free (text), NULL);
	str = (g_free (str), NULL);
	return result;
}


static gboolean gmpc_metadata_browser_browser_album_key_press_event (GmpcMetadataBrowser* self, GtkTreeView* widget, const GdkEventKey* event) {
	gboolean result;
	gunichar uc;
	g_return_val_if_fail (self != NULL, FALSE);
	g_return_val_if_fail (widget != NULL, FALSE);
	uc = (gunichar) gdk_keyval_to_unicode ((*event).keyval);
	if (uc > 0) {
		char* outbuf;
		gint i;
		outbuf = g_strdup ("       ");
		i = g_unichar_to_utf8 (uc, outbuf);
		((gchar*) outbuf)[i] = '\0';
		gtk_entry_set_text (self->priv->album_filter_entry, outbuf);
		gtk_widget_grab_focus ((GtkWidget*) self->priv->album_filter_entry);
		gtk_editable_set_position ((GtkEditable*) self->priv->album_filter_entry, 1);
		result = TRUE;
		outbuf = (g_free (outbuf), NULL);
		return result;
	}
	result = FALSE;
	return result;
}


static void gmpc_metadata_browser_browser_album_entry_changed (GmpcMetadataBrowser* self, GtkEntry* entry) {
	const char* _tmp0_;
	char* text;
	g_return_if_fail (self != NULL);
	g_return_if_fail (entry != NULL);
	_tmp0_ = NULL;
	text = (_tmp0_ = gtk_entry_get_text (entry), (_tmp0_ == NULL) ? NULL : g_strdup (_tmp0_));
	if (strlen (text) > 0) {
		gtk_widget_show ((GtkWidget*) entry);
	} else {
		gtk_widget_hide ((GtkWidget*) entry);
		gtk_widget_grab_focus ((GtkWidget*) self->priv->tree_album);
	}
	gtk_tree_model_filter_refilter (self->priv->model_filter_album);
	text = (g_free (text), NULL);
}


/**
      * Songs 
      */
static void gmpc_metadata_browser_song_add_clicked (GmpcMetadataBrowser* self, GtkImageMenuItem* item) {
	mpd_Song* song;
	g_return_if_fail (self != NULL);
	g_return_if_fail (item != NULL);
	song = gmpc_metadata_browser_browser_get_selected_song (self);
	if (song != NULL) {
		mpd_playlist_add (connection, song->file);
	}
	(song == NULL) ? NULL : (song = (mpd_freeSong (song), NULL));
}


static void gmpc_metadata_browser_song_replace_clicked (GmpcMetadataBrowser* self, GtkImageMenuItem* item) {
	g_return_if_fail (self != NULL);
	g_return_if_fail (item != NULL);
	mpd_playlist_clear (connection);
	gmpc_metadata_browser_song_add_clicked (self, item);
	mpd_player_play (connection);
}


static void _gmpc_metadata_browser_song_add_clicked_gtk_menu_item_activate (GtkImageMenuItem* _sender, gpointer self) {
	gmpc_metadata_browser_song_add_clicked (self, _sender);
}


static void _gmpc_metadata_browser_song_replace_clicked_gtk_menu_item_activate (GtkImageMenuItem* _sender, gpointer self) {
	gmpc_metadata_browser_song_replace_clicked (self, _sender);
}


/* Handle right mouse click */
static gboolean gmpc_metadata_browser_song_browser_button_release_event (GmpcMetadataBrowser* self, GtkTreeView* tree, const GdkEventButton* event) {
	gboolean result;
	g_return_val_if_fail (self != NULL, FALSE);
	g_return_val_if_fail (tree != NULL, FALSE);
	if ((*event).button == 3) {
		if (gtk_tree_selection_count_selected_rows (gtk_tree_view_get_selection (tree)) > 0) {
			GtkMenu* menu;
			GtkImageMenuItem* item;
			GtkImageMenuItem* _tmp0_;
			GtkImage* _tmp1_;
			menu = g_object_ref_sink ((GtkMenu*) gtk_menu_new ());
			item = g_object_ref_sink ((GtkImageMenuItem*) gtk_image_menu_item_new_from_stock ("gtk-add", NULL));
			g_signal_connect_object ((GtkMenuItem*) item, "activate", (GCallback) _gmpc_metadata_browser_song_add_clicked_gtk_menu_item_activate, self, 0);
			gtk_menu_shell_append ((GtkMenuShell*) menu, (GtkWidget*) ((GtkMenuItem*) item));
			_tmp0_ = NULL;
			item = (_tmp0_ = g_object_ref_sink ((GtkImageMenuItem*) gtk_image_menu_item_new_with_mnemonic (_ ("_Replace"))), (item == NULL) ? NULL : (item = (g_object_unref (item), NULL)), _tmp0_);
			_tmp1_ = NULL;
			gtk_image_menu_item_set_image (item, (GtkWidget*) (_tmp1_ = g_object_ref_sink ((GtkImage*) gtk_image_new_from_stock ("gtk-redo", GTK_ICON_SIZE_MENU))));
			(_tmp1_ == NULL) ? NULL : (_tmp1_ = (g_object_unref (_tmp1_), NULL));
			g_signal_connect_object ((GtkMenuItem*) item, "activate", (GCallback) _gmpc_metadata_browser_song_replace_clicked_gtk_menu_item_activate, self, 0);
			gtk_menu_shell_append ((GtkMenuShell*) menu, (GtkWidget*) ((GtkMenuItem*) item));
			gtk_menu_popup (menu, NULL, NULL, NULL, NULL, (*event).button, (*event).time);
			gtk_widget_show_all ((GtkWidget*) menu);
			result = TRUE;
			(menu == NULL) ? NULL : (menu = (g_object_unref (menu), NULL));
			(item == NULL) ? NULL : (item = (g_object_unref (item), NULL));
			return result;
		}
	}
	result = FALSE;
	return result;
}


static gboolean gmpc_metadata_browser_browser_button_release_event (GmpcMetadataBrowser* self, GtkWidget* widget, const GdkEventButton* event) {
	gboolean result;
	g_return_val_if_fail (self != NULL, FALSE);
	g_return_val_if_fail (widget != NULL, FALSE);
	if ((*event).button == 8) {
		gmpc_metadata_browser_history_previous (self);
		result = TRUE;
		return result;
	} else {
		if ((*event).button == 9) {
			gmpc_metadata_browser_history_next (self);
			result = TRUE;
			return result;
		}
	}
	result = FALSE;
	return result;
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


static gboolean _gmpc_metadata_browser_artist_browser_button_release_event_gtk_widget_button_release_event (GtkTreeView* _sender, const GdkEventButton* event, gpointer self) {
	return gmpc_metadata_browser_artist_browser_button_release_event (self, _sender, event);
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


static gboolean _gmpc_metadata_browser_album_browser_button_release_event_gtk_widget_button_release_event (GtkTreeView* _sender, const GdkEventButton* event, gpointer self) {
	return gmpc_metadata_browser_album_browser_button_release_event (self, _sender, event);
}


static gboolean _gmpc_metadata_browser_browser_album_key_press_event_gtk_widget_key_press_event (GtkTreeView* _sender, const GdkEventKey* event, gpointer self) {
	return gmpc_metadata_browser_browser_album_key_press_event (self, _sender, event);
}


static void _gmpc_metadata_browser_browser_album_changed_gtk_tree_selection_changed (GtkTreeSelection* _sender, gpointer self) {
	gmpc_metadata_browser_browser_album_changed (self, _sender);
}


static gboolean _gmpc_metadata_browser_song_browser_button_release_event_gtk_widget_button_release_event (GtkTreeView* _sender, const GdkEventButton* event, gpointer self) {
	return gmpc_metadata_browser_song_browser_button_release_event (self, _sender, event);
}


static void _gmpc_metadata_browser_browser_songs_changed_gtk_tree_selection_changed (GtkTreeSelection* _sender, gpointer self) {
	gmpc_metadata_browser_browser_songs_changed (self, _sender);
}


static void _gmpc_metadata_browser_browser_bg_style_changed_gtk_widget_style_set (GtkScrolledWindow* _sender, GtkStyle* previous_style, gpointer self) {
	gmpc_metadata_browser_browser_bg_style_changed (self, _sender, previous_style);
}


static gboolean _gmpc_metadata_browser_browser_button_release_event_gtk_widget_button_release_event (GtkWidget* _sender, const GdkEventButton* event, gpointer self) {
	return gmpc_metadata_browser_browser_button_release_event (self, _sender, event);
}


static void gmpc_metadata_browser_browser_init (GmpcMetadataBrowser* self) {
	g_return_if_fail (self != NULL);
	if (self->priv->paned == NULL) {
		GtkPaned* _tmp0_;
		GtkBox* _tmp1_;
		GtkVBox* box;
		GtkEntry* _tmp2_;
		GtkScrolledWindow* sw;
		GmpcMpdDataModel* _tmp3_;
		GtkTreeModelFilter* _tmp4_;
		GtkTreeView* _tmp5_;
		GmpcMpdDataTreeviewTooltip* _tmp6_;
		GtkTreeViewColumn* column;
		GtkCellRendererPixbuf* prenderer;
		GtkCellRendererText* trenderer;
		GtkVBox* _tmp7_;
		GtkEntry* _tmp8_;
		GtkScrolledWindow* _tmp9_;
		GmpcMpdDataModel* _tmp10_;
		GtkTreeModelFilter* _tmp11_;
		GtkTreeView* _tmp12_;
		GmpcMpdDataTreeviewTooltip* _tmp13_;
		GtkTreeViewColumn* _tmp14_;
		GtkCellRendererPixbuf* _tmp15_;
		GtkTreeViewColumn* _tmp16_;
		GtkCellRendererText* _tmp17_;
		GtkTreeViewColumn* _tmp18_;
		GtkCellRendererText* _tmp19_;
		GtkScrolledWindow* _tmp20_;
		GmpcMpdDataModel* _tmp21_;
		GtkTreeView* _tmp22_;
		GtkTreeViewColumn* _tmp23_;
		GtkCellRendererPixbuf* _tmp24_;
		GtkCellRendererText* _tmp25_;
		GtkTreeViewColumn* _tmp26_;
		GtkCellRendererText* _tmp27_;
		GtkScrolledWindow* _tmp28_;
		GtkEventBox* _tmp29_;
		_tmp0_ = NULL;
		self->priv->paned = (_tmp0_ = (GtkPaned*) g_object_ref_sink ((GtkHPaned*) gtk_hpaned_new ()), (self->priv->paned == NULL) ? NULL : (self->priv->paned = (g_object_unref (self->priv->paned), NULL)), _tmp0_);
		gmpc_paned_size_group_add_paned (paned_size_group, self->priv->paned);
		/* Bow with browsers */
		_tmp1_ = NULL;
		self->priv->browser_box = (_tmp1_ = (GtkBox*) g_object_ref_sink ((GtkVBox*) gtk_vbox_new (TRUE, 6)), (self->priv->browser_box == NULL) ? NULL : (self->priv->browser_box = (g_object_unref (self->priv->browser_box), NULL)), _tmp1_);
		gtk_paned_add1 (self->priv->paned, (GtkWidget*) self->priv->browser_box);
		/* Artist list  */
		box = g_object_ref_sink ((GtkVBox*) gtk_vbox_new (FALSE, 6));
		gtk_box_pack_start (self->priv->browser_box, (GtkWidget*) box, TRUE, TRUE, (guint) 0);
		_tmp2_ = NULL;
		self->priv->artist_filter_entry = (_tmp2_ = g_object_ref_sink ((GtkEntry*) gtk_entry_new ()), (self->priv->artist_filter_entry == NULL) ? NULL : (self->priv->artist_filter_entry = (g_object_unref (self->priv->artist_filter_entry), NULL)), _tmp2_);
		gtk_widget_set_no_show_all ((GtkWidget*) self->priv->artist_filter_entry, TRUE);
		g_signal_connect_object ((GtkEditable*) self->priv->artist_filter_entry, "changed", (GCallback) _gmpc_metadata_browser_browser_artist_entry_changed_gtk_editable_changed, self, 0);
		gtk_box_pack_start ((GtkBox*) box, (GtkWidget*) self->priv->artist_filter_entry, FALSE, FALSE, (guint) 0);
		sw = g_object_ref_sink ((GtkScrolledWindow*) gtk_scrolled_window_new (NULL, NULL));
		gtk_scrolled_window_set_policy (sw, GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
		gtk_scrolled_window_set_shadow_type (sw, GTK_SHADOW_ETCHED_IN);
		gtk_box_pack_start ((GtkBox*) box, (GtkWidget*) sw, TRUE, TRUE, (guint) 0);
		_tmp3_ = NULL;
		self->priv->model_artist = (_tmp3_ = gmpc_mpddata_model_new (), (self->priv->model_artist == NULL) ? NULL : (self->priv->model_artist = (g_object_unref (self->priv->model_artist), NULL)), _tmp3_);
		_tmp4_ = NULL;
		self->priv->model_filter_artist = (_tmp4_ = (GtkTreeModelFilter*) gtk_tree_model_filter_new ((GtkTreeModel*) self->priv->model_artist, NULL), (self->priv->model_filter_artist == NULL) ? NULL : (self->priv->model_filter_artist = (g_object_unref (self->priv->model_filter_artist), NULL)), _tmp4_);
		gtk_tree_model_filter_set_visible_func (self->priv->model_filter_artist, _gmpc_metadata_browser_visible_func_artist_gtk_tree_model_filter_visible_func, g_object_ref (self), g_object_unref);
		_tmp5_ = NULL;
		self->priv->tree_artist = (_tmp5_ = g_object_ref_sink ((GtkTreeView*) gtk_tree_view_new_with_model ((GtkTreeModel*) self->priv->model_filter_artist)), (self->priv->tree_artist == NULL) ? NULL : (self->priv->tree_artist = (g_object_unref (self->priv->tree_artist), NULL)), _tmp5_);
		_tmp6_ = NULL;
		_tmp6_ = g_object_ref_sink (gmpc_mpd_data_treeview_tooltip_new (self->priv->tree_artist, META_ARTIST_ART));
		(_tmp6_ == NULL) ? NULL : (_tmp6_ = (g_object_unref (_tmp6_), NULL));
		g_signal_connect_object ((GtkWidget*) self->priv->tree_artist, "button-press-event", (GCallback) _gmpc_metadata_browser_browser_button_press_event_gtk_widget_button_press_event, self, 0);
		g_signal_connect_object ((GtkWidget*) self->priv->tree_artist, "button-release-event", (GCallback) _gmpc_metadata_browser_artist_browser_button_release_event_gtk_widget_button_release_event, self, 0);
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
		_tmp7_ = NULL;
		box = (_tmp7_ = g_object_ref_sink ((GtkVBox*) gtk_vbox_new (FALSE, 6)), (box == NULL) ? NULL : (box = (g_object_unref (box), NULL)), _tmp7_);
		gtk_box_pack_start (self->priv->browser_box, (GtkWidget*) box, TRUE, TRUE, (guint) 0);
		_tmp8_ = NULL;
		self->priv->album_filter_entry = (_tmp8_ = g_object_ref_sink ((GtkEntry*) gtk_entry_new ()), (self->priv->album_filter_entry == NULL) ? NULL : (self->priv->album_filter_entry = (g_object_unref (self->priv->album_filter_entry), NULL)), _tmp8_);
		gtk_widget_set_no_show_all ((GtkWidget*) self->priv->album_filter_entry, TRUE);
		g_signal_connect_object ((GtkEditable*) self->priv->album_filter_entry, "changed", (GCallback) _gmpc_metadata_browser_browser_album_entry_changed_gtk_editable_changed, self, 0);
		gtk_box_pack_start ((GtkBox*) box, (GtkWidget*) self->priv->album_filter_entry, FALSE, FALSE, (guint) 0);
		_tmp9_ = NULL;
		sw = (_tmp9_ = g_object_ref_sink ((GtkScrolledWindow*) gtk_scrolled_window_new (NULL, NULL)), (sw == NULL) ? NULL : (sw = (g_object_unref (sw), NULL)), _tmp9_);
		gtk_scrolled_window_set_policy (sw, GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
		gtk_scrolled_window_set_shadow_type (sw, GTK_SHADOW_ETCHED_IN);
		gtk_box_pack_start ((GtkBox*) box, (GtkWidget*) sw, TRUE, TRUE, (guint) 0);
		_tmp10_ = NULL;
		self->priv->model_albums = (_tmp10_ = gmpc_mpddata_model_new (), (self->priv->model_albums == NULL) ? NULL : (self->priv->model_albums = (g_object_unref (self->priv->model_albums), NULL)), _tmp10_);
		_tmp11_ = NULL;
		self->priv->model_filter_album = (_tmp11_ = (GtkTreeModelFilter*) gtk_tree_model_filter_new ((GtkTreeModel*) self->priv->model_albums, NULL), (self->priv->model_filter_album == NULL) ? NULL : (self->priv->model_filter_album = (g_object_unref (self->priv->model_filter_album), NULL)), _tmp11_);
		gtk_tree_model_filter_set_visible_func (self->priv->model_filter_album, _gmpc_metadata_browser_visible_func_album_gtk_tree_model_filter_visible_func, g_object_ref (self), g_object_unref);
		_tmp12_ = NULL;
		self->priv->tree_album = (_tmp12_ = g_object_ref_sink ((GtkTreeView*) gtk_tree_view_new_with_model ((GtkTreeModel*) self->priv->model_filter_album)), (self->priv->tree_album == NULL) ? NULL : (self->priv->tree_album = (g_object_unref (self->priv->tree_album), NULL)), _tmp12_);
		_tmp13_ = NULL;
		_tmp13_ = g_object_ref_sink (gmpc_mpd_data_treeview_tooltip_new (self->priv->tree_album, META_ALBUM_ART));
		(_tmp13_ == NULL) ? NULL : (_tmp13_ = (g_object_unref (_tmp13_), NULL));
		g_signal_connect_object ((GtkWidget*) self->priv->tree_album, "button-press-event", (GCallback) _gmpc_metadata_browser_browser_button_press_event_gtk_widget_button_press_event, self, 0);
		g_signal_connect_object ((GtkWidget*) self->priv->tree_album, "button-release-event", (GCallback) _gmpc_metadata_browser_album_browser_button_release_event_gtk_widget_button_release_event, self, 0);
		g_signal_connect_object ((GtkWidget*) self->priv->tree_album, "key-press-event", (GCallback) _gmpc_metadata_browser_browser_album_key_press_event_gtk_widget_key_press_event, self, 0);
		gtk_container_add ((GtkContainer*) sw, (GtkWidget*) self->priv->tree_album);
		/* setup the columns */
		_tmp14_ = NULL;
		column = (_tmp14_ = g_object_ref_sink (gtk_tree_view_column_new ()), (column == NULL) ? NULL : (column = (g_object_unref (column), NULL)), _tmp14_);
		_tmp15_ = NULL;
		prenderer = (_tmp15_ = g_object_ref_sink ((GtkCellRendererPixbuf*) gtk_cell_renderer_pixbuf_new ()), (prenderer == NULL) ? NULL : (prenderer = (g_object_unref (prenderer), NULL)), _tmp15_);
		g_object_set ((GObject*) prenderer, "height", self->priv->model_albums->icon_size, NULL);
		gtk_cell_layout_pack_start ((GtkCellLayout*) column, (GtkCellRenderer*) prenderer, FALSE);
		gtk_cell_layout_add_attribute ((GtkCellLayout*) column, (GtkCellRenderer*) prenderer, "pixbuf", 27);
		gtk_tree_view_append_column (self->priv->tree_album, column);
		_tmp16_ = NULL;
		column = (_tmp16_ = g_object_ref_sink (gtk_tree_view_column_new ()), (column == NULL) ? NULL : (column = (g_object_unref (column), NULL)), _tmp16_);
		_tmp17_ = NULL;
		trenderer = (_tmp17_ = g_object_ref_sink ((GtkCellRendererText*) gtk_cell_renderer_text_new ()), (trenderer == NULL) ? NULL : (trenderer = (g_object_unref (trenderer), NULL)), _tmp17_);
		gtk_cell_layout_pack_start ((GtkCellLayout*) column, (GtkCellRenderer*) trenderer, TRUE);
		gtk_cell_layout_add_attribute ((GtkCellLayout*) column, (GtkCellRenderer*) trenderer, "text", 14);
		gtk_tree_view_append_column (self->priv->tree_album, column);
		gtk_tree_view_column_set_title (column, _ ("Year"));
		_tmp18_ = NULL;
		column = (_tmp18_ = g_object_ref_sink (gtk_tree_view_column_new ()), (column == NULL) ? NULL : (column = (g_object_unref (column), NULL)), _tmp18_);
		_tmp19_ = NULL;
		trenderer = (_tmp19_ = g_object_ref_sink ((GtkCellRendererText*) gtk_cell_renderer_text_new ()), (trenderer == NULL) ? NULL : (trenderer = (g_object_unref (trenderer), NULL)), _tmp19_);
		gtk_cell_layout_pack_start ((GtkCellLayout*) column, (GtkCellRenderer*) trenderer, TRUE);
		gtk_cell_layout_add_attribute ((GtkCellLayout*) column, (GtkCellRenderer*) trenderer, "text", 6);
		gtk_tree_view_append_column (self->priv->tree_album, column);
		gtk_tree_view_set_search_column (self->priv->tree_album, 6);
		gtk_tree_view_column_set_title (column, _ ("Album"));
		g_signal_connect_object (gtk_tree_view_get_selection (self->priv->tree_album), "changed", (GCallback) _gmpc_metadata_browser_browser_album_changed_gtk_tree_selection_changed, self, 0);
		/* Song list */
		_tmp20_ = NULL;
		sw = (_tmp20_ = g_object_ref_sink ((GtkScrolledWindow*) gtk_scrolled_window_new (NULL, NULL)), (sw == NULL) ? NULL : (sw = (g_object_unref (sw), NULL)), _tmp20_);
		gtk_scrolled_window_set_policy (sw, GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
		gtk_scrolled_window_set_shadow_type (sw, GTK_SHADOW_ETCHED_IN);
		gtk_box_pack_start (self->priv->browser_box, (GtkWidget*) sw, TRUE, TRUE, (guint) 0);
		_tmp21_ = NULL;
		self->priv->model_songs = (_tmp21_ = gmpc_mpddata_model_new (), (self->priv->model_songs == NULL) ? NULL : (self->priv->model_songs = (g_object_unref (self->priv->model_songs), NULL)), _tmp21_);
		_tmp22_ = NULL;
		self->priv->tree_songs = (_tmp22_ = g_object_ref_sink ((GtkTreeView*) gtk_tree_view_new_with_model ((GtkTreeModel*) self->priv->model_songs)), (self->priv->tree_songs == NULL) ? NULL : (self->priv->tree_songs = (g_object_unref (self->priv->tree_songs), NULL)), _tmp22_);
		g_signal_connect_object ((GtkWidget*) self->priv->tree_songs, "button-press-event", (GCallback) _gmpc_metadata_browser_browser_button_press_event_gtk_widget_button_press_event, self, 0);
		g_signal_connect_object ((GtkWidget*) self->priv->tree_songs, "button-release-event", (GCallback) _gmpc_metadata_browser_song_browser_button_release_event_gtk_widget_button_release_event, self, 0);
		gtk_container_add ((GtkContainer*) sw, (GtkWidget*) self->priv->tree_songs);
		/* setup the columns */
		_tmp23_ = NULL;
		column = (_tmp23_ = g_object_ref_sink (gtk_tree_view_column_new ()), (column == NULL) ? NULL : (column = (g_object_unref (column), NULL)), _tmp23_);
		_tmp24_ = NULL;
		prenderer = (_tmp24_ = g_object_ref_sink ((GtkCellRendererPixbuf*) gtk_cell_renderer_pixbuf_new ()), (prenderer == NULL) ? NULL : (prenderer = (g_object_unref (prenderer), NULL)), _tmp24_);
		gtk_cell_layout_pack_start ((GtkCellLayout*) column, (GtkCellRenderer*) prenderer, FALSE);
		gtk_cell_layout_add_attribute ((GtkCellLayout*) column, (GtkCellRenderer*) prenderer, "icon-name", 23);
		_tmp25_ = NULL;
		trenderer = (_tmp25_ = g_object_ref_sink ((GtkCellRendererText*) gtk_cell_renderer_text_new ()), (trenderer == NULL) ? NULL : (trenderer = (g_object_unref (trenderer), NULL)), _tmp25_);
		gtk_cell_layout_pack_start ((GtkCellLayout*) column, (GtkCellRenderer*) trenderer, FALSE);
		gtk_cell_layout_add_attribute ((GtkCellLayout*) column, (GtkCellRenderer*) trenderer, "text", 10);
		gtk_tree_view_column_set_title (column, _ ("Track"));
		gtk_tree_view_append_column (self->priv->tree_songs, column);
		_tmp26_ = NULL;
		column = (_tmp26_ = g_object_ref_sink (gtk_tree_view_column_new ()), (column == NULL) ? NULL : (column = (g_object_unref (column), NULL)), _tmp26_);
		_tmp27_ = NULL;
		trenderer = (_tmp27_ = g_object_ref_sink ((GtkCellRendererText*) gtk_cell_renderer_text_new ()), (trenderer == NULL) ? NULL : (trenderer = (g_object_unref (trenderer), NULL)), _tmp27_);
		gtk_cell_layout_pack_start ((GtkCellLayout*) column, (GtkCellRenderer*) trenderer, TRUE);
		gtk_cell_layout_add_attribute ((GtkCellLayout*) column, (GtkCellRenderer*) trenderer, "text", 7);
		gtk_tree_view_append_column (self->priv->tree_songs, column);
		gtk_tree_view_set_search_column (self->priv->tree_songs, 7);
		gtk_tree_view_column_set_title (column, _ ("Songs"));
		g_signal_connect_object (gtk_tree_view_get_selection (self->priv->tree_songs), "changed", (GCallback) _gmpc_metadata_browser_browser_songs_changed_gtk_tree_selection_changed, self, 0);
		/* The right view */
		_tmp28_ = NULL;
		self->priv->metadata_sw = (_tmp28_ = g_object_ref_sink ((GtkScrolledWindow*) gtk_scrolled_window_new (NULL, NULL)), (self->priv->metadata_sw == NULL) ? NULL : (self->priv->metadata_sw = (g_object_unref (self->priv->metadata_sw), NULL)), _tmp28_);
		gtk_scrolled_window_set_policy (self->priv->metadata_sw, GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
		g_signal_connect_object ((GtkWidget*) self->priv->metadata_sw, "style-set", (GCallback) _gmpc_metadata_browser_browser_bg_style_changed_gtk_widget_style_set, self, 0);
		_tmp29_ = NULL;
		self->priv->metadata_box = (_tmp29_ = g_object_ref_sink ((GtkEventBox*) gtk_event_box_new ()), (self->priv->metadata_box == NULL) ? NULL : (self->priv->metadata_box = (g_object_unref (self->priv->metadata_box), NULL)), _tmp29_);
		gtk_event_box_set_visible_window (self->priv->metadata_box, TRUE);
		gtk_scrolled_window_add_with_viewport (self->priv->metadata_sw, (GtkWidget*) self->priv->metadata_box);
		gtk_paned_add2 (self->priv->paned, (GtkWidget*) self->priv->metadata_sw);
		g_signal_connect_object ((GtkWidget*) self->priv->paned, "button-release-event", (GCallback) _gmpc_metadata_browser_browser_button_release_event_gtk_widget_button_release_event, self, 0);
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
	MpdData* _tmp0_;
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
	data = misc_sort_mpddata_by_album_disc_track (data);
	_tmp0_ = NULL;
	gmpc_mpddata_model_set_mpd_data (self->priv->model_artist, (_tmp0_ = data, data = NULL, _tmp0_));
	(data == NULL) ? NULL : (data = (mpd_data_free (data), NULL));
}


static char* gmpc_metadata_browser_browser_get_selected_artist (GmpcMetadataBrowser* self) {
	char* result;
	GtkTreeIter iter = {0};
	GtkTreeSelection* _tmp0_;
	GtkTreeSelection* sel;
	GtkTreeModel* model;
	GtkTreeModel* _tmp4_;
	GtkTreeModel* _tmp3_;
	gboolean _tmp2_;
	GtkTreeModel* _tmp1_;
	g_return_val_if_fail (self != NULL, NULL);
	_tmp0_ = NULL;
	sel = (_tmp0_ = gtk_tree_view_get_selection (self->priv->tree_artist), (_tmp0_ == NULL) ? NULL : g_object_ref (_tmp0_));
	model = NULL;
	/*this.model_artist;*/
	_tmp4_ = NULL;
	_tmp3_ = NULL;
	_tmp1_ = NULL;
	if ((_tmp2_ = gtk_tree_selection_get_selected (sel, &_tmp1_, &iter), model = (_tmp3_ = (_tmp4_ = _tmp1_, (_tmp4_ == NULL) ? NULL : g_object_ref (_tmp4_)), (model == NULL) ? NULL : (model = (g_object_unref (model), NULL)), _tmp3_), _tmp2_)) {
		char* artist;
		artist = NULL;
		gtk_tree_model_get (model, &iter, 7, &artist, -1, -1);
		result = artist;
		(sel == NULL) ? NULL : (sel = (g_object_unref (sel), NULL));
		(model == NULL) ? NULL : (model = (g_object_unref (model), NULL));
		return result;
	}
	result = NULL;
	(sel == NULL) ? NULL : (sel = (g_object_unref (sel), NULL));
	(model == NULL) ? NULL : (model = (g_object_unref (model), NULL));
	return result;
}


static char* gmpc_metadata_browser_browser_get_selected_album (GmpcMetadataBrowser* self) {
	char* result;
	GtkTreeIter iter = {0};
	GtkTreeSelection* _tmp0_;
	GtkTreeSelection* sel;
	GtkTreeModel* model;
	GtkTreeModel* _tmp4_;
	GtkTreeModel* _tmp3_;
	gboolean _tmp2_;
	GtkTreeModel* _tmp1_;
	g_return_val_if_fail (self != NULL, NULL);
	_tmp0_ = NULL;
	sel = (_tmp0_ = gtk_tree_view_get_selection (self->priv->tree_album), (_tmp0_ == NULL) ? NULL : g_object_ref (_tmp0_));
	model = NULL;
	/*this.model_albums;*/
	_tmp4_ = NULL;
	_tmp3_ = NULL;
	_tmp1_ = NULL;
	if ((_tmp2_ = gtk_tree_selection_get_selected (sel, &_tmp1_, &iter), model = (_tmp3_ = (_tmp4_ = _tmp1_, (_tmp4_ == NULL) ? NULL : g_object_ref (_tmp4_)), (model == NULL) ? NULL : (model = (g_object_unref (model), NULL)), _tmp3_), _tmp2_)) {
		char* album;
		album = NULL;
		gtk_tree_model_get (model, &iter, 6, &album, -1, -1);
		result = album;
		(sel == NULL) ? NULL : (sel = (g_object_unref (sel), NULL));
		(model == NULL) ? NULL : (model = (g_object_unref (model), NULL));
		return result;
	}
	result = NULL;
	(sel == NULL) ? NULL : (sel = (g_object_unref (sel), NULL));
	(model == NULL) ? NULL : (model = (g_object_unref (model), NULL));
	return result;
}


static mpd_Song* gmpc_metadata_browser_browser_get_selected_song (GmpcMetadataBrowser* self) {
	mpd_Song* result;
	GtkTreeIter iter = {0};
	GtkTreeSelection* _tmp0_;
	GtkTreeSelection* sel;
	GtkTreeModel* model;
	GtkTreeModel* _tmp4_;
	GtkTreeModel* _tmp3_;
	gboolean _tmp2_;
	GtkTreeModel* _tmp1_;
	g_return_val_if_fail (self != NULL, NULL);
	_tmp0_ = NULL;
	sel = (_tmp0_ = gtk_tree_view_get_selection (self->priv->tree_songs), (_tmp0_ == NULL) ? NULL : g_object_ref (_tmp0_));
	model = NULL;
	_tmp4_ = NULL;
	_tmp3_ = NULL;
	_tmp1_ = NULL;
	if ((_tmp2_ = gtk_tree_selection_get_selected (sel, &_tmp1_, &iter), model = (_tmp3_ = (_tmp4_ = _tmp1_, (_tmp4_ == NULL) ? NULL : g_object_ref (_tmp4_)), (model == NULL) ? NULL : (model = (g_object_unref (model), NULL)), _tmp3_), _tmp2_)) {
		const mpd_Song* songs;
		const mpd_Song* _tmp5_;
		songs = NULL;
		gtk_tree_model_get ((GtkTreeModel*) self->priv->model_songs, &iter, 0, &songs, -1, -1);
		_tmp5_ = NULL;
		result = (_tmp5_ = songs, (_tmp5_ == NULL) ? NULL : mpd_songDup (_tmp5_));
		(sel == NULL) ? NULL : (sel = (g_object_unref (sel), NULL));
		(model == NULL) ? NULL : (model = (g_object_unref (model), NULL));
		return result;
	}
	result = NULL;
	(sel == NULL) ? NULL : (sel = (g_object_unref (sel), NULL));
	(model == NULL) ? NULL : (model = (g_object_unref (model), NULL));
	return result;
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
		MpdData* list;
		const MpdData* iter;
		MpdData* _tmp8_;
		MpdData* _tmp9_;
		MpdData* _tmp10_;
		/* Fill in the first browser */
		mpd_database_search_field_start (connection, MPD_TAG_ITEM_ALBUM);
		mpd_database_search_add_constraint (connection, MPD_TAG_ITEM_ARTIST, artist);
		data = mpd_database_search_commit (connection);
		data = misc_sort_mpddata_by_album_disc_track (data);
		gmpc_mpddata_model_set_request_artist (self->priv->model_albums, artist);
		list = NULL;
		iter = mpd_data_get_first (data);
		if (iter != NULL) {
			{
				gboolean _tmp0_;
				_tmp0_ = TRUE;
				while (TRUE) {
					mpd_Song* _tmp1_;
					char* _tmp3_;
					const char* _tmp2_;
					char* _tmp5_;
					const char* _tmp4_;
					MpdData* ydata;
					if (!_tmp0_) {
						if (!(iter != NULL)) {
							break;
						}
					}
					_tmp0_ = FALSE;
					list = mpd_new_data_struct_append (list);
					list->type = MPD_DATA_TYPE_SONG;
					_tmp1_ = NULL;
					list->song = (_tmp1_ = mpd_newSong (), (list->song == NULL) ? NULL : (list->song = (mpd_freeSong (list->song), NULL)), _tmp1_);
					_tmp3_ = NULL;
					_tmp2_ = NULL;
					list->song->artist = (_tmp3_ = (_tmp2_ = artist, (_tmp2_ == NULL) ? NULL : g_strdup (_tmp2_)), list->song->artist = (g_free (list->song->artist), NULL), _tmp3_);
					_tmp5_ = NULL;
					_tmp4_ = NULL;
					list->song->album = (_tmp5_ = (_tmp4_ = iter->tag, (_tmp4_ == NULL) ? NULL : g_strdup (_tmp4_)), list->song->album = (g_free (list->song->album), NULL), _tmp5_);
					mpd_database_search_field_start (connection, MPD_TAG_ITEM_DATE);
					mpd_database_search_add_constraint (connection, MPD_TAG_ITEM_ARTIST, artist);
					mpd_database_search_add_constraint (connection, MPD_TAG_ITEM_ALBUM, iter->tag);
					ydata = mpd_database_search_commit (connection);
					if (ydata != NULL) {
						char* _tmp7_;
						const char* _tmp6_;
						_tmp7_ = NULL;
						_tmp6_ = NULL;
						list->song->date = (_tmp7_ = (_tmp6_ = ydata->tag, (_tmp6_ == NULL) ? NULL : g_strdup (_tmp6_)), list->song->date = (g_free (list->song->date), NULL), _tmp7_);
					}
					iter = mpd_data_get_next_real (iter, FALSE);
					(ydata == NULL) ? NULL : (ydata = (mpd_data_free (ydata), NULL));
				}
			}
		}
		list = misc_sort_mpddata_by_album_disc_track (list);
		_tmp8_ = NULL;
		gmpc_mpddata_model_set_mpd_data (self->priv->model_albums, (_tmp8_ = list, list = NULL, _tmp8_));
		mpd_database_search_start (connection, TRUE);
		mpd_database_search_add_constraint (connection, MPD_TAG_ITEM_ARTIST, artist);
		_tmp9_ = NULL;
		data = (_tmp9_ = mpd_database_search_commit (connection), (data == NULL) ? NULL : (data = (mpd_data_free (data), NULL)), _tmp9_);
		data = misc_sort_mpddata_by_album_disc_track (data);
		_tmp10_ = NULL;
		gmpc_mpddata_model_set_mpd_data (self->priv->model_songs, (_tmp10_ = data, data = NULL, _tmp10_));
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
		MpdData* _tmp2_;
		album = gmpc_metadata_browser_browser_get_selected_album (self);
		albumartist = NULL;
		if (album != NULL) {
			MpdData* ydata;
			mpd_database_search_field_start (connection, MPD_TAG_ITEM_ALBUM_ARTIST);
			mpd_database_search_add_constraint (connection, MPD_TAG_ITEM_ALBUM, album);
			ydata = mpd_database_search_commit (connection);
			if (ydata != NULL) {
				if (string_get_length (ydata->tag) > 0) {
					char* _tmp1_;
					const char* _tmp0_;
					_tmp1_ = NULL;
					_tmp0_ = NULL;
					albumartist = (_tmp1_ = (_tmp0_ = ydata->tag, (_tmp0_ == NULL) ? NULL : g_strdup (_tmp0_)), albumartist = (g_free (albumartist), NULL), _tmp1_);
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
		data = misc_sort_mpddata_by_album_disc_track (data);
		_tmp2_ = NULL;
		gmpc_mpddata_model_set_mpd_data (self->priv->model_songs, (_tmp2_ = data, data = NULL, _tmp2_));
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
	const mpd_Song* _tmp0_;
	mpd_Song* song;
	g_return_if_fail (self != NULL);
	g_return_if_fail (button != NULL);
	_tmp0_ = NULL;
	song = (_tmp0_ = (const mpd_Song*) g_object_get_data ((GObject*) button, "song"), (_tmp0_ == NULL) ? NULL : mpd_songDup (_tmp0_));
	if (song != NULL) {
		play_path (song->file);
	}
	(song == NULL) ? NULL : (song = (mpd_freeSong (song), NULL));
}


static void gmpc_metadata_browser_add_song (GmpcMetadataBrowser* self, GtkButton* button) {
	const mpd_Song* _tmp0_;
	mpd_Song* song;
	g_return_if_fail (self != NULL);
	g_return_if_fail (button != NULL);
	_tmp0_ = NULL;
	song = (_tmp0_ = (const mpd_Song*) g_object_get_data ((GObject*) button, "song"), (_tmp0_ == NULL) ? NULL : mpd_songDup (_tmp0_));
	if (song != NULL) {
		mpd_playlist_add (connection, song->file);
		(song == NULL) ? NULL : (song = (mpd_freeSong (song), NULL));
		return;
	}
	(song == NULL) ? NULL : (song = (mpd_freeSong (song), NULL));
}


static void gmpc_metadata_browser_replace_song (GmpcMetadataBrowser* self, GtkButton* button) {
	const mpd_Song* _tmp0_;
	mpd_Song* song;
	g_return_if_fail (self != NULL);
	g_return_if_fail (button != NULL);
	_tmp0_ = NULL;
	song = (_tmp0_ = (const mpd_Song*) g_object_get_data ((GObject*) button, "song"), (_tmp0_ == NULL) ? NULL : mpd_songDup (_tmp0_));
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
					char* _tmp1_;
					const char* _tmp0_;
					_tmp1_ = NULL;
					_tmp0_ = NULL;
					albumartist = (_tmp1_ = (_tmp0_ = ydata->tag, (_tmp0_ == NULL) ? NULL : g_strdup (_tmp0_)), albumartist = (g_free (albumartist), NULL), _tmp1_);
				}
			}
			(ydata == NULL) ? NULL : (ydata = (mpd_data_free (ydata), NULL));
		}
		mpd_database_search_start (connection, TRUE);
		/*server,MPD.Tag.Type.FILENAME);*/
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
			data = misc_sort_mpddata_by_album_disc_track (data);
			while (TRUE) {
				if (!(data != NULL)) {
					break;
				}
				mpd_playlist_queue_add (connection, data->song->file);
				data = mpd_data_get_next (data);
			}
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
			GtkWidget* _tmp0_;
			GtkWidget* child;
			_tmp0_ = NULL;
			child = (_tmp0_ = (GtkWidget*) child_it->data, (_tmp0_ == NULL) ? NULL : g_object_ref (_tmp0_));
			{
				gtk_object_destroy ((GtkObject*) child);
				(child == NULL) ? NULL : (child = (g_object_unref (child), NULL));
			}
		}
	}
	(list == NULL) ? NULL : (list = (g_list_free (list), NULL));
}


/**
      * Add a row to a gtk table
      * <b>$label:</b> $value
      * then increments i 
      */
static void gmpc_metadata_browser_add_entry (GmpcMetadataBrowser* self, GtkTable* table, const char* entry_label, const char* value, GtkWidget* extra, gint* i) {
	gint j;
	gboolean _tmp0_;
	GtkLabel* label;
	char* _tmp1_;
	g_return_if_fail (self != NULL);
	g_return_if_fail (table != NULL);
	g_return_if_fail (entry_label != NULL);
	j = 0;
	_tmp0_ = FALSE;
	if (value == NULL) {
		_tmp0_ = extra == NULL;
	} else {
		_tmp0_ = FALSE;
	}
	if (_tmp0_) {
		return;
	}
	label = g_object_ref_sink ((GtkLabel*) gtk_label_new (""));
	gtk_label_set_selectable (label, TRUE);
	gtk_misc_set_alignment ((GtkMisc*) label, 0.0f, 0.0f);
	_tmp1_ = NULL;
	gtk_label_set_markup (label, _tmp1_ = g_markup_printf_escaped ("<b>%s:</b>", entry_label));
	_tmp1_ = (g_free (_tmp1_), NULL);
	gtk_table_attach (table, (GtkWidget*) label, (guint) j, (guint) (j + 1), (guint) (*i), (guint) ((*i) + 1), GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, (guint) 0, (guint) 0);
	j++;
	if (value != NULL) {
		GtkLabel* pt_label;
		pt_label = g_object_ref_sink ((GtkLabel*) gtk_label_new (value));
		gtk_label_set_selectable (pt_label, TRUE);
		gtk_misc_set_alignment ((GtkMisc*) pt_label, 0.0f, 0.0f);
		gtk_label_set_line_wrap (pt_label, TRUE);
		gtk_table_attach (table, (GtkWidget*) pt_label, (guint) j, (guint) (j + 1), (guint) (*i), (guint) ((*i) + 1), GTK_EXPAND | GTK_FILL, GTK_SHRINK | GTK_FILL, (guint) 0, (guint) 0);
		j++;
		(pt_label == NULL) ? NULL : (pt_label = (g_object_unref (pt_label), NULL));
	}
	if (extra != NULL) {
		gtk_table_attach (table, extra, (guint) j, (guint) (j + 1), (guint) (*i), (guint) ((*i) + 1), GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, (guint) 0, (guint) 0);
		j++;
	}
	(*i)++;
	(label == NULL) ? NULL : (label = (g_object_unref (label), NULL));
}


/* jump buttons */
static void gmpc_metadata_browser_artist_button_clicked (GmpcMetadataBrowser* self, GtkButton* button) {
	const char* _tmp0_;
	char* artist;
	g_return_if_fail (self != NULL);
	g_return_if_fail (button != NULL);
	_tmp0_ = NULL;
	artist = (_tmp0_ = (const char*) g_object_get_data ((GObject*) button, "artist"), (_tmp0_ == NULL) ? NULL : g_strdup (_tmp0_));
	info2_fill_artist_view (artist);
	artist = (g_free (artist), NULL);
}


static void gmpc_metadata_browser_album_button_clicked (GmpcMetadataBrowser* self, GtkButton* button) {
	const char* _tmp0_;
	char* artist;
	const char* _tmp1_;
	char* album;
	g_return_if_fail (self != NULL);
	g_return_if_fail (button != NULL);
	_tmp0_ = NULL;
	artist = (_tmp0_ = (const char*) g_object_get_data ((GObject*) button, "artist"), (_tmp0_ == NULL) ? NULL : g_strdup (_tmp0_));
	_tmp1_ = NULL;
	album = (_tmp1_ = (const char*) g_object_get_data ((GObject*) button, "album"), (_tmp1_ == NULL) ? NULL : g_strdup (_tmp1_));
	info2_fill_album_view (artist, album);
	artist = (g_free (artist), NULL);
	album = (g_free (album), NULL);
}


static void _gmpc_metadata_browser_metadata_find_query_gtk_button_clicked (GtkButton* _sender, gpointer self) {
	gmpc_metadata_browser_metadata_find_query (self, _sender);
}


static void _gmpc_metadata_browser_artist_button_clicked_gtk_button_clicked (GtkButton* _sender, gpointer self) {
	gmpc_metadata_browser_artist_button_clicked (self, _sender);
}


static void _gmpc_metadata_browser_album_button_clicked_gtk_button_clicked (GtkButton* _sender, gpointer self) {
	gmpc_metadata_browser_album_button_clicked (self, _sender);
}


static char* g_time_format (struct tm *self, const char* format) {
	char* result;
	gchar* _tmp0_;
	gint buffer_size;
	gint buffer_length1;
	gchar* buffer;
	const char* _tmp1_;
	g_return_val_if_fail (format != NULL, NULL);
	_tmp0_ = NULL;
	buffer = (_tmp0_ = g_new0 (gchar, 64), buffer_length1 = 64, buffer_size = buffer_length1, _tmp0_);
	strftime (buffer, buffer_length1, format, &(*self));
	_tmp1_ = NULL;
	result = (_tmp1_ = (const char*) buffer, (_tmp1_ == NULL) ? NULL : g_strdup (_tmp1_));
	buffer = (g_free (buffer), NULL);
	return result;
	buffer = (g_free (buffer), NULL);
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


GtkWidget* gmpc_metadata_browser_metadata_box_show_song (GmpcMetadataBrowser* self, const mpd_Song* song, gboolean show_controls) {
	GtkWidget* result;
	GtkVBox* vbox;
	GtkHBox* box;
	GtkLabel* label;
	GtkHBox* hbox;
	GtkAlignment* ali;
	GmpcMetaImage* artist_image;
	GtkTable* info_box;
	gint i;
	GmpcFavoritesButton* fav_button;
	GtkAlignment* _tmp10_;
	g_return_val_if_fail (self != NULL, NULL);
	g_return_val_if_fail (song != NULL, NULL);
	vbox = g_object_ref_sink ((GtkVBox*) gtk_vbox_new (FALSE, 6));
	gtk_container_set_border_width ((GtkContainer*) vbox, (guint) 8);
	box = gmpc_metadata_browser_history_buttons (self);
	label = g_object_ref_sink ((GtkLabel*) gtk_label_new (""));
	gtk_label_set_selectable (label, TRUE);
	if (song->title != NULL) {
		char* _tmp0_;
		_tmp0_ = NULL;
		gtk_label_set_markup (label, _tmp0_ = g_markup_printf_escaped ("<span size='xx-large' weight='bold'>%s</span>", song->title));
		_tmp0_ = (g_free (_tmp0_), NULL);
	} else {
		char* _tmp1_;
		_tmp1_ = NULL;
		gtk_label_set_markup (label, _tmp1_ = g_markup_printf_escaped ("<span size='xx-large' weight='bold'>%s</span>", _ ("Unknown")));
		_tmp1_ = (g_free (_tmp1_), NULL);
	}
	gtk_misc_set_alignment ((GtkMisc*) label, 0.0f, 0.5f);
	gtk_box_pack_start ((GtkBox*) box, (GtkWidget*) label, TRUE, TRUE, (guint) 0);
	gtk_box_pack_start ((GtkBox*) vbox, (GtkWidget*) box, FALSE, FALSE, (guint) 0);
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
	if (song->title != NULL) {
		GtkButton* button;
		GtkImage* _tmp2_;
		/* Button to search for song with same title */
		button = g_object_ref_sink ((GtkButton*) gtk_button_new ());
		_tmp2_ = NULL;
		gtk_container_add ((GtkContainer*) button, (GtkWidget*) (_tmp2_ = g_object_ref_sink ((GtkImage*) gtk_image_new_from_stock ("gtk-find", GTK_ICON_SIZE_MENU))));
		(_tmp2_ == NULL) ? NULL : (_tmp2_ = (g_object_unref (_tmp2_), NULL));
		gtk_button_set_relief (button, GTK_RELIEF_NONE);
		g_object_set_data_full ((GObject*) button, "query", (void*) g_strdup_printf ("title=(%s)", song->title), (GDestroyNotify) g_free);
		g_signal_connect_object (button, "clicked", (GCallback) _gmpc_metadata_browser_metadata_find_query_gtk_button_clicked, self, 0);
		gtk_widget_set_tooltip_text ((GtkWidget*) button, _ ("Search songs with similar title"));
		gmpc_metadata_browser_add_entry (self, info_box, _ ("Title"), song->title, (GtkWidget*) button, &i);
		(button == NULL) ? NULL : (button = (g_object_unref (button), NULL));
	}
	/* Artist label */
	if (song->artist != NULL) {
		GtkButton* button;
		GtkButton* _tmp3_;
		GtkImage* _tmp4_;
		button = NULL;
		_tmp3_ = NULL;
		button = (_tmp3_ = g_object_ref_sink ((GtkButton*) gtk_button_new ()), (button == NULL) ? NULL : (button = (g_object_unref (button), NULL)), _tmp3_);
		_tmp4_ = NULL;
		gtk_container_add ((GtkContainer*) button, (GtkWidget*) (_tmp4_ = g_object_ref_sink ((GtkImage*) gtk_image_new_from_icon_name ("media-artist", GTK_ICON_SIZE_MENU))));
		(_tmp4_ == NULL) ? NULL : (_tmp4_ = (g_object_unref (_tmp4_), NULL));
		gtk_button_set_relief (button, GTK_RELIEF_NONE);
		g_object_set_data_full ((GObject*) button, "artist", (void*) g_strdup_printf ("%s", song->artist), (GDestroyNotify) g_free);
		g_signal_connect_object (button, "clicked", (GCallback) _gmpc_metadata_browser_artist_button_clicked_gtk_button_clicked, self, 0);
		gmpc_metadata_browser_add_entry (self, info_box, _ ("Artist"), song->artist, (GtkWidget*) button, &i);
		(button == NULL) ? NULL : (button = (g_object_unref (button), NULL));
	}
	/* AlbumArtist label */
	gmpc_metadata_browser_add_entry (self, info_box, _ ("Album artist"), song->albumartist, NULL, &i);
	/* Album */
	if (song->album != NULL) {
		GtkButton* button;
		button = NULL;
		if (song->artist != NULL) {
			GtkButton* _tmp5_;
			GtkImage* _tmp6_;
			_tmp5_ = NULL;
			button = (_tmp5_ = g_object_ref_sink ((GtkButton*) gtk_button_new ()), (button == NULL) ? NULL : (button = (g_object_unref (button), NULL)), _tmp5_);
			_tmp6_ = NULL;
			gtk_container_add ((GtkContainer*) button, (GtkWidget*) (_tmp6_ = g_object_ref_sink ((GtkImage*) gtk_image_new_from_icon_name ("media-album", GTK_ICON_SIZE_MENU))));
			(_tmp6_ == NULL) ? NULL : (_tmp6_ = (g_object_unref (_tmp6_), NULL));
			gtk_button_set_relief (button, GTK_RELIEF_NONE);
			g_object_set_data_full ((GObject*) button, "artist", (void*) g_strdup_printf ("%s", song->artist), (GDestroyNotify) g_free);
			g_object_set_data_full ((GObject*) button, "album", (void*) g_strdup_printf ("%s", song->album), (GDestroyNotify) g_free);
			g_signal_connect_object (button, "clicked", (GCallback) _gmpc_metadata_browser_album_button_clicked_gtk_button_clicked, self, 0);
		}
		gmpc_metadata_browser_add_entry (self, info_box, _ ("Album"), song->album, (GtkWidget*) button, &i);
		(button == NULL) ? NULL : (button = (g_object_unref (button), NULL));
	}
	/* track */
	gmpc_metadata_browser_add_entry (self, info_box, _ ("Track"), song->track, NULL, &i);
	/* date */
	gmpc_metadata_browser_add_entry (self, info_box, _ ("Date"), song->date, NULL, &i);
	/* performer */
	gmpc_metadata_browser_add_entry (self, info_box, _ ("Performer"), song->performer, NULL, &i);
	/* disc */
	gmpc_metadata_browser_add_entry (self, info_box, _ ("Disc"), song->disc, NULL, &i);
	/* Genre */
	gmpc_metadata_browser_add_entry (self, info_box, _ ("Genre"), song->genre, NULL, &i);
	if (song->mtime > 0) {
		struct tm _tmp7_ = {0};
		struct tm t;
		struct tm _tmp8_ = {0};
		char* str;
		t = (memset (&_tmp7_, 0, sizeof (struct tm)), _tmp7_);
		fprintf (stdout, "%i\n", (gint) song->mtime);
		t = (localtime_r (&song->mtime, &_tmp8_), _tmp8_);
		fprintf (stdout, "r: %i\n", (gint) mktime (&t));
		str = g_time_format (&t, "%x");
		/* Last modified*/
		gmpc_metadata_browser_add_entry (self, info_box, _ ("Last modified"), str, NULL, &i);
		str = (g_free (str), NULL);
	}
	/* Path */
	if (song->file != NULL) {
		GtkButton* dbutton;
		GtkImage* _tmp9_;
		dbutton = g_object_ref_sink ((GtkButton*) gtk_button_new ());
		gtk_button_set_relief (dbutton, GTK_RELIEF_NONE);
		_tmp9_ = NULL;
		gtk_container_add ((GtkContainer*) dbutton, (GtkWidget*) (_tmp9_ = g_object_ref_sink ((GtkImage*) gtk_image_new_from_stock ("gtk-open", GTK_ICON_SIZE_MENU))));
		(_tmp9_ == NULL) ? NULL : (_tmp9_ = (g_object_unref (_tmp9_), NULL));
		g_object_set_data_full ((GObject*) dbutton, "path", (void*) g_path_get_dirname (song->file), (GDestroyNotify) g_free);
		g_signal_connect_object (dbutton, "clicked", (GCallback) _gmpc_metadata_browser_metadata_button_open_file_browser_path_gtk_button_clicked, self, 0);
		gtk_widget_set_tooltip_text ((GtkWidget*) dbutton, _ ("Open path to song in file browser"));
		gmpc_metadata_browser_add_entry (self, info_box, _ ("Path"), song->file, (GtkWidget*) dbutton, &i);
		(dbutton == NULL) ? NULL : (dbutton = (g_object_unref (dbutton), NULL));
	}
	/* Favored button */
	fav_button = g_object_ref_sink (gmpc_favorites_button_new ());
	gmpc_favorites_button_set_song (fav_button, song);
	_tmp10_ = NULL;
	ali = (_tmp10_ = g_object_ref_sink ((GtkAlignment*) gtk_alignment_new (0.0f, 0.5f, 0.f, 0.f)), (ali == NULL) ? NULL : (ali = (g_object_unref (ali), NULL)), _tmp10_);
	gtk_container_add ((GtkContainer*) ali, (GtkWidget*) fav_button);
	gmpc_metadata_browser_add_entry (self, info_box, _ ("Favored"), NULL, (GtkWidget*) ali, &i);
	if (mpd_sticker_supported (connection)) {
		GmpcRating* rating_button;
		GtkAlignment* _tmp11_;
		/* Favored button */
		rating_button = g_object_ref_sink (gmpc_rating_new (connection, song));
		_tmp11_ = NULL;
		ali = (_tmp11_ = g_object_ref_sink ((GtkAlignment*) gtk_alignment_new (0.0f, 0.5f, 0.f, 0.f)), (ali == NULL) ? NULL : (ali = (g_object_unref (ali), NULL)), _tmp11_);
		gtk_container_add ((GtkContainer*) ali, (GtkWidget*) rating_button);
		gmpc_metadata_browser_add_entry (self, info_box, _ ("Rating"), NULL, (GtkWidget*) ali, &i);
		(rating_button == NULL) ? NULL : (rating_button = (g_object_unref (rating_button), NULL));
	}
	/* Comment */
	gmpc_metadata_browser_add_entry (self, info_box, _ ("Comment"), song->comment, NULL, &i);
	gtk_box_pack_start ((GtkBox*) vbox, (GtkWidget*) hbox, FALSE, FALSE, (guint) 0);
	/* Player controls */
	if (show_controls) {
		GtkButton* button;
		GtkHBox* _tmp12_;
		GtkButton* _tmp13_;
		GtkButton* _tmp14_;
		GtkImage* _tmp15_;
		button = g_object_ref_sink ((GtkButton*) gtk_button_new_from_stock ("gtk-media-play"));
		gtk_button_set_relief (button, GTK_RELIEF_NONE);
		g_object_set_data_full ((GObject*) button, "song", mpd_songDup (song), (GDestroyNotify) mpd_freeSong);
		g_signal_connect_object (button, "clicked", (GCallback) _gmpc_metadata_browser_play_song_gtk_button_clicked, self, 0);
		_tmp12_ = NULL;
		hbox = (_tmp12_ = g_object_ref_sink ((GtkHBox*) gtk_hbox_new (FALSE, 6)), (hbox == NULL) ? NULL : (hbox = (g_object_unref (hbox), NULL)), _tmp12_);
		gtk_box_pack_start ((GtkBox*) hbox, (GtkWidget*) button, FALSE, FALSE, (guint) 0);
		_tmp13_ = NULL;
		button = (_tmp13_ = g_object_ref_sink ((GtkButton*) gtk_button_new_from_stock ("gtk-add")), (button == NULL) ? NULL : (button = (g_object_unref (button), NULL)), _tmp13_);
		gtk_button_set_relief (button, GTK_RELIEF_NONE);
		g_object_set_data_full ((GObject*) button, "song", mpd_songDup (song), (GDestroyNotify) mpd_freeSong);
		g_signal_connect_object (button, "clicked", (GCallback) _gmpc_metadata_browser_add_song_gtk_button_clicked, self, 0);
		gtk_box_pack_start ((GtkBox*) hbox, (GtkWidget*) button, FALSE, FALSE, (guint) 0);
		_tmp14_ = NULL;
		button = (_tmp14_ = g_object_ref_sink ((GtkButton*) gtk_button_new_with_mnemonic (_ ("_Replace"))), (button == NULL) ? NULL : (button = (g_object_unref (button), NULL)), _tmp14_);
		_tmp15_ = NULL;
		gtk_button_set_image (button, (GtkWidget*) (_tmp15_ = g_object_ref_sink ((GtkImage*) gtk_image_new_from_stock ("gtk-redo", GTK_ICON_SIZE_BUTTON))));
		(_tmp15_ == NULL) ? NULL : (_tmp15_ = (g_object_unref (_tmp15_), NULL));
		gtk_button_set_relief (button, GTK_RELIEF_NONE);
		g_object_set_data_full ((GObject*) button, "song", mpd_songDup (song), (GDestroyNotify) mpd_freeSong);
		g_signal_connect_object (button, "clicked", (GCallback) _gmpc_metadata_browser_replace_song_gtk_button_clicked, self, 0);
		gtk_box_pack_start ((GtkBox*) hbox, (GtkWidget*) button, FALSE, FALSE, (guint) 0);
		gtk_table_attach (info_box, (GtkWidget*) hbox, (guint) 0, (guint) 2, (guint) i, (guint) (i + 1), GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, (guint) 0, (guint) 0);
		i++;
		(button == NULL) ? NULL : (button = (g_object_unref (button), NULL));
	}
	/* Lyrics */
	if (cfg_get_single_value_as_int_with_default (config, "MetaData", "show-lyrics", 1) == 1) {
		GmpcMetaTextView* text_view;
		char* _tmp16_;
		GmpcWidgetMore* _tmp17_;
		GmpcWidgetMore* frame;
		text_view = g_object_ref_sink (gmpc_meta_text_view_new (META_SONG_TXT));
		gtk_text_view_set_left_margin ((GtkTextView*) text_view, 8);
		_tmp16_ = NULL;
		_tmp17_ = NULL;
		frame = (_tmp17_ = g_object_ref_sink (gmpc_widget_more_new ("lyrics-view", _tmp16_ = g_markup_printf_escaped ("<b>%s:</b>", _ ("Lyrics")), (GtkWidget*) text_view)), _tmp16_ = (g_free (_tmp16_), NULL), _tmp17_);
		gmpc_meta_text_view_query_text_from_song (text_view, song);
		gtk_box_pack_start ((GtkBox*) vbox, (GtkWidget*) frame, FALSE, FALSE, (guint) 0);
		(text_view == NULL) ? NULL : (text_view = (g_object_unref (text_view), NULL));
		(frame == NULL) ? NULL : (frame = (g_object_unref (frame), NULL));
	}
	/* Guitar Tab */
	if (cfg_get_single_value_as_int_with_default (config, "MetaData", "show-guitar-tabs", 1) == 1) {
		GmpcMetaTextView* text_view;
		char* _tmp18_;
		GmpcWidgetMore* _tmp19_;
		GmpcWidgetMore* frame;
		text_view = g_object_ref_sink (gmpc_meta_text_view_new (META_SONG_GUITAR_TAB));
		text_view->use_monospace = TRUE;
		gtk_text_view_set_left_margin ((GtkTextView*) text_view, 8);
		_tmp18_ = NULL;
		_tmp19_ = NULL;
		frame = (_tmp19_ = g_object_ref_sink (gmpc_widget_more_new ("guitar-tabs-view", _tmp18_ = g_markup_printf_escaped ("<b>%s:</b>", _ ("Guitar Tabs")), (GtkWidget*) text_view)), _tmp18_ = (g_free (_tmp18_), NULL), _tmp19_);
		gmpc_meta_text_view_query_text_from_song (text_view, song);
		gtk_box_pack_start ((GtkBox*) vbox, (GtkWidget*) frame, FALSE, FALSE, (guint) 0);
		(text_view == NULL) ? NULL : (text_view = (g_object_unref (text_view), NULL));
		(frame == NULL) ? NULL : (frame = (g_object_unref (frame), NULL));
	}
	if (cfg_get_single_value_as_int_with_default (config, "MetaData", "show-similar-songs", 1) == 1) {
		GmpcWidgetSimilarSongs* similar_songs;
		similar_songs = g_object_ref_sink (gmpc_widget_similar_songs_new (song));
		gtk_box_pack_start ((GtkBox*) vbox, (GtkWidget*) similar_songs, FALSE, FALSE, (guint) 0);
		(similar_songs == NULL) ? NULL : (similar_songs = (g_object_unref (similar_songs), NULL));
	}
	/* Show web links */
	if (cfg_get_single_value_as_int_with_default (config, "MetaData", "show-web-links", 1) == 1) {
		GmpcSongLinks* song_links;
		song_links = g_object_ref_sink (gmpc_song_links_new (GMPC_SONG_LINKS_TYPE_SONG, song));
		gtk_box_pack_start ((GtkBox*) vbox, (GtkWidget*) song_links, FALSE, FALSE, (guint) 0);
		(song_links == NULL) ? NULL : (song_links = (g_object_unref (song_links), NULL));
	}
	result = (GtkWidget*) vbox;
	(box == NULL) ? NULL : (box = (g_object_unref (box), NULL));
	(label == NULL) ? NULL : (label = (g_object_unref (label), NULL));
	(hbox == NULL) ? NULL : (hbox = (g_object_unref (hbox), NULL));
	(ali == NULL) ? NULL : (ali = (g_object_unref (ali), NULL));
	(artist_image == NULL) ? NULL : (artist_image = (g_object_unref (artist_image), NULL));
	(info_box == NULL) ? NULL : (info_box = (g_object_unref (info_box), NULL));
	(fav_button == NULL) ? NULL : (fav_button = (g_object_unref (fav_button), NULL));
	return result;
}


static void gmpc_metadata_browser_metadata_button_open_file_browser_path (GmpcMetadataBrowser* self, GtkButton* button) {
	const char* _tmp0_;
	char* path;
	g_return_if_fail (self != NULL);
	g_return_if_fail (button != NULL);
	_tmp0_ = NULL;
	path = (_tmp0_ = (const char*) g_object_get_data ((GObject*) button, "path"), (_tmp0_ == NULL) ? NULL : g_strdup (_tmp0_));
	if (path != NULL) {
		pl3_file_browser_open_path (path);
	}
	path = (g_free (path), NULL);
}


static void gmpc_metadata_browser_metadata_find_query (GmpcMetadataBrowser* self, GtkButton* button) {
	const char* _tmp0_;
	char* path;
	g_return_if_fail (self != NULL);
	g_return_if_fail (button != NULL);
	_tmp0_ = NULL;
	path = (_tmp0_ = (const char*) g_object_get_data ((GObject*) button, "query"), (_tmp0_ == NULL) ? NULL : g_strdup (_tmp0_));
	if (path != NULL) {
		pl3_find2_ec_database (NULL, path);
	}
	path = (g_free (path), NULL);
}


static void gmpc_metadata_browser_album_song_tree_row_activated (GmpcMetadataBrowser* self, GtkTreeView* tree, const GtkTreePath* path, GtkTreeViewColumn* column) {
	GtkTreeIter iter = {0};
	GtkTreeModel* _tmp0_;
	GtkTreeModel* model;
	g_return_if_fail (self != NULL);
	g_return_if_fail (tree != NULL);
	g_return_if_fail (path != NULL);
	g_return_if_fail (column != NULL);
	_tmp0_ = NULL;
	model = (_tmp0_ = gtk_tree_view_get_model (tree), (_tmp0_ == NULL) ? NULL : g_object_ref (_tmp0_));
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


static void gmpc_metadata_browser_album_song_browser_play_clicked (GmpcMetadataBrowser* self, GtkImageMenuItem* item) {
	GtkTreeView* _tmp0_;
	GtkTreeView* tree;
	g_return_if_fail (self != NULL);
	g_return_if_fail (item != NULL);
	_tmp0_ = NULL;
	tree = (_tmp0_ = GTK_TREE_VIEW (g_object_get_data ((GObject*) item, "tree")), (_tmp0_ == NULL) ? NULL : g_object_ref (_tmp0_));
	if (tree != NULL) {
		GtkTreeIter iter = {0};
		GtkTreeModel* _tmp1_;
		GtkTreeModel* model;
		GtkTreeSelection* _tmp2_;
		GtkTreeSelection* sel;
		GtkTreeModel* _tmp6_;
		GtkTreeModel* _tmp5_;
		GList* _tmp4_;
		GtkTreeModel* _tmp3_;
		GList* list;
		_tmp1_ = NULL;
		model = (_tmp1_ = gtk_tree_view_get_model (tree), (_tmp1_ == NULL) ? NULL : g_object_ref (_tmp1_));
		_tmp2_ = NULL;
		sel = (_tmp2_ = gtk_tree_view_get_selection (tree), (_tmp2_ == NULL) ? NULL : g_object_ref (_tmp2_));
		_tmp6_ = NULL;
		_tmp5_ = NULL;
		_tmp4_ = NULL;
		_tmp3_ = NULL;
		list = (_tmp4_ = gtk_tree_selection_get_selected_rows (sel, &_tmp3_), model = (_tmp5_ = (_tmp6_ = _tmp3_, (_tmp6_ == NULL) ? NULL : g_object_ref (_tmp6_)), (model == NULL) ? NULL : (model = (g_object_unref (model), NULL)), _tmp5_), _tmp4_);
		{
			GList* path_collection;
			GList* path_it;
			path_collection = list;
			for (path_it = path_collection; path_it != NULL; path_it = path_it->next) {
				const GtkTreePath* _tmp7_;
				GtkTreePath* path;
				_tmp7_ = NULL;
				path = (_tmp7_ = (const GtkTreePath*) path_it->data, (_tmp7_ == NULL) ? NULL : gtk_tree_path_copy (_tmp7_));
				{
					if (gtk_tree_model_get_iter (model, &iter, path)) {
						const mpd_Song* song;
						song = NULL;
						gtk_tree_model_get (model, &iter, 0, &song, -1, -1);
						if (song != NULL) {
							play_path (song->file);
							(path == NULL) ? NULL : (path = (gtk_tree_path_free (path), NULL));
							(model == NULL) ? NULL : (model = (g_object_unref (model), NULL));
							(sel == NULL) ? NULL : (sel = (g_object_unref (sel), NULL));
							(list == NULL) ? NULL : (list = (_g_list_free_gtk_tree_path_free (list), NULL));
							(tree == NULL) ? NULL : (tree = (g_object_unref (tree), NULL));
							return;
						}
					}
					(path == NULL) ? NULL : (path = (gtk_tree_path_free (path), NULL));
				}
			}
		}
		(model == NULL) ? NULL : (model = (g_object_unref (model), NULL));
		(sel == NULL) ? NULL : (sel = (g_object_unref (sel), NULL));
		(list == NULL) ? NULL : (list = (_g_list_free_gtk_tree_path_free (list), NULL));
	}
	(tree == NULL) ? NULL : (tree = (g_object_unref (tree), NULL));
}


static void gmpc_metadata_browser_album_song_browser_add_clicked (GmpcMetadataBrowser* self, GtkImageMenuItem* item) {
	GtkTreeView* _tmp0_;
	GtkTreeView* tree;
	g_return_if_fail (self != NULL);
	g_return_if_fail (item != NULL);
	_tmp0_ = NULL;
	tree = (_tmp0_ = GTK_TREE_VIEW (g_object_get_data ((GObject*) item, "tree")), (_tmp0_ == NULL) ? NULL : g_object_ref (_tmp0_));
	if (tree != NULL) {
		GtkTreeIter iter = {0};
		GtkTreeModel* _tmp1_;
		GtkTreeModel* model;
		GtkTreeSelection* _tmp2_;
		GtkTreeSelection* sel;
		GtkTreeModel* _tmp6_;
		GtkTreeModel* _tmp5_;
		GList* _tmp4_;
		GtkTreeModel* _tmp3_;
		GList* list;
		_tmp1_ = NULL;
		model = (_tmp1_ = gtk_tree_view_get_model (tree), (_tmp1_ == NULL) ? NULL : g_object_ref (_tmp1_));
		_tmp2_ = NULL;
		sel = (_tmp2_ = gtk_tree_view_get_selection (tree), (_tmp2_ == NULL) ? NULL : g_object_ref (_tmp2_));
		_tmp6_ = NULL;
		_tmp5_ = NULL;
		_tmp4_ = NULL;
		_tmp3_ = NULL;
		list = (_tmp4_ = gtk_tree_selection_get_selected_rows (sel, &_tmp3_), model = (_tmp5_ = (_tmp6_ = _tmp3_, (_tmp6_ == NULL) ? NULL : g_object_ref (_tmp6_)), (model == NULL) ? NULL : (model = (g_object_unref (model), NULL)), _tmp5_), _tmp4_);
		{
			GList* path_collection;
			GList* path_it;
			path_collection = list;
			for (path_it = path_collection; path_it != NULL; path_it = path_it->next) {
				const GtkTreePath* _tmp7_;
				GtkTreePath* path;
				_tmp7_ = NULL;
				path = (_tmp7_ = (const GtkTreePath*) path_it->data, (_tmp7_ == NULL) ? NULL : gtk_tree_path_copy (_tmp7_));
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
		(model == NULL) ? NULL : (model = (g_object_unref (model), NULL));
		(sel == NULL) ? NULL : (sel = (g_object_unref (sel), NULL));
		(list == NULL) ? NULL : (list = (_g_list_free_gtk_tree_path_free (list), NULL));
	}
	(tree == NULL) ? NULL : (tree = (g_object_unref (tree), NULL));
}


static void gmpc_metadata_browser_album_song_browser_replace_clicked (GmpcMetadataBrowser* self, GtkImageMenuItem* item) {
	g_return_if_fail (self != NULL);
	g_return_if_fail (item != NULL);
	mpd_playlist_clear (connection);
	gmpc_metadata_browser_album_song_browser_add_clicked (self, item);
	mpd_player_play (connection);
}


static void _gmpc_metadata_browser_album_song_browser_play_clicked_gtk_menu_item_activate (GtkImageMenuItem* _sender, gpointer self) {
	gmpc_metadata_browser_album_song_browser_play_clicked (self, _sender);
}


static void _gmpc_metadata_browser_album_song_browser_add_clicked_gtk_menu_item_activate (GtkImageMenuItem* _sender, gpointer self) {
	gmpc_metadata_browser_album_song_browser_add_clicked (self, _sender);
}


static void _gmpc_metadata_browser_album_song_browser_replace_clicked_gtk_menu_item_activate (GtkImageMenuItem* _sender, gpointer self) {
	gmpc_metadata_browser_album_song_browser_replace_clicked (self, _sender);
}


static gboolean gmpc_metadata_browser_album_song_tree_button_press_event (GmpcMetadataBrowser* self, GmpcMpdDataTreeview* tree, const GdkEventButton* event) {
	gboolean result;
	g_return_val_if_fail (self != NULL, FALSE);
	g_return_val_if_fail (tree != NULL, FALSE);
	if ((*event).button == 3) {
		GtkMenu* menu;
		GtkImageMenuItem* item;
		GtkImageMenuItem* _tmp0_;
		GtkImage* _tmp1_;
		menu = g_object_ref_sink ((GtkMenu*) gtk_menu_new ());
		if (gtk_tree_selection_count_selected_rows (gtk_tree_view_get_selection ((GtkTreeView*) tree)) == 1) {
			GtkImageMenuItem* item;
			item = g_object_ref_sink ((GtkImageMenuItem*) gtk_image_menu_item_new_from_stock ("gtk-media-play", NULL));
			g_signal_connect_object ((GtkMenuItem*) item, "activate", (GCallback) _gmpc_metadata_browser_album_song_browser_play_clicked_gtk_menu_item_activate, self, 0);
			g_object_set_data ((GObject*) item, "tree", (void*) tree);
			gtk_menu_shell_append ((GtkMenuShell*) menu, (GtkWidget*) ((GtkMenuItem*) item));
			(item == NULL) ? NULL : (item = (g_object_unref (item), NULL));
		}
		item = g_object_ref_sink ((GtkImageMenuItem*) gtk_image_menu_item_new_from_stock ("gtk-add", NULL));
		g_signal_connect_object ((GtkMenuItem*) item, "activate", (GCallback) _gmpc_metadata_browser_album_song_browser_add_clicked_gtk_menu_item_activate, self, 0);
		g_object_set_data ((GObject*) item, "tree", (void*) tree);
		gtk_menu_shell_append ((GtkMenuShell*) menu, (GtkWidget*) ((GtkMenuItem*) item));
		_tmp0_ = NULL;
		item = (_tmp0_ = g_object_ref_sink ((GtkImageMenuItem*) gtk_image_menu_item_new_with_mnemonic (_ ("_Replace"))), (item == NULL) ? NULL : (item = (g_object_unref (item), NULL)), _tmp0_);
		_tmp1_ = NULL;
		gtk_image_menu_item_set_image (item, (GtkWidget*) (_tmp1_ = g_object_ref_sink ((GtkImage*) gtk_image_new_from_stock ("gtk-redo", GTK_ICON_SIZE_MENU))));
		(_tmp1_ == NULL) ? NULL : (_tmp1_ = (g_object_unref (_tmp1_), NULL));
		g_object_set_data ((GObject*) item, "tree", (void*) tree);
		g_signal_connect_object ((GtkMenuItem*) item, "activate", (GCallback) _gmpc_metadata_browser_album_song_browser_replace_clicked_gtk_menu_item_activate, self, 0);
		gtk_menu_shell_append ((GtkMenuShell*) menu, (GtkWidget*) ((GtkMenuItem*) item));
		if (gtk_tree_selection_count_selected_rows (gtk_tree_view_get_selection ((GtkTreeView*) tree)) == 1) {
			GtkTreeModel* model;
			GtkTreeModel* _tmp5_;
			GtkTreeModel* _tmp4_;
			GList* _tmp3_;
			GtkTreeModel* _tmp2_;
			GList* list;
			model = NULL;
			_tmp5_ = NULL;
			_tmp4_ = NULL;
			_tmp3_ = NULL;
			_tmp2_ = NULL;
			list = (_tmp3_ = gtk_tree_selection_get_selected_rows (gtk_tree_view_get_selection ((GtkTreeView*) tree), &_tmp2_), model = (_tmp4_ = (_tmp5_ = _tmp2_, (_tmp5_ == NULL) ? NULL : g_object_ref (_tmp5_)), (model == NULL) ? NULL : (model = (g_object_unref (model), NULL)), _tmp4_), _tmp3_);
			if (list != NULL) {
				const GtkTreePath* path;
				GtkTreeIter iter = {0};
				const mpd_Song* song;
				path = (const GtkTreePath*) list->data;
				song = NULL;
				if (gtk_tree_model_get_iter (model, &iter, path)) {
					gtk_tree_model_get (model, &iter, 0, &song, -1);
					submenu_for_song ((GtkWidget*) menu, song);
				}
			}
			(model == NULL) ? NULL : (model = (g_object_unref (model), NULL));
			(list == NULL) ? NULL : (list = (_g_list_free_gtk_tree_path_free (list), NULL));
		}
		gmpc_mpddata_treeview_right_mouse_intergration (tree, menu);
		gtk_menu_popup (menu, NULL, NULL, NULL, NULL, (*event).button, (*event).time);
		gtk_widget_show_all ((GtkWidget*) menu);
		result = TRUE;
		(menu == NULL) ? NULL : (menu = (g_object_unref (menu), NULL));
		(item == NULL) ? NULL : (item = (g_object_unref (item), NULL));
		return result;
	}
	result = FALSE;
	return result;
}


static void _gmpc_metadata_browser_add_selected_song_gtk_button_clicked (GtkButton* _sender, gpointer self) {
	gmpc_metadata_browser_add_selected_song (self, _sender);
}


static void _gmpc_metadata_browser_replace_selected_song_gtk_button_clicked (GtkButton* _sender, gpointer self) {
	gmpc_metadata_browser_replace_selected_song (self, _sender);
}


static gboolean _gmpc_metadata_browser_album_song_tree_button_press_event_gtk_widget_button_release_event (GmpcMpdDataTreeview* _sender, const GdkEventButton* event, gpointer self) {
	return gmpc_metadata_browser_album_song_tree_button_press_event (self, _sender, event);
}


static void _gmpc_metadata_browser_album_song_tree_row_activated_gtk_tree_view_row_activated (GmpcMpdDataTreeview* _sender, const GtkTreePath* path, GtkTreeViewColumn* column, gpointer self) {
	gmpc_metadata_browser_album_song_tree_row_activated (self, _sender, path, column);
}


static void gmpc_metadata_browser_metadata_box_show_album (GmpcMetadataBrowser* self, const char* artist, const char* album) {
	GtkVBox* vbox;
	GtkHBox* box;
	GtkLabel* label;
	const char* _tmp0_;
	const char* _tmp1_;
	char* _tmp2_;
	GtkHBox* hbox;
	GtkAlignment* ali;
	GmpcMetaImage* artist_image;
	mpd_Song* song;
	char* _tmp4_;
	const char* _tmp3_;
	char* _tmp6_;
	const char* _tmp5_;
	GtkTable* info_box;
	gint i;
	GmpcStatsLabel* pt_label;
	GmpcStatsLabel* _tmp9_;
	GmpcStatsLabel* _tmp10_;
	GmpcStatsLabel* _tmp11_;
	GtkHBox* _tmp12_;
	GtkButton* button;
	GtkButton* _tmp13_;
	GtkImage* _tmp14_;
	GtkLabel* _tmp17_;
	char* _tmp18_;
	GtkScrolledWindow* sw;
	GmpcMpdDataTreeview* song_tree;
	g_return_if_fail (self != NULL);
	g_return_if_fail (artist != NULL);
	g_return_if_fail (album != NULL);
	vbox = g_object_ref_sink ((GtkVBox*) gtk_vbox_new (FALSE, 6));
	gtk_container_set_border_width ((GtkContainer*) vbox, (guint) 8);
	box = gmpc_metadata_browser_history_buttons (self);
	label = g_object_ref_sink ((GtkLabel*) gtk_label_new (""));
	gtk_label_set_selectable (label, TRUE);
	_tmp0_ = NULL;
	if (artist != NULL) {
		_tmp0_ = artist;
	} else {
		_tmp0_ = _ ("Unknown");
	}
	_tmp1_ = NULL;
	if (album != NULL) {
		_tmp1_ = album;
	} else {
		_tmp1_ = _ ("Unknown");
	}
	_tmp2_ = NULL;
	gtk_label_set_markup (label, _tmp2_ = g_markup_printf_escaped ("<span size='xx-large' weight='bold'>%s - %s</span>", _tmp0_, _tmp1_));
	_tmp2_ = (g_free (_tmp2_), NULL);
	gtk_misc_set_alignment ((GtkMisc*) label, 0.0f, 0.5f);
	gtk_box_pack_start ((GtkBox*) box, (GtkWidget*) label, TRUE, TRUE, (guint) 0);
	gtk_box_pack_start ((GtkBox*) vbox, (GtkWidget*) box, FALSE, FALSE, (guint) 0);
	/* Artist image */
	hbox = g_object_ref_sink ((GtkHBox*) gtk_hbox_new (FALSE, 6));
	ali = g_object_ref_sink ((GtkAlignment*) gtk_alignment_new (0.f, 0.f, 0.f, 0.f));
	artist_image = g_object_ref_sink (gmpc_metaimage_new_size (META_ALBUM_ART, 250));
	gmpc_metaimage_set_squared (artist_image, FALSE);
	song = mpd_newSong ();
	_tmp4_ = NULL;
	_tmp3_ = NULL;
	song->artist = (_tmp4_ = (_tmp3_ = artist, (_tmp3_ == NULL) ? NULL : g_strdup (_tmp3_)), song->artist = (g_free (song->artist), NULL), _tmp4_);
	_tmp6_ = NULL;
	_tmp5_ = NULL;
	song->album = (_tmp6_ = (_tmp5_ = album, (_tmp5_ == NULL) ? NULL : g_strdup (_tmp5_)), song->album = (g_free (song->album), NULL), _tmp6_);
	gmpc_metaimage_update_cover_from_song (artist_image, song);
	gtk_container_add ((GtkContainer*) ali, (GtkWidget*) artist_image);
	gtk_box_pack_start ((GtkBox*) hbox, (GtkWidget*) ali, FALSE, FALSE, (guint) 0);
	/* Artist information */
	info_box = g_object_ref_sink ((GtkTable*) gtk_table_new ((guint) 4, (guint) 2, FALSE));
	gtk_table_set_row_spacings (info_box, (guint) 3);
	gtk_table_set_col_spacings (info_box, (guint) 8);
	gtk_box_pack_start ((GtkBox*) hbox, (GtkWidget*) info_box, FALSE, FALSE, (guint) 0);
	i = 0;
	/* Artist label */
	if (song->artist != NULL) {
		GtkButton* button;
		GtkButton* _tmp7_;
		GtkImage* _tmp8_;
		button = NULL;
		_tmp7_ = NULL;
		button = (_tmp7_ = g_object_ref_sink ((GtkButton*) gtk_button_new ()), (button == NULL) ? NULL : (button = (g_object_unref (button), NULL)), _tmp7_);
		_tmp8_ = NULL;
		gtk_container_add ((GtkContainer*) button, (GtkWidget*) (_tmp8_ = g_object_ref_sink ((GtkImage*) gtk_image_new_from_icon_name ("media-artist", GTK_ICON_SIZE_MENU))));
		(_tmp8_ == NULL) ? NULL : (_tmp8_ = (g_object_unref (_tmp8_), NULL));
		gtk_button_set_relief (button, GTK_RELIEF_NONE);
		g_object_set_data_full ((GObject*) button, "artist", (void*) g_strdup_printf ("%s", song->artist), (GDestroyNotify) g_free);
		g_signal_connect_object (button, "clicked", (GCallback) _gmpc_metadata_browser_artist_button_clicked_gtk_button_clicked, self, 0);
		gmpc_metadata_browser_add_entry (self, info_box, _ ("Artist"), song->artist, (GtkWidget*) button, &i);
		(button == NULL) ? NULL : (button = (g_object_unref (button), NULL));
	}
	/* Genres of songs */
	pt_label = g_object_ref_sink (gmpc_stats_label_new (ALBUM_GENRES_SONGS, song));
	gtk_misc_set_alignment ((GtkMisc*) pt_label, 0.0f, 0.5f);
	gtk_label_set_line_wrap ((GtkLabel*) pt_label, TRUE);
	gmpc_metadata_browser_add_entry (self, info_box, _ ("Genres"), NULL, (GtkWidget*) pt_label, &i);
	/* Dates of songs */
	_tmp9_ = NULL;
	pt_label = (_tmp9_ = g_object_ref_sink (gmpc_stats_label_new (ALBUM_DATES_SONGS, song)), (pt_label == NULL) ? NULL : (pt_label = (g_object_unref (pt_label), NULL)), _tmp9_);
	gtk_label_set_line_wrap ((GtkLabel*) pt_label, TRUE);
	gtk_misc_set_alignment ((GtkMisc*) pt_label, 0.0f, 0.5f);
	gmpc_metadata_browser_add_entry (self, info_box, _ ("Dates"), NULL, (GtkWidget*) pt_label, &i);
	/* Total number of songs */
	_tmp10_ = NULL;
	pt_label = (_tmp10_ = g_object_ref_sink (gmpc_stats_label_new (ALBUM_NUM_SONGS, song)), (pt_label == NULL) ? NULL : (pt_label = (g_object_unref (pt_label), NULL)), _tmp10_);
	gtk_label_set_line_wrap ((GtkLabel*) pt_label, TRUE);
	gtk_misc_set_alignment ((GtkMisc*) pt_label, 0.0f, 0.5f);
	gmpc_metadata_browser_add_entry (self, info_box, _ ("Songs"), NULL, (GtkWidget*) pt_label, &i);
	/* Total playtime */
	_tmp11_ = NULL;
	pt_label = (_tmp11_ = g_object_ref_sink (gmpc_stats_label_new (ALBUM_PLAYTIME_SONGS, song)), (pt_label == NULL) ? NULL : (pt_label = (g_object_unref (pt_label), NULL)), _tmp11_);
	gtk_label_set_line_wrap ((GtkLabel*) pt_label, TRUE);
	gtk_misc_set_alignment ((GtkMisc*) pt_label, 0.0f, 0.5f);
	gmpc_metadata_browser_add_entry (self, info_box, _ ("Playtime"), NULL, (GtkWidget*) pt_label, &i);
	gtk_box_pack_start ((GtkBox*) vbox, (GtkWidget*) hbox, FALSE, FALSE, (guint) 0);
	/* Player controls */
	_tmp12_ = NULL;
	hbox = (_tmp12_ = g_object_ref_sink ((GtkHBox*) gtk_hbox_new (FALSE, 6)), (hbox == NULL) ? NULL : (hbox = (g_object_unref (hbox), NULL)), _tmp12_);
	button = g_object_ref_sink ((GtkButton*) gtk_button_new_from_stock ("gtk-add"));
	gtk_button_set_relief (button, GTK_RELIEF_NONE);
	g_signal_connect_object (button, "clicked", (GCallback) _gmpc_metadata_browser_add_selected_song_gtk_button_clicked, self, 0);
	gtk_box_pack_start ((GtkBox*) hbox, (GtkWidget*) button, FALSE, FALSE, (guint) 0);
	_tmp13_ = NULL;
	button = (_tmp13_ = g_object_ref_sink ((GtkButton*) gtk_button_new_with_mnemonic (_ ("_Replace"))), (button == NULL) ? NULL : (button = (g_object_unref (button), NULL)), _tmp13_);
	_tmp14_ = NULL;
	gtk_button_set_image (button, (GtkWidget*) (_tmp14_ = g_object_ref_sink ((GtkImage*) gtk_image_new_from_stock ("gtk-redo", GTK_ICON_SIZE_BUTTON))));
	(_tmp14_ == NULL) ? NULL : (_tmp14_ = (g_object_unref (_tmp14_), NULL));
	gtk_button_set_relief (button, GTK_RELIEF_NONE);
	g_signal_connect_object (button, "clicked", (GCallback) _gmpc_metadata_browser_replace_selected_song_gtk_button_clicked, self, 0);
	gtk_box_pack_start ((GtkBox*) hbox, (GtkWidget*) button, FALSE, FALSE, (guint) 0);
	gtk_table_attach (info_box, (GtkWidget*) hbox, (guint) 0, (guint) 2, (guint) i, (guint) (i + 1), GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, (guint) 0, (guint) 0);
	i++;
	/* Album information */
	if (cfg_get_single_value_as_int_with_default (config, "MetaData", "show-album-information", 1) == 1) {
		GmpcMetaTextView* text_view;
		char* _tmp15_;
		GmpcWidgetMore* _tmp16_;
		GmpcWidgetMore* frame;
		text_view = g_object_ref_sink (gmpc_meta_text_view_new (META_ALBUM_TXT));
		gtk_text_view_set_left_margin ((GtkTextView*) text_view, 8);
		_tmp15_ = NULL;
		_tmp16_ = NULL;
		frame = (_tmp16_ = g_object_ref_sink (gmpc_widget_more_new ("album-information-view", _tmp15_ = g_markup_printf_escaped ("<b>%s:</b>", _ ("Album information")), (GtkWidget*) text_view)), _tmp15_ = (g_free (_tmp15_), NULL), _tmp16_);
		gmpc_meta_text_view_query_text_from_song (text_view, song);
		gtk_box_pack_start ((GtkBox*) vbox, (GtkWidget*) frame, FALSE, FALSE, (guint) 0);
		(text_view == NULL) ? NULL : (text_view = (g_object_unref (text_view), NULL));
		(frame == NULL) ? NULL : (frame = (g_object_unref (frame), NULL));
	}
	/* Song list. Show songs in album  */
	_tmp17_ = NULL;
	label = (_tmp17_ = g_object_ref_sink ((GtkLabel*) gtk_label_new ("")), (label == NULL) ? NULL : (label = (g_object_unref (label), NULL)), _tmp17_);
	gtk_label_set_selectable (label, TRUE);
	_tmp18_ = NULL;
	gtk_label_set_markup (label, _tmp18_ = g_strdup_printf ("<b>%s</b>", _ ("Songs")));
	_tmp18_ = (g_free (_tmp18_), NULL);
	gtk_misc_set_alignment ((GtkMisc*) label, 0.0f, 0.5f);
	gtk_box_pack_start ((GtkBox*) vbox, (GtkWidget*) label, FALSE, FALSE, (guint) 0);
	sw = g_object_ref_sink ((GtkScrolledWindow*) gtk_scrolled_window_new (NULL, NULL));
	gtk_scrolled_window_set_policy (sw, GTK_POLICY_AUTOMATIC, GTK_POLICY_NEVER);
	gtk_scrolled_window_set_shadow_type (sw, GTK_SHADOW_ETCHED_IN);
	song_tree = g_object_ref_sink (gmpc_mpddata_treeview_new ("album-songs", TRUE, (GtkTreeModel*) self->priv->model_songs));
	gmpc_mpddata_treeview_enable_click_fix (song_tree);
	g_signal_connect_object ((GtkWidget*) song_tree, "button-release-event", (GCallback) _gmpc_metadata_browser_album_song_tree_button_press_event_gtk_widget_button_release_event, self, 0);
	g_signal_connect_object ((GtkTreeView*) song_tree, "row-activated", (GCallback) _gmpc_metadata_browser_album_song_tree_row_activated_gtk_tree_view_row_activated, self, 0);
	gtk_container_add ((GtkContainer*) sw, (GtkWidget*) song_tree);
	gtk_box_pack_start ((GtkBox*) vbox, (GtkWidget*) sw, FALSE, FALSE, (guint) 0);
	/* Show web links */
	if (cfg_get_single_value_as_int_with_default (config, "MetaData", "show-web-links", 1) == 1) {
		GmpcSongLinks* song_links;
		song_links = g_object_ref_sink (gmpc_song_links_new (GMPC_SONG_LINKS_TYPE_ALBUM, song));
		gtk_box_pack_start ((GtkBox*) vbox, (GtkWidget*) song_links, FALSE, FALSE, (guint) 0);
		(song_links == NULL) ? NULL : (song_links = (g_object_unref (song_links), NULL));
	}
	/**
	         * Add it to the view
	         */
	gtk_container_add ((GtkContainer*) self->priv->metadata_box, (GtkWidget*) vbox);
	gtk_widget_show_all ((GtkWidget*) self->priv->metadata_sw);
	(vbox == NULL) ? NULL : (vbox = (g_object_unref (vbox), NULL));
	(box == NULL) ? NULL : (box = (g_object_unref (box), NULL));
	(label == NULL) ? NULL : (label = (g_object_unref (label), NULL));
	(hbox == NULL) ? NULL : (hbox = (g_object_unref (hbox), NULL));
	(ali == NULL) ? NULL : (ali = (g_object_unref (ali), NULL));
	(artist_image == NULL) ? NULL : (artist_image = (g_object_unref (artist_image), NULL));
	(song == NULL) ? NULL : (song = (mpd_freeSong (song), NULL));
	(info_box == NULL) ? NULL : (info_box = (g_object_unref (info_box), NULL));
	(pt_label == NULL) ? NULL : (pt_label = (g_object_unref (pt_label), NULL));
	(button == NULL) ? NULL : (button = (g_object_unref (button), NULL));
	(sw == NULL) ? NULL : (sw = (g_object_unref (sw), NULL));
	(song_tree == NULL) ? NULL : (song_tree = (g_object_unref (song_tree), NULL));
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
	GtkHBox* box;
	GtkLabel* label;
	const char* _tmp0_;
	char* _tmp1_;
	mpd_Song* song;
	char* _tmp3_;
	const char* _tmp2_;
	GtkHBox* hbox;
	GtkAlignment* ali;
	GmpcMetaImage* artist_image;
	GtkTable* info_box;
	gint i;
	GmpcStatsLabel* pt_label;
	GmpcStatsLabel* _tmp4_;
	GmpcStatsLabel* _tmp5_;
	GmpcStatsLabel* _tmp6_;
	GtkHBox* _tmp7_;
	GtkButton* button;
	GtkButton* _tmp8_;
	GtkImage* _tmp9_;
	g_return_if_fail (self != NULL);
	g_return_if_fail (artist != NULL);
	vbox = g_object_ref_sink ((GtkVBox*) gtk_vbox_new (FALSE, 6));
	gtk_container_set_border_width ((GtkContainer*) vbox, (guint) 8);
	box = gmpc_metadata_browser_history_buttons (self);
	label = g_object_ref_sink ((GtkLabel*) gtk_label_new (""));
	gtk_label_set_selectable (label, TRUE);
	_tmp0_ = NULL;
	if (artist != NULL) {
		_tmp0_ = artist;
	} else {
		_tmp0_ = _ ("Unknown");
	}
	_tmp1_ = NULL;
	gtk_label_set_markup (label, _tmp1_ = g_markup_printf_escaped ("<span size='xx-large' weight='bold'>%s</span>", _tmp0_));
	_tmp1_ = (g_free (_tmp1_), NULL);
	gtk_misc_set_alignment ((GtkMisc*) label, 0.0f, 0.5f);
	gtk_box_pack_start ((GtkBox*) box, (GtkWidget*) label, TRUE, TRUE, (guint) 0);
	gtk_box_pack_start ((GtkBox*) vbox, (GtkWidget*) box, FALSE, FALSE, (guint) 0);
	/* Create an MPD.Song with the info for this type set */
	song = mpd_newSong ();
	_tmp3_ = NULL;
	_tmp2_ = NULL;
	song->artist = (_tmp3_ = (_tmp2_ = artist, (_tmp2_ == NULL) ? NULL : g_strdup (_tmp2_)), song->artist = (g_free (song->artist), NULL), _tmp3_);
	/* Artist image */
	hbox = g_object_ref_sink ((GtkHBox*) gtk_hbox_new (FALSE, 6));
	ali = g_object_ref_sink ((GtkAlignment*) gtk_alignment_new (0.f, 0.f, 0.f, 0.f));
	artist_image = g_object_ref_sink (gmpc_metaimage_new_size (META_ARTIST_ART, 250));
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
	/* Genres of songs */
	pt_label = g_object_ref_sink (gmpc_stats_label_new (ARTIST_GENRES_SONGS, song));
	gtk_misc_set_alignment ((GtkMisc*) pt_label, 0.0f, 0.5f);
	gtk_label_set_line_wrap ((GtkLabel*) pt_label, TRUE);
	gmpc_metadata_browser_add_entry (self, info_box, _ ("Genres"), NULL, (GtkWidget*) pt_label, &i);
	/* Dates of songs */
	_tmp4_ = NULL;
	pt_label = (_tmp4_ = g_object_ref_sink (gmpc_stats_label_new (ARTIST_DATES_SONGS, song)), (pt_label == NULL) ? NULL : (pt_label = (g_object_unref (pt_label), NULL)), _tmp4_);
	gtk_label_set_line_wrap ((GtkLabel*) pt_label, TRUE);
	gtk_misc_set_alignment ((GtkMisc*) pt_label, 0.0f, 0.5f);
	gmpc_metadata_browser_add_entry (self, info_box, _ ("Dates"), NULL, (GtkWidget*) pt_label, &i);
	/* Total number of songs */
	_tmp5_ = NULL;
	pt_label = (_tmp5_ = g_object_ref_sink (gmpc_stats_label_new (ARTIST_NUM_SONGS, song)), (pt_label == NULL) ? NULL : (pt_label = (g_object_unref (pt_label), NULL)), _tmp5_);
	gtk_label_set_line_wrap ((GtkLabel*) pt_label, TRUE);
	gtk_misc_set_alignment ((GtkMisc*) pt_label, 0.0f, 0.5f);
	gmpc_metadata_browser_add_entry (self, info_box, _ ("Songs"), NULL, (GtkWidget*) pt_label, &i);
	/* Total playtime */
	_tmp6_ = NULL;
	pt_label = (_tmp6_ = g_object_ref_sink (gmpc_stats_label_new (ARTIST_PLAYTIME_SONGS, song)), (pt_label == NULL) ? NULL : (pt_label = (g_object_unref (pt_label), NULL)), _tmp6_);
	gtk_label_set_line_wrap ((GtkLabel*) pt_label, TRUE);
	gtk_misc_set_alignment ((GtkMisc*) pt_label, 0.0f, 0.5f);
	gmpc_metadata_browser_add_entry (self, info_box, _ ("Playtime"), NULL, (GtkWidget*) pt_label, &i);
	gtk_box_pack_start ((GtkBox*) vbox, (GtkWidget*) hbox, FALSE, FALSE, (guint) 0);
	/* Player controls */
	_tmp7_ = NULL;
	hbox = (_tmp7_ = g_object_ref_sink ((GtkHBox*) gtk_hbox_new (FALSE, 6)), (hbox == NULL) ? NULL : (hbox = (g_object_unref (hbox), NULL)), _tmp7_);
	button = g_object_ref_sink ((GtkButton*) gtk_button_new_from_stock ("gtk-add"));
	gtk_button_set_relief (button, GTK_RELIEF_NONE);
	g_signal_connect_object (button, "clicked", (GCallback) _gmpc_metadata_browser_add_selected_song_gtk_button_clicked, self, 0);
	gtk_box_pack_start ((GtkBox*) hbox, (GtkWidget*) button, FALSE, FALSE, (guint) 0);
	_tmp8_ = NULL;
	button = (_tmp8_ = g_object_ref_sink ((GtkButton*) gtk_button_new_with_mnemonic (_ ("_Replace"))), (button == NULL) ? NULL : (button = (g_object_unref (button), NULL)), _tmp8_);
	_tmp9_ = NULL;
	gtk_button_set_image (button, (GtkWidget*) (_tmp9_ = g_object_ref_sink ((GtkImage*) gtk_image_new_from_stock ("gtk-redo", GTK_ICON_SIZE_BUTTON))));
	(_tmp9_ == NULL) ? NULL : (_tmp9_ = (g_object_unref (_tmp9_), NULL));
	gtk_button_set_relief (button, GTK_RELIEF_NONE);
	g_signal_connect_object (button, "clicked", (GCallback) _gmpc_metadata_browser_replace_selected_song_gtk_button_clicked, self, 0);
	gtk_box_pack_start ((GtkBox*) hbox, (GtkWidget*) button, FALSE, FALSE, (guint) 0);
	gtk_table_attach (info_box, (GtkWidget*) hbox, (guint) 0, (guint) 2, (guint) i, (guint) (i + 1), GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, (guint) 0, (guint) 0);
	i++;
	/* Artist information */
	if (cfg_get_single_value_as_int_with_default (config, "MetaData", "show-artist-information", 1) == 1) {
		GmpcMetaTextView* text_view;
		char* _tmp10_;
		GmpcWidgetMore* _tmp11_;
		GmpcWidgetMore* frame;
		text_view = g_object_ref_sink (gmpc_meta_text_view_new (META_ARTIST_TXT));
		gtk_text_view_set_left_margin ((GtkTextView*) text_view, 8);
		_tmp10_ = NULL;
		_tmp11_ = NULL;
		frame = (_tmp11_ = g_object_ref_sink (gmpc_widget_more_new ("artist-information-view", _tmp10_ = g_markup_printf_escaped ("<b>%s:</b>", _ ("Artist information")), (GtkWidget*) text_view)), _tmp10_ = (g_free (_tmp10_), NULL), _tmp11_);
		gmpc_meta_text_view_query_text_from_song (text_view, song);
		gtk_box_pack_start ((GtkBox*) vbox, (GtkWidget*) frame, FALSE, FALSE, (guint) 0);
		(text_view == NULL) ? NULL : (text_view = (g_object_unref (text_view), NULL));
		(frame == NULL) ? NULL : (frame = (g_object_unref (frame), NULL));
	}
	/* Show similar artist */
	if (cfg_get_single_value_as_int_with_default (config, "MetaData", "show-similar-artist", 1) == 1) {
		GtkLabel* _tmp12_;
		char* _tmp13_;
		GtkAlignment* _tmp14_;
		GmpcWidgetSimilarArtist* similar_artist;
		_tmp12_ = NULL;
		label = (_tmp12_ = g_object_ref_sink ((GtkLabel*) gtk_label_new (_ ("Similar artist"))), (label == NULL) ? NULL : (label = (g_object_unref (label), NULL)), _tmp12_);
		gtk_label_set_selectable (label, TRUE);
		_tmp13_ = NULL;
		gtk_label_set_markup (label, _tmp13_ = g_strdup_printf ("<span weight='bold'>%s</span>", _ ("Similar artist")));
		_tmp13_ = (g_free (_tmp13_), NULL);
		gtk_misc_set_alignment ((GtkMisc*) label, 0.0f, 0.0f);
		gtk_box_pack_start ((GtkBox*) vbox, (GtkWidget*) label, FALSE, FALSE, (guint) 0);
		_tmp14_ = NULL;
		ali = (_tmp14_ = g_object_ref_sink ((GtkAlignment*) gtk_alignment_new (0.0f, 0.0f, 0.0f, 0.0f)), (ali == NULL) ? NULL : (ali = (g_object_unref (ali), NULL)), _tmp14_);
		similar_artist = g_object_ref_sink (gmpc_widget_similar_artist_new (self, connection, song));
		gtk_container_add ((GtkContainer*) ali, (GtkWidget*) similar_artist);
		gtk_box_pack_start ((GtkBox*) vbox, (GtkWidget*) ali, FALSE, FALSE, (guint) 0);
		(similar_artist == NULL) ? NULL : (similar_artist = (g_object_unref (similar_artist), NULL));
	}
	/* Show web links */
	if (cfg_get_single_value_as_int_with_default (config, "MetaData", "show-web-links", 1) == 1) {
		GmpcSongLinks* song_links;
		song_links = g_object_ref_sink (gmpc_song_links_new (GMPC_SONG_LINKS_TYPE_ARTIST, song));
		gtk_box_pack_start ((GtkBox*) vbox, (GtkWidget*) song_links, FALSE, FALSE, (guint) 0);
		(song_links == NULL) ? NULL : (song_links = (g_object_unref (song_links), NULL));
	}
	/**
	         * Add it to the view
	         */
	gtk_container_add ((GtkContainer*) self->priv->metadata_box, (GtkWidget*) vbox);
	gtk_widget_show_all ((GtkWidget*) self->priv->metadata_box);
	(vbox == NULL) ? NULL : (vbox = (g_object_unref (vbox), NULL));
	(box == NULL) ? NULL : (box = (g_object_unref (box), NULL));
	(label == NULL) ? NULL : (label = (g_object_unref (label), NULL));
	(song == NULL) ? NULL : (song = (mpd_freeSong (song), NULL));
	(hbox == NULL) ? NULL : (hbox = (g_object_unref (hbox), NULL));
	(ali == NULL) ? NULL : (ali = (g_object_unref (ali), NULL));
	(artist_image == NULL) ? NULL : (artist_image = (g_object_unref (artist_image), NULL));
	(info_box == NULL) ? NULL : (info_box = (g_object_unref (info_box), NULL));
	(pt_label == NULL) ? NULL : (pt_label = (g_object_unref (pt_label), NULL));
	(button == NULL) ? NULL : (button = (g_object_unref (button), NULL));
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
	gboolean result;
	char* artist;
	char* album;
	mpd_Song* song;
	g_return_val_if_fail (self != NULL, FALSE);
	if (self->priv->block_update > 0) {
		self->priv->update_timeout = (guint) 0;
		result = FALSE;
		return result;
	}
	artist = gmpc_metadata_browser_browser_get_selected_artist (self);
	album = gmpc_metadata_browser_browser_get_selected_album (self);
	song = gmpc_metadata_browser_browser_get_selected_song (self);
	if (song != NULL) {
		GmpcMetadataBrowserHitem _tmp0_ = {0};
		GmpcMetadataBrowserHitem item;
		mpd_Song* _tmp2_;
		const mpd_Song* _tmp1_;
		GtkWidget* view;
		/** Add item to history */
		item = (memset (&_tmp0_, 0, sizeof (GmpcMetadataBrowserHitem)), _tmp0_);
		_tmp2_ = NULL;
		_tmp1_ = NULL;
		item.song = (_tmp2_ = (_tmp1_ = song, (_tmp1_ == NULL) ? NULL : mpd_songDup (_tmp1_)), (item.song == NULL) ? NULL : (item.song = (mpd_freeSong (item.song), NULL)), _tmp2_);
		item.type = GMPC_METADATA_BROWSER_HITEM_TYPE_SONG;
		gmpc_metadata_browser_history_add (self, &item);
		view = gmpc_metadata_browser_metadata_box_show_song (self, song, TRUE);
		gtk_container_add ((GtkContainer*) self->priv->metadata_box, view);
		gtk_widget_show_all ((GtkWidget*) self->priv->metadata_box);
		gmpc_metadata_browser_hitem_destroy (&item);
		(view == NULL) ? NULL : (view = (g_object_unref (view), NULL));
	} else {
		gboolean _tmp3_;
		_tmp3_ = FALSE;
		if (album != NULL) {
			_tmp3_ = artist != NULL;
		} else {
			_tmp3_ = FALSE;
		}
		if (_tmp3_) {
			GmpcMetadataBrowserHitem _tmp4_ = {0};
			GmpcMetadataBrowserHitem item;
			mpd_Song* _tmp5_;
			char* _tmp7_;
			const char* _tmp6_;
			char* _tmp9_;
			const char* _tmp8_;
			/** Add item to history */
			item = (memset (&_tmp4_, 0, sizeof (GmpcMetadataBrowserHitem)), _tmp4_);
			_tmp5_ = NULL;
			item.song = (_tmp5_ = mpd_newSong (), (item.song == NULL) ? NULL : (item.song = (mpd_freeSong (item.song), NULL)), _tmp5_);
			_tmp7_ = NULL;
			_tmp6_ = NULL;
			item.song->artist = (_tmp7_ = (_tmp6_ = artist, (_tmp6_ == NULL) ? NULL : g_strdup (_tmp6_)), item.song->artist = (g_free (item.song->artist), NULL), _tmp7_);
			_tmp9_ = NULL;
			_tmp8_ = NULL;
			item.song->album = (_tmp9_ = (_tmp8_ = album, (_tmp8_ == NULL) ? NULL : g_strdup (_tmp8_)), item.song->album = (g_free (item.song->album), NULL), _tmp9_);
			item.type = GMPC_METADATA_BROWSER_HITEM_TYPE_ALBUM;
			gmpc_metadata_browser_history_add (self, &item);
			gmpc_metadata_browser_metadata_box_show_album (self, artist, album);
			gmpc_metadata_browser_hitem_destroy (&item);
		} else {
			if (artist != NULL) {
				GmpcMetadataBrowserHitem _tmp10_ = {0};
				GmpcMetadataBrowserHitem item;
				mpd_Song* _tmp11_;
				char* _tmp13_;
				const char* _tmp12_;
				/** Add item to history */
				item = (memset (&_tmp10_, 0, sizeof (GmpcMetadataBrowserHitem)), _tmp10_);
				_tmp11_ = NULL;
				item.song = (_tmp11_ = mpd_newSong (), (item.song == NULL) ? NULL : (item.song = (mpd_freeSong (item.song), NULL)), _tmp11_);
				_tmp13_ = NULL;
				_tmp12_ = NULL;
				item.song->artist = (_tmp13_ = (_tmp12_ = artist, (_tmp12_ == NULL) ? NULL : g_strdup (_tmp12_)), item.song->artist = (g_free (item.song->artist), NULL), _tmp13_);
				item.type = GMPC_METADATA_BROWSER_HITEM_TYPE_ARTIST;
				gmpc_metadata_browser_history_add (self, &item);
				gmpc_metadata_browser_metadata_box_show_artist (self, artist);
				gmpc_metadata_browser_hitem_destroy (&item);
			}
		}
	}
	self->priv->update_timeout = (guint) 0;
	result = FALSE;
	artist = (g_free (artist), NULL);
	album = (g_free (album), NULL);
	(song == NULL) ? NULL : (song = (mpd_freeSong (song), NULL));
	return result;
}


/** 
     * Browser Interface bindings
     */
static void gmpc_metadata_browser_real_browser_add (GmpcPluginBrowserIface* base, GtkWidget* category_tree) {
	GmpcMetadataBrowser * self;
	GtkTreeView* _tmp0_;
	GtkTreeView* tree;
	GtkListStore* _tmp1_;
	GtkListStore* store;
	GtkTreeModel* _tmp2_;
	GtkTreeModel* model;
	GtkTreeIter iter = {0};
	GtkTreeRowReference* _tmp4_;
	GtkTreePath* _tmp3_;
	self = (GmpcMetadataBrowser*) base;
	g_return_if_fail (category_tree != NULL);
	_tmp0_ = NULL;
	tree = (_tmp0_ = GTK_TREE_VIEW (category_tree), (_tmp0_ == NULL) ? NULL : g_object_ref (_tmp0_));
	_tmp1_ = NULL;
	store = (_tmp1_ = GTK_LIST_STORE (gtk_tree_view_get_model (tree)), (_tmp1_ == NULL) ? NULL : g_object_ref (_tmp1_));
	_tmp2_ = NULL;
	model = (_tmp2_ = gtk_tree_view_get_model (tree), (_tmp2_ == NULL) ? NULL : g_object_ref (_tmp2_));
	playlist3_insert_browser (&iter, cfg_get_single_value_as_int_with_default (config, gmpc_plugin_base_get_name ((GmpcPluginBase*) self), "position", 100));
	gtk_list_store_set (store, &iter, 0, ((GmpcPluginBase*) self)->id, 1, _ (gmpc_plugin_base_get_name ((GmpcPluginBase*) self)), 3, "gtk-info", -1);
	/* Create a row reference */
	_tmp4_ = NULL;
	_tmp3_ = NULL;
	self->priv->rref = (_tmp4_ = gtk_tree_row_reference_new (model, _tmp3_ = gtk_tree_model_get_path (model, &iter)), (self->priv->rref == NULL) ? NULL : (self->priv->rref = (gtk_tree_row_reference_free (self->priv->rref), NULL)), _tmp4_);
	(_tmp3_ == NULL) ? NULL : (_tmp3_ = (gtk_tree_path_free (_tmp3_), NULL));
	(tree == NULL) ? NULL : (tree = (g_object_unref (tree), NULL));
	(store == NULL) ? NULL : (store = (g_object_unref (store), NULL));
	(model == NULL) ? NULL : (model = (g_object_unref (model), NULL));
}


static void gmpc_metadata_browser_real_browser_selected (GmpcPluginBrowserIface* base, GtkContainer* container) {
	GmpcMetadataBrowser * self;
	char* artist;
	char* _tmp0_;
	self = (GmpcMetadataBrowser*) base;
	g_return_if_fail (container != NULL);
	artist = NULL;
	self->priv->selected = TRUE;
	gmpc_metadata_browser_browser_init (self);
	gtk_container_add (container, (GtkWidget*) self->priv->paned);
	/* update if non selected */
	_tmp0_ = NULL;
	artist = (_tmp0_ = gmpc_metadata_browser_browser_get_selected_artist (self), artist = (g_free (artist), NULL), _tmp0_);
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
	gmpc_metadata_browser_history_clear (self);
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
	if ((what & MPD_CST_DATABASE) != 0) {
		gmpc_metadata_browser_reload_browsers (self);
		if (self->priv->current != NULL) {
			gmpc_metadata_browser_show_hitem (self, (GmpcMetadataBrowserHitem*) self->priv->current->data);
		}
	}
}


static void gmpc_metadata_browser_show_hitem (GmpcMetadataBrowser* self, const GmpcMetadataBrowserHitem* hi) {
	g_return_if_fail (self != NULL);
	switch ((*hi).type) {
		case GMPC_METADATA_BROWSER_HITEM_TYPE_ARTIST:
		{
			gmpc_metadata_browser_set_artist (self, (*hi).song->artist);
			break;
		}
		case GMPC_METADATA_BROWSER_HITEM_TYPE_ALBUM:
		{
			gmpc_metadata_browser_set_album (self, (*hi).song->artist, (*hi).song->album);
			break;
		}
		case GMPC_METADATA_BROWSER_HITEM_TYPE_SONG:
		{
			gmpc_metadata_browser_set_song (self, (*hi).song);
			break;
		}
		default:
		{
			gmpc_metadata_browser_metadata_box_clear (self);
			break;
		}
	}
}


static void gmpc_metadata_browser_history_previous (GmpcMetadataBrowser* self) {
	gboolean _tmp0_;
	g_return_if_fail (self != NULL);
	_tmp0_ = FALSE;
	if (self->priv->history == NULL) {
		_tmp0_ = TRUE;
	} else {
		_tmp0_ = self->priv->current == NULL;
	}
	if (_tmp0_) {
		return;
	}
	if (self->priv->current->next == NULL) {
		return;
	}
	self->priv->current = self->priv->current->next;
	if (self->priv->current != NULL) {
		gmpc_metadata_browser_show_hitem (self, (GmpcMetadataBrowserHitem*) self->priv->current->data);
	} else {
		gmpc_metadata_browser_metadata_box_clear (self);
	}
}


static void gmpc_metadata_browser_history_next (GmpcMetadataBrowser* self) {
	gboolean _tmp0_;
	g_return_if_fail (self != NULL);
	_tmp0_ = FALSE;
	if (self->priv->history == NULL) {
		_tmp0_ = TRUE;
	} else {
		_tmp0_ = self->priv->current == NULL;
	}
	if (_tmp0_) {
		return;
	}
	if (self->priv->current->prev == NULL) {
		return;
	}
	self->priv->current = self->priv->current->prev;
	if (self->priv->current != NULL) {
		gmpc_metadata_browser_show_hitem (self, (GmpcMetadataBrowserHitem*) self->priv->current->data);
	} else {
		gmpc_metadata_browser_metadata_box_clear (self);
	}
}


static void gmpc_metadata_browser_history_show_list_clicked (GmpcMetadataBrowser* self, GtkMenuItem* item) {
	GList* a;
	g_return_if_fail (self != NULL);
	g_return_if_fail (item != NULL);
	a = (GList*) g_object_get_data ((GObject*) item, "current");
	if (a != NULL) {
		self->priv->current = a;
		gmpc_metadata_browser_show_hitem (self, (GmpcMetadataBrowserHitem*) self->priv->current->data);
	}
}


static void _gmpc_metadata_browser_history_show_list_clicked_gtk_menu_item_activate (GtkMenuItem* _sender, gpointer self) {
	gmpc_metadata_browser_history_show_list_clicked (self, _sender);
}


static void gmpc_metadata_browser_history_show_list (GmpcMetadataBrowser* self) {
	GtkMenu* menu;
	GList* iter;
	g_return_if_fail (self != NULL);
	menu = g_object_ref_sink ((GtkMenu*) gtk_menu_new ());
	iter = g_list_last (self->priv->history);
	while (TRUE) {
		GmpcMetadataBrowserHitem* _tmp0_;
		GmpcMetadataBrowserHitem* i;
		char* label;
		GtkCheckMenuItem* item;
		gboolean _tmp8_;
		if (!(iter != NULL)) {
			break;
		}
		_tmp0_ = NULL;
		i = (_tmp0_ = (GmpcMetadataBrowserHitem*) iter->data, (_tmp0_ == NULL) ? NULL : gmpc_metadata_browser_hitem_dup (_tmp0_));
		label = g_strdup ("");
		if ((*i).type == GMPC_METADATA_BROWSER_HITEM_TYPE_ARTIST) {
			char* _tmp2_;
			const char* _tmp1_;
			_tmp2_ = NULL;
			_tmp1_ = NULL;
			label = (_tmp2_ = (_tmp1_ = (*i).song->artist, (_tmp1_ == NULL) ? NULL : g_strdup (_tmp1_)), label = (g_free (label), NULL), _tmp2_);
		} else {
			if ((*i).type == GMPC_METADATA_BROWSER_HITEM_TYPE_ALBUM) {
				char* _tmp3_;
				_tmp3_ = NULL;
				label = (_tmp3_ = g_strdup_printf ("%s - %s", (*i).song->artist, (*i).song->album), label = (g_free (label), NULL), _tmp3_);
			} else {
				if ((*i).type == GMPC_METADATA_BROWSER_HITEM_TYPE_SONG) {
					if ((*i).song->title != NULL) {
						char* _tmp5_;
						const char* _tmp4_;
						_tmp5_ = NULL;
						_tmp4_ = NULL;
						label = (_tmp5_ = (_tmp4_ = (*i).song->title, (_tmp4_ == NULL) ? NULL : g_strdup (_tmp4_)), label = (g_free (label), NULL), _tmp5_);
					} else {
						char* _tmp7_;
						const char* _tmp6_;
						_tmp7_ = NULL;
						_tmp6_ = NULL;
						label = (_tmp7_ = (_tmp6_ = _ ("Unknown"), (_tmp6_ == NULL) ? NULL : g_strdup (_tmp6_)), label = (g_free (label), NULL), _tmp7_);
					}
				}
			}
		}
		item = g_object_ref_sink ((GtkCheckMenuItem*) gtk_check_menu_item_new_with_label (label));
		gtk_check_menu_item_set_draw_as_radio (item, TRUE);
		_tmp8_ = FALSE;
		if (self->priv->current != NULL) {
			_tmp8_ = self->priv->current == iter;
		} else {
			_tmp8_ = FALSE;
		}
		if (_tmp8_) {
			gtk_check_menu_item_set_active (item, TRUE);
		}
		g_signal_connect_object ((GtkMenuItem*) item, "activate", (GCallback) _gmpc_metadata_browser_history_show_list_clicked_gtk_menu_item_activate, self, 0);
		g_object_set_data ((GObject*) item, "current", (void*) iter);
		gtk_menu_shell_append ((GtkMenuShell*) menu, (GtkWidget*) ((GtkMenuItem*) item));
		iter = iter->prev;
		(i == NULL) ? NULL : (i = (gmpc_metadata_browser_hitem_free (i), NULL));
		label = (g_free (label), NULL);
		(item == NULL) ? NULL : (item = (g_object_unref (item), NULL));
	}
	gtk_widget_show_all ((GtkWidget*) menu);
	gtk_menu_popup (menu, NULL, NULL, NULL, NULL, (guint) 0, gtk_get_current_event_time ());
	(menu == NULL) ? NULL : (menu = (g_object_unref (menu), NULL));
}


static void _gmpc_metadata_browser_history_next_gtk_button_clicked (GtkButton* _sender, gpointer self) {
	gmpc_metadata_browser_history_next (self);
}


static void _gmpc_metadata_browser_history_show_list_gtk_button_clicked (GtkButton* _sender, gpointer self) {
	gmpc_metadata_browser_history_show_list (self);
}


static void _gmpc_metadata_browser_history_previous_gtk_button_clicked (GtkButton* _sender, gpointer self) {
	gmpc_metadata_browser_history_previous (self);
}


static GtkHBox* gmpc_metadata_browser_history_buttons (GmpcMetadataBrowser* self) {
	GtkHBox* result;
	GtkHBox* box;
	gboolean _tmp0_;
	GtkButton* next_but;
	gboolean _tmp1_;
	gboolean _tmp2_;
	GtkButton* back_but;
	gboolean _tmp4_;
	g_return_val_if_fail (self != NULL, NULL);
	box = g_object_ref_sink ((GtkHBox*) gtk_hbox_new (FALSE, 0));
	_tmp0_ = FALSE;
	if (self->priv->history == NULL) {
		_tmp0_ = self->priv->current == NULL;
	} else {
		_tmp0_ = FALSE;
	}
	if (_tmp0_) {
		result = box;
		return result;
	}
	next_but = g_object_ref_sink ((GtkButton*) gtk_button_new_from_stock ("gtk-go-forward"));
	_tmp1_ = FALSE;
	if (self->priv->current == NULL) {
		_tmp1_ = TRUE;
	} else {
		_tmp1_ = self->priv->current->prev == NULL;
	}
	if (_tmp1_) {
		g_object_set ((GtkWidget*) next_but, "sensitive", FALSE, NULL);
	}
	g_signal_connect_object (next_but, "clicked", (GCallback) _gmpc_metadata_browser_history_next_gtk_button_clicked, self, 0);
	gtk_box_pack_end ((GtkBox*) box, (GtkWidget*) next_but, FALSE, FALSE, (guint) 0);
	_tmp2_ = FALSE;
	if (self->priv->current != NULL) {
		gboolean _tmp3_;
		_tmp3_ = FALSE;
		if (self->priv->current->next != NULL) {
			_tmp3_ = TRUE;
		} else {
			_tmp3_ = self->priv->current->prev != NULL;
		}
		_tmp2_ = _tmp3_;
	} else {
		_tmp2_ = FALSE;
	}
	if (_tmp2_) {
		GtkButton* dd_but;
		dd_but = g_object_ref_sink ((GtkButton*) gtk_button_new_with_label ("L"));
		g_signal_connect_object (dd_but, "clicked", (GCallback) _gmpc_metadata_browser_history_show_list_gtk_button_clicked, self, 0);
		gtk_box_pack_end ((GtkBox*) box, (GtkWidget*) dd_but, FALSE, FALSE, (guint) 0);
		(dd_but == NULL) ? NULL : (dd_but = (g_object_unref (dd_but), NULL));
	}
	back_but = g_object_ref_sink ((GtkButton*) gtk_button_new_from_stock ("gtk-go-back"));
	_tmp4_ = FALSE;
	if (self->priv->current == NULL) {
		_tmp4_ = TRUE;
	} else {
		_tmp4_ = self->priv->current->next == NULL;
	}
	if (_tmp4_) {
		g_object_set ((GtkWidget*) back_but, "sensitive", FALSE, NULL);
	}
	g_signal_connect_object (back_but, "clicked", (GCallback) _gmpc_metadata_browser_history_previous_gtk_button_clicked, self, 0);
	gtk_box_pack_end ((GtkBox*) box, (GtkWidget*) back_but, FALSE, FALSE, (guint) 0);
	result = box;
	(next_but == NULL) ? NULL : (next_but = (g_object_unref (next_but), NULL));
	(back_but == NULL) ? NULL : (back_but = (g_object_unref (back_but), NULL));
	return result;
}


static void gmpc_metadata_browser_history_add (GmpcMetadataBrowser* self, const GmpcMetadataBrowserHitem* hi) {
	GmpcMetadataBrowserHitem* _tmp3_;
	g_return_if_fail (self != NULL);
	if (self->priv->history != NULL) {
		GmpcMetadataBrowserHitem a;
		a = *((GmpcMetadataBrowserHitem*) self->priv->current->data);
		if (a.type == (*hi).type) {
			char* _tmp1_;
			char* _tmp0_;
			gboolean _tmp2_;
			_tmp1_ = NULL;
			_tmp0_ = NULL;
			if ((_tmp2_ = _vala_strcmp0 (_tmp0_ = mpd_song_checksum (a.song), _tmp1_ = mpd_song_checksum ((*hi).song)) == 0, _tmp1_ = (g_free (_tmp1_), NULL), _tmp0_ = (g_free (_tmp0_), NULL), _tmp2_)) {
				return;
			}
		}
	}
	_tmp3_ = NULL;
	self->priv->history = g_list_prepend (self->priv->history, (_tmp3_ = &(*hi), (_tmp3_ == NULL) ? NULL : gmpc_metadata_browser_hitem_dup (_tmp3_)));
	if (g_list_length (self->priv->history) > 25) {
		GList* a;
		a = g_list_last (self->priv->history);
		self->priv->history = g_list_remove (self->priv->history, (GmpcMetadataBrowserHitem*) a->data);
	}
	self->priv->current = self->priv->history;
}


static void gmpc_metadata_browser_history_clear (GmpcMetadataBrowser* self) {
	GList* _tmp0_;
	g_return_if_fail (self != NULL);
	self->priv->current = NULL;
	_tmp0_ = NULL;
	self->priv->history = (_tmp0_ = NULL, (self->priv->history == NULL) ? NULL : (self->priv->history = (_g_list_free_gmpc_metadata_browser_hitem_free (self->priv->history), NULL)), _tmp0_);
}


/**
     * Public api 
     */
void gmpc_metadata_browser_set_artist (GmpcMetadataBrowser* self, const char* artist) {
	GtkTreeIter iter = {0};
	g_return_if_fail (self != NULL);
	g_return_if_fail (artist != NULL);
	if (!gmpc_plugin_base_get_enabled ((GmpcPluginBase*) self)) {
		return;
	}
	self->priv->block_update++;
	gtk_tree_selection_unselect_all (gtk_tree_view_get_selection (self->priv->tree_artist));
	gtk_tree_selection_unselect_all (gtk_tree_view_get_selection (self->priv->tree_album));
	/* clear */
	gtk_entry_set_text (self->priv->artist_filter_entry, "");
	if (gtk_tree_model_get_iter_first ((GtkTreeModel*) self->priv->model_filter_artist, &iter)) {
		{
			gboolean _tmp0_;
			_tmp0_ = TRUE;
			while (TRUE) {
				char* lartist;
				gboolean _tmp1_;
				if (!_tmp0_) {
					if (!gtk_tree_model_iter_next ((GtkTreeModel*) self->priv->model_filter_artist, &iter)) {
						break;
					}
				}
				_tmp0_ = FALSE;
				lartist = NULL;
				gtk_tree_model_get ((GtkTreeModel*) self->priv->model_filter_artist, &iter, 7, &lartist, -1, -1);
				_tmp1_ = FALSE;
				if (lartist != NULL) {
					_tmp1_ = g_utf8_collate (lartist, artist) == 0;
				} else {
					_tmp1_ = FALSE;
				}
				if (_tmp1_) {
					GtkTreePath* _tmp2_;
					gtk_tree_selection_select_iter (gtk_tree_view_get_selection (self->priv->tree_artist), &iter);
					_tmp2_ = NULL;
					gtk_tree_view_scroll_to_cell (self->priv->tree_artist, _tmp2_ = gtk_tree_model_get_path ((GtkTreeModel*) self->priv->model_filter_artist, &iter), NULL, TRUE, 0.5f, 0.f);
					(_tmp2_ == NULL) ? NULL : (_tmp2_ = (gtk_tree_path_free (_tmp2_), NULL));
					self->priv->block_update--;
					gmpc_metadata_browser_metadata_box_clear (self);
					gmpc_metadata_browser_metadata_box_update (self);
					lartist = (g_free (lartist), NULL);
					return;
				}
				lartist = (g_free (lartist), NULL);
			}
		}
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
	if (!gmpc_plugin_base_get_enabled ((GmpcPluginBase*) self)) {
		return;
	}
	self->priv->block_update++;
	gmpc_metadata_browser_set_artist (self, artist);
	/* clear */
	gtk_entry_set_text (self->priv->album_filter_entry, "");
	if (gtk_tree_model_get_iter_first ((GtkTreeModel*) self->priv->model_filter_album, &iter)) {
		{
			gboolean _tmp0_;
			_tmp0_ = TRUE;
			while (TRUE) {
				char* lalbum;
				gboolean _tmp1_;
				if (!_tmp0_) {
					if (!gtk_tree_model_iter_next ((GtkTreeModel*) self->priv->model_filter_album, &iter)) {
						break;
					}
				}
				_tmp0_ = FALSE;
				lalbum = NULL;
				gtk_tree_model_get ((GtkTreeModel*) self->priv->model_filter_album, &iter, 6, &lalbum, -1, -1);
				_tmp1_ = FALSE;
				if (lalbum != NULL) {
					_tmp1_ = g_utf8_collate (lalbum, album) == 0;
				} else {
					_tmp1_ = FALSE;
				}
				if (_tmp1_) {
					GtkTreePath* _tmp2_;
					gtk_tree_selection_select_iter (gtk_tree_view_get_selection (self->priv->tree_album), &iter);
					_tmp2_ = NULL;
					gtk_tree_view_scroll_to_cell (self->priv->tree_album, _tmp2_ = gtk_tree_model_get_path ((GtkTreeModel*) self->priv->model_filter_album, &iter), NULL, TRUE, 0.5f, 0.f);
					(_tmp2_ == NULL) ? NULL : (_tmp2_ = (gtk_tree_path_free (_tmp2_), NULL));
					gtk_tree_selection_unselect_all (gtk_tree_view_get_selection (self->priv->tree_songs));
					self->priv->block_update--;
					gmpc_metadata_browser_metadata_box_update (self);
					lalbum = (g_free (lalbum), NULL);
					return;
				}
				lalbum = (g_free (lalbum), NULL);
			}
		}
	}
	gtk_tree_selection_unselect_all (gtk_tree_view_get_selection (self->priv->tree_songs));
	self->priv->block_update--;
	gmpc_metadata_browser_metadata_box_clear (self);
	gmpc_metadata_browser_metadata_box_update (self);
}


void gmpc_metadata_browser_set_song (GmpcMetadataBrowser* self, const mpd_Song* song) {
	GtkTreeIter iter = {0};
	GmpcMetadataBrowserHitem _tmp3_ = {0};
	GmpcMetadataBrowserHitem item;
	mpd_Song* _tmp5_;
	const mpd_Song* _tmp4_;
	GtkWidget* view;
	g_return_if_fail (self != NULL);
	g_return_if_fail (song != NULL);
	if (!gmpc_plugin_base_get_enabled ((GmpcPluginBase*) self)) {
		return;
	}
	self->priv->block_update++;
	if (song->artist != NULL) {
		gmpc_metadata_browser_set_artist (self, song->artist);
		if (song->album != NULL) {
			gmpc_metadata_browser_set_album (self, song->artist, song->album);
		}
	}
	if (gtk_tree_model_get_iter_first ((GtkTreeModel*) self->priv->model_songs, &iter)) {
		{
			gboolean _tmp0_;
			_tmp0_ = TRUE;
			while (TRUE) {
				char* ltitle;
				gboolean _tmp1_;
				if (!_tmp0_) {
					if (!gtk_tree_model_iter_next ((GtkTreeModel*) self->priv->model_songs, &iter)) {
						break;
					}
				}
				_tmp0_ = FALSE;
				ltitle = NULL;
				gtk_tree_model_get ((GtkTreeModel*) self->priv->model_songs, &iter, 7, &ltitle, -1, -1);
				_tmp1_ = FALSE;
				if (ltitle != NULL) {
					_tmp1_ = g_utf8_collate (ltitle, song->title) == 0;
				} else {
					_tmp1_ = FALSE;
				}
				if (_tmp1_) {
					GtkTreePath* _tmp2_;
					gtk_tree_selection_select_iter (gtk_tree_view_get_selection (self->priv->tree_songs), &iter);
					_tmp2_ = NULL;
					gtk_tree_view_scroll_to_cell (self->priv->tree_songs, _tmp2_ = gtk_tree_model_get_path ((GtkTreeModel*) self->priv->model_songs, &iter), NULL, TRUE, 0.5f, 0.f);
					(_tmp2_ == NULL) ? NULL : (_tmp2_ = (gtk_tree_path_free (_tmp2_), NULL));
					self->priv->block_update--;
					gmpc_metadata_browser_metadata_box_update (self);
					ltitle = (g_free (ltitle), NULL);
					return;
				}
				ltitle = (g_free (ltitle), NULL);
			}
		}
	}
	self->priv->block_update--;
	gmpc_metadata_browser_metadata_box_clear (self);
	if (self->priv->update_timeout > 0) {
		g_source_remove (self->priv->update_timeout);
		self->priv->update_timeout = (guint) 0;
	}
	/** Add item to history */
	item = (memset (&_tmp3_, 0, sizeof (GmpcMetadataBrowserHitem)), _tmp3_);
	_tmp5_ = NULL;
	_tmp4_ = NULL;
	item.song = (_tmp5_ = (_tmp4_ = song, (_tmp4_ == NULL) ? NULL : mpd_songDup (_tmp4_)), (item.song == NULL) ? NULL : (item.song = (mpd_freeSong (item.song), NULL)), _tmp5_);
	item.type = GMPC_METADATA_BROWSER_HITEM_TYPE_SONG;
	gmpc_metadata_browser_history_add (self, &item);
	view = gmpc_metadata_browser_metadata_box_show_song (self, song, TRUE);
	gtk_container_add ((GtkContainer*) self->priv->metadata_box, view);
	gtk_widget_show_all ((GtkWidget*) self->priv->metadata_box);
	gmpc_metadata_browser_hitem_destroy (&item);
	(view == NULL) ? NULL : (view = (g_object_unref (view), NULL));
}


void gmpc_metadata_browser_select_browser (GmpcMetadataBrowser* self, GtkTreeView* tree) {
	g_return_if_fail (self != NULL);
	if (self->priv->rref != NULL) {
		GtkTreeView* category_tree;
		GtkTreeSelection* _tmp0_;
		GtkTreeSelection* sel;
		GtkTreePath* path;
		category_tree = playlist3_get_category_tree_view ();
		_tmp0_ = NULL;
		sel = (_tmp0_ = gtk_tree_view_get_selection (category_tree), (_tmp0_ == NULL) ? NULL : g_object_ref (_tmp0_));
		path = gtk_tree_row_reference_get_path (self->priv->rref);
		if (path != NULL) {
			gtk_tree_selection_select_path (sel, path);
		}
		(sel == NULL) ? NULL : (sel = (g_object_unref (sel), NULL));
		(path == NULL) ? NULL : (path = (gtk_tree_path_free (path), NULL));
	}
}


static void _lambda0_ (GtkToggleButton* source, GmpcMetadataBrowser* self) {
	g_return_if_fail (source != NULL);
	cfg_set_single_value_as_int (config, "MetaData", "show-artist-information", (gint) gtk_toggle_button_get_active (source));
}


static void __lambda0__gtk_toggle_button_toggled (GtkToggleButton* _sender, gpointer self) {
	_lambda0_ (_sender, self);
}


static void _lambda1_ (GtkToggleButton* source, GmpcMetadataBrowser* self) {
	g_return_if_fail (source != NULL);
	cfg_set_single_value_as_int (config, "MetaData", "show-album-information", (gint) gtk_toggle_button_get_active (source));
}


static void __lambda1__gtk_toggle_button_toggled (GtkToggleButton* _sender, gpointer self) {
	_lambda1_ (_sender, self);
}


static void _lambda2_ (GtkToggleButton* source, GmpcMetadataBrowser* self) {
	g_return_if_fail (source != NULL);
	cfg_set_single_value_as_int (config, "MetaData", "show-similar-artist", (gint) gtk_toggle_button_get_active (source));
}


static void __lambda2__gtk_toggle_button_toggled (GtkToggleButton* _sender, gpointer self) {
	_lambda2_ (_sender, self);
}


static void _lambda3_ (GtkToggleButton* source, GmpcMetadataBrowser* self) {
	g_return_if_fail (source != NULL);
	cfg_set_single_value_as_int (config, "MetaData", "show-lyrics", (gint) gtk_toggle_button_get_active (source));
}


static void __lambda3__gtk_toggle_button_toggled (GtkToggleButton* _sender, gpointer self) {
	_lambda3_ (_sender, self);
}


static void _lambda4_ (GtkToggleButton* source, GmpcMetadataBrowser* self) {
	g_return_if_fail (source != NULL);
	cfg_set_single_value_as_int (config, "MetaData", "show-guitar-tabs", (gint) gtk_toggle_button_get_active (source));
}


static void __lambda4__gtk_toggle_button_toggled (GtkToggleButton* _sender, gpointer self) {
	_lambda4_ (_sender, self);
}


static void _lambda5_ (GtkToggleButton* source, GmpcMetadataBrowser* self) {
	g_return_if_fail (source != NULL);
	cfg_set_single_value_as_int (config, "MetaData", "show-similar-songs", (gint) gtk_toggle_button_get_active (source));
}


static void __lambda5__gtk_toggle_button_toggled (GtkToggleButton* _sender, gpointer self) {
	_lambda5_ (_sender, self);
}


static void _lambda6_ (GtkToggleButton* source, GmpcMetadataBrowser* self) {
	g_return_if_fail (source != NULL);
	cfg_set_single_value_as_int (config, "MetaData", "show-web-links", (gint) gtk_toggle_button_get_active (source));
}


static void __lambda6__gtk_toggle_button_toggled (GtkToggleButton* _sender, gpointer self) {
	_lambda6_ (_sender, self);
}


/** 
     * Preferences
     */
static void gmpc_metadata_browser_real_preferences_pane_construct (GmpcPluginPreferencesIface* base, GtkContainer* container) {
	GmpcMetadataBrowser * self;
	GtkVBox* box;
	GtkLabel* label;
	GtkCheckButton* chk;
	GtkCheckButton* _tmp0_;
	GtkCheckButton* _tmp1_;
	GtkCheckButton* _tmp2_;
	GtkCheckButton* _tmp3_;
	GtkCheckButton* _tmp4_;
	GtkCheckButton* _tmp5_;
	self = (GmpcMetadataBrowser*) base;
	g_return_if_fail (container != NULL);
	box = g_object_ref_sink ((GtkVBox*) gtk_vbox_new (FALSE, 6));
	/* Title */
	label = g_object_ref_sink ((GtkLabel*) gtk_label_new (_ ("Enable/disable metadata options")));
	gtk_misc_set_alignment ((GtkMisc*) label, 0.0f, 0.5f);
	gtk_box_pack_start ((GtkBox*) box, (GtkWidget*) label, FALSE, FALSE, (guint) 0);
	/* Artist information */
	chk = g_object_ref_sink ((GtkCheckButton*) gtk_check_button_new_with_label (_ ("Artist information")));
	gtk_toggle_button_set_active ((GtkToggleButton*) chk, cfg_get_single_value_as_int_with_default (config, "MetaData", "show-artist-information", 1) == 1);
	gtk_box_pack_start ((GtkBox*) box, (GtkWidget*) chk, FALSE, FALSE, (guint) 0);
	g_signal_connect ((GtkToggleButton*) chk, "toggled", (GCallback) __lambda0__gtk_toggle_button_toggled, self);
	/* Album information */
	_tmp0_ = NULL;
	chk = (_tmp0_ = g_object_ref_sink ((GtkCheckButton*) gtk_check_button_new_with_label (_ ("Album information"))), (chk == NULL) ? NULL : (chk = (g_object_unref (chk), NULL)), _tmp0_);
	gtk_toggle_button_set_active ((GtkToggleButton*) chk, cfg_get_single_value_as_int_with_default (config, "MetaData", "show-album-information", 1) == 1);
	gtk_box_pack_start ((GtkBox*) box, (GtkWidget*) chk, FALSE, FALSE, (guint) 0);
	g_signal_connect ((GtkToggleButton*) chk, "toggled", (GCallback) __lambda1__gtk_toggle_button_toggled, self);
	/* Artist similar */
	_tmp1_ = NULL;
	chk = (_tmp1_ = g_object_ref_sink ((GtkCheckButton*) gtk_check_button_new_with_label (_ ("Similar Artist"))), (chk == NULL) ? NULL : (chk = (g_object_unref (chk), NULL)), _tmp1_);
	gtk_toggle_button_set_active ((GtkToggleButton*) chk, cfg_get_single_value_as_int_with_default (config, "MetaData", "show-similar-artist", 1) == 1);
	gtk_box_pack_start ((GtkBox*) box, (GtkWidget*) chk, FALSE, FALSE, (guint) 0);
	g_signal_connect ((GtkToggleButton*) chk, "toggled", (GCallback) __lambda2__gtk_toggle_button_toggled, self);
	/* Lyrics */
	_tmp2_ = NULL;
	chk = (_tmp2_ = g_object_ref_sink ((GtkCheckButton*) gtk_check_button_new_with_label (_ ("Lyrics"))), (chk == NULL) ? NULL : (chk = (g_object_unref (chk), NULL)), _tmp2_);
	gtk_toggle_button_set_active ((GtkToggleButton*) chk, cfg_get_single_value_as_int_with_default (config, "MetaData", "show-lyrics", 1) == 1);
	gtk_box_pack_start ((GtkBox*) box, (GtkWidget*) chk, FALSE, FALSE, (guint) 0);
	g_signal_connect ((GtkToggleButton*) chk, "toggled", (GCallback) __lambda3__gtk_toggle_button_toggled, self);
	/* Guitar Tabs*/
	_tmp3_ = NULL;
	chk = (_tmp3_ = g_object_ref_sink ((GtkCheckButton*) gtk_check_button_new_with_label (_ ("Guitar Tabs"))), (chk == NULL) ? NULL : (chk = (g_object_unref (chk), NULL)), _tmp3_);
	gtk_toggle_button_set_active ((GtkToggleButton*) chk, cfg_get_single_value_as_int_with_default (config, "MetaData", "show-guitar-tabs", 1) == 1);
	gtk_box_pack_start ((GtkBox*) box, (GtkWidget*) chk, FALSE, FALSE, (guint) 0);
	g_signal_connect ((GtkToggleButton*) chk, "toggled", (GCallback) __lambda4__gtk_toggle_button_toggled, self);
	/* Similar songs*/
	_tmp4_ = NULL;
	chk = (_tmp4_ = g_object_ref_sink ((GtkCheckButton*) gtk_check_button_new_with_label (_ ("Similar Songs"))), (chk == NULL) ? NULL : (chk = (g_object_unref (chk), NULL)), _tmp4_);
	gtk_toggle_button_set_active ((GtkToggleButton*) chk, cfg_get_single_value_as_int_with_default (config, "MetaData", "show-similar-songs", 1) == 1);
	gtk_box_pack_start ((GtkBox*) box, (GtkWidget*) chk, FALSE, FALSE, (guint) 0);
	g_signal_connect ((GtkToggleButton*) chk, "toggled", (GCallback) __lambda5__gtk_toggle_button_toggled, self);
	/* Web links*/
	_tmp5_ = NULL;
	chk = (_tmp5_ = g_object_ref_sink ((GtkCheckButton*) gtk_check_button_new_with_label (_ ("Web links"))), (chk == NULL) ? NULL : (chk = (g_object_unref (chk), NULL)), _tmp5_);
	gtk_toggle_button_set_active ((GtkToggleButton*) chk, cfg_get_single_value_as_int_with_default (config, "MetaData", "show-web-links", 1) == 1);
	gtk_box_pack_start ((GtkBox*) box, (GtkWidget*) chk, FALSE, FALSE, (guint) 0);
	g_signal_connect ((GtkToggleButton*) chk, "toggled", (GCallback) __lambda6__gtk_toggle_button_toggled, self);
	gtk_container_add (container, (GtkWidget*) box);
	gtk_widget_show_all ((GtkWidget*) box);
	(box == NULL) ? NULL : (box = (g_object_unref (box), NULL));
	(label == NULL) ? NULL : (label = (g_object_unref (label), NULL));
	(chk == NULL) ? NULL : (chk = (g_object_unref (chk), NULL));
}


static void gmpc_metadata_browser_real_preferences_pane_destroy (GmpcPluginPreferencesIface* base, GtkContainer* container) {
	GmpcMetadataBrowser * self;
	self = (GmpcMetadataBrowser*) base;
	g_return_if_fail (container != NULL);
	{
		GList* child_collection;
		GList* child_it;
		child_collection = gtk_container_get_children (container);
		for (child_it = child_collection; child_it != NULL; child_it = child_it->next) {
			GtkWidget* _tmp0_;
			GtkWidget* child;
			_tmp0_ = NULL;
			child = (_tmp0_ = (GtkWidget*) child_it->data, (_tmp0_ == NULL) ? NULL : g_object_ref (_tmp0_));
			{
				gtk_container_remove (container, child);
				(child == NULL) ? NULL : (child = (g_object_unref (child), NULL));
			}
		}
		(child_collection == NULL) ? NULL : (child_collection = (g_list_free (child_collection), NULL));
	}
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


static void gmpc_metadata_browser_hitem_copy (const GmpcMetadataBrowserHitem* self, GmpcMetadataBrowserHitem* dest) {
	const mpd_Song* _tmp1_;
	_tmp1_ = NULL;
	dest->type = self->type;
	dest->song = (_tmp1_ = self->song, (_tmp1_ == NULL) ? NULL : mpd_songDup (_tmp1_));
}


static void gmpc_metadata_browser_hitem_destroy (GmpcMetadataBrowserHitem* self) {
	(self->song == NULL) ? NULL : (self->song = (mpd_freeSong (self->song), NULL));
}


static GmpcMetadataBrowserHitem* gmpc_metadata_browser_hitem_dup (const GmpcMetadataBrowserHitem* self) {
	GmpcMetadataBrowserHitem* dup;
	dup = g_new0 (GmpcMetadataBrowserHitem, 1);
	gmpc_metadata_browser_hitem_copy (self, dup);
	return dup;
}


static void gmpc_metadata_browser_hitem_free (GmpcMetadataBrowserHitem* self) {
	gmpc_metadata_browser_hitem_destroy (self);
	g_free (self);
}


static GType gmpc_metadata_browser_hitem_get_type (void) {
	static GType gmpc_metadata_browser_hitem_type_id = 0;
	if (gmpc_metadata_browser_hitem_type_id == 0) {
		gmpc_metadata_browser_hitem_type_id = g_boxed_type_register_static ("GmpcMetadataBrowserHitem", (GBoxedCopyFunc) gmpc_metadata_browser_hitem_dup, (GBoxedFreeFunc) gmpc_metadata_browser_hitem_free);
	}
	return gmpc_metadata_browser_hitem_type_id;
}


static void gmpc_metadata_browser_class_init (GmpcMetadataBrowserClass * klass) {
	gmpc_metadata_browser_parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (GmpcMetadataBrowserPrivate));
	GMPC_PLUGIN_BASE_CLASS (klass)->get_version = gmpc_metadata_browser_real_get_version;
	GMPC_PLUGIN_BASE_CLASS (klass)->get_name = gmpc_metadata_browser_real_get_name;
	GMPC_PLUGIN_BASE_CLASS (klass)->save_yourself = gmpc_metadata_browser_real_save_yourself;
	G_OBJECT_CLASS (klass)->constructor = gmpc_metadata_browser_constructor;
	G_OBJECT_CLASS (klass)->finalize = gmpc_metadata_browser_finalize;
}


static void gmpc_metadata_browser_gmpc_plugin_browser_iface_interface_init (GmpcPluginBrowserIfaceIface * iface) {
	gmpc_metadata_browser_gmpc_plugin_browser_iface_parent_iface = g_type_interface_peek_parent (iface);
	iface->browser_add_go_menu = gmpc_metadata_browser_real_browser_add_go_menu;
	iface->browser_add = gmpc_metadata_browser_real_browser_add;
	iface->browser_selected = gmpc_metadata_browser_real_browser_selected;
	iface->browser_unselected = gmpc_metadata_browser_real_browser_unselected;
}


static void gmpc_metadata_browser_gmpc_plugin_preferences_iface_interface_init (GmpcPluginPreferencesIfaceIface * iface) {
	gmpc_metadata_browser_gmpc_plugin_preferences_iface_parent_iface = g_type_interface_peek_parent (iface);
	iface->preferences_pane_construct = gmpc_metadata_browser_real_preferences_pane_construct;
	iface->preferences_pane_destroy = gmpc_metadata_browser_real_preferences_pane_destroy;
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
	self->priv->history = NULL;
	self->priv->current = NULL;
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
	(self->priv->history == NULL) ? NULL : (self->priv->history = (_g_list_free_gmpc_metadata_browser_hitem_free (self->priv->history), NULL));
	G_OBJECT_CLASS (gmpc_metadata_browser_parent_class)->finalize (obj);
}


GType gmpc_metadata_browser_get_type (void) {
	static GType gmpc_metadata_browser_type_id = 0;
	if (gmpc_metadata_browser_type_id == 0) {
		static const GTypeInfo g_define_type_info = { sizeof (GmpcMetadataBrowserClass), (GBaseInitFunc) NULL, (GBaseFinalizeFunc) NULL, (GClassInitFunc) gmpc_metadata_browser_class_init, (GClassFinalizeFunc) NULL, NULL, sizeof (GmpcMetadataBrowser), 0, (GInstanceInitFunc) gmpc_metadata_browser_instance_init, NULL };
		static const GInterfaceInfo gmpc_plugin_browser_iface_info = { (GInterfaceInitFunc) gmpc_metadata_browser_gmpc_plugin_browser_iface_interface_init, (GInterfaceFinalizeFunc) NULL, NULL};
		static const GInterfaceInfo gmpc_plugin_preferences_iface_info = { (GInterfaceInitFunc) gmpc_metadata_browser_gmpc_plugin_preferences_iface_interface_init, (GInterfaceFinalizeFunc) NULL, NULL};
		gmpc_metadata_browser_type_id = g_type_register_static (GMPC_PLUGIN_TYPE_BASE, "GmpcMetadataBrowser", &g_define_type_info, 0);
		g_type_add_interface_static (gmpc_metadata_browser_type_id, GMPC_PLUGIN_TYPE_BROWSER_IFACE, &gmpc_plugin_browser_iface_info);
		g_type_add_interface_static (gmpc_metadata_browser_type_id, GMPC_PLUGIN_TYPE_PREFERENCES_IFACE, &gmpc_plugin_preferences_iface_info);
	}
	return gmpc_metadata_browser_type_id;
}


static void _vala_array_destroy (gpointer array, gint array_length, GDestroyNotify destroy_func) {
	if ((array != NULL) && (destroy_func != NULL)) {
		int i;
		for (i = 0; i < array_length; i = i + 1) {
			if (((gpointer*) array)[i] != NULL) {
				destroy_func (((gpointer*) array)[i]);
			}
		}
	}
}


static void _vala_array_free (gpointer array, gint array_length, GDestroyNotify destroy_func) {
	_vala_array_destroy (array, array_length, destroy_func);
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




