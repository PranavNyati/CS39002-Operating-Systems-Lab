#!/bin/bash

mkdir -p $2
attrs=("${@:3}")
for file in "$1"/*.jsonl; do
  csv_file="$2/$(basename "$file" .jsonl).csv"

  #create a new string which contains attributes separated by commas
  echo $(IFS=,; echo "${attrs[*]}")  > "$csv_file"
  
  while read -r line; do
    
    field_value=""
    for attr in "${attrs[@]}"; do
      # Extract the value for the current attribute from the JSON object
      cur_value=$(echo "$line" | jq -j --argjson value 128 ".$attr")
      # According to the rules for CSV: 
      # replacing any comma with it surronded by semicolons 
      # if double qoutes are encountered then make them as pair of double qoutes
      if [[ $cur_value == *\"* ]] || [[ $cur_value == *,* ]] || [[ $cur_value == *$'\n'* ]]; then
        cur_value=${cur_value//\"/\"\"}
        cur_value="\"$cur_value\""
      fi
      field_value="$field_value$cur_value,"
    #   echo $line
    done
    # to remove comma in the end
    field_value=${field_value::-1}
    echo "$field_value" >> "$csv_file"
  done < "$file"
done