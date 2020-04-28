#! /usr/bin/env bash

print_graph=""
include_libs=""
output_file="dep_graph.png"
source_dirs=()

while getopts "pls:o:" name; do
  case "$name" in
  p)
    print_graph=1
    ;;
  l)
    include_libs=1
    ;;
  s)
    source_dirs+=("$OPTARG")
    ;;
  o)
    output_file="$OPTARG"
    ;;
  *)
    exit 1
    ;;
  esac
done

# all hail bash array syntax. It is nothing less than art
if test "${#source_dirs[@]}" -eq 0; then
  source_dirs+=(.)
fi

{
  echo "digraph dep_graph {"

  for source_dir in "${source_dirs[@]}"; do
    {
      cd "$source_dir" || exit 1
      grep -rH --include="*.c" --include="*.h" "^#include \""
      if test "x$include_libs" != "x"; then
        grep -rH --include="*.c" --include="*.h" "^#include <"
      fi
    } | sed -n "s/\(.*\):#include [\"<]\(.*\)[\">]/\t\"\1\" -> \"\2\";/p"
  done

  echo "}"
} | if test "x$print_graph" = "x"; then
  dot -T png -o "$output_file"
else
  cat
fi