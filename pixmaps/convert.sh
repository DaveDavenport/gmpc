#!/bin/bash

for a in icons/scalable/status/stylized-*.svg; do
    output=${a/scalable/64x64};
    convert -scale 64x64 "$a" "${output/svg/png}"
done
