gcd(){
    !(($1%$2))&&echo $2||gcd $2 $(($1%$2))
}
cat $1|rev>lcm_rev.txt
curr=$(head -1 lcm_rev.txt)
while read i
do
    curr=$(($curr*$(($i/$(gcd $curr $i) )) ))
done <lcm_rev.txt
echo $curr