#!/bin/bash
if (( $1 > 0 && $2 > 0 && $3 > 0 ))
    then
        r=$(($2*$2));
        # echo $r;
        p=$(($r%$3));
        # echo "p=" $p;
        s=1
        while (( $p > 0))
        do
            s=$(($1*$s));
            p=$(($p-1))
        done
        # echo $s;
        res=$(($s%$3));
        echo $res
else
    echo "invalid input"
fi