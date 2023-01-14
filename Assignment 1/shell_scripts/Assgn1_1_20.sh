#!/bin/bash

cat $1 | rev | sort -n > $2
num_lines=$(wc -l < $2)
declare -a arr
for ((i=1; i<=$num_lines; i++))
do
    arr[$i]=$(sed -n "$i"p $2)
done

curr=${arr[1]}
next=${arr[2]}

for ((i=2; i <= $num_lines; i++))
do

    num1=$curr
    num2=$next
    echo $i
    echo $num1
    echo $num2
    echo ""

    # finding the GCD

    if [ $curr -lt $next ]
    then 
        temp=$curr
        curr=$next
        next=$temp
    fi

    r=`expr $curr % $next`
    while [ $r -ne 0 ]
    do
        curr=$next
        next=$r
        r=`expr $curr % $next`
    done

    gcd=$next

    lcm=`expr $num1 \* $num2 / $gcd`

    curr=$lcm
    if [ $i -eq $num_lines ]
    then
        break
    fi
    next=${arr[`expr $i + 1`]}
done
echo $lcm



    



