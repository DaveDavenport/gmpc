#ifndef __PLUGIN_MAN_H__
#define __PLUGIN_MAN_H__

/**
 * plugin manager takes care of actions that have to be done on all plugins.
 * This basically wraps plugin.c
 */

/**
 * Load internal gmpc plugins
 */
void plugin_manager_load_internal_plugins(void);

/**
 * Initialize all the loaded plugins.
 *
 * This initializes all the internal and external pluginsto initialize.
 */
void plugin_manager_initialize_plugins(void);


/**
 * Destroy all the loaded plugins.
 *
 * This tells all internal and external plugins to destroy itself.
 */
void plugin_manager_destroy_plugins(void);

/**
 * Tell the plugins to save state
 *
 * This executes the gmpc_plugin_save_yourself command on all internal
 * and external plugins.
 */
void plugin_manager_save_state(void);

/**
 * @param mi the #MpdObj 
 * @param connected the new connected state (1 is connected, 0 is disconnected)
 *
 * Tell all plugins that the connection changed
 */
void plugin_manager_connection_changed(MpdObj *mi, const int connected);


/**
 * @param mi the #MpdObj
 * @param what a #changedStatusType bitmask with changes
 *
 * Tell all pluginst that the status of the mi object changed.
 */
void plugin_manager_status_changed(MpdObj *mi, const ChangedStatusType what);

/**
 * Loads the external plugins in gmpc.
 */
void plugin_manager_load_plugins(void);
#endif
