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
#include <glib.h>
#include <gtk/gtk.h>
#include <config.h>
#include <plugin.h>
#include "main.h"
#include "log.h"
#include "options.h"

/* Set default values */
Options settings = 
{
	.show_version = FALSE,
	.disable_plugins = FALSE,
	.start_hidden = FALSE,
	.clean_config = FALSE,
	.quit = FALSE,
	.show_bug_information = FALSE,
	.fullscreen = FALSE,
	.config_path = NULL,
	.debug_level = -1,
	.profile_name = NULL,
	.icon_theme = NULL
};
gboolean parse_options(int *argc, char ***argv)
{
    GError *error = NULL;
    GOptionContext *context = NULL;

    GOptionEntry entries[] =
    {
        {
            "fullscreen", 0, 0, G_OPTION_ARG_NONE,
            &(settings.fullscreen), N_("Start the program in full screen"), NULL
        },
        {
            "version", 'v', 0, G_OPTION_ARG_NONE,
            &(settings.show_version), N_("Show program version and revision"), NULL
        },
        {
            "quit", 'q', 0, G_OPTION_ARG_NONE,
            &(settings.quit), N_("Quits the running gmpc"), NULL
        },
        {
            "disable-plugins", 0, 0, G_OPTION_ARG_NONE,
            &(settings.disable_plugins), N_("Don't load the plugins"), NULL
        },
        {
            "config", 0, 0, G_OPTION_ARG_FILENAME,
            &(settings.config_path), N_("Load alternative config file"), "Path"
        },
        {
            "debug-level", 'd', 0, G_OPTION_ARG_INT,
            &(settings.debug_level), N_("Set the debug level"), "level"
        },
        {
            "start-hidden", 'h', 0, G_OPTION_ARG_NONE,
            &(settings.start_hidden), N_("Start gmpc hidden to tray"), NULL
        },
        {
            "clean-cover-db", 0, 0, G_OPTION_ARG_NONE,
            &(settings.clean_config), N_("Remove all failed hits from metadata cache"), NULL
        },
        {
            "bug-information", 'b', 0, G_OPTION_ARG_NONE,
            &(settings.show_bug_information), N_("Show bug information"), NULL
        },
        {
            "log-filter", 'f', 0, G_OPTION_ARG_CALLBACK,
            log_add_filter, N_("Shows all output from a certain log domain"), "<Log domain>"
        },
        {
            "profile", 'p', 0, G_OPTION_ARG_STRING,
            &(settings.profile_name), N_("Select a profile"), "<Profile Name>"
        },
        {
            "icon-theme", 'i', 0, G_OPTION_ARG_STRING,
            &(settings.icon_theme), N_("Run GMPC with a different icon theme"), "<icon theme name>"
        },

        {NULL}
    };
	INIT_TIC_TAC();
    context = g_option_context_new(_("Gnome Music Player Client"));
	TEC("context new")
    g_option_context_add_main_entries(context, entries, "gmpc");
	TEC(" add main context")
    g_option_context_add_group(context, gtk_get_option_group(TRUE));
	TEC("Add gtk option group")
    g_option_context_parse(context, argc, argv, &error);
	TEC("Parse option group")
    g_option_context_free(context);
    if (error)
    {
        g_log(NULL, G_LOG_LEVEL_ERROR, "Failed to parse commandline options: %s", error->message);
        g_error_free(error);
        return FALSE;
    }
    return TRUE;
}
