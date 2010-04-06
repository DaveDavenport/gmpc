/* Gnome Music Player Client (GMPC)
 * Copyright (C) 2004-2010 Qball Cow <qball@sarine.nl>
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

const string LOG_DOMAIN = "ImageAsync";

public class Gmpc.PixbufLoaderAsync : GLib.Object
{
    private weak GLib.Cancellable? pcancel = null; 
    public string uri = null;
    public Gdk.Pixbuf pixbuf {set;get;default=null;}
    private Gtk.TreeRowReference rref = null;


    public signal void pixbuf_update(Gdk.Pixbuf? pixbuf);

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
        GLib.log(LOG_DOMAIN,GLib.LogLevelFlags.LEVEL_DEBUG,"Create the image loading\n" );
    }

    ~PixbufLoaderAsync() {
        GLib.log(LOG_DOMAIN,GLib.LogLevelFlags.LEVEL_DEBUG,"Free the image loading");
        if(this.pcancel != null) pcancel.cancel();
    }

    private Gdk.Pixbuf? modify_pixbuf(Gdk.Pixbuf? pix, int size,bool casing) 
    {
        if(pix == null) return null;
        if(casing && config.get_int_with_default("metaimage", "addcase", 1) == 1)
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
        
        Gmpc.Fix.add_border(pix);
        /* used to try to track leak */
        return pix.copy();
    }


    public new void set_from_file(string uri, int size, bool border)
    {
        if(this.pcancel != null)
            this.pcancel.cancel();
        this.pcancel = null;
        this.uri = uri;
        
        var pb = Gmpc.PixbufCache.lookup_icon(size, uri);
        if(pb != null)
        {
            this.pixbuf = pb;
            pixbuf_update(pixbuf);
            call_row_changed();
            return;
        }
        GLib.Cancellable cancel= new GLib.Cancellable();
        this.pcancel = cancel;
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
            pixbuf_update(null);
            /* Failed to load the image */
            return;
        }

        if(cancel.is_cancelled())
        {
            GLib.log(LOG_DOMAIN,GLib.LogLevelFlags.LEVEL_DEBUG,"Cancelled loading of image");
            cancel.reset();
            return;
        }

        Gdk.Pixbuf pix = loader.get_pixbuf();
        /* Maybe another thread allready fetched it in the mean time, we want to use that... */
        var final = Gmpc.PixbufCache.lookup_icon(size, uri);
        if(final == null)
        {
            final = this.modify_pixbuf(pix, size,border);
            Gmpc.PixbufCache.add_icon(size,uri, final);
        }
        this.pixbuf = final;
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
	GLib.log(LOG_DOMAIN,GLib.LogLevelFlags.LEVEL_DEBUG,"Freeing metaimageasync\n");
    }

    public new void set_from_file(string uri, int size, bool border)
    {
        this.uri = uri;
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

    public void set_pixbuf(Gdk.Pixbuf? pb)
    {
        this.loader = null;
        this.uri = null;
        if(pb != null) {
            this.set_from_pixbuf(pb);
        }else{
            this.clear();
        }
    }
}
