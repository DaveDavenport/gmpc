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
#include <gdk-pixbuf/gdk-pixdata.h>
#include <float.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <pango/pango.h>
#include <gdk/gdk.h>
#include <cairo.h>
#include <pango/pangocairo.h>


#define GMPC_TYPE_IMAGE (gmpc_image_get_type ())
#define GMPC_IMAGE(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GMPC_TYPE_IMAGE, GmpcImage))
#define GMPC_IMAGE_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GMPC_TYPE_IMAGE, GmpcImageClass))
#define GMPC_IS_IMAGE(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GMPC_TYPE_IMAGE))
#define GMPC_IS_IMAGE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GMPC_TYPE_IMAGE))
#define GMPC_IMAGE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GMPC_TYPE_IMAGE, GmpcImageClass))

typedef struct _GmpcImage GmpcImage;
typedef struct _GmpcImageClass GmpcImageClass;
typedef struct _GmpcImagePrivate GmpcImagePrivate;
#define _g_object_unref0(var) ((var == NULL) ? NULL : (var = (g_object_unref (var), NULL)))
#define _g_free0(var) (var = (g_free (var), NULL))
#define _pango_font_description_free0(var) ((var == NULL) ? NULL : (var = (pango_font_description_free (var), NULL)))
#define _cairo_destroy0(var) ((var == NULL) ? NULL : (var = (cairo_destroy (var), NULL)))

struct _GmpcImage {
	GtkEventBox parent_instance;
	GmpcImagePrivate * priv;
};

struct _GmpcImageClass {
	GtkEventBoxClass parent_class;
};

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


static gpointer gmpc_image_parent_class = NULL;

#define use_transition TRUE
GType gmpc_image_get_type (void);
#define GMPC_IMAGE_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GMPC_TYPE_IMAGE, GmpcImagePrivate))
enum  {
	GMPC_IMAGE_DUMMY_PROPERTY,
	GMPC_IMAGE_TEXT
};
const char* gmpc_image_get_text (GmpcImage* self);
static gboolean gmpc_image_on_expose (GmpcImage* self, GmpcImage* img, const GdkEventExpose* event);
static gboolean gmpc_image_timeout_test (GmpcImage* self);
static gboolean _gmpc_image_timeout_test_gsource_func (gpointer self);
void gmpc_image_set_pixbuf (GmpcImage* self, GdkPixbuf* buf, gboolean border);
void gmpc_image_clear_pixbuf (GmpcImage* self);
GmpcImage* gmpc_image_new (void);
GmpcImage* gmpc_image_construct (GType object_type);
void gmpc_image_set_text (GmpcImage* self, const char* value);
static gboolean _gmpc_image_on_expose_gtk_widget_expose_event (GmpcImage* _sender, const GdkEventExpose* event, gpointer self);
static GObject * gmpc_image_constructor (GType type, guint n_construct_properties, GObjectConstructParam * construct_properties);
static void gmpc_image_finalize (GObject* obj);
static void gmpc_image_get_property (GObject * object, guint property_id, GValue * value, GParamSpec * pspec);
static void gmpc_image_set_property (GObject * object, guint property_id, const GValue * value, GParamSpec * pspec);



static glong string_get_length (const char* self) {
	glong result;
	g_return_val_if_fail (self != NULL, 0L);
	result = g_utf8_strlen (self, -1);
	return result;
}


static gboolean gmpc_image_on_expose (GmpcImage* self, GmpcImage* img, const GdkEventExpose* event) {
	gboolean result;
	cairo_t* ctx;
	gint width;
	gint height;
	gint x;
	gint y;
	gint ww;
	gint wh;
	gboolean _tmp2_;
	gboolean _tmp3_;
	g_return_val_if_fail (self != NULL, FALSE);
	g_return_val_if_fail (img != NULL, FALSE);
	ctx = gdk_cairo_create ((GdkDrawable*) ((GtkWidget*) img)->window);
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
		double _tmp0_;
		double fade2;
		width = gdk_pixbuf_get_width (self->priv->cover);
		height = gdk_pixbuf_get_height (self->priv->cover);
		x_start = x + ceil ((ww - width) / 2.0);
		y_start = y + ceil ((wh - height) / 2.0);
		cairo_set_line_join (ctx, CAIRO_LINE_JOIN_ROUND);
		cairo_new_path (ctx);
		cairo_rectangle (ctx, x_start, y_start, (double) (width - 1), (double) (height - 1));
		_tmp0_ = 0.0;
		if (self->priv->fade <= 0) {
			_tmp0_ = (double) 1;
		} else {
			_tmp0_ = self->priv->fade;
		}
		fade2 = _tmp0_;
		gdk_cairo_set_source_pixbuf (ctx, self->priv->cover, x_start, y_start);
		cairo_paint_with_alpha (ctx, fade2);
		if (self->priv->cover_border) {
			cairo_set_source_rgba (ctx, (double) 0, (double) 0, (double) 0, fade2);
			cairo_stroke (ctx);
		} else {
			cairo_new_path (ctx);
		}
	}
	if (self->priv->temp != NULL) {
		double x_start;
		double y_start;
		double _tmp1_;
		double fade2;
		cairo_new_path (ctx);
		width = gdk_pixbuf_get_width (self->priv->temp);
		height = gdk_pixbuf_get_height (self->priv->temp);
		x_start = x + ceil ((ww - width) / 2.0);
		y_start = y + ceil ((wh - height) / 2.0);
		cairo_set_line_join (ctx, CAIRO_LINE_JOIN_ROUND);
		cairo_rectangle (ctx, x_start, y_start, (double) (width - 1), (double) (height - 1));
		gdk_cairo_set_source_pixbuf (ctx, self->priv->temp, x_start, y_start);
		_tmp1_ = 0.0;
		if (self->priv->fade <= 0) {
			_tmp1_ = (double) 1;
		} else {
			_tmp1_ = self->priv->fade;
		}
		fade2 = _tmp1_;
		cairo_paint_with_alpha (ctx, 1 - fade2);
		if (self->priv->temp_border) {
			cairo_set_source_rgba (ctx, (double) 0, (double) 0, (double) 0, 1 - fade2);
			cairo_stroke (ctx);
		} else {
			cairo_new_path (ctx);
		}
	}
	_tmp2_ = FALSE;
	_tmp3_ = FALSE;
	if (self->priv->cover != NULL) {
		_tmp3_ = self->priv->_text != NULL;
	} else {
		_tmp3_ = FALSE;
	}
	if (_tmp3_) {
		_tmp2_ = string_get_length (self->priv->_text) > 0;
	} else {
		_tmp2_ = FALSE;
	}
	if (_tmp2_) {
		PangoLayout* layout;
		gint tw;
		gint th;
		gint size;
		layout = pango_cairo_create_layout (ctx);
		tw = 0;
		th = 0;
		cairo_set_antialias (ctx, CAIRO_ANTIALIAS_DEFAULT);
		size = gdk_pixbuf_get_width (self->priv->cover) / ((gint) string_get_length (self->priv->_text));
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
		_g_object_unref0 (layout);
	}
	result = FALSE;
	_cairo_destroy0 (ctx);
	return result;
}


static gpointer _g_object_ref0 (gpointer self) {
	return self ? g_object_ref (self) : NULL;
}


static gboolean gmpc_image_timeout_test (GmpcImage* self) {
	gboolean result;
	g_return_val_if_fail (self != NULL, FALSE);
	self->priv->fade = self->priv->fade - 0.10;
	if (self->priv->fade <= 0.0) {
		GdkPixbuf* _tmp0_;
		GdkPixbuf* _tmp1_;
		self->priv->fade = (double) 0;
		self->priv->cover = (_tmp0_ = _g_object_ref0 (self->priv->temp), _g_object_unref0 (self->priv->cover), _tmp0_);
		self->priv->cover_border = self->priv->temp_border;
		self->priv->temp = (_tmp1_ = NULL, _g_object_unref0 (self->priv->temp), _tmp1_);
		gtk_widget_queue_draw ((GtkWidget*) self);
		self->priv->fade_timeout = (guint) 0;
		result = FALSE;
		return result;
	}
	gtk_widget_queue_draw ((GtkWidget*) self);
	result = TRUE;
	return result;
}


static gboolean _gmpc_image_timeout_test_gsource_func (gpointer self) {
	return gmpc_image_timeout_test (self);
}


void gmpc_image_set_pixbuf (GmpcImage* self, GdkPixbuf* buf, gboolean border) {
	gboolean _tmp0_;
	GdkPixbuf* _tmp4_;
	GdkPixbuf* _tmp5_;
	g_return_if_fail (self != NULL);
	g_return_if_fail (buf != NULL);
	_tmp0_ = FALSE;
	if (self->priv->temp == NULL) {
		_tmp0_ = self->priv->cover == NULL;
	} else {
		_tmp0_ = FALSE;
	}
	if (_tmp0_) {
		GdkPixbuf* _tmp1_;
		GdkPixbuf* _tmp2_;
		self->priv->cover_border = border;
		self->priv->cover = (_tmp1_ = NULL, _g_object_unref0 (self->priv->cover), _tmp1_);
		self->priv->cover = (_tmp2_ = _g_object_ref0 (buf), _g_object_unref0 (self->priv->cover), _tmp2_);
		gtk_widget_queue_draw ((GtkWidget*) self);
		return;
	}
	self->priv->fade = 1.0 - self->priv->fade;
	if (self->priv->temp != NULL) {
		GdkPixbuf* _tmp3_;
		self->priv->cover = (_tmp3_ = _g_object_ref0 (self->priv->temp), _g_object_unref0 (self->priv->cover), _tmp3_);
	}
	self->priv->temp = (_tmp4_ = NULL, _g_object_unref0 (self->priv->temp), _tmp4_);
	self->priv->temp = (_tmp5_ = _g_object_ref0 (buf), _g_object_unref0 (self->priv->temp), _tmp5_);
	self->priv->temp_border = border;
	gtk_widget_queue_draw ((GtkWidget*) self);
	if (self->priv->fade_timeout > 0) {
		g_source_remove (self->priv->fade_timeout);
	}
	self->priv->fade_timeout = g_timeout_add_full (G_PRIORITY_DEFAULT, (guint) 50, _gmpc_image_timeout_test_gsource_func, g_object_ref (self), g_object_unref);
}


void gmpc_image_clear_pixbuf (GmpcImage* self) {
	GdkPixbuf* _tmp0_;
	GdkPixbuf* _tmp1_;
	g_return_if_fail (self != NULL);
	self->priv->fade = 0.0;
	self->priv->temp = (_tmp0_ = NULL, _g_object_unref0 (self->priv->temp), _tmp0_);
	if (self->priv->fade_timeout > 0) {
		g_source_remove (self->priv->fade_timeout);
		self->priv->fade_timeout = (guint) 0;
	}
	self->priv->cover = (_tmp1_ = NULL, _g_object_unref0 (self->priv->cover), _tmp1_);
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
	const char* result;
	g_return_val_if_fail (self != NULL, NULL);
	result = self->priv->_text;
	return result;
}


void gmpc_image_set_text (GmpcImage* self, const char* value) {
	char* _tmp0_;
	g_return_if_fail (self != NULL);
	self->priv->_text = (_tmp0_ = g_strdup (value), _g_free0 (self->priv->_text), _tmp0_);
	g_object_notify ((GObject *) self, "text");
}


static gboolean _gmpc_image_on_expose_gtk_widget_expose_event (GmpcImage* _sender, const GdkEventExpose* event, gpointer self) {
	return gmpc_image_on_expose (self, _sender, event);
}


static GObject * gmpc_image_constructor (GType type, guint n_construct_properties, GObjectConstructParam * construct_properties) {
	GObject * obj;
	GObjectClass * parent_class;
	GmpcImage * self;
	parent_class = G_OBJECT_CLASS (gmpc_image_parent_class);
	obj = parent_class->constructor (type, n_construct_properties, construct_properties);
	self = GMPC_IMAGE (obj);
	{
		PangoFontDescription* _tmp0_;
		g_object_set ((GtkWidget*) self, "app-paintable", TRUE, NULL);
		gtk_event_box_set_visible_window ((GtkEventBox*) self, FALSE);
		g_signal_connect_object ((GtkWidget*) self, "expose-event", (GCallback) _gmpc_image_on_expose_gtk_widget_expose_event, self, 0);
		self->priv->fd = (_tmp0_ = pango_font_description_new (), _pango_font_description_free0 (self->priv->fd), _tmp0_);
		pango_font_description_set_family (self->priv->fd, "sans mono");
	}
	return obj;
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
	_g_object_unref0 (self->priv->cover);
	_g_object_unref0 (self->priv->temp);
	_g_free0 (self->priv->_text);
	_pango_font_description_free0 (self->priv->fd);
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


static void gmpc_image_get_property (GObject * object, guint property_id, GValue * value, GParamSpec * pspec) {
	GmpcImage * self;
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




