awk '{major[$2]+=1}END{for(i in major) print i " " major[i] | "sort -k2,2r -k1,1"}' $1
echo 
awk '{stud[$1]+=1}END{for(i in stud){if(stud[i] > 1) print i; else count+=1}print count}' $1