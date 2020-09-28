#! /usr/bin/bash

input_file="${1}"
#for token in $(awk '/{/,/}/' "${input_file}" | grep -v "[{}]" | tr -d '=0' | tr ',' '\n'); do

echo {$(./reformat_space_list.sh -f <(cat bminor.bison | grep %token | sed -E 's|%token||g') -u -q)}

exit 0

for token in $(cat "${input_file}" | grep %token | sed -E 's|%token |,|g' | tr -d '\n '); do
for token in $(cat bminor.bison | grep %token | sed -E 's|%token||g'); do
    if [ -z "${to_return}" ]; then
        to_return="\"${token}\""
    else
        to_return="${to_return},\"${token}\""
    fi
done

echo "{${to_return}}"
