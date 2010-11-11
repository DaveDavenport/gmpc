using Gtk;
using Gdk;
using Cairo;
using Gmpc;

public class Gmpc.Volume : Gtk.DrawingArea
{
    private const string some_unique_name = Config.VERSION;
    private static const int BORDER_WIDTH = 1;
    private static const int DEFAULT_HEIGHT = 32;
    private static const int DEFAULT_WIDTH  = 24;
    private static const double BLOCK_SPACING  = 2;
    private static const int mute_size = 1;            
    private int num_blocks = 8;
    public int _volume_level =0;
    public int volume_level { 
        get { return _volume_level; }
        set {
            int temp = (value > 100)? 100: (value < 0)?0:value;
            if(temp != _volume_level) {
                if(!value_changed(temp)) {
                    _volume_level = temp;
                    if(temp > 0) {
                            this.set_tooltip_text("%s: %i%%".printf(_("Volume"), temp));
                    }else{
                            this.set_tooltip_text("%s: %s".printf(_("Volume"), _("Muted")));
                    }
                    
                    this.queue_draw();
                }
            }
        }
   }

    public signal bool value_changed(int value);

    public Volume()
    {

    }

    construct {
	
        // Enable the events you wish to get notified about.
        add_events (Gdk.EventMask.BUTTON_PRESS_MASK
                  | Gdk.EventMask.BUTTON_RELEASE_MASK
                  | Gdk.EventMask.POINTER_MOTION_MASK
                  | Gdk.EventMask.SCROLL_MASK);
    }

    private void set_volume_level_from_y_pos(double y)
    {
        if(y > BORDER_WIDTH && y < (this.allocation.height-BORDER_WIDTH))
        {   
            double block_height =GLib.Math.floor(
                    (this.allocation.height-BORDER_WIDTH*2)/(double)num_blocks);
            double tvolume_level = 1-(y-BORDER_WIDTH-block_height*0.5)/(block_height*(num_blocks-1));
            this.volume_level = (int)(tvolume_level*100.0);
        }

    }

    /* Scrolling */
    private override bool scroll_event(Gdk.EventScroll event)
    {
        int tvolume_level = _volume_level;
        if(event.direction ==  Gdk.ScrollDirection.UP) {
            volume_level = tvolume_level + 5;
        }else if(event.direction == Gdk.ScrollDirection.DOWN) {
           volume_level  = tvolume_level - 5; 
        }
        return false;
    }
    /* Mouse button got pressed over widget */
    public override bool button_press_event (Gdk.EventButton event) {
        /* Inside X */
        if(event.x > BORDER_WIDTH && event.x < (this.allocation.width-BORDER_WIDTH))
        {
            this.set_volume_level_from_y_pos(event.y);
        }
        return false;
    }

    /* Mouse button got released */
    public override bool button_release_event (Gdk.EventButton event) {
        return false;
    }

    /* Mouse pointer moved over widget */
    public override bool motion_notify_event (Gdk.EventMotion event) {
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
    public override void size_request (out Gtk.Requisition requisition) {
        requisition.width = BORDER_WIDTH*2+DEFAULT_WIDTH; 
        requisition.height =  BORDER_WIDTH*2+DEFAULT_HEIGHT;
    }


    /*
     * This method gets called by Gtk+ when the actual size is known
     * and the widget is told how much space could actually be allocated.
     * It is called every time the widget size changes, for example when the
     * user resizes the window.
     */
    public override void size_allocate (Gdk.Rectangle allocation) {
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
    public override bool expose_event (Gdk.EventExpose event) {
        // Cairo context to draw on
        var cr = Gdk.cairo_create (this.window);

        cr.set_line_width (1.0);
        cr.set_line_join (LineJoin.ROUND);
        Gdk.cairo_set_source_color (cr, this.style.bg[this.state]);
	cr.paint();


        Gdk.cairo_set_source_color (cr, this.style.text[this.state]);


        /* Calculate the height of one block */
        /* Clip it to full intergers so it is pixel aligned. */
        double svol = volume_level/100.0;
        double block_height =GLib.Math.floor(
                (this.allocation.height-BORDER_WIDTH*2)/(double)num_blocks);

        double block_width = this.allocation.width-2*BORDER_WIDTH-BLOCK_SPACING;
        for( int i=0; i < num_blocks;i++) {
            /* get the block volume_level */
            double bvolume_level = (1.0-i/(double)num_blocks);
            int bw = (int)(block_width*(bvolume_level)+BLOCK_SPACING);
            cr.rectangle (
                    this.allocation.width-BORDER_WIDTH-bw+0.5,
                    GLib.Math.ceil(BORDER_WIDTH+block_height*i)+0.5+BLOCK_SPACING,
                    bw,
                    block_height-2*BLOCK_SPACING);

            /* if selected, draw it filled in */
            if(volume_level > 0&& bvolume_level-1/(1.0*num_blocks) <= svol) {
                cr.fill_preserve();
            }
            /* Add border */
            cr.stroke ();
        }
        /* Draw mute */
        if(volume_level == 0)
        {
            cr.arc (
                    0.5+BORDER_WIDTH+mute_size*block_height+4,
                    0.5+this.allocation.height-BORDER_WIDTH-mute_size*block_height-4,
                    mute_size*block_height,
                    0, 2 * GLib.Math.PI);
            cr.set_line_width(3.5);
            cr.stroke_preserve();
            cr.set_line_width(2.0);
            cr.set_source_rgb(1.0,0.0,0.0);
            cr.stroke_preserve();
            cr.clip();
            Gdk.cairo_set_source_color (cr, this.style.text[this.state]);
            cr.new_path ();  /* current path is not
                                     consumed by cairo_clip() */       
            cr.move_to(BORDER_WIDTH+4, this.allocation.height-BORDER_WIDTH-3);
            cr.line_to(BORDER_WIDTH+2*mute_size*block_height+4, 
                    this.allocation.height-BORDER_WIDTH-2*mute_size*block_height-3);
            cr.set_line_width(3.5);
            cr.stroke_preserve();        
            cr.set_line_width(2.0);
            cr.set_source_rgb(1.0,0.0,0.0);
            cr.stroke();

        }

        return true;
    }
}
