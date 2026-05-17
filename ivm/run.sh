#!/bin/bash
set -e # exit on error

IVM_DIR=$(dirname $(realpath "$0"))
BUILD_DIR=$IVM_DIR/../build
ICT=$BUILD_DIR/ict/core/ict
IASM=$IVM_DIR/bin/iasm
IVM=$IVM_DIR/bin/ivm
PROGRAM=$1
shift 1

rm -f $IVM_DIR/program.s
$ICT -I$IVM_DIR/inc $PROGRAM -o$IVM_DIR/program.s $@     # compile

if [[ -f $IVM_DIR/program.s ]]; then
    $IASM $IVM_DIR/link.s $IVM_DIR/program.bin      # assemble/"link"
    $IVM -c -m $IVM_DIR/program.bin                          # run
else
    echo "No program was generated, doing nothing"
fi
