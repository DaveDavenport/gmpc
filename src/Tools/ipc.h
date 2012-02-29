/* Gnome Music Player Client (GMPC)
 * Copyright (C) 2011-2012 Qball Cow <qball@gmpclient.org>
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

#ifndef __IPC_H__
#define __IPC_H__

#ifdef HAVE_UNIQUE

/* Set the define HAVE_IPC.
 * outside ipc.c|h only check that.
 * not HAVE_UNIQUE
 */
#define HAVE_IPC

/**
 * Supported commands
 */
typedef enum
{
	COMMAND_0, /* unused: 0 is an invalid command */
	COMMAND_STREAM,
	COMMAND_EASYCOMMAND
} GmpcToolsIPCCommands;

/**
 * @param ipc: the GmpcToolsIPC object
 * 
 * Check if gmpc is allready running.
 *
 * @returns true if gmpc is allready running
 */
gboolean gmpc_tools_ipc_is_running(GObject *ipc);
/**
 * Create a new GmpcToolsIPC instance
 *
 * @returns a new GmpcToolsIPC instance
 */
GObject *gmpc_tools_ipc_new(void);

/**
 * @param ipc: the GmpcToolsIPC object
 * @param command: The #GmpcToolsIPCCommands to send.
 * @param command_param: The message to send along the command or NULL
 *
 * send a command (with possible message) to the running gmpc. 
 *
 * @returns TRUE is succesfull.
 */

gboolean gmpc_tools_ipc_send(GObject *ipc, GmpcToolsIPCCommands command,const char *command_param); 
#ifndef CLIENT_ONLY

/**
 * @param ipc: the GmpcToolsIPC object
 * @param win: A GtkWindow
 *
 * watch a window. (see libunique documentation) 
 *
 */
void gmpc_tools_ipc_watch_window(GObject *ipc, GtkWindow *win);
#endif
#endif
#endif // __IPC_H__
