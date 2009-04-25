/* Gnome Music Player Client (GMPC)
 * Copyright (C) 2004-2009 Qball Cow <qball@sarine.nl>
 * Project homepage: http://gmpc.wikia.com/
 
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

#include "gmpc-liststore-sort.h"




enum  {
	GMPC_LISTSTORE_SORT_DUMMY_PROPERTY
};
static gboolean gmpc_liststore_sort_real_row_draggable (GtkTreeDragSource* base, const GtkTreePath* path);
static gboolean gmpc_liststore_sort_real_drag_data_get (GtkTreeDragSource* base, const GtkTreePath* path, GtkSelectionData* selection_data);
static gboolean gmpc_liststore_sort_real_drag_data_delete (GtkTreeDragSource* base, const GtkTreePath* path);
static gboolean gmpc_liststore_sort_real_drag_data_received (GtkTreeDragDest* base, const GtkTreePath* dest, GtkSelectionData* selection_data);
static gpointer gmpc_liststore_sort_parent_class = NULL;
static GtkTreeDragSourceIface* gmpc_liststore_sort_gtk_tree_drag_source_parent_iface = NULL;
static GtkTreeDragDestIface* gmpc_liststore_sort_gtk_tree_drag_dest_parent_iface = NULL;



static gboolean gmpc_liststore_sort_real_row_draggable (GtkTreeDragSource* base, const GtkTreePath* path) {
	GmpcListstoreSort * self;
	self = (GmpcListstoreSort*) base;
	g_return_val_if_fail (path != NULL, FALSE);
	return TRUE;
}


static gboolean gmpc_liststore_sort_real_drag_data_get (GtkTreeDragSource* base, const GtkTreePath* path, GtkSelectionData* selection_data) {
	GmpcListstoreSort * self;
	self = (GmpcListstoreSort*) base;
	g_return_val_if_fail (path != NULL, FALSE);
	g_return_val_if_fail (selection_data != NULL, FALSE);
	return FALSE;
}


static gboolean gmpc_liststore_sort_real_drag_data_delete (GtkTreeDragSource* base, const GtkTreePath* path) {
	GmpcListstoreSort * self;
	self = (GmpcListstoreSort*) base;
	g_return_val_if_fail (path != NULL, FALSE);
	return TRUE;
}


static gboolean gmpc_liststore_sort_real_drag_data_received (GtkTreeDragDest* base, const GtkTreePath* dest, GtkSelectionData* selection_data) {
	GmpcListstoreSort * self;
	GtkTreeModel* model;
	GtkTreePath* path;
	const GtkTreePath* _tmp8;
	GtkTreePath* _tmp7;
	gboolean _tmp6;
	const GtkTreePath* _tmp5;
	GtkTreeModel* _tmp4;
	GtkTreeModel* _tmp3;
	gboolean _tmp2;
	GtkTreeModel* _tmp1;
	gboolean _tmp10;
	self = (GmpcListstoreSort*) base;
	g_return_val_if_fail (dest != NULL, FALSE);
	g_return_val_if_fail (selection_data != NULL, FALSE);
	model = NULL;
	path = NULL;
	if (dest == NULL) {
		gboolean _tmp0;
		return (_tmp0 = FALSE, (model == NULL) ? NULL : (model = (g_object_unref (model), NULL)), (path == NULL) ? NULL : (path = (gtk_tree_path_free (path), NULL)), _tmp0);
	}
	_tmp8 = NULL;
	_tmp7 = NULL;
	_tmp5 = NULL;
	_tmp4 = NULL;
	_tmp3 = NULL;
	_tmp1 = NULL;
	if ((_tmp6 = (_tmp2 = gtk_tree_get_row_drag_data (selection_data, &_tmp1, &_tmp5), model = (_tmp3 = (_tmp4 = _tmp1, (_tmp4 == NULL) ? NULL : g_object_ref (_tmp4)), (model == NULL) ? NULL : (model = (g_object_unref (model), NULL)), _tmp3), _tmp2), path = (_tmp7 = (_tmp8 = _tmp5, (_tmp8 == NULL) ? NULL : gtk_tree_path_copy (_tmp8)), (path == NULL) ? NULL : (path = (gtk_tree_path_free (path), NULL)), _tmp7), _tmp6)) {
		GtkTreeIter dest_iter = {0};
		GtkTreeIter source_iter = {0};
		gboolean dest_v;
		gboolean source_v;
		gboolean _tmp9;
		dest_v = gtk_tree_model_get_iter (model, &dest_iter, dest);
		source_v = gtk_tree_model_get_iter (model, &source_iter, path);
		if (source_v) {
			if (dest_v) {
				gtk_list_store_move_before ((GtkListStore*) self, &source_iter, &dest_iter);
			} else {
				gtk_list_store_move_before ((GtkListStore*) self, &source_iter, NULL);
			}
		}
		return (_tmp9 = TRUE, (model == NULL) ? NULL : (model = (g_object_unref (model), NULL)), (path == NULL) ? NULL : (path = (gtk_tree_path_free (path), NULL)), _tmp9);
	}
	return (_tmp10 = FALSE, (model == NULL) ? NULL : (model = (g_object_unref (model), NULL)), (path == NULL) ? NULL : (path = (gtk_tree_path_free (path), NULL)), _tmp10);
}


GmpcListstoreSort* gmpc_liststore_sort_construct (GType object_type) {
	GmpcListstoreSort * self;
	self = g_object_newv (object_type, 0, NULL);
	return self;
}


GmpcListstoreSort* gmpc_liststore_sort_new (void) {
	return gmpc_liststore_sort_construct (GMPC_LISTSTORE_TYPE_SORT);
}


static void gmpc_liststore_sort_class_init (GmpcListstoreSortClass * klass) {
	gmpc_liststore_sort_parent_class = g_type_class_peek_parent (klass);
}


static void gmpc_liststore_sort_gtk_tree_drag_source_interface_init (GtkTreeDragSourceIface * iface) {
	gmpc_liststore_sort_gtk_tree_drag_source_parent_iface = g_type_interface_peek_parent (iface);
	iface->row_draggable = gmpc_liststore_sort_real_row_draggable;
	iface->drag_data_get = gmpc_liststore_sort_real_drag_data_get;
	iface->drag_data_delete = gmpc_liststore_sort_real_drag_data_delete;
}


static void gmpc_liststore_sort_gtk_tree_drag_dest_interface_init (GtkTreeDragDestIface * iface) {
	gmpc_liststore_sort_gtk_tree_drag_dest_parent_iface = g_type_interface_peek_parent (iface);
	iface->drag_data_received = gmpc_liststore_sort_real_drag_data_received;
}


static void gmpc_liststore_sort_instance_init (GmpcListstoreSort * self) {
}


GType gmpc_liststore_sort_get_type (void) {
	static GType gmpc_liststore_sort_type_id = 0;
	if (gmpc_liststore_sort_type_id == 0) {
		static const GTypeInfo g_define_type_info = { sizeof (GmpcListstoreSortClass), (GBaseInitFunc) NULL, (GBaseFinalizeFunc) NULL, (GClassInitFunc) gmpc_liststore_sort_class_init, (GClassFinalizeFunc) NULL, NULL, sizeof (GmpcListstoreSort), 0, (GInstanceInitFunc) gmpc_liststore_sort_instance_init, NULL };
		static const GInterfaceInfo gtk_tree_drag_source_info = { (GInterfaceInitFunc) gmpc_liststore_sort_gtk_tree_drag_source_interface_init, (GInterfaceFinalizeFunc) NULL, NULL};
		static const GInterfaceInfo gtk_tree_drag_dest_info = { (GInterfaceInitFunc) gmpc_liststore_sort_gtk_tree_drag_dest_interface_init, (GInterfaceFinalizeFunc) NULL, NULL};
		gmpc_liststore_sort_type_id = g_type_register_static (GTK_TYPE_LIST_STORE, "GmpcListstoreSort", &g_define_type_info, 0);
		g_type_add_interface_static (gmpc_liststore_sort_type_id, GTK_TYPE_TREE_DRAG_SOURCE, &gtk_tree_drag_source_info);
		g_type_add_interface_static (gmpc_liststore_sort_type_id, GTK_TYPE_TREE_DRAG_DEST, &gtk_tree_drag_dest_info);
	}
	return gmpc_liststore_sort_type_id;
}




