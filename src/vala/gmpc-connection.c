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
#include <libmpd/libmpd.h>


#define GMPC_TYPE_CONNECTION (gmpc_connection_get_type ())
#define GMPC_CONNECTION(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GMPC_TYPE_CONNECTION, GmpcConnection))
#define GMPC_CONNECTION_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GMPC_TYPE_CONNECTION, GmpcConnectionClass))
#define GMPC_IS_CONNECTION(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GMPC_TYPE_CONNECTION))
#define GMPC_IS_CONNECTION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GMPC_TYPE_CONNECTION))
#define GMPC_CONNECTION_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GMPC_TYPE_CONNECTION, GmpcConnectionClass))

typedef struct _GmpcConnection GmpcConnection;
typedef struct _GmpcConnectionClass GmpcConnectionClass;
typedef struct _GmpcConnectionPrivate GmpcConnectionPrivate;

struct _GmpcConnection {
	GObject parent_instance;
	GmpcConnectionPrivate * priv;
};

struct _GmpcConnectionClass {
	GObjectClass parent_class;
};



GType gmpc_connection_get_type (void);
enum  {
	GMPC_CONNECTION_DUMMY_PROPERTY
};
GmpcConnection* gmpc_connection_new (void);
GmpcConnection* gmpc_connection_construct (GType object_type);
GmpcConnection* gmpc_connection_new (void);
static gpointer gmpc_connection_parent_class = NULL;


static void g_cclosure_user_marshal_VOID__POINTER_INT (GClosure * closure, GValue * return_value, guint n_param_values, const GValue * param_values, gpointer invocation_hint, gpointer marshal_data);

GmpcConnection* gmpc_connection_construct (GType object_type) {
	GmpcConnection * self;
	self = g_object_newv (object_type, 0, NULL);
	return self;
}


GmpcConnection* gmpc_connection_new (void) {
	return gmpc_connection_construct (GMPC_TYPE_CONNECTION);
}


static void gmpc_connection_class_init (GmpcConnectionClass * klass) {
	gmpc_connection_parent_class = g_type_class_peek_parent (klass);
	g_signal_new ("connection_changed", GMPC_TYPE_CONNECTION, G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_user_marshal_VOID__POINTER_INT, G_TYPE_NONE, 2, G_TYPE_POINTER, G_TYPE_INT);
	g_signal_new ("status_changed", GMPC_TYPE_CONNECTION, G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_user_marshal_VOID__POINTER_INT, G_TYPE_NONE, 2, G_TYPE_POINTER, G_TYPE_INT);
}


static void gmpc_connection_instance_init (GmpcConnection * self) {
}


GType gmpc_connection_get_type (void) {
	static GType gmpc_connection_type_id = 0;
	if (gmpc_connection_type_id == 0) {
		static const GTypeInfo g_define_type_info = { sizeof (GmpcConnectionClass), (GBaseInitFunc) NULL, (GBaseFinalizeFunc) NULL, (GClassInitFunc) gmpc_connection_class_init, (GClassFinalizeFunc) NULL, NULL, sizeof (GmpcConnection), 0, (GInstanceInitFunc) gmpc_connection_instance_init, NULL };
		gmpc_connection_type_id = g_type_register_static (G_TYPE_OBJECT, "GmpcConnection", &g_define_type_info, 0);
	}
	return gmpc_connection_type_id;
}



static void g_cclosure_user_marshal_VOID__POINTER_INT (GClosure * closure, GValue * return_value, guint n_param_values, const GValue * param_values, gpointer invocation_hint, gpointer marshal_data) {
	typedef void (*GMarshalFunc_VOID__POINTER_INT) (gpointer data1, gpointer arg_1, gint arg_2, gpointer data2);
	register GMarshalFunc_VOID__POINTER_INT callback;
	register GCClosure * cc;
	register gpointer data1, data2;
	cc = (GCClosure *) closure;
	g_return_if_fail (n_param_values == 3);
	if (G_CCLOSURE_SWAP_DATA (closure)) {
		data1 = closure->data;
		data2 = param_values->data[0].v_pointer;
	} else {
		data1 = param_values->data[0].v_pointer;
		data2 = closure->data;
	}
	callback = (GMarshalFunc_VOID__POINTER_INT) (marshal_data ? marshal_data : cc->callback);
	callback (data1, g_value_get_pointer (param_values + 1), g_value_get_int (param_values + 2), data2);
}



