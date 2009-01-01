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

#include "gmpc_image.h"
#include <float.h>
#include <math.h>
#include <cairo.h>
#include <gdk/gdk.h>




struct _GmpcImagePrivate {
	GdkPixbuf* cover;
	gboolean cover_border;
	GdkPixbuf* temp;
	gboolean temp_border;
	double fade;
	guint fade_timeout;
	gboolean cover_round_corners;
	gboolean temp_round_corners;
};

#define GMPC_IMAGE_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GMPC_TYPE_IMAGE, GmpcImagePrivate))
enum  {
	GMPC_IMAGE_DUMMY_PROPERTY
};
static void gmpc_image_draw_curved_rectangle (GmpcImage* self, cairo_t* ctx, double rect_x0, double rect_y0, double rect_width, double rect_height);
static gboolean gmpc_image_on_expose (GmpcImage* self, GmpcImage* img, GdkEventExpose* event);
static gboolean gmpc_image_timeout_test (GmpcImage* self);
static gboolean _gmpc_image_timeout_test_gsource_func (gpointer self);
static gboolean _gmpc_image_on_expose_gtk_widget_expose_event (GmpcImage* _sender, GdkEventExpose* event, gpointer self);
static GObject * gmpc_image_constructor (GType type, guint n_construct_properties, GObjectConstructParam * construct_properties);
static gpointer gmpc_image_parent_class = NULL;
static void gmpc_image_finalize (GObject* obj);



static void gmpc_image_draw_curved_rectangle (GmpcImage* self, cairo_t* ctx, double rect_x0, double rect_y0, double rect_width, double rect_height) {
	double rect_x1;
	double rect_y1;
	double radius;
	gboolean _tmp0;
	g_return_if_fail (self != NULL);
	g_return_if_fail (ctx != NULL);
	rect_x1 = 0.0;
	rect_y1 = 0.0;
	radius = (double) 15;
	/*rect_width/5;*/
	rect_x1 = rect_x0 + rect_width;
	rect_y1 = rect_y0 + rect_height;
	_tmp0 = FALSE;
	if (rect_width == 0) {
		_tmp0 = TRUE;
	} else {
		_tmp0 = rect_height == 0;
	}
	if (_tmp0) {
		return;
	}
	if ((rect_width / 2) < radius) {
		if ((rect_height / 2) < radius) {
			cairo_move_to (ctx, rect_x0, (rect_y0 + rect_y1) / 2);
			cairo_curve_to (ctx, rect_x0, rect_y0, rect_x0, rect_y0, (rect_x0 + rect_x1) / 2, rect_y0);
			cairo_curve_to (ctx, rect_x1, rect_y0, rect_x1, rect_y0, rect_x1, (rect_y0 + rect_y1) / 2);
			cairo_curve_to (ctx, rect_x1, rect_y1, rect_x1, rect_y1, (rect_x1 + rect_x0) / 2, rect_y1);
			cairo_curve_to (ctx, rect_x0, rect_y1, rect_x0, rect_y1, rect_x0, (rect_y0 + rect_y1) / 2);
		} else {
			cairo_move_to (ctx, rect_x0, rect_y0 + radius);
			cairo_curve_to (ctx, rect_x0, rect_y0, rect_x0, rect_y0, (rect_x0 + rect_x1) / 2, rect_y0);
			cairo_curve_to (ctx, rect_x1, rect_y0, rect_x1, rect_y0, rect_x1, rect_y0 + radius);
			cairo_line_to (ctx, rect_x1, rect_y1 - radius);
			cairo_curve_to (ctx, rect_x1, rect_y1, rect_x1, rect_y1, (rect_x1 + rect_x0) / 2, rect_y1);
			cairo_curve_to (ctx, rect_x0, rect_y1, rect_x0, rect_y1, rect_x0, rect_y1 - radius);
		}
	} else {
		if ((rect_height / 2) < radius) {
			cairo_move_to (ctx, rect_x0, (rect_y0 + rect_y1) / 2);
			cairo_curve_to (ctx, rect_x0, rect_y0, rect_x0, rect_y0, rect_x0 + radius, rect_y0);
			cairo_line_to (ctx, rect_x1 - radius, rect_y0);
			cairo_curve_to (ctx, rect_x1, rect_y0, rect_x1, rect_y0, rect_x1, (rect_y0 + rect_y1) / 2);
			cairo_curve_to (ctx, rect_x1, rect_y1, rect_x1, rect_y1, rect_x1 - radius, rect_y1);
			cairo_line_to (ctx, rect_x0 + radius, rect_y1);
			cairo_curve_to (ctx, rect_x0, rect_y1, rect_x0, rect_y1, rect_x0, (rect_y0 + rect_y1) / 2);
		} else {
			cairo_move_to (ctx, rect_x0, rect_y0 + radius);
			cairo_curve_to (ctx, rect_x0, rect_y0, rect_x0, rect_y0, rect_x0 + radius, rect_y0);
			cairo_line_to (ctx, rect_x1 - radius, rect_y0);
			cairo_curve_to (ctx, rect_x1, rect_y0, rect_x1, rect_y0, rect_x1, rect_y0 + radius);
			cairo_line_to (ctx, rect_x1, rect_y1 - radius);
			cairo_curve_to (ctx, rect_x1, rect_y1, rect_x1, rect_y1, rect_x1 - radius, rect_y1);
			cairo_line_to (ctx, rect_x0 + radius, rect_y1);
			cairo_curve_to (ctx, rect_x0, rect_y1, rect_x0, rect_y1, rect_x0, rect_y1 - radius);
		}
	}
	cairo_close_path (ctx);
}


static gboolean gmpc_image_on_expose (GmpcImage* self, GmpcImage* img, GdkEventExpose* event) {
	cairo_t* ctx;
	gint width;
	gint height;
	gint x;
	gint y;
	gint ww;
	gint wh;
	gboolean _tmp1;
	g_return_val_if_fail (self != NULL, FALSE);
	g_return_val_if_fail (img != NULL, FALSE);
	ctx = gdk_cairo_create ((GdkDrawable*) ((GtkWidget*) img)->window);
	width = 0;
	height = 0;
	x = ((GtkWidget*) img)->allocation.x;
	y = ((GtkWidget*) img)->allocation.y;
	ww = ((GtkWidget*) img)->allocation.width;
	wh = ((GtkWidget*) img)->allocation.height;
	cairo_set_line_width (ctx, 0.8);
	cairo_set_tolerance (ctx, 0.1);
	if (self->priv->cover != NULL) {
		double _tmp0;
		double fade2;
		width = gdk_pixbuf_get_width (self->priv->cover);
		height = gdk_pixbuf_get_height (self->priv->cover);
		cairo_set_line_join (ctx, CAIRO_LINE_JOIN_ROUND);
		/* Make the path*/
		cairo_new_path (ctx);
		if (self->priv->cover_round_corners) {
			gmpc_image_draw_curved_rectangle (self, ctx, (x + ((ww - width) / 2)) + 0.5, (y + ((wh - height) / 2)) + 0.5, (double) (width - 1), (double) (height - 1));
		} else {
			cairo_rectangle (ctx, (x + ((ww - width) / 2)) + 0.5, (y + ((wh - height) / 2)) + 0.5, (double) (width - 1), (double) (height - 1));
		}
		_tmp0 = 0.0;
		if ((self->priv->fade <= 0)) {
			_tmp0 = (double) 1;
		} else {
			_tmp0 = self->priv->fade;
		}
		fade2 = _tmp0;
		gdk_cairo_set_source_pixbuf (ctx, self->priv->cover, (double) (x + ((ww - width) / 2)), (double) (y + ((wh - height) / 2)));
		if (self->priv->cover_border) {
			cairo_clip_preserve (ctx);
		} else {
			cairo_clip (ctx);
		}
		cairo_paint_with_alpha (ctx, fade2);
		cairo_reset_clip (ctx);
		if (self->priv->cover_border) {
			cairo_set_source_rgba (ctx, (double) 0, (double) 0, (double) 0, fade2);
			cairo_stroke (ctx);
		}
	}
	if (self->priv->temp != NULL) {
		cairo_new_path (ctx);
		width = gdk_pixbuf_get_width (self->priv->temp);
		height = gdk_pixbuf_get_height (self->priv->temp);
		if (self->priv->temp_round_corners) {
			gmpc_image_draw_curved_rectangle (self, ctx, (x + ((ww - width) / 2)) + 0.5, (y + ((wh - height) / 2)) + 0.5, (double) (width - 1), (double) (height - 1));
		} else {
			cairo_rectangle (ctx, (x + ((ww - width) / 2)) + 0.5, (y + ((wh - height) / 2)) + 0.5, (double) (width - 1), (double) (height - 1));
		}
		gdk_cairo_set_source_pixbuf (ctx, self->priv->temp, (double) (x + ((ww - width) / 2)), (double) (y + ((wh - height) / 2)));
		if (self->priv->temp_border) {
			cairo_clip_preserve (ctx);
		} else {
			cairo_clip (ctx);
		}
		cairo_paint_with_alpha (ctx, 1 - self->priv->fade);
		cairo_reset_clip (ctx);
		if (self->priv->temp_border) {
			cairo_set_source_rgba (ctx, (double) 0, (double) 0, (double) 0, 1 - self->priv->fade);
			cairo_stroke (ctx);
		}
	}
	return (_tmp1 = TRUE, (ctx == NULL) ? NULL : (ctx = (cairo_destroy (ctx), NULL)), _tmp1);
}


static gboolean gmpc_image_timeout_test (GmpcImage* self) {
	g_return_val_if_fail (self != NULL, FALSE);
	self->priv->fade = self->priv->fade - (0.10);
	if (self->priv->fade <= 0.0) {
		GdkPixbuf* _tmp1;
		GdkPixbuf* _tmp0;
		GdkPixbuf* _tmp2;
		_tmp1 = NULL;
		_tmp0 = NULL;
		self->priv->cover = (_tmp1 = (_tmp0 = self->priv->temp, (_tmp0 == NULL) ? NULL : g_object_ref (_tmp0)), (self->priv->cover == NULL) ? NULL : (self->priv->cover = (g_object_unref (self->priv->cover), NULL)), _tmp1);
		self->priv->cover_border = self->priv->temp_border;
		self->priv->cover_round_corners = self->priv->temp_round_corners;
		_tmp2 = NULL;
		self->priv->temp = (_tmp2 = NULL, (self->priv->temp == NULL) ? NULL : (self->priv->temp = (g_object_unref (self->priv->temp), NULL)), _tmp2);
		gtk_widget_queue_draw ((GtkWidget*) self);
		self->priv->fade_timeout = (guint) 0;
		return FALSE;
	}
	gtk_widget_queue_draw ((GtkWidget*) self);
	return TRUE;
}


static gboolean _gmpc_image_timeout_test_gsource_func (gpointer self) {
	return gmpc_image_timeout_test (self);
}


void gmpc_image_set_pixbuf (GmpcImage* self, GdkPixbuf* buf, gboolean border, gboolean round) {
	gboolean _tmp0;
	GdkPixbuf* _tmp4;
	GdkPixbuf* _tmp3;
	g_return_if_fail (self != NULL);
	g_return_if_fail (buf != NULL);
	_tmp0 = FALSE;
	if (self->priv->temp == NULL) {
		_tmp0 = self->priv->cover == NULL;
	} else {
		_tmp0 = FALSE;
	}
	if (_tmp0) {
		GdkPixbuf* _tmp2;
		GdkPixbuf* _tmp1;
		self->priv->cover_border = border;
		_tmp2 = NULL;
		_tmp1 = NULL;
		self->priv->cover = (_tmp2 = (_tmp1 = buf, (_tmp1 == NULL) ? NULL : g_object_ref (_tmp1)), (self->priv->cover == NULL) ? NULL : (self->priv->cover = (g_object_unref (self->priv->cover), NULL)), _tmp2);
		self->priv->cover_round_corners = round;
		gtk_widget_queue_draw ((GtkWidget*) self);
		return;
	}
	self->priv->fade = 1.0;
	_tmp4 = NULL;
	_tmp3 = NULL;
	self->priv->temp = (_tmp4 = (_tmp3 = buf, (_tmp3 == NULL) ? NULL : g_object_ref (_tmp3)), (self->priv->temp == NULL) ? NULL : (self->priv->temp = (g_object_unref (self->priv->temp), NULL)), _tmp4);
	self->priv->temp_border = border;
	self->priv->temp_round_corners = round;
	gtk_widget_queue_draw ((GtkWidget*) self);
	if (self->priv->fade_timeout > 0) {
		g_source_remove (self->priv->fade_timeout);
	}
	self->priv->fade_timeout = g_timeout_add ((guint) 50, _gmpc_image_timeout_test_gsource_func, self);
}


void gmpc_image_clear_pixbuf (GmpcImage* self) {
	GdkPixbuf* _tmp0;
	GdkPixbuf* _tmp1;
	g_return_if_fail (self != NULL);
	self->priv->fade = 0.0;
	_tmp0 = NULL;
	self->priv->temp = (_tmp0 = NULL, (self->priv->temp == NULL) ? NULL : (self->priv->temp = (g_object_unref (self->priv->temp), NULL)), _tmp0);
	if (self->priv->fade_timeout > 0) {
		g_source_remove (self->priv->fade_timeout);
		self->priv->fade_timeout = (guint) 0;
	}
	_tmp1 = NULL;
	self->priv->cover = (_tmp1 = NULL, (self->priv->cover == NULL) ? NULL : (self->priv->cover = (g_object_unref (self->priv->cover), NULL)), _tmp1);
	self->priv->cover_border = FALSE;
	gtk_widget_queue_draw ((GtkWidget*) self);
}


/**
 * Widget that shows a pixbuf by nicely fadeing in and out.
 * Draws a nice border.
 */
GmpcImage* gmpc_image_construct (GType object_type) {
	GmpcImage * self;
	self = g_object_newv (object_type, 0, NULL);
	return self;
}


GmpcImage* gmpc_image_new (void) {
	return gmpc_image_construct (GMPC_TYPE_IMAGE);
}


static gboolean _gmpc_image_on_expose_gtk_widget_expose_event (GmpcImage* _sender, GdkEventExpose* event, gpointer self) {
	return gmpc_image_on_expose (self, _sender, event);
}


static GObject * gmpc_image_constructor (GType type, guint n_construct_properties, GObjectConstructParam * construct_properties) {
	GObject * obj;
	GmpcImageClass * klass;
	GObjectClass * parent_class;
	GmpcImage * self;
	klass = GMPC_IMAGE_CLASS (g_type_class_peek (GMPC_TYPE_IMAGE));
	parent_class = G_OBJECT_CLASS (g_type_class_peek_parent (klass));
	obj = parent_class->constructor (type, n_construct_properties, construct_properties);
	self = GMPC_IMAGE (obj);
	{
		g_object_set ((GtkWidget*) self, "app-paintable", TRUE, NULL);
		gtk_event_box_set_visible_window ((GtkEventBox*) self, FALSE);
		g_signal_connect_object ((GtkWidget*) self, "expose-event", (GCallback) _gmpc_image_on_expose_gtk_widget_expose_event, self, 0);
	}
	return obj;
}


static void gmpc_image_class_init (GmpcImageClass * klass) {
	gmpc_image_parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (GmpcImagePrivate));
	G_OBJECT_CLASS (klass)->constructor = gmpc_image_constructor;
	G_OBJECT_CLASS (klass)->finalize = gmpc_image_finalize;
}


static void gmpc_image_instance_init (GmpcImage * self) {
	self->priv = GMPC_IMAGE_GET_PRIVATE (self);
	self->priv->cover = NULL;
	self->priv->cover_border = TRUE;
	self->priv->temp = NULL;
	self->priv->temp_border = TRUE;
	self->priv->fade = 0.0;
	self->priv->fade_timeout = (guint) 0;
	self->priv->cover_round_corners = (gboolean) 1;
	self->priv->temp_round_corners = (gboolean) 1;
}


static void gmpc_image_finalize (GObject* obj) {
	GmpcImage * self;
	self = GMPC_IMAGE (obj);
	{
		if (self->priv->fade_timeout > 0) {
			g_source_remove (self->priv->fade_timeout);
			self->priv->fade_timeout = (guint) 0;
		}
	}
	(self->priv->cover == NULL) ? NULL : (self->priv->cover = (g_object_unref (self->priv->cover), NULL));
	(self->priv->temp == NULL) ? NULL : (self->priv->temp = (g_object_unref (self->priv->temp), NULL));
	G_OBJECT_CLASS (gmpc_image_parent_class)->finalize (obj);
}


GType gmpc_image_get_type (void) {
	static GType gmpc_image_type_id = 0;
	if (gmpc_image_type_id == 0) {
		static const GTypeInfo g_define_type_info = { sizeof (GmpcImageClass), (GBaseInitFunc) NULL, (GBaseFinalizeFunc) NULL, (GClassInitFunc) gmpc_image_class_init, (GClassFinalizeFunc) NULL, NULL, sizeof (GmpcImage), 0, (GInstanceInitFunc) gmpc_image_instance_init, NULL };
		gmpc_image_type_id = g_type_register_static (GTK_TYPE_EVENT_BOX, "GmpcImage", &g_define_type_info, 0);
	}
	return gmpc_image_type_id;
}




