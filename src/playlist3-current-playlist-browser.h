#ifndef  __PLAYLIST3_CURRENT_PLAYLIST_BROWSER_H__
#define  __PLAYLIST3_CURRENT_PLAYLIST_BROWSER_H__

void pl3_current_playlist_browser_scroll_to_current_song();
void pl3_current_playlist_browser_add();
void pl3_current_playlist_browser_delete_selected_songs ();
void pl3_current_playlist_browser_crop_selected_songs();
void pl3_current_playlist_browser_playlist_popup(GtkTreeView *tree, GdkEventButton *event);
void pl3_current_playlist_browser_row_activated(GtkTreeView *tree, GtkTreePath *path, GtkTreeViewColumn *col);
void pl3_current_playlist_browser_show_info(GtkTreeView *tree, GtkTreeIter *iter);
void pl3_current_playlist_browser_playlist_changed();
void pl3_current_playlist_browser_category_popup(GtkTreeView *tree,GdkEventButton *event);
void pl3_current_playlist_browser_category_selection_changed(GtkTreeView *tree);
int  pl3_current_playlist_browser_button_press_event(GdkEventKey *event);
/* probly doesnt belong here */
void pl3_current_playlist_row_changed(GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter);

#endif
