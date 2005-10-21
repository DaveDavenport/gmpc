#include <stdio.h>
#include <string.h>
#include <glib.h>
#include "../../src/plugin.h"
#include "../../src/strfsong.h"
#include "../../src/misc.h"
#include <xosd.h>

GtkWidget *osd_vbox = NULL;
xosd *osd = NULL;

void osd_song_changed(MpdObj *mi, int old_song, int new_song);
void osd_state_changed(MpdObj *mi, int old_state, int new_state);

void osd_construct(GtkWidget *container);
void osd_destroy(GtkWidget *container);

gmpcPrefPlugin osd_gpp = {
	osd_construct,
	osd_destroy
};



/* set the signals I want */
gmpcMpdSignals osd_gms = {
	NULL,
	osd_song_changed,
	NULL,
	osd_state_changed
};
/* main plugin_osd info */
gmpcPlugin plugin_osd = {
	"osd plugin_osd",
	{0,0,1},
	GMPC_PLUGIN_NO_GUI,
	0,
	NULL,
	&osd_gms,
	&osd_gpp
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
	mpd_Song *song = NULL;
	if(!cfg_get_single_value_as_int_with_default(config, "osd-plugin", "enable", 0))
	{
		return;
	}
	if(osd == NULL) osd_init();
	song = mpd_playlist_get_current_song(connection);
	if(song)
	{
		char buffer[128];
		strfsong(buffer, 128, "[%artist% - %title% ]|[%name%]|[%shortfile%]", song);
		xosd_display (osd, 0, XOSD_string,buffer);
	}
}

void osd_state_changed(MpdObj *mi, int old_state, int new_state)
{
	if(!cfg_get_single_value_as_int_with_default(config, "osd-plugin", "enable", 0))
	{                                                                                   	
		return;                                                                     	
	}                                                                                   	
	
	if(osd == NULL) osd_init();
	if(new_state == MPD_STATUS_STATE_PLAY)
	{
		osd_song_changed(mi,1,1);
	}

}

void osd_enable_toggle(GtkWidget *wid)
{
	int kk = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(wid));
	cfg_set_single_value_as_int(config, "osd-plugin", "enable", kk);
}
void osd_destroy(GtkWidget *container)
{
	gtk_container_remove(GTK_CONTAINER(container), osd_vbox);
}

void osd_construct(GtkWidget *container)
{
	GtkWidget *enable_cg = gtk_check_button_new_with_mnemonic("_Enable OSD");
	osd_vbox = gtk_vbox_new(FALSE,6);

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(enable_cg), 	
			cfg_get_single_value_as_int_with_default(config, "osd-plugin", "enable", 0));


	g_signal_connect(G_OBJECT(enable_cg), "toggled", G_CALLBACK(osd_enable_toggle), NULL);
	gtk_box_pack_start(GTK_BOX(osd_vbox), gtk_label_new("OSD Plugin"), FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(osd_vbox), enable_cg, FALSE, FALSE, 0);
	gtk_container_add(GTK_CONTAINER(container), osd_vbox);
	gtk_widget_show_all(container);
}
