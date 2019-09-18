#!/bin/sh

DIR="`dirname "$0"`"
IN="$1"
INL="`echo "$IN" | tr A-Z a-z`"
OUT="`basename "$INL" .dff`.glr"

"$DIR/dff2glr" "$IN" > "$OUT"
