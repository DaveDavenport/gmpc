#ifndef __PIXBUF_CACHE_H__
#define __PIXBUF_CACHE_H__


typedef enum {
	COVER_SMALL,
	COVER_DEFAULT,
	COVER_LARGE,
	COVER_BROWSER,
	COVER_TOOLTIP,
	NUM_COVER_SIZES
}CoverSize;


int pixbuf_cache_get_closest_size(int size);
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
 * @param mdd the md5 off the icon (16 bytes).
 * @param pb the pixbuf.
 * 
 * Add an icon to the pixbuf cache 
 */
void pixbuf_cache_add_icon(int size,const gchar *mdd, GdkPixbuf *pb);
/**
 * @param size the size of the icon 
 * @param mdd the md5 off the icon (16 bytes).
 *
 * Lookup an icon in the pixbuf cache.
 *
 * @return If found returns the pixbuf. (no reference added)
 */
GdkPixbuf *pixbuf_cache_lookup_icon(int size, const gchar *mdd);

#endif
