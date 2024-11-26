#!/bin/bash

zone_file="$1"

awk '/IN\s+(A|MX)\s+/ {print $1}' "$zone_file" | tr '\n' ' ' && echo