#ifndef __STATUS_ICON_H__
#define __STATUS_ICON_H__

/**
 * Update the default set of status icons.
 * random, repeat, single, consume
 */
void main_window_update_status_icons(void);

/**
 * Add default set of status icons.
 * random, repeat, single, consume
 */
void main_window_init_default_status_icons(void);

/**
 * @param icon  The #GtkWidget to add
 *
 * Add an icon to the status bar.
 */
void main_window_add_status_icon(GtkWidget * icon);
#endif
