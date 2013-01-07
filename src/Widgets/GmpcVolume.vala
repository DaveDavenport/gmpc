using Gtk;
using Gdk;
using Cairo;
using Gmpc;

public class Gmpc.Widgets.Volume : Gtk.EventBox
{
    private const string some_unique_name = Config.VERSION;
    private static const int BORDER_WIDTH = 1;
    private static const int DEFAULT_HEIGHT = 32;
    private static const int DEFAULT_WIDTH  = 24;
    private static const double BLOCK_SPACING  = 2;
    private static const int mute_size = 1;
    private int num_blocks = 8;
    public int _volume_level =0;


    public int volume_level
    {
        get { return _volume_level; }
        set {
            if(value != _volume_level)
            {
                if(!value_changed(value))
                {
                    _volume_level = value;
                    if(value > 0)
                    {
                        this.set_tooltip_text("%s: %i%%".printf(_("Volume"), value));
                    }
                    else
                    {
                        this.set_tooltip_text("%s: %s".printf(_("Volume"), _("Muted")));
                    }

                    this.queue_draw();
                    check_state();
                }
            }
        }
    }

    public signal bool value_changed(int value);

    public Volume()
    {

    }

    construct
    {
        this.set_app_paintable(true);
        this.set_has_window(false);
        this.set_no_show_all(true);
        // Enable the events you wish to get notified about.
        add_events (Gdk.EventMask.BUTTON_PRESS_MASK
        | Gdk.EventMask.BUTTON_RELEASE_MASK
        | Gdk.EventMask.POINTER_MOTION_MASK
        | Gdk.EventMask.SCROLL_MASK);
        this.scroll_event.connect((source, event) => {
            return this.b_scroll_event(event);
        });
    }

    /**
     * Check if we need to show/hide the plugin.
     */
    private void check_state()
    {
        if(_volume_level < 0) {
            this.hide();
        }else{
            this.show();
        }
    }

    private void set_volume_level_from_y_pos(double y)
    {
        if(y > BORDER_WIDTH && y < (this.get_allocated_height()-BORDER_WIDTH))
        {
            double block_height =GLib.Math.floor(
                                     (this.get_allocated_height()-BORDER_WIDTH*2)/(double)num_blocks);
            double tvolume_level = 1-(y-BORDER_WIDTH-block_height*0.5)/(block_height*(num_blocks-1));
            this.volume_level = (int)(tvolume_level*100.0);
        }

    }

    /* Scrolling */
    private bool b_scroll_event(Gdk.EventScroll event)
    {
        int tvolume_level = _volume_level;
        int scroll_step;
        if((event.state&Gdk.ModifierType.MOD1_MASK) == Gdk.ModifierType.MOD1_MASK)
        {
            scroll_step = config.get_int_with_default("Volume", "scroll-sensitivity-fine", 1);
        } else {
            scroll_step = config.get_int_with_default("Volume", "scroll-sensitivity", 5);
        }
        if(event.direction ==  Gdk.ScrollDirection.UP)
        {
            volume_level = tvolume_level + scroll_step;
        }
        else if(event.direction == Gdk.ScrollDirection.DOWN)
        {
            volume_level  = tvolume_level - scroll_step;
        }
        return false;
    }

    /* Mouse button got pressed over widget */
    public override bool button_press_event (Gdk.EventButton event)
    {
        if (!(bool)config.get_int_with_default("Volume", "clickable", 1))
        {
            return false;
        }

        /* Inside X */
        if(event.x > BORDER_WIDTH && event.x < (this.get_allocated_width()-BORDER_WIDTH))
        {
            this.set_volume_level_from_y_pos(event.y);
        }
        return false;
    }

    /* Mouse button got released */
    public override bool button_release_event (Gdk.EventButton event)
    {
        return false;
    }

    /* Mouse pointer moved over widget */
    public override bool motion_notify_event (Gdk.EventMotion event)
    {
        if (!(bool)config.get_int_with_default("Volume", "clickable", 1))
        {
            return false;
        }

        if((event.state&Gdk.ModifierType.BUTTON1_MASK) == Gdk.ModifierType.BUTTON1_MASK)
        {
            this.set_volume_level_from_y_pos(event.y);
        }

        return false;
    }

    /*
     * This method Gtk+ is calling on a widget to ask
     * the widget how large it wishes to be. It's not guaranteed
     * that Gtk+ will actually give this size to the widget.
     */
/*
    public override void size_request (out Gtk.Requisition requisition)
    {
        requisition.width = BORDER_WIDTH*2+DEFAULT_WIDTH;
        requisition.height =  BORDER_WIDTH*2+DEFAULT_HEIGHT;
    }
*/

    /*
     * This method gets called by Gtk+ when the actual size is known
     * and the widget is told how much space could actually be allocated.
     * It is called every time the widget size changes, for example when the
     * user resizes the window.
     */
    public override void size_allocate (Gtk.Allocation allocation)
    {
        // The base method will save the allocation and move/resize the
        // widget's GDK window if the widget is already realized.
        base.size_allocate (allocation);

        // Calculate how much blocks we can have visible.
        num_blocks = int.max((int)GLib.Math.ceil(allocation.height/(8.0)),4);
    }

    /*
     * This method is called when the widget is asked to draw itself.
     * Remember that this will be called a lot of times, so it's usually
     * a good idea to write this code as optimized as it can be, don't
     * create any resources in here.
     */
    public override bool draw (Cairo.Context cr)
    {
        // if volume level is disabled, don't draw anything.
        if(volume_level < 0) return true;

        cr.set_line_width (1.0);
        cr.set_line_join (LineJoin.ROUND);

        Gdk.cairo_set_source_color (cr, this.style.fg[this.get_state()]);


        /* Calculate the height of one block */
        /* Clip it to full intergers so it is pixel aligned. */
        double svol = volume_level/100.0;
        double block_height =GLib.Math.floor(
                                 (this.get_allocated_height()-BORDER_WIDTH*2)/(double)num_blocks);

        double block_width = this.get_allocated_width()-2*BORDER_WIDTH-BLOCK_SPACING;
        Gtk.Allocation alloc;
        this.get_allocation(out alloc);
        for( int i=0; i < num_blocks; i++)
        {
            /* get the block volume_level */
            double bvolume_level = (1.0-i/(double)num_blocks);
            int bw = (int)(block_width*(bvolume_level)+BLOCK_SPACING);
            cr.rectangle (
                alloc.x + alloc.width-BORDER_WIDTH-bw+0.5,
                alloc.y +GLib.Math.ceil(BORDER_WIDTH+block_height*i)+0.5+BLOCK_SPACING,
                bw,
                block_height-2*BLOCK_SPACING
            );

            /* if selected, draw it filled in */
            if(volume_level > 0&& bvolume_level-1/(1.0*num_blocks) <= svol)
            {
                cr.fill_preserve();
            }
            /* Add border */
            cr.stroke ();
        }
        /* Draw mute */
        if(volume_level == 0)
        {
            cr.arc (
                alloc.x +0.5+BORDER_WIDTH+mute_size*block_height+4,
                alloc.y +0.5+alloc.height-BORDER_WIDTH-mute_size*block_height-4,
                mute_size*block_height,
                0, 2 * GLib.Math.PI
            );

            cr.set_line_width(3.5);
            cr.stroke_preserve();
            cr.set_line_width(2.0);
            cr.set_source_rgb(1.0,0.0,0.0);
            cr.stroke_preserve();
            cr.clip();
            Gdk.cairo_set_source_color (cr, this.style.fg[this.get_state()]);
            cr.new_path ();  /* current path is not
                                     consumed by cairo_clip() */
            cr.move_to(
                alloc.x + BORDER_WIDTH+4,
                alloc.y + alloc.height-BORDER_WIDTH-3
            );
            cr.line_to(
                alloc.x +BORDER_WIDTH+2*mute_size*block_height+4,
                alloc.y + alloc.height-BORDER_WIDTH-2*mute_size*block_height-3
            );
            cr.set_line_width(3.5);
            cr.stroke_preserve();
            cr.set_line_width(2.0);
            cr.set_source_rgb(1.0,0.0,0.0);
            cr.stroke();

        }

        return true;
    }
}
