#!/bin/bash
./cleanup.out
input_file=$1
out_file=$2
./part2.2.1.out	$input_file  $out_file &
writer_pid=$!

./part2.2.2.out	$input_file  $out_file &
reader1_pid=$!

./part2.2.3.out	$input_file  $out_file &
reader2_pid=$!


wait $writer_pid
wait $reader1_pid
wait $reader2_pid


./cleanup.out
