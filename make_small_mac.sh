#!/bin/bash

set -e

SNAP="CLinter_snapshot_small.zip"

zip -r "$SNAP" \
    CLinter \
    Tests \
    BUILDS/cmake_config \
    BUILDS/build_scripts \ 

echo "Created $SNAP"

