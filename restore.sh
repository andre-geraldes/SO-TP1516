#!/bin/bash

LOCAL=$PWD
DIR=/home/$USER/.Backup
FILE=$1

cd $DIR/metadata
fn=$(ls -l | grep $FILE | awk '{print $11}')
gunzip -f -k $fn
mv $fn
