#!/bin/bash

cd examples

echo Examples
echo ========
echo
for f in *.js; do
    echo $f
    echo =========
    echo
    echo '```'
    cat $f
    echo '```'
    echo
done
