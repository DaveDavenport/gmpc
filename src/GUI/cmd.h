#ifndef __CMD_H__
#define __CMD_H__

/**
 * Shows the commandline in the playlist window.
 */
void show_command_line(void);

/**
 * @param entry The CMD #GtkEntry 
 * @param event the #GdkEvent to process
 * @param data  user data.
 *
 * Function handles key presses on the #GtkEntry of the CMD.
 * Escape: close the entry.
 * Up: Previous history item.
 * Down: Next history item.
 * Backspace: if empty, close the CMD.
 *
 * @returns TRUE if the keypress is handled and should not propagate.
 */
gboolean show_command_line_key_press_event(
                            GtkWidget *entry,
                            GdkEventKey *event,
                            gpointer data);

/**
 * @param entry The CMD #GtkEntry
 *
 * Handle activation of the CMD. It Executes the command.
 */
void show_command_line_activate(
                            GtkWidget *entry,
                            gpointer data);

/**
 * @param entry The CMD #GtkEntry
 * @param pos The #GtkEntryIconPostion that was released.
 * @param event The #GdkEvent to handle.
 * @param data user data.
 *
 * Handle cliking of the icons in the CMD entry.
 * On right icon, clear the entry.
 */
void show_command_line_icon_release(
                            GtkWidget *entry,
                            GtkEntryIconPosition pos,
                            GdkEvent *event,
                            gpointer data);

/**
 * @param entry The CMD #GtkEntry
 * @param data user data.
 *
 * Handle change event on the entry.
 * Used by the history.
 */
void show_command_line_entry_changed( 
                            GtkWidget *entry,
                            gpointer data);
#endif
