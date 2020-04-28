#! /usr/bin/env bash

print_graph=""
include_libs=""
output_file="dep_graph.png"

while getopts "pls:o:" name; do
  case "$name" in
  p)
    print_graph=1
    ;;
  l)
    include_libs=1
    ;;
  s)
    source_dir+=("$OPTARG")
    ;;
  o)
    output_file="$OPTARG"
    ;;
  *)
    exit 1
    ;;
  esac
done

{
  echo "digraph dep_graph {"

  {
    grep -rH --include="*.c" --include="*.h" "^#include \"" "${source_dir[@]}"
    if test "x$include_libs" != "x"; then
      grep -rH --include="*.c" --include="*.h" "^#include <" "${source_dir[@]}"
    fi
  } | sed -n "s/\(.*\/\)\?\(.*\):#include [\"<]\(.*\)[\">]/\t\"\2\" -> \"\3\";/p"

  echo "}"
} | if test "x$print_graph" = "x"; then
  dot -T png -o "$output_file"
else
  cat
fi
