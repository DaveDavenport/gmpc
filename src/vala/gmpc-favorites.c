
#include "gmpc-favorites.h"
#include <config.h>
#include <libmpd/libmpd.h>
#include <gmpc-connection.h>
#include <stdio.h>
#include <glib/gi18n-lib.h>
#include <main.h>
#include <gdk-pixbuf/gdk-pixdata.h>
#include <gdk/gdk.h>




GmpcFavoritesList* favorites = NULL;
struct _GmpcFavoritesListPrivate {
	MpdData* list;
};

#define GMPC_FAVORITES_LIST_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GMPC_FAVORITES_TYPE_LIST, GmpcFavoritesListPrivate))
enum  {
	GMPC_FAVORITES_LIST_DUMMY_PROPERTY
};
static void gmpc_favorites_list_con_changed (GmpcFavoritesList* self, GmpcConnection* conn, MpdObj* server, gint connect);
static void gmpc_favorites_list_status_changed (GmpcFavoritesList* self, GmpcConnection* conn, MpdObj* server, ChangedStatusType what);
static void _gmpc_favorites_list_con_changed_gmpc_connection_connection_changed (GmpcConnection* _sender, MpdObj* server, gint connect, gpointer self);
static void _gmpc_favorites_list_status_changed_gmpc_connection_status_changed (GmpcConnection* _sender, MpdObj* server, ChangedStatusType what, gpointer self);
static GObject * gmpc_favorites_list_constructor (GType type, guint n_construct_properties, GObjectConstructParam * construct_properties);
static gpointer gmpc_favorites_list_parent_class = NULL;
static void gmpc_favorites_list_finalize (GObject* obj);
struct _GmpcFavoritesButtonPrivate {
	mpd_Song* song;
	GtkImage* image;
	gboolean state;
	GdkPixbuf* pb;
};

#define GMPC_FAVORITES_BUTTON_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GMPC_FAVORITES_TYPE_BUTTON, GmpcFavoritesButtonPrivate))
enum  {
	GMPC_FAVORITES_BUTTON_DUMMY_PROPERTY
};
static gboolean gmpc_favorites_button_button_press_event_callback (GmpcFavoritesButton* self, GmpcFavoritesButton* button, const GdkEventButton* event);
static gboolean gmpc_favorites_button_enter_notify_event_callback (GmpcFavoritesButton* self, GmpcFavoritesButton* button, const GdkEventCrossing* motion);
static gboolean gmpc_favorites_button_leave_notify_event_callback (GmpcFavoritesButton* self, GmpcFavoritesButton* button, const GdkEventCrossing* motion);
static void gmpc_favorites_button_update (GmpcFavoritesButton* self, GmpcFavoritesList* list);
static void _gmpc_favorites_button_update_gmpc_favorites_list_updated (GmpcFavoritesList* _sender, gpointer self);
static gboolean _gmpc_favorites_button_button_press_event_callback_gtk_widget_button_press_event (GmpcFavoritesButton* _sender, const GdkEventButton* event, gpointer self);
static gboolean _gmpc_favorites_button_enter_notify_event_callback_gtk_widget_enter_notify_event (GmpcFavoritesButton* _sender, const GdkEventCrossing* event, gpointer self);
static gboolean _gmpc_favorites_button_leave_notify_event_callback_gtk_widget_leave_notify_event (GmpcFavoritesButton* _sender, const GdkEventCrossing* event, gpointer self);
static GObject * gmpc_favorites_button_constructor (GType type, guint n_construct_properties, GObjectConstructParam * construct_properties);
static gpointer gmpc_favorites_button_parent_class = NULL;
static void gmpc_favorites_button_finalize (GObject* obj);
static int _vala_strcmp0 (const char * str1, const char * str2);



static void gmpc_favorites_list_con_changed (GmpcFavoritesList* self, GmpcConnection* conn, MpdObj* server, gint connect) {
	g_return_if_fail (self != NULL);
	g_return_if_fail (conn != NULL);
	g_return_if_fail (server != NULL);
	if (connect == 1) {
		MpdData* _tmp0;
		fprintf (stdout, "fill list\n");
		_tmp0 = NULL;
		self->priv->list = (_tmp0 = mpd_database_get_playlist_content (server, _ ("Favorites")), (self->priv->list == NULL) ? NULL : (self->priv->list = (mpd_data_free (self->priv->list), NULL)), _tmp0);
		g_signal_emit_by_name (self, "updated");
	} else {
		MpdData* _tmp1;
		_tmp1 = NULL;
		self->priv->list = (_tmp1 = NULL, (self->priv->list == NULL) ? NULL : (self->priv->list = (mpd_data_free (self->priv->list), NULL)), _tmp1);
	}
}


static void gmpc_favorites_list_status_changed (GmpcFavoritesList* self, GmpcConnection* conn, MpdObj* server, ChangedStatusType what) {
	g_return_if_fail (self != NULL);
	g_return_if_fail (conn != NULL);
	g_return_if_fail (server != NULL);
	if ((what & MPD_CST_STORED_PLAYLIST) == MPD_CST_STORED_PLAYLIST) {
		MpdData* _tmp0;
		_tmp0 = NULL;
		self->priv->list = (_tmp0 = mpd_database_get_playlist_content (server, _ ("Favorites")), (self->priv->list == NULL) ? NULL : (self->priv->list = (mpd_data_free (self->priv->list), NULL)), _tmp0);
		g_signal_emit_by_name (self, "updated");
	}
}


/* Public access functions */
gboolean gmpc_favorites_list_is_favorite (GmpcFavoritesList* self, const char* path) {
	const MpdData* iter;
	g_return_val_if_fail (self != NULL, FALSE);
	g_return_val_if_fail (path != NULL, FALSE);
	iter = mpd_data_get_first (self->priv->list);
	while (iter != NULL) {
		if (iter->type == MPD_DATA_TYPE_SONG) {
			if (_vala_strcmp0 (iter->song->file, path) == 0) {
				return TRUE;
			}
		}
		iter = mpd_data_get_next_real (iter, FALSE);
	}
	return FALSE;
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
			while (iter != NULL) {
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
	GmpcFavoritesListClass * klass;
	GObjectClass * parent_class;
	GmpcFavoritesList * self;
	klass = GMPC_FAVORITES_LIST_CLASS (g_type_class_peek (GMPC_FAVORITES_TYPE_LIST));
	parent_class = G_OBJECT_CLASS (g_type_class_peek_parent (klass));
	obj = parent_class->constructor (type, n_construct_properties, construct_properties);
	self = GMPC_FAVORITES_LIST (obj);
	{
		fprintf (stdout, "Create list\n");
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
	{
		fprintf (stdout, "Destroy\n");
	}
	(self->priv->list == NULL) ? NULL : (self->priv->list = (mpd_data_free (self->priv->list), NULL));
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
	gboolean _tmp0;
	g_return_val_if_fail (self != NULL, FALSE);
	g_return_val_if_fail (button != NULL, FALSE);
	_tmp0 = FALSE;
	if ((*event).button == 1) {
		_tmp0 = self->priv->song != NULL;
	} else {
		_tmp0 = FALSE;
	}
	if (_tmp0) {
		gmpc_favorites_list_set_favorite (favorites, self->priv->song->file, !self->priv->state);
		self->priv->state = !self->priv->state;
	}
	return FALSE;
}


/* on mouse over, do some pre-highlighting */
static gboolean gmpc_favorites_button_enter_notify_event_callback (GmpcFavoritesButton* self, GmpcFavoritesButton* button, const GdkEventCrossing* motion) {
	GdkPixbuf* _tmp0;
	GdkPixbuf* pb2;
	gboolean _tmp1;
	g_return_val_if_fail (self != NULL, FALSE);
	g_return_val_if_fail (button != NULL, FALSE);
	_tmp0 = NULL;
	pb2 = (_tmp0 = gdk_pixbuf_copy (self->priv->pb), (_tmp0 == NULL) ? NULL : g_object_ref (_tmp0));
	if (self->priv->state) {
		colorshift_pixbuf (pb2, self->priv->pb, 10);
	} else {
		colorshift_pixbuf (pb2, self->priv->pb, -50);
	}
	gtk_image_set_from_pixbuf (self->priv->image, pb2);
	return (_tmp1 = FALSE, (pb2 == NULL) ? NULL : (pb2 = (g_object_unref (pb2), NULL)), _tmp1);
}


/* Reset default highlighting */
static gboolean gmpc_favorites_button_leave_notify_event_callback (GmpcFavoritesButton* self, GmpcFavoritesButton* button, const GdkEventCrossing* motion) {
	g_return_val_if_fail (self != NULL, FALSE);
	g_return_val_if_fail (button != NULL, FALSE);
	gmpc_favorites_button_update (self, favorites);
	return FALSE;
}


/* Update the icon according to state */
static void gmpc_favorites_button_update (GmpcFavoritesButton* self, GmpcFavoritesList* list) {
	GdkPixbuf* _tmp0;
	GdkPixbuf* pb2;
	g_return_if_fail (self != NULL);
	g_return_if_fail (list != NULL);
	if (self->priv->song != NULL) {
		self->priv->state = gmpc_favorites_list_is_favorite (favorites, self->priv->song->file);
	} else {
		self->priv->state = FALSE;
	}
	_tmp0 = NULL;
	pb2 = (_tmp0 = gdk_pixbuf_copy (self->priv->pb), (_tmp0 == NULL) ? NULL : g_object_ref (_tmp0));
	if (self->priv->state) {
		colorshift_pixbuf (pb2, self->priv->pb, 30);
	} else {
		colorshift_pixbuf (pb2, self->priv->pb, -80);
	}
	gtk_image_set_from_pixbuf (self->priv->image, pb2);
	gtk_widget_show ((GtkWidget*) self);
	(pb2 == NULL) ? NULL : (pb2 = (g_object_unref (pb2), NULL));
}


void gmpc_favorites_button_set_song (GmpcFavoritesButton* self, const mpd_Song* song) {
	gboolean _tmp0;
	gboolean _tmp1;
	gboolean _tmp2;
	mpd_Song* _tmp4;
	const mpd_Song* _tmp3;
	g_return_if_fail (self != NULL);
	_tmp0 = FALSE;
	if (self->priv->song == NULL) {
		_tmp0 = song == NULL;
	} else {
		_tmp0 = FALSE;
	}
	if (_tmp0) {
		return;
	}
	_tmp1 = FALSE;
	_tmp2 = FALSE;
	if (self->priv->song != NULL) {
		_tmp2 = song != NULL;
	} else {
		_tmp2 = FALSE;
	}
	if (_tmp2) {
		_tmp1 = _vala_strcmp0 (self->priv->song->file, song->file) == 0;
	} else {
		_tmp1 = FALSE;
	}
	if (_tmp1) {
		return;
	}
	_tmp4 = NULL;
	_tmp3 = NULL;
	self->priv->song = (_tmp4 = (_tmp3 = song, (_tmp3 == NULL) ? NULL : mpd_songDup (_tmp3)), (self->priv->song == NULL) ? NULL : (self->priv->song = (mpd_freeSong (self->priv->song), NULL)), _tmp4);
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
	GmpcFavoritesButtonClass * klass;
	GObjectClass * parent_class;
	GmpcFavoritesButton * self;
	GError * inner_error;
	klass = GMPC_FAVORITES_BUTTON_CLASS (g_type_class_peek (GMPC_FAVORITES_TYPE_BUTTON));
	parent_class = G_OBJECT_CLASS (g_type_class_peek_parent (klass));
	obj = parent_class->constructor (type, n_construct_properties, construct_properties);
	self = GMPC_FAVORITES_BUTTON (obj);
	inner_error = NULL;
	{
		GtkIconTheme* _tmp0;
		GtkIconTheme* it;
		GtkImage* _tmp5;
		gtk_event_box_set_visible_window ((GtkEventBox*) self, FALSE);
		_tmp0 = NULL;
		it = (_tmp0 = gtk_icon_theme_get_default (), (_tmp0 == NULL) ? NULL : g_object_ref (_tmp0));
		{
			GdkPixbuf* _tmp1;
			GdkPixbuf* _tmp3;
			GdkPixbuf* _tmp2;
			_tmp1 = gtk_icon_theme_load_icon (it, "emblem-favorite", 24, 0, &inner_error);
			if (inner_error != NULL) {
				goto __catch0_g_error;
				goto __finally0;
			}
			_tmp3 = NULL;
			_tmp2 = NULL;
			self->priv->pb = (_tmp3 = (_tmp2 = _tmp1, (_tmp2 == NULL) ? NULL : g_object_ref (_tmp2)), (self->priv->pb == NULL) ? NULL : (self->priv->pb = (g_object_unref (self->priv->pb), NULL)), _tmp3);
		}
		goto __finally0;
		__catch0_g_error:
		{
			GError * e;
			e = inner_error;
			inner_error = NULL;
			{
				fprintf (stdout, "error: %s\n", e->message);
				(e == NULL) ? NULL : (e = (g_error_free (e), NULL));
			}
		}
		__finally0:
		if (inner_error != NULL) {
			(it == NULL) ? NULL : (it = (g_object_unref (it), NULL));
			g_critical ("file %s: line %d: uncaught error: %s", __FILE__, __LINE__, inner_error->message);
			g_clear_error (&inner_error);
		}
		if (favorites == NULL) {
			GmpcFavoritesList* _tmp4;
			_tmp4 = NULL;
			favorites = (_tmp4 = gmpc_favorites_list_new (), (favorites == NULL) ? NULL : (favorites = (g_object_unref (favorites), NULL)), _tmp4);
		} else {
			g_object_ref ((GObject*) favorites);
		}
		g_signal_connect_object (favorites, "updated", (GCallback) _gmpc_favorites_button_update_gmpc_favorites_list_updated, self, 0);
		_tmp5 = NULL;
		self->priv->image = (_tmp5 = g_object_ref_sink ((GtkImage*) gtk_image_new ()), (self->priv->image == NULL) ? NULL : (self->priv->image = (g_object_unref (self->priv->image), NULL)), _tmp5);
		gmpc_favorites_button_update (self, favorites);
		gtk_container_add ((GtkContainer*) self, (GtkWidget*) self->priv->image);
		g_signal_connect_object ((GtkWidget*) self, "button-press-event", (GCallback) _gmpc_favorites_button_button_press_event_callback_gtk_widget_button_press_event, self, 0);
		g_signal_connect_object ((GtkWidget*) self, "enter-notify-event", (GCallback) _gmpc_favorites_button_enter_notify_event_callback_gtk_widget_enter_notify_event, self, 0);
		g_signal_connect_object ((GtkWidget*) self, "leave-notify-event", (GCallback) _gmpc_favorites_button_leave_notify_event_callback_gtk_widget_leave_notify_event, self, 0);
		(it == NULL) ? NULL : (it = (g_object_unref (it), NULL));
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
	self->priv->state = FALSE;
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
	(self->priv->song == NULL) ? NULL : (self->priv->song = (mpd_freeSong (self->priv->song), NULL));
	(self->priv->image == NULL) ? NULL : (self->priv->image = (g_object_unref (self->priv->image), NULL));
	(self->priv->pb == NULL) ? NULL : (self->priv->pb = (g_object_unref (self->priv->pb), NULL));
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




