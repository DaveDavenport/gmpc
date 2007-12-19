#ifndef __MISC_H__
#define __MISC_H__

#define RANGE(l,u,v) (((v)<(l))?(l):(((v)<(u))?(v):(u)))
/**
 * format time into 
 * Total time: %i days %i hours %i minutes
 */

gchar * format_time(unsigned long seconds);
gchar * format_time_real(unsigned long seconds, const gchar *data);
/**
 * this draws a 1 pixel border around a pixbuf.
 * It doesn't work for all color depths (I think)
 */
void mpd_song_markup_escaped(char *buffer, int size, char *markup, mpd_Song *song);


void screenshot_add_shadow (GdkPixbuf **src);
void screenshot_add_border (GdkPixbuf **src);

gchar *escape_single_quotes (const gchar * string);

void open_uri(const gchar *uri);
int *split_version(const char *uri);

MpdData * misc_sort_mpddata(MpdData *data, GCompareDataFunc func, void *user_data);
MpdData * misc_sort_mpddata_by_album_disc_track(MpdData *data);
MpdData *misc_mpddata_remove_duplicate_songs(MpdData *data);


gchar ** tokenize_string(const gchar *string);
#endif
