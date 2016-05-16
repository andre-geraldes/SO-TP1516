#!/bin/bash

DIR=/home/$USER/.Backup
FILE=$1

gzip -f -k $FILE
fsha=$(sha1sum $FILE | awk '{print $1}')
mv $FILE.gz $DIR/data/$fsha
cd $DIR/metadata
ln -s -f ../data/$fsha $FILE
