/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* mp-column-chooser-dialog.c
 * Copyright (C) 2001 Anders Carlsson
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


#ifndef __EGG_COLUMN_CHOOSER_DIALOG_H__
#define __EGG_COLUMN_CHOOSER_DIALOG_H__

#include <gtk/gtkdialog.h>
#include <gtk/gtktreeview.h>
#include <gtk/gtkliststore.h>

#include "eggcolumnmodel.h"

G_BEGIN_DECLS

#define EGG_TYPE_COLUMN_CHOOSER_DIALOG (egg_column_chooser_dialog_get_type ())
#define EGG_COLUMN_CHOOSER_DIALOG(obj) (GTK_CHECK_CAST ((obj), EGG_TYPE_COLUMN_CHOOSER_DIALOG, EggColumnChooserDialog))
#define EGG_COLUMN_CHOOSER_DIALOG_CLASS(klass) (GTK_CHECK_CLASS_CAST ((klass), EGG_TYPE_COLUMN_CHOOSER_DIALOG, EggColumnChooserDialogClass))
#define EGG_IS_COLUMN_CHOOSER_DIALOG(obj)      (GTK_CHECK_TYPE ((obj), EGG_TYPE_COLUMN_CHOOSER_DIALOG))
#define EGG_IS_COLUMN_CHOOSER_DIALOG_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((obj), EGG_TYPE_COLUMN_CHOOSER_DIALOG))
#define EGG_COLUMN_CHOOSER_DIALOG_GET_CLASS(obj) (GTK_CHECK_GET_CLASS ((obj), EGG_TYPE_COLUMN_CHOOSER_DIALOG, GanttViewClass))

typedef struct _EggColumnChooserDialog      EggColumnChooserDialog;
typedef struct _EggColumnChooserDialogClass EggColumnChooserDialogClass;


struct _EggColumnChooserDialog {
	GtkDialog parent_instance;

	EggColumnModel *column_model;

	GtkWidget *show_button;
	GtkWidget *hide_button;

	GtkWidget *move_down_button;
	GtkWidget *move_up_button;

	GtkWidget *tree_view;
};

struct _EggColumnChooserDialogClass {
	GtkDialogClass parent_class;
};

GType      egg_column_chooser_get_type        (void);
GtkWidget *egg_column_chooser_new             (GtkTreeView *tree_view);

GType      egg_column_chooser_dialog_get_type (void);
GtkWidget *egg_column_chooser_dialog_new      (GtkTreeView *tree_view);

G_END_DECLS

#endif /* __EGG_COLUMN_CHOOSER_DIALOG_H__ */
