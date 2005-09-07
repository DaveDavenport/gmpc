#ifndef  __PLAYLIST3_CURRENT_PLAYLIST_BROWSER_H__
#define  __PLAYLIST3_CURRENT_PLAYLIST_BROWSER_H__

void pl3_browser_current_playlist_scroll_to_current_song();
void pl3_browser_current_playlist_add();
void pl3_browser_current_playlist_delete_selected_songs ();
void pl3_browser_current_playlist_crop_selected_songs();



/* probly doesnt belong here */
void pl3_current_playlist_row_changed(GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter);

#endif
