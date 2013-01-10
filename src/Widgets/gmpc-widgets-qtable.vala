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

using Config;
using Gtk;
using Gmpc;

private const bool use_transition_gav = Gmpc.use_transition;
private const string some_unique_name_gav = Config.VERSION;
[compact]
private class QtableEntry
{
    public enum Type
    {
        HEADER,
        ITEM
    }
    public  Type type;
    public  weak Gtk.Widget widget;

}

public class Gmpc.Widgets.Qtable : Gtk.Container, Gtk.Buildable
{
    private int item_width_real     = 0;
    private int item_height_real    = 0;
    private int header_height_real  = 0;
    private int max_columns_real    = 0;
    private int num_items           = 0;
    private int columns             = 3;
    public  int spacing             {get; set; default=8;}
    public  int padding_left        {get; set; default=0;}
    public  int padding_right       {get; set; default=0;}

    public int max_columns
    {
        get {
            return max_columns_real;
        }
        set {
            max_columns_real = value;
            this.queue_resize();
        }
    }

    public int item_width
    {
        get {
            return item_width_real;
        }
        set {
            item_width_real = value;
            this.queue_resize();
        }
    }

    public int item_height
    {
        get {
            return item_height_real;
        }
        set {
            item_height_real = value;
            this.queue_resize();
        }
    }
    public int header_height
    {
        get {
            return header_height_real;
        }
        set {
            header_height_real = value;
            this.queue_resize();
        }
    }


    private List<QtableEntry> children = null;
    /* GtkBuildable override */
    public void add_child(Gtk.Builder build, GLib.Object child, string? type)
    {
        if(!(child is Gtk.Widget))
        {
            GLib.warning("Trying to add non widget");
            return;
        }
        if(type != null && type == "header")
        {
            add_header(child as Gtk.Widget);
        }
        else
        {
            add(child as Gtk.Widget);
        }
    }


    construct
    {
        this.set_has_window(false);
        this.set_redraw_on_allocate(false);
    }

    public Qtable()
    {
        // Set resize mode.
        this.resize_mode = Gtk.ResizeMode.QUEUE;
    }

    /**
     * Calculates the size of the widget.
     */
    public override void get_preferred_height_for_width(int actual_width, out int width, out int nat_width)
    {
        int cover_width = item_width_real;
        int cover_height= item_height_real;
        int header_height = header_height_real;
        int items = 0;

        /* determine max width/height */
        foreach ( var child in children)
        {
            if(child.widget.get_visible())
            {
                int w,h;
                if(child.type == QtableEntry.Type.ITEM)
                {
                    child.widget.get_size_request(out w, out h);
                    cover_width = int.max(w,cover_width);
                    cover_height = int.max(h,cover_height);
                }
                else
                {
                    child.widget.get_size_request(out w, out h);
                    width = int.max(w,width);
                    header_height = int.max(h,header_height);
                }
            }
        }
        if(spacing>0)
        {
            cover_width		+= spacing;
            cover_height	+= spacing;
            header_height	+= spacing;
        }
        int new_columns =  actual_width/cover_width;
        int rows = 0;
        foreach ( var child in children)
        {
            if(child.widget.get_visible())
            {
                if(child.type == QtableEntry.Type.ITEM)
                {
                    items++;
                }
                else
                {
                    if(items != 0)
                    {
                        int nrows = items/new_columns;
                        int remain = (items%new_columns >0)?1:0;
                        rows = rows + (nrows+remain)*cover_height;
                    }
                    items = 0;
                    rows+=header_height;
                }
            }
        }
        if(items != 0)
        {
            int nrows = items/new_columns;
            int remain = (items%new_columns >0)?1:0;
            rows = rows + (nrows+remain)*cover_height;
        }
        /* Width of one column */
        width =  cover_width*new_columns;
        nat_width = width;
        if(columns != new_columns ){
            columns = new_columns;
            this.queue_resize();
        }
    }

    public override void add(Gtk.Widget widget)
    {
        if(widget != null)
        {
            QtableEntry a = new QtableEntry();
            a.type = QtableEntry.Type.ITEM;
            a.widget = widget;
            children.append(a);
            widget.set_parent(this);
            num_items++;
            this.queue_resize();
        }
    }
    public void add_header(Gtk.Widget widget)
    {
        if(widget != null)
        {
            QtableEntry a = new QtableEntry();
            a.type = QtableEntry.Type.HEADER;
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
        if(widget != null )
        {
            QtableEntry a = null;
            /*
               if((a = children.find_custom(widget,compare)) == null) {
               GLib.error("Failed to find widget in container");
               }
             */
            foreach(QtableEntry f in this.children)
            {
                if(f.widget == widget)
                {
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
    public override void size_allocate(Gtk.Allocation alloc)
    {
        int cover_width = item_width_real;
        int cover_height= item_height_real;
        int header_height = header_height_real;
        /* Hack to avvoid pointless resizes,
         * I get this "1" size when a child widget changes */
        if(alloc.width == 1) return;
        int width = alloc.width;
        int new_columns = 0;
        int rows = 0;
        int item = 0;
        // This fixes it so the correct taborder is calculated.
        this.set_allocation(alloc);
        foreach ( var child in children)
        {
            if(child.widget.get_visible())
            {

                if(child.type == QtableEntry.Type.ITEM)
                {
                    Gtk.Requisition cr = {0,0};
                    child.widget.get_preferred_size(out cr, null);
                    cover_width = int.max(cr.width,cover_width);
                    cover_height = int.max(cr.height,cover_height);
                    item++;
                }
                else
                {
                    Gtk.Requisition cr = {0,0};
                    child.widget.get_preferred_size(out cr, null);
                    item = 0;

                    width = int.max(cr.width,width);
                    header_height = int.max(cr.height,header_height);
                }
            }
        }
        if(spacing>0)
        {
            cover_width		+= spacing;
            cover_height	+= spacing;
            header_height	+= spacing;
        }
        new_columns = int.max((width-padding_left - padding_right+spacing)/cover_width, 1);
        if(max_columns_real > 0)
        {
            new_columns = int.min(new_columns,max_columns_real);
        }

        item = 0;
        foreach ( var child in children)
        {
            if(child.widget.get_visible())
            {
                if(child.type == QtableEntry.Type.ITEM)
                {
                    Gtk.Allocation ca = {0,0,0,0};
                    ca.x = alloc.x + (item%new_columns)*cover_width+padding_left;
                    ca.y = rows+alloc.y + (item/new_columns)*cover_height;
                    ca.width = cover_width - spacing;
                    ca.height = cover_height - spacing;

                    child.widget.size_allocate(ca);
                    item++;
                }
                else
                {
                    if(item != 0)
                    {
                        int nrows = item/new_columns;
                        int remain = (item%new_columns >0)?1:0;
                        rows = rows + (nrows+remain)*cover_height;
                    }
                    item = 0;

                    Gtk.Allocation ca = {0,0,0,0};
                    ca.x = alloc.x-padding_left;
                    ca.y = alloc.y+rows;
                    ca.width = cover_width*new_columns;
                    ca.height = header_height - spacing;

                    child.widget.size_allocate(ca);
                    rows+=header_height;
                }
            }
        }
        if(new_columns != columns)
        {
            columns = new_columns;
            this.queue_resize();
        }
    }
    public override void forall_internal(bool include_internals,
                                         Gtk.Callback callback)
    {
        weak List<QtableEntry> iter = children.first();
        /* Somehow it fails when doing a foreach() construction,
            weird vala bug I guess */
        /* would be  nice if I could filter out say only the visible ones */
        while(iter != null)
        {
            weak QtableEntry child = iter.data;
            iter = iter.next;
            callback(child.widget);
        }
    }
    public void clear()
    {
        foreach(var a in children)
        {
            a.widget.unparent();
            num_items--;
        }
        children = null;
        this.queue_resize();
    }


    public override Gtk.SizeRequestMode get_request_mode ()
    {
        return Gtk.SizeRequestMode.HEIGHT_FOR_WIDTH;
    }
}
