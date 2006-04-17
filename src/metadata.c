#include "main.h"
#include "metadata.h"
#include "cover-art.h"
typedef struct {
	mpd_Song *song;
	MetaDataCallback callback;
	MetaDataType type;
	gpointer data;

} meta_thread_data;
void meta_data_recieved_image(mpd_Song *song, meta_thread_data *data)
{
	char *path = NULL;
	printf("bg fetch done\n");
	MetaDataResult ret = meta_data_get_path(song, data->type, &path);
	data->callback(song,(ret==META_DATA_AVAILABLE)?META_DATA_AVAILABLE:META_DATA_UNAVAILABLE,path, data->data);
	mpd_freeSong(data->song);
	g_free(data);
}

void meta_data_get_path_callback(mpd_Song *song, MetaDataType type, MetaDataCallback callback, gpointer data)
{
	char *path = NULL;
	MetaDataResult ret = meta_data_get_path(song, type, &path);
	if(ret == META_DATA_NEED_FETCH)
	{
		meta_thread_data *mtd = g_malloc0(sizeof(*mtd));
		mtd->song = mpd_songDup(song);
		mtd->callback = callback;
		mtd->data = data;
		mtd->type = type;
		/* TODO: do fetch here, and make sure the callback gets called after the fetch */
		
		cover_art_fetch_image(song, (CoverArtCallback)meta_data_recieved_image, mtd);
		return;
	}
	callback(song, ret, path, data);
}

MetaDataResult meta_data_get_path(mpd_Song *song, MetaDataType type, char **path)
{
   int retval = META_DATA_UNAVAILABLE;
   if(!song || *path)
   {
	  return retval;
   }
   switch(type)	
   {
	  case META_ALBUM_ART:
		 {
			int val = cover_art_fetch_image_path(song, path);
			if(val == COVER_ART_OK_LOCAL)
			{
			   retval = META_DATA_AVAILABLE;
			}
			else if(val == COVER_ART_NOT_FETCHED)
			{
			   retval = META_DATA_NEED_FETCH;
			}
			else
			{
			   if(*path) g_free(*path);
			}
			break;
		 }
	  default:
		 break;
   }
   return retval;
}
