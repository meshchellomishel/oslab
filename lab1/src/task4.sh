#!/bin/sh

res=0
count=0
for arg in "$@"
do
	count=$(( $count + 1 ))
	res=$(( $res + $arg ))
done

res=$(( $res / $count ))

echo "Count: $count"
echo "Res: $res"
