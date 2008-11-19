using GLib;
using Gtk;
using Gdk;
using Cairo;


public class Gmpc.Progress : Gtk.EventBox 
{
    private uint total              = 0;
    private uint current            = 0;
    private bool _do_countdown      = false;
    private Pango.Layout _layout    = null;
    public bool _hide_text          = false;

    public bool hide_text {
        get { 
        return _hide_text; }
        set {
        
            _hide_text = value; 
            this.queue_resize();
        }
    }

    public bool do_countdown {
        get { 
            return _do_countdown; }
        set {
            _do_countdown = value; 
            this.redraw();
        }
    }

    /* Construct function */
    construct {
        this.app_paintable = true;
        this.expose_event += this.on_expose2;
        /* Set a string so we can get height */
        this._layout = this.create_pango_layout (" ");
    }
    // The size_request method Gtk+ is calling on a widget to ask
    // it the widget how large it wishes to be. It's not guaranteed
    // that gtk+ will actually give this size to the widget
    public override void size_request (Gtk.Requisition requisition)
    {
        int width, height;
        // In this case, we say that we want to be as big as the
        // text is, plus a little border around it.
        if(this.hide_text)
        {
            requisition.width = 40;
            requisition.height = 10;
        }else{
            this._layout.get_size (out width, out height);
            requisition.width = width / Pango.SCALE + 6;
            requisition.height = height / Pango.SCALE + 6;
        }
    }
    private void draw_curved_rectangle(Context ctx, double rect_x0, double rect_y0, double rect_width, double rect_height) 
    {
        double rect_x1,rect_y1;
        double radius = 10;//rect_width/5;
        rect_x1=rect_x0+rect_width;
        rect_y1=rect_y0+rect_height;
        if (rect_width == 0 || rect_height == 0)
            return;
        if (rect_width/2<radius) {
            if (rect_height/2<radius) {
                ctx.move_to  (rect_x0, (rect_y0 + rect_y1)/2);
                ctx.curve_to (rect_x0 ,rect_y0, rect_x0, rect_y0, (rect_x0 + rect_x1)/2, rect_y0);
                ctx.curve_to (rect_x1, rect_y0, rect_x1, rect_y0, rect_x1, (rect_y0 + rect_y1)/2);
                ctx.curve_to (rect_x1, rect_y1, rect_x1, rect_y1, (rect_x1 + rect_x0)/2, rect_y1);
                ctx.curve_to (rect_x0, rect_y1, rect_x0, rect_y1, rect_x0, (rect_y0 + rect_y1)/2);
            } else {
                ctx.move_to  ( rect_x0, rect_y0 + radius);
                ctx.curve_to ( rect_x0 ,rect_y0, rect_x0, rect_y0, (rect_x0 + rect_x1)/2, rect_y0);
                ctx.curve_to ( rect_x1, rect_y0, rect_x1, rect_y0, rect_x1, rect_y0 + radius);
                ctx.line_to ( rect_x1 , rect_y1 - radius);
                ctx.curve_to ( rect_x1, rect_y1, rect_x1, rect_y1, (rect_x1 + rect_x0)/2, rect_y1);
                ctx.curve_to ( rect_x0, rect_y1, rect_x0, rect_y1, rect_x0, rect_y1- radius);
            }
        }
        else 
        {
            if (rect_height/2<radius) {
                ctx.move_to  ( rect_x0, (rect_y0 + rect_y1)/2);
                ctx.curve_to ( rect_x0 , rect_y0, rect_x0 , rect_y0, rect_x0 + radius, rect_y0);
                ctx.line_to ( rect_x1 - radius, rect_y0);
                ctx.curve_to ( rect_x1, rect_y0, rect_x1, rect_y0, rect_x1, (rect_y0 + rect_y1)/2);
                ctx.curve_to ( rect_x1, rect_y1, rect_x1, rect_y1, rect_x1 - radius, rect_y1);
                ctx.line_to ( rect_x0 + radius, rect_y1);
                ctx.curve_to ( rect_x0, rect_y1, rect_x0, rect_y1, rect_x0, (rect_y0 + rect_y1)/2);
            } else {
                ctx.move_to  ( rect_x0, rect_y0 + radius);
                ctx.curve_to ( rect_x0 , rect_y0, rect_x0 , rect_y0, rect_x0 + radius, rect_y0);
                ctx.line_to ( rect_x1 - radius, rect_y0);
                ctx.curve_to ( rect_x1, rect_y0, rect_x1, rect_y0, rect_x1, rect_y0 + radius);
                ctx.line_to ( rect_x1 , rect_y1 - radius);
                ctx.curve_to ( rect_x1, rect_y1, rect_x1, rect_y1, rect_x1 - radius, rect_y1);
                ctx.line_to ( rect_x0 + radius, rect_y1);
                ctx.curve_to ( rect_x0, rect_y1, rect_x0, rect_y1, rect_x0, rect_y1- radius);
            }
        }

        ctx.close_path();
    }
    private void redraw ()
    {
        if(this.window  != null)
        {
//            this.queue_draw();
            this.window.process_updates(false);
        }
    }

    private bool on_expose2 (Progress pb, Gdk.EventExpose event) 
    {
        var ctx = Gdk.cairo_create(this.window); 
        int width = this.allocation.width-3;
        int height = this.allocation.height-3;
        int pw = width-3;
        int pwidth = (int)((this.current*pw)/(double)this.total);
        if(pwidth > pw)
        {
            pwidth = pw;
        }
        /* Draw border */
        ctx.set_line_width ( 1.0 );
        ctx.set_tolerance ( 0.2 );
        ctx.set_line_join (LineJoin.ROUND);

        //paint background
        Gdk.cairo_set_source_color(ctx, pb.style.bg[(int)Gtk.StateType.NORMAL]);
        ctx.paint();


        ctx.new_path();
        /* Stroke a white line, and clip on that */
        Gdk.cairo_set_source_color(ctx, pb.style.dark[(int)Gtk.StateType.NORMAL]);
        draw_curved_rectangle(ctx, 1.5,1.5,width, height);
        ctx.stroke_preserve ();
        /* Make a clip */
        ctx.clip();



        if(this.total > 0)
        {

            /* don't allow more then 100% */
            if( pwidth > width ) {
                pwidth = width;
            }
            ctx.new_path();
            Gdk.cairo_set_source_color(ctx, pb.style.bg[(int)Gtk.StateType.SELECTED]);
            draw_curved_rectangle(ctx,1.5+2,1.5+2,pwidth, (height-4));
            ctx.fill ();

        }
        /* Paint nice reflection layer on top */
        ctx.new_path();
        var pattern =  new Pattern.linear(0.0,0.0, 0.0, height);
        var start = pb.style.dark[(int)Gtk.StateType.NORMAL];
        var stop = pb.style.light[(int)Gtk.StateType.NORMAL];

        pattern.add_color_stop_rgba(0.0,start.red/(65536.0), start.green/(65536.0), start.blue/(65536.0),   0.6);
        pattern.add_color_stop_rgba(0.40,stop.red/(65536.0), stop.green/(65536.0), stop.blue/(65536.0),      0.2);
        pattern.add_color_stop_rgba(0.551,stop.red/(65536.0), stop.green/(65536.0), stop.blue/(65536.0),   0.0);
        ctx.set_source(pattern);
        ctx.rectangle(1.5,1.5,width, height);

        ctx.fill();
        ctx.reset_clip();
        /**
         * Draw text
         */
        if(this.hide_text == false)
        {

            int fontw, fonth;
            if(this.total == 0) {
                string a;
                if(this.current/60 > 99 ) {
                    a = "%02i:%02i".printf(
                            (int)this.current/3600,
                            (int)(this.current)%60);
                } else {
                    a = "%02i:%02i".printf( 
                            (int)this.current/60,
                            (int)(this.current)%60);
                }
                this._layout.set_text(a,-1);
            } else {
                string a;
                uint p = this.current;
                if(this.do_countdown){
                    p = this.total-this.current;
                }
                if(this.current/60 > 99 ) {
                    a  = "%c%02u:%02u - %02u:%02u".printf( 
                            (this.do_countdown)?'-':' ',
                            p/3600,
                            (p)%60,
                            this.total/3600,
                            (this.total)%60 
                            );
                } else {
                    a = "%c%02u:%02u - %02u:%02u".printf( 
                            (this.do_countdown)?'-':' ',
                            p/60,
                            (p)%60,
                            this.total/60,
                            (this.total)%60 
                            );
                }                                       
                this._layout.set_text(a,-1);
            }

            Pango.cairo_update_layout (ctx, this._layout);
            this._layout.get_pixel_size (out fontw, out fonth);

            if(this.total > 0)
            {

                if(pwidth >= ((width-fontw)/2+1))
                {
                    ctx.new_path();
                    Gdk.cairo_set_source_color(ctx, pb.style.fg[(int)Gtk.StateType.SELECTED]);
                    ctx.rectangle(3.5, 1.5,pwidth, height);
                    ctx.clip();
                    ctx.move_to ((width - fontw)/2+1.5,
                            (height - fonth)/2+1.5);
                    Pango.cairo_show_layout ( ctx, this._layout);
                    ctx.reset_clip();
                }
                if(pwidth < ((width-fontw)/2+1+fontw))
                {
                    ctx.new_path();
                    Gdk.cairo_set_source_color(ctx, pb.style.fg[(int)Gtk.StateType.NORMAL]);
                    ctx.rectangle(pwidth+3.5, 1.5,width, height);
                    ctx.clip();
                    ctx.move_to ((width - fontw)/2+1.5,
                            (height - fonth)/2+1.5);
                    Pango.cairo_show_layout ( ctx, this._layout);
                }

            }
            else
            {
                ctx.new_path();
                Gdk.cairo_set_source_color(ctx, pb.style.fg[(int)Gtk.StateType.NORMAL]);
                ctx.move_to ((width - fontw)/2+1.5,
                        (height - fonth)/2+1.5);
                Pango.cairo_show_layout ( ctx, this._layout);
            }
        }
        return true;
    }

    public void set_time(uint total, uint current)
    {
        if(this.total != total || this.current != current)
        {
            this.total = total;
            this.current = current;
            this.queue_draw();
        }
    }
}
