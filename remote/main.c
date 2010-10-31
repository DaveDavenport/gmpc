/* Gnome Music Player (GMPC)
 * Copyright (C) 2004-2010 Qball Cow <qball@sarine.nl>
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
#include <unique/unique.h>
/* Copied from main.h */
enum
{
	UNIQUE_COMMAND_0, /* unused: 0 is an invalid command */
	UNIQUE_COMMAND_QUIT,
	UNIQUE_COMMAND_PLAY,
	UNIQUE_COMMAND_PAUSE,
	UNIQUE_COMMAND_NEXT,
	UNIQUE_COMMAND_PREV,
	UNIQUE_COMMAND_STOP,
	UNIQUE_COMMAND_VIEW_TOGGLE,
	UNIQUE_COMMAND_VIEW_HIDE,
	UNIQUE_COMMAND_VIEW_SHOW,
	UNIQUE_COMMAND_STREAM,
	UNIQUE_COMMAND_CONNECT,
	UNIQUE_COMMAND_EASYCOMMAND
};

int main ( int argc, char **argv )
{
	UniqueApp *app = NULL;
    UniqueResponse response; /* the response to our command */

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


    app = unique_app_new_with_commands ("org.gmpclient.GMPC", NULL,
            "quit",			UNIQUE_COMMAND_QUIT,
            "play",			UNIQUE_COMMAND_PLAY,
            "pause",		UNIQUE_COMMAND_PAUSE,
            "next",			UNIQUE_COMMAND_NEXT,
            "prev",			UNIQUE_COMMAND_PREV,
            "stop",			UNIQUE_COMMAND_STOP,
            "view-toggle",	UNIQUE_COMMAND_VIEW_TOGGLE,
            "view-hide",	UNIQUE_COMMAND_VIEW_HIDE,
            "view-show",	UNIQUE_COMMAND_VIEW_SHOW,
            "stream",		UNIQUE_COMMAND_STREAM,
            "connect",		UNIQUE_COMMAND_CONNECT,
            "easycommand",	UNIQUE_COMMAND_EASYCOMMAND,
            NULL);
    if(!unique_app_is_running(app)) {
        printf("Error: GMPC is not running");
        g_object_unref(app);
        return EXIT_FAILURE;
    }

    


    if(play || pause)
    {
        response = unique_app_send_message(app, UNIQUE_COMMAND_PLAY, NULL);
        if(response  == UNIQUE_RESPONSE_FAIL) printf("Failed to send PLAY command\n");
    }
    if(prev)
    {
        response = unique_app_send_message(app, UNIQUE_COMMAND_PREV, NULL);
        if(response  == UNIQUE_RESPONSE_FAIL) printf("Failed to send PREV command\n");
    }
    if(next)
    {
        response = unique_app_send_message(app, UNIQUE_COMMAND_NEXT, NULL);
        if(response  == UNIQUE_RESPONSE_FAIL) printf("Failed to send NEXT command\n");
    }
    if(stop)
    {
        response = unique_app_send_message(app, UNIQUE_COMMAND_STOP, NULL);
        if(response  == UNIQUE_RESPONSE_FAIL) printf("Failed to send STOP command\n");
    }
    if(toggle_view)
    {
        response = unique_app_send_message(app, UNIQUE_COMMAND_VIEW_TOGGLE, NULL);
        if(response  == UNIQUE_RESPONSE_FAIL) printf("Failed to send VIEW_TOGGLE command\n");
    }
    if(hide_view)
    {
        response = unique_app_send_message(app, UNIQUE_COMMAND_VIEW_HIDE, NULL);
        if(response  == UNIQUE_RESPONSE_FAIL) printf("Failed to send VIEW_HIDE command\n");
    }
    if(show_view)
    {
        response = unique_app_send_message(app, UNIQUE_COMMAND_VIEW_SHOW, NULL);
        if(response  == UNIQUE_RESPONSE_FAIL) printf("Failed to send VIEW_SHOW command\n");
    }
    if(stream)
    {
        UniqueMessageData *message; /* the payload for the command */

        message = unique_message_data_new();
        unique_message_data_set_filename(message, stream);

        response = unique_app_send_message(app, UNIQUE_COMMAND_STREAM, message);
        if(response  == UNIQUE_RESPONSE_FAIL) printf("Failed to send STREAM command\n");
        /* the message is copied, so we need to free it before returning */
        unique_message_data_free (message);
    }
    if(easycommand)
    {
        UniqueMessageData *message; /* the payload for the command */

        message = unique_message_data_new();
        unique_message_data_set_text(message, easycommand,-1);

        response = unique_app_send_message(app, UNIQUE_COMMAND_EASYCOMMAND, message);
        if(response  == UNIQUE_RESPONSE_FAIL) printf("Failed to send EASYCOMMAND command\n");
        /* the message is copied, so we need to free it before returning */
        unique_message_data_free (message);
    }
    if(quit)
    {
        response = unique_app_send_message(app, UNIQUE_COMMAND_QUIT, NULL);
        if(response  == UNIQUE_RESPONSE_FAIL) printf("Failed to send QUIT command\n");
    }
    g_object_unref(app);
    return EXIT_SUCCESS;
}
