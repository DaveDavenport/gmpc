#ifndef _TAG_BROWSER_H_
#define _TAG_BROWSER_H_

void pl3_custom_tag_browser_add_single(GtkTreeIter *piter, char *title, char *format);
void pl3_custom_tag_browser_list_add(GtkTreeIter *iter);
void pl3_custom_tag_browser_reload();
void pl3_custom_tag_browser_add();
void pl3_custom_tag_browser_fill_tree(GtkTreeIter *iter);
long unsigned pl3_custom_tag_browser_view_folder(GtkTreeIter *iter_cat);
void pl3_custom_tag_browser_right_mouse_menu(GdkEventButton *event);





#endif
