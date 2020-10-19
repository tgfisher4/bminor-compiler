#! /usr/bin/env bash

while read -r line; do
    second=0
    # escape the * and & characters which are special to sed and | which I like to use as my sed selimiter (through which the output of this script will be passed
    for item in $(sed -r 's|([&*|])|\\\1|g' <<< $line); do
        if [ $second -eq 0 ]; then
            to_return="${to_return}\"${item}\""
            second=1
        else
            to_return="${to_return}\t\t\t{ return ${item}; }\n"
        fi
    done
    #sed -r 's|([*&])|\\n\\\1|g' <<<$line
    #sed -r 's|^([^\s]+)\s+([^\s]+)$|\\n"\1"\t\t\t{ return \2; }|' <<< $(sed -r 's|([*&])|\\n\\\1|g' <<<$line)
done < "${1}"

echo "${to_return}"
