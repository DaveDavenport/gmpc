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



static gboolean gmpc_mpd_data_treeview_tooltip_query_tooltip_callback (GmpcMpdDataTreeviewTooltip* self, gint x, gint y, gboolean keyboard_tip, GtkTooltip* tooltip) {
	gboolean result;
	char* tag;
	gint row_type;
	GtkTreePath* path;
	GtkTreeIter iter = {0};
	GtkTreeModel* _tmp0_;
	GtkTreeModel* model;
	gboolean _tmp1_;
	const GtkTreePath* _tmp10_;
	GtkTreePath* _tmp9_;
	gboolean _tmp8_;
	const GtkTreePath* _tmp7_;
	GtkTreeModel* _tmp6_;
	GtkTreeModel* _tmp5_;
	gboolean _tmp4_;
	GtkTreeModel* _tmp3_;
	mpd_Song* song;
	char* new_check;
	gboolean _tmp22_;
	g_return_val_if_fail (self != NULL, FALSE);
	g_return_val_if_fail (tooltip != NULL, FALSE);
	tag = NULL;
	row_type = 0;
	path = NULL;
	_tmp0_ = NULL;
	model = (_tmp0_ = gtk_tree_view_get_model (self->priv->par_widget), (_tmp0_ == NULL) ? NULL : g_object_ref (_tmp0_));
	if (cfg_get_single_value_as_int_with_default (config, "GmpcTreeView", "show-tooltip", 1) != 1) {
		result = FALSE;
		tag = (g_free (tag), NULL);
		(path == NULL) ? NULL : (path = (gtk_tree_path_free (path), NULL));
		(model == NULL) ? NULL : (model = (g_object_unref (model), NULL));
		return result;
	}
	_tmp1_ = FALSE;
	if (self->mtype != META_ARTIST_ART) {
		_tmp1_ = self->mtype != META_ALBUM_ART;
	} else {
		_tmp1_ = FALSE;
	}
	if (_tmp1_) {
		char* _tmp2_;
		_tmp2_ = NULL;
		self->priv->checksum = (_tmp2_ = NULL, self->priv->checksum = (g_free (self->priv->checksum), NULL), _tmp2_);
		result = FALSE;
		tag = (g_free (tag), NULL);
		(path == NULL) ? NULL : (path = (gtk_tree_path_free (path), NULL));
		(model == NULL) ? NULL : (model = (g_object_unref (model), NULL));
		return result;
	}
	_tmp10_ = NULL;
	_tmp9_ = NULL;
	_tmp7_ = NULL;
	_tmp6_ = NULL;
	_tmp5_ = NULL;
	_tmp3_ = NULL;
	if (!(_tmp8_ = (_tmp4_ = gtk_tree_view_get_tooltip_context (self->priv->par_widget, &x, &y, keyboard_tip, &_tmp3_, &_tmp7_, &iter), model = (_tmp5_ = (_tmp6_ = _tmp3_, (_tmp6_ == NULL) ? NULL : g_object_ref (_tmp6_)), (model == NULL) ? NULL : (model = (g_object_unref (model), NULL)), _tmp5_), _tmp4_), path = (_tmp9_ = (_tmp10_ = _tmp7_, (_tmp10_ == NULL) ? NULL : gtk_tree_path_copy (_tmp10_)), (path == NULL) ? NULL : (path = (gtk_tree_path_free (path), NULL)), _tmp9_), _tmp8_)) {
		char* _tmp11_;
		_tmp11_ = NULL;
		self->priv->checksum = (_tmp11_ = NULL, self->priv->checksum = (g_free (self->priv->checksum), NULL), _tmp11_);
		result = FALSE;
		tag = (g_free (tag), NULL);
		(path == NULL) ? NULL : (path = (gtk_tree_path_free (path), NULL));
		(model == NULL) ? NULL : (model = (g_object_unref (model), NULL));
		return result;
	}
	song = mpd_newSong ();
	/* Get the row type */
	gtk_tree_model_get (model, &iter, 26, &row_type, -1);
	if (row_type == MPD_DATA_TYPE_SONG) {
		char* album;
		char* _tmp13_;
		const char* _tmp12_;
		char* _tmp15_;
		const char* _tmp14_;
		album = NULL;
		gtk_tree_model_get (model, &iter, 5, &tag, 6, &album, -1);
		_tmp13_ = NULL;
		_tmp12_ = NULL;
		song->artist = (_tmp13_ = (_tmp12_ = tag, (_tmp12_ == NULL) ? NULL : g_strdup (_tmp12_)), song->artist = (g_free (song->artist), NULL), _tmp13_);
		_tmp15_ = NULL;
		_tmp14_ = NULL;
		song->album = (_tmp15_ = (_tmp14_ = album, (_tmp14_ == NULL) ? NULL : g_strdup (_tmp14_)), song->album = (g_free (song->album), NULL), _tmp15_);
		album = (g_free (album), NULL);
	} else {
		if (row_type == MPD_DATA_TYPE_TAG) {
			if (self->mtype == META_ARTIST_ART) {
				char* _tmp17_;
				const char* _tmp16_;
				gtk_tree_model_get (model, &iter, 7, &tag, -1);
				_tmp17_ = NULL;
				_tmp16_ = NULL;
				song->artist = (_tmp17_ = (_tmp16_ = tag, (_tmp16_ == NULL) ? NULL : g_strdup (_tmp16_)), song->artist = (g_free (song->artist), NULL), _tmp17_);
			} else {
				if (self->mtype == META_ALBUM_ART) {
					char* _tmp19_;
					const char* _tmp18_;
					char* _tmp21_;
					const char* _tmp20_;
					gtk_tree_model_get (model, &iter, 7, &tag, -1);
					_tmp19_ = NULL;
					_tmp18_ = NULL;
					song->artist = (_tmp19_ = (_tmp18_ = self->request_artist, (_tmp18_ == NULL) ? NULL : g_strdup (_tmp18_)), song->artist = (g_free (song->artist), NULL), _tmp19_);
					_tmp21_ = NULL;
					_tmp20_ = NULL;
					song->album = (_tmp21_ = (_tmp20_ = tag, (_tmp20_ == NULL) ? NULL : g_strdup (_tmp20_)), song->album = (g_free (song->album), NULL), _tmp21_);
				}
			}
		}
	}
	new_check = mpd_song_checksum (song);
	_tmp22_ = FALSE;
	if (_vala_strcmp0 (new_check, self->priv->checksum) != 0) {
		_tmp22_ = self->priv->checksum != NULL;
	} else {
		_tmp22_ = FALSE;
	}
	if (_tmp22_) {
		char* _tmp23_;
		_tmp23_ = NULL;
		self->priv->checksum = (_tmp23_ = NULL, self->priv->checksum = (g_free (self->priv->checksum), NULL), _tmp23_);
		result = FALSE;
		tag = (g_free (tag), NULL);
		(path == NULL) ? NULL : (path = (gtk_tree_path_free (path), NULL));
		(model == NULL) ? NULL : (model = (g_object_unref (model), NULL));
		(song == NULL) ? NULL : (song = (mpd_freeSong (song), NULL));
		new_check = (g_free (new_check), NULL);
		return result;
	}
	if (_vala_strcmp0 (new_check, self->priv->checksum) != 0) {
		char* _tmp25_;
		const char* _tmp24_;
		MetaData* met;
		MetaData* _tmp28_;
		MetaDataResult _tmp27_;
		MetaData* _tmp26_;
		MetaDataResult _result_;
		_tmp25_ = NULL;
		_tmp24_ = NULL;
		self->priv->checksum = (_tmp25_ = (_tmp24_ = new_check, (_tmp24_ == NULL) ? NULL : g_strdup (_tmp24_)), self->priv->checksum = (g_free (self->priv->checksum), NULL), _tmp25_);
		met = NULL;
		_tmp28_ = NULL;
		_tmp26_ = NULL;
		_result_ = (_tmp27_ = gmpc_meta_watcher_get_meta_path (gmw, song, self->mtype, &_tmp26_), met = (_tmp28_ = _tmp26_, (met == NULL) ? NULL : (met = (meta_data_free (met), NULL)), _tmp28_), _tmp27_);
		gmpc_mpd_data_treeview_tooltip_metadata_changed (self, gmw, song, self->mtype, _result_, met);
		(met == NULL) ? NULL : (met = (meta_data_free (met), NULL));
	}
	if (gtk_image_get_storage_type (self->priv->image) == GTK_IMAGE_EMPTY) {
		result = FALSE;
		tag = (g_free (tag), NULL);
		(path == NULL) ? NULL : (path = (gtk_tree_path_free (path), NULL));
		(model == NULL) ? NULL : (model = (g_object_unref (model), NULL));
		(song == NULL) ? NULL : (song = (mpd_freeSong (song), NULL));
		new_check = (g_free (new_check), NULL);
		return result;
	}
	result = TRUE;
	tag = (g_free (tag), NULL);
	(path == NULL) ? NULL : (path = (gtk_tree_path_free (path), NULL));
	(model == NULL) ? NULL : (model = (g_object_unref (model), NULL));
	(song == NULL) ? NULL : (song = (mpd_freeSong (song), NULL));
	new_check = (g_free (new_check), NULL);
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
	_tmp0_ = NULL;
	if ((_tmp1_ = _vala_strcmp0 (self->priv->checksum, _tmp0_ = mpd_song_checksum (song)) != 0, _tmp0_ = (g_free (_tmp0_), NULL), _tmp1_)) {
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
						(pb == NULL) ? NULL : (pb = (g_object_unref (pb), NULL));
					}
					goto __finally0;
					__catch0_g_error:
					{
						GError * e;
						e = _inner_error_;
						_inner_error_ = NULL;
						{
							gtk_image_clear (self->priv->image);
							(e == NULL) ? NULL : (e = (g_error_free (e), NULL));
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
	GtkTreeView* _tmp1_;
	GtkTreeView* _tmp0_;
	GtkImage* _tmp2_;
	g_return_val_if_fail (pw != NULL, NULL);
	__params = g_new0 (GParameter, 1);
	__params_it = __params;
	__params_it->name = "type";
	g_value_init (&__params_it->value, GTK_TYPE_WINDOW_TYPE);
	g_value_set_enum (&__params_it->value, GTK_WINDOW_POPUP);
	__params_it++;
	self = g_object_newv (object_type, __params_it - __params, __params);
	gtk_window_set_resizable ((GtkWindow*) self, FALSE);
	_tmp1_ = NULL;
	_tmp0_ = NULL;
	self->priv->par_widget = (_tmp1_ = (_tmp0_ = pw, (_tmp0_ == NULL) ? NULL : g_object_ref (_tmp0_)), (self->priv->par_widget == NULL) ? NULL : (self->priv->par_widget = (g_object_unref (self->priv->par_widget), NULL)), _tmp1_);
	g_signal_connect_object ((GtkWidget*) pw, "query-tooltip", (GCallback) _gmpc_mpd_data_treeview_tooltip_query_tooltip_callback_gtk_widget_query_tooltip, self, 0);
	gtk_widget_set_tooltip_window ((GtkWidget*) self->priv->par_widget, (GtkWindow*) self);
	_tmp2_ = NULL;
	self->priv->image = (_tmp2_ = g_object_ref_sink ((GtkImage*) gtk_image_new ()), (self->priv->image == NULL) ? NULL : (self->priv->image = (g_object_unref (self->priv->image), NULL)), _tmp2_);
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
	(self->priv->par_widget == NULL) ? NULL : (self->priv->par_widget = (g_object_unref (self->priv->par_widget), NULL));
	(self->priv->image == NULL) ? NULL : (self->priv->image = (g_object_unref (self->priv->image), NULL));
	self->request_artist = (g_free (self->request_artist), NULL);
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




