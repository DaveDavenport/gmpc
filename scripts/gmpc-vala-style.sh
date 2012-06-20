#! /bin/sh

astyle -A1 -N -S -n "$@"
sed -i -r \
    -e '/CCode/s/ *= */ = /g' \
    -e '/CCode/s/ *, */, /g' \
    -e '/CCode/s/[ \t]+\]/]/g' \
    -e '/CCode/s/\[[ \t]+/[/g' \
    -e '/CCode/s/\([ \t]+/(/g' \
    -e '/CCode/s/[ \t]+\)/)/g' \
"$@"
