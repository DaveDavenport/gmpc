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
#include <metadata.h>
#include <stdlib.h>
#include <string.h>
#include <plugin.h>
#include <config1.h>
#include <libmpd/libmpdclient.h>
#include <libmpd/libmpd.h>
#include <misc.h>
#include <main.h>
#include <gmpc-meta-watcher.h>
#include <gdk-pixbuf/gdk-pixdata.h>


#define GMPC_MPD_DATA_TREEVIEW_TYPE_TOOLTIP (gmpc_mpd_data_treeview_tooltip_get_type ())
#define GMPC_MPD_DATA_TREEVIEW_TOOLTIP(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GMPC_MPD_DATA_TREEVIEW_TYPE_TOOLTIP, GmpcMpdDataTreeviewTooltip))
#define GMPC_MPD_DATA_TREEVIEW_TOOLTIP_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GMPC_MPD_DATA_TREEVIEW_TYPE_TOOLTIP, GmpcMpdDataTreeviewTooltipClass))
#define GMPC_MPD_DATA_TREEVIEW_IS_TOOLTIP(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GMPC_MPD_DATA_TREEVIEW_TYPE_TOOLTIP))
#define GMPC_MPD_DATA_TREEVIEW_IS_TOOLTIP_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GMPC_MPD_DATA_TREEVIEW_TYPE_TOOLTIP))
#define GMPC_MPD_DATA_TREEVIEW_TOOLTIP_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GMPC_MPD_DATA_TREEVIEW_TYPE_TOOLTIP, GmpcMpdDataTreeviewTooltipClass))

typedef struct _GmpcMpdDataTreeviewTooltip GmpcMpdDataTreeviewTooltip;
typedef struct _GmpcMpdDataTreeviewTooltipClass GmpcMpdDataTreeviewTooltipClass;
typedef struct _GmpcMpdDataTreeviewTooltipPrivate GmpcMpdDataTreeviewTooltipPrivate;
#define _g_object_unref0(var) ((var == NULL) ? NULL : (var = (g_object_unref (var), NULL)))
#define _g_free0(var) (var = (g_free (var), NULL))
#define _gtk_tree_path_free0(var) ((var == NULL) ? NULL : (var = (gtk_tree_path_free (var), NULL)))
#define _mpd_freeSong0(var) ((var == NULL) ? NULL : (var = (mpd_freeSong (var), NULL)))
#define _meta_data_free0(var) ((var == NULL) ? NULL : (var = (meta_data_free (var), NULL)))
#define _g_error_free0(var) ((var == NULL) ? NULL : (var = (g_error_free (var), NULL)))

struct _GmpcMpdDataTreeviewTooltip {
	GtkWindow parent_instance;
	GmpcMpdDataTreeviewTooltipPrivate * priv;
	MetaDataType mtype;
	char* request_artist;
};

struct _GmpcMpdDataTreeviewTooltipClass {
	GtkWindowClass parent_class;
};

struct _GmpcMpdDataTreeviewTooltipPrivate {
	GtkTreeView* par_widget;
	GtkImage* image;
	char* checksum;
};


static gpointer gmpc_mpd_data_treeview_tooltip_parent_class = NULL;

#define use_transition_mtt TRUE
#define some_unique_name_mtt VERSION
GType gmpc_mpd_data_treeview_tooltip_get_type (void);
#define GMPC_MPD_DATA_TREEVIEW_TOOLTIP_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GMPC_MPD_DATA_TREEVIEW_TYPE_TOOLTIP, GmpcMpdDataTreeviewTooltipPrivate))
enum  {
	GMPC_MPD_DATA_TREEVIEW_TOOLTIP_DUMMY_PROPERTY
};
static void gmpc_mpd_data_treeview_tooltip_metadata_changed (GmpcMpdDataTreeviewTooltip* self, GmpcMetaWatcher* gmw, const mpd_Song* song, MetaDataType type, MetaDataResult _result_, const MetaData* met);
static gboolean gmpc_mpd_data_treeview_tooltip_query_tooltip_callback (GmpcMpdDataTreeviewTooltip* self, gint x, gint y, gboolean keyboard_tip, GtkTooltip* tooltip);
static gboolean _gmpc_mpd_data_treeview_tooltip_query_tooltip_callback_gtk_widget_query_tooltip (GtkWidget* _sender, gint x, gint y, gboolean keyboard_tooltip, GtkTooltip* tooltip, gpointer self);
static void _gmpc_mpd_data_treeview_tooltip_metadata_changed_gmpc_meta_watcher_data_changed (GmpcMetaWatcher* _sender, const mpd_Song* song, MetaDataType type, MetaDataResult _result_, const MetaData* met, gpointer self);
GmpcMpdDataTreeviewTooltip* gmpc_mpd_data_treeview_tooltip_new (GtkTreeView* pw, MetaDataType type);
GmpcMpdDataTreeviewTooltip* gmpc_mpd_data_treeview_tooltip_construct (GType object_type, GtkTreeView* pw, MetaDataType type);
static void gmpc_mpd_data_treeview_tooltip_finalize (GObject* obj);
static int _vala_strcmp0 (const char * str1, const char * str2);



static gpointer _g_object_ref0 (gpointer self) {
	return self ? g_object_ref (self) : NULL;
}


static gpointer _gtk_tree_path_copy0 (gpointer self) {
	return self ? gtk_tree_path_copy (self) : NULL;
}


static gboolean gmpc_mpd_data_treeview_tooltip_query_tooltip_callback (GmpcMpdDataTreeviewTooltip* self, gint x, gint y, gboolean keyboard_tip, GtkTooltip* tooltip) {
	gboolean result;
	char* tag;
	gint row_type;
	GtkTreePath* path;
	GtkTreeIter iter = {0};
	GtkTreeModel* model;
	gboolean _tmp0_;
	GtkTreePath* _tmp7_;
	gboolean _tmp6_;
	const GtkTreePath* _tmp5_;
	GtkTreeModel* _tmp4_;
	gboolean _tmp3_;
	GtkTreeModel* _tmp2_;
	mpd_Song* song;
	char* new_check;
	gboolean _tmp14_;
	g_return_val_if_fail (self != NULL, FALSE);
	g_return_val_if_fail (tooltip != NULL, FALSE);
	tag = NULL;
	row_type = 0;
	path = NULL;
	model = _g_object_ref0 (gtk_tree_view_get_model (self->priv->par_widget));
	if (cfg_get_single_value_as_int_with_default (config, "GmpcTreeView", "show-tooltip", 1) != 1) {
		result = FALSE;
		_g_free0 (tag);
		_gtk_tree_path_free0 (path);
		_g_object_unref0 (model);
		return result;
	}
	_tmp0_ = FALSE;
	if (self->mtype != META_ARTIST_ART) {
		_tmp0_ = self->mtype != META_ALBUM_ART;
	} else {
		_tmp0_ = FALSE;
	}
	if (_tmp0_) {
		char* _tmp1_;
		self->priv->checksum = (_tmp1_ = NULL, _g_free0 (self->priv->checksum), _tmp1_);
		result = FALSE;
		_g_free0 (tag);
		_gtk_tree_path_free0 (path);
		_g_object_unref0 (model);
		return result;
	}
	_tmp5_ = NULL;
	_tmp2_ = NULL;
	if (!(_tmp6_ = (_tmp3_ = gtk_tree_view_get_tooltip_context (self->priv->par_widget, &x, &y, keyboard_tip, &_tmp2_, &_tmp5_, &iter), model = (_tmp4_ = _g_object_ref0 (_tmp2_), _g_object_unref0 (model), _tmp4_), _tmp3_), path = (_tmp7_ = _gtk_tree_path_copy0 (_tmp5_), _gtk_tree_path_free0 (path), _tmp7_), _tmp6_)) {
		char* _tmp8_;
		self->priv->checksum = (_tmp8_ = NULL, _g_free0 (self->priv->checksum), _tmp8_);
		result = FALSE;
		_g_free0 (tag);
		_gtk_tree_path_free0 (path);
		_g_object_unref0 (model);
		return result;
	}
	song = mpd_newSong ();
	gtk_tree_model_get (model, &iter, 26, &row_type, -1);
	if (row_type == MPD_DATA_TYPE_SONG) {
		char* album;
		char* _tmp9_;
		char* _tmp10_;
		album = NULL;
		gtk_tree_model_get (model, &iter, 5, &tag, 6, &album, -1);
		song->artist = (_tmp9_ = g_strdup (tag), _g_free0 (song->artist), _tmp9_);
		song->album = (_tmp10_ = g_strdup (album), _g_free0 (song->album), _tmp10_);
		_g_free0 (album);
	} else {
		if (row_type == MPD_DATA_TYPE_TAG) {
			if (self->mtype == META_ARTIST_ART) {
				char* _tmp11_;
				gtk_tree_model_get (model, &iter, 7, &tag, -1);
				song->artist = (_tmp11_ = g_strdup (tag), _g_free0 (song->artist), _tmp11_);
			} else {
				if (self->mtype == META_ALBUM_ART) {
					char* _tmp12_;
					char* _tmp13_;
					gtk_tree_model_get (model, &iter, 7, &tag, -1);
					song->artist = (_tmp12_ = g_strdup (self->request_artist), _g_free0 (song->artist), _tmp12_);
					song->album = (_tmp13_ = g_strdup (tag), _g_free0 (song->album), _tmp13_);
				}
			}
		}
	}
	new_check = mpd_song_checksum (song);
	_tmp14_ = FALSE;
	if (_vala_strcmp0 (new_check, self->priv->checksum) != 0) {
		_tmp14_ = self->priv->checksum != NULL;
	} else {
		_tmp14_ = FALSE;
	}
	if (_tmp14_) {
		char* _tmp15_;
		self->priv->checksum = (_tmp15_ = NULL, _g_free0 (self->priv->checksum), _tmp15_);
		result = FALSE;
		_g_free0 (tag);
		_gtk_tree_path_free0 (path);
		_g_object_unref0 (model);
		_mpd_freeSong0 (song);
		_g_free0 (new_check);
		return result;
	}
	if (_vala_strcmp0 (new_check, self->priv->checksum) != 0) {
		char* _tmp16_;
		MetaData* met;
		MetaData* _tmp19_;
		MetaDataResult _tmp18_;
		MetaData* _tmp17_;
		MetaDataResult _result_;
		self->priv->checksum = (_tmp16_ = g_strdup (new_check), _g_free0 (self->priv->checksum), _tmp16_);
		met = NULL;
		_tmp17_ = NULL;
		_result_ = (_tmp18_ = gmpc_meta_watcher_get_meta_path (gmw, song, self->mtype, &_tmp17_), met = (_tmp19_ = _tmp17_, _meta_data_free0 (met), _tmp19_), _tmp18_);
		gmpc_mpd_data_treeview_tooltip_metadata_changed (self, gmw, song, self->mtype, _result_, met);
		_meta_data_free0 (met);
	}
	if (gtk_image_get_storage_type (self->priv->image) == GTK_IMAGE_EMPTY) {
		result = FALSE;
		_g_free0 (tag);
		_gtk_tree_path_free0 (path);
		_g_object_unref0 (model);
		_mpd_freeSong0 (song);
		_g_free0 (new_check);
		return result;
	}
	result = TRUE;
	_g_free0 (tag);
	_gtk_tree_path_free0 (path);
	_g_object_unref0 (model);
	_mpd_freeSong0 (song);
	_g_free0 (new_check);
	return result;
}


static void gmpc_mpd_data_treeview_tooltip_metadata_changed (GmpcMpdDataTreeviewTooltip* self, GmpcMetaWatcher* gmw, const mpd_Song* song, MetaDataType type, MetaDataResult _result_, const MetaData* met) {
	GError * _inner_error_;
	char* _tmp0_;
	gboolean _tmp1_;
	g_return_if_fail (self != NULL);
	g_return_if_fail (gmw != NULL);
	g_return_if_fail (song != NULL);
	_inner_error_ = NULL;
	if (type != self->mtype) {
		return;
	}
	if ((_tmp1_ = _vala_strcmp0 (self->priv->checksum, _tmp0_ = mpd_song_checksum (song)) != 0, _g_free0 (_tmp0_), _tmp1_)) {
		return;
	}
	if (_result_ == META_DATA_UNAVAILABLE) {
		gtk_image_clear (self->priv->image);
	} else {
		if (_result_ == META_DATA_FETCHING) {
			gtk_image_clear (self->priv->image);
		} else {
			if (_result_ == META_DATA_AVAILABLE) {
				if (met->content_type == META_DATA_CONTENT_URI) {
					{
						GdkPixbuf* pb;
						pb = gdk_pixbuf_new_from_file_at_scale (meta_data_get_uri (met), 150, 150, TRUE, &_inner_error_);
						if (_inner_error_ != NULL) {
							goto __catch0_g_error;
							goto __finally0;
						}
						gtk_image_set_from_pixbuf (self->priv->image, pb);
						_g_object_unref0 (pb);
					}
					goto __finally0;
					__catch0_g_error:
					{
						GError * e;
						e = _inner_error_;
						_inner_error_ = NULL;
						{
							gtk_image_clear (self->priv->image);
							_g_error_free0 (e);
						}
					}
					__finally0:
					if (_inner_error_ != NULL) {
						g_critical ("file %s: line %d: uncaught error: %s", __FILE__, __LINE__, _inner_error_->message);
						g_clear_error (&_inner_error_);
						return;
					}
				} else {
					gtk_image_clear (self->priv->image);
				}
			}
		}
	}
}


static gboolean _gmpc_mpd_data_treeview_tooltip_query_tooltip_callback_gtk_widget_query_tooltip (GtkWidget* _sender, gint x, gint y, gboolean keyboard_tooltip, GtkTooltip* tooltip, gpointer self) {
	return gmpc_mpd_data_treeview_tooltip_query_tooltip_callback (self, x, y, keyboard_tooltip, tooltip);
}


static void _gmpc_mpd_data_treeview_tooltip_metadata_changed_gmpc_meta_watcher_data_changed (GmpcMetaWatcher* _sender, const mpd_Song* song, MetaDataType type, MetaDataResult _result_, const MetaData* met, gpointer self) {
	gmpc_mpd_data_treeview_tooltip_metadata_changed (self, _sender, song, type, _result_, met);
}


GmpcMpdDataTreeviewTooltip* gmpc_mpd_data_treeview_tooltip_construct (GType object_type, GtkTreeView* pw, MetaDataType type) {
	GParameter * __params;
	GParameter * __params_it;
	GmpcMpdDataTreeviewTooltip * self;
	GtkTreeView* _tmp0_;
	GtkImage* _tmp1_;
	g_return_val_if_fail (pw != NULL, NULL);
	__params = g_new0 (GParameter, 1);
	__params_it = __params;
	__params_it->name = "type";
	g_value_init (&__params_it->value, GTK_TYPE_WINDOW_TYPE);
	g_value_set_enum (&__params_it->value, GTK_WINDOW_POPUP);
	__params_it++;
	self = g_object_newv (object_type, __params_it - __params, __params);
	gtk_window_set_resizable ((GtkWindow*) self, FALSE);
	self->priv->par_widget = (_tmp0_ = _g_object_ref0 (pw), _g_object_unref0 (self->priv->par_widget), _tmp0_);
	g_signal_connect_object ((GtkWidget*) pw, "query-tooltip", (GCallback) _gmpc_mpd_data_treeview_tooltip_query_tooltip_callback_gtk_widget_query_tooltip, self, 0);
	gtk_widget_set_tooltip_window ((GtkWidget*) self->priv->par_widget, (GtkWindow*) self);
	self->priv->image = (_tmp1_ = g_object_ref_sink ((GtkImage*) gtk_image_new ()), _g_object_unref0 (self->priv->image), _tmp1_);
	gtk_widget_show ((GtkWidget*) self->priv->image);
	self->mtype = type;
	gtk_container_add ((GtkContainer*) self, (GtkWidget*) self->priv->image);
	gtk_container_set_border_width ((GtkContainer*) self, (guint) 2);
	gtk_widget_modify_bg ((GtkWidget*) self, GTK_STATE_NORMAL, &gtk_widget_get_style ((GtkWidget*) pw)->black);
	g_signal_connect_object (gmw, "data-changed", (GCallback) _gmpc_mpd_data_treeview_tooltip_metadata_changed_gmpc_meta_watcher_data_changed, self, 0);
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
	self->mtype = META_ARTIST_ART;
	self->request_artist = NULL;
	self->priv->checksum = NULL;
}


static void gmpc_mpd_data_treeview_tooltip_finalize (GObject* obj) {
	GmpcMpdDataTreeviewTooltip * self;
	self = GMPC_MPD_DATA_TREEVIEW_TOOLTIP (obj);
	_g_object_unref0 (self->priv->par_widget);
	_g_object_unref0 (self->priv->image);
	_g_free0 (self->request_artist);
	_g_free0 (self->priv->checksum);
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




