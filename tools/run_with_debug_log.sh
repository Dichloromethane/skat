#! /usr/bin/env bash

name=$1

file=$(echo "out${name}.log" | sed 's#/#_#g')

rm -f "$file"
./skat_client "$name" 2>"$file"
