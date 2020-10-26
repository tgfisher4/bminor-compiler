#! /usr/bin/env bash

PARENT="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

while [ $# -gt 0 ]; do
    case $1 in
        -c) shift; column=$(($1+1)) ;;
        -*) echo "unknown flag ${1}" ;;
    esac
    shift
done

prec=2
comm=3
assc=4
strg=5

enum=$( cat expr.h | \
        grep -A1000 'typedef enum {' | \
        grep -B1000 '} expr_t' | \
        grep '//' | \
        sed -E 's|^.*//(.*)$|\1|' | \
        cut -d @ -f ${column} | \
        sed -E 's|([*])|\\\1|g')

res="$(case $column in
        ($prec) echo "$enum" | tail -n +2 | ${PARENT}/reformat_space_list.sh ;;
        ($comm) echo "$enum" | sed 's|no|false|' | sed 's|yes|true|' | ${PARENT}/reformat_space_list.sh ;;
        ($assc) echo "$enum" | sed 's|left|false|' | sed 's|right|true|' | ${PARENT}/reformat_space_list.sh ;;
        ($strg) echo "$enum" | sed -E 's|^.*(".*").*$|\1,|g' ;;
        (*) echo "other" ;;
        esac)"

res="$( if [ $column -eq $strg ]; then
            echo $res | sed 's|.$||'
        else
            echo $res
            fi)"

res=$(echo ${res} | sed -E 's|([*&|])|\\\1|g')
            
echo "{$res}"

