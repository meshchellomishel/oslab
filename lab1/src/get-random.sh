#!/bin/bash


for i in {1..150}
do
   od -N 1 -d -An /dev/random >> $1
done