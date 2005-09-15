#ifndef __PLAYLIST_ARTIST_BROWSER_H__
#define __PLAYLIST_ARTIST_BROWSER_H__
void pl3_artist_browser_add();
long unsigned pl3_artist_browser_view_folder(GtkTreeIter *iter_cat);
void pl3_artist_browser_fill_tree(GtkTreeIter *iter);
void pl3_artist_browser_add_folder();
void pl3_artist_browser_replace_folder();
void pl3_artist_browser_category_key_press(GdkEventKey *event);
void pl3_artist_browser_show_info(GtkTreeIter *iter);
void pl3_artist_browser_row_activated(GtkTreeView *tree, GtkTreePath *tp);
#endif
