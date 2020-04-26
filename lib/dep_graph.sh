#! /usr/bin/env bash

print_gr=

while getopts "p" name; do
  case "$name" in
    p)
		print_gr=1
		;;
	*)
		exit 1
	 	;;
  esac
done

{
  echo "digraph dep_graph {"
  grep -H "#include\"" *.c *.h | sed -n 's/\(.*\):#include"\(.*\)"/  \1 -> \2/p' | sed 's/\./_/g'
  echo "}"
} | if test "x$print_gr" = "x"; then 
		dot -T png -o dep_graph.png
	else
	    cat
	fi
