#!/bin/bash
echo -n "Six x what: " 
read number 
echo "Running 6x$number."
for i in $(seq 1 $number); do seq 1 6 | rush ./a.out & disown; done
./a.out
