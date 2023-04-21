#!/bin/bash

# Helper Function for help option
Help(){
    
    printf "NAME \n"
    printf "\tCSV File Manipulator - A Expense Tracker for when you go out with your friends and choose to use a terminal based program for no apparent reason. (Splitwise FTW) \n"
    printf "\n"

    printf "SYNOPSIS \n"
    printf "\t./Assgn1_8_17.sh [OPTIONS] [RECORD] \n"
    printf "\t./Assgn1_8_17.sh [OPTIONS] \n"
    printf "\t./Assgn1_8_17.sh [RECORD] \n"
    printf "A record is defined as 4 arguments of the form : Date(dd-mm-yy) Category Amount Name "
    printf "\n"

    printf "DESCRIPTION \n"
    printf ""    

    printf "\t-c \n"
    printf "\t Accepts a category of expense and outputs the amount of money spent in that category. \n\n"
    printf "\t-n \n"
    printf "\t Accepts a name of a person and print the amount spent by that person. \n\n"
    printf "\t-s \n"
    printf "\t Accepts a column name of CSV Sort the csv by column name.\n"
    printf "\t Valid arguments are : {date, category, amount, name} \n\n"
    printf "\t-h \n"
    printf "\t Show  Help Prompt. \n\n"

    printf "AUTHOR \n"
    printf "\t Written by Vibhu.  \n"
    printf "\n"

    printf "REPORTING BUGS\n"
    printf "\t PLEASE DON'T (Any mistakes in this part are entirely the fault of Prerit Paliwal)\n"
    printf "\n"

    printf "COPYRIGHT\n"
    printf "\t Yup.\n"
    printf "\n"

    printf "SEE ALSO\n"
    printf "\t Other parts of this assignemnt :)\n"
    printf "\n"
}

# If you want the column headers in a new file, use this

# if [[ ! -e temp.csv ]]; then
#     echo "Date,Category,Amount,Name" > temp.csv
# fi

# Else, use this
touch main.csv

# There could be any number of arguments but the entry record would be the last 4 arguments
# A false positive could be when there are >= 4 args but they are all options. In that scenario, at least one of the last two args would have to begin with a hyphen "-"
if [[ $# -gt 3 ]]; then
    check1="{{@:(-2):1}:0:1}"
    check2="{{@:(-2):1}:0:1}"
    if [ "$check1" != "-" ] && [ "$check2" != "-" ] ; then
        record="${@: -4}"
        arr=($record)
        echo ${arr[0]},${arr[1]},${arr[2]},${arr[3]} >> main.csv
    fi
fi


sort -k1 -t, main.csv > temp.csv
mv temp.csv  main.csv

# Get the options and process them
# The colon after option initials represent that the option requires an additional argument.
while getopts "c:n:s:h" option; do
    case $option in 

    c) # Category
        category=$OPTARG
        sum=0
        # sed "/$category/!d" main.csv | cat
        while IFS=, read -r a b c d;
        do
            if [[ "$b" = "$category" ]]; then
                sum=`expr $sum + $c`
            fi
        done < main.csv 
            echo "$sum rupees spent on $category."
        ;;

    n) # Name
        name=$OPTARG
        sum=0
        while IFS=, read -r a b c d;
        do
            if [[ "$d" = "$name" ]]; then
                sum=`expr $sum + $c`
            fi
        done < main.csv
            echo "$sum rupees spent by $name."
        ;;

    s) # Sort by Column Name in $OPTARG
        col=$OPTARG

        # Sort the csv based on the date  in that entry
        if [[ "$col" = "date" ]]; then
            # sort -k1 -t, main.csv > temp.csv
            sort -n -t"-" -k3 -k2 -k1 main.csv > temp.csv
            mv temp.csv  main.csv

        # Sort the csv based on the name of category in that entry
        elif [[ "$col" = "category" ]]; then
            sort -k2 -t, main.csv > temp.csv
            mv temp.csv  main.csv
        
        # Sort the csv based on amount of money spent in that entry in descending order
        elif [[ "$col" = "amount" ]]; then
            sort -k3 -n -r -t, main.csv > temp.csv
            mv temp.csv  main.csv      

        # Sort the csv based on the name of person in that entry
        elif [[ "$col" = "name" ]]; then
            sort -k4 -t, main.csv > temp.csv
            mv temp.csv  main.csv 
        
        else
            echo "Invalid Column for Sorting! Terminating Script."
            exit

        fi

        ;;

    h) # Display Help
        Help
        ;;

    \?) # Invalid Option
        echo "Invalid Option! Terminating Script."
        exit
        ;;
    
    esac
done
shift "$(($OPTIND -1))"