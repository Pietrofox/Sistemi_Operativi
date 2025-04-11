#!/bin/bash

container="/tmp/container3"
if [ ! -d $container ]; then
    mkdir  $container
fi

i=0
IFS=$'\n'
for riga in $(cat "$1")
        do
            let "i=i+1"
            source=$(echo $riga | cut -d " " -f 1)
            dest=$(echo $riga | cut -d " " -f 2)
            if [ -d $source ]; then
                mkdir -p "$container$dest"                
                bindfs --no-allow-other "$source" "$container$dest" #container/dest
            else
                a=$(dirname "$dest")
                realDest="$container$a"
                mkdir -p "$realDest"
                cp "$source" "$realDest" 
            fi
        done
fakechroot chroot $container "${@:2}"

#./container-run.sh conf-file.txt /bin/ls