#!/bin/bash

file_name=$1
domain_name=$2

grep -A 2 "zone \"$domain_name\"" "$file_name" | grep 'file' | sed -E 's/.*file "(.*)";.*/\1/'