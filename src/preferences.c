#include <gtk/gtk.h>
#include <glade/glade.h>
#include <string.h>
#include "libmpdclient.h"
#include "main.h"

pref_struct preferences;
GladeXML *xml_preferences_window;
gboolean running = 0, connected = 0;
void popup_timeout_changed();
void tray_enable_toggled(GtkToggleButton *but);
void preferences_update();
void popup_enable_toggled(GtkToggleButton *but);
void popup_position_changed(GtkOptionMenu *om);
void update_auth_settings();
void preferences_window_connect(GtkWidget *but);
void preferences_window_disconnect(GtkWidget *but);
void update_popup_settings();
void update_tray_settings();
void update_server_settings();
void show_state_changed(GtkToggleButton *but);
void auth_enable_toggled(GtkToggleButton *but);
void entry_auth_changed(GtkEntry *entry);
void xfade_time_changed(GtkSpinButton *but);
void xfade_enable_toggled(GtkToggleButton *bug);

/* creat the preferences window */
void create_preferences_window()
	{
	GtkWidget *dialog;
	if(running)
	    {
	    if(xml_preferences_window == NULL){
		running = 0;
	    } else{
		dialog = glade_xml_get_widget(xml_preferences_window, "preferences_window");
		gtk_window_present(GTK_WINDOW(dialog));
		return;
	    }
	    }
   	 xml_preferences_window = glade_xml_new(GLADE_PATH"gmpc.glade", "preferences_window", NULL);
	 /* check for errors and axit when there is no gui file */
	 if(xml_preferences_window == NULL)  g_error("Couldnt initialize GUI. Please check installation\n");


	/* set info from struct */
	/* hostname */
	gtk_entry_set_text(GTK_ENTRY(glade_xml_get_widget(xml_preferences_window, "hostname_entry")), preferences.host);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(glade_xml_get_widget(xml_preferences_window, "port_spin")), preferences.port);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(glade_xml_get_widget(xml_preferences_window, "timeout_spin")), preferences.timeout);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(glade_xml_get_widget(xml_preferences_window, "ck_autocon")), preferences.autoconnect);
	dialog = glade_xml_get_widget(xml_preferences_window, "preferences_window");
	gtk_widget_show_all(GTK_WIDGET(dialog));
	running = 1;

	update_server_settings();
	
	/* set the right sensitive stuff */
	if(info.connection == NULL)
	    {
	    gtk_widget_set_sensitive(glade_xml_get_widget(xml_preferences_window, "bt_con"), TRUE);
	    gtk_widget_set_sensitive(glade_xml_get_widget(xml_preferences_window, "bt_dis"), FALSE);	    
	    }
	else
	    {
	    gtk_widget_set_sensitive(glade_xml_get_widget(xml_preferences_window, "bt_con"), FALSE);
	    gtk_widget_set_sensitive(glade_xml_get_widget(xml_preferences_window, "bt_dis"), TRUE);	    
	    }
	update_popup_settings();
	update_tray_settings();
	update_auth_settings();
	glade_xml_signal_autoconnect(xml_preferences_window);	
	
	}

/* destory the preferences window */
void preferences_window_destroy()
	{
	GtkWidget *dialog = glade_xml_get_widget(xml_preferences_window, "preferences_window");
		gtk_widget_destroy(dialog);
	g_object_unref(xml_preferences_window);
	xml_preferences_window = NULL;
	running = 0;
		}
		
void update_preferences_information()
	{
	strncpy(preferences.host, gtk_entry_get_text(GTK_ENTRY(glade_xml_get_widget(xml_preferences_window, "hostname_entry"))), 256);
	preferences.port = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(glade_xml_get_widget(xml_preferences_window, "port_spin")));
	preferences.timeout = gtk_spin_button_get_value_as_float(GTK_SPIN_BUTTON(glade_xml_get_widget(xml_preferences_window, "timeout_spin")));

	}

void preferences_window_autoconnect(GtkToggleButton *tog)
    {
    preferences.autoconnect = gtk_toggle_button_get_active(tog);
    
    }

void preferences_window_connect(GtkWidget *but)
    {
    if(debug)g_print("**DEBUG** connect\n");
    if(info.connection == NULL)
	if(!connect_to_mpd())
	    {
	    	info.conlock = FALSE;
	  	gtk_timeout_remove(update_timeout);
		update_timeout =  gtk_timeout_add(400, (GSourceFunc)update_interface, NULL);
	        }
    
    }

void preferences_window_disconnect(GtkWidget *but)
    {
    if(debug)g_print("**DEBUG** disconnect\n");    
    disconnect_to_mpd();
    
    }

/* this function is called from the main loop, it makes sure stuff is up-to-date(r) */
void preferences_update()
    {
    if(!running)return;
    if((info.connection == NULL? 0:1) != connected)
	{
	if(info.connection == NULL)
	    {
	    gtk_widget_set_sensitive(glade_xml_get_widget(xml_preferences_window, "bt_con"), TRUE);
	    gtk_widget_set_sensitive(glade_xml_get_widget(xml_preferences_window, "bt_dis"), FALSE);	    
	    gtk_widget_set_sensitive(glade_xml_get_widget(xml_preferences_window, "vb_server_set"), FALSE);
	    gtk_widget_show(glade_xml_get_widget(xml_preferences_window, "hb_warning_mesg"));
	    }
	else
	    {
	    gtk_widget_set_sensitive(glade_xml_get_widget(xml_preferences_window, "bt_con"), FALSE);
	    gtk_widget_set_sensitive(glade_xml_get_widget(xml_preferences_window, "bt_dis"), TRUE);	   
	    gtk_widget_set_sensitive(glade_xml_get_widget(xml_preferences_window, "vb_server_set"), TRUE);
	    gtk_widget_hide(glade_xml_get_widget(xml_preferences_window, "hb_warning_mesg"));
	    }
	connected = (info.connection == NULL? 0:1);
	}    
    }
   
void popup_enable_toggled(GtkToggleButton *but)
{
	info.popup.do_popup = gtk_toggle_button_get_active(but);
	update_popup_settings();
}

void popup_position_changed(GtkOptionMenu *om)
{
	info.popup.position = gtk_option_menu_get_history(om);
}
void show_state_changed(GtkToggleButton *but)
{
	info.popup.show_state = gtk_toggle_button_get_active(but);
}

void popup_timeout_changed()
    {
    info.popup.timeout = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(glade_xml_get_widget(xml_preferences_window, "popup_timeout")));
    }

void update_popup_settings()
{
	gtk_toggle_button_set_active((GtkToggleButton *)
			glade_xml_get_widget(xml_preferences_window, "ck_popup_enable"), info.popup.do_popup);
	gtk_widget_set_sensitive(glade_xml_get_widget(xml_preferences_window, "om_popup_position"), info.popup.do_popup);
	gtk_widget_set_sensitive(glade_xml_get_widget(xml_preferences_window, "ck_show_state"), info.popup.do_popup);
	gtk_toggle_button_set_active((GtkToggleButton *)
			glade_xml_get_widget(xml_preferences_window, "ck_show_state"), info.popup.show_state);	
	gtk_option_menu_set_history((GtkOptionMenu *)
			glade_xml_get_widget(xml_preferences_window, "om_popup_position"), info.popup.position);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(glade_xml_get_widget(xml_preferences_window, "popup_timeout")), info.popup.timeout);

}


void update_server_settings()
{
	if(info.connection == NULL)
	{
	    gtk_widget_set_sensitive(glade_xml_get_widget(xml_preferences_window, "vb_server_set"), FALSE);
	    gtk_widget_show(glade_xml_get_widget(xml_preferences_window, "hb_warning_mesg"));
	    return;
	}
	else 
	{
	    gtk_widget_set_sensitive(glade_xml_get_widget(xml_preferences_window, "vb_server_set"), TRUE);
	    gtk_widget_hide(glade_xml_get_widget(xml_preferences_window, "hb_warning_mesg"));
	}	
		
	if(info.status->crossfade == 0)
	{
		gtk_toggle_button_set_active((GtkToggleButton *)
				glade_xml_get_widget(xml_preferences_window, "cb_fading"), FALSE);
	gtk_widget_set_sensitive(glade_xml_get_widget(xml_preferences_window, "sb_fade_time"), FALSE);
	}
	else {
		gtk_toggle_button_set_active((GtkToggleButton *)
				glade_xml_get_widget(xml_preferences_window, "cb_fading"), TRUE);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(glade_xml_get_widget(xml_preferences_window, "sb_fade_time")), info.status->crossfade);
	gtk_widget_set_sensitive(glade_xml_get_widget(xml_preferences_window, "sb_fade_time"), TRUE);

	}
}

/* this sets all the settings in the notification area preferences correct */
void update_tray_settings()
{
	gtk_toggle_button_set_active((GtkToggleButton *)
			glade_xml_get_widget(xml_preferences_window, "ck_tray_enable"), info.do_tray);
}

void tray_enable_toggled(GtkToggleButton *but)
{
	info.do_tray = gtk_toggle_button_get_active(but);
	if(info.do_tray)
	{
		create_tray_icon();
	}
	else
	{
		destroy_tray_icon();
	}
}

void entry_auth_changed(GtkEntry *entry)
{
	strncpy(preferences.password,gtk_entry_get_text(GTK_ENTRY(glade_xml_get_widget(xml_preferences_window, "entry_auth"))),256);
}

void auth_enable_toggled(GtkToggleButton *but)
{
	preferences.user_auth = gtk_toggle_button_get_active(but);
	gtk_widget_set_sensitive(glade_xml_get_widget(xml_preferences_window, "entry_auth"), preferences.user_auth);	
}

void xfade_enable_toggled(GtkToggleButton *but)
{
	int bool1  = gtk_toggle_button_get_active(but);
	gtk_widget_set_sensitive(glade_xml_get_widget(xml_preferences_window, "sb_fade_time"), bool1);
	if(bool1)
	{
		int fade_time = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(glade_xml_get_widget(xml_preferences_window, "sb_fade_time")));	
		mpd_sendCrossfadeCommand(info.connection, fade_time);	
		mpd_finishCommand(info.connection);          
	}
	else {
		mpd_sendCrossfadeCommand(info.connection, 0);
		mpd_finishCommand(info.connection);

	}	
}

void xfade_time_changed(GtkSpinButton *but)
{
	int fade_time = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(glade_xml_get_widget(xml_preferences_window, "sb_fade_time")));	
	if(info.connection == NULL) return;
	mpd_sendCrossfadeCommand(info.connection, fade_time);	
	mpd_finishCommand(info.connection);          
}



/* this sets all the settings in the authentification area preferences correct */
void update_auth_settings()
{
	gtk_toggle_button_set_active((GtkToggleButton *)
			glade_xml_get_widget(xml_preferences_window, "ck_auth"), preferences.user_auth);
	gtk_widget_set_sensitive(glade_xml_get_widget(xml_preferences_window, "entry_auth"), preferences.user_auth);
	gtk_entry_set_text(GTK_ENTRY(glade_xml_get_widget(xml_preferences_window, "entry_auth")),preferences.password);
}



