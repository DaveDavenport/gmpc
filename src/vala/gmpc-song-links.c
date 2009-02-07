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

#include "gmpc-song-links.h"
#include <gmpc_easy_download.h>
#include <config.h>
#include <stdlib.h>
#include <string.h>
#include <plugin.h>
#include <glib/gstdio.h>
#include <stdio.h>
#include <float.h>
#include <math.h>
#include <glib/gi18n-lib.h>
#include <misc.h>
#include <gdk/gdk.h>




static char* string_replace (const char* self, const char* old, const char* replacement);
static glong string_get_length (const char* self);
struct _GmpcSongLinksPrivate {
	GmpcSongLinksType type;
	mpd_Song* song;
	GEADAsyncHandler* handle;
};

#define GMPC_SONG_LINKS_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GMPC_SONG_TYPE_LINKS, GmpcSongLinksPrivate))
enum  {
	GMPC_SONG_LINKS_DUMMY_PROPERTY
};
#define GMPC_SONG_LINKS_some_unique_name VERSION
static void _gmpc_song_links_download_file_gmpc_async_download_callback (const GEADAsyncHandler* handle, GEADStatus status, gpointer self);
static void gmpc_song_links_download (GmpcSongLinks* self, GtkImageMenuItem* item);
static void _gmpc_song_links_download_gtk_menu_item_activate (GtkImageMenuItem* _sender, gpointer self);
static gboolean gmpc_song_links_button_press_event (GmpcSongLinks* self, GtkWidget* label, const GdkEventButton* event);
static gboolean _gmpc_song_links_button_press_event_gtk_widget_button_press_event (GtkEventBox* _sender, const GdkEventButton* event, gpointer self);
static void gmpc_song_links_open_uri (GmpcSongLinks* self, GtkLinkButton* button);
static void gmpc_song_links_download_file (GmpcSongLinks* self, const GEADAsyncHandler* handle, GEADStatus status);
static void _gmpc_song_links_open_uri_gtk_button_clicked (GtkLinkButton* _sender, gpointer self);
static void gmpc_song_links_parse_uris (GmpcSongLinks* self);
static gpointer gmpc_song_links_parent_class = NULL;
static void gmpc_song_links_finalize (GObject* obj);
static void _vala_array_free (gpointer array, gint array_length, GDestroyNotify destroy_func);



static char* string_replace (const char* self, const char* old, const char* replacement) {
	GError * inner_error;
	g_return_val_if_fail (self != NULL, NULL);
	g_return_val_if_fail (old != NULL, NULL);
	g_return_val_if_fail (replacement != NULL, NULL);
	inner_error = NULL;
	{
		char* _tmp0;
		GRegex* _tmp1;
		GRegex* regex;
		char* _tmp2;
		char* _tmp3;
		_tmp0 = NULL;
		_tmp1 = NULL;
		regex = (_tmp1 = g_regex_new (_tmp0 = g_regex_escape_string (old, -1), 0, 0, &inner_error), _tmp0 = (g_free (_tmp0), NULL), _tmp1);
		if (inner_error != NULL) {
			if (inner_error->domain == G_REGEX_ERROR) {
				goto __catch0_g_regex_error;
			}
			goto __finally0;
		}
		_tmp2 = g_regex_replace_literal (regex, self, (glong) (-1), 0, replacement, 0, &inner_error);
		if (inner_error != NULL) {
			(regex == NULL) ? NULL : (regex = (g_regex_unref (regex), NULL));
			if (inner_error->domain == G_REGEX_ERROR) {
				goto __catch0_g_regex_error;
			}
			goto __finally0;
		}
		_tmp3 = NULL;
		return (_tmp3 = _tmp2, (regex == NULL) ? NULL : (regex = (g_regex_unref (regex), NULL)), _tmp3);
	}
	goto __finally0;
	__catch0_g_regex_error:
	{
		GError * e;
		e = inner_error;
		inner_error = NULL;
		{
			g_assert_not_reached ();
			(e == NULL) ? NULL : (e = (g_error_free (e), NULL));
		}
	}
	__finally0:
	if (inner_error != NULL) {
		g_critical ("file %s: line %d: uncaught error: %s", __FILE__, __LINE__, inner_error->message);
		g_clear_error (&inner_error);
		return NULL;
	}
}


static glong string_get_length (const char* self) {
	g_return_val_if_fail (self != NULL, 0L);
	return g_utf8_strlen (self, -1);
}



GType gmpc_song_links_type_get_type (void) {
	static GType gmpc_song_links_type_type_id = 0;
	if (G_UNLIKELY (gmpc_song_links_type_type_id == 0)) {
		static const GEnumValue values[] = {{GMPC_SONG_LINKS_TYPE_ARTIST, "GMPC_SONG_LINKS_TYPE_ARTIST", "artist"}, {GMPC_SONG_LINKS_TYPE_ALBUM, "GMPC_SONG_LINKS_TYPE_ALBUM", "album"}, {GMPC_SONG_LINKS_TYPE_SONG, "GMPC_SONG_LINKS_TYPE_SONG", "song"}, {0, NULL, NULL}};
		gmpc_song_links_type_type_id = g_enum_register_static ("GmpcSongLinksType", values);
	}
	return gmpc_song_links_type_type_id;
}


static void _gmpc_song_links_download_file_gmpc_async_download_callback (const GEADAsyncHandler* handle, GEADStatus status, gpointer self) {
	gmpc_song_links_download_file (self, handle, status);
}


static void gmpc_song_links_download (GmpcSongLinks* self, GtkImageMenuItem* item) {
	GtkWidget* _tmp0;
	GtkWidget* child;
	GtkProgressBar* _tmp1;
	g_return_if_fail (self != NULL);
	g_return_if_fail (item != NULL);
	_tmp0 = NULL;
	child = (_tmp0 = gtk_bin_get_child ((GtkBin*) self), (_tmp0 == NULL) ? NULL : g_object_ref (_tmp0));
	if (child != NULL) {
		gtk_object_destroy ((GtkObject*) child);
	}
	/* now try to download */
	_tmp1 = NULL;
	gtk_container_add ((GtkContainer*) self, (GtkWidget*) (_tmp1 = g_object_ref_sink ((GtkProgressBar*) gtk_progress_bar_new ())));
	(_tmp1 == NULL) ? NULL : (_tmp1 = (g_object_unref (_tmp1), NULL));
	gtk_widget_show_all ((GtkWidget*) self);
	self->priv->handle = gmpc_easy_async_downloader ("http://gmpc.wikia.com/index.php?title=GMPC_METADATA_WEBLINKLIST&action=raw", _gmpc_song_links_download_file_gmpc_async_download_callback, self);
	(child == NULL) ? NULL : (child = (g_object_unref (child), NULL));
}


static void _gmpc_song_links_download_gtk_menu_item_activate (GtkImageMenuItem* _sender, gpointer self) {
	gmpc_song_links_download (self, _sender);
}


static gboolean gmpc_song_links_button_press_event (GmpcSongLinks* self, GtkWidget* label, const GdkEventButton* event) {
	g_return_val_if_fail (self != NULL, FALSE);
	g_return_val_if_fail (label != NULL, FALSE);
	if ((*event).button == 3) {
		GtkMenu* menu;
		GtkImageMenuItem* item;
		GtkImage* _tmp0;
		menu = g_object_ref_sink ((GtkMenu*) gtk_menu_new ());
		item = g_object_ref_sink ((GtkImageMenuItem*) gtk_image_menu_item_new_with_label (_ ("Update list from internet")));
		_tmp0 = NULL;
		gtk_image_menu_item_set_image (item, (GtkWidget*) (_tmp0 = g_object_ref_sink ((GtkImage*) gtk_image_new_from_stock (GTK_STOCK_REFRESH, GTK_ICON_SIZE_MENU))));
		(_tmp0 == NULL) ? NULL : (_tmp0 = (g_object_unref (_tmp0), NULL));
		g_signal_connect_object ((GtkMenuItem*) item, "activate", (GCallback) _gmpc_song_links_download_gtk_menu_item_activate, self, 0);
		gtk_menu_shell_append ((GtkMenuShell*) menu, (GtkWidget*) ((GtkMenuItem*) item));
		gtk_widget_show_all ((GtkWidget*) menu);
		gtk_menu_popup (menu, NULL, NULL, NULL, NULL, (*event).button, (*event).time);
		(menu == NULL) ? NULL : (menu = (g_object_unref (menu), NULL));
		(item == NULL) ? NULL : (item = (g_object_unref (item), NULL));
	}
	return FALSE;
}


static gboolean _gmpc_song_links_button_press_event_gtk_widget_button_press_event (GtkEventBox* _sender, const GdkEventButton* event, gpointer self) {
	return gmpc_song_links_button_press_event (self, _sender, event);
}


GmpcSongLinks* gmpc_song_links_construct (GType object_type, GmpcSongLinksType type, const mpd_Song* song) {
	GmpcSongLinks * self;
	mpd_Song* _tmp1;
	const mpd_Song* _tmp0;
	GtkEventBox* event;
	GtkLabel* label;
	char* _tmp2;
	g_return_val_if_fail (song != NULL, NULL);
	self = g_object_newv (object_type, 0, NULL);
	self->priv->type = type;
	_tmp1 = NULL;
	_tmp0 = NULL;
	self->priv->song = (_tmp1 = (_tmp0 = song, (_tmp0 == NULL) ? NULL : mpd_songDup (_tmp0)), (self->priv->song == NULL) ? NULL : (self->priv->song = (mpd_freeSong (self->priv->song), NULL)), _tmp1);
	event = g_object_ref_sink ((GtkEventBox*) gtk_event_box_new ());
	label = g_object_ref_sink ((GtkLabel*) gtk_label_new (""));
	gtk_container_add ((GtkContainer*) event, (GtkWidget*) label);
	gtk_event_box_set_visible_window (event, FALSE);
	gtk_frame_set_label_widget ((GtkFrame*) self, (GtkWidget*) event);
	_tmp2 = NULL;
	gtk_label_set_markup (label, _tmp2 = g_strdup_printf ("<b>%s:</b>", _ ("Links")));
	_tmp2 = (g_free (_tmp2), NULL);
	g_object_set ((GtkFrame*) self, "shadow", GTK_SHADOW_NONE, NULL);
	g_signal_connect_object ((GtkWidget*) event, "button-press-event", (GCallback) _gmpc_song_links_button_press_event_gtk_widget_button_press_event, self, 0);
	gmpc_song_links_parse_uris (self);
	return self;
}


GmpcSongLinks* gmpc_song_links_new (GmpcSongLinksType type, const mpd_Song* song) {
	return gmpc_song_links_construct (GMPC_SONG_TYPE_LINKS, type, song);
}


static void gmpc_song_links_open_uri (GmpcSongLinks* self, GtkLinkButton* button) {
	GtkLinkButton* _tmp0;
	GtkLinkButton* lb;
	g_return_if_fail (self != NULL);
	g_return_if_fail (button != NULL);
	_tmp0 = NULL;
	lb = (_tmp0 = button, (_tmp0 == NULL) ? NULL : g_object_ref (_tmp0));
	fprintf (stdout, "open uri: %s\n", gtk_link_button_get_uri (lb));
	open_uri (gtk_link_button_get_uri (lb));
	(lb == NULL) ? NULL : (lb = (g_object_unref (lb), NULL));
}


static void gmpc_song_links_download_file (GmpcSongLinks* self, const GEADAsyncHandler* handle, GEADStatus status) {
	GError * inner_error;
	g_return_if_fail (self != NULL);
	g_return_if_fail (handle != NULL);
	inner_error = NULL;
	if (status == GEAD_PROGRESS) {
		GtkProgressBar* _tmp0;
		GtkProgressBar* pb;
		_tmp0 = NULL;
		pb = (_tmp0 = GTK_PROGRESS_BAR (gtk_bin_get_child ((GtkBin*) self)), (_tmp0 == NULL) ? NULL : g_object_ref (_tmp0));
		if (pb != NULL) {
			gtk_progress_bar_pulse (pb);
		}
		(pb == NULL) ? NULL : (pb = (g_object_unref (pb), NULL));
		return;
	}
	if (status == GEAD_DONE) {
		gint64 length;
		const char* _tmp1;
		char* a;
		char* path;
		length = (gint64) 0;
		_tmp1 = NULL;
		a = (_tmp1 = gmpc_easy_handler_get_data (handle, &length), (_tmp1 == NULL) ? NULL : g_strdup (_tmp1));
		path = gmpc_get_user_path ("weblinks.list");
		{
			g_file_set_contents (path, a, (glong) length, &inner_error);
			if (inner_error != NULL) {
				goto __catch1_g_error;
				goto __finally1;
			}
			gmpc_song_links_parse_uris (self);
			gtk_widget_show_all ((GtkWidget*) self);
		}
		goto __finally1;
		__catch1_g_error:
		{
			GError * e;
			e = inner_error;
			inner_error = NULL;
			{
				fprintf (stdout, "Error: %s\n", e->message);
				(e == NULL) ? NULL : (e = (g_error_free (e), NULL));
			}
		}
		__finally1:
		if (inner_error != NULL) {
			a = (g_free (a), NULL);
			path = (g_free (path), NULL);
			g_critical ("file %s: line %d: uncaught error: %s", __FILE__, __LINE__, inner_error->message);
			g_clear_error (&inner_error);
			return;
		}
		a = (g_free (a), NULL);
		path = (g_free (path), NULL);
	} else {
		char* path;
		path = gmpc_get_user_path ("weblinks.list");
		{
			char* a;
			a = g_strdup (" ");
			g_file_set_contents (path, a, string_get_length (a), &inner_error);
			if (inner_error != NULL) {
				a = (g_free (a), NULL);
				goto __catch2_g_error;
				goto __finally2;
			}
			gmpc_song_links_parse_uris (self);
			gtk_widget_show_all ((GtkWidget*) self);
			a = (g_free (a), NULL);
		}
		goto __finally2;
		__catch2_g_error:
		{
			GError * e;
			e = inner_error;
			inner_error = NULL;
			{
				fprintf (stdout, "Error: %s\n", e->message);
				(e == NULL) ? NULL : (e = (g_error_free (e), NULL));
			}
		}
		__finally2:
		if (inner_error != NULL) {
			path = (g_free (path), NULL);
			g_critical ("file %s: line %d: uncaught error: %s", __FILE__, __LINE__, inner_error->message);
			g_clear_error (&inner_error);
			return;
		}
		path = (g_free (path), NULL);
	}
	self->priv->handle = NULL;
}


static void _gmpc_song_links_open_uri_gtk_button_clicked (GtkLinkButton* _sender, gpointer self) {
	gmpc_song_links_open_uri (self, _sender);
}


static void gmpc_song_links_parse_uris (GmpcSongLinks* self) {
	GError * inner_error;
	GtkWidget* _tmp0;
	GtkWidget* child;
	GKeyFile* file;
	char* path;
	GtkAlignment* ali;
	GtkVBox* vbox;
	char** _tmp4;
	gint groups_size;
	gint groups_length1;
	gint _tmp3;
	char** groups;
	g_return_if_fail (self != NULL);
	inner_error = NULL;
	_tmp0 = NULL;
	child = (_tmp0 = gtk_bin_get_child ((GtkBin*) self), (_tmp0 == NULL) ? NULL : g_object_ref (_tmp0));
	if (child != NULL) {
		gtk_object_destroy ((GtkObject*) child);
	}
	file = g_key_file_new ();
	path = gmpc_get_user_path ("weblinks.list");
	if (!g_file_test (path, G_FILE_TEST_EXISTS)) {
		char* _tmp1;
		_tmp1 = NULL;
		path = (_tmp1 = gmpc_get_full_glade_path ("weblinks.list"), path = (g_free (path), NULL), _tmp1);
		if (!g_file_test (path, G_FILE_TEST_EXISTS)) {
			GtkProgressBar* _tmp2;
			/* now try to download */
			_tmp2 = NULL;
			gtk_container_add ((GtkContainer*) self, (GtkWidget*) (_tmp2 = g_object_ref_sink ((GtkProgressBar*) gtk_progress_bar_new ())));
			(_tmp2 == NULL) ? NULL : (_tmp2 = (g_object_unref (_tmp2), NULL));
			gtk_widget_show_all ((GtkWidget*) self);
			self->priv->handle = gmpc_easy_async_downloader ("http://download.sarine.nl/weblinks.list", _gmpc_song_links_download_file_gmpc_async_download_callback, self);
			(child == NULL) ? NULL : (child = (g_object_unref (child), NULL));
			(file == NULL) ? NULL : (file = (g_key_file_free (file), NULL));
			path = (g_free (path), NULL);
			return;
		}
	}
	{
		g_key_file_load_from_file (file, path, G_KEY_FILE_NONE, &inner_error);
		if (inner_error != NULL) {
			goto __catch3_g_error;
			goto __finally3;
		}
	}
	goto __finally3;
	__catch3_g_error:
	{
		GError * e;
		e = inner_error;
		inner_error = NULL;
		{
			fprintf (stdout, "Failed to load file: %s\n", path);
			(e == NULL) ? NULL : (e = (g_error_free (e), NULL));
			(child == NULL) ? NULL : (child = (g_object_unref (child), NULL));
			(file == NULL) ? NULL : (file = (g_key_file_free (file), NULL));
			path = (g_free (path), NULL);
			return;
		}
	}
	__finally3:
	if (inner_error != NULL) {
		(child == NULL) ? NULL : (child = (g_object_unref (child), NULL));
		(file == NULL) ? NULL : (file = (g_key_file_free (file), NULL));
		path = (g_free (path), NULL);
		g_critical ("file %s: line %d: uncaught error: %s", __FILE__, __LINE__, inner_error->message);
		g_clear_error (&inner_error);
		return;
	}
	ali = g_object_ref_sink ((GtkAlignment*) gtk_alignment_new (0.0f, 0.0f, 0.0f, 0.0f));
	gtk_alignment_set_padding (ali, (guint) 8, (guint) 8, (guint) 12, (guint) 6);
	gtk_container_add ((GtkContainer*) self, (GtkWidget*) ali);
	vbox = g_object_ref_sink ((GtkVBox*) gtk_vbox_new (FALSE, 0));
	gtk_container_add ((GtkContainer*) ali, (GtkWidget*) vbox);
	_tmp4 = NULL;
	groups = (_tmp4 = g_key_file_get_groups (file, &_tmp3), groups_length1 = _tmp3, groups_size = groups_length1, _tmp4);
	{
		char** entry_collection;
		int entry_collection_length1;
		int entry_it;
		entry_collection = groups;
		entry_collection_length1 = groups_length1;
		for (entry_it = 0; entry_it < groups_length1; entry_it = entry_it + 1) {
			const char* _tmp14;
			char* entry;
			_tmp14 = NULL;
			entry = (_tmp14 = entry_collection[entry_it], (_tmp14 == NULL) ? NULL : g_strdup (_tmp14));
			{
				{
					char* typestr;
					char* uri;
					GmpcSongLinksType type;
					GQuark _tmp12;
					char* _tmp11;
					typestr = g_key_file_get_string (file, entry, "type", &inner_error);
					if (inner_error != NULL) {
						goto __catch4_g_error;
						goto __finally4;
					}
					uri = g_key_file_get_string (file, entry, "url", &inner_error);
					if (inner_error != NULL) {
						typestr = (g_free (typestr), NULL);
						goto __catch4_g_error;
						goto __finally4;
					}
					type = 0;
					_tmp11 = NULL;
					_tmp11 = typestr;
					_tmp12 = (NULL == _tmp11) ? 0 : g_quark_from_string (_tmp11);
					if (_tmp12 == g_quark_from_string (_ ("artist")))
					do {
						type = GMPC_SONG_LINKS_TYPE_ARTIST;
						if (self->priv->song->artist != NULL) {
							char* _tmp5;
							_tmp5 = NULL;
							uri = (_tmp5 = string_replace (uri, "%ARTIST%", self->priv->song->artist), uri = (g_free (uri), NULL), _tmp5);
						}
						break;
					} while (0); else if (_tmp12 == g_quark_from_string (_ ("album")))
					do {
						type = GMPC_SONG_LINKS_TYPE_ALBUM;
						if (self->priv->song->album != NULL) {
							char* _tmp6;
							_tmp6 = NULL;
							uri = (_tmp6 = string_replace (uri, "%ALBUM%", self->priv->song->album), uri = (g_free (uri), NULL), _tmp6);
						}
						if (self->priv->song->artist != NULL) {
							char* _tmp7;
							_tmp7 = NULL;
							uri = (_tmp7 = string_replace (uri, "%ARTIST%", self->priv->song->artist), uri = (g_free (uri), NULL), _tmp7);
						}
						break;
					} while (0); else
					do {
						type = GMPC_SONG_LINKS_TYPE_SONG;
						if (self->priv->song->title != NULL) {
							char* _tmp8;
							_tmp8 = NULL;
							uri = (_tmp8 = string_replace (uri, "%TITLE%", self->priv->song->title), uri = (g_free (uri), NULL), _tmp8);
						}
						if (self->priv->song->album != NULL) {
							char* _tmp9;
							_tmp9 = NULL;
							uri = (_tmp9 = string_replace (uri, "%ALBUM%", self->priv->song->album), uri = (g_free (uri), NULL), _tmp9);
						}
						if (self->priv->song->artist != NULL) {
							char* _tmp10;
							_tmp10 = NULL;
							uri = (_tmp10 = string_replace (uri, "%ARTIST%", self->priv->song->artist), uri = (g_free (uri), NULL), _tmp10);
						}
						break;
					} while (0);
					if (((gint) type) <= ((gint) self->priv->type)) {
						GtkLinkButton* label;
						char* _tmp13;
						label = g_object_ref_sink ((GtkLinkButton*) gtk_link_button_new (uri));
						_tmp13 = NULL;
						gtk_button_set_label ((GtkButton*) label, _tmp13 = g_strdup_printf (_ ("Lookup %s on %s"), _ (typestr), entry));
						_tmp13 = (g_free (_tmp13), NULL);
						gtk_button_set_alignment ((GtkButton*) label, 0.0f, 0.5f);
						gtk_box_pack_start ((GtkBox*) vbox, (GtkWidget*) label, FALSE, TRUE, (guint) 0);
						g_signal_connect_object ((GtkButton*) label, "clicked", (GCallback) _gmpc_song_links_open_uri_gtk_button_clicked, self, 0);
						(label == NULL) ? NULL : (label = (g_object_unref (label), NULL));
					}
					typestr = (g_free (typestr), NULL);
					uri = (g_free (uri), NULL);
				}
				goto __finally4;
				__catch4_g_error:
				{
					GError * e;
					e = inner_error;
					inner_error = NULL;
					{
						fprintf (stdout, "Failed to get entry from %s: '%s'\n", path, e->message);
						(e == NULL) ? NULL : (e = (g_error_free (e), NULL));
					}
				}
				__finally4:
				if (inner_error != NULL) {
					entry = (g_free (entry), NULL);
					(child == NULL) ? NULL : (child = (g_object_unref (child), NULL));
					(file == NULL) ? NULL : (file = (g_key_file_free (file), NULL));
					path = (g_free (path), NULL);
					(ali == NULL) ? NULL : (ali = (g_object_unref (ali), NULL));
					(vbox == NULL) ? NULL : (vbox = (g_object_unref (vbox), NULL));
					groups = (_vala_array_free (groups, groups_length1, (GDestroyNotify) g_free), NULL);
					g_critical ("file %s: line %d: uncaught error: %s", __FILE__, __LINE__, inner_error->message);
					g_clear_error (&inner_error);
					return;
				}
				entry = (g_free (entry), NULL);
			}
		}
	}
	(child == NULL) ? NULL : (child = (g_object_unref (child), NULL));
	(file == NULL) ? NULL : (file = (g_key_file_free (file), NULL));
	path = (g_free (path), NULL);
	(ali == NULL) ? NULL : (ali = (g_object_unref (ali), NULL));
	(vbox == NULL) ? NULL : (vbox = (g_object_unref (vbox), NULL));
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
	(self->priv->song == NULL) ? NULL : (self->priv->song = (mpd_freeSong (self->priv->song), NULL));
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


static void _vala_array_free (gpointer array, gint array_length, GDestroyNotify destroy_func) {
	if ((array != NULL) && (destroy_func != NULL)) {
		int i;
		for (i = 0; i < array_length; i = i + 1) {
			if (((gpointer*) array)[i] != NULL) {
				destroy_func (((gpointer*) array)[i]);
			}
		}
	}
	g_free (array);
}




