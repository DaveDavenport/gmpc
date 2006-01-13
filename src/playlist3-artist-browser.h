#ifndef __PLAYLIST_ARTIST_BROWSER_H__
#define __PLAYLIST_ARTIST_BROWSER_H__

void pl3_artist_browser_add();
void pl3_artist_browser_fill_tree(GtkTreeIter *iter);
void pl3_artist_browser_category_selection_changed(GtkTreeView *tree,GtkTreeIter *iter);
void pl3_artist_browser_selected(GtkWidget *container);
void pl3_artist_browser_unselected(GtkWidget *container);
int pl3_artist_browser_cat_popup(GtkWidget *menu, int type,GtkTreeView *tree, GdkEventButton *event);
void pl3_artist_browser_category_key_press(GdkEventKey *event);


void pl3_artist_browser_disconnect();

#endif
