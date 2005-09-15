#ifndef __PLAYLIST3_FILE_BROWSER_H__
#define __PLAYLIST3_FILE_BROWSER_H__

void pl3_browser_file_update_folder();
void pl3_browser_file_add_folder();
void pl3_browser_file_add();
void pl3_browser_file_cat_sel_changed(GtkTreeView *tree,GtkTreeIter *iter);
long unsigned pl3_browser_file_view_folder(GtkTreeIter *iter_cat);
void pl3_browser_file_fill_tree(GtkTreeIter *iter);
void pl3_browser_file_cat_popup(GtkTreeView *tree, GdkEventButton *event);
void pl3_browser_file_cat_key_press(GdkEventKey *event);
int pl3_browser_file_playlist_key_press(GdkEventKey *event);
void pl3_browser_file_show_info(GtkTreeIter *iter);
void pl3_file_browser_row_activated(GtkTreeView *tree, GtkTreePath *tp);
#endif
