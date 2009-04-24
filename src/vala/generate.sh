#!/bin/bash

function build_file()
{
    valac -C --pkg gtk+-2.0 --pkg cairo --pkg config --pkg gmpc --pkg libmpd --pkg gmpc-plugin --vapidir=`pwd` $1 $2
    #-H ${1/.vala/.h} $1

}

valac -C gmpc-plugin.vala  --library=gmpc-plugin --pkg gtk+-2.0 --pkg cairo --pkg libmpd --pkg gmpc --vapidir=`pwd` 
#-H gmpc-plugin.h

build_file "gmpc_image.vala"
build_file "gmpc-progress.vala"
build_file "gmpc_rating.vala"
build_file "gmpc-easy-command.vala"
build_file "gmpc-song-links.vala"
build_file "gmpc-favorites.vala"
build_file "gmpc-test-plugin.vala"
build_file "gmpc_menu_item_rating.vala" "gmpc_rating.vala"

#valac -C gmpc_rating.vala  --pkg config --pkg gtk+-2.0 --pkg cairo --pkg libmpd --pkg gmpc --vapidir=`pwd`
#valac -C gmpc_menu_item_rating.vala gmpc_rating.vala --pkg config --pkg gtk+-2.0 --pkg cairo --pkg libmpd --pkg gmpc --vapidir=`pwd`
#valac -C gmpc-easy-command.vala  --pkg config --pkg gtk+-2.0 --pkg cairo --pkg libmpd --pkg gmpc --pkg gmpc-plugin --vapidir=`pwd`

#valac -C gmpc-song-links.vala  --pkg config --pkg gtk+-2.0 --pkg cairo --pkg libmpd --pkg gmpc --vapidir=`pwd`

#valac -C gmpc-test-plugin.vala --pkg config --pkg gtk+-2.0 --pkg cairo --pkg libmpd --pkg gmpc --pkg gmpc-plugin --vapidir=`pwd`
#valac -C gmpc-favorites.vala  --pkg config --pkg gtk+-2.0 --pkg cairo --pkg libmpd --pkg gmpc --pkg gmpc-plugin --vapidir=`pwd`
