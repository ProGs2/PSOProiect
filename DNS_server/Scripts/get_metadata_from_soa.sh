#!/bin/bash

file_path=$1
field=$2

case $field in
    serial)
        grep -E '^[[:space:]]*[0-9]{10}[[:space:]]*;' "$file_path" | awk '{print $1}'
        ;;
    refresh)
        grep -E '^[[:space:]]*[0-9]+[[:space:]]*;[[:space:]]*Refresh' "$file_path" | awk '{print $1}'
        ;;
    retry)
        grep -E '^[[:space:]]*[0-9]+[[:space:]]*;[[:space:]]*Retry' "$file_path" | awk '{print $1}'
        ;;
    expire)
        grep -E '^[[:space:]]*[0-9]+[[:space:]]*;[[:space:]]*Expire' "$file_path" | awk '{print $1}'
        ;;
    minimum_ttl)
        grep -E '^[[:space:]]*[0-9]+[[:space:]]*;[[:space:]]*Minimum TTL' "$file_path" | awk '{print $1}'
        ;;
    *)
        echo "Invalid field. Choose: serial, refresh, retry, expire, minimum_ttl"
        exit 1
        ;;
esac