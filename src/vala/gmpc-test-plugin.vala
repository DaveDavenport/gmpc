using GLib;
using Gtk;
using Gmpc;

public class  Gmpc.TestPlugin : Gmpc.PluginBase {
    public override weak string get_name() {
        return "Vala test plugin";
    }

    public override void save_yourself()
    {
        stdout.printf("Vala plugin save myself\n");
        /* nothing to save */
    }

    public override bool get_enabled() {
        return true;
    }
    public override void set_enabled(bool state) {
        
    }


    /* Plugin functions */
    private void connection_changed(Gmpc.Connection conn,MPD.Server server, int connect){
        stdout.printf("Connection changed: %i\n", connect);
    }
    construct {
        stdout.printf("create %s\n", this.get_name());

        gmpcconn.connection_changed += connection_changed;
    }
    ~TestPlugin() {
        stdout.printf("Destroy vala plugin\n");

    }
}
