#!/bin/bash

file_path=$1
nr_line=$2

line=$(sed -n "${nr_line}p" "$file_path")

type=`echo $line | cut -d" " -f4`
echo $type