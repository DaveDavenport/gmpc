#ifndef __MAIN_H__

#define __MAIN_H__


#ifdef ENABLE_NLS
#include <libintl.h>
#include <locale.h>
#define _(String) gettext (String)
#  ifdef gettext_noop
#    define N_(String) gettext_noop (String)
#  else
#    define N_(String) (String)
#  endif
#else 
#define _(String) String
#endif

#define SEEK_STEP 3


#include "config-defaults.h"
#include <libmpd/libmpd.h>
#include <libmpd/libmpdclient.h>
#include <libmpd/debug_printf.h>
#include <glade/glade.h>
#include "plugin.h"

extern gmpcPlugin **plugins;
extern int num_plugins;


/** main.c **/
extern GladeXML *xml_main_window;
extern MpdObj *connection;
typedef struct
{
	/* playlist number this is to check if the playlist changed */
	int playlist_id;
	int playlist_length;
	int playlist_playtime;
	/* the current song */
	int old_pos;
	/* updating */
	gboolean updating;
	/* misc*/
	gboolean hidden;
	gboolean sb_hidden;
} internal_data;

enum{
	TIME_FORMAT_ELAPSED,
	TIME_FORMAT_REMAINING,
	TIME_FORMAT_PERCENTAGE
};

extern internal_data info;
extern guint update_timeout;
int update_interface();

/* callback.c */
/*int load_playlist();*/
int check_for_errors();

/* mpdinteraction.c*/
int update_mpd_status();
int connect_to_mpd();
int play_song();
int stop_song();
int next_song();
int prev_song();
void random_pl();
void repeat_pl();
void update_mpd_dbase();
int disconnect_to_mpd();
void disconnect_callback(MpdObj *mi);

/* returns FALSE when connected */
gboolean check_connection_state();


/* player.c*/
/* scrolling title in main window */
typedef struct 	{	
	char *msg;
	char *base_msg;
	char *popup_msg;
	GQueue *queue;
	int pos;
	int up;
	gboolean do_scroll;
	gboolean exposed;
} scrollname;

extern scrollname scroll;
int update_player();
void create_player();
gboolean update_msg();
void msg_pop_popup();
void msg_push_popup();
void msg_set_base();
void player_song_changed(int oldsong, int newsong);
void player_state_changed(int old_state, int state);
void create_preferences_window();
void preferences_update();

/* id3info.c*/
void call_id3_window(int song);

/* do tray */
void update_tray_icon();
int create_tray_icon();
void tray_icon_song_change();
void tray_icon_state_change();
/* popup.c: update_popup() */
void destroy_tray_icon();

/* main.h*/
void main_trigger_update();


int  seek_ns(int n);
int  seek_ps(int n);

void pl3_highlight_song_change ();
char * edit_song_markup(char *format);
void pl3_reinitialize_tree();

gboolean playlist_filter_func(GtkTreeModel *model, GtkTreeIter *iter);
void id3_status_update();
void call_id3_window_song(mpd_Song *songstr);



void playlist_changed(MpdObj *mi, int old_playlist_id, int new_playlist_id);
void init_playlist ();
void error_callback(MpdObj *mi, int error_id, char *error_msg, gpointer data);
void song_changed(MpdObj *mi, int oldsong, int newsong);
void state_callback(MpdObj *mi, int old_state, int new_state, gpointer data);
void status_callback(MpdObj *mi);
void connect_callback();
void database_changed();
void main_quit();


/* plugin */
void load_plugins_from_dir(gchar *path);
#endif
