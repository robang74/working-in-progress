#!/bin/bash -e

filename=$1
test -r $filename || exit 1
aspell -c --lang=en $filename | while IFS= read -r line; do
  if [[ "$line" =~ ^R\n ]]; then
    # Extract the word to be deleted
    word="${line:2}"
    # Replace all occurrences of the word in the file with an empty string
    sed -i '' "s/\b${word}\b//g" $filename
  fi
done
