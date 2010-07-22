#!/bin/sh
# Convert git log to GNU-style ChangeLog file.
# (C) Chris

git log --date-order --date=short | \
sed -e '/^commit.*$/d' | \
awk '/^Author/ {sub(/\\$/,""); getline t; print $0 t; next}; 1' | \
sed -e 's/^Author: //g' | \
sed -e 's/>Date:   \([0-9]*-[0-9]*-[0-9]*\)/>\t\1/g' | \
sed -e 's/^\(.*\) \(\)\t\(.*\)/\3    \1    \2/g' > ChangeLog
