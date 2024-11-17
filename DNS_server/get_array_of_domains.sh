#!/bin/bash

filename=$1

domains=()

while read -r line
do
    var=$(echo "$line" | egrep -o 'zone "[^"]+"')
    
    if [[ ! -z "$var" ]]; then
        domain=$(echo "$var" | egrep -o '"[^"]+"' | tr -d '"')
        domains+=("$domain")
    fi
done < "$filename"

echo "${domains[@]}"
