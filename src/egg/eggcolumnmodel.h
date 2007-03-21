/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* mp-column-model.h
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

#ifndef __EGG_COLUMN_MODEL_H__
#define __EGG_COLUMN_MODEL_H__

#include <gtk/gtktreeview.h>

G_BEGIN_DECLS

#define EGG_TYPE_COLUMN_MODEL            (egg_column_model_get_type ())
#define EGG_COLUMN_MODEL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), EGG_TYPE_COLUMN_MODEL, EggColumnModel))
#define EGG_COLUMN_MODEL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), EGG_TYPE_COLUMN_MODEL, EggColumnModelClass))
#define EGG_IS_COLUMN_MODEL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EGG_TYPE_COLUMN_MODEL))
#define EGG_IS_COLUMN_MODEL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), EGG_TYPE_COLUMN_MODEL))
#define EGG_COLUMN_MODEL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), EGG_TYPE_COLUMN_MODEL, EggColumnModelClass))

typedef struct _EggColumnModel      EggColumnModel;
typedef struct _EggColumnModelClass EggColumnModelClass;

struct _EggColumnModel
{
  GObject parent_instance;

  /*< private >*/
  gint stamp;
  GList *columns;
  GtkTreeView *tree_view;
};

struct _EggColumnModelClass
{
  GObjectClass parent_class;

  /* Padding for future expansion */
  void (*_gtk_reserved1) (void);
  void (*_gtk_reserved2) (void);
  void (*_gtk_reserved3) (void);
  void (*_gtk_reserved4) (void);
};


GType           egg_column_model_get_type           (void);
EggColumnModel *egg_column_model_new                (GtkTreeView    *tree_view);

void            egg_column_model_set_column_visible (EggColumnModel *model,
						     GtkTreeIter    *iter,
						     gboolean        visible);
gboolean        egg_column_model_get_column_visible (EggColumnModel *model,
						     GtkTreeIter    *iter);
gboolean        egg_column_model_is_column_first    (EggColumnModel *model,
						     GtkTreeIter    *iter);
gboolean        egg_column_model_is_column_last     (EggColumnModel *model,
						     GtkTreeIter    *iter);
void            egg_column_model_move_down_column   (EggColumnModel *model,
						     GtkTreeIter    *iter);
void            egg_column_model_move_up_column     (EggColumnModel *model,
						     GtkTreeIter    *iter);


G_END_DECLS

#endif /* __EGG_COLUMN_MODEL_H__ */
