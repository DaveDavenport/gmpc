#ifndef __PIXBUF_CACHE_H__
#define __PIXBUF_CACHE_H__

/**
 * Destroy the pixbuf cache.
 *
 * Removes all entries from the cache.
 */
void pixbuf_cache_destroy(void);

/**
 * Create the pixbuf cache. This needs to be called before add_iocn,lookup_icon, invalidate and destroy
 */
void pixbuf_cache_create(void);

/**
 * @param size the size of the icon.
 * @param url the url off the icon.
 * @param pb the pixbuf.
 * 
 * Add an icon to the pixbuf cache 
 */
void pixbuf_cache_add_icon(int size,const gchar *url, GdkPixbuf *pb);
/**
 * @param size the size of the icon 
 * @param url off the icon.
 *
 * Lookup an icon in the pixbuf cache.
 *
 * @return If found returns the pixbuf. (no reference added)
 */
GdkPixbuf *pixbuf_cache_lookup_icon(int size, const gchar *url);
/**
 * @param url the pixbuf location to update. 
 *
 * Invalidate entries in the cache
 */
void pixbuf_cache_invalidate_pixbuf_entry(const gchar *url);
#endif
