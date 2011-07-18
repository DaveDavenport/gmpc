/* Gnome Music Player Client (GMPC)
 * Copyright (C) 2004-2011 Qball Cow <qball@gmpclient.org>
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

#ifndef __MAIN_H__

#define __MAIN_H__
#include <config.h>
#include <gmpc-version.h>
#ifdef ENABLE_NLS
#ifndef __G_I18N_LIB_H__
#include <glib/gi18n.h>
#endif
#endif

#include "config-defaults.h"
#include <libmpd/libmpd.h>
#include <libmpd/libmpdclient.h>
#include "config1.h"
#include "plugin.h"
#include "plugin-internal.h"
#include "gmpc-extras.h"
#include "mpdinteraction.h"

#include "playlist3-messages.h" 


/**
 * Some gobjects
 */
#include "gmpc-profiles.h"
#include "smclient/eggsmclient.h"

extern int 				gmpc_connected;
extern GtkTreeModel 	*playlist;

/* the plugin list */
extern gmpcPluginParent **plugins;
/* num of plugins */
extern int num_plugins;

char * edit_song_markup(char *format);

void main_quit(void);

/* plugin */
void plugin_load_dir(const gchar *path);
void plugin_add(gmpcPlugin *plug, int plugin, GError **error);
void plugin_add_new(GmpcPluginBase *plug, int plugin, GError **error);
int plugin_get_pos(int id);

void show_error_message(const gchar *string);


/**
 * Metadata 
 */

void meta_data_add_plugin(gmpcPluginParent *plug);
/**
 * TODO move this 
 */
void url_start_real(const gchar *url);
void url_start_easy_command(void *data,char *param, void *d );
void url_start(void);

void url_start_custom(const gchar *url, 
	void (*error_callback)(const gchar *error_msg, gpointer user_data),
	void (*result_callback)(GList *result,gpointer user_data),
	void (*progress_callback)(gdouble progress, gpointer user_data),
	gpointer user_data);
/*
 * functions to get patch to different files.
 * This is needed to make the windows port work.
 * (misc.c)
 */
char *gmpc_get_full_image_path(void);

/**
 * Help functions
 */
#define q_free(a) {g_free(a);a=NULL;}

/* tray stuff */
gboolean tray_icon2_get_available(void);
void tray_icon2_create_tooltip(void);
void tray_icon2_update_menu(void);

/* tag stuff */

void mpd_interaction_update_supported_tags(void);
/**
 * Playlist functions 
 */

/* Check if the playlist is fullscreen, this queries the _actual_ state */
gboolean pl3_window_is_fullscreen(void);
/* easy download */
void gmpc_easy_async_quit(void);

/**Hack Handle status changed */
void   GmpcStatusChangedCallback(MpdObj *mi, ChangedStatusType what, void *userdata);


#ifdef DEBUG_TIMING
/* Tic Tac system */
#define TIMER_SUB(start,stop,diff)  diff.tv_usec = stop.tv_usec - start.tv_usec;\
        diff.tv_sec = stop.tv_sec - start.tv_sec;\
        if(diff.tv_usec < 0) {\
            diff.tv_sec -= 1; \
            diff.tv_usec += G_USEC_PER_SEC; \
        }

#define INIT_TIC_TAC() GTimeVal start123, stop123,diff123;\
    g_get_current_time(&start123);

#define TAC(a,ARGS...) g_get_current_time(&stop123);\
    TIMER_SUB(start123, stop123, diff123);\
    printf("%lu.%06lu:%s: "a"\n", (unsigned long)( diff123.tv_sec),(unsigned long)( diff123.tv_usec),__FUNCTION__,##ARGS);    

#define TOC(a,ARGS...) TAC(a,##ARGS);\
    start123 = stop123;

#else // DEBUG_TIMING


#define INIT_TIC_TAC() ;
#define TAC(a, ARGS...) ;
#define TOC(a, ARGS...) ;

#endif // DEBUG_TIMING
#define TEC(a, ARGS...) TAC(a, ##ARGS)

#endif
