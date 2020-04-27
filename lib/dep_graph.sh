#! /usr/bin/env bash

print_graph=""
include_libs=""

while getopts "pl" name; do
  case "$name" in
  p)
    print_graph=1
    ;;
  l)
    include_libs=1
    ;;
  *)
    exit 1
    ;;
  esac
done

{
  echo "digraph dep_graph {"
  grep -H "#include \"" -- *.c *.h | sed -n "s/\(.*\):#include \"\(.*\)\"/  \1 -> \2/p" | sed "s/\.\|\//_/g"
  if test "x$include_libs" != "x"; then
    grep -H "#include <" -- *.c *.h | sed -n "s/\(.*\):#include <\(.*\)>/  \1 -> lib_\2/p" | sed "s/\.\|\//_/g"
  fi
  echo "}"
} | if test "x$print_graph" = "x"; then
  dot -T png -o dep_graph.png
else
  cat
fi
