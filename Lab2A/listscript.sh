#!/bin/bash
for iterations in 10 100 1000 10000 20000; do
    ./lab2_list --iterations=$iterations >> lab2_list.csv
done

for threads in 2 4 8 12; do
    for iterations in 1 10 100 1000; do
	./lab2_list --threads=$threads --iterations=$iterations >> lab2_list.csv 2>/dev/null
    done
done

for threads in 2 4 8 12; do
    for iterations in 1 2 4 8 16 32; do
	./lab2_list --threads=$threads --iterations=$iterations --yield=i >> lab2_list.csv 2>/dev/null
	./lab2_list --threads=$threads --iterations=$iterations --yield=d >> lab2_list.csv 2>/dev/null
	./lab2_list --threads=$threads --iterations=$iterations --yield=il >> lab2_list.csv 2>/dev/null
	./lab2_list --threads=$threads --iterations=$iterations --yield=dl >> lab2_list.csv 2>/dev/null
    done
done

for threads in 1 2 4 8 12 16 24; do
    for sync in m s; do
	./lab2_list --threads=$threads --iterations=1000 --sync=$sync >> lab2_list.csv
    done
done

for sync in m s; do
    ./lab2_list --threads=12 --iterations=32 --yield=i --sync=$sync >> lab2_list.csv
    ./lab2_list --threads=12 --iterations=32 --yield=d --sync=$sync >> lab2_list.csv
    ./lab2_list --threads=12 --iterations=32 --yield=il --sync=$sync >> lab2_list.csv
    ./lab2_list --threads=12 --iterations=32 --yield=dl --sync=$sync >> lab2_list.csv
done
