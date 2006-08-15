#ifndef __MISC_H__
#define __MISC_H__

#define RANGE(l,u,v) (v<l)?l:((v>u)?u:v)
/**
 * format time into 
 * Total time: %i days %i hours %i minutes
 */

gchar * format_time(unsigned long seconds);

/**
 * this draws a 1 pixel border around a pixbuf.
 * It doesn't work for all color depths (I think)
 */
void draw_pixbuf_border(GdkPixbuf *pb);
void mpd_song_markup_escaped(char *buffer, int size, char *markup, mpd_Song *song);
#endif
