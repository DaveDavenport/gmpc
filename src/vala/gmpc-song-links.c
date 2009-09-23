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
#include <libmpd/libmpdclient.h>
#include <libmpd/libmpd.h>
#include <gmpc_easy_download.h>
#include <config.h>
#include <glib/gi18n-lib.h>
#include <gdk/gdk.h>
#include <stdlib.h>
#include <string.h>
#include <plugin.h>
#include <glib/gstdio.h>
#include <stdio.h>


#define GMPC_SONG_TYPE_LINKS (gmpc_song_links_get_type ())
#define GMPC_SONG_LINKS(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GMPC_SONG_TYPE_LINKS, GmpcSongLinks))
#define GMPC_SONG_LINKS_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GMPC_SONG_TYPE_LINKS, GmpcSongLinksClass))
#define GMPC_SONG_IS_LINKS(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GMPC_SONG_TYPE_LINKS))
#define GMPC_SONG_IS_LINKS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GMPC_SONG_TYPE_LINKS))
#define GMPC_SONG_LINKS_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GMPC_SONG_TYPE_LINKS, GmpcSongLinksClass))

typedef struct _GmpcSongLinks GmpcSongLinks;
typedef struct _GmpcSongLinksClass GmpcSongLinksClass;
typedef struct _GmpcSongLinksPrivate GmpcSongLinksPrivate;

#define GMPC_SONG_LINKS_TYPE_TYPE (gmpc_song_links_type_get_type ())
#define _mpd_freeSong0(var) ((var == NULL) ? NULL : (var = (mpd_freeSong (var), NULL)))
#define _g_object_unref0(var) ((var == NULL) ? NULL : (var = (g_object_unref (var), NULL)))
#define _g_free0(var) (var = (g_free (var), NULL))
#define _g_error_free0(var) ((var == NULL) ? NULL : (var = (g_error_free (var), NULL)))
#define _g_key_file_free0(var) ((var == NULL) ? NULL : (var = (g_key_file_free (var), NULL)))
#define _g_regex_unref0(var) ((var == NULL) ? NULL : (var = (g_regex_unref (var), NULL)))

struct _GmpcSongLinks {
	GtkFrame parent_instance;
	GmpcSongLinksPrivate * priv;
};

struct _GmpcSongLinksClass {
	GtkFrameClass parent_class;
};

typedef enum  {
	GMPC_SONG_LINKS_TYPE_ARTIST,
	GMPC_SONG_LINKS_TYPE_ALBUM,
	GMPC_SONG_LINKS_TYPE_SONG
} GmpcSongLinksType;

struct _GmpcSongLinksPrivate {
	GmpcSongLinksType type;
	mpd_Song* song;
	GEADAsyncHandler* handle;
};


static gpointer gmpc_song_links_parent_class = NULL;

#define use_transition TRUE
GType gmpc_song_links_get_type (void);
GType gmpc_song_links_type_get_type (void);
#define GMPC_SONG_LINKS_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GMPC_SONG_TYPE_LINKS, GmpcSongLinksPrivate))
enum  {
	GMPC_SONG_LINKS_DUMMY_PROPERTY
};
#define GMPC_SONG_LINKS_some_unique_name VERSION
static void gmpc_song_links_download_file (GmpcSongLinks* self, const GEADAsyncHandler* handle, GEADStatus status);
static void _gmpc_song_links_download_file_gmpc_async_download_callback (const GEADAsyncHandler* handle, GEADStatus status, gpointer self);
static void gmpc_song_links_download (GmpcSongLinks* self, GtkImageMenuItem* item);
static void _gmpc_song_links_download_gtk_menu_item_activate (GtkImageMenuItem* _sender, gpointer self);
static gboolean gmpc_song_links_button_press_event_callback (GmpcSongLinks* self, GtkEventBox* label, const GdkEventButton* event);
static gboolean _gmpc_song_links_button_press_event_callback_gtk_widget_button_press_event (GtkEventBox* _sender, const GdkEventButton* event, gpointer self);
static void gmpc_song_links_parse_uris (GmpcSongLinks* self);
GmpcSongLinks* gmpc_song_links_new (GmpcSongLinksType type, const mpd_Song* song);
GmpcSongLinks* gmpc_song_links_construct (GType object_type, GmpcSongLinksType type, const mpd_Song* song);
static guchar* _vala_array_dup1 (guchar* self, int length);
static void gmpc_song_links_finalize (GObject* obj);
static void _vala_array_destroy (gpointer array, gint array_length, GDestroyNotify destroy_func);
static void _vala_array_free (gpointer array, gint array_length, GDestroyNotify destroy_func);
static gint _vala_array_length (gpointer array);




GType gmpc_song_links_type_get_type (void) {
	static GType gmpc_song_links_type_type_id = 0;
	if (G_UNLIKELY (gmpc_song_links_type_type_id == 0)) {
		static const GEnumValue values[] = {{GMPC_SONG_LINKS_TYPE_ARTIST, "GMPC_SONG_LINKS_TYPE_ARTIST", "artist"}, {GMPC_SONG_LINKS_TYPE_ALBUM, "GMPC_SONG_LINKS_TYPE_ALBUM", "album"}, {GMPC_SONG_LINKS_TYPE_SONG, "GMPC_SONG_LINKS_TYPE_SONG", "song"}, {0, NULL, NULL}};
		gmpc_song_links_type_type_id = g_enum_register_static ("GmpcSongLinksType", values);
	}
	return gmpc_song_links_type_type_id;
}


static gpointer _g_object_ref0 (gpointer self) {
	return self ? g_object_ref (self) : NULL;
}


static void _gmpc_song_links_download_file_gmpc_async_download_callback (const GEADAsyncHandler* handle, GEADStatus status, gpointer self) {
	gmpc_song_links_download_file (self, handle, status);
}


static void gmpc_song_links_download (GmpcSongLinks* self, GtkImageMenuItem* item) {
	GtkWidget* child;
	GtkProgressBar* _tmp0_;
	g_return_if_fail (self != NULL);
	g_return_if_fail (item != NULL);
	child = _g_object_ref0 (gtk_bin_get_child ((GtkBin*) self));
	if (child != NULL) {
		gtk_object_destroy ((GtkObject*) child);
	}
	gtk_container_add ((GtkContainer*) self, (GtkWidget*) (_tmp0_ = g_object_ref_sink ((GtkProgressBar*) gtk_progress_bar_new ())));
	_g_object_unref0 (_tmp0_);
	gtk_widget_show_all ((GtkWidget*) self);
	self->priv->handle = gmpc_easy_async_downloader ("http://gmpc.wikia.com/index.php?title=GMPC_METADATA_WEBLINKLIST&action=raw", _gmpc_song_links_download_file_gmpc_async_download_callback, g_object_ref (self));
	_g_object_unref0 (child);
}


static void _gmpc_song_links_download_gtk_menu_item_activate (GtkImageMenuItem* _sender, gpointer self) {
	gmpc_song_links_download (self, _sender);
}


static gboolean gmpc_song_links_button_press_event_callback (GmpcSongLinks* self, GtkEventBox* label, const GdkEventButton* event) {
	gboolean result;
	g_return_val_if_fail (self != NULL, FALSE);
	g_return_val_if_fail (label != NULL, FALSE);
	if ((*event).button == 3) {
		GtkMenu* menu;
		GtkImageMenuItem* item;
		GtkImage* _tmp0_;
		menu = g_object_ref_sink ((GtkMenu*) gtk_menu_new ());
		item = g_object_ref_sink ((GtkImageMenuItem*) gtk_image_menu_item_new_with_label (_ ("Update list from internet")));
		gtk_image_menu_item_set_image (item, (GtkWidget*) (_tmp0_ = g_object_ref_sink ((GtkImage*) gtk_image_new_from_stock (GTK_STOCK_REFRESH, GTK_ICON_SIZE_MENU))));
		_g_object_unref0 (_tmp0_);
		g_signal_connect_object ((GtkMenuItem*) item, "activate", (GCallback) _gmpc_song_links_download_gtk_menu_item_activate, self, 0);
		gtk_menu_shell_append ((GtkMenuShell*) menu, (GtkWidget*) ((GtkMenuItem*) item));
		gtk_widget_show_all ((GtkWidget*) menu);
		gtk_menu_popup (menu, NULL, NULL, NULL, NULL, (*event).button, (*event).time);
		_g_object_unref0 (menu);
		_g_object_unref0 (item);
	}
	result = FALSE;
	return result;
}


static gpointer _mpd_songDup0 (gpointer self) {
	return self ? mpd_songDup (self) : NULL;
}


static gboolean _gmpc_song_links_button_press_event_callback_gtk_widget_button_press_event (GtkEventBox* _sender, const GdkEventButton* event, gpointer self) {
	return gmpc_song_links_button_press_event_callback (self, _sender, event);
}


GmpcSongLinks* gmpc_song_links_construct (GType object_type, GmpcSongLinksType type, const mpd_Song* song) {
	GmpcSongLinks * self;
	mpd_Song* _tmp0_;
	GtkEventBox* event;
	GtkLabel* label;
	char* _tmp1_;
	g_return_val_if_fail (song != NULL, NULL);
	self = g_object_newv (object_type, 0, NULL);
	self->priv->type = type;
	self->priv->song = (_tmp0_ = _mpd_songDup0 (song), _mpd_freeSong0 (self->priv->song), _tmp0_);
	event = g_object_ref_sink ((GtkEventBox*) gtk_event_box_new ());
	label = g_object_ref_sink ((GtkLabel*) gtk_label_new (""));
	gtk_container_add ((GtkContainer*) event, (GtkWidget*) label);
	gtk_event_box_set_visible_window (event, FALSE);
	gtk_frame_set_label_widget ((GtkFrame*) self, (GtkWidget*) event);
	gtk_label_set_markup (label, _tmp1_ = g_strdup_printf ("<b>%s:</b>", _ ("Web Links")));
	_g_free0 (_tmp1_);
	g_object_set ((GtkFrame*) self, "shadow", GTK_SHADOW_NONE, NULL);
	g_signal_connect_object ((GtkWidget*) event, "button-press-event", (GCallback) _gmpc_song_links_button_press_event_callback_gtk_widget_button_press_event, self, 0);
	gmpc_song_links_parse_uris (self);
	_g_object_unref0 (event);
	_g_object_unref0 (label);
	return self;
}


GmpcSongLinks* gmpc_song_links_new (GmpcSongLinksType type, const mpd_Song* song) {
	return gmpc_song_links_construct (GMPC_SONG_TYPE_LINKS, type, song);
}


static guchar* _vala_array_dup1 (guchar* self, int length) {
	return g_memdup (self, length * sizeof (guchar));
}


static glong string_get_length (const char* self) {
	glong result;
	g_return_val_if_fail (self != NULL, 0L);
	result = g_utf8_strlen (self, -1);
	return result;
}


static void gmpc_song_links_download_file (GmpcSongLinks* self, const GEADAsyncHandler* handle, GEADStatus status) {
	GError * _inner_error_;
	g_return_if_fail (self != NULL);
	g_return_if_fail (handle != NULL);
	_inner_error_ = NULL;
	if (status == GEAD_PROGRESS) {
		GtkProgressBar* pb;
		pb = _g_object_ref0 (GTK_PROGRESS_BAR (gtk_bin_get_child ((GtkBin*) self)));
		if (pb != NULL) {
			gtk_progress_bar_pulse (pb);
		}
		_g_object_unref0 (pb);
		return;
	}
	if (status == GEAD_DONE) {
		guchar* _tmp2_;
		gint a_size;
		gint a_length1;
		guchar* _tmp1_;
		gint _tmp0_;
		guchar* a;
		char* path;
		a = (_tmp2_ = (_tmp1_ = gmpc_easy_handler_get_data_vala_wrap (handle, &_tmp0_), (_tmp1_ == NULL) ? ((gpointer) _tmp1_) : _vala_array_dup1 (_tmp1_, _tmp0_)), a_length1 = _tmp0_, a_size = a_length1, _tmp2_);
		path = gmpc_get_user_path ("weblinks.list");
		{
			g_file_set_contents (path, (const char*) a, (glong) a_length1, &_inner_error_);
			if (_inner_error_ != NULL) {
				goto __catch0_g_error;
				goto __finally0;
			}
			gmpc_song_links_parse_uris (self);
			gtk_widget_show_all ((GtkWidget*) self);
		}
		goto __finally0;
		__catch0_g_error:
		{
			GError * e;
			e = _inner_error_;
			_inner_error_ = NULL;
			{
				fprintf (stdout, "Error: %s\n", e->message);
				_g_error_free0 (e);
			}
		}
		__finally0:
		if (_inner_error_ != NULL) {
			a = (g_free (a), NULL);
			_g_free0 (path);
			g_critical ("file %s: line %d: uncaught error: %s", __FILE__, __LINE__, _inner_error_->message);
			g_clear_error (&_inner_error_);
			return;
		}
		a = (g_free (a), NULL);
		_g_free0 (path);
	} else {
		char* path;
		path = gmpc_get_user_path ("weblinks.list");
		{
			char* a;
			a = g_strdup (" ");
			g_file_set_contents (path, a, string_get_length (a), &_inner_error_);
			if (_inner_error_ != NULL) {
				_g_free0 (a);
				goto __catch1_g_error;
				goto __finally1;
			}
			gmpc_song_links_parse_uris (self);
			gtk_widget_show_all ((GtkWidget*) self);
			_g_free0 (a);
		}
		goto __finally1;
		__catch1_g_error:
		{
			GError * e;
			e = _inner_error_;
			_inner_error_ = NULL;
			{
				fprintf (stdout, "Error: %s\n", e->message);
				_g_error_free0 (e);
			}
		}
		__finally1:
		if (_inner_error_ != NULL) {
			_g_free0 (path);
			g_critical ("file %s: line %d: uncaught error: %s", __FILE__, __LINE__, _inner_error_->message);
			g_clear_error (&_inner_error_);
			return;
		}
		_g_free0 (path);
	}
	self->priv->handle = NULL;
}


static char* string_replace (const char* self, const char* old, const char* replacement) {
	char* result;
	GError * _inner_error_;
	g_return_val_if_fail (self != NULL, NULL);
	g_return_val_if_fail (old != NULL, NULL);
	g_return_val_if_fail (replacement != NULL, NULL);
	_inner_error_ = NULL;
	{
		char* _tmp0_;
		GRegex* _tmp1_;
		GRegex* regex;
		char* _tmp2_;
		regex = (_tmp1_ = g_regex_new (_tmp0_ = g_regex_escape_string (old, -1), 0, 0, &_inner_error_), _g_free0 (_tmp0_), _tmp1_);
		if (_inner_error_ != NULL) {
			if (_inner_error_->domain == G_REGEX_ERROR) {
				goto __catch4_g_regex_error;
			}
			goto __finally4;
		}
		_tmp2_ = g_regex_replace_literal (regex, self, (glong) (-1), 0, replacement, 0, &_inner_error_);
		if (_inner_error_ != NULL) {
			_g_regex_unref0 (regex);
			if (_inner_error_->domain == G_REGEX_ERROR) {
				goto __catch4_g_regex_error;
			}
			goto __finally4;
		}
		result = _tmp2_;
		_g_regex_unref0 (regex);
		return result;
	}
	goto __finally4;
	__catch4_g_regex_error:
	{
		GError * e;
		e = _inner_error_;
		_inner_error_ = NULL;
		{
			g_assert_not_reached ();
			_g_error_free0 (e);
		}
	}
	__finally4:
	if (_inner_error_ != NULL) {
		g_critical ("file %s: line %d: uncaught error: %s", __FILE__, __LINE__, _inner_error_->message);
		g_clear_error (&_inner_error_);
		return NULL;
	}
}


static void gmpc_song_links_parse_uris (GmpcSongLinks* self) {
	GError * _inner_error_;
	GtkWidget* child;
	GKeyFile* file;
	char* path;
	GtkAlignment* ali;
	GtkVBox* vbox;
	char** _tmp3_;
	gint groups_size;
	gint groups_length1;
	gsize _tmp2_;
	char** groups;
	g_return_if_fail (self != NULL);
	_inner_error_ = NULL;
	child = _g_object_ref0 (gtk_bin_get_child ((GtkBin*) self));
	if (child != NULL) {
		gtk_object_destroy ((GtkObject*) child);
	}
	file = g_key_file_new ();
	path = gmpc_get_user_path ("weblinks.list");
	if (!g_file_test (path, G_FILE_TEST_EXISTS)) {
		char* _tmp0_;
		path = (_tmp0_ = gmpc_get_full_glade_path ("weblinks.list"), _g_free0 (path), _tmp0_);
		if (!g_file_test (path, G_FILE_TEST_EXISTS)) {
			GtkProgressBar* _tmp1_;
			gtk_container_add ((GtkContainer*) self, (GtkWidget*) (_tmp1_ = g_object_ref_sink ((GtkProgressBar*) gtk_progress_bar_new ())));
			_g_object_unref0 (_tmp1_);
			gtk_widget_show_all ((GtkWidget*) self);
			self->priv->handle = gmpc_easy_async_downloader ("http://gmpc.wikia.com/index.php?title=GMPC_METADATA_WEBLINKLIST&action=raw", _gmpc_song_links_download_file_gmpc_async_download_callback, g_object_ref (self));
			_g_object_unref0 (child);
			_g_key_file_free0 (file);
			_g_free0 (path);
			return;
		}
	}
	{
		g_key_file_load_from_file (file, path, G_KEY_FILE_NONE, &_inner_error_);
		if (_inner_error_ != NULL) {
			goto __catch2_g_error;
			goto __finally2;
		}
	}
	goto __finally2;
	__catch2_g_error:
	{
		GError * e;
		e = _inner_error_;
		_inner_error_ = NULL;
		{
			fprintf (stdout, "Failed to load file: %s\n", path);
			_g_error_free0 (e);
			_g_object_unref0 (child);
			_g_key_file_free0 (file);
			_g_free0 (path);
			return;
		}
	}
	__finally2:
	if (_inner_error_ != NULL) {
		_g_object_unref0 (child);
		_g_key_file_free0 (file);
		_g_free0 (path);
		g_critical ("file %s: line %d: uncaught error: %s", __FILE__, __LINE__, _inner_error_->message);
		g_clear_error (&_inner_error_);
		return;
	}
	ali = g_object_ref_sink ((GtkAlignment*) gtk_alignment_new (0.0f, 0.0f, 0.0f, 0.0f));
	gtk_alignment_set_padding (ali, (guint) 8, (guint) 8, (guint) 12, (guint) 6);
	gtk_container_add ((GtkContainer*) self, (GtkWidget*) ali);
	vbox = g_object_ref_sink ((GtkVBox*) gtk_vbox_new (FALSE, 0));
	gtk_container_add ((GtkContainer*) ali, (GtkWidget*) vbox);
	groups = (_tmp3_ = g_key_file_get_groups (file, &_tmp2_), groups_length1 = _tmp2_, groups_size = groups_length1, _tmp3_);
	{
		char** entry_collection;
		int entry_collection_length1;
		int entry_it;
		entry_collection = groups;
		entry_collection_length1 = groups_length1;
		for (entry_it = 0; entry_it < groups_length1; entry_it = entry_it + 1) {
			char* entry;
			entry = g_strdup (entry_collection[entry_it]);
			{
				{
					char* typestr;
					char* uri;
					GmpcSongLinksType type;
					GQuark _tmp17_;
					const char* _tmp16_;
					static GQuark _tmp17__label0 = 0;
					static GQuark _tmp17__label1 = 0;
					typestr = g_key_file_get_string (file, entry, "type", &_inner_error_);
					if (_inner_error_ != NULL) {
						goto __catch3_g_error;
						goto __finally3;
					}
					uri = g_key_file_get_string (file, entry, "url", &_inner_error_);
					if (_inner_error_ != NULL) {
						_g_free0 (typestr);
						goto __catch3_g_error;
						goto __finally3;
					}
					type = 0;
					_tmp16_ = typestr;
					_tmp17_ = (NULL == _tmp16_) ? 0 : g_quark_from_string (_tmp16_);
					if (_tmp17_ == ((0 != _tmp17__label0) ? _tmp17__label0 : (_tmp17__label0 = g_quark_from_static_string ("artist"))))
					do {
						type = GMPC_SONG_LINKS_TYPE_ARTIST;
						if (self->priv->song->artist != NULL) {
							char* _tmp5_;
							char* _tmp4_;
							uri = (_tmp5_ = string_replace (uri, "%ARTIST%", _tmp4_ = gmpc_easy_download_uri_escape (self->priv->song->artist)), _g_free0 (uri), _tmp5_);
							_g_free0 (_tmp4_);
						}
						break;
					} while (0); else if (_tmp17_ == ((0 != _tmp17__label1) ? _tmp17__label1 : (_tmp17__label1 = g_quark_from_static_string ("album"))))
					do {
						type = GMPC_SONG_LINKS_TYPE_ALBUM;
						if (self->priv->song->album != NULL) {
							char* _tmp7_;
							char* _tmp6_;
							uri = (_tmp7_ = string_replace (uri, "%ALBUM%", _tmp6_ = gmpc_easy_download_uri_escape (self->priv->song->album)), _g_free0 (uri), _tmp7_);
							_g_free0 (_tmp6_);
						}
						if (self->priv->song->artist != NULL) {
							char* _tmp9_;
							char* _tmp8_;
							uri = (_tmp9_ = string_replace (uri, "%ARTIST%", _tmp8_ = gmpc_easy_download_uri_escape (self->priv->song->artist)), _g_free0 (uri), _tmp9_);
							_g_free0 (_tmp8_);
						}
						break;
					} while (0); else
					do {
						type = GMPC_SONG_LINKS_TYPE_SONG;
						if (self->priv->song->title != NULL) {
							char* _tmp11_;
							char* _tmp10_;
							uri = (_tmp11_ = string_replace (uri, "%TITLE%", _tmp10_ = gmpc_easy_download_uri_escape (self->priv->song->title)), _g_free0 (uri), _tmp11_);
							_g_free0 (_tmp10_);
						}
						if (self->priv->song->album != NULL) {
							char* _tmp13_;
							char* _tmp12_;
							uri = (_tmp13_ = string_replace (uri, "%ALBUM%", _tmp12_ = gmpc_easy_download_uri_escape (self->priv->song->album)), _g_free0 (uri), _tmp13_);
							_g_free0 (_tmp12_);
						}
						if (self->priv->song->artist != NULL) {
							char* _tmp15_;
							char* _tmp14_;
							uri = (_tmp15_ = string_replace (uri, "%ARTIST%", _tmp14_ = gmpc_easy_download_uri_escape (self->priv->song->artist)), _g_free0 (uri), _tmp15_);
							_g_free0 (_tmp14_);
						}
						break;
					} while (0); {
						char* sar;
						sar = g_key_file_get_string (file, entry, "search-and-replace", &_inner_error_);
						if (_inner_error_ != NULL) {
							goto __catch5_g_error;
							goto __finally5;
						}
						if (sar != NULL) {
							char** _tmp19_;
							gint s_size;
							gint s_length1;
							char** _tmp18_;
							char** s;
							s = (_tmp19_ = _tmp18_ = g_strsplit (sar, "::", 0), s_length1 = _vala_array_length (_tmp18_), s_size = s_length1, _tmp19_);
							if (s_length1 == 2) {
								{
									GRegex* regex;
									char* _tmp20_;
									char* _tmp21_;
									regex = g_regex_new (s[0], 0, 0, &_inner_error_);
									if (_inner_error_ != NULL) {
										if (_inner_error_->domain == G_REGEX_ERROR) {
											goto __catch6_g_regex_error;
										}
										goto __finally6;
									}
									_tmp20_ = g_regex_replace_literal (regex, uri, (glong) (-1), 0, s[1], 0, &_inner_error_);
									if (_inner_error_ != NULL) {
										_g_regex_unref0 (regex);
										if (_inner_error_->domain == G_REGEX_ERROR) {
											goto __catch6_g_regex_error;
										}
										goto __finally6;
									}
									uri = (_tmp21_ = _tmp20_, _g_free0 (uri), _tmp21_);
									_g_regex_unref0 (regex);
								}
								goto __finally6;
								__catch6_g_regex_error:
								{
									GError * e;
									e = _inner_error_;
									_inner_error_ = NULL;
									{
										fprintf (stdout, "Failed to compile regex: '%s'\n", e->message);
										_g_error_free0 (e);
									}
								}
								__finally6:
								if (_inner_error_ != NULL) {
									s = (_vala_array_free (s, s_length1, (GDestroyNotify) g_free), NULL);
									_g_free0 (sar);
									goto __catch5_g_error;
									goto __finally5;
								}
							}
							s = (_vala_array_free (s, s_length1, (GDestroyNotify) g_free), NULL);
						}
						_g_free0 (sar);
					}
					goto __finally5;
					__catch5_g_error:
					{
						GError * e;
						e = _inner_error_;
						_inner_error_ = NULL;
						{
							_g_error_free0 (e);
						}
					}
					__finally5:
					if (_inner_error_ != NULL) {
						_g_free0 (typestr);
						_g_free0 (uri);
						goto __catch3_g_error;
						goto __finally3;
					}
					if (((gint) type) <= ((gint) self->priv->type)) {
						GtkLinkButton* label;
						char* _tmp22_;
						label = g_object_ref_sink ((GtkLinkButton*) gtk_link_button_new (uri));
						gtk_button_set_label ((GtkButton*) label, _tmp22_ = g_strdup_printf (_ ("Lookup %s on %s"), _ (typestr), entry));
						_g_free0 (_tmp22_);
						gtk_button_set_alignment ((GtkButton*) label, 0.0f, 0.5f);
						gtk_box_pack_start ((GtkBox*) vbox, (GtkWidget*) label, FALSE, TRUE, (guint) 0);
						_g_object_unref0 (label);
					}
					_g_free0 (typestr);
					_g_free0 (uri);
				}
				goto __finally3;
				__catch3_g_error:
				{
					GError * e;
					e = _inner_error_;
					_inner_error_ = NULL;
					{
						fprintf (stdout, "Failed to get entry from %s: '%s'\n", path, e->message);
						_g_error_free0 (e);
					}
				}
				__finally3:
				if (_inner_error_ != NULL) {
					_g_free0 (entry);
					_g_object_unref0 (child);
					_g_key_file_free0 (file);
					_g_free0 (path);
					_g_object_unref0 (ali);
					_g_object_unref0 (vbox);
					groups = (_vala_array_free (groups, groups_length1, (GDestroyNotify) g_free), NULL);
					g_critical ("file %s: line %d: uncaught error: %s", __FILE__, __LINE__, _inner_error_->message);
					g_clear_error (&_inner_error_);
					return;
				}
				_g_free0 (entry);
			}
		}
	}
	_g_object_unref0 (child);
	_g_key_file_free0 (file);
	_g_free0 (path);
	_g_object_unref0 (ali);
	_g_object_unref0 (vbox);
	groups = (_vala_array_free (groups, groups_length1, (GDestroyNotify) g_free), NULL);
}


static void gmpc_song_links_class_init (GmpcSongLinksClass * klass) {
	gmpc_song_links_parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (GmpcSongLinksPrivate));
	G_OBJECT_CLASS (klass)->finalize = gmpc_song_links_finalize;
}


static void gmpc_song_links_instance_init (GmpcSongLinks * self) {
	self->priv = GMPC_SONG_LINKS_GET_PRIVATE (self);
	self->priv->type = GMPC_SONG_LINKS_TYPE_ARTIST;
	self->priv->song = NULL;
	self->priv->handle = NULL;
}


static void gmpc_song_links_finalize (GObject* obj) {
	GmpcSongLinks * self;
	self = GMPC_SONG_LINKS (obj);
	{
		if (self->priv->handle != NULL) {
			gmpc_easy_async_cancel (self->priv->handle);
		}
	}
	_mpd_freeSong0 (self->priv->song);
	G_OBJECT_CLASS (gmpc_song_links_parent_class)->finalize (obj);
}


GType gmpc_song_links_get_type (void) {
	static GType gmpc_song_links_type_id = 0;
	if (gmpc_song_links_type_id == 0) {
		static const GTypeInfo g_define_type_info = { sizeof (GmpcSongLinksClass), (GBaseInitFunc) NULL, (GBaseFinalizeFunc) NULL, (GClassInitFunc) gmpc_song_links_class_init, (GClassFinalizeFunc) NULL, NULL, sizeof (GmpcSongLinks), 0, (GInstanceInitFunc) gmpc_song_links_instance_init, NULL };
		gmpc_song_links_type_id = g_type_register_static (GTK_TYPE_FRAME, "GmpcSongLinks", &g_define_type_info, 0);
	}
	return gmpc_song_links_type_id;
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




