#!/bin/ksh
# $Id: $

if [ $# -lt 1 ]; then
    echo $0 program to compile
    exit
fi

FILE=$1


doas cc -I/usr/include -I/usr/local/include -I/usr/local/include/postgresql -c -o ${FILE}.o ${FILE}.c
doas cc -static -L/usr/lib -L/usr/local/lib -L/usr/local/lib/postgresql -o ${FILE} ${FILE}.o -lkcgihtml -lkcgi -lz -lpq -lpgcommon -lpgport -lxml2 -lssl -lcrypto -lz -lreadline -ltermcap -lm

doas install -o www -g www -m 0500 ${FILE} /var/www/cgi-bin
doas install -o www -g www -m 0444 ${FILE}.html /var/www/htdocs
doas rm -f ${FILE}.o
