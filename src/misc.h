#ifndef __MISC_H__
#define __MISC_H__

#define RANGE(l,u,v) (v<l)?l:((v>u)?u:v)
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



#endif
