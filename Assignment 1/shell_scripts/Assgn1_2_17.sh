#!/bin/bash

file1="fruits.txt"
echo "" > $2
yes_count=0
no_count=0
for i in `cat < $1 | tr [A-Z] [a-z]`
do
    len=`expr length $i`
    # echo $i >> $2
    # echo $len >> $2
    if ( ((len >= 5 || len <= 20)) && [[ "$i" =~ ^[a-zA-Z]+[0-9][a-zA-Z0-9]*$ ]]) 
    then 
        flag=0
        for j in `cat < $file1 | tr [A-Z] [a-z]`
        do
            if [[ "$i" == *"$j"* ]]
            then
                # echo "$i contains $j" >> $2
                echo "No" >> $2
                no_count=`expr $no_count + 1`
                flag=1
                break
            fi
        done
        if (($flag == 0))
        then
            echo "Yes" >> $2
            yes_count=`expr $yes_count + 1`
        fi

    else
        echo "No" >> $2
        no_count=`expr $no_count + 1`
    fi
done
echo "Yes: $yes_count"
echo "No: $no_count"