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
#include <plugin.h>
#include <config1.h>
#include <stdio.h>
#include <gdk/gdk.h>


#define GMPC_TYPE_PANED_SIZE_GROUP (gmpc_paned_size_group_get_type ())
#define GMPC_PANED_SIZE_GROUP(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GMPC_TYPE_PANED_SIZE_GROUP, GmpcPanedSizeGroup))
#define GMPC_PANED_SIZE_GROUP_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GMPC_TYPE_PANED_SIZE_GROUP, GmpcPanedSizeGroupClass))
#define GMPC_IS_PANED_SIZE_GROUP(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GMPC_TYPE_PANED_SIZE_GROUP))
#define GMPC_IS_PANED_SIZE_GROUP_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GMPC_TYPE_PANED_SIZE_GROUP))
#define GMPC_PANED_SIZE_GROUP_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GMPC_TYPE_PANED_SIZE_GROUP, GmpcPanedSizeGroupClass))

typedef struct _GmpcPanedSizeGroup GmpcPanedSizeGroup;
typedef struct _GmpcPanedSizeGroupClass GmpcPanedSizeGroupClass;
typedef struct _GmpcPanedSizeGroupPrivate GmpcPanedSizeGroupPrivate;

struct _GmpcPanedSizeGroup {
	GObject parent_instance;
	GmpcPanedSizeGroupPrivate * priv;
};

struct _GmpcPanedSizeGroupClass {
	GObjectClass parent_class;
};

struct _GmpcPanedSizeGroupPrivate {
	GList* list;
	gint position;
	gboolean block_changed_callback;
};


static gpointer gmpc_paned_size_group_parent_class = NULL;

GType gmpc_paned_size_group_get_type (void);
#define GMPC_PANED_SIZE_GROUP_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GMPC_TYPE_PANED_SIZE_GROUP, GmpcPanedSizeGroupPrivate))
enum  {
	GMPC_PANED_SIZE_GROUP_DUMMY_PROPERTY
};
GmpcPanedSizeGroup* gmpc_paned_size_group_new (void);
GmpcPanedSizeGroup* gmpc_paned_size_group_construct (GType object_type);
GmpcPanedSizeGroup* gmpc_paned_size_group_new (void);
static gboolean gmpc_paned_size_group_child_destroy_event (GmpcPanedSizeGroup* self, GtkWidget* paned, GdkEvent* event);
static void gmpc_paned_size_group_child_position_changed (GmpcPanedSizeGroup* self, GObject* paned, GParamSpec* spec);
static void _gmpc_paned_size_group_child_position_changed_g_object_notify (GObject* _sender, GParamSpec* pspec, gpointer self);
static gboolean _gmpc_paned_size_group_child_destroy_event_gtk_widget_destroy_event (GtkWidget* _sender, GdkEvent* event, gpointer self);
void gmpc_paned_size_group_add_paned (GmpcPanedSizeGroup* self, GtkPaned* paned);
static void gmpc_paned_size_group_finalize (GObject* obj);



GmpcPanedSizeGroup* gmpc_paned_size_group_construct (GType object_type) {
	GmpcPanedSizeGroup * self;
	self = g_object_newv (object_type, 0, NULL);
	return self;
}


GmpcPanedSizeGroup* gmpc_paned_size_group_new (void) {
	return gmpc_paned_size_group_construct (GMPC_TYPE_PANED_SIZE_GROUP);
}


static gboolean gmpc_paned_size_group_child_destroy_event (GmpcPanedSizeGroup* self, GtkWidget* paned, GdkEvent* event) {
	gboolean result;
	g_return_val_if_fail (self != NULL, FALSE);
	g_return_val_if_fail (paned != NULL, FALSE);
	g_return_val_if_fail (event != NULL, FALSE);
	self->priv->list = g_list_remove (self->priv->list, GTK_PANED (paned));
	fprintf (stdout, "Remove paned\n");
	result = FALSE;
	return result;
}


static void gmpc_paned_size_group_child_position_changed (GmpcPanedSizeGroup* self, GObject* paned, GParamSpec* spec) {
	GtkPaned* _tmp0_;
	GtkPaned* pane;
	g_return_if_fail (self != NULL);
	g_return_if_fail (paned != NULL);
	g_return_if_fail (spec != NULL);
	if (self->priv->block_changed_callback) {
		return;
	}
	self->priv->block_changed_callback = TRUE;
	_tmp0_ = NULL;
	pane = (_tmp0_ = GTK_PANED (paned), (_tmp0_ == NULL) ? NULL : g_object_ref (_tmp0_));
	self->priv->position = gtk_paned_get_position (pane);
	fprintf (stdout, "position: %i\n", gtk_paned_get_position (pane));
	{
		GList* p_collection;
		GList* p_it;
		p_collection = self->priv->list;
		for (p_it = p_collection; p_it != NULL; p_it = p_it->next) {
			GtkPaned* p;
			p = (GtkPaned*) p_it->data;
			{
				if (G_OBJECT (p) != paned) {
					gtk_paned_set_position (p, self->priv->position);
				}
			}
		}
	}
	self->priv->block_changed_callback = FALSE;
	(pane == NULL) ? NULL : (pane = (g_object_unref (pane), NULL));
}


static void _gmpc_paned_size_group_child_position_changed_g_object_notify (GObject* _sender, GParamSpec* pspec, gpointer self) {
	gmpc_paned_size_group_child_position_changed (self, _sender, pspec);
}


static gboolean _gmpc_paned_size_group_child_destroy_event_gtk_widget_destroy_event (GtkWidget* _sender, GdkEvent* event, gpointer self) {
	return gmpc_paned_size_group_child_destroy_event (self, _sender, event);
}


void gmpc_paned_size_group_add_paned (GmpcPanedSizeGroup* self, GtkPaned* paned) {
	g_return_if_fail (self != NULL);
	g_return_if_fail (paned != NULL);
	g_signal_connect_object ((GObject*) paned, "notify::position", (GCallback) _gmpc_paned_size_group_child_position_changed_g_object_notify, self, 0);
	g_signal_connect_object ((GtkWidget*) paned, "destroy-event", (GCallback) _gmpc_paned_size_group_child_destroy_event_gtk_widget_destroy_event, self, 0);
	self->priv->block_changed_callback = TRUE;
	gtk_paned_set_position (paned, self->priv->position);
	self->priv->block_changed_callback = FALSE;
	self->priv->list = g_list_append (self->priv->list, paned);
}


static void gmpc_paned_size_group_class_init (GmpcPanedSizeGroupClass * klass) {
	gmpc_paned_size_group_parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (GmpcPanedSizeGroupPrivate));
	G_OBJECT_CLASS (klass)->finalize = gmpc_paned_size_group_finalize;
}


static void gmpc_paned_size_group_instance_init (GmpcPanedSizeGroup * self) {
	self->priv = GMPC_PANED_SIZE_GROUP_GET_PRIVATE (self);
	self->priv->list = NULL;
	self->priv->position = cfg_get_single_value_as_int_with_default (config, "paned-size-group", "position", 150);
	self->priv->block_changed_callback = FALSE;
}


static void gmpc_paned_size_group_finalize (GObject* obj) {
	GmpcPanedSizeGroup * self;
	self = GMPC_PANED_SIZE_GROUP (obj);
	{
		fprintf (stdout, "PanedSizeGroup destroy\n");
		cfg_set_single_value_as_int (config, "paned-size-group", "position", self->priv->position);
	}
	(self->priv->list == NULL) ? NULL : (self->priv->list = (g_list_free (self->priv->list), NULL));
	G_OBJECT_CLASS (gmpc_paned_size_group_parent_class)->finalize (obj);
}


GType gmpc_paned_size_group_get_type (void) {
	static GType gmpc_paned_size_group_type_id = 0;
	if (gmpc_paned_size_group_type_id == 0) {
		static const GTypeInfo g_define_type_info = { sizeof (GmpcPanedSizeGroupClass), (GBaseInitFunc) NULL, (GBaseFinalizeFunc) NULL, (GClassInitFunc) gmpc_paned_size_group_class_init, (GClassFinalizeFunc) NULL, NULL, sizeof (GmpcPanedSizeGroup), 0, (GInstanceInitFunc) gmpc_paned_size_group_instance_init, NULL };
		gmpc_paned_size_group_type_id = g_type_register_static (G_TYPE_OBJECT, "GmpcPanedSizeGroup", &g_define_type_info, 0);
	}
	return gmpc_paned_size_group_type_id;
}




