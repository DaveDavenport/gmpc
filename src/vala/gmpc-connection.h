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

#ifndef __GMPC_CONNECTION_H__
#define __GMPC_CONNECTION_H__

#include <glib.h>
#include <glib-object.h>
#include <libmpd/libmpd.h>

G_BEGIN_DECLS


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


void gmpc_connection_call_connection_changed (GmpcConnection* self, MpdObj* mi, gint connect);
void gmpc_connection_call_status_changed (GmpcConnection* self, MpdObj* mi, ChangedStatusType what);
GmpcConnection* gmpc_connection_construct (GType object_type);
GmpcConnection* gmpc_connection_new (void);
GType gmpc_connection_get_type (void);


G_END_DECLS

#endif
