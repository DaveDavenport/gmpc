#ifndef __PLAYLIST3_FILE_BROWSER_H__
#define __PLAYLIST3_FILE_BROWSER_H__

void pl3_file_browser_add();
void pl3_file_browser_unselected();
void pl3_file_browser_selected();
void pl3_file_browser_cat_sel_changed(GtkTreeView *tree,GtkTreeIter *iter);
void pl3_file_browser_fill_tree(GtkTreeIter *iter);
int pl3_file_browser_cat_popup(GtkWidget *menu, int type,GtkTreeView *tree, GdkEventButton *event);
void pl3_file_browser_cat_key_press(GdkEventKey *event);
/* testing */
void pl3_file_browser_reupdate();



void pl3_file_browser_disconnect();

int pl3_file_browser_add_go_menu(GtkWidget *menu);


#endif
