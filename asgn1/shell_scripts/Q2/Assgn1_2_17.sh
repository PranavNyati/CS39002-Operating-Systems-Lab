for i in `cat $1|tr [A-Z] [a-z]`
do
    flag=0
    for j in `cat fruits.txt|tr [A-Z] [a-z]`
    do
        [[ "$i" == *"$j"* ]]&&flag=1
    done
    ((${#i} > 4&&${#i} < 21))&&!((flag))&&[[ "$i" =~ ^[a-z]+[0-9][a-z0-9]*$ ]]&&echo "Yes">> q2_output.txt|| echo "No">> q2_output.txt
done
