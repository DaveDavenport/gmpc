#include <gtk/gtk.h>
#include "main.h" 

extern GladeXML *pl3_xml;
static gboolean error_visible = FALSE;

void playlist3_show_error_message(const gchar *message, ErrorLevel el)
{
	GtkWidget *event = glade_xml_get_widget(pl3_xml, "error_hbox"); 
	GtkWidget *label = NULL; 
	if(error_visible)
	{
		playlist3_close_error();
	}
	/* Error */
	error_visible = TRUE;
	/* right image */
	switch(el)
	{
		case ERROR_CRITICAL:
			label = gtk_image_new_from_stock(GTK_STOCK_DIALOG_ERROR, GTK_ICON_SIZE_BUTTON);
			break;
		case ERROR_WARNING:
			label = gtk_image_new_from_stock(GTK_STOCK_DIALOG_WARNING, GTK_ICON_SIZE_BUTTON);
			break;
		default:
			label = gtk_image_new_from_stock(GTK_STOCK_DIALOG_INFO, GTK_ICON_SIZE_BUTTON);
			break;
	}

	gtk_box_pack_start(GTK_BOX(event), label, FALSE, TRUE, 0);
	label = gtk_label_new("") ;
	gtk_label_set_markup(GTK_LABEL(label),message);

	gtk_box_pack_start(GTK_BOX(event), label, FALSE, TRUE, 0);

	label = gtk_button_new_from_stock(GTK_STOCK_CONNECT);
	gtk_box_pack_end(GTK_BOX(event), label, FALSE, TRUE, 0);	
	g_signal_connect(G_OBJECT(label), "clicked", G_CALLBACK(connect_to_mpd), NULL);
	event = glade_xml_get_widget(pl3_xml, "error_event");
	gtk_widget_show_all(event);


}	

void playlist3_close_error(void)
{
	if(pl3_xml)
	{
		GtkWidget *event = glade_xml_get_widget(pl3_xml, "error_event");
		gtk_widget_hide(event);
		event = glade_xml_get_widget(pl3_xml, "error_hbox"); 
		gtk_container_foreach(GTK_CONTAINER(event), (GtkCallback)(gtk_widget_destroy), NULL);
	}
	error_visible = FALSE;
}
