/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* egg-column-chooser-dialog.c
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

#include "eggcolumnchooserdialog.h"

#include <gtk/gtkbutton.h>
#include <gtk/gtkcellrenderertext.h>
#include <gtk/gtkcellrenderertoggle.h>
#include <gtk/gtkimage.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkstock.h>
#include <gtk/gtktreeview.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtkscrolledwindow.h>
#include <gtk/gtktreeselection.h>

#include "eggcolumnmodel.h"

static void egg_column_chooser_dialog_class_init (EggColumnChooserDialogClass *klass);
static void egg_column_chooser_dialog_init (EggColumnChooserDialog *dialog);

static GtkDialogClass *parent_class = NULL;

GType
egg_column_chooser_dialog_get_type (void)
{
	static GtkType object_type = 0;

	if (!object_type) {
		static const GTypeInfo object_info = {
			sizeof (EggColumnChooserDialogClass),
			NULL,
			NULL,
			(GClassInitFunc) egg_column_chooser_dialog_class_init,
			NULL,
			NULL,
			sizeof (EggColumnChooserDialog),
			0,
			(GInstanceInitFunc) egg_column_chooser_dialog_init
		};

		object_type = g_type_register_static (GTK_TYPE_DIALOG, "EggColumnChooserDialog", &object_info, (GTypeFlags)0);

	}

	return object_type;
}

static void
update_button_states (EggColumnChooserDialog *dialog, GtkTreeIter *iter)
{
	if (egg_column_model_get_column_visible (dialog->column_model, iter)) {
		gtk_widget_set_sensitive (dialog->hide_button, TRUE);
		gtk_widget_set_sensitive (dialog->show_button, FALSE);
	}
	else {
		gtk_widget_set_sensitive (dialog->hide_button, FALSE);
		gtk_widget_set_sensitive (dialog->show_button, TRUE);
	}

	if (egg_column_model_is_column_first (dialog->column_model, iter)) {
		gtk_widget_set_sensitive (dialog->move_up_button, FALSE);
	}
	else {
		gtk_widget_set_sensitive (dialog->move_up_button, TRUE);
	}

	if (egg_column_model_is_column_last (dialog->column_model, iter)) {
		gtk_widget_set_sensitive (dialog->move_down_button, FALSE);
	}
	else {
		gtk_widget_set_sensitive (dialog->move_down_button, TRUE);
	}

}

static void
selection_changed (GtkTreeSelection *selection, EggColumnChooserDialog *dialog)
{
	GtkTreeIter iter;
	
	if(gtk_tree_selection_get_selected (selection, NULL, &iter))
	{
		update_button_states (dialog, &iter);
	}
}

static void
visible_toggled (GtkCellRendererToggle *cell, gchar *path_str, EggColumnChooserDialog *dialog)
{
	GtkTreePath *path;
	GtkTreeIter iter;
	GValue value = {0, };
	GtkTreeModel *model = GTK_TREE_MODEL (dialog->column_model);
	
	path = gtk_tree_path_new_from_string (path_str);
	gtk_tree_model_get_iter (model, &iter, path);

	gtk_tree_model_get_value (model,
				  &iter,
				  0,
				  &value);

	egg_column_model_set_column_visible (dialog->column_model, &iter, !g_value_get_boolean (&value));

	gtk_tree_path_free (path);
	update_button_states (dialog, &iter);
}

static void
show_button_clicked (GtkWidget *button, EggColumnChooserDialog *dialog)
{
	GtkTreeIter iter;
	
	gtk_tree_selection_get_selected (gtk_tree_view_get_selection (GTK_TREE_VIEW (dialog->tree_view)), NULL, &iter);

	egg_column_model_set_column_visible (dialog->column_model, &iter, TRUE);
	
	update_button_states (dialog, &iter);
}

static void
hide_button_clicked (GtkWidget *button, EggColumnChooserDialog *dialog)
{
	GtkTreeIter iter;
	
	gtk_tree_selection_get_selected (gtk_tree_view_get_selection (GTK_TREE_VIEW (dialog->tree_view)), NULL, &iter);

	egg_column_model_set_column_visible (dialog->column_model, &iter, FALSE);
	
	update_button_states (dialog, &iter);
}

static void
move_down_button_clicked (GtkWidget *button, EggColumnChooserDialog *dialog)
{
	GtkTreeIter iter;
	
	gtk_tree_selection_get_selected (gtk_tree_view_get_selection (GTK_TREE_VIEW (dialog->tree_view)), NULL, &iter);

	egg_column_model_move_down_column (dialog->column_model, &iter);

	/* We need to do this again since the model structure has changed */
	gtk_tree_selection_get_selected (gtk_tree_view_get_selection (GTK_TREE_VIEW (dialog->tree_view)), NULL, &iter);
	update_button_states (dialog, &iter);
}

static void
move_up_button_clicked (GtkWidget *button, EggColumnChooserDialog *dialog)
{
	GtkTreeIter iter;
	
	gtk_tree_selection_get_selected (gtk_tree_view_get_selection (GTK_TREE_VIEW (dialog->tree_view)), NULL, &iter);

	egg_column_model_move_up_column (dialog->column_model, &iter);

	/* We need to do this again since the model structure has changed */
	gtk_tree_selection_get_selected (gtk_tree_view_get_selection (GTK_TREE_VIEW (dialog->tree_view)), NULL, &iter);
	update_button_states (dialog, &iter);
}

static void
egg_column_chooser_dialog_finalize (GObject *object)
{
	EggColumnChooserDialog *dialog;

	dialog = EGG_COLUMN_CHOOSER_DIALOG (object);

	g_object_unref (G_OBJECT (dialog->column_model));
	
	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
egg_column_chooser_dialog_response (GtkDialog *dialog, gint response_id)
{
	gtk_widget_destroy (GTK_WIDGET (dialog));
}

static void
egg_column_chooser_dialog_class_init (EggColumnChooserDialogClass *klass)
{
	GObjectClass *object_class;
	GtkDialogClass *dialog_class;
	
	object_class = (GObjectClass *)klass;
	dialog_class = (GtkDialogClass *)klass;
	
	parent_class = (GtkDialogClass *)g_type_class_peek_parent (klass);

	object_class->finalize = egg_column_chooser_dialog_finalize;

	dialog_class->response = egg_column_chooser_dialog_response;
}

static void
egg_column_chooser_dialog_init (EggColumnChooserDialog *dialog)
{
	GtkWidget *hbox, *vbox, *vbox2, *label, *button;
	GtkWidget *scrolled_window;
	GtkTreeSelection *selection;
	GtkCellRenderer *toggle_renderer;
	
	gtk_window_set_title (GTK_WINDOW (dialog), "Columns");
	
	gtk_dialog_add_button (GTK_DIALOG (dialog),
			       GTK_STOCK_CLOSE,
			       GTK_RESPONSE_CLOSE);

	vbox = gtk_vbox_new (FALSE, 12);
	gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);
	
	label = gtk_label_new ("Check the columns that you would like to be visible in this view. "
			       "Use the Move Up and Move Down buttons to reorder the columns "
			       "however you like.");
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
	
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), vbox, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 8);

	
	hbox = gtk_hbox_new (FALSE, 12);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);

	scrolled_window = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window),
					     GTK_SHADOW_IN);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
					GTK_POLICY_AUTOMATIC,
					GTK_POLICY_AUTOMATIC);
	gtk_box_pack_start (GTK_BOX (hbox), scrolled_window, TRUE, TRUE, 0);
	
	dialog->tree_view = gtk_tree_view_new ();
	g_object_set_data (G_OBJECT (dialog->tree_view), "foo-data", GINT_TO_POINTER (TRUE));
	gtk_tree_view_set_reorderable (GTK_TREE_VIEW (dialog->tree_view), FALSE);
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (dialog->tree_view));
	g_signal_connect (G_OBJECT (selection), "changed",
			  G_CALLBACK (selection_changed), dialog);
	
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (dialog->tree_view), FALSE);
	gtk_container_add (GTK_CONTAINER (scrolled_window), dialog->tree_view);

	toggle_renderer = (GtkCellRenderer *)g_object_new (GTK_TYPE_CELL_RENDERER_TOGGLE,
					"activatable", TRUE,
					NULL);
	g_signal_connect (G_OBJECT (toggle_renderer), "toggled",
			  G_CALLBACK (visible_toggled), dialog);
	
	gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (dialog->tree_view),
						     -1, NULL,
						     toggle_renderer,
						     "active", 0,
						     NULL);
	
	gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (dialog->tree_view),
						     -1, NULL,
						     gtk_cell_renderer_text_new (),
						     "text", 1,
						     NULL);

	vbox2 = gtk_vbox_new (FALSE, 8);
	gtk_box_pack_start (GTK_BOX (hbox), vbox2, FALSE, FALSE, 0);
	
	dialog->move_up_button = gtk_button_new_with_mnemonic ("Move _Up");
	g_signal_connect (G_OBJECT (dialog->move_up_button), "clicked",
			  G_CALLBACK (move_up_button_clicked), dialog);
	gtk_box_pack_start (GTK_BOX (vbox2), dialog->move_up_button, FALSE, FALSE, 0);

	dialog->move_down_button = gtk_button_new_with_mnemonic ("Move _Down");
	g_signal_connect (G_OBJECT (dialog->move_down_button), "clicked",
			  G_CALLBACK (move_down_button_clicked), dialog);
	gtk_box_pack_start (GTK_BOX (vbox2), dialog->move_down_button, FALSE, FALSE, 0);
	
	dialog->show_button = gtk_button_new_with_mnemonic ("_Show");
	g_signal_connect (G_OBJECT (dialog->show_button), "clicked",
			  G_CALLBACK (show_button_clicked), dialog);

	gtk_box_pack_start (GTK_BOX (vbox2), dialog->show_button, FALSE, FALSE, 0);

	dialog->hide_button = gtk_button_new_with_mnemonic ("_Hide");
	g_signal_connect (G_OBJECT (dialog->hide_button), "clicked",
			  G_CALLBACK (hide_button_clicked), dialog);
	gtk_box_pack_start (GTK_BOX (vbox2), dialog->hide_button, FALSE, FALSE, 0);

	button = gtk_button_new_with_label ("Reset");
	gtk_widget_set_sensitive (button, FALSE);
	gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, FALSE, 0);

}

static void
rows_reordered (GtkTreeModel *tree_model, GtkTreePath *path, GtkTreeIter *iter, gint *new_order, EggColumnChooserDialog *dialog)
{
	GtkTreeIter my_iter;
	
	gtk_tree_selection_get_selected (gtk_tree_view_get_selection (GTK_TREE_VIEW (dialog->tree_view)), NULL, &my_iter);
	update_button_states (dialog, &my_iter);
}

static void
egg_column_chooser_dialog_construct (EggColumnChooserDialog *dialog, GtkTreeView *tree_view)
{
	dialog->column_model = egg_column_model_new (tree_view);
	g_signal_connect_after (G_OBJECT (dialog->column_model), "rows_reordered",
				G_CALLBACK (rows_reordered), dialog);
	
	gtk_tree_view_set_model (GTK_TREE_VIEW (dialog->tree_view), GTK_TREE_MODEL (dialog->column_model));
	
}

GtkWidget *
egg_column_chooser_dialog_new (GtkTreeView *tree_view)
{
	EggColumnChooserDialog *dialog;
	dialog = (EggColumnChooserDialog *)g_object_new (EGG_TYPE_COLUMN_CHOOSER_DIALOG, NULL);

	egg_column_chooser_dialog_construct (dialog, tree_view);

	/* Select the first element in the list */
/*	path = gtk_tree_path_new ();
	gtk_tree_path_append_index (path, 0);
	gtk_tree_selection_select_path (gtk_tree_view_get_selection (GTK_TREE_VIEW (dialog->tree_view)), path);
	gtk_tree_path_free (path);
*/
	return GTK_WIDGET (dialog);
}
