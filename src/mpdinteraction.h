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

#endif
