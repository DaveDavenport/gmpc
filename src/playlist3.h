#ifndef __PLAYLIST3_H__
#define __PLAYLIST3_H__

#include <libmpd/libmpd.h>

extern GtkTreeModel *pl3_tree;
extern GtkListStore *pl3_store;


void pl3_update_go_menu(void);
void pl3_show_window(void);
void pl3_toggle_hidden(void);
void create_playlist3(void);
void pl3_push_statusbar_message(char *mesg);
void pl3_push_rsb_message(gchar *string);
void pl3_cat_sel_changed(void);
int pl3_hide(void);
int pl3_window_key_press_event(GtkWidget *mw, GdkEventKey *event);
int pl3_cat_get_selected_browser(void);
void pl3_updating_changed(MpdObj *mi, int updating);
void playlist_connection_changed(MpdObj *mi, int connect);
gboolean playlist3_get_active(void);

typedef enum {
	PLAYLIST_NO_ZOOM,
	PLAYLIST_SMALL,
	PLAYLIST_MINI,
	PLAYLIST_ZOOM_LEVELS
}PlaylistZoom;

void playlist3_destroy(void);
void playlist_editor_fill_list(void);

gboolean playlist3_show_playtime(gulong playtime);
#endif
