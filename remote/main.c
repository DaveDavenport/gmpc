/* Gnome Music Player (GMPC)
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <gtk/gtk.h>
#include <unique/unique.h>
#include "gmpc_unique.h"

int main ( int argc, char **argv )
{
    UniqueApp *unique;

    gtk_init(&argc, &argv);


    unique = unique_app_new("nl.Sarine.gmpc", NULL);

    if(unique)
    {
        if (unique_app_is_running(unique))
        {
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

            unique_app_add_command(unique,  "present",  COMMAND_PRESENT);
            unique_app_add_command(unique,  "quit",  COMMAND_QUIT); 
            unique_app_add_command(unique,  "play",  COMMAND_PLAYER_PLAY);
            unique_app_add_command(unique,  "stop",  COMMAND_PLAYER_STOP);
            unique_app_add_command(unique,  "next",  COMMAND_PLAYER_NEXT);
            unique_app_add_command(unique,  "prev",  COMMAND_PLAYER_PREV);
            unique_app_add_command(unique,  "pause",  COMMAND_PLAYER_PAUSE);

            unique_app_add_command(unique,  "viewtoggle",   COMMAND_VIEW_TOGGLE);
            unique_app_add_command(unique,  "viewshow",     COMMAND_VIEW_SHOW);
            unique_app_add_command(unique,  "viewprev",     COMMAND_VIEW_HIDE);


            unique_app_add_command(unique,  "addstream",    COMMAND_PLAYLIST_ADD_STREAM);
            /*Create the commandline option parser */
            context = g_option_context_new("GMPC remote program");
            g_option_context_add_main_entries(context, entries, NULL);

            /*Time to parse the options */
            g_option_context_parse(context, &argc, &argv, &error);
            g_option_context_free(context);

            if(quit)
            {
                printf("send quit\n");
                unique_app_send_message(unique, COMMAND_QUIT, NULL);
            }
            if(play || pause)
            {
                printf("send play\n");
                unique_app_send_message(unique, COMMAND_PLAYER_PLAY, NULL);
            }
            if(prev)
            {
                printf("send prev\n");
                unique_app_send_message(unique, COMMAND_PLAYER_PREV, NULL);
            }
            if(next)
            {
                printf("send next\n");
                unique_app_send_message(unique, COMMAND_PLAYER_NEXT, NULL);
            }
            if(stop)
            {
                printf("send stop\n");
                unique_app_send_message(unique, COMMAND_PLAYER_STOP, NULL);
            }
            if(toggle_view)
            {
                printf("send toggle view\n");
                unique_app_send_message(unique, COMMAND_VIEW_TOGGLE, NULL);
            }
            if(hide_view)
            {
                printf("send hide view\n");
                unique_app_send_message(unique, COMMAND_VIEW_HIDE, NULL);
            }
            if(show_view)
            {
                printf("send show view\n");
                unique_app_send_message(unique, COMMAND_VIEW_SHOW, NULL);
            }
            if(stream)
            {
                UniqueMessageData *umd = unique_message_data_new();
                gchar **uris = g_malloc0(2*sizeof(gchar *));
                printf("Send stream: %s\n", stream);
                uris[0] = stream;

                unique_message_data_set_uris(umd, uris); 
                unique_app_send_message(unique, COMMAND_PLAYLIST_ADD_STREAM, umd);

                unique_message_data_free(umd);

                uris[0] = NULL;
                g_free(uris);
            }


        }
        else {
            printf("GMPC is not running\n");
            g_object_unref(unique);
            return EXIT_FAILURE;
        }
        g_object_unref(unique);
        return EXIT_SUCCESS;
    }
    
    return EXIT_FAILURE;
}
