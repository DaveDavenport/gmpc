#ifndef __PLAYLIST_FIND_BROWSER_H__
#define __PLAYLIST_FIND_BROWSER_H__


void pl3_find_browser_selected(GtkWidget *container);
void pl3_find_browser_unselected(GtkWidget *container);










void pl3_find_browser_add();
unsigned long pl3_find_browser_view_browser();
void pl3_find_browser_entry_change(GtkEntry *entry);
void pl3_find_browser_search();

void pl3_find_browser_category_selection_changed(GtkTreeView *tree, GtkTreeIter *iter);
#endif
