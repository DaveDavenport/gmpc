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

/**
 * This does a most-effort fetch off album and artist art.
 * For album art it looks in the folder the music file is in,
 * Then in the parent directory if it directory is named CD or DISC.
 * It looks for jpg, png, jpeg, gif.
 * 
 * For artist art it looks for artist.jpg and $artist.jpg in the directory the music is in 
 * the parent and the parent of the parent.
 */
using Config;
using Gtk;
using Gmpc;
using Gmpc.Plugin;

private const bool use_transition_lp = Gmpc.use_transition;
private const string some_unique_name_lp = Config.VERSION;


public class Gmpc.Provider.MusicTree : Gmpc.Plugin.Base, Gmpc.Plugin.MetaDataIface 
{
    private GLib.Regex image_filename = null;
    public const int[] version = {0,0,2};

    
    public override weak int[] get_version() {
        return this.version;
    }

    public override weak string get_name() {
        return N_("Music Tree Provider");
    }

    construct {
        this.plugin_type = 8+32;
        /* Todo get list from gdk? */
        try {
			image_filename = new GLib.Regex(".*\\.(png|jpg|jpeg|gif)$", 
					GLib.RegexCompileFlags.CASELESS|GLib.RegexCompileFlags.DOTALL, 0);
		}catch( Error e) {
            log("Gmpc.Plugin.MusicTreeProvider", GLib.LogLevelFlags.LEVEL_ERROR, 
                    "Failed to create regex: %s", e.message);
        }
    }

    public void set_priority(int priority)
    {
        config.set_int(this.get_name(),"priority",priority);
    }
    public int get_priority()
    {
        return config.get_int_with_default(this.get_name(),"priority",0);
    }

    public void get_metadata(MPD.Song song, Gmpc.MetaData.Type type, MetaDataCallback callback)
    {
        List<Gmpc.MetaData.Item> list = null;
        string id =  profiles.get_current_id();
        weak string directory = null;

        log("Gmpc.Plugin.MusicTreeProvider", GLib.LogLevelFlags.LEVEL_DEBUG, 
                "Starting Query");

        if( id != null ) {
            directory = profiles.get_music_directory(id);
        }
       	log("Gmpc.Plugin.MusicTreeProvider", GLib.LogLevelFlags.LEVEL_DEBUG, "Got directory: %s",
			(directory == null)?"(null)":directory); 

        if(directory == null || directory.length == 0)
        {
            log("Gmpc.Plugin.MusicTreeProvider", GLib.LogLevelFlags.LEVEL_DEBUG, 
                    "No Music directory specified");
            /* With no directory specified, I cannot find anything */
            /* Signal this back */
            callback((owned)list);
            /* and stop trying */
            return;
        }
        /* Check song path */
        if(song == null || song.file == null) 
        {
            log("Gmpc.Plugin.MusicTreeProvider", GLib.LogLevelFlags.LEVEL_DEBUG, 
                    "No Song or no song path specified");
            callback((owned)list);
            /* and stop trying */
            return;
        }
        /* Check if it is a db file or not */
        string scheme = GLib.Uri.parse_scheme(song.file); 
        if(scheme != null)
        {
            log("Gmpc.Plugin.MusicTreeProvider", GLib.LogLevelFlags.LEVEL_DEBUG, 
                    "Scheme '%s' indicates no local file.",scheme);
            callback((owned)list);
            /* and stop trying */
            return;
        }
        log("Gmpc.Plugin.MusicTreeProvider", GLib.LogLevelFlags.LEVEL_DEBUG, 
                "Music directory: %s", directory);

        switch(type)
        {
            case Gmpc.MetaData.Type.ALBUM_ART:
                /* A request for an album cover came in. */
                this.get_album_cover(directory, song, callback);
                return; 
            case Gmpc.MetaData.Type.ARTIST_ART:
                /* A request for artist art came in. */
                this.get_artist_art(directory, song, callback);
                return; 
            default:
                break;
        }

        log("Gmpc.Plugin.MusicTreeProvider", GLib.LogLevelFlags.LEVEL_DEBUG, 
                "Query done, %u results", list.length());
        /* Tell that we found nothing */
        callback((owned)list);
    }

    /** 
     * Get artist art 
     */
     private async void get_artist_art(string directory, MPD.Song song, MetaDataCallback callback)
     {
        List<Gmpc.MetaData.Item> list = null;

        log("Gmpc.Plugin.MusicTreeProvider", GLib.LogLevelFlags.LEVEL_DEBUG, 
                "Query artist art ");

        string base_path = GLib.Path.get_dirname(song.file); 
        log("Gmpc.Plugin.MusicTreeProvider", GLib.LogLevelFlags.LEVEL_DEBUG, 
                "Got basename: %s", base_path);


        string path = GLib.Path.build_filename(directory, base_path);
        var dir = File.new_for_path (path);
        List <string> queries = null;

        var ipath = GLib.Path.build_filename(path, "artist.jpg");
        queries.append(ipath);
        if(song.artist != null){
            ipath = GLib.Path.build_filename(path, "%s.jpg".printf(song.artist));
            queries.append(ipath);
        }
        dir = dir.get_parent();
        path = dir.get_path();
        ipath = GLib.Path.build_filename(path, "artist.jpg");
        queries.append(ipath);
        if(song.artist != null){
            ipath = GLib.Path.build_filename(path, "%s.jpg".printf(song.artist));
            queries.append(ipath);
        }
        dir = dir.get_parent();
        path = dir.get_path();
        ipath = GLib.Path.build_filename(path, "artist.jpg");
        queries.append(ipath);
        if(song.artist != null){
            ipath = GLib.Path.build_filename(path, "%s.jpg".printf(song.artist));
            queries.append(ipath);
        }
        dir = null;

        foreach(string opath in queries)
        {
            log("Gmpc.Plugin.MusicTreeProvider", GLib.LogLevelFlags.LEVEL_DEBUG, 
                    "Got path: %s. Checking info", opath);
            /* Start async file info */
            dir = File.new_for_path (opath);
            try {
                var e = yield dir.query_info_async("access::can-read", 0,Priority.DEFAULT,null);
                log("Gmpc.Plugin.MusicTreeProvider", GLib.LogLevelFlags.LEVEL_DEBUG, 
                        "Got path: %s. Have info", opath);
                if(e.get_attribute_boolean("access::can-read"))
                {
                    MetaData.Item item = new MetaData.Item();
                    item.type = Gmpc.MetaData.Type.ALBUM_ART;
                    item.plugin_name = this.get_name();
                    item.content_type = MetaData.ContentType.URI;
                    item.set_uri(opath);
                    list.append((owned)item);
                    log("Gmpc.Plugin.MusicTreeProvider", GLib.LogLevelFlags.LEVEL_DEBUG, 
                            "Found %s, adding",opath);
                }

            } catch (GLib.Error err) {
                log("Gmpc.Plugin.MusicTreeProvider", GLib.LogLevelFlags.LEVEL_DEBUG,
                        "Error trying to get file info from '%s': %s\n",opath, err.message);
            }
        }

        log("Gmpc.Plugin.MusicTreeProvider", GLib.LogLevelFlags.LEVEL_DEBUG, 
                "Query done, %u results", list.length());
        list.first();
        callback((owned)list);

        return ;
     }
    /** 
     * Get album cover
     */
     private async void get_album_cover(string directory, MPD.Song song, MetaDataCallback callback)
     {
        List<Gmpc.MetaData.Item> list = null;

        log("Gmpc.Plugin.MusicTreeProvider", GLib.LogLevelFlags.LEVEL_DEBUG, 
                "Query album cover");

        string base_path = GLib.Path.get_dirname(song.file); 
        log("Gmpc.Plugin.MusicTreeProvider", GLib.LogLevelFlags.LEVEL_DEBUG, 
                "Got basename: %s", base_path);

        string path = GLib.Path.build_filename(directory, base_path);

        log("Gmpc.Plugin.MusicTreeProvider", GLib.LogLevelFlags.LEVEL_DEBUG, 
                "Got path: %s. Starting to itterate over childs", path);
        /* Start async directory walking */
        var dir = File.new_for_path (path);
        try {
            var e = yield dir.enumerate_children_async (FILE_ATTRIBUTE_STANDARD_NAME, 0, Priority.DEFAULT, null);
            while (true) {
                var files = yield e.next_files_async (10, Priority.DEFAULT, null);
                if (files == null) {
                    break;
                }
                foreach (var info in files) {
                    if(image_filename.match(info.get_name(), 0, null))
                    {
                        MetaData.Item item = new MetaData.Item();
                        item.type = Gmpc.MetaData.Type.ALBUM_ART;
                        item.plugin_name = this.get_name();
                        item.content_type = MetaData.ContentType.URI;
                        item.set_uri(GLib.Path.build_filename(path,info.get_name()));
                        list.append((owned)item);
                        log("Gmpc.Plugin.MusicTreeProvider", GLib.LogLevelFlags.LEVEL_DEBUG, 
                                "Found %s, adding",info.get_name()); 
                        /* Match filename against rules */
                    }
                }
            }
        } catch (GLib.Error err) {
            log("Gmpc.Plugin.MusicTreeProvider", GLib.LogLevelFlags.LEVEL_WARNING,
                    "Error trying to walk directory '%s': %s\n",path, err.message);
        }
        /* Try parent, if this fails */
        if(list.length() == 0)
        {
            if(GLib.Regex.match_simple("(DISC|CD)[ 0-9]*$", path, GLib.RegexCompileFlags.CASELESS,0))
            { 
				var pdir = dir.get_parent();
				path = pdir.get_path();
                log("Gmpc.Plugin.MusicTreeProvider", GLib.LogLevelFlags.LEVEL_DEBUG, 
                        "Nothing found, trying parent: %s", path);
                try {
                    var pe = yield pdir.enumerate_children_async (FILE_ATTRIBUTE_STANDARD_NAME, 0, Priority.DEFAULT, null);
                    while (true) {
                        var pfiles = yield pe.next_files_async (10, Priority.DEFAULT, null);
                        if (pfiles == null) {
                            break;
                        }
                        foreach (var pinfo in pfiles) {
                            /* Match filename against rules */
                            if(image_filename.match(pinfo.get_name(), 0, null))
                            {
                                MetaData.Item pitem = new MetaData.Item();
                                pitem.type = Gmpc.MetaData.Type.ALBUM_ART;
                                pitem.plugin_name = this.get_name();
                                pitem.content_type = MetaData.ContentType.URI;
                                pitem.set_uri(GLib.Path.build_filename(path,pinfo.get_name()));
                                list.append((owned)pitem);
                                log("Gmpc.Plugin.MusicTreeProvider", GLib.LogLevelFlags.LEVEL_DEBUG, 
                                        "Found %s, adding",pinfo.get_name()); 
                            }
                        }
                    }
                } catch (GLib.Error perr) {
                    log("Gmpc.Plugin.MusicTreeProvider", GLib.LogLevelFlags.LEVEL_WARNING,
                            "Error trying to walk parent of directory '%s': %s\n",path, perr.message);
                }
            }
        }

        log("Gmpc.Plugin.MusicTreeProvider", GLib.LogLevelFlags.LEVEL_DEBUG, 
                "Query done, %u results", list.length());
        list.first();
        callback((owned)list);

        return ;
     }
}
