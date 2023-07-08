names=$(sort ${1}/*)
[ -d "$2" ] && `rm -rf $2`
mkdir -p $2
touch $2/{a..z}.txt

for n in $names
do  
    echo $n >> $2/$(echo ${n:0:1} | tr '[:upper:]' '[:lower:]').txt
done