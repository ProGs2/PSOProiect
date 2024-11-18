#!/bin/bash

zone_file=$1
ns_record=$2

# Extract lines 9 and 10
line9=$(sed -n '9p' "$zone_file")
line10=$(sed -n '10p' "$zone_file")

if [[ "$ns_record" == "ns1" ]]
then
    var=`echo $line9 | cut -d" " -f3`
    echo $var
else
    var=`echo $line10 | cut -d" " -f3`
    echo $var
fi