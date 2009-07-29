/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* egg-column-model.c
 * Copyright (C) 2001 Anders Carlsson, Jonathan Blanford
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "eggcolumnmodel.h"

#include <gtk/gtktreemodel.h>
#include <gtk/gtktreednd.h>
#include "plugin.h"
static void update_columns (GtkTreeView *tree_view, EggColumnModel *column_model);


enum
{
  COLUMN_VISIBLE,
  COLUMN_NAME,
  COLUMN_OBJECT,
  N_COLUMNS
};



static GObjectClass *parent_class = NULL;


static gint
egg_column_model_get_n_columns (GtkTreeModel *tree_model)
{
	return N_COLUMNS;
}

static GType
egg_column_model_get_column_type (GtkTreeModel *tree_model,
				  gint          temp_index)
{
  switch (temp_index)
    {
    case COLUMN_VISIBLE:
      return G_TYPE_BOOLEAN;
    case COLUMN_NAME:
      return G_TYPE_STRING;
    case COLUMN_OBJECT:
      return GTK_TYPE_TREE_VIEW_COLUMN;
    default:
      return G_TYPE_INVALID;
    }
}

static gboolean
egg_column_model_get_iter (GtkTreeModel *tree_model,
			   GtkTreeIter  *iter,
			   GtkTreePath  *path)
{
  EggColumnModel *column_model = (EggColumnModel *) tree_model;
  GList *list;
  gint i;

  i = gtk_tree_path_get_indices (path)[0];
  list = g_list_nth (column_model->columns, i);

  if (list == NULL)
  {

	  return FALSE;
  }
  iter->stamp = column_model->stamp;
  iter->user_data = list;

  return TRUE;
}

static GtkTreePath *
egg_column_model_get_path (GtkTreeModel *tree_model,
			   GtkTreeIter  *iter)
{
  EggColumnModel *column_model = (EggColumnModel *) tree_model;
  GtkTreePath *path;
  gint i = 0;
  GList *list;

  g_return_val_if_fail (EGG_IS_COLUMN_MODEL (tree_model), NULL);
  g_return_val_if_fail (iter->stamp == EGG_COLUMN_MODEL (tree_model)->stamp, NULL);
  
  for (list = column_model->columns; list; list = list->next)
    {
      if (list == (GList *)iter->user_data)
	break;
      i++;
    }

  if (list == NULL)
    return NULL;

  path = gtk_tree_path_new ();
  gtk_tree_path_append_index (path, i);

  return path;
}


static gboolean
egg_column_model_iter_has_child (GtkTreeModel *tree_model,
				 GtkTreeIter  *iter)
{
  g_return_val_if_fail (EGG_IS_COLUMN_MODEL (tree_model), FALSE);
  g_return_val_if_fail (iter->stamp == EGG_COLUMN_MODEL (tree_model)->stamp, FALSE);
  
  return FALSE;
}

static void
egg_column_model_get_value (GtkTreeModel *tree_model,
			    GtkTreeIter  *iter,
			    gint          column,
			    GValue       *value)
{
  GtkTreeViewColumn *view_column;

  g_return_if_fail (EGG_IS_COLUMN_MODEL (tree_model));
  g_return_if_fail (column < N_COLUMNS);
  g_return_if_fail (column >= 0);
  g_return_if_fail (iter->stamp == EGG_COLUMN_MODEL (tree_model)->stamp);
  
  view_column = GTK_TREE_VIEW_COLUMN (((GList *)iter->user_data)->data);
  switch (column)
    {
    case COLUMN_VISIBLE:
      g_value_init (value, G_TYPE_BOOLEAN);
      g_value_set_boolean (value, gtk_tree_view_column_get_visible (view_column));
      break;
    case COLUMN_NAME:
      g_value_init (value, G_TYPE_STRING);
      g_value_set_string (value, gtk_tree_view_column_get_title (view_column));
      break;
    case COLUMN_OBJECT:
      g_value_init (value, GTK_TYPE_TREE_VIEW_COLUMN);
      g_value_set_object (value, view_column);
      break;
    default:
      g_assert_not_reached ();
      break;
    }
  
}

static gboolean
egg_column_model_iter_next (GtkTreeModel *tree_model,
			    GtkTreeIter  *iter)
{
  EggColumnModel *column_model;

  g_return_val_if_fail (EGG_IS_COLUMN_MODEL (tree_model), FALSE);
  g_return_val_if_fail (iter->stamp == EGG_COLUMN_MODEL (tree_model)->stamp, FALSE);

  column_model = EGG_COLUMN_MODEL (tree_model);

  iter->user_data = ((GList *)iter->user_data)->next;

  return (iter->user_data != NULL);
}

static gint
egg_column_model_iter_n_children (GtkTreeModel *tree_model,
				  GtkTreeIter  *iter)
{
  g_return_val_if_fail (EGG_IS_COLUMN_MODEL (tree_model), 0);

  if (iter)
    {
      g_return_val_if_fail (iter->stamp == EGG_COLUMN_MODEL (tree_model)->stamp, 0);
      return 0;
    }

  return g_list_length (EGG_COLUMN_MODEL (tree_model)->columns);
}

static gboolean
egg_column_model_iter_nth_child (GtkTreeModel *tree_model,
				 GtkTreeIter  *iter,
				 GtkTreeIter  *parent,
				 gint          n)
{
  EggColumnModel *column_model;

  g_return_val_if_fail (EGG_IS_COLUMN_MODEL (tree_model), FALSE);

  column_model = EGG_COLUMN_MODEL (tree_model);
  if (parent)
    {
      g_return_val_if_fail (parent->stamp == EGG_COLUMN_MODEL (tree_model)->stamp, FALSE);
      return FALSE;
    }

  iter->user_data = g_list_nth ((GList *)column_model->columns, n);

  if (iter->user_data == NULL)
      return FALSE;

  iter->stamp = column_model->stamp;
  return TRUE;

}

/**
 * Gtk complains withouth this function, so add a dummy one.
 */
static gboolean
egg_column_model_iter_children(GtkTreeModel * tree_model,
		GtkTreeIter * iter, GtkTreeIter * parent)
{
	return FALSE;
}

static void
egg_column_model_tree_model_init (GtkTreeModelIface *iface)
{
  iface->get_n_columns = egg_column_model_get_n_columns;
  iface->get_column_type = egg_column_model_get_column_type;
  iface->get_iter = egg_column_model_get_iter;
  iface->get_path = egg_column_model_get_path;
  iface->get_value = egg_column_model_get_value;
  iface->iter_has_child = egg_column_model_iter_has_child;
  iface->iter_next = egg_column_model_iter_next;
  iface->iter_nth_child = egg_column_model_iter_nth_child;
  iface->iter_n_children = egg_column_model_iter_n_children;
  iface->iter_children = egg_column_model_iter_children;
}

static gboolean
egg_column_model_drag_data_get (GtkTreeDragSource *drag_source, GtkTreePath *path, GtkSelectionData *selection_data)
{
#if 0
	if (gtk_selection_data_set_tree_row (selection_data,
					     GTK_TREE_MODEL (drag_source),
					     path)) {
		return TRUE;
	}
	else {
	}
#endif
	return FALSE;
}

static gboolean
egg_column_model_drag_data_delete (GtkTreeDragSource *drag_source, GtkTreePath *path)
{
  return FALSE;
}

static void
egg_column_model_drag_source_init (GtkTreeDragSourceIface *iface)
{
  iface->drag_data_get = egg_column_model_drag_data_get;
  iface->drag_data_delete = egg_column_model_drag_data_delete;
}

static gboolean
egg_column_model_row_drop_possible (GtkTreeDragDest  *drag_dest,
				    GtkTreePath      *dest_path,
				    GtkSelectionData *selection_data)
{
  gint *indices;

  g_return_val_if_fail (EGG_IS_COLUMN_MODEL (drag_dest), FALSE);

  if (gtk_tree_path_get_depth (dest_path) != 1)
    return FALSE;

  indices = gtk_tree_path_get_indices (dest_path);

  if ((guint)indices[0] <= g_list_length (((EggColumnModel *)drag_dest)->columns))
    return TRUE;
  else
    return FALSE;
}

static void
egg_column_model_drag_dest_init (GtkTreeDragDestIface *iface)
{
  iface->row_drop_possible = egg_column_model_row_drop_possible;
}

static void
egg_column_model_init (EggColumnModel *model)
{
  do
    {
      model->stamp = g_random_int ();
    }
  while (model->stamp == 0);
}

static void
egg_column_model_finalize (GObject *object)
{
  EggColumnModel *model;

  model = (EggColumnModel *)object;

  g_list_free (model->columns);

  g_signal_handlers_disconnect_by_func (G_OBJECT (model->tree_view), (void*)update_columns, model);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
egg_column_model_class_init (EggColumnModelClass *klass)
{
  GObjectClass *object_class;

  object_class = (GObjectClass *)klass;

  parent_class = (GObjectClass *)g_type_class_peek_parent (klass);

  object_class->finalize = egg_column_model_finalize;
}

GType
egg_column_model_get_type (void)
{
  static GType object_type = 0;
	
  if (!object_type)
    {
      static const GTypeInfo object_info =
	{
	  sizeof (EggColumnModelClass),
	  NULL,
	  NULL,
	  (GClassInitFunc) egg_column_model_class_init,
	  NULL,
	  NULL,
	  sizeof (EggColumnModel),
	  0,
	  (GInstanceInitFunc) egg_column_model_init,
	};

      static const GInterfaceInfo tree_model_info =
	{
	  (GInterfaceInitFunc) egg_column_model_tree_model_init,
	  NULL,
	  NULL
	};

      static const GInterfaceInfo drag_source_info =
	{
	  (GInterfaceInitFunc) egg_column_model_drag_source_init,
	  NULL,
	  NULL
	};

      static const GInterfaceInfo drag_dest_info =
	{
	  (GInterfaceInitFunc) egg_column_model_drag_dest_init,
	  NULL,
	  NULL
	};
      
      object_type = g_type_register_static (G_TYPE_OBJECT, "EggColumnModel", &object_info, (GTypeFlags)0);
      g_type_add_interface_static (object_type,
				   GTK_TYPE_TREE_MODEL,
				   &tree_model_info);
      g_type_add_interface_static (object_type,
				   GTK_TYPE_TREE_DRAG_SOURCE,
				   &drag_source_info);
      g_type_add_interface_static (object_type,
				   GTK_TYPE_TREE_DRAG_DEST,
					     &drag_dest_info);
    }

  return object_type;
}

static void
update_columns (GtkTreeView    *tree_view,
		EggColumnModel *column_model)
{
  GList *old_columns = column_model->columns;
  gint old_length, length;
  GList *a, *b;

  column_model->columns = gtk_tree_view_get_columns (column_model->tree_view);

  /* As the view tells us one change at a time, we can do this hack. */
  length = g_list_length (column_model->columns);
  old_length = g_list_length (old_columns);

  if (length != old_length)
    {
      GtkTreePath *path;
      gint i = 0;

      /* Find out where they differ */
      for (a = old_columns, b = column_model->columns; a && b; a = a->next, b = b->next) {
	if (a->data != b->data)
	  break;
	i++;
      }

      path = gtk_tree_path_new ();
      gtk_tree_path_append_index (path, i);

      if (length < old_length)
	{
	  column_model->stamp++;
	  gtk_tree_model_row_deleted (GTK_TREE_MODEL (column_model), path);
	}
      else
	{
	  GtkTreeIter iter;

	  iter.stamp = column_model->stamp;
	  iter.user_data = b;
	  gtk_tree_model_row_inserted (GTK_TREE_MODEL (column_model), path, &iter);
	}

      gtk_tree_path_free (path);
    }
  else
    {
      gint i;
      gint m = 0, n = 1;
      gint *new_order;
      GtkTreePath *path;

      new_order = g_new (int, length);
      a = old_columns; b = column_model->columns;

      while (a->data == b->data) {
	a = a->next;
	b = b->next;
	
	if (a == NULL)
	{
		g_free(new_order);
		return;
	}
	m++;
      }
      
      if (a->next->data == b->data)
	{
	  b = b->next;
	  while (b->data != a->data)
	    {
	      b = b->next;
	      n++;
	    }
	  for (i = 0; i < m; i++)
	    new_order[i] = i;
	  for (i = m; i < m+n; i++)
	    new_order[i] = i+1;
	  new_order[i] = m;
	  for (i = m + n +1; i < length; i++)
	    new_order[i] = i;
	}
      else
	{
	  a = a->next;
	  while (a->data != b->data)
	    {
	      a = a->next;
	      n++;
	    }
	  for (i = 0; i < m; i++)
	    new_order[i] = i;
	  new_order[m] = m+n;
	  for (i = m+1; i < m + n+ 1; i++)
	    new_order[i] = i - 1;
	  for (i = m + n + 1; i < length; i++)
	    new_order[i] = i;
	}

      path = gtk_tree_path_new ();
      gtk_tree_model_rows_reordered (GTK_TREE_MODEL (column_model),
				     path,
				     NULL,
				     new_order);
      gtk_tree_path_free (path);
      g_free (new_order);
    }

  if (old_columns)
    g_list_free (old_columns);
}

EggColumnModel *
egg_column_model_new (GtkTreeView *tree_view)
{
  EggColumnModel *model;

  model = (EggColumnModel *)g_object_new (egg_column_model_get_type (), NULL);

  model->tree_view = tree_view;
  model->columns = gtk_tree_view_get_columns (tree_view);

  g_signal_connect (G_OBJECT (tree_view), "columns_changed",
		    G_CALLBACK (update_columns), model);
  
  return model;
}

void
egg_column_model_set_column_visible (EggColumnModel *model, GtkTreeIter *iter, gboolean visible)
{
  GtkTreeViewColumn *column;
  GtkTreePath *path;
  gchar *string = NULL;
  gpointer data;
  int colid = 0; 
  path = gtk_tree_model_get_path (GTK_TREE_MODEL (model), iter);

  column = (GtkTreeViewColumn *)((GList *)iter->user_data)->data;
  data = g_object_get_data(G_OBJECT(column), "colid");
  colid = GPOINTER_TO_INT(data);

  gtk_tree_view_column_set_visible (column, visible);
  gtk_tree_model_row_changed (GTK_TREE_MODEL (model), path, iter);

  /* custom code */
  string = g_strdup_printf("%i", colid);
  cfg_set_single_value_as_int(config, "current-playlist-column-enable",string, visible);	

  gtk_tree_path_free (path);
}

gboolean
egg_column_model_get_column_visible (EggColumnModel *model, GtkTreeIter *iter)
{
  GtkTreeViewColumn *column;


  g_return_val_if_fail (model->stamp == iter->stamp, FALSE);

  column = (GtkTreeViewColumn *)((GList *)iter->user_data)->data;
  
  return gtk_tree_view_column_get_visible (column);
}

gboolean
egg_column_model_is_column_first (EggColumnModel *model, GtkTreeIter *iter)
{
  g_return_val_if_fail (model->stamp == iter->stamp, FALSE);
	
  return (((GList *)iter->user_data)->prev == NULL);
}

gboolean
egg_column_model_is_column_last (EggColumnModel *model, GtkTreeIter *iter)
{
  g_return_val_if_fail (model->stamp == iter->stamp, FALSE);
	
  return (((GList *)iter->user_data)->next == NULL);
}

void
egg_column_model_move_down_column (EggColumnModel *model, GtkTreeIter *iter)
{
  GtkTreeViewColumn *column, *base_column;
  GList *node;
 
  g_return_if_fail (model->stamp == iter->stamp);
  g_return_if_fail (((GList *)iter->user_data)->next != NULL);

  node = (GList *)iter->user_data;

  column = (GtkTreeViewColumn *)node->data;
  base_column = (GtkTreeViewColumn *)node->next->data;

  gtk_tree_view_move_column_after (model->tree_view, column, base_column);
}

void
egg_column_model_move_up_column (EggColumnModel *model, GtkTreeIter *iter)
{
  GtkTreeViewColumn *column, *base_column;
  GList *node;
 
  g_return_if_fail (model->stamp == iter->stamp);
  g_return_if_fail (((GList *)iter->user_data)->prev != NULL);

  node = (GList *)iter->user_data;

  column = (GtkTreeViewColumn *)node->prev->data;
  base_column = (GtkTreeViewColumn *)node->data;

  gtk_tree_view_move_column_after (model->tree_view, column, base_column);
}
