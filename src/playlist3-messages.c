#include <gtk/gtk.h>
#include <time.h>
#include "main.h" 

extern int pl3_zoom;
extern GladeXML *pl3_xml;
static gboolean error_visible = FALSE;
guint timeout_callback = 0;
GtkListStore *message_list = NULL;
void message_window_open(void);
void message_window_destroy(GtkWidget *win);
static GIOChannel *log_file = NULL;
void playlist3_message_destroy(void)
{
    gchar *path;
    path = g_strdup_printf("%u:%s\n", (unsigned int)time(NULL), _("Stopped GMPC"));
    g_io_channel_write_chars(log_file, path, -1, NULL, NULL);
    q_free(path);
    g_io_channel_flush(log_file, NULL);
    g_io_channel_unref(log_file);
}

void playlist3_message_init(void)
{
	if(!message_list)
	{
        GError *error = NULL;
        gchar *path = gmpc_get_user_path("gmpc.log"); 
		message_list = gtk_list_store_new(3, G_TYPE_UINT, G_TYPE_STRING, G_TYPE_STRING);

        log_file = g_io_channel_new_file(path, "a", &error);
        if(error)
        {
            g_error("Failed to log file: '%s'", error->message);
        }
        q_free(path);

        path = g_strdup_printf("%u:%s\n", (unsigned int)time(NULL), _("Started GMPC"));
        g_io_channel_write_chars(log_file, path, -1, NULL, NULL);
        q_free(path);
        g_io_channel_flush(log_file, NULL);
    }
}
void playlist3_show_error_message(const gchar *message, ErrorLevel el)
{
	GtkWidget *event;
	GtkWidget *label = NULL; 
	GtkTreeIter iter;
	guint t = time(NULL);
	gchar *image_name;
    gchar *string;
	playlist3_message_init();
	gtk_list_store_prepend(message_list, &iter);
	gtk_list_store_set(message_list, &iter, 0,t, 2, message,-1);

    string = g_strdup_printf("%u:%i:%s\n",(unsigned int)t,el, message);
    g_io_channel_write_chars(log_file, string, -1, NULL, NULL);
    q_free(string);
    g_io_channel_flush(log_file, NULL);

	if(error_visible)
	{
		playlist3_close_error();
		if(timeout_callback)
		{
			g_source_remove(timeout_callback);
		}
		timeout_callback = 0;

	}
	switch(el)
	{
		case ERROR_CRITICAL:
			image_name = GTK_STOCK_DIALOG_ERROR;

			break;
		case ERROR_WARNING:
			image_name = GTK_STOCK_DIALOG_WARNING;
			break;
		default:
			image_name = GTK_STOCK_DIALOG_INFO;
			break;
	}
	gtk_list_store_set(message_list, &iter, 1, image_name,-1);

	if(pl3_xml && pl3_zoom != PLAYLIST_MINI)
	{
		label = gtk_image_new_from_stock(image_name, GTK_ICON_SIZE_BUTTON);

		event = glade_xml_get_widget(pl3_xml, "error_hbox"); 
		/* right image */

		gtk_box_pack_start(GTK_BOX(event), label, FALSE, TRUE, 0);
		label = gtk_label_new("") ;
		gtk_label_set_markup(GTK_LABEL(label),message);
		gtk_misc_set_alignment(GTK_MISC(label), 0,0.5);
		gtk_label_set_ellipsize(GTK_LABEL(label), PANGO_ELLIPSIZE_END);
		gtk_box_pack_start(GTK_BOX(event), label, TRUE, TRUE, 0);

		event = glade_xml_get_widget(pl3_xml, "error_event");
		gtk_widget_show_all(event);

		/* Error */
		error_visible = TRUE;
		timeout_callback = g_timeout_add(5000, (GSourceFunc)playlist3_close_error, NULL);
	}else{
		error_visible = FALSE;
	}
}	

void playlist3_error_add_widget(GtkWidget *widget)
{
	GtkWidget *event = glade_xml_get_widget(pl3_xml, "error_hbox"); 
	gtk_box_pack_end(GTK_BOX(event), widget, FALSE, TRUE, 0);	
	gtk_widget_show_all(event);
}

gboolean playlist3_close_error(void)
{
	if(error_visible)
	{
		error_visible = FALSE;
		g_source_remove(timeout_callback);
		

		if(pl3_xml)
		{
			GtkWidget *event = glade_xml_get_widget(pl3_xml, "error_event");
			gtk_widget_hide(event);
			event = glade_xml_get_widget(pl3_xml, "error_hbox"); 
			gtk_container_foreach(GTK_CONTAINER(event), (GtkCallback)(gtk_widget_destroy), NULL);
		}
	}
	timeout_callback = 0;
	return FALSE;
}
static void message_cell_data_func(GtkTreeViewColumn *tree_column,
		GtkCellRenderer *cell,
		GtkTreeModel *tree_model,
		GtkTreeIter *iter,
		gpointer data)
{
	time_t t;
	guint id;
	gchar text[64];
	struct tm *lt;
	gtk_tree_model_get(tree_model, iter, 0,&id, -1);
	/* gtk_list_store only knows the type unsigned int, not time_T
	 * so lets do some casting)
	 */
	t = (time_t) id;
	lt = localtime(&t);
	strftime(text, 64,"%H:%M:%S", lt);
	g_object_set(G_OBJECT(cell), "text",text,NULL);
}
void message_window_open(void)
{
	GladeXML *xml;
	GtkCellRenderer *renderer;
	GtkWidget *tree;
	gchar *path;
 	path = gmpc_get_full_glade_path("playlist3.glade");
 	xml = glade_xml_new (path, "message_window", NULL);
 	q_free(path);
	playlist3_message_init();
	
	tree= glade_xml_get_widget(xml, "message_tree");
	renderer = gtk_cell_renderer_pixbuf_new();
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree), -1,"", renderer, "stock-id", 1,NULL);
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_insert_column_with_data_func(GTK_TREE_VIEW(tree), -1,_("Time"), renderer,(GtkTreeCellDataFunc)message_cell_data_func, NULL,NULL);
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree), -1,_("Message"), renderer, "markup", 2,NULL);
	gtk_tree_view_set_model(GTK_TREE_VIEW(tree), GTK_TREE_MODEL(message_list));
	
	glade_xml_signal_autoconnect(xml);
}

void message_window_destroy(GtkWidget *win)
{

	GladeXML *xl = glade_get_widget_tree(win);
	gtk_widget_destroy(win);
	g_object_unref(xl);

}

