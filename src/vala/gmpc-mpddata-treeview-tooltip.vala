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
using Config;
using Gtk;
using Gmpc;

private const bool use_transition_mtt = Gmpc.use_transition;
private const string some_unique_name_mtt = Config.VERSION;

public class Gmpc.MpdData.Treeview.Tooltip : Gtk.Window  {
    private Gtk.TreeView par_widget = null;
    private Gmpc.MetaData.Image image = null;
    private Gmpc.MetaData.Type mtype = Gmpc.MetaData.Type.ARTIST_ART;
    /* Destroy function */
    ~Tooltip() {
        stdout.printf("Gmpc.MpdData.Treeview.Tooltip destroy\n");
    }

    private string checksum = null;
    private bool
    query_tooltip_callback(int x, int y, bool keyboard_tip, Gtk.Tooltip tooltip)
    {
        string tag = null;
        var model = this.par_widget.get_model();

        Gtk.TreePath path = null;
        Gtk.TreeIter iter ;
        if(!this.par_widget.get_tooltip_context(out x, out y,keyboard_tip, out model, out path, out iter)){
            this.checksum = null;
            return false;
        }

        MPD.Song song = new MPD.Song();
        if(this.mtype == Gmpc.MetaData.Type.ALBUM_ART) {
            string album = null;
            model.get(iter, 5, out tag, 6 , out album);
            song.artist = tag; 
            song.album = album;
        }
        else{

            model.get(iter, 7, out tag);
            song.artist = tag;
        }

        string new_check = Gmpc.Misc.song_checksum(song);
        if(new_check != this.checksum && this.checksum != null)
        {
            this.checksum = null;
            return false;
        }
        if(new_check != this.checksum)
        {
            this.image.update_from_song(song);
            this.checksum = new_check;
        }
        return true;
/*
        this.checksum = null;
        return false;
  */  }

    public Tooltip(Gtk.TreeView pw, Gmpc.MetaData.Type type){
        this.type = Gtk.WindowType.POPUP;
        this.resizable = false;
        stdout.printf("Create tooltip widget\n");
        this.par_widget = pw;
        /*Set up all needed for tooltip */
        pw.query_tooltip.connect(query_tooltip_callback);
        this.par_widget.set_tooltip_window(this);
        /* setup image */
        this.image = new Gmpc.MetaData.Image(type, 150);
        this.mtype = type;
        this.image.set_squared(false);
        this.image.set_hide_on_na(true);
        this.add(image);
        
    }


}
