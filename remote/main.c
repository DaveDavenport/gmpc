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
#include "bacon-message-connection.h"

int main ( int argc, char **argv )
{
    BaconMessageConnection *bacon_connection = NULL;



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
    gboolean spawn = FALSE;
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
            "Send an easy command query to the running gmpc.", NULL
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
        {
            "spawn",    's', 0, G_OPTION_ARG_NONE, &spawn,
            "Spawn gmpc if not running", NULL
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
        g_error("Failed to parse command line options: '%s'", error->message);

    }
    bacon_connection = bacon_message_connection_new("gmpc");
    while(bacon_connection)
    {
        if (!bacon_message_connection_get_is_server (bacon_connection)) 
        {
            if(play || pause)
            {
                printf("send play\n");
                bacon_message_connection_send(bacon_connection, "PLAY");
            }
            if(prev)
            {
                printf("send prev\n");
                bacon_message_connection_send(bacon_connection, "PREV");
            }
            if(next)
            {
                printf("send next\n");
                bacon_message_connection_send(bacon_connection, "NEXT");
            }
            if(stop)
            {
                printf("send stop\n");
                bacon_message_connection_send(bacon_connection, "STOP");
            }
            if(toggle_view)
            {
                printf("send toggle view\n");
                bacon_message_connection_send(bacon_connection, "TOGGLE_VIEW");
            }
            if(hide_view)
            {
                printf("send hide view\n");
                bacon_message_connection_send(bacon_connection, "HIDE_VIEW");
            }
            if(show_view)
            {
                printf("send show view\n");
                bacon_message_connection_send(bacon_connection, "SHOW_VIEW");
            }
            if(stream)
            {
                gchar *str = g_strdup_printf("STREAM %s", stream);
                printf("Send stream: %s\n", stream);
                bacon_message_connection_send(bacon_connection, str);
                g_free(str);
            }
            if(easycommand)
            {
                gchar *str = g_strdup_printf("EASYCOMMAND %s", easycommand);
                printf("Send easycommand: %s\n", easycommand);
                bacon_message_connection_send(bacon_connection, str);
                g_free(str);
            }

            if(quit)
            {
                printf("send quit\n");
                bacon_message_connection_send(bacon_connection, "QUIT");
            }
            bacon_message_connection_free (bacon_connection);
            return EXIT_SUCCESS;
        }
        else if(spawn){
            int count = 10;
            printf("starting gmpc\n");

            bacon_message_connection_free (bacon_connection);
            bacon_connection = NULL;

            g_spawn_command_line_async("gmpc",&error);
            if(error){
                g_error("Failed to spawn gmpc: '%s'", error->message);
            }
            do{
                g_usleep(300000);
                printf("waiting for gmpc to come up\n");
                if(bacon_connection)
                    bacon_message_connection_free (bacon_connection);
                bacon_connection = bacon_message_connection_new("gmpc");
            }while(bacon_message_connection_get_is_server (bacon_connection) && (--count > 0));
            if(count == 0){
                g_error("Failed to get gmpc to respond");
            }
            /* Send connect */
            bacon_message_connection_send(bacon_connection, "CONNECT");
            g_usleep(300000);
        }else{ 
            printf("GMPC is not running\n");
            bacon_message_connection_free (bacon_connection);
            return EXIT_FAILURE;
        }
    }
    g_error("Failed to create IPC connection\n");
    return EXIT_FAILURE;
}
