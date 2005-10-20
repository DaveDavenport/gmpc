#ifndef __PLAYLIST_FIND_BROWSER_H__
#define __PLAYLIST_FIND_BROWSER_H__

void pl3_find_browser_category_selection_changed(GtkTreeView *tree, GtkTreeIter *iter);
void pl3_find_browser_selected(GtkWidget *container);
void pl3_find_browser_unselected(GtkWidget *container);
void pl3_find_browser_playlist_changed();



void pl3_find_browser_add();
void pl3_find_browser_search_playlist();



#endif
