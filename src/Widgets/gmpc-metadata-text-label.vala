/* Gnome Music Player Client 
 * Copyright (C) 2011-2012 Qball Cow <qball@gmpclient.org>
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

private const bool use_transition_mdtl = Gmpc.use_transition;
private const string some_unique_name_mdtl = Config.VERSION;


namespace Gmpc.MetaData.Widgets
{
    public class TextLabel : Gtk.Label
    {
        private MPD.Song? cur_song = null;
        private string song_checksum = null;
        private Gmpc.MetaData.Type cur_type = Gmpc.MetaData.Type.ALBUM_TXT; 
        private void set_from_item(Gmpc.MetaData.Item? item)
        {
            if(item != null )
            {
                if(item.content_type == Gmpc.MetaData.ContentType.TEXT)
                {
                   string res = item.get_text();
                   this.set_text(res);
                }
                else if(item.content_type == Gmpc.MetaData.ContentType.HTML)
                { 

                   string res = item.get_text_from_html();
                   this.set_text(res);
                }
                else if(item.content_type == Gmpc.MetaData.ContentType.URI )
                {
                    string path = item.get_uri();
                    string res = null;
                    try{
                        GLib.FileUtils.get_contents( path, out res);
                        this.set_text(res);
                    }catch (Error e) {
                        this.set_text(_("Error reading file: %s").printf(e.message));
                    }
                }
                else 
                {
                    this.set_text(_("Not available"));
                }
            }
            else
            {
                this.set_text(_("Not available"));
            }
        }

        /**
         * @param song The #MPD.Song to display the text for.
         * @param type The #Gmpc.MetaData.Type of metadata to display. 
         * Create a text label for Song, that displays 
         * Text metadata
         * @return a #TextLabel or type #Gtk.Label
         */
       
        public TextLabel(MPD.Song song,Gmpc.MetaData.Type type)
        {
            this.set_line_wrap(true);
            this.set_text("Not available");
            this.set_alignment(0.0f, 0.0f);
            this.set_padding(4,4);
            this.set_selectable(true);
            cur_type = type;
            cur_song = song.copy();
            song_checksum = Gmpc.Misc.song_checksum(song);

            metawatcher.data_changed.connect((csong, type, result, met) => {
                if(type == cur_type && song_checksum == Gmpc.Misc.song_checksum(csong))
                {
                    if(result == Gmpc.MetaData.Result.AVAILABLE) {
                            this.set_from_item(met);
                     }else if (result == Gmpc.MetaData.Result.FETCHING) {
                            this.set_text(_("Fetching..."));
                     }else {
                            this.set_from_item(null);
                    }
                    
                }
            });

            /** Query */
            Gmpc.MetaData.Item item = null;
            var a = metawatcher.query(cur_song, type, out item);
            if(a == Gmpc.MetaData.Result.AVAILABLE) {
                this.set_from_item(item);
            }else if (a == Gmpc.MetaData.Result.FETCHING) {
                this.set_text(_("Fetching..."));
            }else {
                this.set_from_item(null);
            }
            this.populate_popup.connect((source, menu)=>{
                /*  Add refetch */
                var mitem = new Gtk.ImageMenuItem.with_label(_("Refetch"));
                mitem.set_image(
                    new Gtk.Image.from_stock("gtk-refresh", Gtk.IconSize.MENU));
                mitem.activate.connect((source)=>{
                    metawatcher.query(cur_song, type|Gmpc.MetaData.Type.QUERY_NO_CACHE, out item);
                 });
                 menu.append(mitem);
                 mitem.show();
                             
                /*  Add selector */
                mitem = new Gtk.ImageMenuItem.with_label(_("Metadata selector"));
                mitem.set_image(
                    new Gtk.Image.from_stock("gtk-edit", Gtk.IconSize.MENU));
                mitem.activate.connect((source)=>{
                    new Gmpc.MetaData.EditWindow(cur_song, cur_type);
                 });
                 menu.append(mitem);
                 mitem.show();
            });

        }

    }

}// end namespace Gmpc.MetaData
