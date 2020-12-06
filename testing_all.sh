#!/bin/bash

echo -ne "\nSearching using the ordinary algorithm\n\n"


st1=$(date +%s.%N)

sleep 1

(./a.out $@)

duration1=$(echo "$(date +%s.%N) - $st1" | bc)
exec_time1=`printf "%.2f seconds" $duration1`

echo -ne "\n"
sleep 1
echo "Execution Time: $exec_time1"

sleep 2

echo -ne "\n\nSearching using Dots' algorithm\n\n"

st2=$(date +%s.%N)

sleep 1

(./a.out $@)


duration2=$(echo "$(date +%s.%N) - $st2" | bc)
exec_time2=`printf "%.2f seconds" $duration2`

echo -ne "\n"
sleep 2
echo "Execution Time: $exec_time2"
