#!/bin/bash

raspistill -t 1000 -o test.png --mode 2 -w 1920 -h 1080 -roi 0.2075,0.2075,0.585,0.585 -ss 10000 -ISO 100 -ev 0 -awb off -awbg 1,1
# raspistill -t 3000 -o test.jpg --mode 2 -w 1920 -h 1080 -roi 0.2075,0.2075,0.585,0.585
# raspistill -t 1000 -o test2.jpg -rot 180 --mode 1 -w 1920 -h 1080 -ss 20000 -ISO 200