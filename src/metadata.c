#include "main.h"
#include "metadata.h"
#include "cover-art.h"

MetaDataResult meta_data_get_path(mpd_Song *song, MetaDataType type, char **path);

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
	if(path) g_free(path);
}

void meta_data_get_path_callback(mpd_Song *song, MetaDataType type, MetaDataCallback callback, gpointer data)
{
	char *path = NULL;
	MetaDataResult ret = meta_data_get_path(song, type, &path);
	if(ret == META_DATA_FETCHING)
	{
		meta_thread_data *mtd = g_malloc0(sizeof(*mtd));
		mtd->song = mpd_songDup(song);
		mtd->callback = callback;
		mtd->data = data;
		mtd->type = type;
		cover_art_fetch_image(song, (CoverArtCallback)meta_data_recieved_image, mtd);
	}
	callback(song, ret, path, data);
	if(path)g_free(path);
}

/**
 * Sorting the plugins on priority.
 * The higher the number the more important
 */
static void meta_data_sort_plugins()
{


}

MetaDataResult meta_data_get_path(mpd_Song *song, MetaDataType type, char **path)
{
   int retval = META_DATA_UNAVAILABLE;
   /* Check if there is actually something to search for. */
   if(!song || *path)
   {
	  return retval;
   }
   /**
	* Query Cache.
	* If found, return path.
	*/


   /**
	* Sort the plugins on priority.
	* Only do this on priority change/adding of plugins?
	* if sorted it shouldn't take to much time.
	*/

	meta_data_sort_plugins();


	/** 
	 * Walk through every plugin and see if it has the metadata is availible.
	 * There are 3 possible returns for a plugin:
	 * 1. Cover Art Available.
	 * 2. No Cover Art Available. (f.e. not supported)
	 * 3. Needs to fetch information.
	 * In case 3. we don't directly want to run that (do we?), it returns NEED_FETCHING.
	 * This will trigger the process to be threaded and executed.
	 */
   
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
			   retval = META_DATA_FETCHING;
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


   /**
	*  Update Cache
	*  Save it or it isn't found.
	*/
   return retval;
}
