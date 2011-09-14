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


using Config;
using Gtk;
using Gmpc;

private const bool use_transition_mdss = Gmpc.use_transition;
private const string some_unique_name_mdss = Config.VERSION;

public class Gmpc.MetaData.Widgets.SimilarSongs : Gtk.Alignment
{
    private MPD.Song? song = null;
    private Gtk.Widget pchild = null;
    private uint idle_add = 0;
    ~SimilarSongs ()
    {
        if(this.idle_add > 0){
            GLib.Source.remove(this.idle_add);
            this.idle_add = 0;
        }
    }

    public SimilarSongs (MPD.Song song) 
    {
        this.song = song.copy();
        this.set(0.0f, 0.0f, 1.0f, 0.0f);
    }
    private void add_clicked(Gtk.Widget item)
    {
        Gtk.TreeView tree = (Gtk.TreeView)this.pchild;

        var sel = tree.get_selection();
        Gtk.TreeModel model = null;
        Gtk.TreeIter iter;
        List<Gtk.TreePath> list = sel.get_selected_rows(out model);
        foreach(Gtk.TreePath path in list)
        {
            if(model.get_iter(out iter, path))
            {
                unowned MPD.Song? song = null;
                model.get(iter, 0, out song, -1);
                if(song != null)
                {
                   MPD.PlayQueue.queue_add_song(server, song.file); 
                }
            }
        }
        MPD.PlayQueue.queue_commit(server);

    }
    private void play_clicked(Gtk.Widget item)
    {
        Gtk.TreeView tree = (Gtk.TreeView)this.pchild;

        var sel = tree.get_selection();
        Gtk.TreeModel model = null;
        Gtk.TreeIter iter;
        List<Gtk.TreePath> list = sel.get_selected_rows(out model);
        if(list != null)
        {
            Gtk.TreePath path = list.data;
            if(model.get_iter(out iter, path))
            {
                unowned MPD.Song? song = null;
                model.get(iter, 0, out song, -1);
                if(song != null)
                {
                    Gmpc.MpdInteraction.play_path(song.file);
                }
            }
        }
    }
    private void replace_clicked(Gtk.Widget item)
    {
        bool found = false;
        Gtk.TreeView tree = (Gtk.TreeView)this.pchild;
        var sel = tree.get_selection();
        Gtk.TreeModel model = null;
        Gtk.TreeIter iter;
        List<Gtk.TreePath> list = sel.get_selected_rows(out model);
        foreach(Gtk.TreePath path in list)
        {
            if(model.get_iter(out iter, path))
            {
                unowned MPD.Song? song = null;
                model.get(iter, 0, out song, -1);
                if(song != null)
                {
                   MPD.PlayQueue.queue_add_song(server, song.file); 
                   found = true;
                }
            }
        }
        if(found)
        {
            MPD.PlayQueue.clear(server);
            MPD.PlayQueue.queue_commit(server);
            MPD.Player.play(server);
        }


        this.play_clicked(item);
    }
    private void tree_row_activated(Gtk.Widget tree, Gtk.TreePath path , Gtk.TreeViewColumn column)
    {
        var model = (tree as Gtk.TreeView).get_model();
        Gtk.TreeIter iter;
        if(model.get_iter(out iter, path))
        {
            unowned MPD.Song? song = null;
            model.get(iter, 0, out song, -1);
            if(song != null)
            {
                Gmpc.MpdInteraction.play_path(song.file);
            }
        }
    }
    private bool tree_right_menu(Gtk.Widget tree, Gdk.EventButton event)
    {
        if(event.button == 3)
        {
            var menu = new Gtk.Menu();
            var item = new Gtk.ImageMenuItem.from_stock("gtk-media-play",null);
            item.activate.connect(play_clicked);
            menu.append(item);

            item = new Gtk.ImageMenuItem.from_stock("gtk-add",null);
            item.activate.connect(add_clicked);
            menu.append(item);

            item = new Gtk.ImageMenuItem.with_mnemonic(_("_Replace"));
            item.set_image(new Gtk.Image.from_stock("gtk-redo", Gtk.IconSize.MENU));
            item.activate.connect(replace_clicked);
            menu.append(item);

            (tree as Gmpc.MpdData.TreeView).right_mouse_integration(menu);
            menu.popup(null, null, null, event.button, event.time);
            menu.show_all();
            return true;
        }
        return false;
    }

    private Gmpc.MetaData.Item copy = null;
    MPD.Data.Item item = null;
    private unowned List <unowned string> current = null;
    private bool update_sim_song()
    {
        if(current == null){
           current = copy.get_text_list(); 
           pchild = new Gtk.ProgressBar();
           this.add(pchild);
           this.show_all();
        }
        ((Gtk.ProgressBar)pchild).pulse();
        if(current != null)
        {
            string entry = current.data;
            if(entry != null){
                var split = entry.split("::",2);
                if(split.length == 2)
                {
                    MPD.Database.search_start(server, false);
                    var art_split = split[0].split(" ");
                    foreach(string artist in art_split)
                    {
                        MPD.Database.search_add_constraint(server, MPD.Tag.Type.ARTIST, artist);
                    }

                    MPD.Database.search_add_constraint(server, MPD.Tag.Type.TITLE, split[1]);
                    var data = MPD.Database.search_commit(server);
                    if(data != null)
                    {
                        item.concatenate((owned)data); 
                    }
                }
            }
            current = current.next;
            if(current != null) return true;
        }
        this.pchild.destroy();
        if(item != null)
            {
            var model = new Gmpc.MpdData.Model();
            item.remove_duplicate_songs();
            model.set_mpd_data((owned)item);
            Gmpc.MpdData.TreeView tree = new Gmpc.MpdData.TreeView("similar-song", true, model);
            tree.enable_click_fix();
            tree.button_release_event.connect(tree_right_menu);
            tree.row_activated.connect(tree_row_activated);
            this.add(tree);

            this.pchild = tree;
        }else {
            var label = new Gtk.Label(_("Unavailable"));
            label.set_alignment(0.0f, 0.0f);
            this.add(label);
            this.pchild = label;
        }

        copy = null;
        this.idle_add = 0;

        this.show_all();
        return false;
    }
    private void metadata_changed(MetaWatcher gmw2, MPD.Song song, Gmpc.MetaData.Type type, Gmpc.MetaData.Result result, Gmpc.MetaData.Item? met)
    {
        if(this.song.artist.collate(song.artist)!=0) return;
        if(type != Gmpc.MetaData.Type.SONG_SIMILAR) return;
        
        if(this.pchild != null) this.pchild.destroy(); 

        if(result == Gmpc.MetaData.Result.FETCHING) {
            var label = new Gtk.Label(_("Fetching .. "));
            label.set_alignment(0.0f, 0.0f);
            this.add(label);
            this.pchild = label;
        }else if (result == Gmpc.MetaData.Result.UNAVAILABLE)
        {
            var label = new Gtk.Label(_("Unavailable"));
            label.set_alignment(0.0f, 0.0f);
            this.add(label);
            this.pchild = label;
        }else{
            if(met.is_text_list())
            {
                this.copy = met.dup_steal();
                this.idle_add =  GLib.Idle.add(this.update_sim_song);
                return;
            }else {
                var label = new Gtk.Label(_("Unavailable"));
                label.set_alignment(0.0f, 0.0f);
                this.add(label);
                this.pchild = label;
            }
        }
        this.show_all();

    }
    public void update()
    {
        MetaData.Item item = null;
        metawatcher.data_changed.connect(metadata_changed);
        Gmpc.MetaData.Result gm_result = metawatcher.query(song, Gmpc.MetaData.Type.SONG_SIMILAR,out item);
        this.metadata_changed(metawatcher, this.song, Gmpc.MetaData.Type.SONG_SIMILAR, gm_result, item); 
    }

}

