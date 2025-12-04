#!/bin/sh

if [ ! -p input ] ; then
	mkfifo input
fi

echo "" > input & # hack: if this is left out, the sim skips the first input

xmake && xmake run < input
