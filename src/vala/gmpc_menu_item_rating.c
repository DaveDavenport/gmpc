
#include "gmpc_menu_item_rating.h"
#include <config.h>
#include <gdk/gdk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib/gi18n-lib.h>
#include "gtktransition.h"




enum  {
	GMPC_MENU_ITEM_RATING_DUMMY_PROPERTY
};
#define GMPC_MENU_ITEM_RATING_some_unique_name VERSION
static gboolean gmpc_menu_item_rating_button_press_event_callback (GmpcMenuItemRating* self, const GdkEventButton* event, void* userdata);
static gboolean gmpc_menu_item_rating_button_release_event_callback (GmpcMenuItemRating* self, const GdkEventButton* event, void* userdata);
static gpointer gmpc_menu_item_rating_parent_class = NULL;
static void gmpc_menu_item_rating_finalize (GObject* obj);



gint gmpc_menu_item_rating_get_rating (GmpcMenuItemRating* self) {
	g_return_val_if_fail (self != NULL, 0);
	return 0;
}


static gboolean gmpc_menu_item_rating_button_press_event_callback (GmpcMenuItemRating* self, const GdkEventButton* event, void* userdata) {
	g_return_val_if_fail (self != NULL, FALSE);
	fprintf (stdout, "Button press event\n");
	gmpc_rating_button_press_event_callback (self->rating, self->rating->event_box, &(*event));
	return TRUE;
}


static gboolean gmpc_menu_item_rating_button_release_event_callback (GmpcMenuItemRating* self, const GdkEventButton* event, void* userdata) {
	g_return_val_if_fail (self != NULL, FALSE);
	return TRUE;
}


GmpcMenuItemRating* gmpc_menu_item_rating_construct (GType object_type, MpdObj* server, const mpd_Song* song) {
	GmpcMenuItemRating * self;
	GtkVBox* _tmp0;
	GmpcRating* _tmp1;
	GtkLabel* _tmp2;
	g_return_val_if_fail (server != NULL, NULL);
	g_return_val_if_fail (song != NULL, NULL);
	self = g_object_newv (object_type, 0, NULL);
	g_signal_connect_swapped (self, "button-press-event", (GCallback) gmpc_menu_item_rating_button_press_event_callback, self);
	g_signal_connect_swapped (self, "button-release-event", (GCallback) gmpc_menu_item_rating_button_release_event_callback, self);
	_tmp0 = NULL;
	self->hbox = (_tmp0 = g_object_ref_sink ((GtkVBox*) gtk_vbox_new (FALSE, 6)), (self->hbox == NULL) ? NULL : (self->hbox = (g_object_unref (self->hbox), NULL)), _tmp0);
	_tmp1 = NULL;
	self->rating = (_tmp1 = g_object_ref_sink (gmpc_rating_new (server, song)), (self->rating == NULL) ? NULL : (self->rating = (g_object_unref (self->rating), NULL)), _tmp1);
	_tmp2 = NULL;
	gtk_box_pack_start ((GtkBox*) self->hbox, (GtkWidget*) (_tmp2 = g_object_ref_sink ((GtkLabel*) gtk_label_new (_ ("Rating:")))), FALSE, TRUE, (guint) 0);
	(_tmp2 == NULL) ? NULL : (_tmp2 = (g_object_unref (_tmp2), NULL));
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




