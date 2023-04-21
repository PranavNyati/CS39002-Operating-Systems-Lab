#!/bin/bash

# Redirected the output to a output_file instead of printing on terminal for better readabilty, delete it before running again

output_file="output_q5.txt"
for file in $1/*; do
    if [[ $file == *.py ]]; then
        echo "File Name: $file" >> "$output_file"
        echo "" >> "$output_file"
        lineno=1
        while read -r line; do

            # if the line contains a comment then print the line number in the file and the line
            if [[ $line == *\#* ]]; then
                if [[ $line != *\#*\"* ]] || [[ $line == *\#*\"*\"* ]]; then
                    echo "Line number: $lineno" >> "$output_file"
                    echo "$line" >> "$output_file"
                    echo "" >> "$output_file"
                fi
            fi

            # if its a multi line comment then print onlt the starting line number in the file and the entire comment
            if [[ $line == \"\"\"* ]]; then
                if [[ $line != \"\"\" ]] && [[ $line != \"\"\"\)* ]]; then
                    echo "Line number: $lineno" >> "$output_file"
                    num=1
                    while read -r line2; do
                        if [[ $num -ge $lineno ]]; then
                            echo "$line2" >> "$output_file"
                            if [[ $line2 == *\"\"\" ]]; then
                                break
                            fi
                        fi
                        num=$((num+1))
                    done < "$file"
                    echo "" >> "$output_file"
                fi
                
            fi
            lineno=$((lineno+1))
        done < "$file"
        echo $'\n' $'\n' '----------------------------------------------------------------------------------------------------------' $'\n' >> "$output_file"
    fi
    if [[ -d $file ]]; then
        bash Assgn1_5_17.sh $file >> "$output_file"
    fi
done
