#!/bin/sh
# $Id: $

if [ $# -lt 1 ]; then
        echo USAGE: $0 name
        exit 1
fi

cd .. && make && make dist && cd - && ./make.sh $1
