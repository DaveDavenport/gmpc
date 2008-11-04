
#include "gmpc-progress.h"
#include <pango/pango.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cairo.h>
#include <float.h>
#include <math.h>
#include <gdk/gdk.h>
#include <pango/pangocairo.h>




struct _GmpcProgressPrivate {
	guint total;
	guint current;
	PangoLayout* _layout;
};

#define GMPC_PROGRESS_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GMPC_TYPE_PROGRESS, GmpcProgressPrivate))
enum  {
	GMPC_PROGRESS_DUMMY_PROPERTY,
	GMPC_PROGRESS_HIDE_TEXT
};
static void gmpc_progress_real_size_request (GtkWidget* base, GtkRequisition* requisition);
static void gmpc_progress_draw_curved_rectangle (GmpcProgress* self, cairo_t* ctx, double rect_x0, double rect_y0, double rect_width, double rect_height);
static gboolean gmpc_progress_on_expose (GmpcProgress* self, GmpcProgress* pb, GdkEventExpose* event);
static gboolean _gmpc_progress_on_expose_gtk_widget_expose_event (GmpcProgress* _sender, GdkEventExpose* event, gpointer self);
static GObject * gmpc_progress_constructor (GType type, guint n_construct_properties, GObjectConstructParam * construct_properties);
static gpointer gmpc_progress_parent_class = NULL;
static void gmpc_progress_finalize (GObject * obj);



/* The size_request method Gtk+ is calling on a widget to ask
 it the widget how large it wishes to be. It's not guaranteed
 that gtk+ will actually give this size to the widget*/
static void gmpc_progress_real_size_request (GtkWidget* base, GtkRequisition* requisition) {
	GmpcProgress * self;
	gint width;
	gint height;
	self = GMPC_PROGRESS (base);
	width = 0;
	height = 0;
	fprintf (stdout, "size request\n");
	/* In this case, we say that we want to be as big as the
	 text is, plus a little border around it.*/
	if (gmpc_progress_get_hide_text (self)) {
		(*requisition).width = 40;
		(*requisition).height = 8;
	} else {
		pango_layout_get_size (self->priv->_layout, &width, &height);
		(*requisition).width = width / PANGO_SCALE + 6;
		(*requisition).height = height / PANGO_SCALE + 6;
	}
}


static void gmpc_progress_draw_curved_rectangle (GmpcProgress* self, cairo_t* ctx, double rect_x0, double rect_y0, double rect_width, double rect_height) {
	double rect_x1;
	double rect_y1;
	double radius;
	g_return_if_fail (GMPC_IS_PROGRESS (self));
	g_return_if_fail (ctx != NULL);
	rect_x1 = 0.0;
	rect_y1 = 0.0;
	radius = ((double) (15));
	/*rect_width/5;*/
	rect_x1 = rect_x0 + rect_width;
	rect_y1 = rect_y0 + rect_height;
	if (rect_width == 0 || rect_height == 0) {
		return;
	}
	if (rect_width / 2 < radius) {
		if (rect_height / 2 < radius) {
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
		if (rect_height / 2 < radius) {
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


static gboolean gmpc_progress_on_expose (GmpcProgress* self, GmpcProgress* pb, GdkEventExpose* event) {
	cairo_t* ctx;
	gint width;
	gint height;
	GdkColor _tmp0 = {0};
	GdkColor _tmp1 = {0};
	gboolean _tmp11;
	g_return_val_if_fail (GMPC_IS_PROGRESS (self), FALSE);
	g_return_val_if_fail (GMPC_IS_PROGRESS (pb), FALSE);
	ctx = gdk_cairo_create (GDK_DRAWABLE (GTK_WIDGET (pb)->window));
	width = GTK_WIDGET (pb)->allocation.width - 3;
	height = GTK_WIDGET (pb)->allocation.height - 3;
	/* Draw border */
	cairo_set_line_width (ctx, 1.0);
	cairo_set_tolerance (ctx, 0.2);
	cairo_set_line_join (ctx, CAIRO_LINE_JOIN_ROUND);
	/*paint background*/
	gdk_cairo_set_source_color (ctx, (_tmp0 = gtk_widget_get_style (GTK_WIDGET (pb))->bg[((gint) (GTK_STATE_NORMAL))], &_tmp0));
	cairo_paint (ctx);
	cairo_new_path (ctx);
	gdk_cairo_set_source_color (ctx, (_tmp1 = gtk_widget_get_style (GTK_WIDGET (pb))->dark[((gint) (GTK_STATE_NORMAL))], &_tmp1));
	/*        ctx.rectangle(1.5,1.5,width, height);*/
	gmpc_progress_draw_curved_rectangle (self, ctx, 1.5, 1.5, ((double) (width)), ((double) (height)));
	cairo_stroke_preserve (ctx);
	cairo_clip (ctx);
	if (self->priv->total > 0) {
		double step_size;
		gint pwidth;
		GdkColor _tmp2 = {0};
		GdkColor _tmp3 = {0};
		step_size = width / ((double) (self->priv->total));
		pwidth = ((gint) ((step_size * self->priv->current)));
		/* don't allow more then 100% */
		if (pwidth > width) {
			pwidth = width;
		}
		cairo_new_path (ctx);
		gdk_cairo_set_source_color (ctx, (_tmp2 = gtk_widget_get_style (GTK_WIDGET (pb))->bg[((gint) (GTK_STATE_SELECTED))], &_tmp2));
		cairo_rectangle (ctx, 1.5, 1.5, ((double) (pwidth)), ((double) (height)));
		/*draw_curved_rectangle(ctx, 1.5,1.5,pwidth, height);*/
		cairo_fill_preserve (ctx);
		gdk_cairo_set_source_color (ctx, (_tmp3 = gtk_widget_get_style (GTK_WIDGET (pb))->dark[((gint) (GTK_STATE_NORMAL))], &_tmp3));
		cairo_stroke (ctx);
	}
	cairo_reset_clip (ctx);
	/**
	         * Draw text
	         */
	if (gmpc_progress_get_hide_text (self) == FALSE) {
		gint fontw;
		gint fonth;
		fontw = 0;
		fonth = 0;
		if (self->priv->total == 0) {
			char* a;
			a = NULL;
			if (self->priv->current / 60 > 99) {
				char* _tmp4;
				_tmp4 = NULL;
				a = (_tmp4 = g_strdup_printf ("%02i:%02i", ((gint) (self->priv->current)) / 3600, ((gint) ((self->priv->current))) % 60), (a = (g_free (a), NULL)), _tmp4);
			} else {
				char* _tmp5;
				_tmp5 = NULL;
				a = (_tmp5 = g_strdup_printf ("%02i:%02i", ((gint) (self->priv->current)) / 60, ((gint) ((self->priv->current))) % 60), (a = (g_free (a), NULL)), _tmp5);
			}
			pango_layout_set_text (self->priv->_layout, a, -1);
			a = (g_free (a), NULL);
		} else {
			char* a;
			a = NULL;
			if (self->priv->current / 60 > 99) {
				char* _tmp6;
				_tmp6 = NULL;
				a = (_tmp6 = g_strdup_printf ("%02u:%02u - %02u:%02u", self->priv->current / 3600, (self->priv->current) % 60, self->priv->total / 3600, (self->priv->total) % 60), (a = (g_free (a), NULL)), _tmp6);
			} else {
				char* _tmp7;
				_tmp7 = NULL;
				a = (_tmp7 = g_strdup_printf ("%02u:%02u - %02u:%02u", self->priv->current / 60, (self->priv->current) % 60, self->priv->total / 60, (self->priv->total) % 60), (a = (g_free (a), NULL)), _tmp7);
			}
			pango_layout_set_text (self->priv->_layout, a, -1);
			a = (g_free (a), NULL);
		}
		pango_cairo_update_layout (ctx, self->priv->_layout);
		pango_layout_get_pixel_size (self->priv->_layout, &fontw, &fonth);
		if (self->priv->total > 0) {
			double step_size;
			gint pwidth;
			step_size = width / ((double) (self->priv->total));
			pwidth = ((gint) ((step_size * self->priv->current)));
			if (pwidth >= ((width - fontw) / 2 + 1)) {
				GdkColor _tmp8 = {0};
				cairo_new_path (ctx);
				gdk_cairo_set_source_color (ctx, (_tmp8 = gtk_widget_get_style (GTK_WIDGET (pb))->fg[((gint) (GTK_STATE_SELECTED))], &_tmp8));
				cairo_rectangle (ctx, ((double) (1)), ((double) (1)), ((double) (pwidth)), ((double) (height)));
				cairo_clip (ctx);
				cairo_move_to (ctx, (width - fontw) / 2 + 1.5, (height - fonth) / 2 + 1.5);
				pango_cairo_show_layout (ctx, self->priv->_layout);
			}
			if (pwidth < ((width - fontw) / 2 + 1 + fontw)) {
				GdkColor _tmp9 = {0};
				cairo_new_path (ctx);
				gdk_cairo_set_source_color (ctx, (_tmp9 = gtk_widget_get_style (GTK_WIDGET (pb))->fg[((gint) (GTK_STATE_NORMAL))], &_tmp9));
				cairo_reset_clip (ctx);
				cairo_rectangle (ctx, ((double) (pwidth + 1)), ((double) (1)), ((double) (width)), ((double) (height)));
				cairo_clip (ctx);
				cairo_move_to (ctx, (width - fontw) / 2 + 1.5, (height - fonth) / 2 + 1.5);
				pango_cairo_show_layout (ctx, self->priv->_layout);
			}
		} else {
			GdkColor _tmp10 = {0};
			cairo_new_path (ctx);
			gdk_cairo_set_source_color (ctx, (_tmp10 = gtk_widget_get_style (GTK_WIDGET (pb))->fg[((gint) (GTK_STATE_NORMAL))], &_tmp10));
			cairo_move_to (ctx, (width - fontw) / 2 + 1.5, (height - fonth) / 2 + 1.5);
			pango_cairo_show_layout (ctx, self->priv->_layout);
		}
	}
	return (_tmp11 = TRUE, (ctx == NULL ? NULL : (ctx = (cairo_destroy (ctx), NULL))), _tmp11);
}


void gmpc_progress_set_time (GmpcProgress* self, guint total, guint current) {
	g_return_if_fail (GMPC_IS_PROGRESS (self));
	self->priv->total = total;
	self->priv->current = current;
	gtk_widget_queue_draw (GTK_WIDGET (self));
}


GmpcProgress* gmpc_progress_new (void) {
	GmpcProgress * self;
	self = g_object_newv (GMPC_TYPE_PROGRESS, 0, NULL);
	return self;
}


gboolean gmpc_progress_get_hide_text (GmpcProgress* self) {
	g_return_val_if_fail (GMPC_IS_PROGRESS (self), FALSE);
	return self->_hide_text;
}


void gmpc_progress_set_hide_text (GmpcProgress* self, gboolean value) {
	g_return_if_fail (GMPC_IS_PROGRESS (self));
	self->_hide_text = value;
	gtk_widget_queue_resize (GTK_WIDGET (self));
	g_object_notify (((GObject *) (self)), "hide-text");
}


static gboolean _gmpc_progress_on_expose_gtk_widget_expose_event (GmpcProgress* _sender, GdkEventExpose* event, gpointer self) {
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
		PangoLayout* _tmp1;
		PangoLayout* _tmp0;
		g_signal_connect_object (GTK_WIDGET (self), "expose-event", ((GCallback) (_gmpc_progress_on_expose_gtk_widget_expose_event)), self, 0);
		_tmp1 = NULL;
		_tmp0 = NULL;
		self->priv->_layout = (_tmp1 = (_tmp0 = gtk_widget_create_pango_layout (GTK_WIDGET (self), " "), (_tmp0 == NULL ? NULL : g_object_ref (_tmp0))), (self->priv->_layout == NULL ? NULL : (self->priv->_layout = (g_object_unref (self->priv->_layout), NULL))), _tmp1);
	}
	return obj;
}


static void gmpc_progress_get_property (GObject * object, guint property_id, GValue * value, GParamSpec * pspec) {
	GmpcProgress * self;
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
	GTK_WIDGET_CLASS (klass)->size_request = gmpc_progress_real_size_request;
	g_object_class_install_property (G_OBJECT_CLASS (klass), GMPC_PROGRESS_HIDE_TEXT, g_param_spec_boolean ("hide-text", "hide-text", "hide-text", FALSE, G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB | G_PARAM_READABLE | G_PARAM_WRITABLE));
}


static void gmpc_progress_instance_init (GmpcProgress * self) {
	self->priv = GMPC_PROGRESS_GET_PRIVATE (self);
	self->priv->total = ((guint) (0));
	self->priv->current = ((guint) (0));
	self->_hide_text = FALSE;
}


static void gmpc_progress_finalize (GObject * obj) {
	GmpcProgress * self;
	self = GMPC_PROGRESS (obj);
	{
	}
	(self->priv->_layout == NULL ? NULL : (self->priv->_layout = (g_object_unref (self->priv->_layout), NULL)));
	G_OBJECT_CLASS (gmpc_progress_parent_class)->finalize (obj);
}


GType gmpc_progress_get_type (void) {
	static GType gmpc_progress_type_id = 0;
	if (gmpc_progress_type_id == 0) {
		static const GTypeInfo g_define_type_info = { sizeof (GmpcProgressClass), (GBaseInitFunc) NULL, (GBaseFinalizeFunc) NULL, (GClassInitFunc) gmpc_progress_class_init, (GClassFinalizeFunc) NULL, NULL, sizeof (GmpcProgress), 0, (GInstanceInitFunc) gmpc_progress_instance_init };
		gmpc_progress_type_id = g_type_register_static (GTK_TYPE_EVENT_BOX, "GmpcProgress", &g_define_type_info, 0);
	}
	return gmpc_progress_type_id;
}




