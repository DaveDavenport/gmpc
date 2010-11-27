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

using Config;
using GLib;
using Gmpc;
using Gmpc.Plugin;

static const string LOG_DOMAIN_GMC="GMC";
private const bool use_transition_gmc = Gmpc.use_transition;
private const string some_unique_name_gmc = Config.VERSION;

public class Header
{
        public enum Type {
                ALBUM_ART,
		ALBUM_INFORMATION,
                ARTIST_ART,
		ARTIST_BIOGRAPHY,
		SONG_LYRICS
        }
        public Type type;
        public string  artist = "";
        public string  album  = "";
        public string  title  = "";
        public string  file   = "";
}

class Gmpc.Provider.GMC : Gmpc.Plugin.Base, Gmpc.Plugin.MetaDataIface
{
	private const int[] version = {0,0,2};
	public override unowned int[] get_version() {
		return this.version;
	}
	public override unowned string get_name() {
		return N_("GMC Provider");
	}
	construct {
		this.plugin_type = 32;
		/* Todo get list from gdk? */
	}

	public void set_priority(int priority)
	{
		config.set_int(this.get_name(),"priority",priority);
	}
	public int get_priority()
	{
		return config.get_int_with_default(this.get_name(),"priority",0);
	}

	private void get_metadata_song_txt(MPD.Song song, MetaDataCallback callback)
	{
		if(song == null) {
			GLib.log(LOG_DOMAIN_GMC, GLib.LogLevelFlags.LEVEL_DEBUG,"no song");
			callback(null);
			return;
		}
		if(song.file == null) {
			GLib.log(LOG_DOMAIN_GMC, GLib.LogLevelFlags.LEVEL_DEBUG,"no file");
			callback(null);
			return;
		}
		GLib.log(LOG_DOMAIN_GMC, GLib.LogLevelFlags.LEVEL_DEBUG,"Create header");
		Header h = new Header();
		h.type = Header.Type.SONG_LYRICS;
		h.artist = song.artist;
		h.title = song.title;
		h.file = song.file;
		process_request(h,song, callback, Gmpc.MetaData.Type.SONG_TXT);
	}
	private void get_metadata_artist_biography(MPD.Song song, MetaDataCallback callback)
	{
		if(song == null) {
			GLib.log(LOG_DOMAIN_GMC, GLib.LogLevelFlags.LEVEL_DEBUG,"no song");
			callback(null);
			return;
		}
		if(song.file == null) {
			GLib.log(LOG_DOMAIN_GMC, GLib.LogLevelFlags.LEVEL_DEBUG,"no file");
			callback(null);
			return;
		}
		if(song.artist == null) {
			GLib.log(LOG_DOMAIN_GMC, GLib.LogLevelFlags.LEVEL_DEBUG,"no artist");
			callback(null);
			return;
		}
		GLib.log(LOG_DOMAIN_GMC, GLib.LogLevelFlags.LEVEL_DEBUG,"Create header");
		Header h = new Header();
		h.type = Header.Type.ARTIST_BIOGRAPHY;
		h.artist = song.artist;
		h.file = song.file;
		process_request(h,song, callback, Gmpc.MetaData.Type.ARTIST_TXT);
	}
    
	private void get_metadata_artist(MPD.Song song, MetaDataCallback callback)
	{
		if(song == null) {
			GLib.log(LOG_DOMAIN_GMC, GLib.LogLevelFlags.LEVEL_DEBUG,"no song");
			callback(null);
			return;
		}
		if(song.file == null) {
			GLib.log(LOG_DOMAIN_GMC, GLib.LogLevelFlags.LEVEL_DEBUG,"no file");
			callback(null);
			return;
		}
		if(song.artist == null) {
			GLib.log(LOG_DOMAIN_GMC, GLib.LogLevelFlags.LEVEL_DEBUG,"no artist");
			callback(null);
			return;
		}
		GLib.log(LOG_DOMAIN_GMC, GLib.LogLevelFlags.LEVEL_DEBUG,"Create header");
		Header h = new Header();
		h.type = Header.Type.ARTIST_ART;
		h.artist = song.artist;
		h.file = song.file;
		process_request(h,song, callback, Gmpc.MetaData.Type.ARTIST_ART);
	}

	private void get_metadata_album(MPD.Song song, MetaDataCallback callback)
	{
		if(song == null) {
			GLib.log(LOG_DOMAIN_GMC, GLib.LogLevelFlags.LEVEL_DEBUG,"no song");
			callback(null);
			return;
		}
		if(song.file == null) {
			GLib.log(LOG_DOMAIN_GMC, GLib.LogLevelFlags.LEVEL_DEBUG,"no file");
			callback(null);
			return;
		}
		if(song.album == null) {
			GLib.log(LOG_DOMAIN_GMC, GLib.LogLevelFlags.LEVEL_DEBUG,"no album");
			callback(null);
			return;
		}
		GLib.log(LOG_DOMAIN_GMC, GLib.LogLevelFlags.LEVEL_DEBUG,"Create header");
		Header h = new Header();
		h.type = Header.Type.ALBUM_ART;
		h.album = song.album;
		h.file = song.file;
		process_request(h,song, callback, Gmpc.MetaData.Type.ALBUM_ART);

	}
	private void get_metadata_album_information(MPD.Song song, MetaDataCallback callback)
	{
		if(song == null) {
			GLib.log(LOG_DOMAIN_GMC, GLib.LogLevelFlags.LEVEL_DEBUG,"no song");
			callback(null);
			return;
		}
		if(song.file == null) {
			GLib.log(LOG_DOMAIN_GMC, GLib.LogLevelFlags.LEVEL_DEBUG,"no file");
			callback(null);
			return;
		}
		if(song.album == null) {
			GLib.log(LOG_DOMAIN_GMC, GLib.LogLevelFlags.LEVEL_DEBUG,"no album");
			callback(null);
			return;
		}
		GLib.log(LOG_DOMAIN_GMC, GLib.LogLevelFlags.LEVEL_DEBUG,"Create header");
		Header h = new Header();
		h.type = Header.Type.ALBUM_INFORMATION;
		h.album = song.album;
		h.file = song.file;
		process_request(h,song, callback, Gmpc.MetaData.Type.ALBUM_TXT);

	}
	public void get_metadata(MPD.Song song, Gmpc.MetaData.Type type, MetaDataCallback callback)
	{
		GLib.log(LOG_DOMAIN_GMC, GLib.LogLevelFlags.LEVEL_DEBUG,"query GMC");
		if(type == Gmpc.MetaData.Type.ALBUM_ART) {
			this.get_metadata_album(song, callback);
			return;
        } else if(type == Gmpc.MetaData.Type.ALBUM_TXT) {
            this.get_metadata_album_information(song, callback);
            return;
		}else if (type == Gmpc.MetaData.Type.ARTIST_ART) {
			this.get_metadata_artist(song, callback);
			return;
		}else if (type == Gmpc.MetaData.Type.ARTIST_TXT) {
			this.get_metadata_artist_biography(song, callback);
			return;
		} else if (type == Gmpc.MetaData.Type.SONG_TXT) {
			this.get_metadata_song_txt(song, callback);
			return;
		}


		GLib.log(LOG_DOMAIN_GMC, GLib.LogLevelFlags.LEVEL_DEBUG,"Wrong type");
		callback(null);
	}


	private async void process_request(Header h, MPD.Song song, MetaDataCallback callback, Gmpc.MetaData.Type gmt_type)
	{
		GLib.log(LOG_DOMAIN_GMC, GLib.LogLevelFlags.LEVEL_DEBUG,"Process request");
		GLib.SocketClient client = new GLib.SocketClient();
		GLib.SocketConnection conn = null;
        /* TODO make hostname an option */
		try{
			conn = yield client.connect_to_host_async("localhost", 6601);
		}catch (Error err8) {
			callback(null);
			GLib.debug("Failed to connect: %s".printf(err8.message));
			return;
		}
		var output = conn.get_output_stream();
		var input = conn.get_input_stream();
		var out_data = new GLib.DataOutputStream(output);
        out_data.set_byte_order(GLib.DataStreamByteOrder.LITTLE_ENDIAN);
		uint32 type = h.type;
		try{
			out_data.put_uint32(type);
		}catch (Error err0) {
			callback(null);
			GLib.debug("failed to serialize header");                
            return;
		}
		/*  Artist */
		uint32 size = (uint32)h.artist.size();
		try{
			out_data.put_uint32(size);
		}catch (Error err1) {
			callback(null);
			GLib.debug("failed to serialize header");                
            return;
		}
		if(size > 0) {
			out_data.write(h.artist, size);
		}
		/*  Album */
		size = (uint32)h.album.size();
		try{
			out_data.put_uint32(size);
		}catch (Error err2) {
			callback(null);
			GLib.debug("failed to serialize header");                
            return;
		}
		if(size > 0) {
			out_data.write(h.album, size);
		}
		/*  Title */
		size = (uint32)h.title.size();
		try{
			out_data.put_uint32(size);
		}catch (Error err3) {
			callback(null);
			GLib.debug("failed to serialize header");                
            return;
		}
		if(size > 0) {
			out_data.write(h.title, size);
		}

		/*  File */
		size = (uint32)h.file.size();
		try{
			out_data.put_uint32(size);
		}catch (Error err4) {
			callback(null);
			GLib.debug("failed to serialize header");                
            return;
		}
		if(size > 0) {
			out_data.write(h.file, size);
		}
		size = 0;

		var in_data = new GLib.DataInputStream(input);
        in_data.set_byte_order(GLib.DataStreamByteOrder.LITTLE_ENDIAN);
		try{
			uint32 rsize = 0; 
			ssize_t rs2 = yield in_data.read_async(&rsize, 4,0);
            /* TODO: why does the async read_async fail? seems hussled byte order.. */
            //int rs2 = 4;
            //rsize = in_data.read_uint32();
            GLib.log(LOG_DOMAIN_GMC, GLib.LogLevelFlags.LEVEL_DEBUG,"Read size: %u:%i",rsize,
                (int)in_data.get_byte_order()
            );                
            if(rs2  == 4) {
                /* Why need to convert endianess? */
				size = uint32.from_little_endian(rsize);
			}else {
                GLib.log(LOG_DOMAIN_GMC, GLib.LogLevelFlags.LEVEL_DEBUG,"failed to read size");                
                callback(null);
                return;
            }

		}catch (Error err6) {
			GLib.log(LOG_DOMAIN_GMC, GLib.LogLevelFlags.LEVEL_DEBUG,"failed to read size");                
			callback(null);
			return;
		}

		if(size == 0) {
			GLib.log(LOG_DOMAIN_GMC, GLib.LogLevelFlags.LEVEL_DEBUG,"File size is null");
			callback(null);
			return;
		}
        GLib.log(LOG_DOMAIN_GMC, GLib.LogLevelFlags.LEVEL_DEBUG, "File size is: %u\n", size);

		uchar[] buffer= new uchar[size];
		try{
			ssize_t rs=0, total=0;
			do{
				rs = yield in_data.read_async(&buffer[total], 
						(size-total >1024)?1024:(size-total),0); 

				total+=rs;
				GLib.log(LOG_DOMAIN_GMC, GLib.LogLevelFlags.LEVEL_DEBUG,"read : %u:%u\n",(uint32)size, (uint32)total);
			}while(rs>0);
		}catch(Error err) {
			GLib.log(LOG_DOMAIN_GMC, GLib.LogLevelFlags.LEVEL_DEBUG,"Failed to recieve image.");
			callback(null);
			return;
		}
        GLib.log(LOG_DOMAIN_GMC, GLib.LogLevelFlags.LEVEL_DEBUG,"read done: %u:",(uint32)size);

		List<Gmpc.MetaData.Item> list = null;

		Gmpc.MetaData.Item item = new MetaData.Item();
		item.type = gmt_type; 
		item.plugin_name = "GMC Fetcher";
		item.content_type = MetaData.ContentType.RAW;
		item.content = (void *)(owned)buffer;
		item.size = (int)size;
		list.append((owned)item);
		callback((owned)list);

	}
}
