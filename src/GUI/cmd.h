#ifndef __CMD_H__
#define __CMD_H__

/* Define to make gtkbuilder happy */
void show_command_line(void);
gboolean show_command_line_key_press_event(
                            GtkWidget *entry,
                            GdkEventKey *event,
                            gpointer data);
void show_command_line_activate(
                            GtkWidget *entry,
                            gpointer data);
void show_command_line_icon_release(
                            GtkWidget *entry,
                            GtkEntryIconPosition pos,
                            GdkEvent *event,
                            gpointer data);

#endif
