#! /usr/bin/env bash

while [ $# -gt 0 ]; do
    case $1 in
        -q) quote=1 ;;
        -u) upper=1      ;;
        -f) shift; input_file=$1 ;; 
        -t) bison_term=1    ;;
        -l) last_col=1       ;;
        -*) echo "unknown flag ${1}"; exit 1  ;;
    esac
    shift
done

maybe_quote=$(if [ ! -z $quote ]; then echo "\""; fi)
alphabet=$(if [ ! -z $upper ]; then echo "[:upper:]"; else echo "[:lower:]"; fi)
delim=$(if [ ! -z $bison_term ]; then echo -e "\n%token "; else echo ","; fi)
iterate_over="$(if [ ! -z $last_col ]; then echo $(cat ${input_file} | awk '{print $NF}'); else echo $(cat ${input_file}); fi)"

for keyword in $iterate_over; do
    if [ -z "${to_return}" ]; then
        to_return="${maybe_quote}$(echo ${keyword} | tr [:lower:] ${alphabet})${maybe_quote}"
    else
        to_return="${to_return}${delim}${maybe_quote}$(echo ${keyword} | tr [:lower:] ${alphabet})${maybe_quote}"
    fi
done

echo "$(if [ ! -z $bison_term ]; then echo -e "${delim}"; fi)${to_return}"
