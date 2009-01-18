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

#include "gmpc-progress.h"
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gdk/gdk.h>




static glong string_get_length (const char* self);
struct _GmpcProgressPrivate {
	guint total;
	guint current;
	gboolean _do_countdown;
	GtkScale* scale;
	GtkLabel* label;
};

#define GMPC_PROGRESS_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GMPC_TYPE_PROGRESS, GmpcProgressPrivate))
enum  {
	GMPC_PROGRESS_DUMMY_PROPERTY,
	GMPC_PROGRESS_HIDE_TEXT,
	GMPC_PROGRESS_DO_COUNTDOWN
};
static void gmpc_progress_value_changed (GmpcProgress* self, GtkRange* range);
static gboolean gmpc_progress_button_press_event (GmpcProgress* self, GtkWidget* scale, const GdkEventButton* event);
static gboolean gmpc_progress_scroll_event (GmpcProgress* self, GtkScale* scale, const GdkEventScroll* event);
static void _gmpc_progress_value_changed_gtk_range_value_changed (GtkScale* _sender, gpointer self);
static gboolean _gmpc_progress_scroll_event_gtk_widget_scroll_event (GtkScale* _sender, const GdkEventScroll* event, gpointer self);
static gboolean _gmpc_progress_button_press_event_gtk_widget_button_press_event (GtkScale* _sender, const GdkEventButton* event, gpointer self);
static GObject * gmpc_progress_constructor (GType type, guint n_construct_properties, GObjectConstructParam * construct_properties);
static gpointer gmpc_progress_parent_class = NULL;
static void gmpc_progress_finalize (GObject* obj);



static glong string_get_length (const char* self) {
	g_return_val_if_fail (self != NULL, 0L);
	return g_utf8_strlen (self, -1);
}


static void gmpc_progress_value_changed (GmpcProgress* self, GtkRange* range) {
	g_return_if_fail (self != NULL);
	g_return_if_fail (range != NULL);
	if (self->priv->total > 0) {
		if (self->priv->_do_countdown) {
			if (gtk_range_get_value (range) != (1 - (self->priv->current / ((double) self->priv->total)))) {
				fprintf (stdout, "changed\n");
				g_signal_emit_by_name (self, "seek-event", (guint) ((1 - gtk_range_get_value (range)) * self->priv->total));
			}
		} else {
			if (gtk_range_get_value (range) != (self->priv->current / ((double) self->priv->total))) {
				fprintf (stdout, "changed\n");
				g_signal_emit_by_name (self, "seek-event", (guint) (gtk_range_get_value (range) * self->priv->total));
			}
		}
	}
}


static gboolean gmpc_progress_button_press_event (GmpcProgress* self, GtkWidget* scale, const GdkEventButton* event) {
	g_return_val_if_fail (self != NULL, FALSE);
	g_return_val_if_fail (scale != NULL, FALSE);
	if ((*event).type == GDK_BUTTON_PRESS) {
		if ((*event).button == 3) {
			guint cur;
			guint tot;
			fprintf (stdout, "right button press\n");
			self->priv->_do_countdown = !self->priv->_do_countdown;
			gtk_range_set_inverted ((GtkRange*) self->priv->scale, self->priv->_do_countdown);
			cur = self->priv->current;
			tot = self->priv->total;
			self->priv->total = self->priv->current = (guint) 0;
			gmpc_progress_set_time (self, tot, cur);
		}
	}
	return FALSE;
}


static gboolean gmpc_progress_scroll_event (GmpcProgress* self, GtkScale* scale, const GdkEventScroll* event) {
	g_return_val_if_fail (self != NULL, FALSE);
	g_return_val_if_fail (scale != NULL, FALSE);
	fprintf (stdout, "scrolling\n");
	if ((*event).direction == GDK_SCROLL_UP) {
		if (gmpc_progress_get_do_countdown (self)) {
			gtk_range_set_value ((GtkRange*) scale, 1 - ((self->priv->current + 5) / ((double) self->priv->total)));
		} else {
			gtk_range_set_value ((GtkRange*) scale, (self->priv->current + 5) / ((double) self->priv->total));
		}
	} else {
		if ((*event).direction == GDK_SCROLL_DOWN) {
			if (gmpc_progress_get_do_countdown (self)) {
				gtk_range_set_value ((GtkRange*) scale, 1 - ((self->priv->current - 5) / ((double) self->priv->total)));
			} else {
				gtk_range_set_value ((GtkRange*) scale, (self->priv->current - 5) / ((double) self->priv->total));
			}
		}
	}
	return FALSE;
}


void gmpc_progress_set_time (GmpcProgress* self, guint total, guint current) {
	gboolean _tmp0;
	g_return_if_fail (self != NULL);
	_tmp0 = FALSE;
	if (self->priv->total != total) {
		_tmp0 = TRUE;
	} else {
		_tmp0 = self->priv->current != current;
	}
	if (_tmp0) {
		self->priv->total = total;
		self->priv->current = current;
		if (self->priv->total > 0) {
			if (self->priv->_do_countdown) {
				gtk_range_set_value ((GtkRange*) self->priv->scale, 1 - (self->priv->current / ((double) self->priv->total)));
			} else {
				gtk_range_set_value ((GtkRange*) self->priv->scale, self->priv->current / ((double) self->priv->total));
			}
		} else {
			gtk_range_set_value ((GtkRange*) self->priv->scale, 0.0);
		}
		if (gmpc_progress_get_hide_text (self) == FALSE) {
			gint e_hour;
			gint e_minutes;
			gint e_seconds;
			gint t_hour;
			gint t_minutes;
			gint t_seconds;
			char* a;
			guint p;
			char* _tmp6;
			char* _tmp5;
			e_hour = 0;
			e_minutes = 0;
			e_seconds = 0;
			t_hour = ((gint) self->priv->total) / 3600;
			t_minutes = (((gint) self->priv->total) % 3600) / 60;
			t_seconds = ((gint) self->priv->total) % 60;
			a = g_strdup ("");
			p = self->priv->current;
			if (gmpc_progress_get_do_countdown (self)) {
				char* _tmp1;
				p = self->priv->total - self->priv->current;
				_tmp1 = NULL;
				a = (_tmp1 = g_strconcat (a, ("-"), NULL), a = (g_free (a), NULL), _tmp1);
			}
			e_hour = ((gint) p) / 3600;
			e_minutes = ((gint) (p % 3600)) / 60;
			e_seconds = (gint) (p % 60);
			if (e_hour > 0) {
				char* _tmp3;
				char* _tmp2;
				_tmp3 = NULL;
				_tmp2 = NULL;
				a = (_tmp3 = g_strconcat (a, _tmp2 = (g_strdup_printf ("%02i", e_hour)), NULL), a = (g_free (a), NULL), _tmp3);
				_tmp2 = (g_free (_tmp2), NULL);
				if (e_minutes > 0) {
					char* _tmp4;
					_tmp4 = NULL;
					a = (_tmp4 = g_strconcat (a, (":"), NULL), a = (g_free (a), NULL), _tmp4);
				}
			}
			_tmp6 = NULL;
			_tmp5 = NULL;
			a = (_tmp6 = g_strconcat (a, _tmp5 = (g_strdup_printf ("%02i:%02i", e_minutes, e_seconds)), NULL), a = (g_free (a), NULL), _tmp6);
			_tmp5 = (g_free (_tmp5), NULL);
			if (self->priv->total > 0) {
				char* _tmp7;
				char* _tmp12;
				char* _tmp11;
				_tmp7 = NULL;
				a = (_tmp7 = g_strconcat (a, (" - "), NULL), a = (g_free (a), NULL), _tmp7);
				if (t_hour > 0) {
					char* _tmp9;
					char* _tmp8;
					_tmp9 = NULL;
					_tmp8 = NULL;
					a = (_tmp9 = g_strconcat (a, _tmp8 = (g_strdup_printf ("%02i", t_hour)), NULL), a = (g_free (a), NULL), _tmp9);
					_tmp8 = (g_free (_tmp8), NULL);
					if (t_minutes > 0) {
						char* _tmp10;
						_tmp10 = NULL;
						a = (_tmp10 = g_strconcat (a, (":"), NULL), a = (g_free (a), NULL), _tmp10);
					}
				}
				_tmp12 = NULL;
				_tmp11 = NULL;
				a = (_tmp12 = g_strconcat (a, _tmp11 = (g_strdup_printf ("%02i:%02i", t_minutes, t_seconds)), NULL), a = (g_free (a), NULL), _tmp12);
				_tmp11 = (g_free (_tmp11), NULL);
			}
			gtk_label_set_width_chars (self->priv->label, (gint) string_get_length (a));
			gtk_label_set_text (self->priv->label, a);
			a = (g_free (a), NULL);
		}
	}
}


GmpcProgress* gmpc_progress_construct (GType object_type) {
	GmpcProgress * self;
	self = g_object_newv (object_type, 0, NULL);
	return self;
}


GmpcProgress* gmpc_progress_new (void) {
	return gmpc_progress_construct (GMPC_TYPE_PROGRESS);
}


gboolean gmpc_progress_get_hide_text (GmpcProgress* self) {
	g_return_val_if_fail (self != NULL, FALSE);
	return self->_hide_text;
}


void gmpc_progress_set_hide_text (GmpcProgress* self, gboolean value) {
	g_return_if_fail (self != NULL);
	self->_hide_text = value;
	if (self->_hide_text) {
		gtk_widget_hide ((GtkWidget*) self->priv->label);
	} else {
		gtk_widget_show ((GtkWidget*) self->priv->label);
	}
	g_object_notify ((GObject *) self, "hide-text");
}


gboolean gmpc_progress_get_do_countdown (GmpcProgress* self) {
	g_return_val_if_fail (self != NULL, FALSE);
	return self->priv->_do_countdown;
}


void gmpc_progress_set_do_countdown (GmpcProgress* self, gboolean value) {
	g_return_if_fail (self != NULL);
	self->priv->_do_countdown = value;
	g_object_notify ((GObject *) self, "do-countdown");
}


static void _gmpc_progress_value_changed_gtk_range_value_changed (GtkScale* _sender, gpointer self) {
	gmpc_progress_value_changed (self, _sender);
}


static gboolean _gmpc_progress_scroll_event_gtk_widget_scroll_event (GtkScale* _sender, const GdkEventScroll* event, gpointer self) {
	return gmpc_progress_scroll_event (self, _sender, event);
}


static gboolean _gmpc_progress_button_press_event_gtk_widget_button_press_event (GtkScale* _sender, const GdkEventButton* event, gpointer self) {
	return gmpc_progress_button_press_event (self, _sender, event);
}


/* Construct function */
static GObject * gmpc_progress_constructor (GType type, guint n_construct_properties, GObjectConstructParam * construct_properties) {
	GObject * obj;
	GmpcProgressClass * klass;
	GObjectClass * parent_class;
	GmpcProgress * self;
	klass = GMPC_PROGRESS_CLASS (g_type_class_peek (GMPC_TYPE_PROGRESS));
	parent_class = G_OBJECT_CLASS (g_type_class_peek_parent (klass));
	obj = parent_class->constructor (type, n_construct_properties, construct_properties);
	self = GMPC_PROGRESS (obj);
	{
		GtkScale* _tmp0;
		GtkLabel* _tmp1;
		_tmp0 = NULL;
		self->priv->scale = (_tmp0 = (GtkScale*) g_object_ref_sink ((GtkHScale*) gtk_hscale_new (NULL)), (self->priv->scale == NULL) ? NULL : (self->priv->scale = (g_object_unref (self->priv->scale), NULL)), _tmp0);
		gtk_range_set_range ((GtkRange*) self->priv->scale, 0.0, 1.0);
		gtk_scale_set_draw_value (self->priv->scale, FALSE);
		g_signal_connect_object ((GtkRange*) self->priv->scale, "value-changed", (GCallback) _gmpc_progress_value_changed_gtk_range_value_changed, self, 0);
		gtk_widget_add_events ((GtkWidget*) self->priv->scale, (gint) GDK_SCROLL_MASK);
		g_signal_connect_object ((GtkWidget*) self->priv->scale, "scroll-event", (GCallback) _gmpc_progress_scroll_event_gtk_widget_scroll_event, self, 0);
		g_signal_connect_object ((GtkWidget*) self->priv->scale, "button-press-event", (GCallback) _gmpc_progress_button_press_event_gtk_widget_button_press_event, self, 0);
		_tmp1 = NULL;
		self->priv->label = (_tmp1 = g_object_ref_sink ((GtkLabel*) gtk_label_new ("")), (self->priv->label == NULL) ? NULL : (self->priv->label = (g_object_unref (self->priv->label), NULL)), _tmp1);
		gtk_misc_set_alignment ((GtkMisc*) self->priv->label, 1.0f, 0.5f);
		g_signal_connect_object ((GtkWidget*) self->priv->label, "button-press-event", (GCallback) _gmpc_progress_button_press_event_gtk_widget_button_press_event, self, 0);
		gtk_box_pack_start ((GtkBox*) self, (GtkWidget*) self->priv->scale, TRUE, TRUE, (guint) 0);
		gtk_box_pack_end ((GtkBox*) self, (GtkWidget*) self->priv->label, FALSE, TRUE, (guint) 0);
		gtk_widget_show_all ((GtkWidget*) self);
	}
	return obj;
}


static void gmpc_progress_get_property (GObject * object, guint property_id, GValue * value, GParamSpec * pspec) {
	GmpcProgress * self;
	gpointer boxed;
	self = GMPC_PROGRESS (object);
	switch (property_id) {
		case GMPC_PROGRESS_HIDE_TEXT:
		g_value_set_boolean (value, gmpc_progress_get_hide_text (self));
		break;
		case GMPC_PROGRESS_DO_COUNTDOWN:
		g_value_set_boolean (value, gmpc_progress_get_do_countdown (self));
		break;
		default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
		break;
	}
}


static void gmpc_progress_set_property (GObject * object, guint property_id, const GValue * value, GParamSpec * pspec) {
	GmpcProgress * self;
	self = GMPC_PROGRESS (object);
	switch (property_id) {
		case GMPC_PROGRESS_HIDE_TEXT:
		gmpc_progress_set_hide_text (self, g_value_get_boolean (value));
		break;
		case GMPC_PROGRESS_DO_COUNTDOWN:
		gmpc_progress_set_do_countdown (self, g_value_get_boolean (value));
		break;
		default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
		break;
	}
}


static void gmpc_progress_class_init (GmpcProgressClass * klass) {
	gmpc_progress_parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (GmpcProgressPrivate));
	G_OBJECT_CLASS (klass)->get_property = gmpc_progress_get_property;
	G_OBJECT_CLASS (klass)->set_property = gmpc_progress_set_property;
	G_OBJECT_CLASS (klass)->constructor = gmpc_progress_constructor;
	G_OBJECT_CLASS (klass)->finalize = gmpc_progress_finalize;
	g_object_class_install_property (G_OBJECT_CLASS (klass), GMPC_PROGRESS_HIDE_TEXT, g_param_spec_boolean ("hide-text", "hide-text", "hide-text", FALSE, G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB | G_PARAM_READABLE | G_PARAM_WRITABLE));
	g_object_class_install_property (G_OBJECT_CLASS (klass), GMPC_PROGRESS_DO_COUNTDOWN, g_param_spec_boolean ("do-countdown", "do-countdown", "do-countdown", FALSE, G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB | G_PARAM_READABLE | G_PARAM_WRITABLE));
	g_signal_new ("seek_event", GMPC_TYPE_PROGRESS, G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__UINT, G_TYPE_NONE, 1, G_TYPE_UINT);
}


static void gmpc_progress_instance_init (GmpcProgress * self) {
	self->priv = GMPC_PROGRESS_GET_PRIVATE (self);
	self->priv->total = (guint) 0;
	self->priv->current = (guint) 0;
	self->priv->_do_countdown = FALSE;
	self->_hide_text = FALSE;
	self->priv->scale = NULL;
	self->priv->label = NULL;
}


static void gmpc_progress_finalize (GObject* obj) {
	GmpcProgress * self;
	self = GMPC_PROGRESS (obj);
	(self->priv->scale == NULL) ? NULL : (self->priv->scale = (g_object_unref (self->priv->scale), NULL));
	(self->priv->label == NULL) ? NULL : (self->priv->label = (g_object_unref (self->priv->label), NULL));
	G_OBJECT_CLASS (gmpc_progress_parent_class)->finalize (obj);
}


GType gmpc_progress_get_type (void) {
	static GType gmpc_progress_type_id = 0;
	if (gmpc_progress_type_id == 0) {
		static const GTypeInfo g_define_type_info = { sizeof (GmpcProgressClass), (GBaseInitFunc) NULL, (GBaseFinalizeFunc) NULL, (GClassInitFunc) gmpc_progress_class_init, (GClassFinalizeFunc) NULL, NULL, sizeof (GmpcProgress), 0, (GInstanceInitFunc) gmpc_progress_instance_init, NULL };
		gmpc_progress_type_id = g_type_register_static (GTK_TYPE_HBOX, "GmpcProgress", &g_define_type_info, 0);
	}
	return gmpc_progress_type_id;
}




