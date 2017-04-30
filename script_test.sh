#!/bin/bash

for ((i = 1; 11 - $i ; i++))
	do 
	let "result = $((0 + ($i * 500)))"
	eval "echo $result"
	eval "time ./bw tests/text $result"
	eval "ls -all tests/text"
	eval "ls -all tests/text.bw"
	eval "echo "
	eval "time ./ubw $result"
	eval "echo "
done
