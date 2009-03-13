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

#ifndef __GMPC_EASY_COMMAND_H__
#define __GMPC_EASY_COMMAND_H__

#include <glib.h>
#include <glib-object.h>
#include <stdlib.h>
#include <string.h>
#include "gmpc-plugin2.h"

G_BEGIN_DECLS


#define GMPC_EASY_TYPE_COMMAND (gmpc_easy_command_get_type ())
#define GMPC_EASY_COMMAND(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GMPC_EASY_TYPE_COMMAND, GmpcEasyCommand))
#define GMPC_EASY_COMMAND_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GMPC_EASY_TYPE_COMMAND, GmpcEasyCommandClass))
#define GMPC_EASY_IS_COMMAND(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GMPC_EASY_TYPE_COMMAND))
#define GMPC_EASY_IS_COMMAND_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GMPC_EASY_TYPE_COMMAND))
#define GMPC_EASY_COMMAND_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GMPC_EASY_TYPE_COMMAND, GmpcEasyCommandClass))

typedef struct _GmpcEasyCommand GmpcEasyCommand;
typedef struct _GmpcEasyCommandClass GmpcEasyCommandClass;
typedef struct _GmpcEasyCommandPrivate GmpcEasyCommandPrivate;
typedef void (*GmpcEasyCommandCallback) (void* data, const char* param, void* user_data);

struct _GmpcEasyCommand {
	GmpcPluginBase parent_instance;
	GmpcEasyCommandPrivate * priv;
};

struct _GmpcEasyCommandClass {
	GmpcPluginBaseClass parent_class;
};


guint gmpc_easy_command_add_entry (GmpcEasyCommand* self, const char* name, const char* pattern, const char* hint, GmpcEasyCommandCallback* callback, void* userdata);
void gmpc_easy_command_popup (GmpcEasyCommand* self);
GmpcEasyCommand* gmpc_easy_command_construct (GType object_type);
GmpcEasyCommand* gmpc_easy_command_new (void);
GType gmpc_easy_command_get_type (void);


G_END_DECLS

#endif
