#!/bin/bash

set -e

make clean
make -j

for f in $(find . -name "*"); do
    if [[ -x $f && ! -d $f && $(basename $f) != 'test_all.sh' ]]; then
        echo "========== Running $f =========="
        HCLIB_HPT_FILE=hpt0.xml $f
        echo
    fi
done
