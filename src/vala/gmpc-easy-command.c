
#include "gmpc-easy-command.h"
#include <stdio.h>
#include <playlist3-messages.h>
#include <gdk/gdk.h>
#include <cairo.h>
#include <float.h>
#include <math.h>




static glong string_get_length (const char* self);
static char* string_substring (const char* self, glong offset, glong len);
struct _GmpcEasyCommandPrivate {
	GtkEntryCompletion* completion;
	GtkListStore* store;
	guint signals;
	GtkWindow* window;
};

#define GMPC_EASY_COMMAND_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GMPC_EASY_TYPE_COMMAND, GmpcEasyCommandPrivate))
enum  {
	GMPC_EASY_COMMAND_DUMMY_PROPERTY
};
static gboolean gmpc_easy_command_key_press_event (GmpcEasyCommand* self, GtkWidget* widget, const GdkEventKey* event);
static gboolean gmpc_easy_command_popup_expose_handler (GmpcEasyCommand* self, GtkWidget* widget, const GdkEventExpose* event);
static gboolean _gmpc_easy_command_popup_expose_handler_gtk_widget_expose_event (GtkWindow* _sender, const GdkEventExpose* event, gpointer self);
static void _gmpc_easy_command_activate_gtk_entry_activate (GtkEntry* _sender, gpointer self);
static gboolean _gmpc_easy_command_key_press_event_gtk_widget_key_press_event (GtkEntry* _sender, const GdkEventKey* event, gpointer self);
static GObject * gmpc_easy_command_constructor (GType type, guint n_construct_properties, GObjectConstructParam * construct_properties);
static gpointer gmpc_easy_command_parent_class = NULL;
static void gmpc_easy_command_finalize (GObject* obj);



static glong string_get_length (const char* self) {
	g_return_val_if_fail (self != NULL, 0L);
	return g_utf8_strlen (self, -1);
}


static char* string_substring (const char* self, glong offset, glong len) {
	glong string_length;
	const char* start;
	g_return_val_if_fail (self != NULL, NULL);
	string_length = g_utf8_strlen (self, -1);
	if (offset < 0) {
		offset = string_length + offset;
		g_return_val_if_fail (offset >= 0, NULL);
	} else {
		g_return_val_if_fail (offset <= string_length, NULL);
	}
	if (len < 0) {
		len = string_length - offset;
	}
	g_return_val_if_fail ((offset + len) <= string_length, NULL);
	start = g_utf8_offset_to_pointer (self, offset);
	return g_strndup (start, ((gchar*) g_utf8_offset_to_pointer (start, len)) - ((gchar*) start));
}


guint gmpc_easy_command_add_entry (GmpcEasyCommand* self, const char* name, const char* pattern, const char* hint, GmpcEasyCommandCallback* callback, void* userdata) {
	GtkTreeIter iter = {0};
	g_return_val_if_fail (self != NULL, 0U);
	g_return_val_if_fail (name != NULL, 0U);
	g_return_val_if_fail (pattern != NULL, 0U);
	g_return_val_if_fail (hint != NULL, 0U);
	self->priv->signals++;
	gtk_list_store_append (self->priv->store, &iter);
	gtk_list_store_set (self->priv->store, &iter, 0, self->priv->signals, 1, name, 2, pattern, 3, callback, 4, userdata, 5, hint, -1, -1);
	return self->priv->signals;
}


void gmpc_easy_command_activate (GmpcEasyCommand* self, GtkEntry* entry) {
	GtkTreeModel* model;
	const char* _tmp0;
	char* value;
	GtkTreeIter iter = {0};
	GtkWindow* _tmp6;
	char* _tmp7;
	g_return_if_fail (self != NULL);
	g_return_if_fail (entry != NULL);
	model = (GtkTreeModel*) self->priv->store;
	_tmp0 = NULL;
	value = (_tmp0 = gtk_entry_get_text (entry), (_tmp0 == NULL) ? NULL : g_strdup (_tmp0));
	if (string_get_length (value) == 0) {
		GtkWindow* _tmp1;
		gtk_object_destroy ((GtkObject*) self->priv->window);
		_tmp1 = NULL;
		self->priv->window = (_tmp1 = NULL, (self->priv->window == NULL) ? NULL : (self->priv->window = (g_object_unref (self->priv->window), NULL)), _tmp1);
		value = (g_free (value), NULL);
		return;
	}
	/* ToDo: Make this nicer... maybe some fancy parsing */
	if (gtk_tree_model_get_iter_first (model, &iter)) {
		do {
			char* name;
			char* pattern;
			char* test;
			GmpcEasyCommandCallback _tmp2;
			void* callback_target;
			GmpcEasyCommandCallback callback;
			void* data;
			char* _tmp3;
			name = NULL;
			pattern = NULL;
			test = NULL;
			callback = (_tmp2 = NULL, callback_target = NULL, _tmp2);
			data = NULL;
			gtk_tree_model_get (model, &iter, 1, &name, 2, &pattern, 3, &callback, 4, &data, -1, -1);
			_tmp3 = NULL;
			test = (_tmp3 = g_strdup_printf ("%s[ ]*%s$", name, pattern), test = (g_free (test), NULL), _tmp3);
			if (g_regex_match_simple (test, value, G_REGEX_CASELESS, 0)) {
				char* param;
				const char* _tmp4;
				char* param_str;
				GtkWindow* _tmp5;
				fprintf (stdout, "matched: %s to %s\n", test, value);
				param = string_substring (value, string_get_length (name), (glong) (-1));
				_tmp4 = NULL;
				param_str = (_tmp4 = g_strstrip (param), (_tmp4 == NULL) ? NULL : g_strdup (_tmp4));
				callback (data, param_str, callback_target);
				gtk_object_destroy ((GtkObject*) self->priv->window);
				_tmp5 = NULL;
				self->priv->window = (_tmp5 = NULL, (self->priv->window == NULL) ? NULL : (self->priv->window = (g_object_unref (self->priv->window), NULL)), _tmp5);
				param = (g_free (param), NULL);
				param_str = (g_free (param_str), NULL);
				name = (g_free (name), NULL);
				pattern = (g_free (pattern), NULL);
				test = (g_free (test), NULL);
				value = (g_free (value), NULL);
				return;
			}
			name = (g_free (name), NULL);
			pattern = (g_free (pattern), NULL);
			test = (g_free (test), NULL);
		} while (gtk_tree_model_iter_next (model, &iter));
	}
	gtk_object_destroy ((GtkObject*) self->priv->window);
	_tmp6 = NULL;
	self->priv->window = (_tmp6 = NULL, (self->priv->window == NULL) ? NULL : (self->priv->window = (g_object_unref (self->priv->window), NULL)), _tmp6);
	_tmp7 = NULL;
	playlist3_show_error_message (_tmp7 = g_strdup_printf ("Unknown command: '%s'", gtk_entry_get_text (entry)), ERROR_INFO);
	_tmp7 = (g_free (_tmp7), NULL);
	value = (g_free (value), NULL);
}


static gboolean gmpc_easy_command_key_press_event (GmpcEasyCommand* self, GtkWidget* widget, const GdkEventKey* event) {
	g_return_val_if_fail (self != NULL, FALSE);
	g_return_val_if_fail (widget != NULL, FALSE);
	/* Escape */
	if ((*event).keyval == 0xff1b) {
		GtkWindow* _tmp0;
		gtk_object_destroy ((GtkObject*) self->priv->window);
		_tmp0 = NULL;
		self->priv->window = (_tmp0 = NULL, (self->priv->window == NULL) ? NULL : (self->priv->window = (g_object_unref (self->priv->window), NULL)), _tmp0);
		return TRUE;
	} else {
		if ((*event).keyval == 0xff09) {
			gtk_editable_set_position ((GTK_EDITABLE (widget)), -1);
			return TRUE;
		}
	}
	return FALSE;
}


static gboolean gmpc_easy_command_popup_expose_handler (GmpcEasyCommand* self, GtkWidget* widget, const GdkEventExpose* event) {
	cairo_t* ctx;
	gint width;
	gint height;
	cairo_pattern_t* pattern;
	gboolean _tmp0;
	g_return_val_if_fail (self != NULL, FALSE);
	g_return_val_if_fail (widget != NULL, FALSE);
	ctx = gdk_cairo_create ((GdkDrawable*) widget->window);
	width = widget->allocation.width;
	height = widget->allocation.height;
	if (gtk_widget_is_composited (widget)) {
		cairo_set_operator (ctx, CAIRO_OPERATOR_SOURCE);
		cairo_set_source_rgba (ctx, 1.0, 1.0, 1.0, 0.0);
	} else {
		cairo_set_source_rgb (ctx, 1.0, 1.0, 1.0);
	}
	cairo_paint (ctx);
	/* */
	cairo_rectangle (ctx, 1.0, 1.0, (double) (width - 2), (double) (height - 2));
	pattern = cairo_pattern_create_linear (0.0, 0.0, 0.0, (double) height);
	cairo_pattern_add_color_stop_rgba (pattern, 0.0, 0.0, 0.0, 0.0, 0.5);
	cairo_pattern_add_color_stop_rgba (pattern, 0.5, 0.0, 0.0, 0.0, 1.0);
	cairo_pattern_add_color_stop_rgba (pattern, 1.0, 0.0, 0.0, 0.0, 0.5);
	cairo_set_source (ctx, pattern);
	cairo_fill_preserve (ctx);
	cairo_set_source_rgba (ctx, 1.0, 1.0, 1.0, 1.0);
	cairo_stroke (ctx);
	cairo_rectangle (ctx, 0.0, 0.0, (double) width, (double) height);
	cairo_set_source_rgba (ctx, 0.0, 0.0, 0.0, 1.0);
	cairo_stroke (ctx);
	return (_tmp0 = FALSE, (ctx == NULL) ? NULL : (ctx = (cairo_destroy (ctx), NULL)), (pattern == NULL) ? NULL : (pattern = (cairo_pattern_destroy (pattern), NULL)), _tmp0);
}


static gboolean _gmpc_easy_command_popup_expose_handler_gtk_widget_expose_event (GtkWindow* _sender, const GdkEventExpose* event, gpointer self) {
	return gmpc_easy_command_popup_expose_handler (self, _sender, event);
}


static void _gmpc_easy_command_activate_gtk_entry_activate (GtkEntry* _sender, gpointer self) {
	gmpc_easy_command_activate (self, _sender);
}


static gboolean _gmpc_easy_command_key_press_event_gtk_widget_key_press_event (GtkEntry* _sender, const GdkEventKey* event, gpointer self) {
	return gmpc_easy_command_key_press_event (self, _sender, event);
}


void gmpc_easy_command_popup (GmpcEasyCommand* self, GtkWidget* win) {
	g_return_if_fail (self != NULL);
	g_return_if_fail (win != NULL);
	if (self->priv->window == NULL) {
		GtkWindow* _tmp0;
		GtkEntry* entry;
		_tmp0 = NULL;
		self->priv->window = (_tmp0 = g_object_ref_sink ((GtkWindow*) gtk_window_new (GTK_WINDOW_TOPLEVEL)), (self->priv->window == NULL) ? NULL : (self->priv->window = (g_object_unref (self->priv->window), NULL)), _tmp0);
		entry = g_object_ref_sink ((GtkEntry*) gtk_entry_new ());
		gtk_container_set_border_width ((GtkContainer*) self->priv->window, (guint) 24);
		gtk_entry_set_width_chars (entry, 50);
		gtk_container_add ((GtkContainer*) self->priv->window, (GtkWidget*) entry);
		/* Composite */
		if (gtk_widget_is_composited ((GtkWidget*) self->priv->window)) {
			GdkScreen* _tmp1;
			GdkScreen* screen;
			GdkColormap* _tmp2;
			GdkColormap* colormap;
			_tmp1 = NULL;
			screen = (_tmp1 = gtk_window_get_screen (self->priv->window), (_tmp1 == NULL) ? NULL : g_object_ref (_tmp1));
			_tmp2 = NULL;
			colormap = (_tmp2 = gdk_screen_get_rgba_colormap (screen), (_tmp2 == NULL) ? NULL : g_object_ref (_tmp2));
			gtk_widget_set_colormap ((GtkWidget*) self->priv->window, colormap);
			(screen == NULL) ? NULL : (screen = (g_object_unref (screen), NULL));
			(colormap == NULL) ? NULL : (colormap = (g_object_unref (colormap), NULL));
		}
		g_object_set ((GtkWidget*) self->priv->window, "app-paintable", TRUE, NULL);
		g_signal_connect_object ((GtkWidget*) self->priv->window, "expose-event", (GCallback) _gmpc_easy_command_popup_expose_handler_gtk_widget_expose_event, self, 0);
		/* Setup window */
		gtk_window_set_decorated (self->priv->window, FALSE);
		gtk_window_set_modal (self->priv->window, TRUE);
		gtk_window_set_keep_above (self->priv->window, TRUE);
		gtk_window_set_transient_for (self->priv->window, GTK_WINDOW (win));
		self->priv->window->position = (guint) GTK_WIN_POS_CENTER_ON_PARENT;
		/* setup entry */
		gtk_entry_set_completion (entry, self->priv->completion);
		g_signal_connect_object (entry, "activate", (GCallback) _gmpc_easy_command_activate_gtk_entry_activate, self, 0);
		g_signal_connect_object ((GtkWidget*) entry, "key-press-event", (GCallback) _gmpc_easy_command_key_press_event_gtk_widget_key_press_event, self, 0);
		gtk_widget_show_all ((GtkWidget*) self->priv->window);
		gtk_window_present (self->priv->window);
		gtk_widget_grab_focus ((GtkWidget*) entry);
		(entry == NULL) ? NULL : (entry = (g_object_unref (entry), NULL));
	} else {
		gtk_window_present (self->priv->window);
	}
}


/* Gnome Music Player Client (GMPC)
 * Copyright (C) 2004-2009 Qball Cow <qball@sarine.nl>
 * Project homepage: http://gmpcwiki.sarine.nl/
 
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
GmpcEasyCommand* gmpc_easy_command_construct (GType object_type) {
	GmpcEasyCommand * self;
	self = g_object_newv (object_type, 0, NULL);
	return self;
}


GmpcEasyCommand* gmpc_easy_command_new (void) {
	return gmpc_easy_command_construct (GMPC_EASY_TYPE_COMMAND);
}


static GObject * gmpc_easy_command_constructor (GType type, guint n_construct_properties, GObjectConstructParam * construct_properties) {
	GObject * obj;
	GmpcEasyCommandClass * klass;
	GObjectClass * parent_class;
	GmpcEasyCommand * self;
	klass = GMPC_EASY_COMMAND_CLASS (g_type_class_peek (GMPC_EASY_TYPE_COMMAND));
	parent_class = G_OBJECT_CLASS (g_type_class_peek_parent (klass));
	obj = parent_class->constructor (type, n_construct_properties, construct_properties);
	self = GMPC_EASY_COMMAND (obj);
	{
		GtkListStore* _tmp0;
		GtkEntryCompletion* _tmp1;
		GtkCellRendererText* renderer;
		_tmp0 = NULL;
		self->priv->store = (_tmp0 = gtk_list_store_new (6, G_TYPE_UINT, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_POINTER, G_TYPE_POINTER, G_TYPE_STRING, NULL), (self->priv->store == NULL) ? NULL : (self->priv->store = (g_object_unref (self->priv->store), NULL)), _tmp0);
		_tmp1 = NULL;
		self->priv->completion = (_tmp1 = gtk_entry_completion_new (), (self->priv->completion == NULL) ? NULL : (self->priv->completion = (g_object_unref (self->priv->completion), NULL)), _tmp1);
		gtk_entry_completion_set_model (self->priv->completion, (GtkTreeModel*) self->priv->store);
		gtk_entry_completion_set_text_column (self->priv->completion, 1);
		gtk_entry_completion_set_inline_completion (self->priv->completion, TRUE);
		gtk_entry_completion_set_inline_selection (self->priv->completion, TRUE);
		gtk_entry_completion_set_popup_completion (self->priv->completion, TRUE);
		renderer = g_object_ref_sink ((GtkCellRendererText*) gtk_cell_renderer_text_new ());
		gtk_cell_layout_pack_end ((GtkCellLayout*) self->priv->completion, (GtkCellRenderer*) renderer, FALSE);
		gtk_cell_layout_add_attribute ((GtkCellLayout*) self->priv->completion, (GtkCellRenderer*) renderer, "text", 5);
		g_object_set ((GObject*) renderer, "foreground", "grey", NULL, NULL);
		(renderer == NULL) ? NULL : (renderer = (g_object_unref (renderer), NULL));
	}
	return obj;
}


static void gmpc_easy_command_class_init (GmpcEasyCommandClass * klass) {
	gmpc_easy_command_parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (GmpcEasyCommandPrivate));
	G_OBJECT_CLASS (klass)->constructor = gmpc_easy_command_constructor;
	G_OBJECT_CLASS (klass)->finalize = gmpc_easy_command_finalize;
}


static void gmpc_easy_command_instance_init (GmpcEasyCommand * self) {
	self->priv = GMPC_EASY_COMMAND_GET_PRIVATE (self);
	self->priv->completion = NULL;
	self->priv->store = NULL;
	self->priv->signals = (guint) 0;
	self->priv->window = NULL;
}


static void gmpc_easy_command_finalize (GObject* obj) {
	GmpcEasyCommand * self;
	self = GMPC_EASY_COMMAND (obj);
	(self->priv->completion == NULL) ? NULL : (self->priv->completion = (g_object_unref (self->priv->completion), NULL));
	(self->priv->store == NULL) ? NULL : (self->priv->store = (g_object_unref (self->priv->store), NULL));
	(self->priv->window == NULL) ? NULL : (self->priv->window = (g_object_unref (self->priv->window), NULL));
	G_OBJECT_CLASS (gmpc_easy_command_parent_class)->finalize (obj);
}


GType gmpc_easy_command_get_type (void) {
	static GType gmpc_easy_command_type_id = 0;
	if (gmpc_easy_command_type_id == 0) {
		static const GTypeInfo g_define_type_info = { sizeof (GmpcEasyCommandClass), (GBaseInitFunc) NULL, (GBaseFinalizeFunc) NULL, (GClassInitFunc) gmpc_easy_command_class_init, (GClassFinalizeFunc) NULL, NULL, sizeof (GmpcEasyCommand), 0, (GInstanceInitFunc) gmpc_easy_command_instance_init, NULL };
		gmpc_easy_command_type_id = g_type_register_static (G_TYPE_OBJECT, "GmpcEasyCommand", &g_define_type_info, 0);
	}
	return gmpc_easy_command_type_id;
}




