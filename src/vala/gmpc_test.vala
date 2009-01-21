using Gtk;
using MPD;
using Gmpc;

namespace Gmpc.Misc {
    const Gmpc.Plugin plugin = Plugin() {
            type=1,
            name="Test Plugin"
    };

    public void test(MPD.Server server, MPD.Song song) {
        MPD.Song b = null;
        b = song;
        song.artist = "test";
        stdout.printf("test: %s\n", plugin.name);
    }



}

