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
	"On Screen Display",
	{0,0,3},
	GMPC_PLUGIN_NO_GUI,
	0,
	NULL,
	&osd_mpd_status_changed,
	&osd_gpp
};

void osd_init()
{
	char *string;
	osd = xosd_create(2);
	string= cfg_get_single_value_as_string_with_default(config, "osd-plugin", "colour", "LawnGreen");
	xosd_set_colour(osd,string);
	cfg_free_string(string);
	xosd_set_timeout(osd, cfg_get_single_value_as_int_with_default(config, "osd-plugin", "timeout", 3));
	xosd_set_shadow_offset(osd, cfg_get_single_value_as_int_with_default(config, "osd-plugin", "shadowoffset", 2));
	xosd_set_pos(osd, cfg_get_single_value_as_int_with_default(config, "osd-plugin", "position", 1));
	xosd_set_horizontal_offset(osd, cfg_get_single_value_as_int_with_default(config, "osd-plugin", "x-offset", 0));
	xosd_set_vertical_offset(osd, cfg_get_single_value_as_int_with_default(config, "osd-plugin", "y-offset", 0));
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
		if(mpd_player_get_state(connection) == MPD_STATUS_STATE_PAUSE)
		{
			xosd_display (osd, 0, XOSD_string,"Paused:");
		}
		else if(mpd_player_get_state(connection) == MPD_STATUS_STATE_PLAY)
		{
			xosd_display (osd, 0, XOSD_string,"Playing:");
		}
		else if(mpd_player_get_state(connection) == MPD_STATUS_STATE_STOP)
		{
			xosd_display (osd, 0, XOSD_string,"Stopped:");
		}                                                                 		
		else
		{
			xosd_display (osd, 0, XOSD_string,"");
		}
		xosd_display (osd, 1, XOSD_string,buffer);
	}
}



void osd_enable_toggle(GtkWidget *wid,GtkWidget *table)
{
	int kk = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(wid));
	cfg_set_single_value_as_int(config, "osd-plugin", "enable", kk);
	gtk_widget_set_sensitive(table, 
			cfg_get_single_value_as_int_with_default(config, "osd-plugin", "enable", 0));

}
void osd_spin_value_changed(GtkWidget *wid)
{
	int kk = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(wid));
	cfg_set_single_value_as_int(config, "osd-plugin", "timeout", kk);
	xosd_set_timeout(osd,kk);
}


void osd_x_offset_changed(GtkWidget *wid)
{
	int kk = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(wid));
	cfg_set_single_value_as_int(config, "osd-plugin", "x-offset", kk);
	xosd_set_horizontal_offset(osd,kk);
}

void osd_y_offset_changed(GtkWidget *wid)
{
	int kk = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(wid));
	cfg_set_single_value_as_int(config, "osd-plugin", "y-offset", kk);
	xosd_set_vertical_offset(osd,kk);
}
void osd_test_settings()
{
	xosd_display(osd, 0, XOSD_string,"Gnome Music Player Client");
	xosd_display(osd, 1,XOSD_string, "Test string");
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
	GtkWidget *table = NULL;
	gchar *string = NULL;
	GdkColor color;
	osd_vbox = gtk_vbox_new(FALSE,6);
	if(osd == NULL) osd_init();
	/* table */
	table = gtk_table_new(5,2,FALSE);


	/* enable/disable */
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(enable_cg), 	
			cfg_get_single_value_as_int_with_default(config, "osd-plugin", "enable", 0));
	gtk_widget_set_sensitive(table, 
			cfg_get_single_value_as_int_with_default(config, "osd-plugin", "enable", 0));
	g_signal_connect(G_OBJECT(enable_cg), "toggled", G_CALLBACK(osd_enable_toggle), table);
	gtk_box_pack_start(GTK_BOX(osd_vbox), enable_cg, FALSE, FALSE, 0);





	gtk_table_set_col_spacings(GTK_TABLE(table), 6);
	gtk_table_set_row_spacings(GTK_TABLE(table), 6);

	gtk_box_pack_start(GTK_BOX(osd_vbox), table, FALSE, FALSE,0);

	/* timeout */
	label = gtk_label_new("Timeout:"); 
	gtk_misc_set_alignment(GTK_MISC(label), 1,0.5);
	gtk_table_attach(GTK_TABLE(table), label, 0,1,0,1,GTK_FILL|GTK_SHRINK,GTK_FILL|GTK_SHRINK,0,0);	
	label = gtk_spin_button_new_with_range(1.0,10.0,1.0); 
	gtk_table_attach_defaults(GTK_TABLE(table), label, 1,2,0,1);	
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(label),(gdouble)cfg_get_single_value_as_int_with_default(config, "osd-plugin","timeout",3));
	g_signal_connect(G_OBJECT(label), "value-changed", G_CALLBACK(osd_spin_value_changed), NULL);

	/* Color Selector */
	label = gtk_label_new("Color:");        
	gtk_misc_set_alignment(GTK_MISC(label), 1,0.5);
	gtk_table_attach(GTK_TABLE(table), label, 0,1,1,2,GTK_FILL|GTK_SHRINK,GTK_FILL|GTK_SHRINK,0,0);	
	string = cfg_get_single_value_as_string_with_default(config, "osd-plugin", "colour", "LawnGreen");
	gdk_color_parse(string,&color);
	cfg_free_string(string);

	label = gtk_color_button_new_with_color(&color);
	gtk_table_attach_defaults(GTK_TABLE(table), label, 1,2,1,2);	
	g_signal_connect(G_OBJECT(label), "color-set", G_CALLBACK(osd_color_set), NULL);



	/* position selector */
	/* TODO*/







	
	/* x-offset */
	label = gtk_label_new("X-Offset:"); 
	gtk_misc_set_alignment(GTK_MISC(label), 1,0.5);
	gtk_table_attach(GTK_TABLE(table), label, 0,1,2,3,GTK_FILL|GTK_SHRINK,GTK_FILL|GTK_SHRINK,0,0);	
	label = gtk_spin_button_new_with_range(0.0,100.0,1.0); 
	gtk_table_attach_defaults(GTK_TABLE(table), label, 1,2,2,3);	
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(label),(gdouble)cfg_get_single_value_as_int_with_default(config, "osd-plugin","x-offset",0));
	g_signal_connect(G_OBJECT(label), "value-changed", G_CALLBACK(osd_x_offset_changed), NULL);

	/* y-offset */
	label = gtk_label_new("Y-Offset:"); 
	gtk_misc_set_alignment(GTK_MISC(label), 1,0.5);
	gtk_table_attach(GTK_TABLE(table), label, 0,1,3,4,GTK_FILL|GTK_SHRINK,GTK_FILL|GTK_SHRINK,0,0);	
	label = gtk_spin_button_new_with_range(0.0,100.0,1.0); 
	gtk_table_attach_defaults(GTK_TABLE(table), label, 1,2,3,4);	
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(label),(gdouble)cfg_get_single_value_as_int_with_default(config, "osd-plugin","y-offset",0));
	g_signal_connect(G_OBJECT(label), "value-changed", G_CALLBACK(osd_y_offset_changed), NULL);


	label = gtk_button_new_with_label("Test Settings");
	g_signal_connect(G_OBJECT(label), "clicked", G_CALLBACK(osd_test_settings), NULL);
	wid2 = gtk_alignment_new(1,0.5, 0,0);
	gtk_container_add(GTK_CONTAINER(wid2), label);
	gtk_table_attach(GTK_TABLE(table), wid2, 1,2,4,5,GTK_SHRINK|GTK_FILL,GTK_FILL|GTK_SHRINK,0,0);	


	gtk_container_add(GTK_CONTAINER(container), osd_vbox);
	gtk_widget_show_all(container);
}



/* mpd changed */

void   osd_mpd_status_changed(MpdObj *mi, ChangedStatusType what, void *data)
{
	if(what&(MPD_CST_SONGID|MPD_CST_STATE))
	{
		osd_song_changed(mi);
		return;
	}
	/* don't do this on song change */
	if(what&MPD_CST_VOLUME)
	{
		xosd_display(osd, 0,XOSD_string,"Volume:");
		xosd_display(osd, 1,XOSD_slider,mpd_status_get_volume(mi));
	}
	if(what&MPD_CST_REPEAT)
	{
		xosd_display(osd, 0,XOSD_string,"Repeat:");
		xosd_display(osd, 1,XOSD_string,(mpd_player_get_repeat(mi))?"On":"Off");
	}
	if(what&MPD_CST_RANDOM)
	{
		xosd_display(osd, 0,XOSD_string,"Random:");
		xosd_display(osd, 1,XOSD_string,(mpd_player_get_random(mi))?"On":"Off");
	}
	
}
