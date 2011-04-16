/* Gnome Music Player Client (GMPC)
 * Copyright (C) 2011-2011 Qball Cow <qball@gmpclient.org>
 * Project homepage: http://gmpclient.org/

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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <strings.h>
#include <config.h>
#include <glib.h>
#include <glib-object.h>
#ifndef CLIENT_ONLY
#include "main.h"
#include "playlist3.h"
#include "plugin.h"
#include "misc.h"
#include "gmpc_easy_download.h"
#endif
#include "ipc.h"
#ifdef HAVE_IPC
#include <unique/unique.h>

#define LOG_DOMAIN_IPC "IPC"


static GType gmpc_tools_ipc_get_type(void);

/* For easy casting and type checking */
#define GMPC_TOOLS_IPC(obj)	G_TYPE_CHECK_INSTANCE_CAST((obj), gmpc_tools_ipc_get_type(), GmpcToolsIPC)

/**
 * Private data structure
 */
/* Hold the objects data fields */
typedef struct _GmpcToolsIPCPrivate
{
	/* The unique object */
	UniqueApp *app;

} GmpcToolsIPCPrivate;

/* The object, contains the parent GObject and a pointer to our
 * private data
 */
typedef struct _GmpcToolsIPC
{
	GObject parent_instance;
	GmpcToolsIPCPrivate *priv;
} GmpcToolsIPC;

/* Class object */
typedef struct _GmpcToolsIPCClass
{
	GObjectClass parent_class;
} GmpcToolsIPCClass;

#ifndef CLIENT_ONLY
static UniqueResponse
gmpc_tools_ipc_message_received_cb (UniqueApp         *app,
UniqueCommand      command,
UniqueMessageData *message,
guint              time_,
gpointer           user_data)
{
	UniqueResponse res;

	/* Cast to (int) to avoid stupid compile warning */
	switch ((int)command)
	{
		case UNIQUE_ACTIVATE:
			/* move the main window to the screen that sent us the command */
			gtk_window_set_screen (GTK_WINDOW (playlist3_get_window()),
				unique_message_data_get_screen (message));
			gtk_window_present_with_time (GTK_WINDOW (playlist3_get_window()), time_);
			res = UNIQUE_RESPONSE_OK;
			break;
		case COMMAND_STREAM:
		{
			gchar *fn = unique_message_data_get_filename(message);
			res = UNIQUE_RESPONSE_FAIL;
			if(fn)
			{
				url_start_real(fn);
				g_free(fn);
				res = UNIQUE_RESPONSE_OK;
			}
			break;
		}
		case COMMAND_EASYCOMMAND:
		{
			gchar *fn = unique_message_data_get_text(message);
			res = UNIQUE_RESPONSE_FAIL;
			if(fn)
			{
				gmpc_easy_command_do_query(gmpc_easy_command,
					fn);
				g_free(fn);
				res = UNIQUE_RESPONSE_OK;
			}
			break;
		}
		case UNIQUE_INVALID:
		case UNIQUE_NEW:
		case UNIQUE_CLOSE:
		case UNIQUE_OPEN:
		default:
			res = UNIQUE_RESPONSE_OK;
			break;
	}

	return res;
}
#endif


/**
 * Initialize the plugin, this is done on object construction.
 */
static void gmpc_tools_ipc_init(GmpcToolsIPC *self)
{
	/* Setup the unique app */
	self->priv->app = unique_app_new_with_commands ("org.gmpclient.GMPC", NULL,
		"stream",       COMMAND_STREAM,
		"easycommand",  COMMAND_EASYCOMMAND,
		NULL);
	/* using this signal we get notifications from the newly launched instances
	 * and we can reply to them; the default signal handler will just return
	 * RESPONSE_OK and terminate the startup notification sequence on each
	 * watched window, so you can connect to the message-received signal only if
	 * you want to handle the commands and responses
	 */

#ifndef CLIENT_ONLY
	g_signal_connect (self->priv->app,
		"message-received",
		G_CALLBACK (gmpc_tools_ipc_message_received_cb), self);
#endif
}


/**
 * Constructor. This is called when the GObject gets created (via GObject::new).
 * This initializes the parent, and then calls init.
 */
static GObject *gmpc_tools_ipc_constructor(GType type, guint n_construct_properties,
GObjectConstructParam * construct_properties)
{
	GmpcToolsIPCClass *klass;
	GmpcToolsIPC *self;
	GObjectClass *parent_class;
	klass = (g_type_class_peek(gmpc_tools_ipc_get_type()));
	parent_class = G_OBJECT_CLASS(g_type_class_peek_parent(klass));
	self = (GmpcToolsIPC *) parent_class->constructor(type,
		n_construct_properties,
		construct_properties);

	/* setup private structure */
	self->priv = g_malloc0(sizeof(GmpcToolsIPCPrivate));
	self->priv->app = NULL;

	/* call init function? */

	gmpc_tools_ipc_init(self);
	return G_OBJECT(self);
}


/* Function that gets called on destruction off the object */
static void gmpc_tools_ipc_finalize(GObject * obj)
{
	GmpcToolsIPC *self = GMPC_TOOLS_IPC(obj); 
	GmpcToolsIPCClass *klass = (g_type_class_peek(gmpc_tools_ipc_get_type()));
	gpointer parent_class = g_type_class_peek_parent(klass);

	if (self->priv != NULL)
	{
		GmpcToolsIPCPrivate *priv = self->priv;
		/* Destroy unique app */
		if(priv->app != NULL)
		{
			g_object_unref(priv->app);
			priv->app = NULL;
		}
		/* Free private data */
		g_free(priv);
		self->priv = NULL;
	}
	if (parent_class)
		G_OBJECT_CLASS(parent_class)->finalize(obj);
}


static void gmpc_tools_ipc_class_init(GmpcToolsIPCClass * klass)
{
	/* Connect destroy and construct */
	G_OBJECT_CLASS(klass)->finalize = gmpc_tools_ipc_finalize;
	G_OBJECT_CLASS(klass)->constructor = gmpc_tools_ipc_constructor;

}


static GType gmpc_tools_ipc_get_type(void)
{
	static GType gmpc_tools_ipc_type_id = 0;
	if (gmpc_tools_ipc_type_id == 0)
	{
		static const GTypeInfo info =
		{
			.class_size = sizeof(GmpcToolsIPCClass),
			.class_init = (GClassInitFunc) gmpc_tools_ipc_class_init,
			.instance_size = sizeof(GmpcToolsIPC),
			.n_preallocs = 0
		};
		gmpc_tools_ipc_type_id =
			g_type_register_static(G_TYPE_OBJECT,
			"GmpcToolIPC", &info, 0);
	}
	return gmpc_tools_ipc_type_id;
}


/* Public api */
GObject *gmpc_tools_ipc_new(void)
{
	return g_object_new(gmpc_tools_ipc_get_type(), NULL);
}


gboolean gmpc_tools_ipc_is_running(GObject *ipc)
{
	GmpcToolsIPC *self = GMPC_TOOLS_IPC(ipc); 
	return unique_app_is_running(self->priv->app);
}


gboolean gmpc_tools_ipc_send(GObject *ipc, GmpcToolsIPCCommands command,const char *command_param)
{
	GmpcToolsIPC *self = GMPC_TOOLS_IPC(ipc); 
	UniqueResponse response;	 /* the response to our command */
	switch(command)
	{
		case COMMAND_STREAM:
		{
			/* the payload for the command */
			UniqueMessageData *message;
			message = unique_message_data_new();
			unique_message_data_set_filename(message, command_param);

			response = unique_app_send_message(self->priv->app, command, message);
			/* the message is copied, so we need to free it before returning */
			unique_message_data_free (message);
			if(response  == UNIQUE_RESPONSE_FAIL)
			{
				g_log(LOG_DOMAIN_IPC, G_LOG_LEVEL_WARNING,
					"Failed to send %i command",
					command);
				return FALSE;
			}
			break;
		}
		case COMMAND_EASYCOMMAND:
		{
			/* the payload for the command */
			UniqueMessageData *message;
			message = unique_message_data_new();
			unique_message_data_set_text(message, command_param,-1);

			response = unique_app_send_message(self->priv->app, command, message);
			/* the message is copied, so we need to free it before returning */
			unique_message_data_free (message);
			if(response  == UNIQUE_RESPONSE_FAIL)
			{
				g_log(LOG_DOMAIN_IPC, G_LOG_LEVEL_WARNING,
					"Failed to send %i command",
					command);
				return FALSE;
			}
			break;
		}
		case COMMAND_0:
		default:
			return FALSE;
			break;
	}
	return TRUE;
}

#ifndef CLIENT_ONLY
void gmpc_tools_ipc_watch_window(GObject *ipc, GtkWindow *win)
{
	GmpcToolsIPC *self = GMPC_TOOLS_IPC(ipc); 
	unique_app_watch_window(self->priv->app, win);
}
#endif
#endif
