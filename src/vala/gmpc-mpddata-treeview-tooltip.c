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
#include <gmpc-metaimage.h>
#include <metadata.h>
#include <stdlib.h>
#include <string.h>
#include <libmpd/libmpdclient.h>
#include <libmpd/libmpd.h>
#include <misc.h>
#include <stdio.h>


#define GMPC_MPD_DATA_TREEVIEW_TYPE_TOOLTIP (gmpc_mpd_data_treeview_tooltip_get_type ())
#define GMPC_MPD_DATA_TREEVIEW_TOOLTIP(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GMPC_MPD_DATA_TREEVIEW_TYPE_TOOLTIP, GmpcMpdDataTreeviewTooltip))
#define GMPC_MPD_DATA_TREEVIEW_TOOLTIP_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GMPC_MPD_DATA_TREEVIEW_TYPE_TOOLTIP, GmpcMpdDataTreeviewTooltipClass))
#define GMPC_MPD_DATA_TREEVIEW_IS_TOOLTIP(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GMPC_MPD_DATA_TREEVIEW_TYPE_TOOLTIP))
#define GMPC_MPD_DATA_TREEVIEW_IS_TOOLTIP_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GMPC_MPD_DATA_TREEVIEW_TYPE_TOOLTIP))
#define GMPC_MPD_DATA_TREEVIEW_TOOLTIP_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GMPC_MPD_DATA_TREEVIEW_TYPE_TOOLTIP, GmpcMpdDataTreeviewTooltipClass))

typedef struct _GmpcMpdDataTreeviewTooltip GmpcMpdDataTreeviewTooltip;
typedef struct _GmpcMpdDataTreeviewTooltipClass GmpcMpdDataTreeviewTooltipClass;
typedef struct _GmpcMpdDataTreeviewTooltipPrivate GmpcMpdDataTreeviewTooltipPrivate;

struct _GmpcMpdDataTreeviewTooltip {
	GtkWindow parent_instance;
	GmpcMpdDataTreeviewTooltipPrivate * priv;
};

struct _GmpcMpdDataTreeviewTooltipClass {
	GtkWindowClass parent_class;
};

struct _GmpcMpdDataTreeviewTooltipPrivate {
	GtkTreeView* par_widget;
	GmpcMetaImage* image;
	MetaDataType mtype;
	char* checksum;
};



#define use_transition_mtt TRUE
#define some_unique_name_mtt VERSION
GType gmpc_mpd_data_treeview_tooltip_get_type (void);
#define GMPC_MPD_DATA_TREEVIEW_TOOLTIP_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GMPC_MPD_DATA_TREEVIEW_TYPE_TOOLTIP, GmpcMpdDataTreeviewTooltipPrivate))
enum  {
	GMPC_MPD_DATA_TREEVIEW_TOOLTIP_DUMMY_PROPERTY
};
static gboolean gmpc_mpd_data_treeview_tooltip_query_tooltip_callback (GmpcMpdDataTreeviewTooltip* self, gint x, gint y, gboolean keyboard_tip, GtkTooltip* tooltip);
static gboolean _gmpc_mpd_data_treeview_tooltip_query_tooltip_callback_gtk_widget_query_tooltip (GtkWidget* _sender, gint x, gint y, gboolean keyboard_tooltip, GtkTooltip* tooltip, gpointer self);
GmpcMpdDataTreeviewTooltip* gmpc_mpd_data_treeview_tooltip_new (GtkTreeView* pw, MetaDataType type);
GmpcMpdDataTreeviewTooltip* gmpc_mpd_data_treeview_tooltip_construct (GType object_type, GtkTreeView* pw, MetaDataType type);
GmpcMpdDataTreeviewTooltip* gmpc_mpd_data_treeview_tooltip_new (GtkTreeView* pw, MetaDataType type);
static gpointer gmpc_mpd_data_treeview_tooltip_parent_class = NULL;
static void gmpc_mpd_data_treeview_tooltip_finalize (GObject* obj);
static int _vala_strcmp0 (const char * str1, const char * str2);



static gboolean gmpc_mpd_data_treeview_tooltip_query_tooltip_callback (GmpcMpdDataTreeviewTooltip* self, gint x, gint y, gboolean keyboard_tip, GtkTooltip* tooltip) {
	char* tag;
	GtkTreeModel* _tmp0_;
	GtkTreeModel* model;
	GtkTreePath* path;
	GtkTreeIter iter = {0};
	const GtkTreePath* _tmp8_;
	GtkTreePath* _tmp7_;
	gboolean _tmp6_;
	const GtkTreePath* _tmp5_;
	GtkTreeModel* _tmp4_;
	GtkTreeModel* _tmp3_;
	gboolean _tmp2_;
	GtkTreeModel* _tmp1_;
	mpd_Song* song;
	char* new_check;
	gboolean _tmp17_;
	gboolean _tmp22_;
	g_return_val_if_fail (self != NULL, FALSE);
	g_return_val_if_fail (tooltip != NULL, FALSE);
	tag = NULL;
	_tmp0_ = NULL;
	model = (_tmp0_ = gtk_tree_view_get_model (self->priv->par_widget), (_tmp0_ == NULL) ? NULL : g_object_ref (_tmp0_));
	path = NULL;
	_tmp8_ = NULL;
	_tmp7_ = NULL;
	_tmp5_ = NULL;
	_tmp4_ = NULL;
	_tmp3_ = NULL;
	_tmp1_ = NULL;
	if (!(_tmp6_ = (_tmp2_ = gtk_tree_view_get_tooltip_context (self->priv->par_widget, &x, &y, keyboard_tip, &_tmp1_, &_tmp5_, &iter), model = (_tmp3_ = (_tmp4_ = _tmp1_, (_tmp4_ == NULL) ? NULL : g_object_ref (_tmp4_)), (model == NULL) ? NULL : (model = (g_object_unref (model), NULL)), _tmp3_), _tmp2_), path = (_tmp7_ = (_tmp8_ = _tmp5_, (_tmp8_ == NULL) ? NULL : gtk_tree_path_copy (_tmp8_)), (path == NULL) ? NULL : (path = (gtk_tree_path_free (path), NULL)), _tmp7_), _tmp6_)) {
		char* _tmp9_;
		gboolean _tmp10_;
		_tmp9_ = NULL;
		self->priv->checksum = (_tmp9_ = NULL, self->priv->checksum = (g_free (self->priv->checksum), NULL), _tmp9_);
		return (_tmp10_ = FALSE, tag = (g_free (tag), NULL), (model == NULL) ? NULL : (model = (g_object_unref (model), NULL)), (path == NULL) ? NULL : (path = (gtk_tree_path_free (path), NULL)), _tmp10_);
	}
	song = mpd_newSong ();
	if (self->priv->mtype == META_ALBUM_ART) {
		char* album;
		char* _tmp12_;
		const char* _tmp11_;
		char* _tmp14_;
		const char* _tmp13_;
		album = NULL;
		gtk_tree_model_get (model, &iter, 5, &tag, 6, &album, -1);
		_tmp12_ = NULL;
		_tmp11_ = NULL;
		song->artist = (_tmp12_ = (_tmp11_ = tag, (_tmp11_ == NULL) ? NULL : g_strdup (_tmp11_)), song->artist = (g_free (song->artist), NULL), _tmp12_);
		_tmp14_ = NULL;
		_tmp13_ = NULL;
		song->album = (_tmp14_ = (_tmp13_ = album, (_tmp13_ == NULL) ? NULL : g_strdup (_tmp13_)), song->album = (g_free (song->album), NULL), _tmp14_);
		album = (g_free (album), NULL);
	} else {
		char* _tmp16_;
		const char* _tmp15_;
		gtk_tree_model_get (model, &iter, 7, &tag, -1);
		_tmp16_ = NULL;
		_tmp15_ = NULL;
		song->artist = (_tmp16_ = (_tmp15_ = tag, (_tmp15_ == NULL) ? NULL : g_strdup (_tmp15_)), song->artist = (g_free (song->artist), NULL), _tmp16_);
	}
	new_check = mpd_song_checksum (song);
	_tmp17_ = FALSE;
	if (_vala_strcmp0 (new_check, self->priv->checksum) != 0) {
		_tmp17_ = self->priv->checksum != NULL;
	} else {
		_tmp17_ = FALSE;
	}
	if (_tmp17_) {
		char* _tmp18_;
		gboolean _tmp19_;
		_tmp18_ = NULL;
		self->priv->checksum = (_tmp18_ = NULL, self->priv->checksum = (g_free (self->priv->checksum), NULL), _tmp18_);
		return (_tmp19_ = FALSE, tag = (g_free (tag), NULL), (model == NULL) ? NULL : (model = (g_object_unref (model), NULL)), (path == NULL) ? NULL : (path = (gtk_tree_path_free (path), NULL)), (song == NULL) ? NULL : (song = (mpd_freeSong (song), NULL)), new_check = (g_free (new_check), NULL), _tmp19_);
	}
	if (_vala_strcmp0 (new_check, self->priv->checksum) != 0) {
		char* _tmp21_;
		const char* _tmp20_;
		gmpc_metaimage_update_cover_from_song (self->priv->image, song);
		_tmp21_ = NULL;
		_tmp20_ = NULL;
		self->priv->checksum = (_tmp21_ = (_tmp20_ = new_check, (_tmp20_ == NULL) ? NULL : g_strdup (_tmp20_)), self->priv->checksum = (g_free (self->priv->checksum), NULL), _tmp21_);
	}
	return (_tmp22_ = TRUE, tag = (g_free (tag), NULL), (model == NULL) ? NULL : (model = (g_object_unref (model), NULL)), (path == NULL) ? NULL : (path = (gtk_tree_path_free (path), NULL)), (song == NULL) ? NULL : (song = (mpd_freeSong (song), NULL)), new_check = (g_free (new_check), NULL), _tmp22_);
}


static gboolean _gmpc_mpd_data_treeview_tooltip_query_tooltip_callback_gtk_widget_query_tooltip (GtkWidget* _sender, gint x, gint y, gboolean keyboard_tooltip, GtkTooltip* tooltip, gpointer self) {
	return gmpc_mpd_data_treeview_tooltip_query_tooltip_callback (self, x, y, keyboard_tooltip, tooltip);
}


/*
        this.checksum = null;
        return false;
  */
GmpcMpdDataTreeviewTooltip* gmpc_mpd_data_treeview_tooltip_construct (GType object_type, GtkTreeView* pw, MetaDataType type) {
	GParameter * __params;
	GParameter * __params_it;
	GmpcMpdDataTreeviewTooltip * self;
	GtkTreeView* _tmp1_;
	GtkTreeView* _tmp0_;
	GmpcMetaImage* _tmp2_;
	g_return_val_if_fail (pw != NULL, NULL);
	__params = g_new0 (GParameter, 1);
	__params_it = __params;
	__params_it->name = "type";
	g_value_init (&__params_it->value, GTK_TYPE_WINDOW_TYPE);
	g_value_set_enum (&__params_it->value, GTK_WINDOW_POPUP);
	__params_it++;
	self = g_object_newv (object_type, __params_it - __params, __params);
	gtk_window_set_resizable ((GtkWindow*) self, FALSE);
	fprintf (stdout, "Create tooltip widget\n");
	_tmp1_ = NULL;
	_tmp0_ = NULL;
	self->priv->par_widget = (_tmp1_ = (_tmp0_ = pw, (_tmp0_ == NULL) ? NULL : g_object_ref (_tmp0_)), (self->priv->par_widget == NULL) ? NULL : (self->priv->par_widget = (g_object_unref (self->priv->par_widget), NULL)), _tmp1_);
	g_signal_connect_object ((GtkWidget*) pw, "query-tooltip", (GCallback) _gmpc_mpd_data_treeview_tooltip_query_tooltip_callback_gtk_widget_query_tooltip, self, 0);
	gtk_widget_set_tooltip_window ((GtkWidget*) self->priv->par_widget, (GtkWindow*) self);
	_tmp2_ = NULL;
	self->priv->image = (_tmp2_ = g_object_ref_sink (gmpc_metaimage_new_size (type, 150)), (self->priv->image == NULL) ? NULL : (self->priv->image = (g_object_unref (self->priv->image), NULL)), _tmp2_);
	self->priv->mtype = type;
	gmpc_metaimage_set_squared (self->priv->image, FALSE);
	gmpc_metaimage_set_hide_on_na (self->priv->image, TRUE);
	gtk_container_add ((GtkContainer*) self, (GtkWidget*) self->priv->image);
	while (__params_it > __params) {
		--__params_it;
		g_value_unset (&__params_it->value);
	}
	g_free (__params);
	return self;
}


GmpcMpdDataTreeviewTooltip* gmpc_mpd_data_treeview_tooltip_new (GtkTreeView* pw, MetaDataType type) {
	return gmpc_mpd_data_treeview_tooltip_construct (GMPC_MPD_DATA_TREEVIEW_TYPE_TOOLTIP, pw, type);
}


static void gmpc_mpd_data_treeview_tooltip_class_init (GmpcMpdDataTreeviewTooltipClass * klass) {
	gmpc_mpd_data_treeview_tooltip_parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (GmpcMpdDataTreeviewTooltipPrivate));
	G_OBJECT_CLASS (klass)->finalize = gmpc_mpd_data_treeview_tooltip_finalize;
}


static void gmpc_mpd_data_treeview_tooltip_instance_init (GmpcMpdDataTreeviewTooltip * self) {
	self->priv = GMPC_MPD_DATA_TREEVIEW_TOOLTIP_GET_PRIVATE (self);
	self->priv->par_widget = NULL;
	self->priv->image = NULL;
	self->priv->mtype = META_ARTIST_ART;
	self->priv->checksum = NULL;
}


static void gmpc_mpd_data_treeview_tooltip_finalize (GObject* obj) {
	GmpcMpdDataTreeviewTooltip * self;
	self = GMPC_MPD_DATA_TREEVIEW_TOOLTIP (obj);
	{
		fprintf (stdout, "Gmpc.MpdData.Treeview.Tooltip destroy\n");
	}
	(self->priv->par_widget == NULL) ? NULL : (self->priv->par_widget = (g_object_unref (self->priv->par_widget), NULL));
	(self->priv->image == NULL) ? NULL : (self->priv->image = (g_object_unref (self->priv->image), NULL));
	self->priv->checksum = (g_free (self->priv->checksum), NULL);
	G_OBJECT_CLASS (gmpc_mpd_data_treeview_tooltip_parent_class)->finalize (obj);
}


GType gmpc_mpd_data_treeview_tooltip_get_type (void) {
	static GType gmpc_mpd_data_treeview_tooltip_type_id = 0;
	if (gmpc_mpd_data_treeview_tooltip_type_id == 0) {
		static const GTypeInfo g_define_type_info = { sizeof (GmpcMpdDataTreeviewTooltipClass), (GBaseInitFunc) NULL, (GBaseFinalizeFunc) NULL, (GClassInitFunc) gmpc_mpd_data_treeview_tooltip_class_init, (GClassFinalizeFunc) NULL, NULL, sizeof (GmpcMpdDataTreeviewTooltip), 0, (GInstanceInitFunc) gmpc_mpd_data_treeview_tooltip_instance_init, NULL };
		gmpc_mpd_data_treeview_tooltip_type_id = g_type_register_static (GTK_TYPE_WINDOW, "GmpcMpdDataTreeviewTooltip", &g_define_type_info, 0);
	}
	return gmpc_mpd_data_treeview_tooltip_type_id;
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




