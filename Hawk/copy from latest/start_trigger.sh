#!/bin/sh

i2cset -y -f 10 0x60 0x4F 0x00 0x01 i
i2cset -y -f 10 0x60 0x01 0x00 0x00 i