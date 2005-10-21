#ifndef _TAG_BROWSER_H_
#define _TAG_BROWSER_H_

void pl3_tag_browser_selected(GtkWidget *container);
void pl3_tag_browser_unselected(GtkWidget *container);
void pl3_custom_tag_browser_add();
void pl3_custom_tag_browser_fill_tree(GtkTreeIter *iter);
long unsigned pl3_custom_tag_browser_view_folder(GtkTreeIter *iter_cat);
void pl3_custom_tag_browser_category_selection_changed(GtkTreeView *tree,GtkTreeIter *iter);





void pl3_custom_tag_browser_right_mouse_menu(GdkEventButton *event);




#endif
