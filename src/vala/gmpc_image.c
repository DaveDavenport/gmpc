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
#include <gtktransition.h>
#include <float.h>
#include <math.h>
#include <pango/pango.h>
#include <gdk/gdk.h>
#include <cairo.h>
#include <pango/pangocairo.h>
#include <stdio.h>




static glong string_get_length (const char* self);
struct _GmpcImagePrivate {
	GdkPixbuf* cover;
	gboolean cover_border;
	GdkPixbuf* temp;
	gboolean temp_border;
	double fade;
	guint fade_timeout;
	char* _text;
	PangoFontDescription* fd;
};

#define GMPC_IMAGE_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GMPC_TYPE_IMAGE, GmpcImagePrivate))
enum  {
	GMPC_IMAGE_DUMMY_PROPERTY,
	GMPC_IMAGE_TEXT
};
static gboolean gmpc_image_on_expose (GmpcImage* self, GmpcImage* img, const GdkEventExpose* event);
static gboolean gmpc_image_timeout_test (GmpcImage* self);
static gboolean _gmpc_image_timeout_test_gsource_func (gpointer self);
static gboolean _gmpc_image_on_expose_gtk_widget_expose_event (GmpcImage* _sender, const GdkEventExpose* event, gpointer self);
static GObject * gmpc_image_constructor (GType type, guint n_construct_properties, GObjectConstructParam * construct_properties);
static gpointer gmpc_image_parent_class = NULL;
static void gmpc_image_finalize (GObject* obj);



static glong string_get_length (const char* self) {
	g_return_val_if_fail (self != NULL, 0L);
	return g_utf8_strlen (self, -1);
}


static gboolean gmpc_image_on_expose (GmpcImage* self, GmpcImage* img, const GdkEventExpose* event) {
	cairo_t* ctx;
	gint width;
	gint height;
	gint x;
	gint y;
	gint ww;
	gint wh;
	gboolean _tmp2;
	gboolean _tmp3;
	gboolean _tmp5;
	g_return_val_if_fail (self != NULL, FALSE);
	g_return_val_if_fail (img != NULL, FALSE);
	ctx = gdk_cairo_create ((GdkDrawable*) gtk_widget_get_window ((GtkWidget*) img));
	width = 0;
	height = 0;
	x = ((GtkWidget*) img)->allocation.x;
	y = ((GtkWidget*) img)->allocation.y;
	ww = ((GtkWidget*) img)->allocation.width;
	wh = ((GtkWidget*) img)->allocation.height;
	cairo_set_antialias (ctx, CAIRO_ANTIALIAS_NONE);
	cairo_rectangle (ctx, (double) (*event).area.x, (double) (*event).area.y, (double) (*event).area.width, (double) (*event).area.height);
	cairo_clip (ctx);
	cairo_save (ctx);
	cairo_set_line_width (ctx, 1.0);
	cairo_set_tolerance (ctx, 0.0);
	if (self->priv->cover != NULL) {
		double x_start;
		double y_start;
		double _tmp0;
		double fade2;
		width = gdk_pixbuf_get_width (self->priv->cover);
		height = gdk_pixbuf_get_height (self->priv->cover);
		x_start = x + ceil ((ww - width) / 2.0);
		y_start = y + ceil ((wh - height) / 2.0);
		cairo_set_line_join (ctx, CAIRO_LINE_JOIN_ROUND);
		/* Make the path*/
		cairo_new_path (ctx);
		cairo_rectangle (ctx, x_start, y_start, (double) (width - 1), (double) (height - 1));
		_tmp0 = 0.0;
		if (self->priv->fade <= 0) {
			_tmp0 = (double) 1;
		} else {
			_tmp0 = self->priv->fade;
		}
		fade2 = _tmp0;
		gdk_cairo_set_source_pixbuf (ctx, self->priv->cover, x_start, y_start);
		/*
		if (cover_border)
		ctx.clip_preserve();
		else
		ctx.clip();
		*/
		cairo_paint_with_alpha (ctx, fade2);
		if (self->priv->cover_border) {
			cairo_set_source_rgba (ctx, (double) 0, (double) 0, (double) 0, fade2);
			cairo_stroke (ctx);
		} else {
			cairo_new_path (ctx);
		}
	}
	/*ctx.reset_clip();
	ctx.restore();
	*/
	if (self->priv->temp != NULL) {
		double x_start;
		double y_start;
		double _tmp1;
		double fade2;
		cairo_new_path (ctx);
		width = gdk_pixbuf_get_width (self->priv->temp);
		height = gdk_pixbuf_get_height (self->priv->temp);
		x_start = x + ceil ((ww - width) / 2.0);
		y_start = y + ceil ((wh - height) / 2.0);
		cairo_set_line_join (ctx, CAIRO_LINE_JOIN_ROUND);
		cairo_rectangle (ctx, x_start, y_start, (double) (width - 1), (double) (height - 1));
		gdk_cairo_set_source_pixbuf (ctx, self->priv->temp, x_start, y_start);
		_tmp1 = 0.0;
		if (self->priv->fade <= 0) {
			_tmp1 = (double) 1;
		} else {
			_tmp1 = self->priv->fade;
		}
		/*
		if (temp_border)
		ctx.clip_preserve();
		else
		ctx.clip();
		*/
		fade2 = _tmp1;
		cairo_paint_with_alpha (ctx, 1 - fade2);
		if (self->priv->temp_border) {
			cairo_set_source_rgba (ctx, (double) 0, (double) 0, (double) 0, 1 - fade2);
			cairo_stroke (ctx);
		} else {
			cairo_new_path (ctx);
		}
	}
	_tmp2 = FALSE;
	_tmp3 = FALSE;
	if (self->priv->cover != NULL) {
		_tmp3 = self->priv->_text != NULL;
	} else {
		_tmp3 = FALSE;
	}
	if (_tmp3) {
		_tmp2 = string_get_length (self->priv->_text) > 0;
	} else {
		_tmp2 = FALSE;
	}
	/*ctx.reset_clip();*/
	if (_tmp2) {
		PangoLayout* _tmp4;
		PangoLayout* layout;
		gint tw;
		gint th;
		gint size;
		_tmp4 = NULL;
		layout = (_tmp4 = pango_cairo_create_layout (ctx), (_tmp4 == NULL) ? NULL : g_object_ref (_tmp4));
		tw = 0;
		th = 0;
		cairo_set_antialias (ctx, CAIRO_ANTIALIAS_DEFAULT);
		size = gdk_pixbuf_get_width (self->priv->cover) / ((gint) string_get_length (self->priv->_text));
		fprintf (stdout, "%i-%i-%i\n", size, ww, (gint) string_get_length (self->priv->_text));
		pango_font_description_set_absolute_size (self->priv->fd, (double) (size * 1024));
		pango_layout_set_font_description (layout, self->priv->fd);
		pango_layout_set_text (layout, self->priv->_text, -1);
		pango_layout_get_pixel_size (layout, &tw, &th);
		cairo_move_to (ctx, x + ((ww - tw) / 2.0), y + ((wh - th) / 2.0));
		pango_cairo_layout_path (ctx, layout);
		cairo_set_source_rgba (ctx, (double) 0, (double) 0, (double) 0, (double) 1);
		cairo_stroke_preserve (ctx);
		cairo_set_source_rgba (ctx, (double) 1, (double) 1, (double) 1, (double) 1);
		cairo_fill (ctx);
		(layout == NULL) ? NULL : (layout = (g_object_unref (layout), NULL));
	}
	return (_tmp5 = FALSE, (ctx == NULL) ? NULL : (ctx = (cairo_destroy (ctx), NULL)), _tmp5);
}


static gboolean gmpc_image_timeout_test (GmpcImage* self) {
	g_return_val_if_fail (self != NULL, FALSE);
	self->priv->fade = self->priv->fade - 0.10;
	if (self->priv->fade <= 0.0) {
		GdkPixbuf* _tmp1;
		GdkPixbuf* _tmp0;
		GdkPixbuf* _tmp2;
		self->priv->fade = (double) 0;
		_tmp1 = NULL;
		_tmp0 = NULL;
		self->priv->cover = (_tmp1 = (_tmp0 = self->priv->temp, (_tmp0 == NULL) ? NULL : g_object_ref (_tmp0)), (self->priv->cover == NULL) ? NULL : (self->priv->cover = (g_object_unref (self->priv->cover), NULL)), _tmp1);
		self->priv->cover_border = self->priv->temp_border;
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
	gboolean _tmp0;
	GdkPixbuf* _tmp6;
	GdkPixbuf* _tmp8;
	GdkPixbuf* _tmp7;
	g_return_if_fail (self != NULL);
	g_return_if_fail (buf != NULL);
	_tmp0 = FALSE;
	if (self->priv->temp == NULL) {
		_tmp0 = self->priv->cover == NULL;
	} else {
		_tmp0 = FALSE;
	}
	if (_tmp0) {
		GdkPixbuf* _tmp1;
		GdkPixbuf* _tmp3;
		GdkPixbuf* _tmp2;
		self->priv->cover_border = border;
		_tmp1 = NULL;
		self->priv->cover = (_tmp1 = NULL, (self->priv->cover == NULL) ? NULL : (self->priv->cover = (g_object_unref (self->priv->cover), NULL)), _tmp1);
		_tmp3 = NULL;
		_tmp2 = NULL;
		self->priv->cover = (_tmp3 = (_tmp2 = buf, (_tmp2 == NULL) ? NULL : g_object_ref (_tmp2)), (self->priv->cover == NULL) ? NULL : (self->priv->cover = (g_object_unref (self->priv->cover), NULL)), _tmp3);
		gtk_widget_queue_draw ((GtkWidget*) self);
		return;
	}
	self->priv->fade = 1.0 - self->priv->fade;
	if (self->priv->temp != NULL) {
		GdkPixbuf* _tmp5;
		GdkPixbuf* _tmp4;
		_tmp5 = NULL;
		_tmp4 = NULL;
		self->priv->cover = (_tmp5 = (_tmp4 = self->priv->temp, (_tmp4 == NULL) ? NULL : g_object_ref (_tmp4)), (self->priv->cover == NULL) ? NULL : (self->priv->cover = (g_object_unref (self->priv->cover), NULL)), _tmp5);
	}
	_tmp6 = NULL;
	self->priv->temp = (_tmp6 = NULL, (self->priv->temp == NULL) ? NULL : (self->priv->temp = (g_object_unref (self->priv->temp), NULL)), _tmp6);
	_tmp8 = NULL;
	_tmp7 = NULL;
	self->priv->temp = (_tmp8 = (_tmp7 = buf, (_tmp7 == NULL) ? NULL : g_object_ref (_tmp7)), (self->priv->temp == NULL) ? NULL : (self->priv->temp = (g_object_unref (self->priv->temp), NULL)), _tmp8);
	self->priv->temp_border = border;
	gtk_widget_queue_draw ((GtkWidget*) self);
	if (self->priv->fade_timeout > 0) {
		g_source_remove (self->priv->fade_timeout);
	}
	self->priv->fade_timeout = g_timeout_add ((guint) 50, _gmpc_image_timeout_test_gsource_func, self);
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


GmpcImage* gmpc_image_construct (GType object_type) {
	GmpcImage * self;
	self = g_object_newv (object_type, 0, NULL);
	return self;
}


GmpcImage* gmpc_image_new (void) {
	return gmpc_image_construct (GMPC_TYPE_IMAGE);
}


const char* gmpc_image_get_text (GmpcImage* self) {
	g_return_val_if_fail (self != NULL, NULL);
	return self->priv->_text;
}


void gmpc_image_set_text (GmpcImage* self, const char* value) {
	char* _tmp2;
	const char* _tmp1;
	g_return_if_fail (self != NULL);
	_tmp2 = NULL;
	_tmp1 = NULL;
	self->priv->_text = (_tmp2 = (_tmp1 = value, (_tmp1 == NULL) ? NULL : g_strdup (_tmp1)), self->priv->_text = (g_free (self->priv->_text), NULL), _tmp2);
	g_object_notify ((GObject *) self, "text");
}


static gboolean _gmpc_image_on_expose_gtk_widget_expose_event (GmpcImage* _sender, const GdkEventExpose* event, gpointer self) {
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
		PangoFontDescription* _tmp0;
		g_object_set ((GtkWidget*) self, "app-paintable", TRUE, NULL);
		gtk_event_box_set_visible_window ((GtkEventBox*) self, FALSE);
		g_signal_connect_object ((GtkWidget*) self, "expose-event", (GCallback) _gmpc_image_on_expose_gtk_widget_expose_event, self, 0);
		_tmp0 = NULL;
		self->priv->fd = (_tmp0 = pango_font_description_new (), (self->priv->fd == NULL) ? NULL : (self->priv->fd = (pango_font_description_free (self->priv->fd), NULL)), _tmp0);
		/*from_string("sans mono"); */
		pango_font_description_set_family (self->priv->fd, "sans mono");
	}
	return obj;
}


static void gmpc_image_get_property (GObject * object, guint property_id, GValue * value, GParamSpec * pspec) {
	GmpcImage * self;
	gpointer boxed;
	self = GMPC_IMAGE (object);
	switch (property_id) {
		case GMPC_IMAGE_TEXT:
		g_value_set_string (value, gmpc_image_get_text (self));
		break;
		default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
		break;
	}
}


static void gmpc_image_set_property (GObject * object, guint property_id, const GValue * value, GParamSpec * pspec) {
	GmpcImage * self;
	self = GMPC_IMAGE (object);
	switch (property_id) {
		case GMPC_IMAGE_TEXT:
		gmpc_image_set_text (self, g_value_get_string (value));
		break;
		default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
		break;
	}
}


static void gmpc_image_class_init (GmpcImageClass * klass) {
	gmpc_image_parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (GmpcImagePrivate));
	G_OBJECT_CLASS (klass)->get_property = gmpc_image_get_property;
	G_OBJECT_CLASS (klass)->set_property = gmpc_image_set_property;
	G_OBJECT_CLASS (klass)->constructor = gmpc_image_constructor;
	G_OBJECT_CLASS (klass)->finalize = gmpc_image_finalize;
	g_object_class_install_property (G_OBJECT_CLASS (klass), GMPC_IMAGE_TEXT, g_param_spec_string ("text", "text", "text", NULL, G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB | G_PARAM_READABLE | G_PARAM_WRITABLE));
}


static void gmpc_image_instance_init (GmpcImage * self) {
	self->priv = GMPC_IMAGE_GET_PRIVATE (self);
	self->priv->cover = NULL;
	self->priv->cover_border = TRUE;
	self->priv->temp = NULL;
	self->priv->temp_border = TRUE;
	self->priv->fade = 0.0;
	self->priv->fade_timeout = (guint) 0;
	self->priv->fd = NULL;
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
	self->priv->_text = (g_free (self->priv->_text), NULL);
	(self->priv->fd == NULL) ? NULL : (self->priv->fd = (pango_font_description_free (self->priv->fd), NULL));
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




