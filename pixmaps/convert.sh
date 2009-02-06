#!/bin/bash

for a in icons/scalable/status/stylized-*cover.svg icons/scalable/status/*-artist.svg; do
    for b in "48" "64" "72" "96" "128"; do
        size=$b'x'$b
        output=${a/scalable/$size};
        width=`inkscape -W "$a"`
        height=`inkscape -H "$a"`
        echo ${height/.*/} -ge ${width/.*/}
        if [ ${height/.*/} -ge ${width/.*/} ]; then 
            echo inkscape -z -D -h $b -e "${output/svg/png}" "$a";
            inkscape -z -D -h $b -e "${output/svg/png}" "$a";
        else
            echo inkscape -z -D -w $b -e "${output/svg/png}" "$a";
            inkscape -z -D -w $b -e "${output/svg/png}" "$a";
        fi
#        inkscape -z -D -w $b -h $b -e "${output/svg/png}" "$a"
#        convert -scale $b "${output/svg/png}" "${output/svg/png}"
    done
done
