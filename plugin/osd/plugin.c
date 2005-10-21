#include <stdio.h>
#include <string.h>
#include <glib.h>
#include "../../src/plugin.h"
#include "../../src/strfsong.h"
#include "../../src/misc.h"
#include <xosd.h>


xosd *osd = NULL;

void osd_song_changed(MpdObj *mi, int old_song, int new_song);
void osd_state_changed(MpdObj *mi, int old_state, int new_state);

/* set the signals I want */
gmpcMpdSignals gms = {
	NULL,
	osd_song_changed,
	NULL,
	osd_state_changed
};
/* main plugin info */
gmpcPlugin plugin = {
	"osd plugin",
	{0,0,1},
	GMPC_PLUGIN_NO_GUI,
	0,
	NULL,
	&gms
};

void osd_init()
{
	char *string;
	osd = xosd_create(1);
	string= cfg_get_single_value_as_string_with_default(config, "osd-plugin", "colour", "LawnGreen");
	xosd_set_colour(osd,string);
	cfg_free_string(string);
	xosd_set_timeout(osd, cfg_get_single_value_as_int_with_default(config, "osd-plugin", "timeout", 3));
	xosd_set_shadow_offset(osd, cfg_get_single_value_as_int_with_default(config, "osd-plugin", "shadowoffset", 2));
	xosd_set_pos(osd, cfg_get_single_value_as_int_with_default(config, "osd-plugin", "position", 1));
	string = cfg_get_single_value_as_string_with_default(config, "osd-plugin", "font", " -*-lucidatypewriter-bold-*-*-*-*-240-*-*-*-*-*-*");
	xosd_set_font(osd, string);
	cfg_free_string(string);
}


void osd_song_changed(MpdObj *mi, int old_song, int new_song)
{
	char *string = NULL;
	mpd_Song *song = NULL;
	if(osd == NULL) osd_init();
	song = mpd_playlist_get_current_song(connection);
	if(song)
	{
		char buffer[128];
		strfsong(buffer, 128, "[%artist% - %title%]|[%name%]|[%shortfile%]", song);
		xosd_display (osd, 0, XOSD_string, buffer);
	}
}

void osd_state_changed(MpdObj *mi, int old_state, int new_state)
{
	if(osd == NULL) osd_init();
	if(new_state == MPD_STATUS_STATE_PLAY)
	{
		osd_song_changed(mi,1,1);
	}

}
