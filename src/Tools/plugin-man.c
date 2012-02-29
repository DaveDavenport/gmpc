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

/* @todo this needs to be globally available. find a good solution to where to
 *       hide this one
 */
GmpcBrowsersMetadata *browsers_metadata = NULL;

void plugin_manager_load_internal_plugins(void)
{
    /** file browser */
    plugin_add(&file_browser_plug, 0, NULL);
    /** current playlist */
    plugin_add_new((GmpcPluginBase *)
            play_queue_plugin_new("current-pl"),
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
            GMPC_PLUGIN_BASE((browsers_metadata = gmpc_browsers_metadata_new())),
            0, NULL);

    plugin_add_new(
            GMPC_PLUGIN_BASE(gmpc_tools_metadata_prefetcher_new()),
            0, NULL);

    plugin_add_new(
            GMPC_PLUGIN_BASE(gmpc_tools_database_update_tracker_new()),
            0, NULL);

    plugin_add_new(
            GMPC_PLUGIN_BASE(gmpc_browsers_nowplaying_new()),
            0, NULL);

    plugin_add_new(
            GMPC_PLUGIN_BASE(gmpc_tools_metadata_appearance_new()),
            0, NULL);


    /* Initialize the message system */
    plugin_add_new(GMPC_PLUGIN_BASE(pl3_messages), 0, NULL);
    /** Provider */
    plugin_add_new((GmpcPluginBase *) 
            gmpc_plugins_auto_mpd_new(),
            0, NULL);

    plugin_add_new((GmpcPluginBase *)
            gmpc_plugins_sidebar_search_new(),
            0, NULL);

    plugin_add_new((GmpcPluginBase *)
            gmpc_plugins_sidebar_next_song_new(),
            0, NULL);
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
    for (i = 0; i < num_plugins; i++)
    {
        g_log(LOG_DOMAIN,
                G_LOG_LEVEL_DEBUG,
                "Connection changed plugin: %s\n",
                gmpc_plugin_get_name(plugins[i]));

        gmpc_plugin_mpd_connection_changed(plugins[i], mi, connected, NULL);
    }
}

void plugin_manager_status_changed(MpdObj *mi, const ChangedStatusType what)
{
    int i = 0;
    for (i = 0; i < num_plugins; i++)
    {
        g_log(LOG_DOMAIN,
                G_LOG_LEVEL_DEBUG,
                "Status changed plugin: %s\n",
                gmpc_plugin_get_name(plugins[i]));
        gmpc_plugin_status_changed(plugins[i], mi, what);
    }
}

/* \todo change this away from ~/.config/*/
static void plugin_manager_load_userspace_plugins(void)
{
	char *url = gmpc_get_user_path("plugins");
    /**
     * if dir exists, try to load the plugins.
     */
    if (g_file_test(url, G_FILE_TEST_IS_DIR))
    {
        g_log(LOG_DOMAIN, 
				G_LOG_LEVEL_DEBUG,
				"Trying to load plugins in: %s", url);
  		plugin_load_dir(url);
    }
    g_free(url);
}
static void plugin_manager_load_env_path(void)
{
	/* Load plugin from $PLUGIN_DIR if set */
    if (g_getenv("PLUGIN_DIR") != NULL)
    {
        gchar *path = g_build_filename(g_getenv("PLUGIN_DIR"), NULL);
        if (path && g_file_test(path, G_FILE_TEST_IS_DIR))
        {
            plugin_load_dir(path);
        }
        if (path)
            g_free(path);
    }
}
static void plugin_manager_load_global_plugins(void)
{
	gchar *url = NULL;
#ifdef WIN32
    gchar *packdir = g_win32_get_package_installation_directory_of_module(NULL);
    g_log(LOG_DOMAIN,
	    G_LOG_LEVEL_DEBUG,
		"Got %s as package installation dir", packdir);
    url = g_build_filename(packdir, "lib", "gmpc", "plugins", NULL);
    g_free(packdir);

    plugin_load_dir(url);
    g_free(url);
#else
    /* This is the right location to load gmpc plugins */
    url = g_build_path(G_DIR_SEPARATOR_S, PACKAGE_LIB_DIR, "plugins", NULL);
    plugin_load_dir(url);
    g_free(url);
#endif
}

void plugin_manager_load_plugins(void)
{
	plugin_manager_load_global_plugins();
	plugin_manager_load_env_path();
	plugin_manager_load_userspace_plugins();
}

/* vim: set noexpandtab ts=4 sw=4 sts=4 tw=80: */
