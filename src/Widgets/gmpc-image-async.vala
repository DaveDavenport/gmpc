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

using GLib;
using Gtk;
using Gdk;

public class Gmpc.PixbufLoaderAsync : GLib.Object
{
    private weak GLib.Cancellable? pcancel = null; 
    public string uri = null;
    public Gdk.Pixbuf pixbuf {set;get;default=null;}
    private Gtk.TreeRowReference rref = null;


    signal void pixbuf_update(Gdk.Pixbuf? pixbuf);

    public void set_rref(Gtk.TreeRowReference rreference)
    {
        this.rref = rreference;
    }

    private void call_row_changed()
    {
        if(rref != null) {
            var model = rref.get_model();
            var path = rref.get_path();
            Gtk.TreeIter iter;
            if(model.get_iter(out iter, path))
            {
                model.row_changed(path, iter);
            }
        }
    }

    construct {
	stdout.printf("Create the image loading\n" );
    }

    ~PixbufLoaderAsync() {
        warning("Free the image loading");
        if(this.pcancel != null) pcancel.cancel();
    }

    private Gdk.Pixbuf? modify_pixbuf(Gdk.Pixbuf? pix, int size,bool casing) 
    {
        if(pix == null) return null;
        if(casing)
        {
            int width = pix.width;
            int height = pix.height;
            double spineRatio = 5.0/65.0;

            var ii = Gtk.IconTheme.get_default().lookup_icon("stylized-cover", size, 0);
            if(ii != null) {
                var path = ii.get_filename();
                try {
                    var case_image = new Gdk.Pixbuf.from_file_at_scale(path, size, size, true);

                    var tempw = (int)(case_image.width*(1.0-spineRatio));
                    Gdk.Pixbuf pix2;
                    if((case_image.height/(double)height)*width < tempw) {
                        pix2 = pix.scale_simple(tempw, (int)((height*tempw)/width), Gdk.InterpType.BILINEAR);
                    }else{
                        pix2 = pix.scale_simple((int)(width*(case_image.height/(double)height)), case_image.height, Gdk.InterpType.BILINEAR);
                    }
                    var blank = new Gdk.Pixbuf(Gdk.Colorspace.RGB, true, 8, case_image.width, case_image.height);
                    blank.fill(0x000000FF);
                    tempw = (tempw >= pix2.width)? pix2.width:tempw;
                    var temph = (case_image.height > pix2.height)?pix2.height:case_image.height;
                    pix2.copy_area(0,0, tempw-1, temph-2, blank, case_image.width-tempw, 1);
                    case_image.composite(blank, 0,0,case_image.width, case_image.height, 0,0,1,1,Gdk.InterpType.BILINEAR, 250);
                    return blank;
                }catch (Error e) {

                }

            }
        }
        Gdk.Pixbuf retv = pix;
        Gmpc.Fix.add_border(ref retv);
        return retv;
    }


    public new void set_from_file(string uri, int size, bool border)
    {
        GLib.Cancellable cancel= new GLib.Cancellable();
        if(this.pcancel != null)
            this.pcancel.cancel();
        this.pcancel = cancel;
        this.uri = uri;
        this.load_from_file_async(uri, size, cancel, border);
    }

    private async void load_from_file_async(string uri, int size, GLib.Cancellable cancel, bool border)
    {
        GLib.File file = GLib.File.new_for_path(uri);
        size_t result = 0;
        Gdk.PixbufLoader loader = new Gdk.PixbufLoader();
        loader.size_prepared.connect((source, width, height) => {
            double dsize = (double)size;
            int nwidth = (height > width)? (int)((dsize/height)*width): size;
            int nheight= (width > height )? (int)((dsize/width)*height): size;
            loader.set_size(nwidth, nheight);

        });
        loader.area_prepared.connect((source) => {
                var apix = loader.get_pixbuf();
                var afinal = this.modify_pixbuf(apix, size,border);
                
                pixbuf = afinal;
                pixbuf_update(pixbuf);
                call_row_changed();
                });
        try{
            var stream = yield file.read_async(0, cancel);
            if(!cancel.is_cancelled() && stream != null )
            {
                do{
                    try {
                          uchar data[1024]; 
                          result = yield stream.read_async(data,1024, 0, cancel);
                          Gmpc.Fix.write_loader(loader,(string)data, result);
                      }catch ( Error erro) {
                          warning("Error trying to fetch image: %s", erro.message);
                      }
                  }while(!cancel.is_cancelled() && result > 0);
            }      
        }catch ( Error e) {
            warning("Error trying to fetch image: %s", e.message);
        }
        try {
            loader.close();
        }catch (Error err) {
            warning("Error trying to parse image: %s", err.message);
        }

        if(cancel.is_cancelled())
        {
            warning("Cancelled loading of image");
            cancel.reset();
            return;
        }

        var pix = loader.get_pixbuf();
        var final = this.modify_pixbuf(pix, size,border);
        pixbuf = final;
        pixbuf_update(pixbuf);
        call_row_changed();
	this.pcancel = null;
	loader = null;
    }
}

public class Gmpc.MetaImageAsync : Gtk.Image
{
    private Gmpc.PixbufLoaderAsync? loader = null;
    public string uri = null;

    construct {
    }

    ~MetaImageAsync() {
	stdout.printf("Freeing metaimageasync\n");
    }

    public new void set_from_file(string uri, int size, bool border)
    {
        loader = new PixbufLoaderAsync(); 
        loader.pixbuf_update.connect((source, pixbuf)=>{
                this.set_from_pixbuf(pixbuf);
                });
        loader.set_from_file(uri, size, border);
    }
    public void clear_now()
    {
        this.loader = null;
        this.uri = null;
        this.clear();
    }

    public void set_pixbuf(Gdk.Pixbuf pb)
    {
        this.loader = null;
        this.uri = null;
        this.set_from_pixbuf(pb);
    }
}