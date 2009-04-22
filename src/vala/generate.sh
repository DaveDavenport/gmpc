valac -C gmpc_image.vala  --pkg gtk+-2.0 --pkg cairo
valac -C gmpc-progress.vala  --pkg gtk+-2.0 --pkg cairo


valac -C gmpc-plugin.vala  --library=gmpc-plugin --pkg gtk+-2.0 --pkg cairo --pkg libmpd --pkg gmpc --vapidir=`pwd`


valac -C gmpc_rating.vala  --pkg config --pkg gtk+-2.0 --pkg cairo --pkg libmpd --pkg gmpc --vapidir=`pwd`

valac -C gmpc_menu_item_rating.vala gmpc_rating.vala --pkg config --pkg gtk+-2.0 --pkg cairo --pkg libmpd --pkg gmpc --vapidir=`pwd`


valac -C gmpc-easy-command.vala  --pkg config --pkg gtk+-2.0 --pkg cairo --pkg libmpd --pkg gmpc --pkg gmpc-plugin --vapidir=`pwd`

valac -C gmpc-song-links.vala  --pkg config --pkg gtk+-2.0 --pkg cairo --pkg libmpd --pkg gmpc --vapidir=`pwd`

valac -C gmpc-test-plugin.vala --pkg config --pkg gtk+-2.0 --pkg cairo --pkg libmpd --pkg gmpc --pkg gmpc-plugin --vapidir=`pwd`


valac -C gmpc-favorites.vala  --pkg config --pkg gtk+-2.0 --pkg cairo --pkg libmpd --pkg gmpc --pkg gmpc-plugin --vapidir=`pwd`
