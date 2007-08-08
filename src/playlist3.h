#ifndef __PLAYLIST3_H__
#define __PLAYLIST3_H__

#include <libmpd/libmpd.h>

extern GtkTreeStore *pl3_tree;
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



enum PlaylistColums {
	PL_COLUMN_ICON,
	PL_COLUMN_MARKUP,
	PL_COLUMN_ARTIST,
	PL_COLUMN_TRACK,
	PL_COLUMN_TITLEFILE,
	PL_COLUMN_FILEBASENAME,
	PL_COLUMN_ALBUM,	
	PL_COLUMN_GENRE,
	PL_COLUMN_COMPOSER,
	PL_COLUMN_PERFORMER,
	PL_COLUMN_LENGTH,
	PL_COLUMN_DISC,
	PL_COLUMN_COMMENT,
	PL_COLUMN_DATE,
	PL_COLUMN_SONGPOS,
	PL_COLUMN_TOTAL
};


#endif
