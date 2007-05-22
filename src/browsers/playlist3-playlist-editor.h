#ifndef __FAVORITES_H__
#define __FAVORITES_H__

#define FAV_CONFIG "playlist-plugin"

void playlist_init(void);
void playlist_destroy(void);
/* connection changed callback */
void playlist_conn_changed(MpdObj *mi, int connect, void *userdata);
/* browser */
void playlist_browser_add(GtkWidget *cat_tree);
void playlist_browser_selected(GtkWidget *container);
void playlist_browser_unselected(GtkWidget *container);
void playlist_browser_changed(GtkWidget *tree, GtkTreeIter *iter);
void playlist_browser_fill_list(void);
int playlist_browser_cat_menu(GtkWidget *menu, int type, GtkWidget *tree, GdkEventButton *event);
void playlist_browser_row_activated(GtkTreeView *tree, GtkTreePath *path, GtkTreeViewColumn *col, gpointer data);
/* mw intergration */
int playlist_go_menu(GtkWidget *menu);
int playlist_key_press(GtkWidget *mw, GdkEventKey *event, int type);
/**
 */
void playlist_add_current_song(void);
void playlist_save(void);
void playlist_create_playlist(void);
void playlist_clear(void);
#endif
