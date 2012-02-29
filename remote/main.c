/* Gnome Music Player (GMPC)
 * Copyright (C) 2004-2012 Qball Cow <qball@gmpclient.org>
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <gtk/gtk.h>
#include <config.h>
#include "ipc.h"
int main ( int argc, char **argv )
{
    int i =0;
	GObject *ipc = NULL;
	gboolean response = FALSE;

	GError *error = NULL;
	GOptionContext *context;
	/* version */
	gboolean version;
	/* quit gmpc */
	gboolean quit=FALSE;
	/* play */
	gboolean play=FALSE;
	gboolean pause=FALSE;
	gboolean prev=FALSE;
	gboolean next=FALSE;
	gboolean stop=FALSE;
	gboolean toggle_view = FALSE;
	gboolean hide_view = FALSE;
	gboolean show_view = FALSE;
	gchar *stream = NULL;
	gchar *easycommand = NULL;

	GOptionEntry entries[] = {
		{
			"version",  0, 0, G_OPTION_ARG_NONE, &version,
			"Print the version number and exit", NULL
		},
		{
			"quit",    'q', 0, G_OPTION_ARG_NONE, &quit,
			"Quit the running gmpc", NULL
		},
		{
			"play",     'c', 0, G_OPTION_ARG_NONE, &play,
			"Give the running gmpc the play command", NULL
		},
		{
			"pause",     'v', 0, G_OPTION_ARG_NONE, &pause,
			"Give the running gmpc the pause command", NULL
		},
		{
			"next",     'b', 0, G_OPTION_ARG_NONE, &next,
			"Give the running gmpc the next command", NULL
		},
		{
			"prev",     'z', 0, G_OPTION_ARG_NONE, &prev,
			"Give the running gmpc the prev command", NULL
		},
		{
			"stop",     'x', 0, G_OPTION_ARG_NONE, &stop,
			"Give the running gmpc the stop command", NULL
		},
		{

			"stream",     's', 0, G_OPTION_ARG_STRING, &stream,
			"Give the running gmpc a stream to play", NULL
		},
		{
			"easycommand",     'e', 0, G_OPTION_ARG_STRING, &easycommand,
			"Send an easy command query to the running gmpc. Enclose query in double quotes if it contains spaces", NULL
		},
		{

			"toggle-view", 't', 0, G_OPTION_ARG_NONE, &toggle_view,
			"Give the running gmpc the command to toggle the window visibility", NULL
		},
		{

			"hide-view", 'h', 0, G_OPTION_ARG_NONE, &hide_view,
			"Give the running gmpc the command to hide the window.", NULL
		},
		{
			"show-view", 'k', 0, G_OPTION_ARG_NONE, &show_view,
			"Give the running gmpc the command to show the window.", NULL
		},  
		{NULL}
	};

	gtk_init(&argc, &argv);
	/*Create the commandline option parser */
	context = g_option_context_new("GMPC remote program");
	g_option_context_add_main_entries(context, entries, NULL);

	/*Time to parse the options */
	g_option_context_parse(context, &argc, &argv, &error);
	g_option_context_free(context);
	if(error){
		printf("ERROR: failed to parse command line options: '%s'\n", error->message);
		return EXIT_FAILURE;
	}

	ipc = gmpc_tools_ipc_new();
	if(!gmpc_tools_ipc_is_running(ipc)) {
		fprintf(stderr, "Error: GMPC is not running\n");
		g_object_unref(ipc);
		return EXIT_FAILURE;
	}

	if(play)
	{
		response = gmpc_tools_ipc_send(ipc, COMMAND_EASYCOMMAND, "play");
		if(!response) printf("Failed to send PLAY command\n");
	}
	if (pause)
	{
		response = gmpc_tools_ipc_send(ipc, COMMAND_EASYCOMMAND, "pause");
		if(!response) printf("Failed to send PAUSE command\n");
	}
	if(prev)
	{
		response = gmpc_tools_ipc_send(ipc, COMMAND_EASYCOMMAND, "prev");
		if(!response) printf("Failed to send PREV command\n");
	}
	if(next)
	{
		response = gmpc_tools_ipc_send(ipc, COMMAND_EASYCOMMAND, "next");
		if(!response) printf("Failed to send NEXT command\n");
	}
	if(stop)
	{
		response = gmpc_tools_ipc_send(ipc, COMMAND_EASYCOMMAND, "stop");
		if(!response) printf("Failed to send STOP command\n");
	}
	if(toggle_view)
	{
		response = gmpc_tools_ipc_send(ipc, COMMAND_EASYCOMMAND, "toggle");
		if(!response) printf("Failed to send VIEW_TOGGLE command\n");
	}
	if(hide_view)
	{
		response = gmpc_tools_ipc_send(ipc, COMMAND_EASYCOMMAND, "hide");
		if(!response) printf("Failed to send VIEW_HIDE command\n");
	}
	if(show_view)
	{
		response = gmpc_tools_ipc_send(ipc, COMMAND_EASYCOMMAND, "show");
		if(!response) printf("Failed to send VIEW_SHOW command\n");
	}
	if(stream)
	{
		gchar *command = g_strdup_printf("url %s", stream);
		response = gmpc_tools_ipc_send(ipc, COMMAND_EASYCOMMAND, command);
		g_free(command);
		if(!response) printf("Failed to send STREAM command\n");
	}
	if(easycommand)
	{
		response = gmpc_tools_ipc_send(ipc, COMMAND_EASYCOMMAND, easycommand);
		if(!response) printf("Failed to send EASYCOMMAND command\n");
	}
	if(quit)
	{
		response = gmpc_tools_ipc_send(ipc, COMMAND_EASYCOMMAND, "quit");
		if(!response) printf("Failed to send EASYCOMMAND command\n");
	}
    for ( i=1; i < argc ; i++ ) {
		response = gmpc_tools_ipc_send(ipc, COMMAND_EASYCOMMAND, argv[i]);
		if(!response) printf("Failed to send EASYCOMMAND command\n");
    }
	g_object_unref(ipc);
	return EXIT_SUCCESS;
}
