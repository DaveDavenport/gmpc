#ifndef __OSB_BROWSER__
#define __OSB_BROWSER__

void pl3_osb_browser_add();
void pl3_osb_browser_fill_view(char *buffer);
void pl3_osb_browser_view_browser(gchar *url,gchar *name);
void pl3_osb_browser_refresh();
void pl3_osb_browser_add_source();
void pl3_osb_browser_del_source();
void pl3_osb_browser_row_activated(GtkTreeView *tree, GtkTreePath *tp);
#endif
