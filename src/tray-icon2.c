#include <gtk/gtk.h>
#include <libmpd/libmpd.h>
#include "main.h"
#include "plugin.h"

GtkStatusIcon *tray_icon2_gsi = NULL;

gboolean tray_icon2_get_available(void)
{
	if(tray_icon2_gsi)
		return gtk_status_icon_is_embedded(tray_icon2_gsi);
	return FALSE;
}

static void tray_icon2_activate(GtkStatusIcon *gsi, gpointer user_data)
{
	pl3_toggle_hidden();
}

static void tray_icon2_populate_menu(GtkStatusIcon *gsi,guint button, guint activate_time, gpointer user_data)
{
	GtkWidget *item;
	GtkWidget *menu = gtk_menu_new();

	if(mpd_check_connected(connection))
	{
		if(mpd_server_check_command_allowed(connection, "play"))
		{
			item = gtk_image_menu_item_new_from_stock(GTK_STOCK_MEDIA_PLAY, NULL);
			gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
			g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(play_song), NULL);


			item = gtk_image_menu_item_new_from_stock(GTK_STOCK_MEDIA_STOP, NULL);
			gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
			g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(stop_song), NULL);

			item = gtk_image_menu_item_new_from_stock(GTK_STOCK_MEDIA_NEXT, NULL);
			gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
			g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(next_song), NULL);


			item = gtk_image_menu_item_new_from_stock(GTK_STOCK_MEDIA_PREVIOUS, NULL);
			gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
			g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(prev_song), NULL);
			item = gtk_separator_menu_item_new();
			gtk_menu_shell_append(GTK_MENU_SHELL(menu),item);
		}
	}
	else
	{
		item = gtk_image_menu_item_new_from_stock(GTK_STOCK_CONNECT, NULL);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
		g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(connect_to_mpd), NULL);
		item = gtk_separator_menu_item_new();
		gtk_menu_shell_append(GTK_MENU_SHELL(menu),item);			
	}
	item = gtk_image_menu_item_new_with_mnemonic(_("Pla_ylist"));
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item),
			gtk_image_new_from_stock("gtk-justify-fill", GTK_ICON_SIZE_MENU));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(create_playlist3), NULL);


	item = gtk_separator_menu_item_new();
	gtk_menu_shell_append(GTK_MENU_SHELL(menu),item);

	item = gtk_image_menu_item_new_from_stock(GTK_STOCK_QUIT,NULL);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
	g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(main_quit), NULL);
	gtk_widget_show_all(menu);
	gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, button, activate_time);
}

static void tray_icon2_status_changed(MpdObj *mi, ChangedStatusType what, void *userdata)
{
	if(tray_icon2_gsi == NULL)
		return;
	if(what&MPD_CST_STATE)
	{
		int state = mpd_player_get_state(connection);
		if(state == MPD_PLAYER_STOP || state == MPD_PLAYER_UNKNOWN)
		{
			gtk_status_icon_set_from_icon_name(tray_icon2_gsi, "gmpc-tray");
		}
		else if(state == MPD_PLAYER_PLAY){
			gtk_status_icon_set_from_icon_name(tray_icon2_gsi, "gmpc-tray-play");
		}
		else if(state == MPD_PLAYER_PAUSE){
			gtk_status_icon_set_from_icon_name(tray_icon2_gsi, "gmpc-tray-pause");
		}
	}



}
static void tray_icon2_connection_changed(MpdObj *mi, int connect,void *user_data)
{
	if(tray_icon2_gsi == NULL)
		return;

	if(connect)	{
		gtk_status_icon_set_from_icon_name(tray_icon2_gsi, "gmpc-tray");
		tray_icon2_status_changed(mi, MPD_CST_STATE,NULL);
	} else {
		gtk_status_icon_set_from_icon_name(tray_icon2_gsi, "gmpc-tray-disconnected");
	}
}

static void tray_icon2_init(void)
{
	tray_icon2_gsi = gtk_status_icon_new_from_icon_name ("gmpc-tray-disconnected");
	g_signal_connect(G_OBJECT(tray_icon2_gsi), "popup-menu", G_CALLBACK(tray_icon2_populate_menu), NULL);
	g_signal_connect(G_OBJECT(tray_icon2_gsi), "activate", G_CALLBACK(tray_icon2_activate), NULL);

}

static gboolean tray_icon2_get_enabled(void)
{
	return TRUE;
}

static void tray_icon2_set_enabled(int enabled)
{
}

gmpcPlugin tray_icon2_plug = {
	.name 						= "Notification Tray 2",
	.version 					= {0,0,0},
	.plugin_type 				= GMPC_INTERNALL,
	.init 						= tray_icon2_init,
	.mpd_status_changed 		= tray_icon2_status_changed,
	.mpd_connection_changed 	= tray_icon2_connection_changed,
	.set_enabled 				= tray_icon2_set_enabled,
	.get_enabled				= tray_icon2_get_enabled
};
