void ol_create(GtkWidget *wid);
void ol_destroy();
void ol_create_url(GtkWidget *wid,char *url);
void ol_drag_data_recieved(GtkWidget *window, GdkDragContext *context,
		gint x, gint y, GtkSelectionData *selection_data,
		guint info, guint time);
