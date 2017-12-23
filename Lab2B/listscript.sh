#!/bin/bash
>lab2b_list.csv
for sync in m s; do
    for threads in 1 2 4 8 12; do
	for lists in 1 4 8 16; do
	./lab2_list --threads=$threads --iterations=1000 --sync=$sync --lists=$lists >> lab2b_list.csv
	done
    done
done

for sync in m s; do
    for threads in 16 24; do
	./lab2_list --threads=$threads --iterations=1000 --sync=$sync >> lab2b_list.csv
    done
done

for threads in 1 4 8 12 16; do
    for iterations in 1 2 4 8 16; do
	./lab2_list --threads=$threads --iterations=$iterations --yield=id --lists=4 >> lab2b_list.csv 2>/dev/null
    done
done

for sync in m s; do
    for threads in 1 4 8 12 16; do
	for iterations in 10 20 40 80; do
	    ./lab2_list --threads=$threads --iterations=$iterations --yield=id --lists=4 --sync=$sync >> lab2b_list.csv
	done
    done
done

