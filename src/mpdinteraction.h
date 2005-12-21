#ifndef __MPDINTERACTION_H__
#define __MPDINTERACTION_H__
int play_song();
int stop_song();
int next_song();
int prev_song();
void random_pl();
void repeat_pl();

int  seek_ns(int n);
int  seek_ps(int n);
int connect_to_mpd();

extern gmpcPlugin server_plug;

#endif
