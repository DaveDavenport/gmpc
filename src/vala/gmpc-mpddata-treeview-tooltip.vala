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
using Config;
using Gtk;
using Gmpc;

private const bool use_transition_mtt = Gmpc.use_transition;
private const string some_unique_name_mtt = Config.VERSION;

public class Gmpc.MpdData.Treeview.Tooltip : Gtk.Window  {
    private Gtk.TreeView par_widget = null;
    private Gtk.Image image = null;
    public Gmpc.MetaData.Type mtype = Gmpc.MetaData.Type.ARTIST_ART;
    public string? request_artist = null;
    private string checksum = null;
    private bool
    query_tooltip_callback(int x, int y, bool keyboard_tip, Gtk.Tooltip tooltip)
    {
        string tag = null;
        int row_type = 0;
        Gtk.TreePath path = null;
        Gtk.TreeIter iter ;
        var model = this.par_widget.get_model();
        if(config.get_int_with_default("GmpcTreeView", "show-tooltip", 1) != 1) return false; 
        if(this.mtype != Gmpc.MetaData.Type.ARTIST_ART && this.mtype != Gmpc.MetaData.Type.ALBUM_ART) {
            this.checksum = null;
            return false;
        }

        if(!this.par_widget.get_tooltip_context(out x, out y,keyboard_tip, out model, out path, out iter)){
            this.checksum = null;
            return false;
        }

        MPD.Song song = new MPD.Song();
        /* Get the row type */
        model.get(iter, 26, out row_type);
        if(row_type == MPD.Data.Type.SONG)
        {
                string album = null;
                model.get(iter, 5, out tag, 6 , out album);
                song.artist = tag; 
                song.album = album;
        }
        else if (row_type == MPD.Data.Type.TAG)
        {
            if(this.mtype == Gmpc.MetaData.Type.ARTIST_ART)
            {
                model.get(iter, 7, out tag);
                song.artist = tag;
            }else if (this.mtype == Gmpc.MetaData.Type.ALBUM_ART)
            {
                model.get(iter, 7, out tag);
                song.artist = this.request_artist;
                song.album = tag;
            }

        }
        string new_check = Gmpc.Misc.song_checksum(song);
        if(new_check != this.checksum && this.checksum != null)
        {
            this.checksum = null;
            return false;
        }
        if(new_check != this.checksum)
        {

            this.checksum = new_check;
            Gmpc.MetaData.Item met = null;
            var result = metawatcher.query(song, this.mtype, out met);
            metadata_changed(metawatcher, song, this.mtype,result, met); 
        }
        if(this.image.get_storage_type() == Gtk.ImageType.EMPTY) return false;
        return true;
    }

    private void metadata_changed(MetaWatcher gmw2, MPD.Song song, Gmpc.MetaData.Type type, Gmpc.MetaData.Result result, Gmpc.MetaData.Item? met)
    {
        if(type != this.mtype) return;
        
        if(this.checksum !=  Gmpc.Misc.song_checksum(song)) return;
        if(result == Gmpc.MetaData.Result.UNAVAILABLE) {
            this.image.clear();
        }
        else if (result == Gmpc.MetaData.Result.FETCHING) {
            this.image.clear();
        }else if (result == Gmpc.MetaData.Result.AVAILABLE) {
            if(met.content_type == Gmpc.MetaData.ContentType.URI) {
                try {
                    var pb = new Gdk.Pixbuf.from_file_at_scale(met.get_uri(), 150, 150, true);
                    image.set_from_pixbuf(pb);
                } catch (Error e) {
                    this.image.clear();
                }
            } else {
                this.image.clear();
            }
        }
    }

    construct{
        this.type = Gtk.WindowType.POPUP;
    }
    public
    Tooltip(Gtk.TreeView pw, Gmpc.MetaData.Type type){
        this.resizable = false;
        this.par_widget = pw;
        /*Set up all needed for tooltip */
        pw.query_tooltip.connect(query_tooltip_callback);
        this.par_widget.set_tooltip_window(this);
        /* setup image */
        this.image = new Gtk.Image(); 
        this.image.show();
        this.mtype = type;
        this.add(image);
        this.set_border_width(2); 
        this.modify_bg(Gtk.StateType.NORMAL, pw.style.black);

        metawatcher.data_changed.connect(metadata_changed);
    }
}
