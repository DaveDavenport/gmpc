/* Gnome Music Player Client (GMPC)
 * Copyright (C) 2004-2012 Qball Cow <qball@gmpclient.org>
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

/**
 * This gob file provides a hack, so rows in a gtk_list_store can be moved, (with gtk_tree_reorderable enabled)
 * While the gtk_tree_row_reference stays in tracked.
 */

using Gtk;

namespace Gmpc {
namespace Tools
{
    public class ListstoreSort : Gtk.ListStore, Gtk.TreeDragSource ,Gtk.TreeDragDest
    {
        private const bool use_transition = Gmpc.use_transition;

        private 
        bool row_draggable (Gtk.TreePath path)
        {
            return true;
        }
        private
        bool drag_data_get (Gtk.TreePath path, Gtk.SelectionData selection_data)
        {
            return false;
        }

        private
        bool drag_data_delete (Gtk.TreePath path)
        {
            return true;
        }
        private 
        bool drag_data_received(Gtk.TreePath dest, Gtk.SelectionData selection_data)
        {
            Gtk.TreeModel model;
            Gtk.TreePath path = null;

            if(dest == null )
            {
                return false;
            }
            if(Gtk.tree_get_row_drag_data(selection_data, out model, out path))
            {
                Gtk.TreeIter dest_iter, source_iter;
                var dest_v = model.get_iter(out dest_iter, dest);
                var source_v = model.get_iter(out source_iter, path);
                if(source_v)
                {
                    if(dest_v)
                        this.move_before(ref source_iter, dest_iter);
                    else
                        this.move_before(ref source_iter, null);
                }
                return true;
            }
            return false;
        }
		private
		bool row_drop_possible (Gtk.TreePath dest_path, Gtk.SelectionData data)
		{
			if(dest_path != null)
			{
				Gtk.TreePath path = null;
				Gtk.TreeModel model = null;
				if(Gtk.tree_get_row_drag_data(data, out model, out path))
				{
					Gtk.TreeIter iter;
					int source = -1;
					int dest = -1;
					if(model.get_iter(out iter, path)) {
						model.get(iter, 4, out source);	
						if(model.get_iter(out iter, dest_path))
						{
							model.get(iter, 4, out dest);
							// only in current category, and not above the separator!!!
							if(source/1000 == dest/1000 && dest%1000 != 0){
								return true;
							} 
						}

					}	
				}
			}
			return false;
		}
	}
}
}

