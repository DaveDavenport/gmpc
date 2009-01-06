#!/bin/bash

for a in icons/scalable/status/stylized-*-cover.svg; do
    for b in "48x48" "64x64" "96x96" "128x128"; do
        output=${a/scalable/$b};
        convert -scale $b "$a" "${output/svg/png}"
    done
done
