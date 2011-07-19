#!/bin/bash

# yes this might not work on all installs, sorry.
# calling bash fixes calling gnome-doc-tool on debian.

for a in *.page
do
    bash gnome-doc-tool xhtml $a
done    
