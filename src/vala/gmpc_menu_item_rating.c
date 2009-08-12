
#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include <gtktransition.h>
#include <gmpc-rating.h>
#include <config.h>
#include <stdio.h>
#include <gdk/gdk.h>
#include <glib/gi18n-lib.h>
#include <libmpd/libmpd.h>
#include <libmpd/libmpdclient.h>


#define GMPC_MENU_ITEM_TYPE_RATING (gmpc_menu_item_rating_get_type ())
#define GMPC_MENU_ITEM_RATING(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GMPC_MENU_ITEM_TYPE_RATING, GmpcMenuItemRating))
#define GMPC_MENU_ITEM_RATING_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GMPC_MENU_ITEM_TYPE_RATING, GmpcMenuItemRatingClass))
#define GMPC_MENU_ITEM_IS_RATING(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GMPC_MENU_ITEM_TYPE_RATING))
#define GMPC_MENU_ITEM_IS_RATING_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GMPC_MENU_ITEM_TYPE_RATING))
#define GMPC_MENU_ITEM_RATING_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GMPC_MENU_ITEM_TYPE_RATING, GmpcMenuItemRatingClass))

typedef struct _GmpcMenuItemRating GmpcMenuItemRating;
typedef struct _GmpcMenuItemRatingClass GmpcMenuItemRatingClass;
typedef struct _GmpcMenuItemRatingPrivate GmpcMenuItemRatingPrivate;

struct _GmpcMenuItemRating {
	GtkMenuItem parent_instance;
	GmpcMenuItemRatingPrivate * priv;
	GtkVBox* hbox;
	GmpcRating* rating;
};

struct _GmpcMenuItemRatingClass {
	GtkMenuItemClass parent_class;
};


static gpointer gmpc_menu_item_rating_parent_class = NULL;

#define use_transition TRUE
GType gmpc_menu_item_rating_get_type (void);
enum  {
	GMPC_MENU_ITEM_RATING_DUMMY_PROPERTY
};
#define GMPC_MENU_ITEM_RATING_some_unique_name VERSION
gint gmpc_menu_item_rating_get_rating (GmpcMenuItemRating* self);
static gboolean gmpc_menu_item_rating_button_press_event_callback (GmpcMenuItemRating* self, const GdkEventButton* event, void* userdata);
static gboolean gmpc_menu_item_rating_button_release_event_callback (GmpcMenuItemRating* self, const GdkEventButton* event, void* userdata);
GmpcMenuItemRating* gmpc_menu_item_rating_new (MpdObj* server, const mpd_Song* song);
GmpcMenuItemRating* gmpc_menu_item_rating_construct (GType object_type, MpdObj* server, const mpd_Song* song);
static void gmpc_menu_item_rating_finalize (GObject* obj);



gint gmpc_menu_item_rating_get_rating (GmpcMenuItemRating* self) {
	gint result;
	g_return_val_if_fail (self != NULL, 0);
	result = 0;
	return result;
}


static gboolean gmpc_menu_item_rating_button_press_event_callback (GmpcMenuItemRating* self, const GdkEventButton* event, void* userdata) {
	gboolean result;
	g_return_val_if_fail (self != NULL, FALSE);
	fprintf (stdout, "Button press event\n");
	gmpc_rating_button_press_event_callback (self->rating, self->rating->event_box, &(*event));
	result = TRUE;
	return result;
}


static gboolean gmpc_menu_item_rating_button_release_event_callback (GmpcMenuItemRating* self, const GdkEventButton* event, void* userdata) {
	gboolean result;
	g_return_val_if_fail (self != NULL, FALSE);
	result = TRUE;
	return result;
}


GmpcMenuItemRating* gmpc_menu_item_rating_construct (GType object_type, MpdObj* server, const mpd_Song* song) {
	GmpcMenuItemRating * self;
	GtkVBox* _tmp0_;
	GmpcRating* _tmp1_;
	GtkLabel* _tmp2_;
	g_return_val_if_fail (server != NULL, NULL);
	g_return_val_if_fail (song != NULL, NULL);
	self = g_object_newv (object_type, 0, NULL);
	g_signal_connect_swapped (self, "button-press-event", (GCallback) gmpc_menu_item_rating_button_press_event_callback, self);
	g_signal_connect_swapped (self, "button-release-event", (GCallback) gmpc_menu_item_rating_button_release_event_callback, self);
	_tmp0_ = NULL;
	self->hbox = (_tmp0_ = g_object_ref_sink ((GtkVBox*) gtk_vbox_new (FALSE, 6)), (self->hbox == NULL) ? NULL : (self->hbox = (g_object_unref (self->hbox), NULL)), _tmp0_);
	_tmp1_ = NULL;
	self->rating = (_tmp1_ = g_object_ref_sink (gmpc_rating_new (server, song)), (self->rating == NULL) ? NULL : (self->rating = (g_object_unref (self->rating), NULL)), _tmp1_);
	_tmp2_ = NULL;
	gtk_box_pack_start ((GtkBox*) self->hbox, (GtkWidget*) (_tmp2_ = g_object_ref_sink ((GtkLabel*) gtk_label_new (_ ("Rating:")))), FALSE, TRUE, (guint) 0);
	(_tmp2_ == NULL) ? NULL : (_tmp2_ = (g_object_unref (_tmp2_), NULL));
	gtk_box_pack_start ((GtkBox*) self->hbox, (GtkWidget*) self->rating, FALSE, TRUE, (guint) 0);
	gtk_container_add ((GtkContainer*) self, (GtkWidget*) self->hbox);
	gtk_widget_show_all ((GtkWidget*) self);
	return self;
}


GmpcMenuItemRating* gmpc_menu_item_rating_new (MpdObj* server, const mpd_Song* song) {
	return gmpc_menu_item_rating_construct (GMPC_MENU_ITEM_TYPE_RATING, server, song);
}


static void gmpc_menu_item_rating_class_init (GmpcMenuItemRatingClass * klass) {
	gmpc_menu_item_rating_parent_class = g_type_class_peek_parent (klass);
	G_OBJECT_CLASS (klass)->finalize = gmpc_menu_item_rating_finalize;
}


static void gmpc_menu_item_rating_instance_init (GmpcMenuItemRating * self) {
	self->hbox = NULL;
	self->rating = NULL;
}


static void gmpc_menu_item_rating_finalize (GObject* obj) {
	GmpcMenuItemRating * self;
	self = GMPC_MENU_ITEM_RATING (obj);
	(self->hbox == NULL) ? NULL : (self->hbox = (g_object_unref (self->hbox), NULL));
	(self->rating == NULL) ? NULL : (self->rating = (g_object_unref (self->rating), NULL));
	G_OBJECT_CLASS (gmpc_menu_item_rating_parent_class)->finalize (obj);
}


GType gmpc_menu_item_rating_get_type (void) {
	static GType gmpc_menu_item_rating_type_id = 0;
	if (gmpc_menu_item_rating_type_id == 0) {
		static const GTypeInfo g_define_type_info = { sizeof (GmpcMenuItemRatingClass), (GBaseInitFunc) NULL, (GBaseFinalizeFunc) NULL, (GClassInitFunc) gmpc_menu_item_rating_class_init, (GClassFinalizeFunc) NULL, NULL, sizeof (GmpcMenuItemRating), 0, (GInstanceInitFunc) gmpc_menu_item_rating_instance_init, NULL };
		gmpc_menu_item_rating_type_id = g_type_register_static (GTK_TYPE_MENU_ITEM, "GmpcMenuItemRating", &g_define_type_info, 0);
	}
	return gmpc_menu_item_rating_type_id;
}




