
#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include <libmpd/libmpd.h>
#include <libmpd/libmpdclient.h>
#include <gtktransition.h>
#include <gdk/gdk.h>
#include <gmpc-connection.h>
#include <main.h>
#include <stdlib.h>
#include <string.h>


#define GMPC_TYPE_RATING (gmpc_rating_get_type ())
#define GMPC_RATING(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GMPC_TYPE_RATING, GmpcRating))
#define GMPC_RATING_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GMPC_TYPE_RATING, GmpcRatingClass))
#define GMPC_IS_RATING(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GMPC_TYPE_RATING))
#define GMPC_IS_RATING_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GMPC_TYPE_RATING))
#define GMPC_RATING_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GMPC_TYPE_RATING, GmpcRatingClass))

typedef struct _GmpcRating GmpcRating;
typedef struct _GmpcRatingClass GmpcRatingClass;
typedef struct _GmpcRatingPrivate GmpcRatingPrivate;

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
struct _GmpcRating {
	GtkFrame parent_instance;
	GmpcRatingPrivate * priv;
	GtkEventBox* event_box;
};

struct _GmpcRatingClass {
	GtkFrameClass parent_class;
};

struct _GmpcRatingPrivate {
	MpdObj* server;
	mpd_Song* song;
	GtkImage** rat;
	gint rat_length1;
	gint rat_size;
	GtkHBox* box;
	gint rating;
	gulong status_changed_id;
};



GType gmpc_rating_get_type (void);
#define GMPC_RATING_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GMPC_TYPE_RATING, GmpcRatingPrivate))
enum  {
	GMPC_RATING_DUMMY_PROPERTY
};
static gint gmpc_rating_id;
static gint gmpc_rating_id = 0;
#define GMPC_RATING_use_transition TRUE
void gmpc_rating_set_rating (GmpcRating* self, gint rating);
gboolean gmpc_rating_button_press_event_callback (GmpcRating* self, GtkEventBox* wid, const GdkEventButton* event);
void gmpc_rating_update (GmpcRating* self);
static void gmpc_rating_status_changed (GmpcRating* self, MpdObj* server, ChangedStatusType what, GmpcConnection* conn);
GmpcRating* gmpc_rating_new (MpdObj* server, const mpd_Song* song);
GmpcRating* gmpc_rating_construct (GType object_type, MpdObj* server, const mpd_Song* song);
GmpcRating* gmpc_rating_new (MpdObj* server, const mpd_Song* song);
static gboolean _gmpc_rating_button_press_event_callback_gtk_widget_button_press_event (GtkEventBox* _sender, const GdkEventButton* event, gpointer self);
static GObject * gmpc_rating_constructor (GType type, guint n_construct_properties, GObjectConstructParam * construct_properties);
static gpointer gmpc_rating_parent_class = NULL;
static void gmpc_rating_finalize (GObject* obj);
static void _vala_array_destroy (gpointer array, gint array_length, GDestroyNotify destroy_func);
static void _vala_array_free (gpointer array, gint array_length, GDestroyNotify destroy_func);



gboolean gmpc_rating_button_press_event_callback (GmpcRating* self, GtkEventBox* wid, const GdkEventButton* event) {
	g_return_val_if_fail (self != NULL, FALSE);
	g_return_val_if_fail (wid != NULL, FALSE);
	if ((*event).type == GDK_BUTTON_PRESS) {
		if ((*event).button == 1) {
			gint width;
			gint button;
			char* _tmp0_;
			width = ((GtkWidget*) self)->allocation.width;
			button = (gint) ((((*event).x / ((double) width)) + 0.15) * 5);
			_tmp0_ = NULL;
			mpd_sticker_song_set (self->priv->server, self->priv->song->file, "rating", _tmp0_ = g_strdup_printf ("%i", button));
			_tmp0_ = (g_free (_tmp0_), NULL);
			gmpc_rating_set_rating (self, button);
		}
	}
	return FALSE;
}


static void gmpc_rating_status_changed (GmpcRating* self, MpdObj* server, ChangedStatusType what, GmpcConnection* conn) {
	g_return_if_fail (self != NULL);
	g_return_if_fail (server != NULL);
	g_return_if_fail (conn != NULL);
	if ((what & MPD_CST_STICKER) != 0) {
		gmpc_rating_update (self);
	}
}


GmpcRating* gmpc_rating_construct (GType object_type, MpdObj* server, const mpd_Song* song) {
	GmpcRating * self;
	mpd_Song* _tmp1_;
	const mpd_Song* _tmp0_;
	g_return_val_if_fail (server != NULL, NULL);
	g_return_val_if_fail (song != NULL, NULL);
	self = g_object_newv (object_type, 0, NULL);
	self->priv->server = server;
	_tmp1_ = NULL;
	_tmp0_ = NULL;
	self->priv->song = (_tmp1_ = (_tmp0_ = song, (_tmp0_ == NULL) ? NULL : mpd_songDup (_tmp0_)), (self->priv->song == NULL) ? NULL : (self->priv->song = (mpd_freeSong (self->priv->song), NULL)), _tmp1_);
	gmpc_rating_update (self);
	self->priv->status_changed_id = g_signal_connect_swapped (gmpcconn, "status_changed", (GCallback) gmpc_rating_status_changed, self);
	return self;
}


GmpcRating* gmpc_rating_new (MpdObj* server, const mpd_Song* song) {
	return gmpc_rating_construct (GMPC_TYPE_RATING, server, song);
}


void gmpc_rating_set_rating (GmpcRating* self, gint rating) {
	gint i;
	g_return_if_fail (self != NULL);
	i = 0;
	if (rating != self->priv->rating) {
		for (i = 0; i < 5; i++) {
			g_object_set ((GtkWidget*) self->priv->rat[i], "sensitive", i < rating, NULL);
		}
		self->priv->rating = rating;
	}
}


void gmpc_rating_update (GmpcRating* self) {
	char* value;
	g_return_if_fail (self != NULL);
	value = mpd_sticker_song_get (self->priv->server, self->priv->song->file, "rating");
	if (value == NULL) {
		gmpc_rating_set_rating (self, 0);
	} else {
		gmpc_rating_set_rating (self, atoi (value));
	}
	value = (g_free (value), NULL);
}


static gboolean _gmpc_rating_button_press_event_callback_gtk_widget_button_press_event (GtkEventBox* _sender, const GdkEventButton* event, gpointer self) {
	return gmpc_rating_button_press_event_callback (self, _sender, event);
}


static GObject * gmpc_rating_constructor (GType type, guint n_construct_properties, GObjectConstructParam * construct_properties) {
	GObject * obj;
	GmpcRatingClass * klass;
	GObjectClass * parent_class;
	GmpcRating * self;
	klass = GMPC_RATING_CLASS (g_type_class_peek (GMPC_TYPE_RATING));
	parent_class = G_OBJECT_CLASS (g_type_class_peek_parent (klass));
	obj = parent_class->constructor (type, n_construct_properties, construct_properties);
	self = GMPC_RATING (obj);
	{
		gint i;
		GtkHBox* _tmp0_;
		GtkEventBox* _tmp1_;
		GtkImage** _tmp2_;
		i = 0;
		g_object_set ((GtkFrame*) self, "shadow", GTK_SHADOW_NONE, NULL);
		_tmp0_ = NULL;
		self->priv->box = (_tmp0_ = g_object_ref_sink ((GtkHBox*) gtk_hbox_new (TRUE, 6)), (self->priv->box == NULL) ? NULL : (self->priv->box = (g_object_unref (self->priv->box), NULL)), _tmp0_);
		_tmp1_ = NULL;
		self->event_box = (_tmp1_ = g_object_ref_sink ((GtkEventBox*) gtk_event_box_new ()), (self->event_box == NULL) ? NULL : (self->event_box = (g_object_unref (self->event_box), NULL)), _tmp1_);
		gtk_event_box_set_visible_window (self->event_box, FALSE);
		_tmp2_ = NULL;
		self->priv->rat = (_tmp2_ = g_new0 (GtkImage*, 5 + 1), self->priv->rat = (_vala_array_free (self->priv->rat, self->priv->rat_length1, (GDestroyNotify) g_object_unref), NULL), self->priv->rat_length1 = 5, self->priv->rat_size = self->priv->rat_length1, _tmp2_);
		for (i = 0; i < 5; i++) {
			GtkImage* _tmp3_;
			_tmp3_ = NULL;
			self->priv->rat[i] = (_tmp3_ = g_object_ref_sink ((GtkImage*) gtk_image_new_from_icon_name ("emblem-favorite", GTK_ICON_SIZE_MENU)), (self->priv->rat[i] == NULL) ? NULL : (self->priv->rat[i] = (g_object_unref (self->priv->rat[i]), NULL)), _tmp3_);
			gtk_box_pack_start ((GtkBox*) self->priv->box, (GtkWidget*) self->priv->rat[i], FALSE, FALSE, (guint) 0);
		}
		gtk_container_add ((GtkContainer*) self, (GtkWidget*) self->event_box);
		gtk_container_add ((GtkContainer*) self->event_box, (GtkWidget*) self->priv->box);
		g_signal_connect_object ((GtkWidget*) self->event_box, "button-press-event", (GCallback) _gmpc_rating_button_press_event_callback_gtk_widget_button_press_event, self, 0);
		gtk_widget_show_all ((GtkWidget*) self);
	}
	return obj;
}


static void gmpc_rating_class_init (GmpcRatingClass * klass) {
	gmpc_rating_parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (GmpcRatingPrivate));
	G_OBJECT_CLASS (klass)->constructor = gmpc_rating_constructor;
	G_OBJECT_CLASS (klass)->finalize = gmpc_rating_finalize;
	gmpc_rating_id = gmpc_rating_id + 1;
}


static void gmpc_rating_instance_init (GmpcRating * self) {
	self->priv = GMPC_RATING_GET_PRIVATE (self);
	self->priv->server = NULL;
	self->priv->song = NULL;
	self->priv->rating = -1;
	self->priv->status_changed_id = (gulong) 0;
}


static void gmpc_rating_finalize (GObject* obj) {
	GmpcRating * self;
	self = GMPC_RATING (obj);
	{
		gboolean _tmp4_;
		_tmp4_ = FALSE;
		if (self->priv->status_changed_id > 0) {
			_tmp4_ = g_signal_handler_is_connected (gmpcconn, self->priv->status_changed_id);
		} else {
			_tmp4_ = FALSE;
		}
		if (_tmp4_) {
			g_signal_handler_disconnect (gmpcconn, self->priv->status_changed_id);
			self->priv->status_changed_id = (gulong) 0;
		}
	}
	(self->priv->song == NULL) ? NULL : (self->priv->song = (mpd_freeSong (self->priv->song), NULL));
	self->priv->rat = (_vala_array_free (self->priv->rat, self->priv->rat_length1, (GDestroyNotify) g_object_unref), NULL);
	(self->priv->box == NULL) ? NULL : (self->priv->box = (g_object_unref (self->priv->box), NULL));
	(self->event_box == NULL) ? NULL : (self->event_box = (g_object_unref (self->event_box), NULL));
	G_OBJECT_CLASS (gmpc_rating_parent_class)->finalize (obj);
}


GType gmpc_rating_get_type (void) {
	static GType gmpc_rating_type_id = 0;
	if (gmpc_rating_type_id == 0) {
		static const GTypeInfo g_define_type_info = { sizeof (GmpcRatingClass), (GBaseInitFunc) NULL, (GBaseFinalizeFunc) NULL, (GClassInitFunc) gmpc_rating_class_init, (GClassFinalizeFunc) NULL, NULL, sizeof (GmpcRating), 0, (GInstanceInitFunc) gmpc_rating_instance_init, NULL };
		gmpc_rating_type_id = g_type_register_static (GTK_TYPE_FRAME, "GmpcRating", &g_define_type_info, 0);
	}
	return gmpc_rating_type_id;
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




