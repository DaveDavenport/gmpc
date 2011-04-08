#ifndef __THV_H__
#define __THV_H__

/**
 * THV
 */


/**
 * @param button the index of the button to be active. -1 is no selection.
 *
 * Set the active button.
 */ 
void thv_set_button_state(int button);

/**
 * @param model a #GtkTreeModel
 *
 * Initialize the the button bar.
 * Use model as backend
 */
void thv_init(GtkTreeModel *model);
#endif
