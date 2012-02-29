/* Gnome Music Player Client (GMPC)
 * Copyright (C) 2004-2012 Qball Cow <qball@gmpclient.org>
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

#include <glib.h>
#include <glib/gstdio.h>
#include <libmpd/debug_printf.h>

#include "log.h"

#define LOG_DOMAIN "Gmpc.Log"


static GLogLevelFlags global_log_level = G_LOG_LEVEL_MESSAGE;

static void gmpc_log_func(
            const gchar * log_domain,
            GLogLevelFlags log_level,
            const gchar * message,
            gpointer user_data)
{
    if (log_level <= global_log_level)
    {
        g_log_default_handler(log_domain, log_level, message, user_data);
    }
}


gboolean log_add_filter(
            const gchar * option_name,
            const gchar * value,
            gpointer data,
            GError ** error)
{
    if (value == NULL || value[0] == 0)
    {
        g_set_error(error, 0, 0, "--log-filter requires a log domain as argument");
        return FALSE;
    }

    g_log_set_handler(
        value,
        G_LOG_LEVEL_MASK | G_LOG_FLAG_FATAL | G_LOG_FLAG_RECURSION,
        g_log_default_handler,
        NULL);
    return TRUE;
}

void log_init(void)
{

    g_log_set_default_handler(gmpc_log_func, NULL);

}

void log_set_debug_level(int debug_level)
{
    /**
     * Set debug level, options are
     * 0 = No debug
     * 1 = Error messages
     * 2 = Error + Warning messages
     * 3 = All messages
     */
    if (debug_level >= 0)
    {
        if (debug_level == 3)
        {
            global_log_level = G_LOG_LEVEL_DEBUG;
        } else if (debug_level == 2)
        {
            global_log_level = G_LOG_LEVEL_INFO;
        }
        debug_set_level(debug_level);
    }
}
