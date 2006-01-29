#ifndef __PLAYER_H__
#define __PLAYER_H__

#define SEEK_STEP 3

void player_destroy();
void player_show();
void player_hide();
int player_get_hidden();
int update_player();
void player_create();
void player_mpd_state_changed(MpdObj *mi, ChangedStatusType what, void *userdata);


gboolean update_msg();
int msg_pop_popup();
void msg_push_popup();
void msg_set_base();

void player_connection_changed(MpdObj *mi, int connect);
#endif
