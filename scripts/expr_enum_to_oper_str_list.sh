#! /usr/bin/env bash

PARENT="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

res=$(cat expr.h | grep -A1000 'typedef enum {' | grep -B1000 '} expr_t;' | grep '// "' | sed -E 's|^.*(".*").*$|\1,|g' | sed -E 's|([|&*])|\\\1|g')
# | ${PARENT}/reformat_space_list.sh | sed -E 's|([&*|])|\\\1|g')

echo "{${res:0:${#res}-1}}"
