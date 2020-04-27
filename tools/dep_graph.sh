#! /usr/bin/env bash

print_graph=""
include_libs=""
source_dir="."
output="dep_out.png"

while getopts "pls:o:" name; do
  case "$name" in
  p)
    print_graph=1
    ;;
  l)
    include_libs=1
    ;;
  s)
    source_dir="$OPTARG/"	
  	;;
  o)
  	output="$OPTARG/"
  	;;
  *)
    exit 1
    ;;
  esac
done

{
  echo "digraph dep_graph {"

  {
    grep -H "#include \"" -- $( find $source_dir -type f -iregex '.*\.\(h\|c\)') | sed -n "s/\(.*\):#include \"\(.*\)\"/\1 -> \2/p"
    if test "x$include_libs" != "x"; then
      grep -H "#include <" -- $(find $source_dir -type f -iregex '.*\.\(h\|c\)') | sed -n "s/\(.*\):#include <\(.*\)>/\1 -> lib_\2/p"
    fi 
  } \
  | awk -F' ' '{ 
		   $1 = gensub(/^.*\/([^/]*)$/, "\\1", "g", $1);
		   print "\t", $1, $2, $3
		 }' \
  | sed "s/\.\|\//_/g"

  echo "}"
} | if test "x$print_graph" = "x"; then
  dot -T png -o $output_file
else
  cat
fi
