#ifndef __ADVANCED_SEARCH_H__
#define __ADVANCED_SEARCH_H__


void advanced_search_init(void);
void advanced_search_update_taglist(void);
void advanced_search_destroy(void);
MpdData *advanced_search(const gchar *query, int playlist);

#endif
