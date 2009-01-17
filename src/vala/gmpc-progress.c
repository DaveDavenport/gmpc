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
#include <pango/pango.h>
#include <stdlib.h>
#include <string.h>
#include <gdk/gdk.h>
#include <float.h>
#include <math.h>




struct _GmpcProgressPrivate {
	guint total;
	guint current;
	gboolean _do_countdown;
	GtkProgressBar* bar;
	GtkStyle* my_style;
};

#define GMPC_PROGRESS_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GMPC_TYPE_PROGRESS, GmpcProgressPrivate))
enum  {
	GMPC_PROGRESS_DUMMY_PROPERTY,
	GMPC_PROGRESS_HIDE_TEXT,
	GMPC_PROGRESS_DO_COUNTDOWN
};
static void gmpc_progress_real_style_set (GtkWidget* base, GtkStyle* old_style);
static void gmpc_progress_real_size_request (GtkWidget* base, GtkRequisition* requisition);
static void gmpc_progress_redraw (GmpcProgress* self);
static gboolean gmpc_progress_on_expose (GmpcProgress* self, GmpcProgress* pb, const GdkEventExpose* event);
static gboolean _gmpc_progress_on_expose_gtk_widget_expose_event (GmpcProgress* _sender, const GdkEventExpose* event, gpointer self);
static GObject * gmpc_progress_constructor (GType type, guint n_construct_properties, GObjectConstructParam * construct_properties);
static gpointer gmpc_progress_parent_class = NULL;
static void gmpc_progress_finalize (GObject* obj);



static void gmpc_progress_real_style_set (GtkWidget* base, GtkStyle* old_style) {
	GmpcProgress * self;
	GtkStyle* _tmp0;
	self = (GmpcProgress*) base;
	/* Reset it, so it gets reloaded on redraw */
	if (self->priv->my_style != NULL) {
		gtk_style_detach (self->priv->my_style);
	}
	_tmp0 = NULL;
	self->priv->my_style = (_tmp0 = NULL, (self->priv->my_style == NULL) ? NULL : (self->priv->my_style = (g_object_unref (self->priv->my_style), NULL)), _tmp0);
}


/* The size_request method Gtk+ is calling on a widget to ask
 it the widget how large it wishes to be. It's not guaranteed
 that gtk+ will actually give this size to the widget*/
static void gmpc_progress_real_size_request (GtkWidget* base, GtkRequisition* requisition) {
	GmpcProgress * self;
	GtkWidget* _tmp0;
	GtkWidget* widget;
	PangoRectangle logical_rect = {0};
	gint width;
	gint height;
	gint xspacing;
	gint yspacing;
	self = (GmpcProgress*) base;
	_tmp0 = NULL;
	widget = (_tmp0 = (GtkWidget*) self, (_tmp0 == NULL) ? NULL : g_object_ref (_tmp0));
	width = 0;
	height = 0;
	xspacing = 0;
	yspacing = 0;
	gtk_widget_set_style ((GtkWidget*) self->priv->bar, self->priv->my_style);
	gtk_widget_style_get ((GtkWidget*) self->priv->bar, "xspacing", &xspacing, "yspacing", &yspacing, NULL, NULL);
	width = (2 * gtk_widget_get_style (widget)->xthickness) + xspacing;
	height = (2 * gtk_widget_get_style (widget)->ythickness) + yspacing;
	if (!gmpc_progress_get_hide_text (self)) {
		PangoLayout* _tmp1;
		PangoLayout* layout;
		_tmp1 = NULL;
		layout = (_tmp1 = gtk_widget_create_pango_layout (widget, " "), (_tmp1 == NULL) ? NULL : g_object_ref (_tmp1));
		pango_layout_get_pixel_extents (layout, NULL, &logical_rect);
		width = width + (logical_rect.width);
		height = height + (logical_rect.height);
		(layout == NULL) ? NULL : (layout = (g_object_unref (layout), NULL));
	}
	(*requisition).width = width;
	(*requisition).height = height;
	(widget == NULL) ? NULL : (widget = (g_object_unref (widget), NULL));
}


/*     int width, height;

        // In this case, we say that we want to be as big as the
        // text is, plus a little border around it.
        if(this.hide_text) {
        requisition.width = 40;
        requisition.height = 10;
        } else {
        var layout = this.create_pango_layout(" ");
        layout.get_size (out width, out height);
        requisition.width = width / Pango.SCALE + 6;
        requisition.height = height / Pango.SCALE + 6;
        }
         */
static void gmpc_progress_redraw (GmpcProgress* self) {
	g_return_if_fail (self != NULL);
	if (((GtkWidget*) self)->window != NULL) {
		gdk_window_process_updates (((GtkWidget*) self)->window, FALSE);
	}
}


static gboolean gmpc_progress_on_expose (GmpcProgress* self, GmpcProgress* pb, const GdkEventExpose* event) {
	PangoRectangle logical_rect = {0};
	gint x;
	gint y;
	gint w;
	gint h;
	gint perc_w;
	gint pos;
	GdkRectangle clip = {0, 0, 0, 0};
	g_return_val_if_fail (self != NULL, FALSE);
	g_return_val_if_fail (pb != NULL, FALSE);
	x = 0;
	y = 0;
	w = 0;
	h = 0;
	perc_w = 0;
	pos = 0;
	if (self->priv->my_style == NULL) {
		GtkStyle* _tmp1;
		GtkStyle* _tmp0;
		GtkStyle* _tmp3;
		GtkStyle* _tmp2;
		_tmp1 = NULL;
		_tmp0 = NULL;
		self->priv->my_style = (_tmp1 = (_tmp0 = gtk_rc_get_style_by_paths (gtk_widget_get_settings ((GtkWidget*) self), "GtkProgressBar", "GtkProgressBar", GTK_TYPE_PROGRESS_BAR), (_tmp0 == NULL) ? NULL : g_object_ref (_tmp0)), (self->priv->my_style == NULL) ? NULL : (self->priv->my_style = (g_object_unref (self->priv->my_style), NULL)), _tmp1);
		_tmp3 = NULL;
		_tmp2 = NULL;
		self->priv->my_style = (_tmp3 = (_tmp2 = gtk_style_attach (self->priv->my_style, (*event).window), (_tmp2 == NULL) ? NULL : g_object_ref (_tmp2)), (self->priv->my_style == NULL) ? NULL : (self->priv->my_style = (g_object_unref (self->priv->my_style), NULL)), _tmp3);
	}
	x = 0;
	y = 0;
	/*  Gtk.paint_box (this.my_style, event.window, Gtk.StateType.NORMAL, Gtk.ShadowType.NONE,
	                        null, this.bar, "", 0,0,this.allocation.width, this.allocation.height);
	                        */
	w = ((GtkWidget*) self)->allocation.width;
	h = ((GtkWidget*) self)->allocation.height;
	gtk_paint_box (self->priv->my_style, (*event).window, GTK_STATE_NORMAL, GTK_SHADOW_IN, &(*event).area, (GtkWidget*) self->priv->bar, "trough", x, y, w, h);
	x = x + (self->priv->my_style->xthickness);
	y = y + (self->priv->my_style->ythickness);
	w = w - (self->priv->my_style->xthickness * 2);
	h = h - (self->priv->my_style->ythickness * 2);
	perc_w = 0;
	if (self->priv->total > 0) {
		perc_w = (gint) (w * (self->priv->current / ((double) self->priv->total)));
	}
	if (perc_w > w) {
		perc_w = w;
	}
	if (perc_w > 0) {
		gtk_paint_box (self->priv->my_style, (*event).window, GTK_STATE_PRELIGHT, GTK_SHADOW_OUT, &(*event).area, (GtkWidget*) self->priv->bar, "bar", x, y, perc_w, h);
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
		char* _tmp9;
		char* _tmp8;
		PangoLayout* _tmp16;
		PangoLayout* layout;
		e_hour = 0;
		e_minutes = 0;
		e_seconds = 0;
		t_hour = ((gint) self->priv->total) / 3600;
		t_minutes = (((gint) self->priv->total) % 3600) / 60;
		t_seconds = ((gint) self->priv->total) % 60;
		a = g_strdup ("");
		p = self->priv->current;
		if (gmpc_progress_get_do_countdown (self)) {
			char* _tmp4;
			p = self->priv->total - self->priv->current;
			_tmp4 = NULL;
			a = (_tmp4 = g_strconcat (a, ("-"), NULL), a = (g_free (a), NULL), _tmp4);
		}
		e_hour = ((gint) p) / 3600;
		e_minutes = ((gint) (p % 3600)) / 60;
		e_seconds = (gint) (p % 60);
		if (e_hour > 0) {
			char* _tmp6;
			char* _tmp5;
			_tmp6 = NULL;
			_tmp5 = NULL;
			a = (_tmp6 = g_strconcat (a, _tmp5 = (g_strdup_printf ("%02i", e_hour)), NULL), a = (g_free (a), NULL), _tmp6);
			_tmp5 = (g_free (_tmp5), NULL);
			if (e_minutes > 0) {
				char* _tmp7;
				_tmp7 = NULL;
				a = (_tmp7 = g_strconcat (a, (":"), NULL), a = (g_free (a), NULL), _tmp7);
			}
		}
		_tmp9 = NULL;
		_tmp8 = NULL;
		a = (_tmp9 = g_strconcat (a, _tmp8 = (g_strdup_printf ("%02i:%02i", e_minutes, e_seconds)), NULL), a = (g_free (a), NULL), _tmp9);
		_tmp8 = (g_free (_tmp8), NULL);
		if (self->priv->total > 0) {
			char* _tmp10;
			char* _tmp15;
			char* _tmp14;
			_tmp10 = NULL;
			a = (_tmp10 = g_strconcat (a, (" -  "), NULL), a = (g_free (a), NULL), _tmp10);
			if (t_hour > 0) {
				char* _tmp12;
				char* _tmp11;
				_tmp12 = NULL;
				_tmp11 = NULL;
				a = (_tmp12 = g_strconcat (a, _tmp11 = (g_strdup_printf ("%02i", t_hour)), NULL), a = (g_free (a), NULL), _tmp12);
				_tmp11 = (g_free (_tmp11), NULL);
				if (t_minutes > 0) {
					char* _tmp13;
					_tmp13 = NULL;
					a = (_tmp13 = g_strconcat (a, (":"), NULL), a = (g_free (a), NULL), _tmp13);
				}
			}
			_tmp15 = NULL;
			_tmp14 = NULL;
			a = (_tmp15 = g_strconcat (a, _tmp14 = (g_strdup_printf ("%02i:%02i", t_minutes, t_seconds)), NULL), a = (g_free (a), NULL), _tmp15);
			_tmp14 = (g_free (_tmp14), NULL);
		}
		_tmp16 = NULL;
		layout = (_tmp16 = gtk_widget_create_pango_layout ((GtkWidget*) pb, a), (_tmp16 == NULL) ? NULL : g_object_ref (_tmp16));
		pango_layout_get_pixel_extents (layout, NULL, &logical_rect);
		pos = (w - logical_rect.width) / 2;
		clip.x = x;
		clip.y = y;
		clip.width = perc_w;
		clip.height = h;
		gtk_paint_layout (self->priv->my_style, ((GtkWidget*) self)->window, GTK_STATE_SELECTED, FALSE, &clip, (GtkWidget*) self->priv->bar, "progressbar", x + pos, y + ((h - logical_rect.height) / 2), layout);
		clip.x = clip.x + clip.width;
		clip.width = w - clip.width;
		gtk_paint_layout (self->priv->my_style, ((GtkWidget*) self)->window, GTK_STATE_NORMAL, FALSE, &clip, (GtkWidget*) self->priv->bar, "progressbar", x + pos, y + ((h - logical_rect.height) / 2), layout);
		a = (g_free (a), NULL);
		(layout == NULL) ? NULL : (layout = (g_object_unref (layout), NULL));
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
		gtk_widget_queue_draw ((GtkWidget*) self);
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
	gtk_widget_queue_resize ((GtkWidget*) self);
	g_object_notify ((GObject *) self, "hide-text");
}


gboolean gmpc_progress_get_do_countdown (GmpcProgress* self) {
	g_return_val_if_fail (self != NULL, FALSE);
	return self->priv->_do_countdown;
}


void gmpc_progress_set_do_countdown (GmpcProgress* self, gboolean value) {
	g_return_if_fail (self != NULL);
	self->priv->_do_countdown = value;
	gmpc_progress_redraw (self);
	g_object_notify ((GObject *) self, "do-countdown");
}


static gboolean _gmpc_progress_on_expose_gtk_widget_expose_event (GmpcProgress* _sender, const GdkEventExpose* event, gpointer self) {
	return gmpc_progress_on_expose (self, _sender, event);
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
		GtkProgressBar* _tmp0;
		/*this.add_events((int)Gdk.EventMask.EXPOSURE_MASK);*/
		g_object_set ((GtkWidget*) self, "app-paintable", TRUE, NULL);
		_tmp0 = NULL;
		self->priv->bar = (_tmp0 = g_object_ref_sink ((GtkProgressBar*) gtk_progress_bar_new ()), (self->priv->bar == NULL) ? NULL : (self->priv->bar = (g_object_unref (self->priv->bar), NULL)), _tmp0);
		gtk_widget_set_parent ((GtkWidget*) self->priv->bar, (GtkWidget*) self);
		g_signal_connect_object ((GtkWidget*) self, "expose-event", (GCallback) _gmpc_progress_on_expose_gtk_widget_expose_event, self, 0);
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
	GTK_WIDGET_CLASS (klass)->style_set = gmpc_progress_real_style_set;
	GTK_WIDGET_CLASS (klass)->size_request = gmpc_progress_real_size_request;
	g_object_class_install_property (G_OBJECT_CLASS (klass), GMPC_PROGRESS_HIDE_TEXT, g_param_spec_boolean ("hide-text", "hide-text", "hide-text", FALSE, G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB | G_PARAM_READABLE | G_PARAM_WRITABLE));
	g_object_class_install_property (G_OBJECT_CLASS (klass), GMPC_PROGRESS_DO_COUNTDOWN, g_param_spec_boolean ("do-countdown", "do-countdown", "do-countdown", FALSE, G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB | G_PARAM_READABLE | G_PARAM_WRITABLE));
}


static void gmpc_progress_instance_init (GmpcProgress * self) {
	self->priv = GMPC_PROGRESS_GET_PRIVATE (self);
	self->priv->total = (guint) 0;
	self->priv->current = (guint) 0;
	self->priv->_do_countdown = FALSE;
	self->priv->bar = NULL;
	self->_hide_text = FALSE;
	self->priv->my_style = NULL;
}


static void gmpc_progress_finalize (GObject* obj) {
	GmpcProgress * self;
	self = GMPC_PROGRESS (obj);
	{
		if (self->priv->my_style != NULL) {
			gtk_style_detach (self->priv->my_style);
		}
	}
	(self->priv->bar == NULL) ? NULL : (self->priv->bar = (g_object_unref (self->priv->bar), NULL));
	(self->priv->my_style == NULL) ? NULL : (self->priv->my_style = (g_object_unref (self->priv->my_style), NULL));
	G_OBJECT_CLASS (gmpc_progress_parent_class)->finalize (obj);
}


GType gmpc_progress_get_type (void) {
	static GType gmpc_progress_type_id = 0;
	if (gmpc_progress_type_id == 0) {
		static const GTypeInfo g_define_type_info = { sizeof (GmpcProgressClass), (GBaseInitFunc) NULL, (GBaseFinalizeFunc) NULL, (GClassInitFunc) gmpc_progress_class_init, (GClassFinalizeFunc) NULL, NULL, sizeof (GmpcProgress), 0, (GInstanceInitFunc) gmpc_progress_instance_init, NULL };
		gmpc_progress_type_id = g_type_register_static (GTK_TYPE_EVENT_BOX, "GmpcProgress", &g_define_type_info, 0);
	}
	return gmpc_progress_type_id;
}




