#!/bin/bash

for ((i = 1; 30 - $i ; i++))
	do 
	let "result = $((10000 + ($i * 2000)))"
	eval "echo $result"
	eval "time ./bw tests/text $result"
	eval "ls -all tests/text"
	eval "ls -all tests/text.bw"
	eval "echo "
done
