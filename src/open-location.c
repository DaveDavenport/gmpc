#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <string.h>
#include <glade/glade.h>
#include "libmpdclient.h"
#include "main.h"
#include "misc.h"
#include "open-location.h"

GladeXML *ol_xml = NULL;

void ol_destroy()
{
	gtk_widget_destroy(glade_xml_get_widget(ol_xml, "add_location"));
	g_object_unref(ol_xml);
	ol_xml = NULL;
}

void ol_add_location()
{
	if(strlen(gtk_entry_get_text(GTK_ENTRY(glade_xml_get_widget(ol_xml, "entry_stream")))) == 0 ) return;
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(glade_xml_get_widget(ol_xml, "tb_playlist"))))
	{
		
						

	}
	else
	{
		mpd_sendAddCommand(info.connection, gtk_entry_get_text(GTK_ENTRY(glade_xml_get_widget(ol_xml, "entry_stream"))));
		mpd_finishCommand(info.connection);
	}
	ol_destroy();
}


void ol_replace_location()
{
	mpd_sendClearCommand(info.connection);
	mpd_finishCommand(info.connection);
	ol_add_location();
}



void ol_create(GtkWidget *wid)
{
	if(ol_xml != NULL)
	{
		gtk_widget_show_all(
				glade_xml_get_widget(ol_xml, "add_location"));
		gtk_window_present(GTK_WINDOW(
					glade_xml_get_widget(ol_xml, "add_location")));
		return;          	
	}
	ol_xml = glade_xml_new(GLADE_PATH"open-location.glade", "add_location",NULL);
	gtk_window_set_transient_for(GTK_WINDOW(glade_xml_get_widget(ol_xml, "add_location")), 
			GTK_WINDOW(gtk_widget_get_toplevel(wid)));

	gtk_window_set_position(GTK_WINDOW(glade_xml_get_widget(ol_xml, "add_location")), GTK_WIN_POS_CENTER_ON_PARENT);
		
	gtk_image_set_from_stock(GTK_IMAGE(glade_xml_get_widget(ol_xml, "image")), "media-stream", GTK_ICON_SIZE_DIALOG);

	gtk_widget_show_all(glade_xml_get_widget(ol_xml, "add_location"));
	glade_xml_signal_autoconnect(ol_xml);	
}
