#ifndef __PREFERENCES_H__
#define __PREFERENCES_H__

/**
 * Create the preferences window
 */
void create_preferences_window(void);

/**
 * If a plugin state changed (enabled/disable) this forces the preferences 
 * window to update
 * the list.
 */
void preferences_window_update(void);
#endif
