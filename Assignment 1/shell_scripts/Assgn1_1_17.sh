#!/bin/bash
cat $1 | rev | sort -n > $2
curr=$(sed -n "1"p $2)

for i in `seq 2 $(wc -l < $2)`
do
    next=$(sed -n "$i"p $2)
    num1=$curr
    num2=$next
    if [ $curr -lt $next ]
    then 
        curr=$next
        next=$num1
    fi
    r=`expr $curr % $next`
    while [ $r -ne 0 ]
    do
        curr=$next
        next=$r
        r=`expr $curr % $next`
    done
    curr=`expr $num1 \* $num2 / $next`
done
echo $curr



    



