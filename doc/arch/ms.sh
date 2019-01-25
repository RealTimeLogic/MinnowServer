#!/bin/bash
cd `dirname "$0"`
make $1 -j4 -C esp-open-rtos/examples/ms/ FLASH_MODE=dio ESPPORT=/dev/ttyUSB0
