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

#include "gmpc-easy-command.h"
#include <config.h>
#include <glib/gi18n-lib.h>
#include <plugin.h>
#include <config1.h>
#include <stdio.h>
#include <playlist3-messages.h>
#include <gdk/gdk.h>
#include <cairo.h>
#include <float.h>
#include <math.h>




static char* string_substring (const char* self, glong offset, glong len);
static glong string_get_length (const char* self);
struct _GmpcEasyCommandPrivate {
	GtkEntryCompletion* completion;
	guint signals;
	GtkWindow* window;
	gint* version;
	gint version_length1;
	gint version_size;
};

#define GMPC_EASY_COMMAND_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GMPC_EASY_TYPE_COMMAND, GmpcEasyCommandPrivate))
enum  {
	GMPC_EASY_COMMAND_DUMMY_PROPERTY
};
#define GMPC_EASY_COMMAND_some_unique_name VERSION
static const char* gmpc_easy_command_real_get_name (GmpcPluginBase* base);
static gint* gmpc_easy_command_real_get_version (GmpcPluginBase* base, int* result_length1);
static void gmpc_easy_command_real_save_yourself (GmpcPluginBase* base);
static gboolean gmpc_easy_command_real_get_enabled (GmpcPluginBase* base);
static void gmpc_easy_command_real_set_enabled (GmpcPluginBase* base, gboolean state);
static gboolean gmpc_easy_command_completion_function (GmpcEasyCommand* self, GtkEntryCompletion* comp, const char* key, GtkTreeIter* iter);
static void gmpc_easy_command_activate (GmpcEasyCommand* self, GtkEntry* entry);
static gboolean gmpc_easy_command_key_press_event (GmpcEasyCommand* self, GtkEntry* widget, const GdkEventKey* event);
static gboolean gmpc_easy_command_popup_expose_handler (GmpcEasyCommand* self, GtkWindow* widget, const GdkEventExpose* event);
static gboolean _gmpc_easy_command_popup_expose_handler_gtk_widget_expose_event (GtkWindow* _sender, const GdkEventExpose* event, gpointer self);
static void _gmpc_easy_command_activate_gtk_entry_activate (GtkEntry* _sender, gpointer self);
static gboolean _gmpc_easy_command_key_press_event_gtk_widget_key_press_event (GtkEntry* _sender, const GdkEventKey* event, gpointer self);
static void _gmpc_easy_command_help_window_destroy_gtk_dialog_response (GtkDialog* _sender, gint response_id, gpointer self);
static gboolean _gmpc_easy_command_completion_function_gtk_entry_completion_match_func (GtkEntryCompletion* completion, const char* key, GtkTreeIter* iter, gpointer self);
static GObject * gmpc_easy_command_constructor (GType type, guint n_construct_properties, GObjectConstructParam * construct_properties);
static gpointer gmpc_easy_command_parent_class = NULL;
static void gmpc_easy_command_finalize (GObject* obj);
static void _vala_array_free (gpointer array, gint array_length, GDestroyNotify destroy_func);
static gint _vala_array_length (gpointer array);



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


static glong string_get_length (const char* self) {
	g_return_val_if_fail (self != NULL, 0L);
	return g_utf8_strlen (self, -1);
}


/**
 * Required plugin implementation
 */
static const char* gmpc_easy_command_real_get_name (GmpcPluginBase* base) {
	GmpcEasyCommand * self;
	self = (GmpcEasyCommand*) base;
	return _ ("Gmpc Easy Command");
}


static gint* gmpc_easy_command_real_get_version (GmpcPluginBase* base, int* result_length1) {
	GmpcEasyCommand * self;
	gint* _tmp0;
	self = (GmpcEasyCommand*) base;
	_tmp0 = NULL;
	return (_tmp0 = self->priv->version, *result_length1 = self->priv->version_length1, _tmp0);
}


/**
     * Tells the plugin to save itself
     */
static void gmpc_easy_command_real_save_yourself (GmpcPluginBase* base) {
	GmpcEasyCommand * self;
	self = (GmpcEasyCommand*) base;
}


/* nothing to save 
*
     * Get set enabled
     */
static gboolean gmpc_easy_command_real_get_enabled (GmpcPluginBase* base) {
	GmpcEasyCommand * self;
	self = (GmpcEasyCommand*) base;
	return (gboolean) cfg_get_single_value_as_int_with_default (config, gmpc_plugin_base_get_name ((GmpcPluginBase*) self), "enabled", 1);
}


static void gmpc_easy_command_real_set_enabled (GmpcPluginBase* base, gboolean state) {
	GmpcEasyCommand * self;
	gboolean _tmp0;
	self = (GmpcEasyCommand*) base;
	_tmp0 = FALSE;
	if (!state) {
		_tmp0 = self->priv->window != NULL;
	} else {
		_tmp0 = FALSE;
	}
	/* if disabling and popup is open, close it */
	if (_tmp0) {
		GtkWindow* _tmp1;
		gtk_object_destroy ((GtkObject*) self->priv->window);
		_tmp1 = NULL;
		self->priv->window = (_tmp1 = NULL, (self->priv->window == NULL) ? NULL : (self->priv->window = (g_object_unref (self->priv->window), NULL)), _tmp1);
	}
	cfg_set_single_value_as_int (config, gmpc_plugin_base_get_name ((GmpcPluginBase*) self), "enabled", (gint) state);
}


/************************************************
 * private
 */
static gboolean gmpc_easy_command_completion_function (GmpcEasyCommand* self, GtkEntryCompletion* comp, const char* key, GtkTreeIter* iter) {
	char* value;
	GtkTreeModel* _tmp0;
	GtkTreeModel* model;
	gboolean _tmp2;
	g_return_val_if_fail (self != NULL, FALSE);
	g_return_val_if_fail (comp != NULL, FALSE);
	g_return_val_if_fail (key != NULL, FALSE);
	value = NULL;
	_tmp0 = NULL;
	model = (_tmp0 = gtk_entry_completion_get_model (comp), (_tmp0 == NULL) ? NULL : g_object_ref (_tmp0));
	gtk_tree_model_get (model, &(*iter), 1, &value, -1);
	if (value != NULL) {
		char* a;
		gboolean _tmp1;
		a = g_strdup_printf ("^%s.*", key);
		return (_tmp1 = g_regex_match_simple (a, value, G_REGEX_CASELESS, 0), a = (g_free (a), NULL), value = (g_free (value), NULL), (model == NULL) ? NULL : (model = (g_object_unref (model), NULL)), _tmp1);
	}
	return (_tmp2 = FALSE, value = (g_free (value), NULL), (model == NULL) ? NULL : (model = (g_object_unref (model), NULL)), _tmp2);
}


/**
     * Add a match entry to the Easy command object.
     * param self the GmpcEasyCommand object.
     * param name the name of the command. This is the "prefix" that needs to be matched.
     * param pattern the pattern where the parameters need to match.
     * param callback a GmpcEasyCommandCallback that returns when a entry is matched.
     * param userdata a pointer that is passed to callback.
     *
     * return an unique id for the entry.
     */
guint gmpc_easy_command_add_entry (GmpcEasyCommand* self, const char* name, const char* pattern, const char* hint, GmpcEasyCommandCallback* callback, void* userdata) {
	GtkTreeIter iter = {0};
	g_return_val_if_fail (self != NULL, 0U);
	g_return_val_if_fail (name != NULL, 0U);
	g_return_val_if_fail (pattern != NULL, 0U);
	g_return_val_if_fail (hint != NULL, 0U);
	self->priv->signals++;
	gtk_list_store_append (self->store, &iter);
	gtk_list_store_set (self->store, &iter, 0, self->priv->signals, 1, name, 2, pattern, 3, callback, 4, userdata, 5, hint, -1, -1);
	return self->priv->signals;
}


static void gmpc_easy_command_activate (GmpcEasyCommand* self, GtkEntry* entry) {
	GtkTreeModel* model;
	const char* _tmp0;
	char* value_unsplit;
	GtkTreeIter iter = {0};
	GtkWindow* _tmp19;
	g_return_if_fail (self != NULL);
	g_return_if_fail (entry != NULL);
	model = (GtkTreeModel*) self->store;
	_tmp0 = NULL;
	value_unsplit = (_tmp0 = gtk_entry_get_text (entry), (_tmp0 == NULL) ? NULL : g_strdup (_tmp0));
	if (string_get_length (value_unsplit) == 0) {
		GtkWindow* _tmp1;
		gtk_object_destroy ((GtkObject*) self->priv->window);
		_tmp1 = NULL;
		self->priv->window = (_tmp1 = NULL, (self->priv->window == NULL) ? NULL : (self->priv->window = (g_object_unref (self->priv->window), NULL)), _tmp1);
		value_unsplit = (g_free (value_unsplit), NULL);
		return;
	}
	{
		char** _tmp2;
		char** value_collection;
		int value_collection_length1;
		int value_it;
		_tmp2 = NULL;
		value_collection = _tmp2 = g_strsplit (value_unsplit, ";", 0);
		value_collection_length1 = _vala_array_length (_tmp2);
		for (value_it = 0; value_it < _vala_array_length (_tmp2); value_it = value_it + 1) {
			const char* _tmp18;
			char* value;
			_tmp18 = NULL;
			value = (_tmp18 = value_collection[value_it], (_tmp18 == NULL) ? NULL : g_strdup (_tmp18));
			{
				gboolean found;
				found = FALSE;
				/* ToDo: Make this nicer... maybe some fancy parsing */
				if (gtk_tree_model_get_iter_first (model, &iter)) {
					gboolean _tmp3;
					_tmp3 = FALSE;
					do {
						char* name;
						char* pattern;
						char* test;
						GmpcEasyCommandCallback _tmp5;
						void* callback_target;
						GmpcEasyCommandCallback callback;
						void* data;
						char* _tmp6;
						if (_tmp3) {
							gboolean _tmp4;
							_tmp4 = FALSE;
							if (gtk_tree_model_iter_next (model, &iter)) {
								_tmp4 = !found;
							} else {
								_tmp4 = FALSE;
							}
							if (!_tmp4) {
								break;
							}
						}
						_tmp3 = TRUE;
						name = NULL;
						pattern = NULL;
						test = NULL;
						callback = (_tmp5 = NULL, callback_target = NULL, _tmp5);
						data = NULL;
						gtk_tree_model_get (model, &iter, 1, &name, 2, &pattern, 3, &callback, 4, &data, -1);
						_tmp6 = NULL;
						test = (_tmp6 = g_strdup_printf ("%s[ ]*%s$", name, pattern), test = (g_free (test), NULL), _tmp6);
						if (g_regex_match_simple (test, g_strstrip (value), G_REGEX_CASELESS, 0)) {
							char* param;
							const char* _tmp9;
							char* param_str;
							param = NULL;
							fprintf (stdout, "matched: %s to %s\n", test, g_strstrip (value));
							if (string_get_length (value) > string_get_length (name)) {
								char* _tmp7;
								_tmp7 = NULL;
								param = (_tmp7 = string_substring (value, string_get_length (name), (glong) (-1)), param = (g_free (param), NULL), _tmp7);
							} else {
								char* _tmp8;
								_tmp8 = NULL;
								param = (_tmp8 = g_strdup (""), param = (g_free (param), NULL), _tmp8);
							}
							_tmp9 = NULL;
							param_str = (_tmp9 = g_strstrip (param), (_tmp9 == NULL) ? NULL : g_strdup (_tmp9));
							callback (data, param_str, callback_target);
							found = TRUE;
							param = (g_free (param), NULL);
							param_str = (g_free (param_str), NULL);
						}
						name = (g_free (name), NULL);
						pattern = (g_free (pattern), NULL);
						test = (g_free (test), NULL);
					} while (TRUE);
				}
				/* If now exact match is found, use the partial matching that is
				             * also used by the completion popup.
				             * First, partial, match is taken.
				             */
				if (!found) {
					if (gtk_tree_model_get_iter_first (model, &iter)) {
						gboolean _tmp10;
						_tmp10 = FALSE;
						do {
							char* name;
							char* pattern;
							char* test;
							GmpcEasyCommandCallback _tmp12;
							void* callback_target;
							GmpcEasyCommandCallback callback;
							void* data;
							char* _tmp13;
							if (_tmp10) {
								gboolean _tmp11;
								_tmp11 = FALSE;
								if (gtk_tree_model_iter_next (model, &iter)) {
									_tmp11 = !found;
								} else {
									_tmp11 = FALSE;
								}
								if (!_tmp11) {
									break;
								}
							}
							_tmp10 = TRUE;
							name = NULL;
							pattern = NULL;
							test = NULL;
							callback = (_tmp12 = NULL, callback_target = NULL, _tmp12);
							data = NULL;
							gtk_tree_model_get (model, &iter, 1, &name, 2, &pattern, 3, &callback, 4, &data, -1);
							_tmp13 = NULL;
							test = (_tmp13 = g_strdup_printf ("^%s.*", g_strstrip (value)), test = (g_free (test), NULL), _tmp13);
							if (g_regex_match_simple (test, name, G_REGEX_CASELESS, 0)) {
								char* param;
								const char* _tmp16;
								char* param_str;
								param = NULL;
								fprintf (stdout, "matched: %s to %s\n", test, name);
								if (string_get_length (value) > string_get_length (name)) {
									char* _tmp14;
									_tmp14 = NULL;
									param = (_tmp14 = string_substring (value, string_get_length (name), (glong) (-1)), param = (g_free (param), NULL), _tmp14);
								} else {
									char* _tmp15;
									_tmp15 = NULL;
									param = (_tmp15 = g_strdup (""), param = (g_free (param), NULL), _tmp15);
								}
								_tmp16 = NULL;
								param_str = (_tmp16 = g_strstrip (param), (_tmp16 == NULL) ? NULL : g_strdup (_tmp16));
								callback (data, param_str, callback_target);
								found = TRUE;
								param = (g_free (param), NULL);
								param_str = (g_free (param_str), NULL);
							} else {
								fprintf (stdout, "!matched: %s to %s\n", test, name);
							}
							name = (g_free (name), NULL);
							pattern = (g_free (pattern), NULL);
							test = (g_free (test), NULL);
						} while (TRUE);
					}
				}
				/* If we still cannot match it, give a message */
				if (!found) {
					char* _tmp17;
					_tmp17 = NULL;
					playlist3_show_error_message (_tmp17 = g_strdup_printf ("Unknown command: '%s'", g_strstrip (value)), ERROR_INFO);
					_tmp17 = (g_free (_tmp17), NULL);
				}
				value = (g_free (value), NULL);
			}
		}
		value_collection = (_vala_array_free (value_collection, value_collection_length1, (GDestroyNotify) g_free), NULL);
	}
	gtk_object_destroy ((GtkObject*) self->priv->window);
	_tmp19 = NULL;
	self->priv->window = (_tmp19 = NULL, (self->priv->window == NULL) ? NULL : (self->priv->window = (g_object_unref (self->priv->window), NULL)), _tmp19);
	value_unsplit = (g_free (value_unsplit), NULL);
}


static gboolean gmpc_easy_command_key_press_event (GmpcEasyCommand* self, GtkEntry* widget, const GdkEventKey* event) {
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


static gboolean gmpc_easy_command_popup_expose_handler (GmpcEasyCommand* self, GtkWindow* widget, const GdkEventExpose* event) {
	cairo_t* ctx;
	gint width;
	gint height;
	cairo_pattern_t* pattern;
	gboolean _tmp0;
	g_return_val_if_fail (self != NULL, FALSE);
	g_return_val_if_fail (widget != NULL, FALSE);
	ctx = gdk_cairo_create ((GdkDrawable*) ((GtkWidget*) widget)->window);
	width = ((GtkWidget*) widget)->allocation.width;
	height = ((GtkWidget*) widget)->allocation.height;
	if (gtk_widget_is_composited ((GtkWidget*) widget)) {
		cairo_set_operator (ctx, CAIRO_OPERATOR_SOURCE);
		cairo_set_source_rgba (ctx, 1.0, 1.0, 1.0, 0.0);
	} else {
		cairo_set_source_rgb (ctx, 1.0, 1.0, 1.0);
	}
	cairo_paint (ctx);
	/* */
	cairo_rectangle (ctx, 1.0, 1.0, (double) (width - 2), (double) (height - 2));
	pattern = cairo_pattern_create_linear (0.0, 0.0, 0.0, (double) height);
	cairo_pattern_add_color_stop_rgba (pattern, 0.0, 0.0, 0.0, 0.2, 0.5);
	cairo_pattern_add_color_stop_rgba (pattern, 0.5, 0.0, 0.0, 0.0, 1.0);
	cairo_pattern_add_color_stop_rgba (pattern, 1.0, 0.0, 0.0, 0.2, 0.5);
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


/** 
     * Tell gmpc-easy-command to popup.
     * @param self The GmpcEasyCommand object to popup
     *
     * This function will popup GmpcEasyCommand, or if allready open, preset it to the user.
     */
void gmpc_easy_command_popup (GmpcEasyCommand* self) {
	g_return_if_fail (self != NULL);
	/* if not enabled, don't popup */
	if (!gmpc_plugin_base_get_enabled ((GmpcPluginBase*) self)) {
		return;
	}
	if (self->priv->window == NULL) {
		GtkWindow* _tmp0;
		GtkEntry* entry;
		_tmp0 = NULL;
		self->priv->window = (_tmp0 = g_object_ref_sink ((GtkWindow*) gtk_window_new (GTK_WINDOW_TOPLEVEL)), (self->priv->window == NULL) ? NULL : (self->priv->window = (g_object_unref (self->priv->window), NULL)), _tmp0);
		entry = g_object_ref_sink ((GtkEntry*) gtk_entry_new ());
		/* Setup window */
		gtk_window_set_role (self->priv->window, "easy command");
		gtk_window_set_type_hint (self->priv->window, GDK_WINDOW_TYPE_HINT_DIALOG);
		gtk_window_set_decorated (self->priv->window, FALSE);
		gtk_window_set_modal (self->priv->window, TRUE);
		gtk_window_set_keep_above (self->priv->window, TRUE);
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
		if (!playlist3_window_is_hidden ()) {
			gtk_window_set_transient_for (self->priv->window, (GtkWindow *)playlist3_get_window ());
			self->priv->window->position = (guint) GTK_WIN_POS_CENTER_ON_PARENT;
		}
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


void gmpc_easy_command_help_window_destroy (GtkWindow* window, gint response) {
	g_return_if_fail (window != NULL);
	gtk_object_destroy ((GtkObject*) window);
}


static void _gmpc_easy_command_help_window_destroy_gtk_dialog_response (GtkDialog* _sender, gint response_id, gpointer self) {
	gmpc_easy_command_help_window_destroy (_sender, response_id);
}


void gmpc_easy_command_help_window (void* data, const char* param) {
	GmpcEasyCommand* _tmp0;
	GmpcEasyCommand* ec;
	GtkDialog* window;
	GtkTreeView* tree;
	GtkScrolledWindow* sw;
	GtkCellRendererText* renderer;
	GtkTreeViewColumn* column;
	GtkCellRendererText* _tmp1;
	GtkTreeViewColumn* _tmp2;
	g_return_if_fail (param != NULL);
	_tmp0 = NULL;
	ec = (_tmp0 = (GmpcEasyCommand*) data, (_tmp0 == NULL) ? NULL : g_object_ref (_tmp0));
	/*  Create window */
	window = g_object_ref_sink ((GtkDialog*) gtk_dialog_new_with_buttons (_ ("Easy Command help"), NULL, 0, "gtk-close", GTK_RESPONSE_OK, NULL, NULL));
	gtk_window_set_default_size ((GtkWindow*) window, 600, 400);
	/* Treeview with commands */
	tree = g_object_ref_sink ((GtkTreeView*) gtk_tree_view_new ());
	gtk_tree_view_set_model (tree, (GtkTreeModel*) ec->store);
	gtk_tree_view_set_rules_hint (tree, TRUE);
	/* scrolled window to add it in */
	sw = g_object_ref_sink ((GtkScrolledWindow*) gtk_scrolled_window_new (NULL, NULL));
	/* setup scrolled window */
	gtk_container_set_border_width ((GtkContainer*) sw, (guint) 8);
	gtk_scrolled_window_set_shadow_type (sw, GTK_SHADOW_ETCHED_IN);
	gtk_scrolled_window_set_policy (sw, GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	/* add sw */
	gtk_container_add ((GtkContainer*) sw, (GtkWidget*) tree);
	/* Add columns */
	renderer = g_object_ref_sink ((GtkCellRendererText*) gtk_cell_renderer_text_new ());
	column = g_object_ref_sink (gtk_tree_view_column_new ());
	gtk_tree_view_append_column (tree, column);
	gtk_tree_view_column_set_title (column, _ ("Command"));
	gtk_cell_layout_pack_start ((GtkCellLayout*) column, (GtkCellRenderer*) renderer, FALSE);
	gtk_cell_layout_add_attribute ((GtkCellLayout*) column, (GtkCellRenderer*) renderer, "text", 1);
	_tmp1 = NULL;
	renderer = (_tmp1 = g_object_ref_sink ((GtkCellRendererText*) gtk_cell_renderer_text_new ()), (renderer == NULL) ? NULL : (renderer = (g_object_unref (renderer), NULL)), _tmp1);
	_tmp2 = NULL;
	column = (_tmp2 = g_object_ref_sink (gtk_tree_view_column_new ()), (column == NULL) ? NULL : (column = (g_object_unref (column), NULL)), _tmp2);
	gtk_tree_view_append_column (tree, column);
	gtk_cell_layout_pack_start ((GtkCellLayout*) column, (GtkCellRenderer*) renderer, FALSE);
	gtk_tree_view_column_set_title (column, _ ("Usage"));
	gtk_cell_layout_add_attribute ((GtkCellLayout*) column, (GtkCellRenderer*) renderer, "text", 5);
	gtk_box_pack_start ((GtkBox*) window->vbox, (GtkWidget*) sw, TRUE, TRUE, (guint) 0);
	/* show all */
	gtk_widget_show_all ((GtkWidget*) window);
	/* delete event */
	g_signal_connect (window, "response", (GCallback) _gmpc_easy_command_help_window_destroy_gtk_dialog_response, NULL);
	(ec == NULL) ? NULL : (ec = (g_object_unref (ec), NULL));
	(window == NULL) ? NULL : (window = (g_object_unref (window), NULL));
	(tree == NULL) ? NULL : (tree = (g_object_unref (tree), NULL));
	(sw == NULL) ? NULL : (sw = (g_object_unref (sw), NULL));
	(renderer == NULL) ? NULL : (renderer = (g_object_unref (renderer), NULL));
	(column == NULL) ? NULL : (column = (g_object_unref (column), NULL));
}


GmpcEasyCommand* gmpc_easy_command_construct (GType object_type) {
	GmpcEasyCommand * self;
	self = g_object_newv (object_type, 0, NULL);
	return self;
}


GmpcEasyCommand* gmpc_easy_command_new (void) {
	return gmpc_easy_command_construct (GMPC_EASY_TYPE_COMMAND);
}


static gboolean _gmpc_easy_command_completion_function_gtk_entry_completion_match_func (GtkEntryCompletion* completion, const char* key, GtkTreeIter* iter, gpointer self) {
	return gmpc_easy_command_completion_function (self, completion, key, iter);
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
		GtkListStore* _tmp1;
		GtkEntryCompletion* _tmp2;
		GtkCellRendererText* renderer;
		_tmp1 = NULL;
		self->store = (_tmp1 = gtk_list_store_new (6, G_TYPE_UINT, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_POINTER, G_TYPE_POINTER, G_TYPE_STRING, NULL), (self->store == NULL) ? NULL : (self->store = (g_object_unref (self->store), NULL)), _tmp1);
		_tmp2 = NULL;
		self->priv->completion = (_tmp2 = gtk_entry_completion_new (), (self->priv->completion == NULL) ? NULL : (self->priv->completion = (g_object_unref (self->priv->completion), NULL)), _tmp2);
		gtk_entry_completion_set_model (self->priv->completion, (GtkTreeModel*) self->store);
		gtk_entry_completion_set_text_column (self->priv->completion, 1);
		gtk_entry_completion_set_inline_completion (self->priv->completion, TRUE);
		gtk_entry_completion_set_inline_selection (self->priv->completion, TRUE);
		gtk_entry_completion_set_popup_completion (self->priv->completion, TRUE);
		gtk_entry_completion_set_match_func (self->priv->completion, _gmpc_easy_command_completion_function_gtk_entry_completion_match_func, self, NULL);
		renderer = g_object_ref_sink ((GtkCellRendererText*) gtk_cell_renderer_text_new ());
		gtk_cell_layout_pack_end ((GtkCellLayout*) self->priv->completion, (GtkCellRenderer*) renderer, FALSE);
		gtk_cell_layout_add_attribute ((GtkCellLayout*) self->priv->completion, (GtkCellRenderer*) renderer, "text", 5);
		g_object_set ((GObject*) renderer, "foreground", "grey", NULL, NULL);
		gmpc_easy_command_add_entry (self, _ ("Help"), "", _ ("Get a list of available commands"), (GmpcEasyCommandCallback*) gmpc_easy_command_help_window, self);
		(renderer == NULL) ? NULL : (renderer = (g_object_unref (renderer), NULL));
	}
	return obj;
}


static void gmpc_easy_command_class_init (GmpcEasyCommandClass * klass) {
	gmpc_easy_command_parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (GmpcEasyCommandPrivate));
	G_OBJECT_CLASS (klass)->constructor = gmpc_easy_command_constructor;
	G_OBJECT_CLASS (klass)->finalize = gmpc_easy_command_finalize;
	GMPC_PLUGIN_BASE_CLASS (klass)->get_name = gmpc_easy_command_real_get_name;
	GMPC_PLUGIN_BASE_CLASS (klass)->get_version = gmpc_easy_command_real_get_version;
	GMPC_PLUGIN_BASE_CLASS (klass)->save_yourself = gmpc_easy_command_real_save_yourself;
	GMPC_PLUGIN_BASE_CLASS (klass)->get_enabled = gmpc_easy_command_real_get_enabled;
	GMPC_PLUGIN_BASE_CLASS (klass)->set_enabled = gmpc_easy_command_real_set_enabled;
}


static void gmpc_easy_command_instance_init (GmpcEasyCommand * self) {
	gint* _tmp0;
	self->priv = GMPC_EASY_COMMAND_GET_PRIVATE (self);
	self->priv->completion = NULL;
	self->store = NULL;
	self->priv->signals = (guint) 0;
	self->priv->window = NULL;
	self->priv->version = (_tmp0 = g_new0 (gint, 3), _tmp0[0] = 0, _tmp0[1] = 0, _tmp0[2] = 1, _tmp0);
	self->priv->version_length1 = 3;
	_tmp0 = NULL;
}


static void gmpc_easy_command_finalize (GObject* obj) {
	GmpcEasyCommand * self;
	self = GMPC_EASY_COMMAND (obj);
	(self->priv->completion == NULL) ? NULL : (self->priv->completion = (g_object_unref (self->priv->completion), NULL));
	(self->store == NULL) ? NULL : (self->store = (g_object_unref (self->store), NULL));
	(self->priv->window == NULL) ? NULL : (self->priv->window = (g_object_unref (self->priv->window), NULL));
	self->priv->version = (g_free (self->priv->version), NULL);
	G_OBJECT_CLASS (gmpc_easy_command_parent_class)->finalize (obj);
}


GType gmpc_easy_command_get_type (void) {
	static GType gmpc_easy_command_type_id = 0;
	if (gmpc_easy_command_type_id == 0) {
		static const GTypeInfo g_define_type_info = { sizeof (GmpcEasyCommandClass), (GBaseInitFunc) NULL, (GBaseFinalizeFunc) NULL, (GClassInitFunc) gmpc_easy_command_class_init, (GClassFinalizeFunc) NULL, NULL, sizeof (GmpcEasyCommand), 0, (GInstanceInitFunc) gmpc_easy_command_instance_init, NULL };
		gmpc_easy_command_type_id = g_type_register_static (GMPC_PLUGIN_TYPE_BASE, "GmpcEasyCommand", &g_define_type_info, 0);
	}
	return gmpc_easy_command_type_id;
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




