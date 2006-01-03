#ifndef __COVERART_H__
#define __COVERART_H__
typedef void (*CoverArtCallback) (mpd_Song *song, gpointer userdata);
typedef enum _CoverArtResult{
	COVER_ART_NO_IMAGE 	= 1, /* There is no cover art image availible */
	COVER_ART_NOT_FETCHED 	= 2, /* The cover art image might be availible, but it isn't fetched yet */
	COVER_ART_OK_LOCAL 	= 3, /* The Cover art is availible, and it's a local image */
	COVER_ART_OK_REMOTE	= 4  /* The Cover art is availible on a local link */
} CoverArtResult;

CoverArtResult cover_art_fetch_image_path (mpd_Song *song,gchar **path);
void cover_art_fetch_image(mpd_Song *song, CoverArtCallback function, gpointer userdata);











#endif
