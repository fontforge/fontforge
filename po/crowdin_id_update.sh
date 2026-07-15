#!/bin/bash

# Check if a file is provided as an argument
if [ "$#" -ne 1 ]; then
    echo "Usage: $0 <filename>"
    exit 1
fi

# Read the file line by line
while IFS= read -r line; do
    crowdin_id=`java -jar build/4.8.0/crowdin-cli.jar string list -c .crowdin.yml --plain --croql="identifier contains \"$line\""`
    echo $crowdin_id
    java -jar build/4.8.0/crowdin-cli.jar string edit $crowdin_id -c .crowdin.yml --text="$line"
done < "$1"
