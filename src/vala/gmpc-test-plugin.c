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

#include "gmpc-test-plugin.h"
#include <libmpd/libmpdclient.h>
#include <metadata.h>
#include <config.h>
#include <gdk-pixbuf/gdk-pixdata.h>
#include <glib/gi18n-lib.h>
#include <stdio.h>
#include <float.h>
#include <math.h>
#include <glib/gstdio.h>
#include <main.h>
#include <gmpc-meta-watcher.h>
#include <plugin.h>
#include <config1.h>
#include <libmpd/libmpd.h>




struct _SongWindowPrivate {
	GtkListStore* model;
	GtkTreeView* tree;
	GList* downloads;
	mpd_Song* song;
	MetaDataType query_type;
	void* handle;
	void* handle2;
	GtkEntry* artist_entry;
	GtkEntry* album_entry;
	GtkButton* refresh;
};

#define SONG_WINDOW_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), TYPE_SONG_WINDOW, SongWindowPrivate))
enum  {
	SONG_WINDOW_DUMMY_PROPERTY
};
#define SONG_WINDOW_some_unique_name VERSION
static void song_window_add_entry (SongWindow* self, const char* uri, GdkPixbufFormat* format, GdkPixbuf* pb);
static void _song_window_image_downloaded_gmpc_async_download_callback (const GEADAsyncHandler* handle, GEADStatus status, gpointer self);
static void _song_window_store_image_gmpc_async_download_callback (const GEADAsyncHandler* handle, GEADStatus status, gpointer self);
static void song_window_set_metadata (SongWindow* self, GtkButton* button);
static void _song_window_callback_gmpc_meta_data_callback (void* handle, const char* plugin_name, GList* list, gpointer self);
static void _song_window_destroy_popup_gtk_button_clicked (GtkButton* _sender, gpointer self);
static void _song_window_set_metadata_gtk_button_clicked (GtkButton* _sender, gpointer self);
static void _song_window_refresh_query_gtk_button_clicked (GtkButton* _sender, gpointer self);
static SongWindow* song_window_construct (GType object_type, const mpd_Song* song, MetaDataType type);
static SongWindow* song_window_new (const mpd_Song* song, MetaDataType type);
static GObject * song_window_constructor (GType type, guint n_construct_properties, GObjectConstructParam * construct_properties);
static gpointer song_window_parent_class = NULL;
static void song_window_finalize (GObject* obj);
enum  {
	GMPC_TEST_PLUGIN_DUMMY_PROPERTY
};
static gint* gmpc_test_plugin_real_get_version (GmpcPluginBase* base, int* result_length1);
static const char* gmpc_test_plugin_real_get_name (GmpcPluginBase* base);
static void gmpc_test_plugin_real_save_yourself (GmpcPluginBase* base);
static gboolean gmpc_test_plugin_real_get_enabled (GmpcPluginBase* base);
static void gmpc_test_plugin_real_set_enabled (GmpcPluginBase* base, gboolean state);
static void gmpc_test_plugin_real_pane_construct (GmpcPluginPreferencesIface* base, GtkContainer* container);
static void gmpc_test_plugin_real_pane_destroy (GmpcPluginPreferencesIface* base, GtkContainer* container);
static void _gmpc_test_plugin_menu_activated_album_gtk_menu_item_activate (GtkMenuItem* _sender, gpointer self);
static void _gmpc_test_plugin_menu_activated_artist_gtk_menu_item_activate (GtkMenuItem* _sender, gpointer self);
static gint gmpc_test_plugin_real_tool_menu_integration (GmpcPluginToolMenuIface* base, GtkMenu* menu);
static gpointer gmpc_test_plugin_parent_class = NULL;
static GmpcPluginPreferencesIfaceIface* gmpc_test_plugin_gmpc_plugin_preferences_iface_parent_iface = NULL;
static GmpcPluginToolMenuIfaceIface* gmpc_test_plugin_gmpc_plugin_tool_menu_iface_parent_iface = NULL;



static void song_window_add_entry (SongWindow* self, const char* uri, GdkPixbufFormat* format, GdkPixbuf* pb) {
	GtkTreeIter iter = {0};
	char* a;
	char* _tmp0;
	gint new_h;
	gint new_w;
	g_return_if_fail (self != NULL);
	g_return_if_fail (uri != NULL);
	g_return_if_fail (pb != NULL);
	a = NULL;
	_tmp0 = NULL;
	a = (_tmp0 = g_strdup_printf ("<b>%s</b>: %s", _ ("Uri"), uri), a = (g_free (a), NULL), _tmp0);
	if (format != NULL) {
		char* _tmp2;
		char* _tmp1;
		_tmp2 = NULL;
		_tmp1 = NULL;
		a = (_tmp2 = g_strconcat (a, _tmp1 = (g_strdup_printf ("\n<b>%s</b>: %s", _ ("Filetype"), gdk_pixbuf_format_get_name (format))), NULL), a = (g_free (a), NULL), _tmp2);
		_tmp1 = (g_free (_tmp1), NULL);
		fprintf (stdout, "%s\n", gdk_pixbuf_format_get_name (format));
	}
	if (pb != NULL) {
		char* _tmp4;
		char* _tmp3;
		_tmp4 = NULL;
		_tmp3 = NULL;
		a = (_tmp4 = g_strconcat (a, _tmp3 = (g_strdup_printf ("\n<b>%s</b>: %ix%i (%s)", _ ("Size"), gdk_pixbuf_get_width (pb), gdk_pixbuf_get_height (pb), _ ("wxh"))), NULL), a = (g_free (a), NULL), _tmp4);
		_tmp3 = (g_free (_tmp3), NULL);
	}
	new_h = 0;
	new_w = 0;
	if (gdk_pixbuf_get_width (pb) > gdk_pixbuf_get_height (pb)) {
		new_h = 150;
		new_w = (gint) ((150.0 / ((double) gdk_pixbuf_get_height (pb))) * gdk_pixbuf_get_width (pb));
	} else {
		new_w = 150;
		new_h = (gint) ((150.0 / ((double) gdk_pixbuf_get_width (pb))) * gdk_pixbuf_get_height (pb));
	}
	gtk_list_store_append (self->priv->model, &iter);
	gtk_list_store_set (self->priv->model, &iter, 0, gdk_pixbuf_scale_simple (pb, new_w, new_h, GDK_INTERP_BILINEAR), 1, uri, 2, a, -1, -1);
	a = (g_free (a), NULL);
}


void song_window_image_downloaded (SongWindow* self, const GEADAsyncHandler* handle, GEADStatus status) {
	GError * inner_error;
	g_return_if_fail (self != NULL);
	g_return_if_fail (handle != NULL);
	inner_error = NULL;
	if (status == GEAD_PROGRESS) {
		return;
	}
	fprintf (stdout, "Result in: %s\n", gmpc_easy_handler_get_uri (handle));
	self->priv->downloads = g_list_remove (self->priv->downloads, handle);
	if (status == GEAD_DONE) {
		guchar* _tmp1;
		gint data_size;
		gint data_length1;
		gint _tmp0;
		guchar* data;
		_tmp1 = NULL;
		data = (_tmp1 = gmpc_easy_handler_get_data_vala_wrap (handle, &_tmp0), data_length1 = _tmp0, data_size = data_length1, _tmp1);
		{
			GdkPixbufLoader* load;
			GdkPixbuf* _tmp2;
			GdkPixbuf* pb;
			load = gdk_pixbuf_loader_new ();
			{
				gdk_pixbuf_loader_write (load, data, data_length1, &inner_error);
				if (inner_error != NULL) {
					goto __catch1_g_error;
					goto __finally1;
				}
			}
			goto __finally1;
			__catch1_g_error:
			{
				GError * e;
				e = inner_error;
				inner_error = NULL;
				{
					fprintf (stdout, "Failed to load file: %s::%s\n", e->message, gmpc_easy_handler_get_uri (handle));
					(e == NULL) ? NULL : (e = (g_error_free (e), NULL));
				}
			}
			__finally1:
			if (inner_error != NULL) {
				(load == NULL) ? NULL : (load = (g_object_unref (load), NULL));
				goto __catch0_g_error;
				goto __finally0;
			}
			gdk_pixbuf_loader_close (load, &inner_error);
			if (inner_error != NULL) {
				(load == NULL) ? NULL : (load = (g_object_unref (load), NULL));
				goto __catch0_g_error;
				goto __finally0;
			}
			_tmp2 = NULL;
			pb = (_tmp2 = gdk_pixbuf_loader_get_pixbuf (load), (_tmp2 == NULL) ? NULL : g_object_ref (_tmp2));
			/*new Gdk.Pixbuf.from_inline((int)length, (uchar[])data, true); */
			if (pb != NULL) {
				song_window_add_entry (self, gmpc_easy_handler_get_uri (handle), gdk_pixbuf_loader_get_format (load), pb);
			}
			(load == NULL) ? NULL : (load = (g_object_unref (load), NULL));
			(pb == NULL) ? NULL : (pb = (g_object_unref (pb), NULL));
		}
		goto __finally0;
		__catch0_g_error:
		{
			GError * e;
			e = inner_error;
			inner_error = NULL;
			{
				fprintf (stdout, "Failed to load file: %s::%s\n", e->message, gmpc_easy_handler_get_uri (handle));
				(e == NULL) ? NULL : (e = (g_error_free (e), NULL));
				return;
			}
		}
		__finally0:
		if (inner_error != NULL) {
			g_critical ("file %s: line %d: uncaught error: %s", __FILE__, __LINE__, inner_error->message);
			g_clear_error (&inner_error);
			return;
		}
	}
}


static void _song_window_image_downloaded_gmpc_async_download_callback (const GEADAsyncHandler* handle, GEADStatus status, gpointer self) {
	song_window_image_downloaded (self, handle, status);
}


void song_window_callback (SongWindow* self, void* handle, const char* plugin_name, GList* list) {
	GError * inner_error;
	g_return_if_fail (self != NULL);
	inner_error = NULL;
	if (list == NULL) {
		fprintf (stdout, "Done fetching\n");
		if (self->priv->handle == handle) {
			fprintf (stdout, "done 1\n");
			self->priv->handle = NULL;
			if (self->priv->handle == NULL) {
				g_object_set ((GtkWidget*) self->priv->refresh, "sensitive", TRUE, NULL);
			}
		}
		if (self->priv->handle2 == handle) {
			fprintf (stdout, "done 1\n");
			self->priv->handle2 = NULL;
			if (self->priv->handle == NULL) {
				g_object_set ((GtkWidget*) self->priv->refresh, "sensitive", TRUE, NULL);
			}
		}
	}
	{
		GList* uri_collection;
		GList* uri_it;
		uri_collection = list;
		for (uri_it = uri_collection; uri_it != NULL; uri_it = uri_it->next) {
			const char* uri;
			uri = (const char*) uri_it->data;
			{
				fprintf (stdout, "Uri: %s\n", uri);
				if (g_utf8_get_char (g_utf8_offset_to_pointer (uri, 0)) == '/') {
					{
						GdkPixbuf* pb;
						pb = gdk_pixbuf_new_from_file (uri, &inner_error);
						if (inner_error != NULL) {
							goto __catch2_g_error;
							goto __finally2;
						}
						if (pb != NULL) {
							song_window_add_entry (self, uri, gdk_pixbuf_get_file_info (uri, NULL, NULL), pb);
						}
						(pb == NULL) ? NULL : (pb = (g_object_unref (pb), NULL));
					}
					goto __finally2;
					__catch2_g_error:
					{
						GError * e;
						e = inner_error;
						inner_error = NULL;
						{
							(e == NULL) ? NULL : (e = (g_error_free (e), NULL));
						}
					}
					__finally2:
					if (inner_error != NULL) {
						g_critical ("file %s: line %d: uncaught error: %s", __FILE__, __LINE__, inner_error->message);
						g_clear_error (&inner_error);
						return;
					}
				} else {
					GEADAsyncHandler* h;
					h = gmpc_easy_async_downloader (uri, _song_window_image_downloaded_gmpc_async_download_callback, self);
					self->priv->downloads = g_list_append (self->priv->downloads, h);
				}
			}
		}
	}
}


void song_window_store_image (SongWindow* self, const GEADAsyncHandler* handle, GEADStatus status) {
	GError * inner_error;
	g_return_if_fail (self != NULL);
	g_return_if_fail (handle != NULL);
	inner_error = NULL;
	if (status == GEAD_PROGRESS) {
		return;
	}
	self->priv->downloads = g_list_remove (self->priv->downloads, handle);
	fprintf (stdout, "Aap noot mies: %s\n", gmpc_easy_handler_get_uri (handle));
	if (status == GEAD_DONE) {
		guchar* _tmp1;
		gint data_size;
		gint data_length1;
		gint _tmp0;
		guchar* data;
		char* file;
		_tmp1 = NULL;
		data = (_tmp1 = gmpc_easy_handler_get_data_vala_wrap (handle, &_tmp0), data_length1 = _tmp0, data_size = data_length1, _tmp1);
		file = gmpc_get_metadata_filename (self->priv->query_type, self->priv->song, "jpg");
		{
			fprintf (stdout, "Storing into: %s\n", file);
			g_file_set_contents (file, (const char*) data, (glong) data_length1, &inner_error);
			if (inner_error != NULL) {
				goto __catch3_g_error;
				goto __finally3;
			}
			meta_data_set_cache (self->priv->song, self->priv->query_type, META_DATA_AVAILABLE, file);
			gmpc_meta_watcher_data_changed (gmw, self->priv->song, self->priv->query_type, META_DATA_UNAVAILABLE, NULL);
			gmpc_meta_watcher_data_changed (gmw, self->priv->song, self->priv->query_type, META_DATA_AVAILABLE, file);
		}
		goto __finally3;
		__catch3_g_error:
		{
			GError * e;
			e = inner_error;
			inner_error = NULL;
			{
				(e == NULL) ? NULL : (e = (g_error_free (e), NULL));
			}
		}
		__finally3:
		if (inner_error != NULL) {
			file = (g_free (file), NULL);
			g_critical ("file %s: line %d: uncaught error: %s", __FILE__, __LINE__, inner_error->message);
			g_clear_error (&inner_error);
			return;
		}
		file = (g_free (file), NULL);
	}
}


static void _song_window_store_image_gmpc_async_download_callback (const GEADAsyncHandler* handle, GEADStatus status, gpointer self) {
	song_window_store_image (self, handle, status);
}


static void song_window_set_metadata (SongWindow* self, GtkButton* button) {
	GtkTreeIter iter = {0};
	GtkTreeSelection* _tmp0;
	GtkTreeSelection* sel;
	char* path;
	GtkListStore* _tmp4;
	GtkListStore* _tmp3;
	gboolean _tmp2;
	GtkTreeModel* _tmp1;
	g_return_if_fail (self != NULL);
	g_return_if_fail (button != NULL);
	_tmp0 = NULL;
	sel = (_tmp0 = gtk_tree_view_get_selection (self->priv->tree), (_tmp0 == NULL) ? NULL : g_object_ref (_tmp0));
	path = NULL;
	_tmp4 = NULL;
	_tmp3 = NULL;
	_tmp1 = NULL;
	if ((_tmp2 = gtk_tree_selection_get_selected (sel, &_tmp1, &iter), self->priv->model = (_tmp3 = (_tmp4 = (GtkListStore*) _tmp1, (_tmp4 == NULL) ? NULL : g_object_ref (_tmp4)), (self->priv->model == NULL) ? NULL : (self->priv->model = (g_object_unref (self->priv->model), NULL)), _tmp3), _tmp2)) {
		gtk_tree_model_get ((GtkTreeModel*) self->priv->model, &iter, 1, &path, -1);
		fprintf (stdout, "clicked %s\n", path);
		if (g_utf8_get_char (g_utf8_offset_to_pointer (path, 0)) == '/') {
			meta_data_set_cache (self->priv->song, self->priv->query_type, META_DATA_AVAILABLE, path);
			gmpc_meta_watcher_data_changed (gmw, self->priv->song, self->priv->query_type, META_DATA_UNAVAILABLE, NULL);
			gmpc_meta_watcher_data_changed (gmw, self->priv->song, self->priv->query_type, META_DATA_AVAILABLE, path);
		} else {
			GEADAsyncHandler* h;
			h = gmpc_easy_async_downloader (path, _song_window_store_image_gmpc_async_download_callback, self);
			self->priv->downloads = g_list_append (self->priv->downloads, h);
		}
	}
	(sel == NULL) ? NULL : (sel = (g_object_unref (sel), NULL));
	path = (g_free (path), NULL);
}


void song_window_destroy_popup (SongWindow* self, GtkButton* button) {
	g_return_if_fail (self != NULL);
	g_return_if_fail (button != NULL);
	gtk_object_destroy ((GtkObject*) self);
}


static void _song_window_callback_gmpc_meta_data_callback (void* handle, const char* plugin_name, GList* list, gpointer self) {
	song_window_callback (self, handle, plugin_name, list);
}


void song_window_refresh_query (SongWindow* self, GtkButton* button) {
	const mpd_Song* _tmp0;
	mpd_Song* ss;
	char* _tmp2;
	const char* _tmp1;
	gboolean _tmp5;
	g_return_if_fail (self != NULL);
	g_return_if_fail (button != NULL);
	gtk_list_store_clear (self->priv->model);
	_tmp0 = NULL;
	ss = (_tmp0 = self->priv->song, (_tmp0 == NULL) ? NULL : mpd_songDup (_tmp0));
	_tmp2 = NULL;
	_tmp1 = NULL;
	ss->artist = (_tmp2 = (_tmp1 = gtk_entry_get_text (self->priv->artist_entry), (_tmp1 == NULL) ? NULL : g_strdup (_tmp1)), ss->artist = (g_free (ss->artist), NULL), _tmp2);
	if (self->priv->query_type == META_ALBUM_ART) {
		char* _tmp4;
		const char* _tmp3;
		_tmp4 = NULL;
		_tmp3 = NULL;
		ss->album = (_tmp4 = (_tmp3 = gtk_entry_get_text (self->priv->album_entry), (_tmp3 == NULL) ? NULL : g_strdup (_tmp3)), ss->album = (g_free (ss->album), NULL), _tmp4);
	}
	_tmp5 = FALSE;
	if (self->priv->handle == NULL) {
		_tmp5 = self->priv->handle2 == NULL;
	} else {
		_tmp5 = FALSE;
	}
	if (_tmp5) {
		self->priv->handle = metadata_get_list (ss, self->priv->query_type, _song_window_callback_gmpc_meta_data_callback, self);
		g_object_set ((GtkWidget*) self->priv->refresh, "sensitive", FALSE, NULL);
	}
	(ss == NULL) ? NULL : (ss = (mpd_freeSong (ss), NULL));
}


static void _song_window_destroy_popup_gtk_button_clicked (GtkButton* _sender, gpointer self) {
	song_window_destroy_popup (self, _sender);
}


static void _song_window_set_metadata_gtk_button_clicked (GtkButton* _sender, gpointer self) {
	song_window_set_metadata (self, _sender);
}


static void _song_window_refresh_query_gtk_button_clicked (GtkButton* _sender, gpointer self) {
	song_window_refresh_query (self, _sender);
}


static SongWindow* song_window_construct (GType object_type, const mpd_Song* song, MetaDataType type) {
	SongWindow * self;
	GtkVBox* vbox;
	mpd_Song* _tmp1;
	const mpd_Song* _tmp0;
	GtkListStore* _tmp2;
	GtkScrolledWindow* sw;
	GtkTreeView* iv;
	GtkTreeView* _tmp4;
	GtkTreeView* _tmp3;
	GtkCellRendererPixbuf* renderer;
	GtkTreeViewColumn* column;
	GtkCellRendererText* rendererpb;
	GtkTreeViewColumn* _tmp5;
	GtkHBox* hbox;
	GtkButton* button;
	GtkButton* _tmp6;
	GtkSizeGroup* group;
	GtkHBox* qhbox;
	GtkLabel* label;
	GtkEntry* _tmp7;
	GtkButton* _tmp13;
	GtkButton* _tmp12;
	GtkButton* _tmp11;
	GtkAlignment* ali;
	g_return_val_if_fail (song != NULL, NULL);
	self = g_object_newv (object_type, 0, NULL);
	vbox = g_object_ref_sink ((GtkVBox*) gtk_vbox_new (FALSE, 6));
	_tmp1 = NULL;
	_tmp0 = NULL;
	self->priv->song = (_tmp1 = (_tmp0 = song, (_tmp0 == NULL) ? NULL : mpd_songDup (_tmp0)), (self->priv->song == NULL) ? NULL : (self->priv->song = (mpd_freeSong (self->priv->song), NULL)), _tmp1);
	self->priv->query_type = type;
	_tmp2 = NULL;
	self->priv->model = (_tmp2 = gtk_list_store_new (3, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_STRING, NULL), (self->priv->model == NULL) ? NULL : (self->priv->model = (g_object_unref (self->priv->model), NULL)), _tmp2);
	sw = g_object_ref_sink ((GtkScrolledWindow*) gtk_scrolled_window_new (NULL, NULL));
	iv = g_object_ref_sink ((GtkTreeView*) gtk_tree_view_new ());
	_tmp4 = NULL;
	_tmp3 = NULL;
	self->priv->tree = (_tmp4 = (_tmp3 = iv, (_tmp3 == NULL) ? NULL : g_object_ref (_tmp3)), (self->priv->tree == NULL) ? NULL : (self->priv->tree = (g_object_unref (self->priv->tree), NULL)), _tmp4);
	gtk_scrolled_window_set_policy (sw, GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (sw, GTK_SHADOW_ETCHED_IN);
	gtk_tree_view_set_model (iv, (GtkTreeModel*) self->priv->model);
	renderer = g_object_ref_sink ((GtkCellRendererPixbuf*) gtk_cell_renderer_pixbuf_new ());
	column = g_object_ref_sink (gtk_tree_view_column_new ());
	gtk_cell_layout_pack_start ((GtkCellLayout*) column, (GtkCellRenderer*) renderer, FALSE);
	gtk_tree_view_append_column (iv, column);
	gtk_tree_view_column_set_title (column, _ ("Cover"));
	gtk_cell_layout_add_attribute ((GtkCellLayout*) column, (GtkCellRenderer*) renderer, "pixbuf", 0);
	rendererpb = g_object_ref_sink ((GtkCellRendererText*) gtk_cell_renderer_text_new ());
	_tmp5 = NULL;
	column = (_tmp5 = g_object_ref_sink (gtk_tree_view_column_new ()), (column == NULL) ? NULL : (column = (g_object_unref (column), NULL)), _tmp5);
	gtk_cell_layout_pack_start ((GtkCellLayout*) column, (GtkCellRenderer*) rendererpb, TRUE);
	gtk_tree_view_append_column (iv, column);
	gtk_tree_view_column_set_title (column, _ ("Url"));
	gtk_cell_layout_add_attribute ((GtkCellLayout*) column, (GtkCellRenderer*) rendererpb, "markup", 2);
	gtk_box_pack_start ((GtkBox*) vbox, (GtkWidget*) sw, TRUE, TRUE, (guint) 0);
	hbox = g_object_ref_sink ((GtkHBox*) gtk_hbox_new (FALSE, 6));
	button = g_object_ref_sink ((GtkButton*) gtk_button_new_from_stock ("gtk-quit"));
	g_signal_connect_object (button, "clicked", (GCallback) _song_window_destroy_popup_gtk_button_clicked, self, 0);
	gtk_box_pack_end ((GtkBox*) hbox, (GtkWidget*) button, FALSE, FALSE, (guint) 0);
	_tmp6 = NULL;
	button = (_tmp6 = g_object_ref_sink ((GtkButton*) gtk_button_new_with_label ("Set cover")), (button == NULL) ? NULL : (button = (g_object_unref (button), NULL)), _tmp6);
	g_signal_connect_object (button, "clicked", (GCallback) _song_window_set_metadata_gtk_button_clicked, self, 0);
	gtk_box_pack_end ((GtkBox*) hbox, (GtkWidget*) button, FALSE, FALSE, (guint) 0);
	gtk_box_pack_end ((GtkBox*) vbox, (GtkWidget*) hbox, FALSE, FALSE, (guint) 0);
	group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
	qhbox = g_object_ref_sink ((GtkHBox*) gtk_hbox_new (FALSE, 6));
	label = g_object_ref_sink ((GtkLabel*) gtk_label_new (_ ("Artist")));
	gtk_size_group_add_widget (group, (GtkWidget*) label);
	gtk_box_pack_start ((GtkBox*) qhbox, (GtkWidget*) label, FALSE, FALSE, (guint) 0);
	_tmp7 = NULL;
	self->priv->artist_entry = (_tmp7 = g_object_ref_sink ((GtkEntry*) gtk_entry_new ()), (self->priv->artist_entry == NULL) ? NULL : (self->priv->artist_entry = (g_object_unref (self->priv->artist_entry), NULL)), _tmp7);
	gtk_entry_set_text (self->priv->artist_entry, song->artist);
	gtk_box_pack_start ((GtkBox*) qhbox, (GtkWidget*) self->priv->artist_entry, TRUE, TRUE, (guint) 0);
	gtk_box_pack_start ((GtkBox*) vbox, (GtkWidget*) qhbox, FALSE, FALSE, (guint) 0);
	if (type == META_ALBUM_ART) {
		GtkHBox* _tmp8;
		GtkLabel* _tmp9;
		GtkEntry* _tmp10;
		_tmp8 = NULL;
		qhbox = (_tmp8 = g_object_ref_sink ((GtkHBox*) gtk_hbox_new (FALSE, 6)), (qhbox == NULL) ? NULL : (qhbox = (g_object_unref (qhbox), NULL)), _tmp8);
		_tmp9 = NULL;
		label = (_tmp9 = g_object_ref_sink ((GtkLabel*) gtk_label_new (_ ("Album"))), (label == NULL) ? NULL : (label = (g_object_unref (label), NULL)), _tmp9);
		gtk_size_group_add_widget (group, (GtkWidget*) label);
		gtk_box_pack_start ((GtkBox*) qhbox, (GtkWidget*) label, FALSE, FALSE, (guint) 0);
		_tmp10 = NULL;
		self->priv->album_entry = (_tmp10 = g_object_ref_sink ((GtkEntry*) gtk_entry_new ()), (self->priv->album_entry == NULL) ? NULL : (self->priv->album_entry = (g_object_unref (self->priv->album_entry), NULL)), _tmp10);
		gtk_entry_set_text (self->priv->album_entry, song->album);
		gtk_box_pack_start ((GtkBox*) qhbox, (GtkWidget*) self->priv->album_entry, TRUE, TRUE, (guint) 0);
		gtk_box_pack_start ((GtkBox*) vbox, (GtkWidget*) qhbox, FALSE, FALSE, (guint) 0);
	}
	_tmp13 = NULL;
	_tmp12 = NULL;
	_tmp11 = NULL;
	self->priv->refresh = (_tmp13 = (_tmp12 = button = (_tmp11 = g_object_ref_sink ((GtkButton*) gtk_button_new_from_stock ("gtk-refresh")), (button == NULL) ? NULL : (button = (g_object_unref (button), NULL)), _tmp11), (_tmp12 == NULL) ? NULL : g_object_ref (_tmp12)), (self->priv->refresh == NULL) ? NULL : (self->priv->refresh = (g_object_unref (self->priv->refresh), NULL)), _tmp13);
	ali = g_object_ref_sink ((GtkAlignment*) gtk_alignment_new (1.0f, 0.5f, 0.0f, 0.0f));
	gtk_container_add ((GtkContainer*) ali, (GtkWidget*) button);
	gtk_box_pack_start ((GtkBox*) vbox, (GtkWidget*) ali, FALSE, FALSE, (guint) 0);
	g_signal_connect_object (button, "clicked", (GCallback) _song_window_refresh_query_gtk_button_clicked, self, 0);
	g_object_set ((GtkWidget*) self->priv->refresh, "sensitive", FALSE, NULL);
	gtk_container_add ((GtkContainer*) self, (GtkWidget*) vbox);
	gtk_widget_hide_on_delete ((GtkWidget*) self);
	gtk_container_add ((GtkContainer*) sw, (GtkWidget*) iv);
	gtk_widget_show_all ((GtkWidget*) self);
	self->priv->handle = metadata_get_list (song, self->priv->query_type, _song_window_callback_gmpc_meta_data_callback, self);
	fprintf (stdout, "Query 1\n");
	if (self->priv->song->albumartist != NULL) {
		const mpd_Song* _tmp14;
		mpd_Song* song2;
		char* _tmp16;
		const char* _tmp15;
		_tmp14 = NULL;
		song2 = (_tmp14 = song, (_tmp14 == NULL) ? NULL : mpd_songDup (_tmp14));
		_tmp16 = NULL;
		_tmp15 = NULL;
		song2->artist = (_tmp16 = (_tmp15 = song2->albumartist, (_tmp15 == NULL) ? NULL : g_strdup (_tmp15)), song2->artist = (g_free (song2->artist), NULL), _tmp16);
		fprintf (stdout, "query 2\n");
		self->priv->handle2 = metadata_get_list (song2, self->priv->query_type, _song_window_callback_gmpc_meta_data_callback, self);
		(song2 == NULL) ? NULL : (song2 = (mpd_freeSong (song2), NULL));
	}
	return self;
}


static SongWindow* song_window_new (const mpd_Song* song, MetaDataType type) {
	return song_window_construct (TYPE_SONG_WINDOW, song, type);
}


static GObject * song_window_constructor (GType type, guint n_construct_properties, GObjectConstructParam * construct_properties) {
	GObject * obj;
	SongWindowClass * klass;
	GObjectClass * parent_class;
	SongWindow * self;
	klass = SONG_WINDOW_CLASS (g_type_class_peek (TYPE_SONG_WINDOW));
	parent_class = G_OBJECT_CLASS (g_type_class_peek_parent (klass));
	obj = parent_class->constructor (type, n_construct_properties, construct_properties);
	self = SONG_WINDOW (obj);
	{
		g_object_set ((GtkWindow*) self, "type", GTK_WINDOW_TOPLEVEL, NULL);
		gtk_window_set_default_size ((GtkWindow*) self, 650, 800);
		gtk_container_set_border_width ((GtkContainer*) self, (guint) 8);
	}
	return obj;
}


static void song_window_class_init (SongWindowClass * klass) {
	song_window_parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (SongWindowPrivate));
	G_OBJECT_CLASS (klass)->constructor = song_window_constructor;
	G_OBJECT_CLASS (klass)->finalize = song_window_finalize;
}


static void song_window_instance_init (SongWindow * self) {
	self->priv = SONG_WINDOW_GET_PRIVATE (self);
	self->priv->model = NULL;
	self->priv->tree = NULL;
	self->priv->downloads = NULL;
	self->priv->song = NULL;
	self->priv->query_type = META_ALBUM_ART;
	self->priv->handle = NULL;
	self->priv->handle2 = NULL;
	self->priv->refresh = NULL;
}


static void song_window_finalize (GObject* obj) {
	SongWindow * self;
	self = SONG_WINDOW (obj);
	{
		if (self->priv->handle != NULL) {
			fprintf (stdout, "cancel 1\n");
			metadata_get_list_cancel (self->priv->handle);
			self->priv->handle = NULL;
		}
		if (self->priv->handle2 != NULL) {
			fprintf (stdout, "cancel 2\n");
			metadata_get_list_cancel (self->priv->handle2);
			self->priv->handle2 = NULL;
		}
		g_list_first (self->priv->downloads);
		while (self->priv->downloads != NULL) {
			GEADAsyncHandler* handle;
			handle = (const GEADAsyncHandler*) self->priv->downloads->data;
			fprintf (stdout, "cancel download: %s\n", gmpc_easy_handler_get_uri (handle));
			gmpc_easy_async_cancel (handle);
			g_list_first (self->priv->downloads);
		}
		fprintf (stdout, "song window destroy\n");
	}
	(self->priv->model == NULL) ? NULL : (self->priv->model = (g_object_unref (self->priv->model), NULL));
	(self->priv->tree == NULL) ? NULL : (self->priv->tree = (g_object_unref (self->priv->tree), NULL));
	(self->priv->downloads == NULL) ? NULL : (self->priv->downloads = (g_list_free (self->priv->downloads), NULL));
	(self->priv->song == NULL) ? NULL : (self->priv->song = (mpd_freeSong (self->priv->song), NULL));
	(self->priv->artist_entry == NULL) ? NULL : (self->priv->artist_entry = (g_object_unref (self->priv->artist_entry), NULL));
	(self->priv->album_entry == NULL) ? NULL : (self->priv->album_entry = (g_object_unref (self->priv->album_entry), NULL));
	(self->priv->refresh == NULL) ? NULL : (self->priv->refresh = (g_object_unref (self->priv->refresh), NULL));
	G_OBJECT_CLASS (song_window_parent_class)->finalize (obj);
}


GType song_window_get_type (void) {
	static GType song_window_type_id = 0;
	if (song_window_type_id == 0) {
		static const GTypeInfo g_define_type_info = { sizeof (SongWindowClass), (GBaseInitFunc) NULL, (GBaseFinalizeFunc) NULL, (GClassInitFunc) song_window_class_init, (GClassFinalizeFunc) NULL, NULL, sizeof (SongWindow), 0, (GInstanceInitFunc) song_window_instance_init, NULL };
		song_window_type_id = g_type_register_static (GTK_TYPE_WINDOW, "SongWindow", &g_define_type_info, 0);
	}
	return song_window_type_id;
}


/*********************************************************************************
     * Plugin base functions 
     * These functions are required.
     ********************************************************************************/
static gint* gmpc_test_plugin_real_get_version (GmpcPluginBase* base, int* result_length1) {
	GmpcTestPlugin * self;
	gint* _tmp0;
	self = (GmpcTestPlugin*) base;
	_tmp0 = NULL;
	return (_tmp0 = (gint*) GMPC_TEST_PLUGIN_version, *result_length1 = -1, _tmp0);
}


/**
     * The name of the plugin
     */
static const char* gmpc_test_plugin_real_get_name (GmpcPluginBase* base) {
	GmpcTestPlugin * self;
	self = (GmpcTestPlugin*) base;
	return "Vala test plugin";
}


/**
     * Tells the plugin to save itself
     */
static void gmpc_test_plugin_real_save_yourself (GmpcPluginBase* base) {
	GmpcTestPlugin * self;
	self = (GmpcTestPlugin*) base;
}


/* nothing to save 
*
     * Get set enabled
     */
static gboolean gmpc_test_plugin_real_get_enabled (GmpcPluginBase* base) {
	GmpcTestPlugin * self;
	self = (GmpcTestPlugin*) base;
	return (gboolean) cfg_get_single_value_as_int_with_default (config, gmpc_plugin_base_get_name ((GmpcPluginBase*) self), "enabled", 1);
}


static void gmpc_test_plugin_real_set_enabled (GmpcPluginBase* base, gboolean state) {
	GmpcTestPlugin * self;
	self = (GmpcTestPlugin*) base;
	cfg_set_single_value_as_int (config, gmpc_plugin_base_get_name ((GmpcPluginBase*) self), "enabled", (gint) state);
}


/*********************************************************************************
     * Plugin preferences functions 
     ********************************************************************************/
static void gmpc_test_plugin_real_pane_construct (GmpcPluginPreferencesIface* base, GtkContainer* container) {
	GmpcTestPlugin * self;
	GtkHBox* box;
	GtkLabel* label;
	self = (GmpcTestPlugin*) base;
	g_return_if_fail (container != NULL);
	box = g_object_ref_sink ((GtkHBox*) gtk_hbox_new (FALSE, 6));
	label = g_object_ref_sink ((GtkLabel*) gtk_label_new ("This is a test preferences pane"));
	gtk_box_pack_start ((GtkBox*) box, (GtkWidget*) label, FALSE, FALSE, (guint) 0);
	gtk_container_add (container, (GtkWidget*) box);
	gtk_widget_show_all ((GtkWidget*) container);
	fprintf (stdout, "%s: Create preferences panel\n", gmpc_plugin_base_get_name ((GmpcPluginBase*) self));
	(box == NULL) ? NULL : (box = (g_object_unref (box), NULL));
	(label == NULL) ? NULL : (label = (g_object_unref (label), NULL));
}


static void gmpc_test_plugin_real_pane_destroy (GmpcPluginPreferencesIface* base, GtkContainer* container) {
	GmpcTestPlugin * self;
	GtkBin* _tmp0;
	GtkBin* bin;
	self = (GmpcTestPlugin*) base;
	g_return_if_fail (container != NULL);
	_tmp0 = NULL;
	bin = (_tmp0 = GTK_BIN (container), (_tmp0 == NULL) ? NULL : g_object_ref (_tmp0));
	gtk_object_destroy ((GtkObject*) bin->child);
	(bin == NULL) ? NULL : (bin = (g_object_unref (bin), NULL));
}


/*********************************************************************************
     * Private  
     ********************************************************************************/
void gmpc_test_plugin_menu_activated_album (GmpcTestPlugin* self, GtkMenuItem* item) {
	const mpd_Song* song;
	SongWindow* _tmp0;
	g_return_if_fail (self != NULL);
	g_return_if_fail (item != NULL);
	song = mpd_playlist_get_current_song (connection);
	_tmp0 = NULL;
	_tmp0 = g_object_ref_sink (song_window_new (song, META_ALBUM_ART));
	(_tmp0 == NULL) ? NULL : (_tmp0 = (g_object_unref (_tmp0), NULL));
}


void gmpc_test_plugin_menu_activated_artist (GmpcTestPlugin* self, GtkMenuItem* item) {
	const mpd_Song* song;
	SongWindow* _tmp0;
	g_return_if_fail (self != NULL);
	g_return_if_fail (item != NULL);
	song = mpd_playlist_get_current_song (connection);
	_tmp0 = NULL;
	_tmp0 = g_object_ref_sink (song_window_new (song, META_ARTIST_ART));
	(_tmp0 == NULL) ? NULL : (_tmp0 = (g_object_unref (_tmp0), NULL));
}


static void _gmpc_test_plugin_menu_activated_album_gtk_menu_item_activate (GtkMenuItem* _sender, gpointer self) {
	gmpc_test_plugin_menu_activated_album (self, _sender);
}


static void _gmpc_test_plugin_menu_activated_artist_gtk_menu_item_activate (GtkMenuItem* _sender, gpointer self) {
	gmpc_test_plugin_menu_activated_artist (self, _sender);
}


static gint gmpc_test_plugin_real_tool_menu_integration (GmpcPluginToolMenuIface* base, GtkMenu* menu) {
	GmpcTestPlugin * self;
	GtkMenuItem* item;
	GtkMenuItem* _tmp0;
	gint _tmp1;
	self = (GmpcTestPlugin*) base;
	g_return_val_if_fail (menu != NULL, 0);
	item = g_object_ref_sink ((GtkMenuItem*) gtk_menu_item_new_with_label ("Test plugin album"));
	gtk_menu_shell_append ((GtkMenuShell*) menu, (GtkWidget*) item);
	g_signal_connect_object (item, "activate", (GCallback) _gmpc_test_plugin_menu_activated_album_gtk_menu_item_activate, self, 0);
	_tmp0 = NULL;
	item = (_tmp0 = g_object_ref_sink ((GtkMenuItem*) gtk_menu_item_new_with_label ("Test plugin artist")), (item == NULL) ? NULL : (item = (g_object_unref (item), NULL)), _tmp0);
	gtk_menu_shell_append ((GtkMenuShell*) menu, (GtkWidget*) item);
	g_signal_connect_object (item, "activate", (GCallback) _gmpc_test_plugin_menu_activated_artist_gtk_menu_item_activate, self, 0);
	return (_tmp1 = 2, (item == NULL) ? NULL : (item = (g_object_unref (item), NULL)), _tmp1);
}


GmpcTestPlugin* gmpc_test_plugin_construct (GType object_type) {
	GmpcTestPlugin * self;
	self = g_object_newv (object_type, 0, NULL);
	return self;
}


GmpcTestPlugin* gmpc_test_plugin_new (void) {
	return gmpc_test_plugin_construct (GMPC_TYPE_TEST_PLUGIN);
}


static void gmpc_test_plugin_class_init (GmpcTestPluginClass * klass) {
	gmpc_test_plugin_parent_class = g_type_class_peek_parent (klass);
	GMPC_PLUGIN_BASE_CLASS (klass)->get_version = gmpc_test_plugin_real_get_version;
	GMPC_PLUGIN_BASE_CLASS (klass)->get_name = gmpc_test_plugin_real_get_name;
	GMPC_PLUGIN_BASE_CLASS (klass)->save_yourself = gmpc_test_plugin_real_save_yourself;
	GMPC_PLUGIN_BASE_CLASS (klass)->get_enabled = gmpc_test_plugin_real_get_enabled;
	GMPC_PLUGIN_BASE_CLASS (klass)->set_enabled = gmpc_test_plugin_real_set_enabled;
}


static void gmpc_test_plugin_gmpc_plugin_preferences_iface_interface_init (GmpcPluginPreferencesIfaceIface * iface) {
	gmpc_test_plugin_gmpc_plugin_preferences_iface_parent_iface = g_type_interface_peek_parent (iface);
	iface->pane_construct = gmpc_test_plugin_real_pane_construct;
	iface->pane_destroy = gmpc_test_plugin_real_pane_destroy;
}


static void gmpc_test_plugin_gmpc_plugin_tool_menu_iface_interface_init (GmpcPluginToolMenuIfaceIface * iface) {
	gmpc_test_plugin_gmpc_plugin_tool_menu_iface_parent_iface = g_type_interface_peek_parent (iface);
	iface->tool_menu_integration = gmpc_test_plugin_real_tool_menu_integration;
}


static void gmpc_test_plugin_instance_init (GmpcTestPlugin * self) {
}


GType gmpc_test_plugin_get_type (void) {
	static GType gmpc_test_plugin_type_id = 0;
	if (gmpc_test_plugin_type_id == 0) {
		static const GTypeInfo g_define_type_info = { sizeof (GmpcTestPluginClass), (GBaseInitFunc) NULL, (GBaseFinalizeFunc) NULL, (GClassInitFunc) gmpc_test_plugin_class_init, (GClassFinalizeFunc) NULL, NULL, sizeof (GmpcTestPlugin), 0, (GInstanceInitFunc) gmpc_test_plugin_instance_init, NULL };
		static const GInterfaceInfo gmpc_plugin_preferences_iface_info = { (GInterfaceInitFunc) gmpc_test_plugin_gmpc_plugin_preferences_iface_interface_init, (GInterfaceFinalizeFunc) NULL, NULL};
		static const GInterfaceInfo gmpc_plugin_tool_menu_iface_info = { (GInterfaceInitFunc) gmpc_test_plugin_gmpc_plugin_tool_menu_iface_interface_init, (GInterfaceFinalizeFunc) NULL, NULL};
		gmpc_test_plugin_type_id = g_type_register_static (GMPC_PLUGIN_TYPE_BASE, "GmpcTestPlugin", &g_define_type_info, 0);
		g_type_add_interface_static (gmpc_test_plugin_type_id, GMPC_PLUGIN_TYPE_PREFERENCES_IFACE, &gmpc_plugin_preferences_iface_info);
		g_type_add_interface_static (gmpc_test_plugin_type_id, GMPC_PLUGIN_TYPE_TOOL_MENU_IFACE, &gmpc_plugin_tool_menu_iface_info);
	}
	return gmpc_test_plugin_type_id;
}




