#ifndef __PIXBUF_CACHE_H__
#define __PIXBUF_CACHE_H__


void pixbuf_cache_destroy(void);
void pixbuf_cache_create(void);

void pixbuf_cache_add_icon(int size,const gchar *url, GdkPixbuf *pb);
GdkPixbuf *pixbuf_cache_lookup_icon(int size, const gchar *url);
void pixbuf_cache_invalidate_pixbuf_entry(gchar *url);
#endif
