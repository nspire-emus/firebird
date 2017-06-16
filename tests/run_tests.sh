#!/bin/bash
set -eou pipefail

(cd ../headless && make -j8)

FIREBIRD_BIN=../headless/firebird-headless

echo "stop" | ${FIREBIRD_BIN} --debug-on-start --product 0xF0 --features 0x5 --rampayload bkpt.bin | grep "Software breakpoint at 10000004 (1234)" >/dev/null

