#include <stdio.h>
#include <string.h>
#include <glib.h>
#include "../../src/plugin.h"
#include "../../src/strfsong.h"
#include "../../src/misc.h"
#include <xosd.h>

GtkWidget *osd_vbox = NULL;
xosd *osd = NULL;

void osd_mpd_status_changed(MpdObj *mi, ChangedStatusType what, void *data);
void osd_construct(GtkWidget *container);
void osd_destroy(GtkWidget *container);

gmpcPrefPlugin osd_gpp = {
	osd_construct,
	osd_destroy
};


/* main plugin_osd info */
gmpcPlugin plugin = {
	"osd plugin",
	{0,0,1},
	GMPC_PLUGIN_NO_GUI,
	0,
	NULL,
	&osd_mpd_status_changed,
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


void osd_song_changed(MpdObj *mi)
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

void osd_state_changed(MpdObj *mi)
{
	if(!cfg_get_single_value_as_int_with_default(config, "osd-plugin", "enable", 0))
	{                                                                                   	
		return;                                                                     	
	}                                                                                   	
	if(osd == NULL) osd_init();
	if(mpd_player_get_state(connection) == MPD_STATUS_STATE_PLAY)
	{
		osd_song_changed(mi);
	}
}

void osd_enable_toggle(GtkWidget *wid)
{
	int kk = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(wid));
	cfg_set_single_value_as_int(config, "osd-plugin", "enable", kk);
}
void osd_spin_value_changed(GtkWidget *wid)
{
	int kk = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(wid));
	cfg_set_single_value_as_int(config, "osd-plugin", "timeout", kk);
	xosd_set_timeout(osd,kk);
}







void osd_destroy(GtkWidget *container)
{
	gtk_container_remove(GTK_CONTAINER(container), osd_vbox);
}
void osd_color_set(GtkColorButton *colorbut)
{
	GdkColor color;
	gchar *string = NULL;
	gtk_color_button_get_color(colorbut, &color);
	string = g_strdup_printf("#%02x%02x%02x", color.red>>8, color.green>>8, color.blue>>8);
	cfg_set_single_value_as_string(config, "osd-plugin", "colour", string);
	xosd_set_colour(osd,string);
	g_free(string);
}

void osd_construct(GtkWidget *container)
{
	GtkWidget *enable_cg = gtk_check_button_new_with_mnemonic("_Enable OSD");
	GtkWidget *label = NULL;
	GtkWidget *wid2 = NULL;
	gchar *string = NULL;
	GdkColor color;
	osd_vbox = gtk_vbox_new(FALSE,6);
	if(osd == NULL) osd_init();
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(enable_cg), 	
			cfg_get_single_value_as_int_with_default(config, "osd-plugin", "enable", 0));

	g_signal_connect(G_OBJECT(enable_cg), "toggled", G_CALLBACK(osd_enable_toggle), NULL);
	gtk_box_pack_start(GTK_BOX(osd_vbox), enable_cg, FALSE, FALSE, 0);

	/* timeout */
	wid2 = gtk_hbox_new(FALSE,6);
	gtk_box_pack_start(GTK_BOX(osd_vbox), wid2,FALSE, FALSE, 0);
	label = gtk_label_new("Timeout:"); 
	gtk_box_pack_start(GTK_BOX(wid2), label,FALSE, FALSE, 0);
	label = gtk_spin_button_new_with_range(1.0,10.0,1.0); 
	gtk_box_pack_start(GTK_BOX(wid2), label,FALSE, FALSE, 0);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(label),(gdouble)cfg_get_single_value_as_int_with_default(config, "osd-plugin","timeout",3));
	g_signal_connect(G_OBJECT(label), "value-changed", G_CALLBACK(osd_spin_value_changed), NULL);

	/* Color Selector */
	wid2 = gtk_hbox_new(FALSE,6);
	gtk_box_pack_start(GTK_BOX(osd_vbox), wid2,FALSE, FALSE, 0);
	label = gtk_label_new("Colour:");        
	gtk_box_pack_start(GTK_BOX(wid2), label,FALSE, FALSE, 0);
	string = cfg_get_single_value_as_string_with_default(config, "osd-plugin", "colour", "LawnGreen");
	gdk_color_parse(string,&color);
	cfg_free_string(string);

	label = gtk_color_button_new_with_color(&color);
	gtk_box_pack_start(GTK_BOX(wid2), label,FALSE, FALSE, 0);
	g_signal_connect(G_OBJECT(label), "color-set", G_CALLBACK(osd_color_set), NULL);
	
	


	gtk_container_add(GTK_CONTAINER(container), osd_vbox);
	gtk_widget_show_all(container);
}



/* mpd changed */

void   osd_mpd_status_changed(MpdObj *mi, ChangedStatusType what, void *data)
{
	if(what&MPD_CST_SONGID)
	{
		osd_song_changed(mi);
	}
	if(what&MPD_CST_STATE)
	{
		osd_state_changed(mi);
	}
}
