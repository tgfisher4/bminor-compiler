#! /usr/bin/env bash

PARENT="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

res=$(cat type.h | grep -A1000 'typedef enum {' | grep -B1000 '} type_t;' | grep 'TYPE' | sed -E 's|^.*TYPE_(.*),.*$|\1|g' | tr [:upper:] [:lower:] | ${PARENT}/reformat_space_list.sh -q)

echo "{$res}"
