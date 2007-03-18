#ifndef __MPDINTERACTION_H__
#define __MPDINTERACTION_H__
int play_song(void);
int stop_song(void);
int next_song(void);
int prev_song(void);
void random_pl(GtkToggleButton *tb);
void repeat_pl(GtkToggleButton *tb);
void random_toggle(void);
void repeat_toggle(void);

int  seek_ns(int n);
int  seek_ps(int n);
int connect_to_mpd(void);
void song_fastforward(void);
void song_fastbackward(void);
void volume_up(void);
void volume_down(void);
int update_mpd_status(void);

extern gmpcPlugin server_plug;
/**
 * Connection stuff 
 */
void connection_set_password(char *password);
int connection_use_auth(void);
char *connection_get_hostname(void);
int connection_get_port(void);
char *connection_get_password(void);
char *connection_get_current_profile(void);
void connection_set_current_profile(const char *uid);

/**
 * Helper functions
 */

void play_path(const gchar *path);
void add_artist(const gchar *artist);
void add_album(const gchar *artist,const gchar *album);
void add_genre(const gchar *genre);
void add_directory(const gchar *path);

/**
 * Helper menu functions *
 */
void submenu_for_song(GtkMenu *menu, mpd_Song *song);
#endif
