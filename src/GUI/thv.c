/* Gnome Music Player Client (GMPC)
 * Copyright (C) 2004-2011 Qball Cow <qball@gmpclient.org>
 * Project homepage: http://gmpclient.org/

 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
 
#include <gtk/gtk.h>
#include "playlist3.h"
#include "thv.h"

/***
 * Tabbed view hooks
 */
/* List keeping track of all the button */
static GList *thv_list = NULL;
/* Tab button structure */
typedef struct _TabButton
{
    GtkToggleButton *button;
    GtkImage *image;
    GtkLabel *label;
    gint pos;
    guint handler;
} TabButton;


static int last_button = -1;
void thv_set_button_state(int button)
{
    TabButton *tb = NULL;
    if (button >= 0)
    {
        tb = g_list_nth_data(thv_list, button);
        if (tb)
        {
            g_signal_handler_block(G_OBJECT(tb->button), tb->handler);
            if (!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(tb->button)))
            {
                gtk_toggle_button_set_active(
                            GTK_TOGGLE_BUTTON(tb->button),
                            TRUE);
            }
            g_signal_handler_unblock(G_OBJECT(tb->button), tb->handler);

            last_button = button;
        }
    } else
    {
        if (last_button >= 0)
        {
            tb = g_list_nth_data(thv_list, last_button);
            if (tb)
            {

                g_signal_handler_block(G_OBJECT(tb->button), tb->handler);
                if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(tb->button)))
                {
                    gtk_toggle_button_set_active(
                            GTK_TOGGLE_BUTTON(tb->button),
                            FALSE);
                }
                g_signal_handler_unblock(G_OBJECT(tb->button), tb->handler);
            }
            last_button = -1;
        }
    }
}


/* if a row in the sidebar changed (f.e. title or icon)
 * update the button accordingly
 */
static void thv_row_changed_signal(
        GtkTreeModel * model,
        GtkTreePath * path,
        GtkTreeIter * iter,
        gpointer data)
{
    gint *ind = gtk_tree_path_get_indices(path);
    TabButton *tb = g_list_nth_data(thv_list, ind[0]);
    if (tb)
    {
        gchar *title, *image;
        gtk_tree_model_get(model, iter, 3, &image, 1, &title, -1);
        /* update label */
        if (title)
        {
            gtk_label_set_text(tb->label, title);
            gtk_widget_set_tooltip_text(GTK_WIDGET(tb->label), title);
        }
        /* update image */
        if (image)
        {
            gtk_image_set_from_icon_name(tb->image, image, GTK_ICON_SIZE_MENU);
        }
        /* free gtk_tree_model_get items */
        if (title)
            g_free(title);
        if (image)
            g_free(image);
    }
}


static void thv_row_deleted_signal(
                    GtkTreeModel * model,
                    GtkTreePath * path,
                    gpointer data)
{
    gint *ind = gtk_tree_path_get_indices(path);
    TabButton *tb = g_list_nth_data(thv_list, ind[0]);
    /* Remove from the thv_list */
    thv_list = g_list_remove(thv_list, tb);
    /* Remove from the top bar and also destroy/free the widgets */
    /* remove the extra ref we added */
    g_object_unref(tb->button);
    gtk_widget_destroy(GTK_WIDGET(tb->button));
    /* Free the TabButton structure */
    g_free(tb);
}


/* Sorts TabButton based on positition */
static int thv_sort_func(gconstpointer a, gconstpointer b)
{
    TabButton *tb1 = (TabButton *) a;
    TabButton *tb2 = (TabButton *) b;
    return tb1->pos - tb2->pos;
}


static void thv_row_reordered_signal(GtkTreeModel * model,
        GtkTreePath * path,
        GtkTreeIter * titer,
        gpointer arg3,
        gpointer data)
{
    gint *r = arg3;
    int length = gtk_tree_model_iter_n_children(model, NULL);
    int i;
    GList *iter = NULL;

    /* Fix the position entries, and remove all the buttons from the header */
    for (i = 0; i < length; i++)
    {
        TabButton *tb = g_list_nth_data(thv_list, r[i]);
        tb->pos = i;
        gtk_container_remove(
                    GTK_CONTAINER(
                            gtk_builder_get_object(pl3_xml, "box_tab_bar")),
                    GTK_WIDGET(tb->button));
    }
    /* Add the buttons to the header in the right order */
    thv_list = g_list_sort(thv_list, thv_sort_func);
    for (iter = g_list_first(thv_list); iter; iter = g_list_next(iter))
    {
        TabButton *tb = iter->data;
        gtk_box_pack_start(GTK_BOX
            (gtk_builder_get_object(pl3_xml, "box_tab_bar")),
            GTK_WIDGET(tb->button),
            FALSE,
            TRUE,
            0);
    }
}


/**
 * If the button is clicked,
 * trigger an update by calling a select on the sidebar
 */
static void thv_button_clicked(GtkToggleButton * button, TabButton * tb)
{
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)))
    {
        GtkTreeView *tree = (GtkTreeView*)gtk_builder_get_object(pl3_xml,
                                                            "cat_tree");
        GtkTreeSelection *selec = gtk_tree_view_get_selection(tree);
        GtkTreePath *path = gtk_tree_path_new_from_indices(tb->pos, -1);
        gtk_tree_selection_select_path(selec, path);
        gtk_tree_path_free(path);
    } else
    {
        g_signal_handler_block(G_OBJECT(tb->button), tb->handler);
        gtk_toggle_button_set_active(button, TRUE);
        g_signal_handler_unblock(G_OBJECT(tb->button), tb->handler);
    }
}


static void thv_row_inserted_signal(
                    GtkTreeModel * model,
                    GtkTreePath * path,
                    GtkTreeIter * iter,
                    gpointer data)
{
    TabButton *tb;
    GtkToggleButton *button = (GtkToggleButton *) gtk_toggle_button_new();
    GtkHBox *box = (GtkHBox *) gtk_hbox_new(FALSE, 6);
    gchar *title, *image;
    GtkImage *imagew = (GtkImage *) gtk_image_new();
    GtkLabel *label = NULL;
    gtk_tree_model_get(model, iter, 3, &image, 1, &title, -1);

    label = (GtkLabel *) gtk_label_new(title ? "" : title);

    /* Create new tabbed-button object */
    tb = g_malloc0(sizeof(*tb));
    tb->button = button;

    tb->handler = g_signal_connect(G_OBJECT(button),
                                "toggled",
                                G_CALLBACK(thv_button_clicked), tb);

    /* Create image for in button at menu size */
    if (image)
    {
        gtk_image_set_from_icon_name(imagew, image, GTK_ICON_SIZE_MENU);
    }
    tb->image = imagew;
    /* Add it */
    gtk_box_pack_start(GTK_BOX(box), GTK_WIDGET(imagew), FALSE, TRUE, 0);

    tb->label = label;
    /* Set tooltip on the widget, if there is a title */
    if (title)
    {
        gtk_widget_set_tooltip_text(GTK_WIDGET(label), title);
    }
    /* Ellipsize the label */
    gtk_label_set_ellipsize(label, PANGO_ELLIPSIZE_END);
    /* Align the label to the right */
    gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
    /* Add it to the button */
    gtk_box_pack_start(GTK_BOX(box), GTK_WIDGET(label), TRUE, TRUE, 0);

    /* Add the content to the button */
    gtk_container_add(GTK_CONTAINER(button), GTK_WIDGET(box));

    /* Show everything */
    gtk_widget_show_all(GTK_WIDGET(button));

    /* Add an extra reference to the widget,
     * so when it is removed from the heading
     * box it does not get free'ed
     */
    g_object_ref(G_OBJECT(button));

    /* Add it to the heading */
    gtk_box_pack_start(GTK_BOX(gtk_builder_get_object(pl3_xml, "box_tab_bar")),
                    GTK_WIDGET(button),
                    FALSE,
                    TRUE,
                    0);

    /* Set the position in the TabButton */
    {
        gint *in = gtk_tree_path_get_indices(path);
        tb->pos = in[0];
    }
    /* Add this button to the list */
    thv_list = g_list_insert(thv_list, tb, tb->pos);
    /* hack to fix numbering
     * \TODO: if you insert, only the items after inserting need updating.
     */
    {
        int i = 0;
        GList *si = g_list_first(thv_list);
        for (; si; si = g_list_next(si))
        {
            ((TabButton *) (si->data))->pos = i;
            i++;
        }
    }
    /* Move the button in the correct position */
    gtk_box_reorder_child(
                GTK_BOX(gtk_builder_get_object(pl3_xml, "box_tab_bar")),
                GTK_WIDGET(button),
                tb->pos);

    /* free (possible) results from gtk_tree_model_get */
    if (title)
        g_free(title);
    if (image)
        g_free(image);
}

void thv_init(GtkTreeModel *model)
{
    g_signal_connect(G_OBJECT(model), 
            "row_inserted",
            G_CALLBACK(thv_row_inserted_signal), NULL);
    g_signal_connect(G_OBJECT(model),
            "row_changed",
            G_CALLBACK(thv_row_changed_signal), NULL);
    g_signal_connect(G_OBJECT(model),
            "row_deleted",
            G_CALLBACK(thv_row_deleted_signal), NULL);
    g_signal_connect(G_OBJECT(model),
            "rows_reordered",
            G_CALLBACK(thv_row_reordered_signal), NULL);
}

/* vim: set noexpandtab ts=4 sw=4 sts=4 tw=80: */
