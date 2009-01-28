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
#include <gdk/gdk.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <math.h>




static glong string_get_length (const char* self);
struct _GmpcProgressPrivate {
	guint total;
	guint current;
	gboolean do_countdown;
	GtkScale* scale;
	GtkLabel* label;
	gulong set_value_handler;
	GtkWindow* tooltip;
	GtkLabel* tooltip_label;
};

#define GMPC_PROGRESS_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GMPC_TYPE_PROGRESS, GmpcProgressPrivate))
enum  {
	GMPC_PROGRESS_DUMMY_PROPERTY,
	GMPC_PROGRESS_HIDE_TEXT
};
static gboolean gmpc_progress_tooltip_expose_event (GmpcProgress* self, GtkWindow* tooltip, const GdkEventExpose* event);
static gboolean _gmpc_progress_tooltip_expose_event_gtk_widget_expose_event (GtkWindow* _sender, const GdkEventExpose* event, gpointer self);
static gboolean gmpc_progress_enter_notify_event (GmpcProgress* self, GtkScale* scale, const GdkEventCrossing* event);
static gboolean gmpc_progress_motion_notify_event (GmpcProgress* self, GtkScale* scale, const GdkEventMotion* event);
static void gmpc_progress_value_changed (GmpcProgress* self, GtkScale* range);
static gboolean gmpc_progress_button_press_event (GmpcProgress* self, GtkScale* scale, const GdkEventButton* event);
static gboolean gmpc_progress_scroll_event (GmpcProgress* self, GtkScale* scale, const GdkEventScroll* event);
static gboolean _gmpc_progress_scroll_event_gtk_widget_scroll_event (GtkScale* _sender, const GdkEventScroll* event, gpointer self);
static gboolean _gmpc_progress_button_press_event_gtk_widget_button_press_event (GtkScale* _sender, const GdkEventButton* event, gpointer self);
static gboolean _gmpc_progress_motion_notify_event_gtk_widget_motion_notify_event (GtkScale* _sender, const GdkEventMotion* event, gpointer self);
static gboolean _gmpc_progress_enter_notify_event_gtk_widget_enter_notify_event (GtkScale* _sender, const GdkEventCrossing* event, gpointer self);
static gboolean _gmpc_progress_enter_notify_event_gtk_widget_leave_notify_event (GtkScale* _sender, const GdkEventCrossing* event, gpointer self);
static GObject * gmpc_progress_constructor (GType type, guint n_construct_properties, GObjectConstructParam * construct_properties);
static gpointer gmpc_progress_parent_class = NULL;
static void gmpc_progress_finalize (GObject* obj);



static glong string_get_length (const char* self) {
	g_return_val_if_fail (self != NULL, 0L);
	return g_utf8_strlen (self, -1);
}


/**
     * Paint a nice box around it
     */
static gboolean gmpc_progress_tooltip_expose_event (GmpcProgress* self, GtkWindow* tooltip, const GdkEventExpose* event) {
	g_return_val_if_fail (self != NULL, FALSE);
	g_return_val_if_fail (tooltip != NULL, FALSE);
	gtk_paint_box (gtk_widget_get_style ((GtkWidget*) tooltip), (*event).window, GTK_STATE_NORMAL, GTK_SHADOW_OUT, NULL, (GtkWidget*) tooltip, "tooltip", 0, 0, ((GtkWidget*) tooltip)->allocation.width, ((GtkWidget*) tooltip)->allocation.height);
	return FALSE;
}


static gboolean _gmpc_progress_tooltip_expose_event_gtk_widget_expose_event (GtkWindow* _sender, const GdkEventExpose* event, gpointer self) {
	return gmpc_progress_tooltip_expose_event (self, _sender, event);
}


static gboolean gmpc_progress_enter_notify_event (GmpcProgress* self, GtkScale* scale, const GdkEventCrossing* event) {
	g_return_val_if_fail (self != NULL, FALSE);
	g_return_val_if_fail (scale != NULL, FALSE);
	/* Create tooltip if mouse enters the event window */
	if ((*event).type == GDK_ENTER_NOTIFY) {
		GtkWindow* _tmp0;
		GtkLabel* _tmp1;
		_tmp0 = NULL;
		self->priv->tooltip = (_tmp0 = g_object_ref_sink ((GtkWindow*) gtk_window_new (GTK_WINDOW_POPUP)), (self->priv->tooltip == NULL) ? NULL : (self->priv->tooltip = (g_object_unref (self->priv->tooltip), NULL)), _tmp0);
		_tmp1 = NULL;
		self->priv->tooltip_label = (_tmp1 = g_object_ref_sink ((GtkLabel*) gtk_label_new ("test")), (self->priv->tooltip_label == NULL) ? NULL : (self->priv->tooltip_label = (g_object_unref (self->priv->tooltip_label), NULL)), _tmp1);
		gtk_container_add ((GtkContainer*) self->priv->tooltip, (GtkWidget*) self->priv->tooltip_label);
		gtk_container_set_border_width ((GtkContainer*) self->priv->tooltip, (guint) 4);
		gtk_widget_set_app_paintable ((GtkWidget*) self->priv->tooltip, TRUE);
		g_signal_connect_object ((GtkWidget*) self->priv->tooltip, "expose-event", (GCallback) _gmpc_progress_tooltip_expose_event_gtk_widget_expose_event, self, 0);
	}
	/* Destroy tooltip if mouse leaves the event window */
	if ((*event).type == GDK_LEAVE_NOTIFY) {
		if (self->priv->tooltip != NULL) {
			GtkWindow* _tmp2;
			gtk_object_destroy ((GtkObject*) self->priv->tooltip);
			_tmp2 = NULL;
			self->priv->tooltip = (_tmp2 = NULL, (self->priv->tooltip == NULL) ? NULL : (self->priv->tooltip = (g_object_unref (self->priv->tooltip), NULL)), _tmp2);
		}
	}
	return FALSE;
}


static gboolean gmpc_progress_motion_notify_event (GmpcProgress* self, GtkScale* scale, const GdkEventMotion* event) {
	g_return_val_if_fail (self != NULL, FALSE);
	g_return_val_if_fail (scale != NULL, FALSE);
	if ((*event).type == GDK_MOTION_NOTIFY) {
		if (self->priv->tooltip != NULL) {
			gint e_hour;
			gint e_minutes;
			gint e_seconds;
			gint t_hour;
			gint t_minutes;
			gint t_seconds;
			char* a;
			guint p;
			char* _tmp5;
			char* _tmp4;
			e_hour = 0;
			e_minutes = 0;
			e_seconds = 0;
			t_hour = ((gint) self->priv->total) / 3600;
			t_minutes = (((gint) self->priv->total) % 3600) / 60;
			t_seconds = ((gint) self->priv->total) % 60;
			a = g_strdup ("");
			p = (guint) (self->priv->total * ((*event).x / ((double) (((GtkWidget*) scale)->allocation.width - gtk_widget_get_style ((GtkWidget*) scale)->xthickness))));
			if (self->priv->do_countdown) {
				char* _tmp0;
				p = (guint) (self->priv->total * ((*event).x / ((double) (((GtkWidget*) scale)->allocation.width - gtk_widget_get_style ((GtkWidget*) scale)->xthickness))));
				_tmp0 = NULL;
				a = (_tmp0 = g_strconcat (a, ("-"), NULL), a = (g_free (a), NULL), _tmp0);
			}
			e_hour = ((gint) p) / 3600;
			e_minutes = ((gint) (p % 3600)) / 60;
			e_seconds = (gint) (p % 60);
			if (e_hour > 0) {
				char* _tmp2;
				char* _tmp1;
				_tmp2 = NULL;
				_tmp1 = NULL;
				a = (_tmp2 = g_strconcat (a, _tmp1 = (g_strdup_printf ("%02i", e_hour)), NULL), a = (g_free (a), NULL), _tmp2);
				_tmp1 = (g_free (_tmp1), NULL);
				if (e_minutes > 0) {
					char* _tmp3;
					_tmp3 = NULL;
					a = (_tmp3 = g_strconcat (a, (":"), NULL), a = (g_free (a), NULL), _tmp3);
				}
			}
			_tmp5 = NULL;
			_tmp4 = NULL;
			a = (_tmp5 = g_strconcat (a, _tmp4 = (g_strdup_printf ("%02i:%02i", e_minutes, e_seconds)), NULL), a = (g_free (a), NULL), _tmp5);
			_tmp4 = (g_free (_tmp4), NULL);
			if (self->priv->total > 0) {
				char* _tmp6;
				char* _tmp11;
				char* _tmp10;
				_tmp6 = NULL;
				a = (_tmp6 = g_strconcat (a, (" - "), NULL), a = (g_free (a), NULL), _tmp6);
				if (t_hour > 0) {
					char* _tmp8;
					char* _tmp7;
					_tmp8 = NULL;
					_tmp7 = NULL;
					a = (_tmp8 = g_strconcat (a, _tmp7 = (g_strdup_printf ("%02i", t_hour)), NULL), a = (g_free (a), NULL), _tmp8);
					_tmp7 = (g_free (_tmp7), NULL);
					if (t_minutes > 0) {
						char* _tmp9;
						_tmp9 = NULL;
						a = (_tmp9 = g_strconcat (a, (":"), NULL), a = (g_free (a), NULL), _tmp9);
					}
				}
				_tmp11 = NULL;
				_tmp10 = NULL;
				a = (_tmp11 = g_strconcat (a, _tmp10 = (g_strdup_printf ("%02i:%02i", t_minutes, t_seconds)), NULL), a = (g_free (a), NULL), _tmp11);
				_tmp10 = (g_free (_tmp10), NULL);
			}
			if (self->priv->do_countdown) {
				gtk_label_set_width_chars (self->priv->tooltip_label, (gint) string_get_length (a));
			} else {
				gtk_label_set_width_chars (self->priv->tooltip_label, ((gint) string_get_length (a)) + 1);
			}
			gtk_label_set_text (self->priv->tooltip_label, a);
			gtk_widget_show_all ((GtkWidget*) self->priv->tooltip);
			gtk_widget_realize ((GtkWidget*) self->priv->tooltip);
			gtk_window_move (self->priv->tooltip, ((gint) (*event).x_root) - (((GtkWidget*) self->priv->tooltip)->allocation.width / 2), ((gint) (*event).y_root) + ((GtkWidget*) self->priv->tooltip)->allocation.height);
			a = (g_free (a), NULL);
		}
	}
	return FALSE;
}


static void gmpc_progress_value_changed (GmpcProgress* self, GtkScale* range) {
	g_return_if_fail (self != NULL);
	g_return_if_fail (range != NULL);
	if (self->priv->total > 0) {
		if (self->priv->do_countdown) {
			guint seconds;
			seconds = (guint) (self->priv->total * (1 - gtk_range_get_value ((GtkRange*) range)));
			g_signal_emit_by_name (self, "seek-event", seconds);
		} else {
			guint seconds;
			seconds = (guint) (self->priv->total * (gtk_range_get_value ((GtkRange*) range)));
			g_signal_emit_by_name (self, "seek-event", seconds);
		}
	}
}


static gboolean gmpc_progress_button_press_event (GmpcProgress* self, GtkScale* scale, const GdkEventButton* event) {
	g_return_val_if_fail (self != NULL, FALSE);
	g_return_val_if_fail (scale != NULL, FALSE);
	if ((*event).type == GDK_BUTTON_PRESS) {
		gboolean _tmp0;
		if ((*event).button == 3) {
			guint cur;
			guint tot;
			self->priv->do_countdown = !self->priv->do_countdown;
			gtk_range_set_inverted ((GtkRange*) self->priv->scale, self->priv->do_countdown);
			cur = self->priv->current;
			tot = self->priv->total;
			self->priv->total = self->priv->current = (guint) 0;
			gmpc_progress_set_time (self, tot, cur);
		}
		_tmp0 = FALSE;
		if ((*event).button == 2) {
			_tmp0 = TRUE;
		} else {
			_tmp0 = (*event).button == 1;
		}
		if (_tmp0) {
			guint p;
			p = (guint) (self->priv->total * ((*event).x / ((double) (((GtkWidget*) scale)->allocation.width - gtk_widget_get_style ((GtkWidget*) scale)->xthickness))));
			g_signal_emit_by_name (self, "seek-event", p);
			return TRUE;
		}
	}
	return FALSE;
}


static gboolean gmpc_progress_scroll_event (GmpcProgress* self, GtkScale* scale, const GdkEventScroll* event) {
	g_return_val_if_fail (self != NULL, FALSE);
	g_return_val_if_fail (scale != NULL, FALSE);
	if ((*event).direction == GDK_SCROLL_UP) {
		g_signal_emit_by_name (self, "seek-event", self->priv->current + 5);
	} else {
		if ((*event).direction == GDK_SCROLL_DOWN) {
			g_signal_emit_by_name (self, "seek-event", self->priv->current - 5);
		}
	}
	return FALSE;
}


void gmpc_progress_set_time (GmpcProgress* self, guint total, guint current) {
	gboolean _tmp0;
	g_return_if_fail (self != NULL);
	if (self->priv->total != total) {
		g_object_set ((GtkWidget*) self->priv->scale, "sensitive", (total > 0), NULL);
	}
	_tmp0 = FALSE;
	if (self->priv->total != total) {
		_tmp0 = TRUE;
	} else {
		_tmp0 = self->priv->current != current;
	}
	if (_tmp0) {
		self->priv->total = total;
		self->priv->current = current;
		g_signal_handler_block (self->priv->scale, self->priv->set_value_handler);
		if (self->priv->total > 0) {
			if (self->priv->do_countdown) {
				gtk_range_set_value ((GtkRange*) self->priv->scale, 1 - (self->priv->current / ((double) self->priv->total)));
			} else {
				gtk_range_set_value ((GtkRange*) self->priv->scale, self->priv->current / ((double) self->priv->total));
			}
		} else {
			gtk_range_set_value ((GtkRange*) self->priv->scale, 0.0);
		}
		g_signal_handler_unblock (self->priv->scale, self->priv->set_value_handler);
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
			if (self->priv->do_countdown) {
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
			if (self->priv->do_countdown) {
				gtk_label_set_width_chars (self->priv->label, (gint) string_get_length (a));
			} else {
				gtk_label_set_width_chars (self->priv->label, ((gint) string_get_length (a)) + 1);
			}
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


static gboolean _gmpc_progress_scroll_event_gtk_widget_scroll_event (GtkScale* _sender, const GdkEventScroll* event, gpointer self) {
	return gmpc_progress_scroll_event (self, _sender, event);
}


static gboolean _gmpc_progress_button_press_event_gtk_widget_button_press_event (GtkScale* _sender, const GdkEventButton* event, gpointer self) {
	return gmpc_progress_button_press_event (self, _sender, event);
}


static gboolean _gmpc_progress_motion_notify_event_gtk_widget_motion_notify_event (GtkScale* _sender, const GdkEventMotion* event, gpointer self) {
	return gmpc_progress_motion_notify_event (self, _sender, event);
}


static gboolean _gmpc_progress_enter_notify_event_gtk_widget_enter_notify_event (GtkScale* _sender, const GdkEventCrossing* event, gpointer self) {
	return gmpc_progress_enter_notify_event (self, _sender, event);
}


static gboolean _gmpc_progress_enter_notify_event_gtk_widget_leave_notify_event (GtkScale* _sender, const GdkEventCrossing* event, gpointer self) {
	return gmpc_progress_enter_notify_event (self, _sender, event);
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
		self->priv->set_value_handler = g_signal_connect_swapped (self->priv->scale, "value_changed", (GCallback) gmpc_progress_value_changed, self);
		gtk_range_set_update_policy ((GtkRange*) self->priv->scale, GTK_UPDATE_DISCONTINUOUS);
		g_object_set ((GtkWidget*) self->priv->scale, "sensitive", FALSE, NULL);
		gtk_widget_add_events ((GtkWidget*) self->priv->scale, (gint) GDK_SCROLL_MASK);
		gtk_widget_add_events ((GtkWidget*) self->priv->scale, (gint) GDK_POINTER_MOTION_MASK);
		gtk_widget_add_events ((GtkWidget*) self->priv->scale, (gint) GDK_ENTER_NOTIFY_MASK);
		gtk_widget_add_events ((GtkWidget*) self->priv->scale, (gint) GDK_LEAVE_NOTIFY_MASK);
		g_signal_connect_object ((GtkWidget*) self->priv->scale, "scroll-event", (GCallback) _gmpc_progress_scroll_event_gtk_widget_scroll_event, self, 0);
		g_signal_connect_object ((GtkWidget*) self->priv->scale, "button-press-event", (GCallback) _gmpc_progress_button_press_event_gtk_widget_button_press_event, self, 0);
		g_signal_connect_object ((GtkWidget*) self->priv->scale, "motion-notify-event", (GCallback) _gmpc_progress_motion_notify_event_gtk_widget_motion_notify_event, self, 0);
		g_signal_connect_object ((GtkWidget*) self->priv->scale, "enter-notify-event", (GCallback) _gmpc_progress_enter_notify_event_gtk_widget_enter_notify_event, self, 0);
		g_signal_connect_object ((GtkWidget*) self->priv->scale, "leave-notify-event", (GCallback) _gmpc_progress_enter_notify_event_gtk_widget_leave_notify_event, self, 0);
		_tmp1 = NULL;
		self->priv->label = (_tmp1 = g_object_ref_sink ((GtkLabel*) gtk_label_new ("")), (self->priv->label == NULL) ? NULL : (self->priv->label = (g_object_unref (self->priv->label), NULL)), _tmp1);
		gtk_misc_set_alignment ((GtkMisc*) self->priv->label, 1.0f, 0.5f);
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
	g_signal_new ("seek_event", GMPC_TYPE_PROGRESS, G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__UINT, G_TYPE_NONE, 1, G_TYPE_UINT);
}


static void gmpc_progress_instance_init (GmpcProgress * self) {
	self->priv = GMPC_PROGRESS_GET_PRIVATE (self);
	self->priv->total = (guint) 0;
	self->priv->current = (guint) 0;
	self->priv->do_countdown = FALSE;
	self->_hide_text = FALSE;
	self->priv->scale = NULL;
	self->priv->label = NULL;
	self->priv->set_value_handler = (gulong) 0;
	self->priv->tooltip = NULL;
	self->priv->tooltip_label = NULL;
}


static void gmpc_progress_finalize (GObject* obj) {
	GmpcProgress * self;
	self = GMPC_PROGRESS (obj);
	{
		/* If there is a tooltip on destruction of slider, destroy it */
		if (self->priv->tooltip != NULL) {
			GtkWindow* _tmp2;
			gtk_object_destroy ((GtkObject*) self->priv->tooltip);
			_tmp2 = NULL;
			self->priv->tooltip = (_tmp2 = NULL, (self->priv->tooltip == NULL) ? NULL : (self->priv->tooltip = (g_object_unref (self->priv->tooltip), NULL)), _tmp2);
		}
	}
	(self->priv->scale == NULL) ? NULL : (self->priv->scale = (g_object_unref (self->priv->scale), NULL));
	(self->priv->label == NULL) ? NULL : (self->priv->label = (g_object_unref (self->priv->label), NULL));
	(self->priv->tooltip == NULL) ? NULL : (self->priv->tooltip = (g_object_unref (self->priv->tooltip), NULL));
	(self->priv->tooltip_label == NULL) ? NULL : (self->priv->tooltip_label = (g_object_unref (self->priv->tooltip_label), NULL));
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




