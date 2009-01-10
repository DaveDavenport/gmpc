#!/bin/bash

for a in icons/scalable/status/stylized-*cover.svg icons/scalable/status/*-artist.svg; do
    for b in "48" "64" "96" "128"; do
        size="$bx$b"
        output=${a/scalable/$size};
        inkscape -z -D -w $b -h $b -e "${output/svg/png}" "$a"
#        convert -scale $b "${output/svg/png}" "${output/svg/png}"
    done
done
