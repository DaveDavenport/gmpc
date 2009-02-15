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

#include "gmpc_image.h"
#include <float.h>
#include <math.h>
#include <gdk/gdk.h>
#include <cairo.h>




struct _GmpcImagePrivate {
	GdkPixbuf* cover;
	gboolean cover_border;
	GdkPixbuf* temp;
	gboolean temp_border;
	double fade;
	guint fade_timeout;
};

#define GMPC_IMAGE_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GMPC_TYPE_IMAGE, GmpcImagePrivate))
enum  {
	GMPC_IMAGE_DUMMY_PROPERTY
};
static gboolean gmpc_image_on_expose (GmpcImage* self, GmpcImage* img, GdkEventExpose* event);
static gboolean gmpc_image_timeout_test (GmpcImage* self);
static gboolean _gmpc_image_timeout_test_gsource_func (gpointer self);
static gboolean _gmpc_image_on_expose_gtk_widget_expose_event (GmpcImage* _sender, GdkEventExpose* event, gpointer self);
static GObject * gmpc_image_constructor (GType type, guint n_construct_properties, GObjectConstructParam * construct_properties);
static gpointer gmpc_image_parent_class = NULL;
static void gmpc_image_dispose (GObject * obj);



static gboolean gmpc_image_on_expose (GmpcImage* self, GmpcImage* img, GdkEventExpose* event) {
	cairo_t* ctx;
	gint width;
	gint height;
	gint x;
	gint y;
	gint ww;
	gint wh;
	gboolean _tmp0;
	g_return_val_if_fail (GMPC_IS_IMAGE (self), FALSE);
	g_return_val_if_fail (GMPC_IS_IMAGE (img), FALSE);
	ctx = gdk_cairo_create (GDK_DRAWABLE (GTK_WIDGET (img)->window));
	width = 0;
	height = 0;
	x = GTK_WIDGET (img)->allocation.x;
	y = GTK_WIDGET (img)->allocation.y;
	ww = GTK_WIDGET (img)->allocation.width;
	wh = GTK_WIDGET (img)->allocation.height;
	cairo_set_antialias (ctx, CAIRO_ANTIALIAS_NONE);
	cairo_rectangle (ctx, ((double) ((*event).area.x)), ((double) ((*event).area.y)), ((double) ((*event).area.width)), ((double) ((*event).area.height)));
	cairo_clip (ctx);
	cairo_save (ctx);
	cairo_set_line_width (ctx, 1.0);
	cairo_set_tolerance (ctx, 0.0);
	if (self->priv->cover != NULL) {
		double fade2;
		width = gdk_pixbuf_get_width (self->priv->cover);
		height = gdk_pixbuf_get_height (self->priv->cover);
		cairo_set_line_join (ctx, CAIRO_LINE_JOIN_ROUND);
		/* Make the path*/
		cairo_new_path (ctx);
		cairo_rectangle (ctx, ((double) (x + (ww - width) / 2)), ((double) (y + (wh - height) / 2)), ((double) (width - 1)), ((double) (height - 1)));
		fade2 = ((self->priv->fade <= 0) ? 1 : self->priv->fade);
		gdk_cairo_set_source_pixbuf (ctx, self->priv->cover, ((double) (x + (ww - width) / 2)), ((double) (y + (wh - height) / 2)));
		/*
		if (cover_border)
		ctx.clip_preserve();
		else
		ctx.clip();
		*/
		cairo_paint_with_alpha (ctx, fade2);
		if (self->priv->cover_border) {
			cairo_set_source_rgba (ctx, ((double) (0)), ((double) (0)), ((double) (0)), fade2);
			cairo_stroke (ctx);
		}
	}
	/*ctx.reset_clip();
	ctx.restore();
	*/
	if (self->priv->temp != NULL) {
		double fade2;
		cairo_new_path (ctx);
		width = gdk_pixbuf_get_width (self->priv->temp);
		height = gdk_pixbuf_get_height (self->priv->temp);
		cairo_rectangle (ctx, ((double) (x + (ww - width) / 2)), ((double) (y + (wh - height) / 2)), ((double) (width - 1)), ((double) (height - 1)));
		gdk_cairo_set_source_pixbuf (ctx, self->priv->temp, ((double) (x + (ww - width) / 2)), ((double) (y + (wh - height) / 2)));
		/*
		if (temp_border)
		ctx.clip_preserve();
		else
		ctx.clip();
		*/
		fade2 = ((self->priv->fade <= 0) ? 1 : self->priv->fade);
		cairo_paint_with_alpha (ctx, 1 - fade2);
		if (self->priv->temp_border) {
			cairo_set_source_rgba (ctx, ((double) (0)), ((double) (0)), ((double) (0)), 1 - fade2);
			cairo_stroke (ctx);
		}
	}
	/*ctx.reset_clip();*/
	return (_tmp0 = TRUE, (ctx == NULL ? NULL : (ctx = (cairo_destroy (ctx), NULL))), _tmp0);
}


static gboolean gmpc_image_timeout_test (GmpcImage* self) {
	g_return_val_if_fail (GMPC_IS_IMAGE (self), FALSE);
	self->priv->fade = self->priv->fade - (0.10);
	if (self->priv->fade <= 0.0) {
		GdkPixbuf* _tmp1;
		GdkPixbuf* _tmp0;
		GdkPixbuf* _tmp2;
		self->priv->fade = ((double) (0));
		_tmp1 = NULL;
		_tmp0 = NULL;
		self->priv->cover = (_tmp1 = (_tmp0 = self->priv->temp, (_tmp0 == NULL ? NULL : g_object_ref (_tmp0))), (self->priv->cover == NULL ? NULL : (self->priv->cover = (g_object_unref (self->priv->cover), NULL))), _tmp1);
		self->priv->cover_border = self->priv->temp_border;
		_tmp2 = NULL;
		self->priv->temp = (_tmp2 = NULL, (self->priv->temp == NULL ? NULL : (self->priv->temp = (g_object_unref (self->priv->temp), NULL))), _tmp2);
		gtk_widget_queue_draw (GTK_WIDGET (self));
		self->priv->fade_timeout = ((guint) (0));
		return FALSE;
	}
	gtk_widget_queue_draw (GTK_WIDGET (self));
	return TRUE;
}


static gboolean _gmpc_image_timeout_test_gsource_func (gpointer self) {
	return gmpc_image_timeout_test (self);
}


/**
     * Set a new pixbuf to be displayed next.
     * param self a GmpcImage to set the pixbuf on.
     * param bug the new GdkPixbuf to display.
     * param border flag that indicates if a border should be drawn.
     *
     * Queues the pixbuf buf to be drawn next. If a pixbuf is allready shown, it will fade out and buf 
     * will fade in.
     */
void gmpc_image_set_pixbuf (GmpcImage* self, GdkPixbuf* buf, gboolean border) {
	GdkPixbuf* _tmp5;
	GdkPixbuf* _tmp7;
	GdkPixbuf* _tmp6;
	g_return_if_fail (GMPC_IS_IMAGE (self));
	g_return_if_fail (GDK_IS_PIXBUF (buf));
	if (self->priv->temp == NULL && self->priv->cover == NULL) {
		GdkPixbuf* _tmp0;
		GdkPixbuf* _tmp2;
		GdkPixbuf* _tmp1;
		self->priv->cover_border = border;
		_tmp0 = NULL;
		self->priv->cover = (_tmp0 = NULL, (self->priv->cover == NULL ? NULL : (self->priv->cover = (g_object_unref (self->priv->cover), NULL))), _tmp0);
		_tmp2 = NULL;
		_tmp1 = NULL;
		self->priv->cover = (_tmp2 = (_tmp1 = buf, (_tmp1 == NULL ? NULL : g_object_ref (_tmp1))), (self->priv->cover == NULL ? NULL : (self->priv->cover = (g_object_unref (self->priv->cover), NULL))), _tmp2);
		gtk_widget_queue_draw (GTK_WIDGET (self));
		return;
	}
	self->priv->fade = 1.0 - self->priv->fade;
	if (self->priv->temp != NULL) {
		GdkPixbuf* _tmp4;
		GdkPixbuf* _tmp3;
		_tmp4 = NULL;
		_tmp3 = NULL;
		self->priv->cover = (_tmp4 = (_tmp3 = self->priv->temp, (_tmp3 == NULL ? NULL : g_object_ref (_tmp3))), (self->priv->cover == NULL ? NULL : (self->priv->cover = (g_object_unref (self->priv->cover), NULL))), _tmp4);
	}
	_tmp5 = NULL;
	self->priv->temp = (_tmp5 = NULL, (self->priv->temp == NULL ? NULL : (self->priv->temp = (g_object_unref (self->priv->temp), NULL))), _tmp5);
	_tmp7 = NULL;
	_tmp6 = NULL;
	self->priv->temp = (_tmp7 = (_tmp6 = buf, (_tmp6 == NULL ? NULL : g_object_ref (_tmp6))), (self->priv->temp == NULL ? NULL : (self->priv->temp = (g_object_unref (self->priv->temp), NULL))), _tmp7);
	self->priv->temp_border = border;
	gtk_widget_queue_draw (GTK_WIDGET (self));
	if (self->priv->fade_timeout > 0) {
		g_source_remove (self->priv->fade_timeout);
	}
	self->priv->fade_timeout = g_timeout_add (((guint) (50)), _gmpc_image_timeout_test_gsource_func, self);
}


/**
     * Clears the image.
     * param self a GmpcImage to clear
     * 
     * Clears the image. Next set_pixbuf won't cause a fade.
     */
void gmpc_image_clear_pixbuf (GmpcImage* self) {
	GdkPixbuf* _tmp0;
	GdkPixbuf* _tmp1;
	g_return_if_fail (GMPC_IS_IMAGE (self));
	self->priv->fade = 0.0;
	_tmp0 = NULL;
	self->priv->temp = (_tmp0 = NULL, (self->priv->temp == NULL ? NULL : (self->priv->temp = (g_object_unref (self->priv->temp), NULL))), _tmp0);
	if (self->priv->fade_timeout > 0) {
		g_source_remove (self->priv->fade_timeout);
		self->priv->fade_timeout = ((guint) (0));
	}
	_tmp1 = NULL;
	self->priv->cover = (_tmp1 = NULL, (self->priv->cover == NULL ? NULL : (self->priv->cover = (g_object_unref (self->priv->cover), NULL))), _tmp1);
	self->priv->cover_border = FALSE;
	gtk_widget_queue_draw (GTK_WIDGET (self));
}


/**
 * Widget that shows a pixbuf by nicely fadeing in and out.
 * Draws a nice border.
 */
GmpcImage* gmpc_image_new (void) {
	GmpcImage * self;
	self = g_object_newv (GMPC_TYPE_IMAGE, 0, NULL);
	return self;
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
		gboolean _tmp0;
		g_object_set (GTK_WIDGET (self), "app-paintable", TRUE, NULL);
		gtk_event_box_set_visible_window (GTK_EVENT_BOX (self), FALSE);
		g_signal_connect_object (GTK_WIDGET (self), "expose-event", ((GCallback) (_gmpc_image_on_expose_gtk_widget_expose_event)), self, 0);
	}
	return obj;
}


static void gmpc_image_class_init (GmpcImageClass * klass) {
	gmpc_image_parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (GmpcImagePrivate));
	G_OBJECT_CLASS (klass)->constructor = gmpc_image_constructor;
	G_OBJECT_CLASS (klass)->dispose = gmpc_image_dispose;
}


static void gmpc_image_instance_init (GmpcImage * self) {
	self->priv = GMPC_IMAGE_GET_PRIVATE (self);
	self->priv->cover = NULL;
	self->priv->cover_border = TRUE;
	self->priv->temp = NULL;
	self->priv->temp_border = TRUE;
	self->priv->fade = 0.0;
	self->priv->fade_timeout = ((guint) (0));
}


static void gmpc_image_dispose (GObject * obj) {
	GmpcImage * self;
	self = GMPC_IMAGE (obj);
	{
		if (self->priv->fade_timeout > 0) {
			g_source_remove (self->priv->fade_timeout);
			self->priv->fade_timeout = ((guint) (0));
		}
	}
	(self->priv->cover == NULL ? NULL : (self->priv->cover = (g_object_unref (self->priv->cover), NULL)));
	(self->priv->temp == NULL ? NULL : (self->priv->temp = (g_object_unref (self->priv->temp), NULL)));
	G_OBJECT_CLASS (gmpc_image_parent_class)->dispose (obj);
}


GType gmpc_image_get_type (void) {
	static GType gmpc_image_type_id = 0;
	if (G_UNLIKELY (gmpc_image_type_id == 0)) {
		static const GTypeInfo g_define_type_info = { sizeof (GmpcImageClass), (GBaseInitFunc) NULL, (GBaseFinalizeFunc) NULL, (GClassInitFunc) gmpc_image_class_init, (GClassFinalizeFunc) NULL, NULL, sizeof (GmpcImage), 0, (GInstanceInitFunc) gmpc_image_instance_init };
		gmpc_image_type_id = g_type_register_static (GTK_TYPE_EVENT_BOX, "GmpcImage", &g_define_type_info, 0);
	}
	return gmpc_image_type_id;
}




