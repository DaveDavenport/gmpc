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

/**
 * This plugin queries RenderCover.com for artist images (backdrops)
 * It renders the image to a png (in memory) and passes this as raw data
 * to the metadata system.
 */

using Config;
using Gmpc;
using Gmpc.Plugin;

private const bool use_transition_prc = Gmpc.use_transition;
private const string some_unique_name_prc = Config.VERSION;
private const string log_domain_prc = "Gmpc.Provider.RenderCover";

public class Gmpc.Provider.RenderCover: 
            Gmpc.Plugin.Base,Gmpc.Plugin.MetaDataIface 
{
    private const int[] version = {0,0,2};
    
    public override unowned int[] get_version() {
        return this.version;
    }

    public override unowned string get_name() {
        return N_("Backdrop Renderer");
    }

    construct {
        this.plugin_type = 8+32;
    }

    public void set_priority(int priority) {
        config.set_int(this.get_name(),"priority",priority);
    }

    public int get_priority() {
        return config.get_int_with_default(this.get_name(),"priority",100);
    }

    public void get_metadata(MPD.Song song, Gmpc.MetaData.Type type, MetaDataCallback callback)
    {

        if(song == null || song.artist == null || song.album == null)
        {
            log(log_domain_prc, GLib.LogLevelFlags.LEVEL_DEBUG, 
                    "Insufficient information. doing nothing");
            /* Tell that we found nothing */
            callback(null);
            return;
        }
        switch(type)
        {
            case Gmpc.MetaData.Type.ALBUM_ART:
                /* A request for artist art came in. */
                this.get_album_art(song, callback);
                return; 
			case Gmpc.MetaData.Type.ARTIST_TXT:	
			case Gmpc.MetaData.Type.ARTIST_SIMILAR:	
			case Gmpc.MetaData.Type.ARTIST_ART:
			case Gmpc.MetaData.Type.ALBUM_TXT:	
			case Gmpc.MetaData.Type.SONG_TXT:	
			case Gmpc.MetaData.Type.SONG_SIMILAR:	
			case Gmpc.MetaData.Type.GENRE_SIMILAR:	
			case Gmpc.MetaData.Type.SONG_GUITAR_TAB:	
			case Gmpc.MetaData.Type.QUERY_DATA_TYPES:	
			case Gmpc.MetaData.Type.QUERY_NO_CACHE:	
            default:
                break;
        }

        /* Tell what we found */ 
        callback(null);
    }

    /** 
     * Get album art 
     */
     public const int album_size = 400;
     private void get_album_art(MPD.Song song, MetaDataCallback callback)
     {
        Cairo.Pattern p;
        Cairo.Surface surf = new Cairo.ImageSurface(Cairo.Format.ARGB32, album_size,album_size);
        Cairo.Context ct = new Cairo.Context(surf);
	// Color the background based on hash of artist/album.
	uint hash = song.album.hash()/2;
	hash+= song.artist.hash()/2;

	// Background
        ct.set_source_rgb(
		(hash&255)/255.0,
		((hash>>8)&255)/255.0,
		((hash>>16)&255)/255.0);
        ct.paint();

	var it = Gtk.IconTheme.get_default();
	try{
		var pb = it.load_icon("gmpc", 256,0);
		Misc.decolor_pixbuf(pb, pb);
       		Gdk.cairo_set_source_pixbuf(ct, pb, 200-pb.width/2 ,390-pb.height);
		ct.paint_with_alpha(0.6);

	}catch (GLib.Error e) {

	}
        // Header pattern 
        p = new Cairo.Pattern.linear(0, 0, album_size,0);
        p.add_color_stop_rgb(0, 0.8,0.8,0.8);
        p.add_color_stop_rgb(1, 0.5,0.5,0.5);        
        ct.set_source(p);
        ct.rectangle(0.0,0.0,album_size, album_size/3);
        ct.fill();

        // Bar
        p = new Cairo.Pattern.linear(0, 0, album_size,0);
        p.add_color_stop_rgba(0,     0,0,0,0.1);
        p.add_color_stop_rgba(0.5,   0,0,0,0.9);        
        p.add_color_stop_rgba(1,     0,0,0,0.1);    
        ct.set_source(p);
        ct.rectangle(0.0,album_size/3-6,album_size, 12.0);
        ct.fill();
        
        
        ct.set_source_rgb(0.0,0.0,0.0);
        Pango.Layout layout = Pango.cairo_create_layout(ct);
        Pango.FontDescription fd = Pango.FontDescription.from_string("Serif bold 32");
        layout.set_font_description(fd);
        layout.set_text(song.album,-1);
        int aheight=0, awidth=0;
        layout.get_pixel_size(out awidth, out aheight);
        if(awidth >= (album_size-50)) {
            p = new Cairo.Pattern.linear(0, 0, album_size-25,0);
            p.add_color_stop_rgba(0,     0,0,0,1);
            p.add_color_stop_rgba((album_size-50)/(double)(album_size),   0,0,0,1);
            p.add_color_stop_rgba(1,     0,0,0,0.0);        
            ct.set_source(p);
        }else{
            ct.set_source_rgb(0.0,0.0,0.0);
        }
        //layout.set_ellipsize(Pango.EllipsizeMode.END);
        
        ct.move_to(25,25);
        Pango.cairo_layout_path(ct,layout);
        ct.fill();
        
        fd = Pango.FontDescription.from_string("Sans bold 18");
        layout.set_font_description(fd);
        layout.set_text(song.artist, -1);
        int bheight=0, bwidth=0;
        layout.get_pixel_size(out bwidth, out bheight);
        
        if(bwidth >=(album_size-50)) {
            ct.move_to(25, aheight+25+5);
            p = new Cairo.Pattern.linear(0, 0, album_size-25,0);
            p.add_color_stop_rgba(0,     1,1,1,1);
            p.add_color_stop_rgba((album_size-50)/(double)(album_size),   1,1,1,1);
            p.add_color_stop_rgba(1,     1,1,1,0.0);        
            ct.set_source(p);
        }else {
            ct.move_to(album_size-25-bwidth, aheight+25+5);
            ct.set_source_rgb(1,1,1);
        }
        Pango.cairo_layout_path(ct,layout);
        ct.fill();
        


	/* We get blocks of image data,
	 * Do manual memory management because
	 * vala cannot do this?
         */	
	void *data = null;
	uint len = 0;
        surf.write_to_png_stream((imgdata)=>
	{
		data = GLib.realloc(data, len+imgdata.length);
		GLib.Memory.copy(&data[len], imgdata, imgdata.length);
		len += imgdata.length;
		return Cairo.Status.SUCCESS;
	});
	/* Create result message */
	MetaData.Item pitem = new MetaData.Item();
	pitem.type = Gmpc.MetaData.Type.ALBUM_ART;
	pitem.plugin_name = get_name();
	pitem.content_type = MetaData.ContentType.RAW;
	/* This function will take over the data and set data=null 
	 * len = 0
  	 */
	pitem.set_raw_void(ref data,ref len);
	/* this isn't needed as set_raw_void takes ownershit */
	GLib.free(data);
	List<MetaData.Item> list = null;
	list.append((owned)pitem);
	callback((owned)list);


     }
}
