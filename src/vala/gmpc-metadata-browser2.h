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

#ifndef __GMPC_METADATA_BROWSER2_H__
#define __GMPC_METADATA_BROWSER2_H__

#include <glib.h>
#include <glib-object.h>
#include <gmpc-plugin.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS


#define GMPC_TYPE_METADATA_BROWSER (gmpc_metadata_browser_get_type ())
#define GMPC_METADATA_BROWSER(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GMPC_TYPE_METADATA_BROWSER, GmpcMetadataBrowser))
#define GMPC_METADATA_BROWSER_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GMPC_TYPE_METADATA_BROWSER, GmpcMetadataBrowserClass))
#define GMPC_IS_METADATA_BROWSER(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GMPC_TYPE_METADATA_BROWSER))
#define GMPC_IS_METADATA_BROWSER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GMPC_TYPE_METADATA_BROWSER))
#define GMPC_METADATA_BROWSER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GMPC_TYPE_METADATA_BROWSER, GmpcMetadataBrowserClass))

typedef struct _GmpcMetadataBrowser GmpcMetadataBrowser;
typedef struct _GmpcMetadataBrowserClass GmpcMetadataBrowserClass;
typedef struct _GmpcMetadataBrowserPrivate GmpcMetadataBrowserPrivate;

struct _GmpcMetadataBrowser {
	GmpcPluginBase parent_instance;
	GmpcMetadataBrowserPrivate * priv;
};

struct _GmpcMetadataBrowserClass {
	GmpcPluginBaseClass parent_class;
};


#define use_transition_mb TRUE
#define some_unique_name VERSION
GmpcMetadataBrowser* gmpc_metadata_browser_construct (GType object_type);
GmpcMetadataBrowser* gmpc_metadata_browser_new (void);
GType gmpc_metadata_browser_get_type (void);

static const gint GMPC_METADATA_BROWSER_version[] = {0, 0, 0};

G_END_DECLS

#endif
