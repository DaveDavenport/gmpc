/* Gnome Music Player Client (GMPC)
 * Copyright (C) 2004-2011 Qball Cow <qball@sarine.nl>
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <strings.h>
#include <config.h>

#include "main.h"
#include "plugin-man.h"
#include "internal-plugins.h"

/* Internal plugins definition */
#include "browsers/playlist3-current-playlist-browser.h"
#include "browsers/playlist3-file-browser.h"
#include "browsers/playlist3-find2-browser.h"
#include "browsers/playlist3-playlist-editor.h"
#include "browsers/playlist3-tag2-browser.h"
#ifdef ENABLE_MMKEYS
#include "mm-keys.h"
#endif

#define LOG_DOMAIN "Gmpc.Plugin.Manager"

/* \todo this needs to be globally available. find a good solution to where to
 *       hide this one
 */
GmpcMetadataBrowser *metadata_browser = NULL;

void plugin_manager_load_internal_plugins(void)
{
    /** file browser */
    plugin_add(&file_browser_plug, 0, NULL);
    /** current playlist */
    plugin_add_new((GmpcPluginBase *)
            play_queue_plugin_new("current-pl"),
            0, NULL);
    plugin_add_new((GmpcPluginBase *) 
            gmpc_provider_music_tree_new(),
            0, NULL);
    /* Add it to the plugin command */
    plugin_add_new(GMPC_PLUGIN_BASE(gmpc_easy_command), 0, NULL);
    /** Find Browser */
    plugin_add(&find2_browser_plug, 0, NULL);
    /* this shows the connection preferences */
    plugin_add(&connection_plug, 0, NULL);
    /* this the server preferences */
    plugin_add(&server_plug, 0, NULL);
    /* this shows the playlist preferences */
    plugin_add(&playlist_plug, 0, NULL);
    /* this shows the markup stuff */
    plugin_add(&tag2_plug, 0, NULL);
    #ifdef ENABLE_MMKEYS
    plugin_add(&mmkeys_plug, 0, NULL);
    #endif
    /* the tray icon */
    plugin_add(&tray_icon2_plug, 0, NULL);

    /* Playlist editor */
    plugin_add(&playlist_editor_plugin, 0, NULL);

    plugin_add(&statistics_plugin, 0, NULL);
    plugin_add(&metadata_plug, 0, NULL);
    plugin_add(&proxyplug, 0, NULL);

    plugin_add(&extraplaylist_plugin, 0, NULL);
    plugin_add_new(
            GMPC_PLUGIN_BASE((metadata_browser = gmpc_metadata_browser_new())),
            0, NULL);

    plugin_add_new(
            GMPC_PLUGIN_BASE(gmpc_plugin_metadata_prefetcher_new()),
            0, NULL);

    plugin_add_new(
            GMPC_PLUGIN_BASE(gmpc_plugin_database_update_tracker_new()),
            0, NULL);

    plugin_add_new(
            GMPC_PLUGIN_BASE(gmpc_plugin_mockup_new()),
            0, NULL);


    /* Initialize the message system */
    plugin_add_new(GMPC_PLUGIN_BASE(pl3_messages), 0, NULL);
}

void plugin_manager_initialize_plugins(void)
{
    int i = 0;
    INIT_TIC_TAC();

    for (i = 0; i < num_plugins && plugins[i] != NULL; i++)
    {
        gmpc_plugin_init(plugins[i]);
        TEC("Initializing plugin: %s", gmpc_plugin_get_name(plugins[i]));
        g_log(LOG_DOMAIN, 
                G_LOG_LEVEL_DEBUG, 
                "Initializing '%s'", 
                gmpc_plugin_get_name(plugins[i]));
    }

}

void plugin_manager_destroy_plugins(void)
{
    int i = 0;
    INIT_TIC_TAC();
    /* time todo some destruction of plugins */
    for (; i < num_plugins && plugins[i] != NULL; i++)
    {
        TEC("Destroying plugin: %s", gmpc_plugin_get_name(plugins[i]));
        g_log(LOG_DOMAIN, 
                G_LOG_LEVEL_DEBUG, 
                "Telling '%s' to destroy itself", 
                gmpc_plugin_get_name(plugins[i]));
        gmpc_plugin_destroy(plugins[i]);
    }
}

void plugin_manager_save_state(void)
{
    int i = 0;
    INIT_TIC_TAC();
    for (i = 0; i < num_plugins && plugins[i] != NULL; i++)
    {
        g_log(LOG_DOMAIN,
            G_LOG_LEVEL_DEBUG, "Telling '%s' to save itself",
            gmpc_plugin_get_name(plugins[i]));
        gmpc_plugin_save_yourself(plugins[i]);
        TEC("Saving state: %s", gmpc_plugin_get_name(plugins[i]));
    }


}


void plugin_manager_connection_changed(MpdObj *mi, const int connected)
{
    int i = 0;
    INIT_TIC_TAC();
    for (i = 0; i < num_plugins; i++)
    {
        g_log(LOG_DOMAIN,
                G_LOG_LEVEL_DEBUG,
                "Connection changed plugin: %s\n",
                gmpc_plugin_get_name(plugins[i]));

        gmpc_plugin_mpd_connection_changed(plugins[i], mi, connected, NULL);
        TEC("Connection changed plugin: %s", gmpc_plugin_get_name(plugins[i]));
    }
}

void plugin_manager_status_changed(MpdObj *mi, const ChangedStatusType what)
{
    int i = 0;
    INIT_TIC_TAC();
    for (i = 0; i < num_plugins; i++)
    {
        g_log(LOG_DOMAIN,
                G_LOG_LEVEL_DEBUG,
                "Status changed plugin: %s\n",
                gmpc_plugin_get_name(plugins[i]));
        gmpc_plugin_status_changed(plugins[i], mi, what);
        TEC("Status changed plugin: %s", gmpc_plugin_get_name(plugins[i]));
    }
}
