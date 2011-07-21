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

using Config;
using Gtk;
using Gmpc;

private const bool use_transition_gav = Gmpc.use_transition;
private const string some_unique_name_gav = Config.VERSION;
[compact]
private class AlbumviewEntry 
{
    public enum Type { 
        HEADER,
        ITEM
    }
    public  Type type;
    public  weak Gtk.Widget widget;

}

public class Gmpc.Widget.Albumview : Gtk.Container
{
    private int cover_width_real     = 220;
    private int cover_height_real    = 220+30;
    private int header_height_real   = 150;
    private int num_items       = 0;
    private int columns         = 3;


    private List<AlbumviewEntry> children = null;

    /** Accessor */
    public void set_cover_size(int width, int height) 
    {
        cover_width_real = width;
        cover_height_real = height;
        this.queue_resize();
    }
    public void set_header_size(int height)
    {
        header_height_real = height;
        this.queue_resize();
    }

    public Albumview()
    {
        this.set_has_window(false);
        this.set_redraw_on_allocate(false);
    }

    ~Albumview()
    {
    }

	/**
     * Calculates the size of the widget. 
	 */
	public override void size_request(out Gtk.Requisition req)
    {
        req = Gtk.Requisition();
		int cover_width = cover_width_real; 
		int cover_height= cover_height_real; 
		int header_height = header_height_real; 
        /* request minimum of 1 column */
        int width = cover_width_real*1; 
        int items = 0;
		/* determine max width/height */
		foreach ( var child in children)
		{
			if(child.widget.get_visible())
			{
				if(child.type == AlbumviewEntry.Type.ITEM) {
					Gtk.Requisition cr = {0,0};
					child.widget.size_request(out cr);
					cover_width = int.max(cr.width,cover_width);
					cover_height = int.max(cr.height,cover_height);
				}else{
					Gtk.Requisition cr = {0,0};
					child.widget.size_request(out cr);
					width = int.max(cr.width,width);
					header_height = int.max(cr.height,header_height);
				}
			}
		}

        int rows = 0;
		foreach ( var child in children)
		{
			if(child.widget.get_visible())
			{
				if(child.type == AlbumviewEntry.Type.ITEM) {
					items++;
				}else{
					if(items != 0)
					{
						int nrows = items/columns;
						int remain = (items%columns >0)?1:0;
						rows = rows + (nrows+remain)*cover_height; 
						rows+=cover_height;
					}
					items = 0;
				}
			}
		}
		if(items != 0)
		{
			int nrows = items/columns;
			int remain = (items%columns >0)?1:0;
			rows = rows + (nrows+remain)*cover_height; 
		}

        req.width =  width;
        req.height = rows; 
    }

    public override void add(Gtk.Widget widget)
    {
        if(widget != null) {
            AlbumviewEntry a = new AlbumviewEntry();
            a.type = AlbumviewEntry.Type.ITEM;
            a.widget = widget;
            children.append(a);
            widget.set_parent(this);
            num_items++;
            this.queue_resize();
        }
    }
    public void add_header(Gtk.Widget widget)
    {
        if(widget != null) {
            AlbumviewEntry a = new AlbumviewEntry();
            a.type = AlbumviewEntry.Type.HEADER;
            a.widget = widget;
            children.append(a);
            widget.set_parent(this);
            num_items++;
            this.queue_resize();
        }
    }

    public override GLib.Type child_type()
    {
        return typeof(Gtk.Widget);
    }
    public override void remove(Gtk.Widget widget)
    {
        if(widget != null ) {
            AlbumviewEntry a = null; 
            /*
               if((a = children.find_custom(widget,compare)) == null) {
               GLib.error("Failed to find widget in container");
               }
             */
            foreach(AlbumviewEntry f in this.children) {
                if(f.widget == widget) {
                    a = f;
                    break;
                }
            }
            if(a == null) 
                GLib.error("Failed to find widget in container");
            bool visible = widget.get_visible();
            widget.unparent();

            /* owned is needed to avoid leak */
            children.remove((owned)a);
            num_items--;
            if(visible) 
                this.queue_resize();
        }
    }
    public override void size_allocate(Gdk.Rectangle alloc)
    {
		int cover_width = cover_width_real; 
		int cover_height= cover_height_real; 
		int header_height = header_height_real; 
        /* Hack to avvoid pointless resizes, I get this "1" size when a child widget changes */
        if(alloc.width == 1) return;
		int width = alloc.width;
        int new_columns = 0; 
        int rows = 0;
        int item = 0;

		foreach ( var child in children)
		{
			if(child.widget.get_visible())
			{

				if(child.type == AlbumviewEntry.Type.ITEM) {
					Gtk.Requisition cr = {0,0};
					child.widget.size_request(out cr);
					cover_width = int.max(cr.width,cover_width);
					cover_height = int.max(cr.height,cover_height);
					item++;
				}else{
					Gtk.Requisition cr = {0,0};
					child.widget.size_request(out cr);
					item = 0;

					width = int.max(cr.width,width);
					header_height = int.max(cr.height,header_height);
				}
			}
		}
		new_columns = int.max(width/cover_width, 1);
		item = 0;
		rows = 0;
		foreach ( var child in children)
		{
            if(child.widget.get_visible())
            {
                if(child.type == AlbumviewEntry.Type.ITEM) {
                    Gdk.Rectangle ca = {0,0,0,0};
                    ca.x = alloc.x + (item%columns)*cover_width;
                    ca.y = rows+alloc.y + (item/columns)*cover_height;
                    ca.width = cover_width;
                    ca.height = cover_height;

                    child.widget.size_allocate(ca);
                    item++;
                }else{
                    if(item != 0)
                    {
						int nrows = item/columns;
						int remain = (item%columns >0)?1:0;
						rows = rows + (nrows+remain)*cover_height; 
                    }
                    item = 0;

                    Gdk.Rectangle ca = {0,0,0,0};
                    ca.x = alloc.x; 
                    ca.y = alloc.y+rows;
                    ca.width = cover_width*columns;
                    ca.height = header_height;

                    child.widget.size_allocate(ca);
                    rows+=header_height;
                }
            }
        }
        if(new_columns != columns) {
            columns = new_columns;
            this.queue_resize();
        }
    }
    public override void forall_internal(bool include_internals, Gtk.Callback callback) 
    {
        weak List<AlbumviewEntry> iter = children.first();
        /* Somehow it fails when doing a foreach() construction, weird vala bug I guess */
        /* would be  nice if I could filter out say only the visible ones */
        while(iter != null) {
            weak AlbumviewEntry child = iter.data;
            iter = iter.next;
            callback(child.widget);
        }
    }
    public void clear()
    {
        foreach(var a in children) {
            a.widget.unparent();
            num_items--;
        }
        children = null;
        this.queue_resize();
    }
}
