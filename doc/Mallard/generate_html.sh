#!/bin/bash

# yes this might not work on all installs, sorry.

for a in *.page
do
    gnome-doc-tool xhtml $a
done    
