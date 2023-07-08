#!/bin/bash

while read line
do
    if [[ $line =~ ^[0-9]+$ ]]
    then
        if [[ $line -lt $prev ]]
        then
            echo "Not sorted"
            exit 0
        fi
        prev=$line
    else
        echo "Not an integer"
        exit 0
    fi
done < $1
echo "Sorted"