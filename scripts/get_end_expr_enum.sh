#! /usr/bin/env bash

while [ $# -gt 0 ]; do
    case $1 in
        -f) last=0 ;;
        -l) last=1 ;;
        -*) echo "unknown flag ${1}"; exit 1 ;;
    esac
    shift
done

enum=$(cat expr.h | grep -A1000 'typedef enum {' | grep -B1000 '} expr_t;' | grep '// "')
if [ $last -eq 1 ]; then
    echo "$enum" | tail -n 1 | cut -d ',' -f 1
else
    echo "$enum" | head -n 1 | cut -d ',' -f 1
fi
