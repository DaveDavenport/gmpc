#ifndef  __PLAYLIST3_CURRENT_PLAYLIST_BROWSER_H__
#define  __PLAYLIST3_CURRENT_PLAYLIST_BROWSER_H__

void pl3_current_playlist_browser_scroll_to_current_song();
void pl3_current_playlist_browser_add();

void pl3_current_playlist_browser_playlist_popup(GtkTreeView *tree, GdkEventButton *event);

void pl3_current_playlist_browser_playlist_changed();
void pl3_current_playlist_browser_selected();
void pl3_current_playlist_browser_unselected();

int pl3_current_playlist_browser_cat_menu_popup(GtkWidget *menu, int type, GtkTreeView *tree, GdkEventButton *event);
#endif
