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

#ifndef __GMPC_METADATA_BROWSER2_H__
#define __GMPC_METADATA_BROWSER2_H__

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include <gmpc-plugin.h>
#include <stdlib.h>
#include <string.h>
#include <libmpd/libmpdclient.h>

G_BEGIN_DECLS


#define GMPC_WIDGET_TYPE_SIMILAR_ARTIST (gmpc_widget_similar_artist_get_type ())
#define GMPC_WIDGET_SIMILAR_ARTIST(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GMPC_WIDGET_TYPE_SIMILAR_ARTIST, GmpcWidgetSimilarArtist))
#define GMPC_WIDGET_SIMILAR_ARTIST_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GMPC_WIDGET_TYPE_SIMILAR_ARTIST, GmpcWidgetSimilarArtistClass))
#define GMPC_WIDGET_IS_SIMILAR_ARTIST(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GMPC_WIDGET_TYPE_SIMILAR_ARTIST))
#define GMPC_WIDGET_IS_SIMILAR_ARTIST_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GMPC_WIDGET_TYPE_SIMILAR_ARTIST))
#define GMPC_WIDGET_SIMILAR_ARTIST_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GMPC_WIDGET_TYPE_SIMILAR_ARTIST, GmpcWidgetSimilarArtistClass))

typedef struct _GmpcWidgetSimilarArtist GmpcWidgetSimilarArtist;
typedef struct _GmpcWidgetSimilarArtistClass GmpcWidgetSimilarArtistClass;
typedef struct _GmpcWidgetSimilarArtistPrivate GmpcWidgetSimilarArtistPrivate;

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

#define GMPC_TYPE_METADATA_BROWSER (gmpc_metadata_browser_get_type ())
#define GMPC_METADATA_BROWSER(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GMPC_TYPE_METADATA_BROWSER, GmpcMetadataBrowser))
#define GMPC_METADATA_BROWSER_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GMPC_TYPE_METADATA_BROWSER, GmpcMetadataBrowserClass))
#define GMPC_IS_METADATA_BROWSER(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GMPC_TYPE_METADATA_BROWSER))
#define GMPC_IS_METADATA_BROWSER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GMPC_TYPE_METADATA_BROWSER))
#define GMPC_METADATA_BROWSER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GMPC_TYPE_METADATA_BROWSER, GmpcMetadataBrowserClass))

typedef struct _GmpcMetadataBrowser GmpcMetadataBrowser;
typedef struct _GmpcMetadataBrowserClass GmpcMetadataBrowserClass;
typedef struct _GmpcMetadataBrowserPrivate GmpcMetadataBrowserPrivate;

struct _GmpcWidgetSimilarArtist {
	GtkTable parent_instance;
	GmpcWidgetSimilarArtistPrivate * priv;
};

struct _GmpcWidgetSimilarArtistClass {
	GtkTableClass parent_class;
};

struct _GmpcWidgetMore {
	GtkFrame parent_instance;
	GmpcWidgetMorePrivate * priv;
};

struct _GmpcWidgetMoreClass {
	GtkFrameClass parent_class;
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

struct _GmpcMetadataBrowser {
	GmpcPluginBase parent_instance;
	GmpcMetadataBrowserPrivate * priv;
};

struct _GmpcMetadataBrowserClass {
	GmpcPluginBaseClass parent_class;
};


#define use_transition_mb TRUE
#define some_unique_name_mb VERSION
GtkWidget* gmpc_widget_similar_artist_new_artist_button (GmpcWidgetSimilarArtist* self, const char* artist, gboolean in_db);
GType gmpc_widget_similar_artist_get_type (void);
GType gmpc_widget_more_get_type (void);
GmpcNowPlaying* gmpc_now_playing_construct (GType object_type);
GmpcNowPlaying* gmpc_now_playing_new (void);
GType gmpc_now_playing_get_type (void);
GtkWidget* gmpc_metadata_browser_metadata_box_show_song (GmpcMetadataBrowser* self, const mpd_Song* song);
void gmpc_metadata_browser_set_artist (GmpcMetadataBrowser* self, const char* artist);
void gmpc_metadata_browser_set_album (GmpcMetadataBrowser* self, const char* artist, const char* album);
void gmpc_metadata_browser_set_song (GmpcMetadataBrowser* self, const mpd_Song* song);
void gmpc_metadata_browser_select_browser (GmpcMetadataBrowser* self, GtkTreeView* tree);
GmpcMetadataBrowser* gmpc_metadata_browser_construct (GType object_type);
GmpcMetadataBrowser* gmpc_metadata_browser_new (void);
GType gmpc_metadata_browser_get_type (void);

static const gint GMPC_NOW_PLAYING_version[] = {0, 0, 0};
static const gint GMPC_METADATA_BROWSER_version[] = {0, 0, 0};

G_END_DECLS

#endif
