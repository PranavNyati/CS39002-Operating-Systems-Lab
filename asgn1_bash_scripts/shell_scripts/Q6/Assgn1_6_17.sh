#!/bin/bash
declare -i limit=1000000

numbers=($(seq 2 $limit))
spf=($(seq 2 $limit))

for ((i=2; i<=$limit; i+=1))
do
    spf[i-2]=0
done

for ((i=2; i<=$limit; i+=1))
do
    if [ ${spf[i-2]} -eq 0 ]
    then
        spf[i-2]=$i
        for ((j=i*i; j<=$limit; j+=i))
        do
            if [ ${spf[j-2]} -eq 0 ]
            then
              spf[j-2]=$i
            fi
        done
    fi
done

while read -r line;
do
  while [ $line -ne 1 ]
  do
    index=$(($line-2))
    echo -n "${spf[$index]} " >> "q6_output.txt"
    line=$(($line/${spf[$index]}))
  done
  echo "" >> "q6_output.txt"
done < "q6_input.txt"
