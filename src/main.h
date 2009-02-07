/* Gnome Music Player Client (GMPC)
 * Copyright (C) 2004-2009 Qball Cow <qball@sarine.nl>
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

#ifndef __MAIN_H__

#define __MAIN_H__
#include <config.h>

#ifdef ENABLE_NLS
#include <glib/gi18n.h>
#endif

#include "config-defaults.h"
#include <libmpd/libmpd.h>
#include <libmpd/libmpdclient.h>
#include <libmpd/debug_printf.h>
#include "config1.h"
#include "plugin.h"
#include "mpdinteraction.h"

#include "playlist3-messages.h" 


/**
 * Some gobjects
 */
#include "gob/gmpc-profiles.h"
#include "gob/gmpc-connection.h"
#include "gob/gmpc-signals.h"

#include "vala/gmpc-easy-command.h"

extern GmpcEasyCommand *gmpc_easy_command;
extern GmpcConnection 	*gmpcconn;
extern GmpcSignals 		*gmpc_signals;
extern int 				gmpc_connected;
extern GtkTreeModel 	*playlist;

/* the plugin list */
extern gmpcPlugin **plugins;
/* num of plugins */
extern int num_plugins;

char * edit_song_markup(char *format);

void main_quit(void);

/* plugin */
void plugin_load_dir(gchar *path);
void plugin_add(gmpcPlugin *plug, int plugin);
int plugin_get_pos(int id);

void show_error_message(const gchar *string,const int block);

/** Handle status changed */
void   GmpcStatusChangedCallback(MpdObj *mi, ChangedStatusType what, void *userdata);

/**
 * Metadata 
 */

void meta_data_add_plugin(gmpcPlugin *plug);
/**
 * TODO move this 
 */
void url_start_real(const gchar *url);
void url_start(void);

/*
 * functions to get patch to different files.
 * This is needed to make the windows port work.
 * (misc.c)
 */
char *gmpc_get_full_image_path(const char *filename);

/**
 * Help functions
 */
#define q_free(a) {g_free(a);a=NULL;}

/* help */
void info2_activate(void);
void info2_fill_artist_view(const char *);
void info2_fill_album_view(const char *,const char *);
void info2_fill_song_view(mpd_Song *);
/* tray stuff */
gboolean tray_icon2_get_available(void);
void tray_icon2_create_tooltip(void);

/**
 * Playlist functions 
 */

/* Check if the playlist is fullscreen, this queries the _actual_ state */
gboolean pl3_window_is_fullscreen(void);
/* easy download */
void gmpc_easy_async_quit(void);

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
    printf(a": %lu s, %lu us\n",##ARGS, (unsigned long)( diff123.tv_sec),(unsigned long)( diff123.tv_usec));    

#define TOC(a,ARGS...) TAC(a,##ARGS);\
    start123 = stop123;

#else // DEBUG_TIMING


#define INIT_TIC_TAC() ;
#define TAC(a, ARGS...) ;
#define TOC(a, ARGS...) ;

#endif // DEBUG_TIMING
#define TEC(a, ARGS...) TAC(a, ##ARGS)

#endif
