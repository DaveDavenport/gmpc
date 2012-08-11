using GLib;
using MPD;
using Gmpc;

public class Gmpc.Meta.Watcher : GLib.Object
{
    public signal void data_changed (MPD.Song song, MetaData.Type type, MetaData.Result result, MetaData.Item? item);

    public signal void force_reload();

    public void data_changed_cb(MPD.Song song, MetaData.Type type, MetaData.Result result, MetaData.Item? item)
    {
        data_changed(song, type, result, item);
    }
    public void force_reload_cb()
    {
        force_reload();
    }



}
