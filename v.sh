#!/bin/bash
while true; do
    fswebcam -r 160x120 --png 9 -S 2 - | convert - bmp:- | ./a.out - - | paplay
    test $? -gt 128 && break
done
