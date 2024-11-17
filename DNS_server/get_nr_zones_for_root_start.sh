#!/bin/bash

filename="$1"

# Check if the file exists
if [ ! -f "$filename" ]; then
    echo "error"
    exit 1
fi

# Count the lines starting with "zone"
count=$(grep -c '^zone' "$filename")

# Output the result
echo $count