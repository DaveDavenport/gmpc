void ol_create(GtkWidget *wid);
void ol_destroy();
void ol_drag_data_recieved(GtkWidget *window, GdkDragContext *context,
		gint x, gint y, GtkSelectionData *selection_data,
		guint info, guint time);
