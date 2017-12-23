#!/bin/bash
for threads in 1 2 4 8 12; do
    for iterations in 10 20 40 80 100 1000 10000 100000; do
        ./lab2_add --threads=$threads --iterations=$iterations >> lab2_add.csv
        ./lab2_add --threads=$threads --iterations=$iterations --yield >> \
            lab2_add.csv
    done
done

for threads in 1 2 4 8 12; do
    for sync in m c s; do
        ./lab2_add --threads=$threads --iterations=10000 --sync=$sync >> \
            lab2_add.csv
        ./lab2_add --threads=$threads --iterations=10000 --sync=$sync \
            --yield >> lab2_add.csv
    done
done

