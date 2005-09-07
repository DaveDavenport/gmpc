#ifndef __PLAYLIST3_FILE_BROWSER_H__
#define __PLAYLIST3_FILE_BROWSER_H__

void pl3_browse_file_update_folder();
void pl3_browse_file_add_folder();
void pl3_browse_file_add();
long unsigned pl3_browse_file_view_folder(GtkTreeIter *iter_cat);
void pl3_browse_file_fill_tree(GtkTreeIter *iter);
void pl3_browse_file_cat_popup(GtkTreeView *tree, GdkEventButton *event);
void pl3_browse_file_cat_key_press(GdkEventKey *event);
#endif
