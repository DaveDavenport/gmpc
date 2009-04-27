#ifndef __METADATA_CACHE_H__
#define __METADATA_CACHE_H__

MetaDataResult meta_data_get_from_cache(mpd_Song *song, MetaDataType type, MetaData **met);
mpd_Song *rewrite_mpd_song(mpd_Song *tsong, MetaDataType type);

void meta_data_set_cache_real(mpd_Song *song, MetaDataResult result, MetaData *met);

void meta_data_set_cache(mpd_Song *song, MetaDataResult result, MetaData *met);

void metadata_cache_init(void);

void metadata_cache_cleanup(void);
void metadata_cache_destroy(void);
#endif
