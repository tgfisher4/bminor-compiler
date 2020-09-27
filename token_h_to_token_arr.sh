#! /usr/bin/bash

input_file="${1}"
for token in $(awk '/{/,/}/' "${input_file}" | grep -v "[{}]" | tr -d '=0' | tr ',' '\n'); do
    if [ -z "${to_return}" ]; then
        to_return="\"${token}\""
    else
        to_return="${to_return},\"${token}\""
    fi
done

echo "{${to_return}}"
