#!/bin/bash -e

now=$(date +%s); echo $now;

if false; then
for k in a b c d e f g h i j k l m n o p q r s t u v w x y z; do 
    for i in $(grep -e "^$k" words_alpha.txt); do
        test -x /usr/bin/ispell || break
        echo $i | /usr/bin/ispell -P | grep -qe "^word: ok$" &&\
            echo "$i" >> words-alpha.valid-$k-$now.txt
    done &
done
fi

if false; then
for k in a b c d e f g h i j k l m n o p q r s t u v w x y z; do
    for i in $(cat words-alpha.valid-$k-*.txt); do
        if grep -qw "$i" wikitext.txt; then
            echo $i >> words-alpha.wiki-$k.txt
        fi
    done &
done
fi

if false; then
for k in a b c d e f g h i j k l m n o p q r s t u v w x y z; do
    cat words-alpha.wiki-$k.txt >> uk-wiki-words.txt
done
fi

for i in $(cat us-wiki-words.txt); do
    if ! grep -qw $i uk-wiki-words.txt; then
        echo $i >> us-wiki-words.new
    fi
done

