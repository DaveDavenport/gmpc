#ifndef __METADATA_H__
#define __METADATA_H__

typedef enum {
	META_ALBUM_ART = 1, 	/* Album Cover art 	*/
	META_ARTIST_ART = 2, 	/* Artist  image 	*/
	META_ALBUM_TXT = 4,	/* Album story 		*/
	META_ARTIST_TXT = 8, 	/* Artist biography 	*/
	META_SONG_TXT	= 16	/* Lyrics 		*/
}MetaDataType;

typedef enum {
	META_DATA_AVAILABLE,
	META_DATA_UNAVAILABLE,
	META_DATA_NEED_FETCH
} MetaDataResult;

typedef gboolean (*MetaDataCallback)(mpd_Song *song, MetaDataResult result, char *path, gpointer data);
MetaDataResult meta_data_get_path(mpd_Song *song, MetaDataType type, char **path);
void meta_data_get_path_callback(mpd_Song *song, MetaDataType type, MetaDataCallback callback, gpointer data);



#endif
