#ifndef __MISC_H__
#define __MISC_H__
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
#endif
