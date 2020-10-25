#! /usr/bin/env bash

while [ $# -gt 0 ]; do
    case $1 in
        -f) last=0 ;;
        -l) last=1 ;;
        -*) echo "unknown flag ${1}"; exit 1 ;;
    esac
    shift
done

cat type.h | grep -A1000 'typedef enum {' | grep -B1000 '} type_t;' | grep 'TYPE_' | head -n 1 | cut -d ',' -f 1
