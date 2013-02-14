#!/bin/sh

scheme="racket -f"

if [ -z "$1" ]; then exit 1; fi

lib_path=`cd $(dirname "$0"); pwd`
data_path=`pwd`

cd "$lib_path"

$scheme compiler.ss -e "(shell:binary-compiler \"$data_path/$1\" \"$data_path/$1.bin\")"
