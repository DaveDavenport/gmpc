
#include "gmpc-easy-command.h"
#include <playlist3-messages.h>
#include <stdio.h>




struct _GmpcEasyCommandPrivate {
	GtkEntryCompletion* completion;
	GtkListStore* store;
	guint signals;
};

#define GMPC_EASY_COMMAND_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GMPC_EASY_TYPE_COMMAND, GmpcEasyCommandPrivate))
enum  {
	GMPC_EASY_COMMAND_DUMMY_PROPERTY
};
static void _gmpc_easy_command_activate_gtk_entry_activate (GtkEntry* _sender, gpointer self);
static GObject * gmpc_easy_command_constructor (GType type, guint n_construct_properties, GObjectConstructParam * construct_properties);
static gpointer gmpc_easy_command_parent_class = NULL;
static void gmpc_easy_command_finalize (GObject* obj);
static int _vala_strcmp0 (const char * str1, const char * str2);



guint gmpc_easy_command_add_entry (GmpcEasyCommand* self, const char* name, GmpcEasyCommandgcallback* callback, void* userdata) {
	GtkTreeIter iter = {0};
	g_return_val_if_fail (self != NULL, 0U);
	g_return_val_if_fail (name != NULL, 0U);
	self->priv->signals++;
	gtk_list_store_append (self->priv->store, &iter);
	gtk_list_store_set (self->priv->store, &iter, 0, self->priv->signals, 1, name, 2, callback, 3, userdata, -1, -1);
	return self->priv->signals;
}


void gmpc_easy_command_activate (GmpcEasyCommand* self, GtkEntry* entry) {
	GtkTreeModel* _tmp0;
	GtkTreeModel* model;
	GtkTreeIter iter = {0};
	char* _tmp2;
	g_return_if_fail (self != NULL);
	g_return_if_fail (entry != NULL);
	_tmp0 = NULL;
	model = (_tmp0 = (GtkTreeModel*) self->priv->store, (_tmp0 == NULL) ? NULL : g_object_ref (_tmp0));
	/* ToDo: Make this nicer... maybe some fancy parsing */
	if (gtk_tree_model_get_iter_first (model, &iter)) {
		do {
			char* name;
			GmpcEasyCommandgcallback _tmp1;
			void* callback_target;
			GmpcEasyCommandgcallback callback;
			void* data;
			name = NULL;
			callback = (_tmp1 = NULL, callback_target = NULL, _tmp1);
			data = NULL;
			gtk_tree_model_get (model, &iter, 1, &name, 2, &callback, 3, &data, -1, -1);
			if (_vala_strcmp0 (name, gtk_entry_get_text (entry)) == 0) {
				callback (data, callback_target);
				gtk_object_destroy ((GtkObject*) gtk_widget_get_toplevel ((GtkWidget*) entry));
				name = (g_free (name), NULL);
				(model == NULL) ? NULL : (model = (g_object_unref (model), NULL));
				return;
			}
			name = (g_free (name), NULL);
		} while (gtk_tree_model_iter_next (model, &iter));
	}
	gtk_object_destroy ((GtkObject*) gtk_widget_get_toplevel ((GtkWidget*) entry));
	_tmp2 = NULL;
	playlist3_show_error_message (_tmp2 = g_strdup_printf ("Unkown command: '%s'", gtk_entry_get_text (entry)), ERROR_INFO);
	_tmp2 = (g_free (_tmp2), NULL);
	(model == NULL) ? NULL : (model = (g_object_unref (model), NULL));
}


static void _gmpc_easy_command_activate_gtk_entry_activate (GtkEntry* _sender, gpointer self) {
	gmpc_easy_command_activate (self, _sender);
}


void gmpc_easy_command_popup (GmpcEasyCommand* self) {
	GtkWindow* window;
	GtkEntry* entry;
	g_return_if_fail (self != NULL);
	window = g_object_ref_sink ((GtkWindow*) gtk_window_new (GTK_WINDOW_TOPLEVEL));
	entry = g_object_ref_sink ((GtkEntry*) gtk_entry_new ());
	gtk_window_set_decorated (window, FALSE);
	gtk_window_set_modal (window, TRUE);
	gtk_window_set_keep_above (window, TRUE);
	fprintf (stdout, "popup\n");
	gtk_entry_set_completion (entry, self->priv->completion);
	g_signal_connect_object (entry, "activate", (GCallback) _gmpc_easy_command_activate_gtk_entry_activate, self, 0);
	gtk_container_add ((GtkContainer*) window, (GtkWidget*) entry);
	gtk_widget_show_all ((GtkWidget*) window);
	gtk_widget_grab_focus ((GtkWidget*) entry);
	(window == NULL) ? NULL : (window = (g_object_unref (window), NULL));
	(entry == NULL) ? NULL : (entry = (g_object_unref (entry), NULL));
}


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
GmpcEasyCommand* gmpc_easy_command_construct (GType object_type) {
	GmpcEasyCommand * self;
	self = g_object_newv (object_type, 0, NULL);
	return self;
}


GmpcEasyCommand* gmpc_easy_command_new (void) {
	return gmpc_easy_command_construct (GMPC_EASY_TYPE_COMMAND);
}


static GObject * gmpc_easy_command_constructor (GType type, guint n_construct_properties, GObjectConstructParam * construct_properties) {
	GObject * obj;
	GmpcEasyCommandClass * klass;
	GObjectClass * parent_class;
	GmpcEasyCommand * self;
	klass = GMPC_EASY_COMMAND_CLASS (g_type_class_peek (GMPC_EASY_TYPE_COMMAND));
	parent_class = G_OBJECT_CLASS (g_type_class_peek_parent (klass));
	obj = parent_class->constructor (type, n_construct_properties, construct_properties);
	self = GMPC_EASY_COMMAND (obj);
	{
		GtkListStore* _tmp0;
		GtkEntryCompletion* _tmp1;
		_tmp0 = NULL;
		self->priv->store = (_tmp0 = gtk_list_store_new (4, G_TYPE_UINT, G_TYPE_STRING, G_TYPE_POINTER, G_TYPE_POINTER, NULL), (self->priv->store == NULL) ? NULL : (self->priv->store = (g_object_unref (self->priv->store), NULL)), _tmp0);
		_tmp1 = NULL;
		self->priv->completion = (_tmp1 = gtk_entry_completion_new (), (self->priv->completion == NULL) ? NULL : (self->priv->completion = (g_object_unref (self->priv->completion), NULL)), _tmp1);
		gtk_entry_completion_set_model (self->priv->completion, (GtkTreeModel*) self->priv->store);
		gtk_entry_completion_set_text_column (self->priv->completion, 1);
		gtk_entry_completion_set_inline_completion (self->priv->completion, TRUE);
		gtk_entry_completion_set_inline_selection (self->priv->completion, TRUE);
	}
	return obj;
}


static void gmpc_easy_command_class_init (GmpcEasyCommandClass * klass) {
	gmpc_easy_command_parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (GmpcEasyCommandPrivate));
	G_OBJECT_CLASS (klass)->constructor = gmpc_easy_command_constructor;
	G_OBJECT_CLASS (klass)->finalize = gmpc_easy_command_finalize;
}


static void gmpc_easy_command_instance_init (GmpcEasyCommand * self) {
	self->priv = GMPC_EASY_COMMAND_GET_PRIVATE (self);
	self->priv->completion = NULL;
	self->priv->store = NULL;
	self->priv->signals = (guint) 0;
}


static void gmpc_easy_command_finalize (GObject* obj) {
	GmpcEasyCommand * self;
	self = GMPC_EASY_COMMAND (obj);
	(self->priv->completion == NULL) ? NULL : (self->priv->completion = (g_object_unref (self->priv->completion), NULL));
	(self->priv->store == NULL) ? NULL : (self->priv->store = (g_object_unref (self->priv->store), NULL));
	G_OBJECT_CLASS (gmpc_easy_command_parent_class)->finalize (obj);
}


GType gmpc_easy_command_get_type (void) {
	static GType gmpc_easy_command_type_id = 0;
	if (gmpc_easy_command_type_id == 0) {
		static const GTypeInfo g_define_type_info = { sizeof (GmpcEasyCommandClass), (GBaseInitFunc) NULL, (GBaseFinalizeFunc) NULL, (GClassInitFunc) gmpc_easy_command_class_init, (GClassFinalizeFunc) NULL, NULL, sizeof (GmpcEasyCommand), 0, (GInstanceInitFunc) gmpc_easy_command_instance_init, NULL };
		gmpc_easy_command_type_id = g_type_register_static (G_TYPE_OBJECT, "GmpcEasyCommand", &g_define_type_info, 0);
	}
	return gmpc_easy_command_type_id;
}


static int _vala_strcmp0 (const char * str1, const char * str2) {
	if (str1 == NULL) {
		return -(str1 != str2);
	}
	if (str2 == NULL) {
		return str1 != str2;
	}
	return strcmp (str1, str2);
}




