
#include "gmpc-progress2.h"
#include <float.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>




struct _GmpcProgressPrivate {
	guint total;
	guint current;
	gboolean _do_countdown;
	GtkProgressBar* bar;
};

#define GMPC_PROGRESS_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GMPC_TYPE_PROGRESS, GmpcProgressPrivate))
enum  {
	GMPC_PROGRESS_DUMMY_PROPERTY,
	GMPC_PROGRESS_HIDE_TEXT,
	GMPC_PROGRESS_DO_COUNTDOWN
};
static GObject * gmpc_progress_constructor (GType type, guint n_construct_properties, GObjectConstructParam * construct_properties);
static gpointer gmpc_progress_parent_class = NULL;
static void gmpc_progress_finalize (GObject * obj);



/* The size_request method Gtk+ is calling on a widget to ask
 it the widget how large it wishes to be. It's not guaranteed
 that gtk+ will actually give this size to the widget

    public override void size_request (Gtk.Requisition requisition)
    {
        int width, height;
        // In this case, we say that we want to be as big as the
        // text is, plus a little border around it.
        if(this.hide_text)
        {
            requisition.width = 40;
            requisition.height = 10;
        }else{
            requisition.width = -1; 
            requisition.height = -1;
        }
    }
*/
void gmpc_progress_set_time (GmpcProgress* self, guint total, guint current) {
	g_return_if_fail (GMPC_IS_PROGRESS (self));
	if (self->priv->total != total || self->priv->current != current) {
		self->priv->total = total;
		self->priv->current = current;
		if (self->priv->total > 0) {
			gtk_progress_bar_set_fraction (self->priv->bar, ((double) (self->priv->current)) / ((double) (self->priv->total)));
		} else {
			gtk_progress_bar_set_fraction (self->priv->bar, 0.0);
		}
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
					char* _tmp0;
					_tmp0 = NULL;
					a = (_tmp0 = g_strdup_printf ("%02i:%02i", ((gint) (self->priv->current)) / 3600, ((gint) ((self->priv->current))) % 60), (a = (g_free (a), NULL)), _tmp0);
				} else {
					char* _tmp1;
					_tmp1 = NULL;
					a = (_tmp1 = g_strdup_printf ("%02i:%02i", ((gint) (self->priv->current)) / 60, ((gint) ((self->priv->current))) % 60), (a = (g_free (a), NULL)), _tmp1);
				}
				gtk_progress_bar_set_text (self->priv->bar, a);
				a = (g_free (a), NULL);
			} else {
				char* a;
				guint p;
				a = NULL;
				p = self->priv->current;
				if (gmpc_progress_get_do_countdown (self)) {
					p = self->priv->total - self->priv->current;
				}
				if (self->priv->current / 60 > 99) {
					char* _tmp2;
					_tmp2 = NULL;
					a = (_tmp2 = g_strdup_printf ("%c%02u:%02u - %02u:%02u", ((gint) (((gmpc_progress_get_do_countdown (self)) ? '-' : ' '))), p / 3600, (p) % 60, self->priv->total / 3600, (self->priv->total) % 60), (a = (g_free (a), NULL)), _tmp2);
				} else {
					char* _tmp3;
					_tmp3 = NULL;
					a = (_tmp3 = g_strdup_printf ("%c%02u:%02u - %02u:%02u", ((gint) (((gmpc_progress_get_do_countdown (self)) ? '-' : ' '))), p / 60, (p) % 60, self->priv->total / 60, (self->priv->total) % 60), (a = (g_free (a), NULL)), _tmp3);
				}
				gtk_progress_bar_set_text (self->priv->bar, a);
				a = (g_free (a), NULL);
			}
		} else {
			gtk_progress_bar_set_text (self->priv->bar, " ");
		}
	}
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
	if (value) {
		gtk_widget_set_size_request (GTK_WIDGET (self), -1, 10);
		gtk_progress_bar_set_text (self->priv->bar, "");
	} else {
		gtk_widget_set_size_request (GTK_WIDGET (self), -1, -1);
	}
	self->_hide_text = value;
	g_object_notify (((GObject *) (self)), "hide-text");
}


gboolean gmpc_progress_get_do_countdown (GmpcProgress* self) {
	g_return_val_if_fail (GMPC_IS_PROGRESS (self), FALSE);
	return self->priv->_do_countdown;
}


void gmpc_progress_set_do_countdown (GmpcProgress* self, gboolean value) {
	g_return_if_fail (GMPC_IS_PROGRESS (self));
	self->priv->_do_countdown = value;
	gtk_widget_queue_resize (GTK_WIDGET (self));
	g_object_notify (((GObject *) (self)), "do-countdown");
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
		gtk_container_add (GTK_CONTAINER (self), GTK_WIDGET (self->priv->bar));
		gtk_widget_show_all (GTK_WIDGET (self));
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
}


static void gmpc_progress_instance_init (GmpcProgress * self) {
	self->priv = GMPC_PROGRESS_GET_PRIVATE (self);
	self->priv->total = ((guint) (0));
	self->priv->current = ((guint) (0));
	self->priv->_do_countdown = FALSE;
	self->_hide_text = FALSE;
	self->priv->bar = g_object_ref_sink (((GtkProgressBar*) (gtk_progress_bar_new ())));
}


static void gmpc_progress_finalize (GObject * obj) {
	GmpcProgress * self;
	self = GMPC_PROGRESS (obj);
	(self->priv->bar == NULL ? NULL : (self->priv->bar = (g_object_unref (self->priv->bar), NULL)));
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




