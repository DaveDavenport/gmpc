#!/bin/bash

function build_file()
{
    echo "building: '$1'";
    if test $1 -nt $1.stamp; then 
        valac -C --pkg gtk+-2.0 --pkg cairo --pkg config --pkg gmpc --pkg libmpd --pkg gmpc-plugin --vapidir=`pwd` $1 $2 && touch $1.stamp
    fi;

}

valac -C gmpc-plugin.vala  --library=gmpc-plugin --pkg gtk+-2.0 --pkg cairo --pkg libmpd --pkg gmpc --vapidir=`pwd` 
#-H gmpc-plugin.h

build_file "gmpc-image.vala"
build_file "gmpc-progress.vala"
build_file "gmpc-rating.vala"
build_file "gmpc-easy-command.vala"
build_file "gmpc-song-links.vala"
build_file "gmpc-favorites.vala"
build_file "gmpc-test-plugin.vala"
build_file "gmpc_menu_item_rating.vala" "gmpc-rating.vala"
build_file "gmpc-liststore-sort.vala"

valac -C --pkg libmpd --vapidir=`pwd` "gmpc-connection.vala"
