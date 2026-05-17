#!/bin/bash
cd ../build/
./ict/core/ict ../debug.scl
../ivm/iasm ../ivm/test.s test.bin
../ivm/ivm test.bin
