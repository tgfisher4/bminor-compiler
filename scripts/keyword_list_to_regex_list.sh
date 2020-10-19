#! /usr/bin/env bash

input_file=${1}
for keyword in $(cat ${input_file}); do
    if [ ! -z "${to_return}" ]; then
        to_return="${to_return}\n"
    fi
    to_return="${to_return}${keyword}\t\t\t{ return $(echo "${keyword}" | tr [:lower:] [:upper:]); }"
done

echo "${to_return}"
