#! /bin/sh

sed -i -r \
    -e 's/\t/    /g' \
    -e 's/ +$//' \
"$@"
