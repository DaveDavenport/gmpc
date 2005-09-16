#ifndef __PL3_CUSTOM_STREAM_H__
#define __PL3_CUSTOM_STREAM_H__
void pl3_custom_stream_add();
void pl3_custom_stream_view_browser();
void pl3_custom_stream_add_url_changed(GtkEntry *entry, GtkWidget *button);
void pl3_custom_stream_add_stream(gchar *name, gchar *url);
void pl3_custom_stream_save_tree();
void pl3_custom_stream_remove();
void pl3_custom_stream_row_activated(GtkTreeView *tree, GtkTreePath *tp);
void pl3_custom_stream_category_selection_changed(GtkTreeView *tree,GtkTreeIter *iter);

#endif
