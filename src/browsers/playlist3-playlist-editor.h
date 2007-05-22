#ifndef __FAVORITES_H__
#define __FAVORITES_H__

#define FAV_CONFIG "playlist-plugin"

void playlist_editor_init(void);
void playlist_editor_destroy(void);
/* connection changed callback */
void playlist_editor_conn_changed(MpdObj *mi, int connect, void *userdata);
/* browser */
void playlist_editor_browser_add(GtkWidget *cat_tree);
void playlist_editor_browser_selected(GtkWidget *container);
void playlist_editor_browser_unselected(GtkWidget *container);
void playlist_editor_browser_changed(GtkWidget *tree, GtkTreeIter *iter);
void playlist_editor_browser_fill_list(void);
int playlist_editor_browser_cat_menu(GtkWidget *menu, int type, GtkWidget *tree, GdkEventButton *event);
void playlist_editor_browser_row_activated(GtkTreeView *tree, GtkTreePath *path, GtkTreeViewColumn *col, gpointer data);
/* mw intergration */
int playlist_editor_go_menu(GtkWidget *menu);
int playlist_editor_key_press(GtkWidget *mw, GdkEventKey *event, int type);
/**
 */
void playlist_editor_add_current_song(void);
void playlist_editor_save(void);
void playlist_editor_create_playlist(void);
void playlist_editor_clear(void);

void playlist_editor_right_mouse(GtkWidget *menu, void (*add_to_playlist)(GtkWidget *menu));
#endif
