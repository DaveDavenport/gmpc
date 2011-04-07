#ifndef __THV_H__
#define __THV_H__

/**
 * THV
 */

/* These signals allow us to copy the state of the Model. */
void thv_row_inserted_signal(GtkTreeModel * model, 
            GtkTreePath * path,
            GtkTreeIter * iter,
            gpointer data);

void thv_row_changed_signal(GtkTreeModel * model,
            GtkTreePath * path,
            GtkTreeIter * iter,
            gpointer data);

void thv_row_deleted_signal(GtkTreeModel * model,
            GtkTreePath * path,
            gpointer data);

void thv_row_reordered_signal(
            GtkTreeModel * model,
            GtkTreePath * path,
            GtkTreeIter * iter,
            gpointer arg3,
            gpointer data);

void thv_set_button_state(int button);

#endif
