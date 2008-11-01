using GLib;
using Gtk;
using Gdk;
using Cairo;


public class Gmpc.Progress : Gtk.EventBox {
    private uint total = 0;
    private uint current = 0;
    private Pango.Layout _layout;
    public bool _hide_text = false;
    public bool hide_text {
        get { 
        return _hide_text; }
        set {
        
            _hide_text = value; 
            this.queue_resize();
        }
    }


    /* Destroy function */
    ~Progress () {

    }

    /* Construct function */
    construct {
        this.expose_event += this.on_expose;
        this._layout = this.create_pango_layout (" ");
    }
    // The size_request method Gtk+ is calling on a widget to ask
    // it the widget how large it wishes to be. It's not guaranteed
    // that gtk+ will actually give this size to the widget
    public override void size_request (Gtk.Requisition requisition)
    {
        int width, height;
        stdout.printf("size request\n");
        // In this case, we say that we want to be as big as the
        // text is, plus a little border around it.
        if(this.hide_text)
        {
            requisition.width = 40;
            requisition.height = 8;
        }else{
            this._layout.get_size (out width, out height);
            requisition.width = width / Pango.SCALE + 6;
            requisition.height = height / Pango.SCALE + 6;
        }
    }

    private bool on_expose (Progress pb,Gdk.EventExpose event) {
        var ctx = Gdk.cairo_create(pb.window); 
        int width = pb.allocation.width-3;
        int height = pb.allocation.height-3;

        /* Draw border */
        ctx.set_line_width ( 1.0 );
        ctx.set_tolerance ( 0.2 );
        ctx.set_line_join (LineJoin.ROUND);

      
        if(this.total > 0)
        {
            double step_size = width/(double)this.total;
            int pwidth = (int)(step_size*current);
            /* don't allow more then 100% */
            if( pwidth > width ) {
                pwidth = width;
            }
            ctx.new_path();
            Gdk.cairo_set_source_color(ctx, pb.style.bg[(int)Gtk.StateType.SELECTED]);
            ctx.rectangle(1.5,1.5,pwidth, height);
            ctx.fill_preserve ();
            Gdk.cairo_set_source_color(ctx, pb.style.dark[(int)Gtk.StateType.NORMAL]);
            ctx.stroke ();
        }
        ctx.new_path();
        Gdk.cairo_set_source_color(ctx, pb.style.dark[(int)Gtk.StateType.NORMAL]);
        ctx.rectangle(1.5,1.5,width, height);
        ctx.stroke ();


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
                if(this.current/60 > 99 ) {
                    a  = "%02u:%02u - %02u:%02u".printf( 
                            this.current/3600,
                            (this.current)%60,
                            this.total/3600,
                            (this.total)%60 
                            );
                } else {
                    a = "%02u:%02u - %02u:%02u".printf( 
                            this.current/60,
                            (this.current)%60,
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
                double step_size = width/(double)this.total;
                int pwidth = (int)(step_size*current);

                if(pwidth >= ((width-fontw)/2+1))
                {
                    ctx.new_path();
                    Gdk.cairo_set_source_color(ctx, pb.style.fg[(int)Gtk.StateType.SELECTED]);
                    ctx.rectangle(1, 1,pwidth, height);
                    ctx.clip();
                    ctx.move_to ((width - fontw)/2+1.5,
                            (height - fonth)/2+1.5);
                    Pango.cairo_show_layout ( ctx, this._layout);
                }
                if(pwidth < ((width-fontw)/2+1+fontw))
                {
                    ctx.new_path();
                    Gdk.cairo_set_source_color(ctx, pb.style.fg[(int)Gtk.StateType.NORMAL]);
                    ctx.reset_clip();
                    ctx.rectangle(pwidth+1, 1,width, height);
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
        this.total = total;
        this.current = current;
        this.queue_draw();
    }
}