#!/bin/bash

for a in icons/scalable/status/stylized-*cover.svg icons/scalable/status/*-artist.svg; do
    for b in "48x48" "64x64" "96x96" "128x128"; do
        output=${a/scalable/$b};
        inkscape -z -D -e "${output/svg/png}" "$a"
        convert -scale $b "${output/svg/png}" "${output/svg/png}"
    done
done
