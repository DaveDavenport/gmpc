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

using GLib;
using Gtk;
using Gdk;

const string LOG_DOMAIN = "ImageAsync";
namespace Gmpc
{
	/**
	 * Operations you can do on the image. 
	 * The modified pixbuf will be stored in cache.
     */
    public enum ModificationType {
            NONE    = 0,        // Add nothing
            CASING  = 1,        // Add border and or casing
            DARKEN  = 2,        // Darken the image (for backdrop)
            DECOLOR = 4,        // Remove color from image.
			BORDER  = 8
    }
    public class PixbufLoaderAsync : GLib.Object
    {
        public string uri = null;
        public Gdk.Pixbuf pixbuf {set;get;default=null;}
        private Gtk.TreeRowReference rref = null;
        private int width=0;
        private int height=0;

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


        private Gdk.Pixbuf? modify_pixbuf(owned Gdk.Pixbuf? pix, int size,ModificationType casing) 
        {
            if(pix == null) return null;
            if((casing&ModificationType.CASING) == ModificationType.CASING)
            {
                if(config.get_int_with_default("metaimage", "addcase", 1) == 1)
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
                            pix = (owned)blank;
                        }catch (Error e) {
                            GLib.log(LOG_DOMAIN,GLib.LogLevelFlags.LEVEL_WARNING, 
                                    "Failed to get the stylized-cover image");
                        }

                    }
                }else{
                    Gmpc.Fix.add_border(pix);
                }
            }
            if ((casing&ModificationType.DARKEN) == ModificationType.DARKEN)
            {
                Gmpc.Misc.darken_pixbuf(pix, 2);
            }
            if ((casing&ModificationType.DECOLOR) == ModificationType.DECOLOR)
            {
                Gmpc.Misc.decolor_pixbuf(pix, pix);
            }
            if ((casing&ModificationType.BORDER) == ModificationType.BORDER)
            {
                Gmpc.Misc.border_pixbuf(pix);
            }

            return pix;
        }


		private uint 				loader_timeout 	= 0;
		private Gdk.PixbufLoader? 	loader 			= null;
		private uchar[]				loader_data			= null;
		private uint				loader_data_offset	= 0;
		private uchar[]				loader_md5sum		= null;
		private ModificationType	loader_border		= ModificationType.NONE;

        ~PixbufLoaderAsync() {
            GLib.log(LOG_DOMAIN,GLib.LogLevelFlags.LEVEL_DEBUG,"Free the image loading");
			// Cancel previous load.
			if ( loader_timeout > 0) {
				GLib.Source.remove(loader_timeout);

				loader_data = null;
				loader_md5sum = null;
				loader_timeout = 0;
				try{
	                loader.close();
				}catch (Error e) {
					// Ignore this.
				}
				loader = null;
			}
        }



        public void set_from_raw(uchar[] data, int req_width, int req_height, ModificationType border,[CCode (array_length = false)]uchar[] md5sum)
		{
            width = req_width;
            height = req_height;

			// Cancel previous load.
			if ( loader_timeout > 0) {
				GLib.Source.remove(loader_timeout);

				loader_data = null;
				loader_md5sum = null;
				loader_timeout = 0;
				try{
	                loader.close();
				}catch (Error e) {
					// Ignore this.
				}
				loader = null;
			}


            var pb = Gmpc.PixbufCache.lookup_icon(int.max(width,height), md5sum);
            if(pb != null)
            {
                this.pixbuf = pb;
                pixbuf_update(pixbuf);
                call_row_changed();
                return;
            }

            loader = new Gdk.PixbufLoader();
            loader.size_prepared.connect(size_prepare);

			loader_data = data;
			loader_data_offset = 0;
			loader_md5sum = md5sum[0:16];
			loader_border = border;


			loader_timeout = GLib.Idle.add(loader_idle_callback);
//			loader_timeout = GLib.Timeout.add(200, loader_idle_callback);
			return;
		}

		private bool loader_idle_callback()
		{

            try{
				uint end = uint.min(loader_data_offset+2048, loader_data.length);
				Gmpc.Fix.write_loader(loader, (string)loader_data[loader_data_offset:end], 2048);
				loader_data_offset = end;;
            }catch ( Error e) {
                warning("Error trying to load image: %s::%s", e.message,uri);
            }
			if(loader_data_offset < loader_data.length) return true;

            try {
                loader.close();
            }catch (Error err) {
                debug("Error trying to parse image: %s::%s? query cancelled?", err.message,uri);
                pixbuf_update(null);
                call_row_changed();

				loader_data = null;
				loader_md5sum = null;
				loader = null;
				loader_timeout = 0;
				return false;
            }

            Gdk.Pixbuf pix = loader.get_pixbuf();
            /* Maybe another thread allready fetched it in the mean time, we want to use that... */
            var final = Gmpc.PixbufCache.lookup_icon(int.max(height, width), loader_md5sum);
            if(final == null)
            {
				final = this.modify_pixbuf((owned)pix, int.max(height, width),loader_border);
				Gmpc.PixbufCache.add_icon(int.max(height, width),loader_md5sum, final);
            }

			this.pixbuf = final; 
            pixbuf_update(pixbuf);
            call_row_changed();

			// Cleanup
            loader = null;
			loader_data = null;
			loader_md5sum = null;
			loader_timeout = 0;
			return false;
		}

        private void size_prepare(Gdk.PixbufLoader loader,int  gwidth, int gheight)
        {
             double dsize = (double)(int.max(width,height)); 
             int nwidth = 0, nheight = 0;
             if(height < 0) {
                    double scale = width/(double)gwidth;
                    nwidth = width;
                    nheight = (int)(gheight*scale);
             } else if (width < 0) {
                    double scale = height/(double)gheight;
                    nheight = height;
                    nwidth = (int)(gwidth*scale);
             }else{
                    nwidth = (gheight >gwidth)? (int)((dsize/gheight)*gwidth): (int)dsize;
                    nheight= (gwidth > gheight )? (int)((dsize/gwidth)*gheight): (int)dsize;
             }
             loader.set_size(nwidth, nheight);
        }
	}
	public class MetaImageAsync : Gtk.Image
    {
        private Gmpc.PixbufLoaderAsync? loader = null;
        public string uri = null;

        construct {
        }

        ~MetaImageAsync() {
            GLib.log(LOG_DOMAIN,GLib.LogLevelFlags.LEVEL_DEBUG,"Freeing metaimageasync\n");
        }
		public new void set_from_raw(uchar[] data, int size, ModificationType border,[CCode (array_length = false)] uchar[] md5sum)
		{
            if(loader == null) {
                loader = new PixbufLoaderAsync(); 
                loader.pixbuf_update.connect((source, pixbuf)=>{
                        this.set_from_pixbuf(pixbuf);
                        });
            }
            loader.set_from_raw(data, size,size, border,md5sum);
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
}
