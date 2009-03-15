#!/bin/bash

for a in *.po; do
    intltool-update ${a/.po/}
done
