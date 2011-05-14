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
 * This plugin queries HTBackdrops.com for artist images (backdrops)
 */

using Config;
using Gmpc;
using Gmpc.Plugin;
using Xml;

private const bool use_transition_ppbd = Gmpc.use_transition;
private const string some_unique_name_ppbd = Config.VERSION;
private const string log_domain_ppbd = "Gmpc.Provider.HTBackdrops";

public class Gmpc.Provider.HTBackdrops: 
            Gmpc.Plugin.Base,Gmpc.Plugin.MetaDataIface 
{
    private const int[] version = {0,0,2};
    
    public override unowned int[] get_version() {
        return this.version;
    }

    public override unowned string get_name() {
        return N_("Home Theater Backdrops");
    }

    construct {
        this.plugin_type = 8+32;
    }

    public void set_priority(int priority) {
        config.set_int(this.get_name(),"priority",priority);
    }

    public int get_priority() {
        return config.get_int_with_default(this.get_name(),"priority",0);
    }

    public void get_metadata(MPD.Song song, Gmpc.MetaData.Type type, MetaDataCallback callback)
    {

        if(song == null || song.artist == null)
        {
            log(log_domain_ppbd, GLib.LogLevelFlags.LEVEL_DEBUG, 
                    "Insufficient information. doing nothing");
            /* Tell that we found nothing */
            callback(null);
            return;
        }
        switch(type)
        {
            case Gmpc.MetaData.Type.ARTIST_ART:
                /* A request for artist art came in. */
                this.get_artist_art(song, callback);
                return; 
			case Gmpc.MetaData.Type.ARTIST_TXT:	
			case Gmpc.MetaData.Type.ARTIST_SIMILAR:	
			case Gmpc.MetaData.Type.ALBUM_ART:
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

    /* HT Backdrops specific code */
    const string query = "http://htbackdrops.com/api/%s/searchXML?keywords=%s&default_operator=and&fields=title";
    const string download_url = "http://htbackdrops.com/api/%s/download/%s/fullsize";
    const string thumbnail_url = "http://htbackdrops.com/api/%s/download/%s/thumbnail";
    const string api_key = "b3085ed18168f083aa69179b3364c9d8";
    
    private void add_image(ref List<Gmpc.MetaData.Item> list, Xml.Node *image)
    {
        for(Xml.Node *entries = image->children;
                      entries != null; 
                      entries = entries->next)
        {
            if(entries->name == "id")
            {
                var path = download_url.printf(api_key, entries->get_content());
                var thumb_path = thumbnail_url.printf(api_key, entries->get_content());
                log(log_domain_ppbd, GLib.LogLevelFlags.LEVEL_DEBUG, 
                        "Entry : %s", path);
                MetaData.Item pitem = new MetaData.Item();
                pitem.type = Gmpc.MetaData.Type.ARTIST_ART;
                pitem.plugin_name = get_name();
                pitem.content_type = MetaData.ContentType.URI;
                pitem.set_uri(path);
                pitem.set_thumbnail_uri(thumb_path);
                
                list.append((owned)pitem);
            }
        }
    }

    private void parse_data(ref List<Gmpc.MetaData.Item> list, uchar[] data)
    {
       Xml.Doc *doc = Xml.Parser.parse_memory((string)data, data.length); 
       if(doc == null) return;
       
       Xml.Node *root = doc->get_root_element();
       if(root != null)
       {
            for(Xml.Node *child = root->children; 
                                child != null ; 
                                child = child->next)
            {
                if(child->name == "images")
                {
                    for ( Xml.Node *image = child->children; 
                                image != null; 
                                image = image->next)
                    {
                        if(image->name == "image"){
                            add_image(ref list, image);                        
                        }
                    }

                }
            }
       }

    }

    private void handle_download(Gmpc.AsyncDownload.Handle handle, 
                                 Gmpc.AsyncDownload.Status status, 
                                 void *d)
    {
        Prop *p = (Prop *)d;
        if(status == Gmpc.AsyncDownload.Status.DONE) { 
            List<Gmpc.MetaData.Item> list = null;
            var data = handle.get_data();
            if(data != null)
                parse_data(ref list , data);

            log(log_domain_ppbd, GLib.LogLevelFlags.LEVEL_DEBUG, 
                    "Download done: results: %u ", list.length());
            p->callback((owned)list);
            /* Cleanup */
            delete p;
        }else if (status == Gmpc.AsyncDownload.Status.PROGRESS) {
            // do nothing when downloading 
        }else{
            /* Nothying found, or error, so cleanup */
            List<Gmpc.MetaData.Item> list = null;
            p->callback((owned)list);
            /* delete Prop */
            delete p;
        }
    }
    /** 
     * Get artist art 
     */
     private void get_artist_art(MPD.Song song, MetaDataCallback callback)
     {

        Prop *p = new Prop();
        p->this = this;
        p->callback = callback;
        var path = query.printf(api_key, 
                                Gmpc.AsyncDownload.escape_uri(song.artist));
        log(log_domain_ppbd, GLib.LogLevelFlags.LEVEL_DEBUG, 
                "Query artist art: %s ", path);
        Gmpc.AsyncDownload.download_vala(path, p, handle_download);
     }
    /**
     * This class is used to go around the limitations of 
     *  GmpcEasyAsyncDownload (it is no GLibSimpleAsync implementation.) 
     */
    [Compact]
    private class Prop {
        public HTBackdrops this;
        public MetaDataCallback callback;
    }
}
