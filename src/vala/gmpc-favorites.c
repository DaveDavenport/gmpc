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
#include <config.h>
#include <gtk/gtk.h>
#include <gtktransition.h>
#include <libmpd/libmpd.h>
#include <glib/gi18n-lib.h>
#include <gmpc-connection.h>
#include <libmpd/libmpdclient.h>
#include <stdlib.h>
#include <string.h>
#include <plugin.h>
#include <main.h>
#include <gdk-pixbuf/gdk-pixdata.h>
#include <gdk/gdk.h>
#include <misc.h>
#include <stdio.h>


#define GMPC_FAVORITES_TYPE_LIST (gmpc_favorites_list_get_type ())
#define GMPC_FAVORITES_LIST(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GMPC_FAVORITES_TYPE_LIST, GmpcFavoritesList))
#define GMPC_FAVORITES_LIST_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GMPC_FAVORITES_TYPE_LIST, GmpcFavoritesListClass))
#define GMPC_FAVORITES_IS_LIST(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GMPC_FAVORITES_TYPE_LIST))
#define GMPC_FAVORITES_IS_LIST_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GMPC_FAVORITES_TYPE_LIST))
#define GMPC_FAVORITES_LIST_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GMPC_FAVORITES_TYPE_LIST, GmpcFavoritesListClass))

typedef struct _GmpcFavoritesList GmpcFavoritesList;
typedef struct _GmpcFavoritesListClass GmpcFavoritesListClass;
typedef struct _GmpcFavoritesListPrivate GmpcFavoritesListPrivate;
#define _mpd_data_free0(var) ((var == NULL) ? NULL : (var = (mpd_data_free (var), NULL)))

#define GMPC_FAVORITES_TYPE_BUTTON (gmpc_favorites_button_get_type ())
#define GMPC_FAVORITES_BUTTON(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GMPC_FAVORITES_TYPE_BUTTON, GmpcFavoritesButton))
#define GMPC_FAVORITES_BUTTON_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GMPC_FAVORITES_TYPE_BUTTON, GmpcFavoritesButtonClass))
#define GMPC_FAVORITES_IS_BUTTON(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GMPC_FAVORITES_TYPE_BUTTON))
#define GMPC_FAVORITES_IS_BUTTON_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GMPC_FAVORITES_TYPE_BUTTON))
#define GMPC_FAVORITES_BUTTON_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GMPC_FAVORITES_TYPE_BUTTON, GmpcFavoritesButtonClass))

typedef struct _GmpcFavoritesButton GmpcFavoritesButton;
typedef struct _GmpcFavoritesButtonClass GmpcFavoritesButtonClass;
typedef struct _GmpcFavoritesButtonPrivate GmpcFavoritesButtonPrivate;
#define _mpd_freeSong0(var) ((var == NULL) ? NULL : (var = (mpd_freeSong (var), NULL)))
#define _g_object_unref0(var) ((var == NULL) ? NULL : (var = (g_object_unref (var), NULL)))
#define _g_error_free0(var) ((var == NULL) ? NULL : (var = (g_error_free (var), NULL)))

struct _GmpcFavoritesList {
	GObject parent_instance;
	GmpcFavoritesListPrivate * priv;
};

struct _GmpcFavoritesListClass {
	GObjectClass parent_class;
};

struct _GmpcFavoritesListPrivate {
	MpdData* list;
};

struct _GmpcFavoritesButton {
	GtkEventBox parent_instance;
	GmpcFavoritesButtonPrivate * priv;
};

struct _GmpcFavoritesButtonClass {
	GtkEventBoxClass parent_class;
};

struct _GmpcFavoritesButtonPrivate {
	mpd_Song* song;
	GtkImage* image;
	gboolean fstate;
	GdkPixbuf* pb;
};


extern GmpcFavoritesList* favorites;
GmpcFavoritesList* favorites = NULL;
static gpointer gmpc_favorites_list_parent_class = NULL;
static gpointer gmpc_favorites_button_parent_class = NULL;

#define some_unique_name_fav VERSION
#define use_transition_fav TRUE
GType gmpc_favorites_list_get_type (void);
#define GMPC_FAVORITES_LIST_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GMPC_FAVORITES_TYPE_LIST, GmpcFavoritesListPrivate))
enum  {
	GMPC_FAVORITES_LIST_DUMMY_PROPERTY
};
static void gmpc_favorites_list_con_changed (GmpcFavoritesList* self, GmpcConnection* conn, MpdObj* server, gint connect);
static void gmpc_favorites_list_status_changed (GmpcFavoritesList* self, GmpcConnection* conn, MpdObj* server, ChangedStatusType what);
gboolean gmpc_favorites_list_is_favorite (GmpcFavoritesList* self, const char* path);
void gmpc_favorites_list_set_favorite (GmpcFavoritesList* self, const char* path, gboolean favorite);
GmpcFavoritesList* gmpc_favorites_list_new (void);
GmpcFavoritesList* gmpc_favorites_list_construct (GType object_type);
static void _gmpc_favorites_list_con_changed_gmpc_connection_connection_changed (GmpcConnection* _sender, MpdObj* server, gint connect, gpointer self);
static void _gmpc_favorites_list_status_changed_gmpc_connection_status_changed (GmpcConnection* _sender, MpdObj* server, ChangedStatusType what, gpointer self);
static GObject * gmpc_favorites_list_constructor (GType type, guint n_construct_properties, GObjectConstructParam * construct_properties);
static void gmpc_favorites_list_finalize (GObject* obj);
GType gmpc_favorites_button_get_type (void);
#define GMPC_FAVORITES_BUTTON_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GMPC_FAVORITES_TYPE_BUTTON, GmpcFavoritesButtonPrivate))
enum  {
	GMPC_FAVORITES_BUTTON_DUMMY_PROPERTY
};
static gboolean gmpc_favorites_button_button_press_event_callback (GmpcFavoritesButton* self, GmpcFavoritesButton* button, const GdkEventButton* event);
static gboolean gmpc_favorites_button_enter_notify_event_callback (GmpcFavoritesButton* self, GmpcFavoritesButton* button, const GdkEventCrossing* motion);
static void gmpc_favorites_button_update (GmpcFavoritesButton* self, GmpcFavoritesList* list);
static gboolean gmpc_favorites_button_leave_notify_event_callback (GmpcFavoritesButton* self, GmpcFavoritesButton* button, const GdkEventCrossing* motion);
void gmpc_favorites_button_set_song (GmpcFavoritesButton* self, const mpd_Song* song);
GmpcFavoritesButton* gmpc_favorites_button_new (void);
GmpcFavoritesButton* gmpc_favorites_button_construct (GType object_type);
static void _gmpc_favorites_button_update_gmpc_favorites_list_updated (GmpcFavoritesList* _sender, gpointer self);
static gboolean _gmpc_favorites_button_button_press_event_callback_gtk_widget_button_press_event (GmpcFavoritesButton* _sender, const GdkEventButton* event, gpointer self);
static gboolean _gmpc_favorites_button_enter_notify_event_callback_gtk_widget_enter_notify_event (GmpcFavoritesButton* _sender, const GdkEventCrossing* event, gpointer self);
static gboolean _gmpc_favorites_button_leave_notify_event_callback_gtk_widget_leave_notify_event (GmpcFavoritesButton* _sender, const GdkEventCrossing* event, gpointer self);
static GObject * gmpc_favorites_button_constructor (GType type, guint n_construct_properties, GObjectConstructParam * construct_properties);
static void gmpc_favorites_button_finalize (GObject* obj);
static int _vala_strcmp0 (const char * str1, const char * str2);



static void gmpc_favorites_list_con_changed (GmpcFavoritesList* self, GmpcConnection* conn, MpdObj* server, gint connect) {
	g_return_if_fail (self != NULL);
	g_return_if_fail (conn != NULL);
	g_return_if_fail (server != NULL);
	if (connect == 1) {
		MpdData* _tmp0_;
		self->priv->list = (_tmp0_ = mpd_database_get_playlist_content (server, _ ("Favorites")), _mpd_data_free0 (self->priv->list), _tmp0_);
		g_signal_emit_by_name (self, "updated");
	} else {
		MpdData* _tmp1_;
		self->priv->list = (_tmp1_ = NULL, _mpd_data_free0 (self->priv->list), _tmp1_);
	}
}


static void gmpc_favorites_list_status_changed (GmpcFavoritesList* self, GmpcConnection* conn, MpdObj* server, ChangedStatusType what) {
	g_return_if_fail (self != NULL);
	g_return_if_fail (conn != NULL);
	g_return_if_fail (server != NULL);
	if ((what & MPD_CST_STORED_PLAYLIST) == MPD_CST_STORED_PLAYLIST) {
		MpdData* _tmp0_;
		self->priv->list = (_tmp0_ = mpd_database_get_playlist_content (server, _ ("Favorites")), _mpd_data_free0 (self->priv->list), _tmp0_);
		g_signal_emit_by_name (self, "updated");
	}
}


gboolean gmpc_favorites_list_is_favorite (GmpcFavoritesList* self, const char* path) {
	gboolean result;
	const MpdData* iter;
	g_return_val_if_fail (self != NULL, FALSE);
	g_return_val_if_fail (path != NULL, FALSE);
	iter = mpd_data_get_first (self->priv->list);
	while (TRUE) {
		if (!(iter != NULL)) {
			break;
		}
		if (iter->type == MPD_DATA_TYPE_SONG) {
			if (_vala_strcmp0 (iter->song->file, path) == 0) {
				result = TRUE;
				return result;
			}
		}
		iter = mpd_data_get_next_real (iter, FALSE);
	}
	result = FALSE;
	return result;
}


void gmpc_favorites_list_set_favorite (GmpcFavoritesList* self, const char* path, gboolean favorite) {
	gboolean current;
	g_return_if_fail (self != NULL);
	g_return_if_fail (path != NULL);
	current = gmpc_favorites_list_is_favorite (self, path);
	if (current != favorite) {
		if (favorite) {
			mpd_database_playlist_list_add (connection, _ ("Favorites"), path);
		} else {
			const MpdData* iter;
			iter = mpd_data_get_first (self->priv->list);
			while (TRUE) {
				if (!(iter != NULL)) {
					break;
				}
				if (iter->type == MPD_DATA_TYPE_SONG) {
					if (_vala_strcmp0 (iter->song->file, path) == 0) {
						mpd_database_playlist_list_delete (connection, _ ("Favorites"), iter->song->pos);
						return;
					}
				}
				iter = mpd_data_get_next_real (iter, FALSE);
			}
		}
	}
}


GmpcFavoritesList* gmpc_favorites_list_construct (GType object_type) {
	GmpcFavoritesList * self;
	self = g_object_newv (object_type, 0, NULL);
	return self;
}


GmpcFavoritesList* gmpc_favorites_list_new (void) {
	return gmpc_favorites_list_construct (GMPC_FAVORITES_TYPE_LIST);
}


static void _gmpc_favorites_list_con_changed_gmpc_connection_connection_changed (GmpcConnection* _sender, MpdObj* server, gint connect, gpointer self) {
	gmpc_favorites_list_con_changed (self, _sender, server, connect);
}


static void _gmpc_favorites_list_status_changed_gmpc_connection_status_changed (GmpcConnection* _sender, MpdObj* server, ChangedStatusType what, gpointer self) {
	gmpc_favorites_list_status_changed (self, _sender, server, what);
}


static GObject * gmpc_favorites_list_constructor (GType type, guint n_construct_properties, GObjectConstructParam * construct_properties) {
	GObject * obj;
	GObjectClass * parent_class;
	GmpcFavoritesList * self;
	parent_class = G_OBJECT_CLASS (gmpc_favorites_list_parent_class);
	obj = parent_class->constructor (type, n_construct_properties, construct_properties);
	self = GMPC_FAVORITES_LIST (obj);
	{
		g_signal_connect_object (gmpcconn, "connection-changed", (GCallback) _gmpc_favorites_list_con_changed_gmpc_connection_connection_changed, self, 0);
		g_signal_connect_object (gmpcconn, "status-changed", (GCallback) _gmpc_favorites_list_status_changed_gmpc_connection_status_changed, self, 0);
	}
	return obj;
}


static void gmpc_favorites_list_class_init (GmpcFavoritesListClass * klass) {
	gmpc_favorites_list_parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (GmpcFavoritesListPrivate));
	G_OBJECT_CLASS (klass)->constructor = gmpc_favorites_list_constructor;
	G_OBJECT_CLASS (klass)->finalize = gmpc_favorites_list_finalize;
	g_signal_new ("updated", GMPC_FAVORITES_TYPE_LIST, G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
}


static void gmpc_favorites_list_instance_init (GmpcFavoritesList * self) {
	self->priv = GMPC_FAVORITES_LIST_GET_PRIVATE (self);
	self->priv->list = NULL;
}


static void gmpc_favorites_list_finalize (GObject* obj) {
	GmpcFavoritesList * self;
	self = GMPC_FAVORITES_LIST (obj);
	_mpd_data_free0 (self->priv->list);
	G_OBJECT_CLASS (gmpc_favorites_list_parent_class)->finalize (obj);
}


GType gmpc_favorites_list_get_type (void) {
	static GType gmpc_favorites_list_type_id = 0;
	if (gmpc_favorites_list_type_id == 0) {
		static const GTypeInfo g_define_type_info = { sizeof (GmpcFavoritesListClass), (GBaseInitFunc) NULL, (GBaseFinalizeFunc) NULL, (GClassInitFunc) gmpc_favorites_list_class_init, (GClassFinalizeFunc) NULL, NULL, sizeof (GmpcFavoritesList), 0, (GInstanceInitFunc) gmpc_favorites_list_instance_init, NULL };
		gmpc_favorites_list_type_id = g_type_register_static (G_TYPE_OBJECT, "GmpcFavoritesList", &g_define_type_info, 0);
	}
	return gmpc_favorites_list_type_id;
}


static gboolean gmpc_favorites_button_button_press_event_callback (GmpcFavoritesButton* self, GmpcFavoritesButton* button, const GdkEventButton* event) {
	gboolean result;
	gboolean _tmp0_;
	g_return_val_if_fail (self != NULL, FALSE);
	g_return_val_if_fail (button != NULL, FALSE);
	_tmp0_ = FALSE;
	if ((*event).button == 1) {
		_tmp0_ = self->priv->song != NULL;
	} else {
		_tmp0_ = FALSE;
	}
	if (_tmp0_) {
		gmpc_favorites_list_set_favorite (favorites, self->priv->song->file, !self->priv->fstate);
		self->priv->fstate = !self->priv->fstate;
	}
	result = FALSE;
	return result;
}


static gboolean gmpc_favorites_button_enter_notify_event_callback (GmpcFavoritesButton* self, GmpcFavoritesButton* button, const GdkEventCrossing* motion) {
	gboolean result;
	GdkPixbuf* pb2;
	g_return_val_if_fail (self != NULL, FALSE);
	g_return_val_if_fail (button != NULL, FALSE);
	pb2 = gdk_pixbuf_copy (self->priv->pb);
	if (self->priv->fstate) {
		colorshift_pixbuf (pb2, self->priv->pb, 10);
	} else {
		colorshift_pixbuf (pb2, self->priv->pb, -50);
	}
	gtk_image_set_from_pixbuf (self->priv->image, pb2);
	result = FALSE;
	_g_object_unref0 (pb2);
	return result;
}


static gboolean gmpc_favorites_button_leave_notify_event_callback (GmpcFavoritesButton* self, GmpcFavoritesButton* button, const GdkEventCrossing* motion) {
	gboolean result;
	g_return_val_if_fail (self != NULL, FALSE);
	g_return_val_if_fail (button != NULL, FALSE);
	gmpc_favorites_button_update (self, favorites);
	result = FALSE;
	return result;
}


static void gmpc_favorites_button_update (GmpcFavoritesButton* self, GmpcFavoritesList* list) {
	GdkPixbuf* pb2;
	g_return_if_fail (self != NULL);
	g_return_if_fail (list != NULL);
	if (self->priv->song != NULL) {
		self->priv->fstate = gmpc_favorites_list_is_favorite (favorites, self->priv->song->file);
	} else {
		gtk_widget_hide ((GtkWidget*) self);
		return;
	}
	pb2 = gdk_pixbuf_copy (self->priv->pb);
	if (self->priv->fstate) {
		colorshift_pixbuf (pb2, self->priv->pb, 30);
	} else {
		colorshift_pixbuf (pb2, self->priv->pb, -80);
	}
	gtk_image_set_from_pixbuf (self->priv->image, pb2);
	gtk_widget_show ((GtkWidget*) self->priv->image);
	gtk_widget_show ((GtkWidget*) self);
	_g_object_unref0 (pb2);
}


static gpointer _mpd_songDup0 (gpointer self) {
	return self ? mpd_songDup (self) : NULL;
}


void gmpc_favorites_button_set_song (GmpcFavoritesButton* self, const mpd_Song* song) {
	gboolean _tmp0_;
	gboolean _tmp1_;
	gboolean _tmp2_;
	mpd_Song* _tmp3_;
	g_return_if_fail (self != NULL);
	_tmp0_ = FALSE;
	if (self->priv->song == NULL) {
		_tmp0_ = song == NULL;
	} else {
		_tmp0_ = FALSE;
	}
	if (_tmp0_) {
		return;
	}
	_tmp1_ = FALSE;
	_tmp2_ = FALSE;
	if (self->priv->song != NULL) {
		_tmp2_ = song != NULL;
	} else {
		_tmp2_ = FALSE;
	}
	if (_tmp2_) {
		_tmp1_ = _vala_strcmp0 (self->priv->song->file, song->file) == 0;
	} else {
		_tmp1_ = FALSE;
	}
	if (_tmp1_) {
		return;
	}
	self->priv->song = (_tmp3_ = _mpd_songDup0 (song), _mpd_freeSong0 (self->priv->song), _tmp3_);
	gmpc_favorites_button_update (self, favorites);
}


GmpcFavoritesButton* gmpc_favorites_button_construct (GType object_type) {
	GmpcFavoritesButton * self;
	self = g_object_newv (object_type, 0, NULL);
	return self;
}


GmpcFavoritesButton* gmpc_favorites_button_new (void) {
	return gmpc_favorites_button_construct (GMPC_FAVORITES_TYPE_BUTTON);
}


static gpointer _g_object_ref0 (gpointer self) {
	return self ? g_object_ref (self) : NULL;
}


static void _gmpc_favorites_button_update_gmpc_favorites_list_updated (GmpcFavoritesList* _sender, gpointer self) {
	gmpc_favorites_button_update (self, _sender);
}


static gboolean _gmpc_favorites_button_button_press_event_callback_gtk_widget_button_press_event (GmpcFavoritesButton* _sender, const GdkEventButton* event, gpointer self) {
	return gmpc_favorites_button_button_press_event_callback (self, _sender, event);
}


static gboolean _gmpc_favorites_button_enter_notify_event_callback_gtk_widget_enter_notify_event (GmpcFavoritesButton* _sender, const GdkEventCrossing* event, gpointer self) {
	return gmpc_favorites_button_enter_notify_event_callback (self, _sender, event);
}


static gboolean _gmpc_favorites_button_leave_notify_event_callback_gtk_widget_leave_notify_event (GmpcFavoritesButton* _sender, const GdkEventCrossing* event, gpointer self) {
	return gmpc_favorites_button_leave_notify_event_callback (self, _sender, event);
}


static GObject * gmpc_favorites_button_constructor (GType type, guint n_construct_properties, GObjectConstructParam * construct_properties) {
	GObject * obj;
	GObjectClass * parent_class;
	GmpcFavoritesButton * self;
	GError * _inner_error_;
	parent_class = G_OBJECT_CLASS (gmpc_favorites_button_parent_class);
	obj = parent_class->constructor (type, n_construct_properties, construct_properties);
	self = GMPC_FAVORITES_BUTTON (obj);
	_inner_error_ = NULL;
	{
		GtkIconTheme* it;
		GtkImage* _tmp3_;
		gtk_widget_set_no_show_all ((GtkWidget*) self, TRUE);
		gtk_event_box_set_visible_window ((GtkEventBox*) self, FALSE);
		it = _g_object_ref0 (gtk_icon_theme_get_default ());
		{
			GdkPixbuf* _tmp0_;
			GdkPixbuf* _tmp1_;
			_tmp0_ = gtk_icon_theme_load_icon (it, "emblem-favorite", 24, 0, &_inner_error_);
			if (_inner_error_ != NULL) {
				goto __catch0_g_error;
				goto __finally0;
			}
			self->priv->pb = (_tmp1_ = _g_object_ref0 (_tmp0_), _g_object_unref0 (self->priv->pb), _tmp1_);
		}
		goto __finally0;
		__catch0_g_error:
		{
			GError * e;
			e = _inner_error_;
			_inner_error_ = NULL;
			{
				fprintf (stdout, "error: %s\n", e->message);
				_g_error_free0 (e);
			}
		}
		__finally0:
		if (_inner_error_ != NULL) {
			_g_object_unref0 (it);
			g_critical ("file %s: line %d: uncaught error: %s", __FILE__, __LINE__, _inner_error_->message);
			g_clear_error (&_inner_error_);
		}
		if (favorites == NULL) {
			GmpcFavoritesList* _tmp2_;
			favorites = (_tmp2_ = gmpc_favorites_list_new (), _g_object_unref0 (favorites), _tmp2_);
		} else {
			g_object_ref ((GObject*) favorites);
		}
		g_signal_connect_object (favorites, "updated", (GCallback) _gmpc_favorites_button_update_gmpc_favorites_list_updated, self, 0);
		self->priv->image = (_tmp3_ = g_object_ref_sink ((GtkImage*) gtk_image_new ()), _g_object_unref0 (self->priv->image), _tmp3_);
		gmpc_favorites_button_update (self, favorites);
		gtk_container_add ((GtkContainer*) self, (GtkWidget*) self->priv->image);
		g_signal_connect_object ((GtkWidget*) self, "button-press-event", (GCallback) _gmpc_favorites_button_button_press_event_callback_gtk_widget_button_press_event, self, 0);
		g_signal_connect_object ((GtkWidget*) self, "enter-notify-event", (GCallback) _gmpc_favorites_button_enter_notify_event_callback_gtk_widget_enter_notify_event, self, 0);
		g_signal_connect_object ((GtkWidget*) self, "leave-notify-event", (GCallback) _gmpc_favorites_button_leave_notify_event_callback_gtk_widget_leave_notify_event, self, 0);
		_g_object_unref0 (it);
	}
	return obj;
}


static void gmpc_favorites_button_class_init (GmpcFavoritesButtonClass * klass) {
	gmpc_favorites_button_parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (GmpcFavoritesButtonPrivate));
	G_OBJECT_CLASS (klass)->constructor = gmpc_favorites_button_constructor;
	G_OBJECT_CLASS (klass)->finalize = gmpc_favorites_button_finalize;
}


static void gmpc_favorites_button_instance_init (GmpcFavoritesButton * self) {
	self->priv = GMPC_FAVORITES_BUTTON_GET_PRIVATE (self);
	self->priv->fstate = FALSE;
	self->priv->pb = NULL;
}


static void gmpc_favorites_button_finalize (GObject* obj) {
	GmpcFavoritesButton * self;
	self = GMPC_FAVORITES_BUTTON (obj);
	{
		if (favorites != NULL) {
			g_object_unref ((GObject*) favorites);
		}
	}
	_mpd_freeSong0 (self->priv->song);
	_g_object_unref0 (self->priv->image);
	_g_object_unref0 (self->priv->pb);
	G_OBJECT_CLASS (gmpc_favorites_button_parent_class)->finalize (obj);
}


GType gmpc_favorites_button_get_type (void) {
	static GType gmpc_favorites_button_type_id = 0;
	if (gmpc_favorites_button_type_id == 0) {
		static const GTypeInfo g_define_type_info = { sizeof (GmpcFavoritesButtonClass), (GBaseInitFunc) NULL, (GBaseFinalizeFunc) NULL, (GClassInitFunc) gmpc_favorites_button_class_init, (GClassFinalizeFunc) NULL, NULL, sizeof (GmpcFavoritesButton), 0, (GInstanceInitFunc) gmpc_favorites_button_instance_init, NULL };
		gmpc_favorites_button_type_id = g_type_register_static (GTK_TYPE_EVENT_BOX, "GmpcFavoritesButton", &g_define_type_info, 0);
	}
	return gmpc_favorites_button_type_id;
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




