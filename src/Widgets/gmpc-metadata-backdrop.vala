/* Gnome Music Player Client
 * Copyright (C) 2011 Qball Cow <qball@gmpclient.org>
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
using Gmpc;

private const bool use_transition_mdbd = Gmpc.use_transition;
private const string some_unique_name_mdbd = Config.VERSION;
private const string log_domain_mdbd = "Gmpc.Widgets.MetaData.Backdrop";
namespace Gmpc {
    namespace Widgets
	{
		class MetaData.Backdrop : Gtk.EventBox
        {
            private string song_checksum = null;
            private Gmpc.MetaData.Type cur_type = Gmpc.MetaData.Type.ARTIST_ART; 
            private Gdk.Pixbuf pb = null;
            private ModificationType mod_type = (ModificationType)
                        config.get_int_with_default(
                            "Backdrop", "alterations",2);

            // Image loading
            private Gmpc.PixbufLoaderAsync? loader = null;
            private void set_from_item (Gmpc.MetaData.Item? item)
            {
                pb = null;
                if(item == null || song_checksum == null)
                {
                   return; 
                }
                if(item.content_type == Gmpc.MetaData.ContentType.URI)
                {
                    string uri = item.get_uri();
                    if(uri != null)
                    {
                        Gtk.Allocation req;

                        this.get_allocation(out req); 
						// Pixbuf loader needs only width constraint support.
                        loader = new PixbufLoaderAsync(); 
                        loader.pixbuf_update.connect((source, pixbuf)=>{
                            pb = pixbuf;
                            log (log_domain_mdbd,GLib.LogLevelFlags.LEVEL_DEBUG, 
                            "Updating background");
                            this.queue_draw();
                            });
                        loader.set_from_file(uri, req.width, -1,mod_type);
                    }
                }
            }
            /**
             * @param The #MPD.Song to set the background for or NULL to clear.
             *
             * Set the background for song, or clear the background.
             */
            public void set_song(MPD.Song? song)
            {
                loader = null;
                if(song == null) {
                    song_checksum = null;
                    set_from_item(null);
                    return;
                }
                // Same artist/bg do not update.
                if(song_checksum == Gmpc.Misc.song_checksum_type(song, cur_type))
                    return;
                    
                song_checksum = Gmpc.Misc.song_checksum_type(song, cur_type);
                Gmpc.MetaData.Item item = null;
                var a = metawatcher.query(song, cur_type, out item);
                if(a == Gmpc.MetaData.Result.AVAILABLE) {
                    this.set_from_item(item);
                }else {
                    this.set_from_item(null);
                }
            }
            /**
             * @param type The type of backdrop (artist or album image)
             * Create backdrop widget of type.
             * @return a new #Backdrop (of type #Gtk.Eventbox)
             */
            public Backdrop(Gmpc.MetaData.Type type)
            {
                assert(type == Gmpc.MetaData.Type.ARTIST_ART || 
                        type == Gmpc.MetaData.Type.ALBUM_ART);
                cur_type = type;

                // Set visible window
                this.set_visible_window(true);
                // Set paintable 
                this.set_app_paintable(true);
                // Watch changes */
                metawatcher.data_changed.connect((csong, type, result, met) => {
                        if(song_checksum != null && met != null &&
                            type == cur_type &&
                            song_checksum == Gmpc.Misc.song_checksum_type(csong,cur_type))
                        {
                            if(result == Gmpc.MetaData.Result.AVAILABLE) {
                                this.set_from_item(met);
                         }else {
                                this.set_from_item(null);
                        }
                }
                });

                // Add expose event
                this.expose_event.connect(container_expose);

            }
            /**
             * Draw the background. (only exposed part) 
             */
            private bool container_expose(Gtk.Widget ev, Gdk.EventExpose event)
            {
                /* If there is an background image set. */
                if(pb != null)
                {
                    var gc = Gdk.cairo_create(ev.window);
                    Gdk.cairo_region(gc,event.region);
                    gc.set_source_rgb(0.0,0.0,0.0);
                    gc.fill_preserve();
                    Gdk.cairo_set_source_pixbuf(gc, pb,0.0,0.0);
                    gc.fill();
                }
                /* no image set */
                else
                {
                    var gc = Gdk.cairo_create(ev.window);
                    Gdk.cairo_region(gc,event.region);
                    gc.set_source_rgb(0.0,0.0,0.0);
                    gc.fill();
                }
                return false;
            }


        }

    }
}
