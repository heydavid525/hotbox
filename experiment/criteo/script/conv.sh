#!/bin/bash
#data_path=/storage2/datasets/criteo
#output_path=/storage2/datasets/criteo_libsvm
data_path=/storage1/datasets/script/criteo
output_path=/storage1/datasets/script/criteo_libsvm
day=0
mkdir -p $output_path/day_$day/dict
gcc process_libsvm.c -DDAY0
./a.out $data_path/day_$day $output_path/day_$dict_day/dict/ $output_path/day_$day/
gcc process_libsvm.c
for ((day=1; day<=23;day++))
do
	dict_day=`expr $day - 1`
	mkdir -p $output_path/day_$day/dict
	./a.out $data_path/day_$day $output_path/day_$dict_day/dict/ $output_path/day_$day/
done
