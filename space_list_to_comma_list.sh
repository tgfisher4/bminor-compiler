#! /usr/bin/env bash

while [ $# -gt 0 ]; do
    case $1 in
        -q) quote=1 ;;
        -u) upper=1      ;;
        -f) shift; input_file=$1 ;; 
        -*) exit 1      ;;
    esac
    shift
done

maybe_quote=$(if [ ! -z $quote ]; then echo "\""; fi)
alphabet=$(if [ ! -z $upper ]; then echo "[:upper:]"; else echo "[:lower:]"; fi)

for keyword in $(cat ${input_file} | awk '{print $NF}'); do
    if [ -z "${cskw}" ]; then
        cskw="${maybe_quote}$(echo ${keyword} | tr [:lower:] ${alphabet})${maybe_quote}"
    else
        cskw="${cskw},${maybe_quote}$(echo ${keyword} | tr [:lower:] ${alphabet})${maybe_quote}"
    fi
done

echo "${cskw}"
